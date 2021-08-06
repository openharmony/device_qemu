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

#include "los_config.h"
#ifdef LOSCFG_SHELL
#include "reset_shell.h"
#include "signal.h"
#include "asm/io.h"
#include "stdlib.h"
#include "soc/sys_ctrl.h"
#include "unistd.h"
#include "shcmd.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

Hook_Func_Node *g_hook_func_node = (Hook_Func_Node*)NULL;

extern void OsReboot(void);
void cmd_reset(void)
{
    osReHookFuncHandle();
    sleep(1);/*lint !e534*/
    /* Any value to this reg will reset the cpu */
    OsReboot();
}

UINT32 osReHookFuncAdd(STORAGE_HOOK_FUNC handler, VOID *param)
{
    Hook_Func_Node *pstFuncNode;

    pstFuncNode = (Hook_Func_Node*)malloc(sizeof(Hook_Func_Node));
    if (pstFuncNode == NULL) {
        return OS_ERROR;
    }

    pstFuncNode->pHandler = handler;
    pstFuncNode->pParam = param;

    pstFuncNode->pNext = g_hook_func_node;
    g_hook_func_node = pstFuncNode;

    return  LOS_OK;
}

UINT32 osReHookFuncDel(STORAGE_HOOK_FUNC handler)
{
    Hook_Func_Node *pstFuncNode = NULL;
    Hook_Func_Node *pstCurFuncNode = NULL;
    while (g_hook_func_node) {
        pstCurFuncNode = g_hook_func_node;
        if (g_hook_func_node->pHandler == handler) {
            g_hook_func_node = g_hook_func_node->pNext;
            free(pstCurFuncNode);
            continue;
        }
        break;
    }

    if (g_hook_func_node) {
        pstCurFuncNode = g_hook_func_node;
        while (pstCurFuncNode->pNext) {
            pstFuncNode = pstCurFuncNode->pNext;
            if (pstFuncNode->pHandler == handler) {
                pstCurFuncNode->pNext = pstFuncNode->pNext;
                free(pstFuncNode);
                continue;
            }
            pstCurFuncNode = pstCurFuncNode->pNext;
        }
    }
    return LOS_OK;
}

VOID osReHookFuncHandle(VOID)
{
    Hook_Func_Node *pstFuncNode;

    pstFuncNode = g_hook_func_node;
    while (pstFuncNode) {
        (void)pstFuncNode->pHandler(pstFuncNode->pParam);
        pstFuncNode = pstFuncNode->pNext;
    }
}

SHELLCMD_ENTRY(reset_shellcmd, CMD_TYPE_EX, "reset", 0, (CmdCallBackFunc)cmd_reset); /*lint !e19*/

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif /* LOSCFG_SHELL */
