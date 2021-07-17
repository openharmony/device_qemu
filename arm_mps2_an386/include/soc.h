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

#ifndef _ST_CONFIG_H
#define _ST_CONFIG_H

#define IRQn_Type int

#define __CM4_REV              0x0001
#define __NVIC_PRIO_BITS       4
#define __MPU_PRESENT          1
#define __Vendor_SysTickConfig 0
#define __FPU_PRESENT          1

#define SysTick_IRQn           (-1)
#define PendSV_IRQn            (-2)
#define NonMaskableInt_IRQn    (-14)
#define MemoryManagement_IRQn  (-12)
#define BusFault_IRQn          (-11)
#define UsageFault_IRQn        (-10)
#define SVCall_IRQn            (-5)

#define SYSCLK_FREQ    25000000

#define UART0_BASE     0x40004000
#define UART1_BASE     0x40005000
#define UART2_BASE     0x40006000
#define UART3_BASE     0x40007000
#define UART4_BASE     0x40009000

#define UART0_CLK_FREQ SYSCLK_FREQ
#define UART0_BAUDRAT  115200

#define WATCHDOG_BASE  0x40008000
#define GPIO_BASE(no)  (0x40010000 + (unsigned int)(no) * 0x1000) /* no: 0 ~ 3 */
#define TIMER_BASE(no) (0x40000000 + (unsigned int)(no) * 0x1000)

#endif

#include "core_cm4.h"
