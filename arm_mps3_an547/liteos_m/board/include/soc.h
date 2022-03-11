/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd. All rights reserved.
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

#ifndef _SOC_H
#define _SOC_H

#define IRQn_Type              int

#define __CM55_REV             0x0001
#define __NVIC_PRIO_BITS       3
#define __Vendor_SysTickConfig 0
#define __VTOR_PRESET          1
#define __MPU_PRESENT          1
#define __FPU_PRESENT          1
#define __DSP_PRESENT          1
#define ARM_MATH_HELIUM

#define SysTick_IRQn           (-1)
#define PendSV_IRQn            (-2)
#define NonMaskableInt_IRQn    (-14)
#define MemoryManagement_IRQn  (-12)
#define BusFault_IRQn          (-11)
#define UsageFault_IRQn        (-10)
#define SVCall_IRQn            (-5)

#define UART0_RX_IRQn          43

#define SYSCLK_FREQ    25000000

#define UART0_BASE     0x49303000
#define UART1_BASE     0x49304000
#define UART2_BASE     0x49305000

#define UART0_CLK_FREQ SYSCLK_FREQ
#define UART0_BAUDRAT  115200

#include "core_cm55.h"

#endif

