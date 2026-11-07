#include "main.h"
#include "runner.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdarg.h>

/* The state, I2C, flash and stdio remain single-owner-at-a-time. */
static SemaphoreHandle_t state_lock, print_lock;
static uint32_t sensor_lock_wait_max_ms, sensor_lock_waits;
static TaskHandle_t sensor_handle, service_handle;
volatile uint32_t mg_rtos_started;

void mg_state_lock(void)
{
  if (xSemaphoreTake(state_lock, portMAX_DELAY) != pdTRUE) Error_Handler();
#ifdef MG_MUTEX_TRACE
  GPIOA->BSRR = GPIO_PIN_9;
#endif
}
static void sensor(void *arg)
{
  (void)arg;
  TickType_t release = xTaskGetTickCount();
  mg_rtos_started = 1;
  for (;;) {
    uint32_t start = HAL_GetTick();
    mg_state_lock();
    uint32_t waited = HAL_GetTick() - start;
    if (waited) ++sensor_lock_waits;
    if (waited > sensor_lock_wait_max_ms) sensor_lock_wait_max_ms = waited;
    runner_sensor_step();
    mg_state_unlock();
    if (xTaskDelayUntil(&release, pdMS_TO_TICKS(20)) == pdFALSE)
      release = xTaskGetTickCount();
  }
}
static void service(void *arg)
{
  (void)arg;
  for (;;) {
    mg_state_lock();
    runner_service_step();
    mg_state_unlock();
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}
void mg_state_unlock(void)
{
#ifdef MG_MUTEX_TRACE
  GPIOA->BRR = GPIO_PIN_9;
#endif
  if (xSemaphoreGive(state_lock) != pdTRUE) Error_Handler();
}
int mg_rtos_printf(const char *format, ...)
{
  va_list args; int result;
  va_start(args, format);
  if (!mg_rtos_started) {
    result = vprintf(format, args);
  } else {
    /* Only service may release state here. Sensor-side I2C fault diagnostics
       keep the bus protected until the current transaction has unwound. */
    BaseType_t service_caller = xTaskGetCurrentTaskHandle() == service_handle;
#ifndef MG_MUTEX_WIDE
    if (service_caller) mg_state_unlock();
#else
    (void)service_caller;
#endif
    if (xSemaphoreTake(print_lock, portMAX_DELAY) != pdTRUE) Error_Handler();
    result = vprintf(format, args);
    if (xSemaphoreGive(print_lock) != pdTRUE) Error_Handler();
#ifndef MG_MUTEX_WIDE
    if (service_caller) mg_state_lock();
#endif
  }
  va_end(args);
  return result;
}
void mg_rtos_report(void)
{
  mg_rtos_printf("RTOS free=%lu min_free=%lu stack_sensor_words=%lu stack_service_words=%lu stack_idle_words=%lu sensor_lock_waits=%lu sensor_wait_max_ms=%lu\r\n",
         (unsigned long)xPortGetFreeHeapSize(), (unsigned long)xPortGetMinimumEverFreeHeapSize(),
         (unsigned long)uxTaskGetStackHighWaterMark(sensor_handle),
         (unsigned long)uxTaskGetStackHighWaterMark(service_handle),
         (unsigned long)uxTaskGetStackHighWaterMark(xTaskGetIdleTaskHandle()),
         (unsigned long)sensor_lock_waits, (unsigned long)sensor_lock_wait_max_ms);
}
void mg_rtos_start(void)
{
#ifdef MG_MUTEX_TRACE
  GPIO_InitTypeDef probe = {0};
  probe.Pin = GPIO_PIN_9; probe.Mode = GPIO_MODE_OUTPUT_PP; probe.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &probe); GPIOA->BRR = GPIO_PIN_9;
#endif
  state_lock = xSemaphoreCreateMutex();
  print_lock = xSemaphoreCreateMutex();
  if (!state_lock || !print_lock || xTaskCreate(sensor,"sensor",384,NULL,3,&sensor_handle)!=pdPASS ||
      xTaskCreate(service,"service",640,NULL,2,&service_handle)!=pdPASS) Error_Handler();
  vTaskStartScheduler();
  Error_Handler();
}
void vApplicationMallocFailedHook(void) { Error_Handler(); }
void vApplicationStackOverflowHook(TaskHandle_t task,char *name)
{
  (void)task; (void)name; Error_Handler();
}
