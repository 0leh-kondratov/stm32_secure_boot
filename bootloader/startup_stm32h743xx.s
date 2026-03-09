/*
 * Minimal startup for STM32H743 (Cortex-M7).
 * Sets stack pointer and branches to main.
 */
.syntax unified
.cpu cortex-m7
.fpu softvfp
.thumb

.global g_pfnVectors
.global Default_Handler
.global Reset_Handler

.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    .rept 240
    .word 0
    .endr

.thumb_func
Reset_Handler:
    ldr r0, =_estack
    msr msp, r0

    /* LED1 (PB0) on immediately: RCC_AHB4ENR |= GPIOB, GPIOB MODER/BSRR */
    ldr r0, =0x58024400
    ldr r1, [r0, #0xE0]
    orr r1, r1, #(1 << 1)
    str r1, [r0, #0xE0]
    ldr r0, =0x58020400
    ldr r1, [r0, #0]
    bic r1, r1, #3
    orr r1, r1, #1
    str r1, [r0, #0]
    mov r1, #1
    str r1, [r0, #0x18]

    /* Delay ~1 s so LED1 is visible after any reset (SP and regs only, no RAM) */
    movw r2, #0
    movt r2, #0x00F0
1:  subs r2, r2, #1
    bne 1b

    ldr r0, =_sbss
    ldr r1, =_ebss
    mov r2, #0
    b 2f
1:  str r2, [r0]
    adds r0, r0, #4
2:  cmp r0, r1
    bcc 1b

    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    b 4f
3:  ldr r3, [r2]
    str r3, [r0]
    adds r0, r0, #4
    adds r2, r2, #4
4:  cmp r0, r1
    bcc 3b

    bl main
    b .

.thumb_func
Default_Handler:
NMI_Handler:
HardFault_Handler:
MemManage_Handler:
BusFault_Handler:
UsageFault_Handler:
SVC_Handler:
DebugMon_Handler:
PendSV_Handler:
SysTick_Handler:
    b .

.end
