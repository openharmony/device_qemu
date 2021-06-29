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

#include "mtimer.h"
#include "los_reg.h"
#include "los_interrupt.h"
#include "los_tick.h"
#include "riscv_hal.h"
#include "soc.h"
#include "plic.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

UINT32 MTimerTickInit(OS_TICK_HANDLER handler, UINT32 period)
{
    unsigned int ret;

    ret = HalHwiCreate(RISCV_MACH_TIMER_IRQ, 0x1, 0, (HWI_PROC_FUNC)handler, period);
    if (ret != LOS_OK) {
        return ret;
    }

    WRITE_UINT32(0xffffffff, MTIMERCMP + 4);
    WRITE_UINT32(period, MTIMERCMP);
    WRITE_UINT32(0x0, MTIMERCMP + 4);

    HalIrqEnable(RISCV_MACH_TIMER_IRQ);
    return LOS_OK;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
