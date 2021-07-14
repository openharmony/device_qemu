/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

   .syntax unified
  .cpu cortex-m4
  .fpu softvfp
  .thumb

.extern HalExcNMI;
.extern HalExcHardFault;
.extern HalExcMemFault;
.extern HalExcBusFault;
.extern HalExcUsageFault;
.extern HalExcSvcCall;
.extern HalPendSV;
.extern SysTick_Handler;
.global Vector_Table
.global Reset_Handler

.section .text
.type  Reset_Handler, %function
Reset_Handler:
    ldr  r0, =__bss_start
    ldr  r1, =__bss_end
    mov  r2, #0

bss_loop:
    str  r2, [r0, #0]
    add  r0, r0, #4
    subs r3, r1, r0
    bne  bss_loop

    ldr  sp, =__irq_stack_top
    b    main
.size  Reset_Handler, .-Reset_Handler

.type  DefaultHandler, %function
DefaultHandler:
loop:
  b  loop
.size  DefaultHandler, .-DefaultHandler

.section .Vector_Table,"a",%progbits
.type  Vector_Table, %object
Vector_Table:
    .word _estack
    .word Reset_Handler
    .word HalExcNMI
    .word HalExcHardFault
    .word HalExcMemFault
    .word HalExcBusFault
    .word HalExcUsageFault
    .word 0
    .word 0
    .word 0
    .word 0
    .word DefaultHandler
    .word DefaultHandler
    .word 0
    .word HalPendSV
    .word SysTick_Handler

.size  Vector_Table, .-Vector_Table

