/*
 * FreeRTOS configuration for step1 (NUCLEO-H743ZI, HAL, 400 MHz).
 * SysTick drives the RTOS tick; HAL_IncTick() is called from vApplicationTickHook.
 * When LWIP_BUILD is defined (app/lwip), use CMSIS-RTOS2 compatible config.
 */
#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#ifdef LWIP_BUILD
#include "FreeRTOSConfig_lwip.h"
#else

#if defined(__GNUC__)
#include <stdint.h>
extern uint32_t SystemCoreClock;
#endif

#define configENABLE_FPU                         1
#define configENABLE_MPU                         0

#define configUSE_PREEMPTION                     1
#define configSUPPORT_STATIC_ALLOCATION          0
#define configSUPPORT_DYNAMIC_ALLOCATION         1
#define configUSE_IDLE_HOOK                      0
#define configUSE_TICK_HOOK                      1
#define configCPU_CLOCK_HZ                       (SystemCoreClock)
#define configTICK_RATE_HZ                       ((TickType_t)1000)
#define configMAX_PRIORITIES                     (8)
#define configUSE_MINI_LIST_ITEM                 1
#define configMINIMAL_STACK_SIZE                 ((uint16_t)128)
#define configTOTAL_HEAP_SIZE                    ((size_t)(12 * 1024))
#define configMAX_TASK_NAME_LEN                  (16)
#define configUSE_16_BIT_TICKS                   0
#define configIDLE_SHOULD_YIELD                  1
#define configUSE_MUTEXES                        0
#define configUSE_TIMERS                         0
#define configUSE_TRACE_FACILITY                 0
#define configUSE_PORT_OPTIMISED_TASK_SELECTION  0

#define configPRIO_BITS                         4
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY  15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5
#define configKERNEL_INTERRUPT_PRIORITY          (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))
#define configMAX_SYSCALL_INTERRUPT_PRIORITY     (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS))

#define configASSERT(x) do { if (!(x)) { for (;;); } } while (0)

#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler

#define INCLUDE_vTaskDelay      1

#endif /* !LWIP_BUILD */

#endif /* FREERTOS_CONFIG_H */
