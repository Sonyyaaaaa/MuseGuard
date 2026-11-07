#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H
#include <stdint.h>
extern uint32_t SystemCoreClock;
void Error_Handler(void);
#define configUSE_PREEMPTION 1
#define configUSE_IDLE_HOOK 0
#define configUSE_TICK_HOOK 0
#define configCPU_CLOCK_HZ (SystemCoreClock)
#define configTICK_RATE_HZ 1000
#define configMAX_PRIORITIES 4
#define configMINIMAL_STACK_SIZE 128
#define configTOTAL_HEAP_SIZE 6144
#define configMAX_TASK_NAME_LEN 12
#define configUSE_16_BIT_TICKS 0
#define configIDLE_SHOULD_YIELD 1
#define configUSE_MUTEXES 1
#define configUSE_TIMERS 0
#define configUSE_TRACE_FACILITY 0
#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK 1
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configSUPPORT_STATIC_ALLOCATION 0
#define configUSE_TASK_NOTIFICATIONS 1
#define INCLUDE_vTaskDelay 1
#define INCLUDE_xTaskDelayUntil 1
#define INCLUDE_uxTaskGetStackHighWaterMark 1
#define INCLUDE_xTaskGetIdleTaskHandle 1
#define INCLUDE_xTaskGetCurrentTaskHandle 1
#define configASSERT(x) do { if (!(x)) Error_Handler(); } while (0)
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#endif
