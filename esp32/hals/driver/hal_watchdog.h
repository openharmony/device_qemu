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

#ifndef _HAL_WATCHDOG_H
#define _HAL_WATCHDOG_H

#include "los_compiler.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define WDT_WRWITE_MASK                0x50d83aa1
#define RTC_CNTL                       0x3ff48000
#define RTC_WDT_PROTECT                0x3ff480a4
#define RTC_WDT_CFG0                   0x3ff4808c
#define RTC_WDT_FEED                   0x3ff480a0
#define TIMER_GROUP0                   0x3ff5F000
#define WDT_TIMER_GROUP0_CNTL          0x3ff5F048
#define TIMER_GROUP1                   0x3ff60000
#define WDT_TIMER_GROUP1_CNTL          0x3ff60048
#define RTC_CNTL_WDT_FEED              (31)
#define RTC_CNTL_WDT_EN                (31)
#define RTC_CNTL_WDT_STG0              (28)
#define RTC_CNTL_WDT_FLASHBOOT_MOD_EN  (10)
#define RTC_CNTL_WDT_INT               (1)
#define RTC_CNTL_WDT_CPU_RESET         (2)
#define RTC_CNTL_WDT_SYS_RESET         (3)
#define RTC_CNTL_WDT_RTC_RESET         (4)
#define RTC_CNTL_WDT_SYS_RESET_MASK    (7)
#define RTC_CNTL_WDT_SYS_RESET_LENGTH  (11)

#define REG32_READ(reg)                (*(volatile UINT32*) (reg))
#define REG32_WRITE(reg, value)        (*(volatile UINT32*) (reg) &= value)

VOID WdtEnable(VOID);
VOID WdtDisable(VOID);
VOID WdtFeed(VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* _HAL_WATCHDOG_H */
