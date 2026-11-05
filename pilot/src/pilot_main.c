/* Minimal scheduler pilot, not MuseGuard application. */
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>

static UART_HandleTypeDef uart;
static TaskHandle_t event_task, periodic_task, load_task;
static volatile uint32_t events;
static volatile uint32_t started;
extern void xPortSysTickHandler(void);

void pilot_fail(void) { __disable_irq(); for (;;) { GPIOA->BSRR = GPIO_PIN_5; } }
void Error_Handler(void) { pilot_fail(); }
void vApplicationMallocFailedHook(void) { pilot_fail(); }
void vApplicationStackOverflowHook(TaskHandle_t task, char *name) { (void)task; (void)name; pilot_fail(); }
void SysTick_Handler(void) {
    HAL_IncTick();
    if (started) {
        static uint32_t count;
        xPortSysTickHandler();
        if (++count == 500) {
            BaseType_t woken = pdFALSE;
            count = 0;
            GPIOA->BSRR = GPIO_PIN_4;
            vTaskNotifyGiveFromISR(event_task, &woken);
            portYIELD_FROM_ISR(woken);
        }
    }
}
static void send(const char *s) {
    uint16_t n = 0; while (s[n]) ++n;
    if (HAL_UART_Transmit(&uart, (uint8_t *)s, n, 100) != HAL_OK) pilot_fail();
}
static void number(uint32_t n) {
    char b[11]; unsigned i = sizeof b; b[--i] = 0;
    do { b[--i] = (char)('0' + n % 10); n /= 10; } while (n);
    send(&b[i]);
}
static void event_fn(void *arg) {
    (void)arg;
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        GPIOA->BRR = GPIO_PIN_7; /* Lower-priority load stopped making progress. */
        GPIOA->BSRR = GPIO_PIN_6;
        ++events;
        GPIOA->BRR = GPIO_PIN_4 | GPIO_PIN_6;
    }
}
static void periodic_fn(void *arg) {
    (void)arg; TickType_t last = xTaskGetTickCount();
    started = 1;
    send("PILOT FreeRTOS V10.6.2 tick=1ms\r\n");
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(1000));
        send("P hal="); number(HAL_GetTick()); send(" rtos="); number(xTaskGetTickCount());
        send(" ev="); number(events); send(" free="); number(xPortGetFreeHeapSize());
        send(" minfree="); number(xPortGetMinimumEverFreeHeapSize());
        send(" stacks_words="); number(uxTaskGetStackHighWaterMark(periodic_task));
        send(","); number(uxTaskGetStackHighWaterMark(event_task));
        send(","); number(uxTaskGetStackHighWaterMark(load_task));
        send(","); number(uxTaskGetStackHighWaterMark(xTaskGetIdleTaskHandle())); send("\r\n");
    }
}
static void load_fn(void *arg) { (void)arg; for (;;) { GPIOA->BSRR = GPIO_PIN_7; __asm volatile ("nop"); } }
int main(void) {
    GPIO_InitTypeDef pin = {0};
    HAL_Init();
    /* Match the Lab 3 HSI clock setup; HAL_RCC_ClockConfig updates SysTick. */
    RCC_OscInitTypeDef osc = {0}; RCC_ClkInitTypeDef clocks = {0};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSI; osc.HSIState = RCC_HSI_ON;
    osc.HSIDiv = RCC_HSI_DIV1; osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) pilot_fail();
    clocks.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
    clocks.SYSCLKSource = RCC_SYSCLKSOURCE_HSI; clocks.SYSCLKDivider = RCC_SYSCLK_DIV1;
    clocks.AHBCLKDivider = RCC_HCLK_DIV1; clocks.APB1CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_1) != HAL_OK) pilot_fail();
    __HAL_RCC_GPIOA_CLK_ENABLE(); __HAL_RCC_USART2_CLK_ENABLE();
    pin.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7; pin.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &pin);
    pin.Pin = GPIO_PIN_2 | GPIO_PIN_3; pin.Mode = GPIO_MODE_AF_PP; pin.Alternate = GPIO_AF1_USART2;
    HAL_GPIO_Init(GPIOA, &pin);
    uart.Instance = USART2; uart.Init.BaudRate = 115200; uart.Init.WordLength = UART_WORDLENGTH_8B;
    uart.Init.StopBits = UART_STOPBITS_1; uart.Init.Parity = UART_PARITY_NONE; uart.Init.Mode = UART_MODE_TX_RX;
    uart.Init.HwFlowCtl = UART_HWCONTROL_NONE; uart.Init.OverSampling = UART_OVERSAMPLING_16;
    uart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE; uart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    if (HAL_UART_Init(&uart) != HAL_OK) pilot_fail();
    if (xTaskCreate(event_fn,"event",128,NULL,3,&event_task) != pdPASS ||
        xTaskCreate(periodic_fn,"periodic",256,NULL,2,&periodic_task) != pdPASS ||
        xTaskCreate(load_fn,"load",128,NULL,1,&load_task) != pdPASS) pilot_fail();
    vTaskStartScheduler(); pilot_fail();
}
