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

VOID TaskSampleEntry2(VOID)
{
    while(1) {
        printf("TaskSampleEntry2 running...\n\r");
        LOS_TaskDelay(1000);
    }
}


VOID TaskSampleEntry1(VOID)
{
    while(1) {
        printf("TaskSampleEntry1 running...\n\r");
        LOS_TaskDelay(1000);
    }

}

UINT32 TaskSample(VOID)
{
    UINT32  ret;
    UINT32  taskID1, taskID2;
    TSK_INIT_PARAM_S task1 = { 0 };
    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry1;
    task1.uwStackSize  = 0x1000;
    task1.pcName       = "TaskSampleEntry1";
    task1.usTaskPrio   = 6;
    ret = LOS_TaskCreate(&taskID1, &task1);

    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry2;
    task1.uwStackSize  = 0x1000;
    task1.pcName       = "TaskSampleEntry2";
    task1.usTaskPrio   = 7;
    ret = LOS_TaskCreate(&taskID2, &task1);

    return ret;
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

    UartInit();

    PRINTK("\n OHOS start \n\r");

    ret = LOS_KernelInit();
    if (ret != LOS_OK) {
        goto START_FAILED;
    }

    TaskSample();

    PRINTK("\n OHOS scheduler!!! \n\r");

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
