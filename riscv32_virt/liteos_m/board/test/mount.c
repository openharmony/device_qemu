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

#include "los_memory.h"
#include "shcmd.h"
#include "show.h"

int OsShellCmdMount(int argc, const char **argv)
{
    int ret = 0;

    if (argc < 3) {
        PRINT_ERR("Usage: mount [DEVICE] [PATH] [FSTYPE]\n");
        return -1;
    }

    int strLen = strlen("/dev/cfiblk");
    if (strncmp(argv[0], "/dev/cfiblk", strLen)) {
        PRINT_ERR("mount can not found this device [%s], [/dev/cfiblk] support only now\n", argv[0]);
        return -1;
    }

    strLen = strlen(argv[2]);
    if ((strncmp(argv[2], "fat", strLen) && strncmp(argv[2], "FAT", strLen)) || strLen != strlen("fat"))  {
        PRINT_ERR("mount do not support this fstype [%s] now, try 'fat'\n", argv[2]);
        return -1;
    }

    if (ret = mount(argv[1], argv[1], argv[2], 0, NULL)) {
        PRINT_ERR("Mount error:%d\n", ret);
        return -1;
    }

    return 0;
}

CmdItem g_shellcmd[] = {
    {CMD_TYPE_EX, "mount", XARGS, (CmdCallBackFunc)OsShellCmdMount},
};

void MountShellInit(void)
{
    UINT8 *cmdItemGroup = NULL;
    CmdItemNode *cmdItem = NULL;

    OsShellInit();
    CmdModInfo *cmdInfo = OsCmdInfoGet();

    cmdItemGroup = (UINT8 *)LOS_MemAlloc(m_aucSysMem0, sizeof(CmdItemNode));
    if (cmdItemGroup == NULL) {
        PRINT_ERR("MountShellInit error ...\n");
        return;
    }

    cmdItem = (CmdItemNode *)cmdItemGroup;
    cmdItem->cmd = &g_shellcmd[0];
    OsCmdAscendingInsert(cmdItem);
    cmdInfo->listNum += 1;

    if (DiscDrvInit()) {
        PRINT_ERR("SetupDiscDrv error\n");
    }
}
