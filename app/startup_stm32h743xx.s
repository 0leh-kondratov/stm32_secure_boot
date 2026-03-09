/*
 * Minimal startup for Application, STM32H743 (Cortex-M7).
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
    /* LED2 (PE1) on immediately so we see we entered the app (bootloader uses LED1) */
    ldr r0, =0x58024400
    ldr r1, [r0, #0xE0]
    orr r1, r1, #(1 << 4)
    str r1, [r0, #0xE0]
    ldr r0, =0x58021000
    ldr r1, [r0, #0]
    bic r1, r1, #(3 << 2)
    orr r1, r1, #(1 << 2)
    str r1, [r0, #0]
    mov r1, #(1 << 1)
    str r1, [r0, #0x18]

    ldr r0, =_sbss
    ldr r1, =_ebss
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
DebugMon_Handler:
    b .

.weak SVC_Handler
.weak PendSV_Handler
.weak SysTick_Handler
.thumb_func
SVC_Handler:
PendSV_Handler:
SysTick_Handler:
    b .

.end
