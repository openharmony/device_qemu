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

#include "los_task.h"
#include "los_debug.h"

#define TASK_DELAY_TICKS     1000
#define TASK_STACK_SIZE      0x1000
#define TASK1_PRI            6
#define TASK2_PRI            7

static void TaskSampleEntry2(void)
{
    while (1) {
        printf("TaskSampleEntry2 running...\n\r");
        LOS_TaskDelay(TASK_DELAY_TICKS);
    }
}


static void TaskSampleEntry1(void)
{
    while (1) {
        printf("TaskSampleEntry1 running...\n\r");
        LOS_TaskDelay(TASK_DELAY_TICKS);
    }
}

unsigned int LosAppInit(VOID)
{
    unsigned int ret;
    unsigned int taskID1, taskID2;
    TSK_INIT_PARAM_S task1 = { 0 };
    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry1;
    task1.uwStackSize  = TASK_STACK_SIZE;
    task1.pcName       = "TaskSampleEntry1";
    task1.usTaskPrio   = TASK1_PRI;
    ret = LOS_TaskCreate(&taskID1, &task1);
    if (ret != LOS_OK) {
        printf("Create Task failed! ERROR: %d\n", ret);
        return ret;
    }

    task1.pfnTaskEntry = (TSK_ENTRY_FUNC)TaskSampleEntry2;
    task1.uwStackSize  = TASK_STACK_SIZE;
    task1.pcName       = "TaskSampleEntry2";
    task1.usTaskPrio   = TASK2_PRI;
    ret = LOS_TaskCreate(&taskID2, &task1);
    if (ret != LOS_OK) {
        printf("Create Task failed! ERROR: %d\n", ret);
    }

    return ret;
}
