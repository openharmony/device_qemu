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

#include "fs_init.h"
#include "los_task.h"
#include "securec.h"
#include "hal_littlefs.h"

#define FS_INIT_TASK_SIZE     0x1000
#define FS_INIT_TASK_PRIORITY 2

static uint32_t g_taskId;

static void FileSystemEntry(void)
{
    uint32_t ret;
    struct lfs_config *littlefsConfig = LittlefsConfigGet();
    ret = LittlefsInit(0, littlefsConfig);
    if (ret != LOS_OK) {
        printf("Littlefs init failed.\n");
    }
    printf("Littlefs inited.\n");
}

void FileSystemInit(void)
{
    uint32_t ret;
    TSK_INIT_PARAM_S taskInitParam;

    ret = memset_s(&taskInitParam, sizeof(TSK_INIT_PARAM_S), 0, sizeof(TSK_INIT_PARAM_S));
    if (ret != EOK) {
        return;
    }

    taskInitParam.usTaskPrio = FS_INIT_TASK_PRIORITY;
    taskInitParam.pcName = "FileSystemTask";
    taskInitParam.pfnTaskEntry = (TSK_ENTRY_FUNC)FileSystemEntry;
    taskInitParam.uwStackSize = FS_INIT_TASK_SIZE;
    ret = LOS_TaskCreate(&g_taskId, &taskInitParam);
    if (ret != LOS_OK) {
        printf("Create filesystem init task failed.\n");
    }
}
