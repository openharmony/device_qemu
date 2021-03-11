/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "soc.h"

.macro PUSH_CALLER_REG
    addi  sp, sp, -(INT_SIZE_ON_STACK)
    SREG  t6, 16 * REGBYTES(sp)
    SREG  t5, 17 * REGBYTES(sp)
    SREG  t4, 18 * REGBYTES(sp)
    SREG  t3, 19 * REGBYTES(sp)
    SREG  a7, 20 * REGBYTES(sp)
    SREG  a6, 21 * REGBYTES(sp)
    SREG  a5, 22 * REGBYTES(sp)
    SREG  a4, 23 * REGBYTES(sp)
    SREG  a3, 24 * REGBYTES(sp)
    SREG  a2, 25 * REGBYTES(sp)
    SREG  a1, 26 * REGBYTES(sp)
    SREG  a0, 27 * REGBYTES(sp)
    SREG  t2, 28 * REGBYTES(sp)
    SREG  t1, 29 * REGBYTES(sp)
    SREG  t0, 30 * REGBYTES(sp)
    SREG  ra, 31 * REGBYTES(sp)
.endm

.extern memset
.global _start
.section .start.text
.align 4
_start:
    li    t0, RISCV_MSTATUS_MPP
    csrw  mstatus, t0
    csrw  mie, zero
    la    t0, HalTrapVector
    csrw  mtvec, t0      # direct mode

    .option push
    .option norelax
    la    gp, __global_pointer$
    .option pop

    la    t0, __bss_start
    la    t1, __bss_end
2:
    sw    zero, 0x0(t0)
    addi  t0, t0, 0x4
    bgtu  t1, t0, 2b

    la    sp, __start_and_irq_stack_top

    tail  main
