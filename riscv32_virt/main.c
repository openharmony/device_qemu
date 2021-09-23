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

#include "los_tick.h"
#include "los_task.h"
#include "los_config.h"
#include "los_interrupt.h"
#include "los_debug.h"
#include "uart.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

UINT32 LosAppInit(VOID);

UINT32 QemuCLZ(UINT32 data)
{
    UINT32 count = 32; /* 32-bit data length */
    if (data == 0) {
        return count;
    }

    if (data & 0xFFFF0000) {
        data = data >> 16; /* 16-bit data length */
        count -= 16; /* 16-bit data length */
    }

    if (data & 0xFF00) {
        data = data >> 8; /* 8-bit data length */
        count -= 8; /* 8-bit data length */
    }

    if (data & 0xF0) {
        data = data >> 4; /* 4-bit data length */
        count -= 4; /* 4-bit data length */
    }

    if (data & 0x8) {
        return (count - 4); /* 4-bit data length */
    } else if (data & 0x4) {
        return (count - 3); /* 3-bit data length */
    } else if (data & 0x2) {
        return (count - 2); /* 2-bit data length */
    } else if (data & 0x1) {
        return (count - 1);
    }

    return 0;
}

/*****************************************************************************
 Function    : main
 Description : Main function entry
 Input       : None
 Output      : None
 Return      : None
 *****************************************************************************/
LITE_OS_SEC_TEXT_INIT INT32 main(VOID)
{
    UINT32 ret;

    printf("\n OHOS start \n\r");

    ret = LOS_KernelInit();
    if (ret != LOS_OK) {
        printf("Liteos kernel init failed! ERROR: 0x%x\n", ret);
        goto START_FAILED;
    }

    ret = LosAppInit();
    if (ret != LOS_OK) {
        printf("LosAppInit failed! ERROR: 0x%x\n", ret);
    }

    LOS_Start();

START_FAILED:
    while (1) {
        __asm volatile("wfi");
    }
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
