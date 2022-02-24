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
#include "target_config.h"
#include "los_typedef.h"

#include "stdlib.h"
#include "stdio.h"
#include "los_process_pri.h"
#ifdef LOSCFG_FS_VFS
#include "disk.h"
#endif
#include "los_bootargs.h"
#include "los_rootfs.h"
#ifdef LOSCFG_SHELL
#include "shell.h"
#include "shcmd.h"
#endif

#ifdef LOSCFG_DRIVERS_RANDOM
#include "los_random.h"
#include "soc/random.h"
#endif

#ifdef LOSCFG_DRIVERS_MEM
#include "los_dev_mem.h"
#endif

#ifdef LOSCFG_DRIVERS_HDF_PLATFORM_UART
#include "console.h"
#include "soc/uart.h"
#endif

#ifdef LOSCFG_DRIVERS_HDF
#include "devmgr_service_start.h"
#endif

#ifdef LOSCFG_DRIVERS_NETDEV
#include "lwip/tcpip.h"
void net_init(void)
{
    tcpip_init(NULL, NULL);
}
#endif

#ifdef LOSCFG_DRIVERS_MEM
int mem_dev_register(void)
{
    return DevMemRegister();
}
#endif

void SystemInit(void)
{
#ifdef LOSCFG_DRIVERS_RANDOM
    dprintf("dev random init ...\n");
    (void)DevRandomRegister();
#ifdef LOSCFG_HW_RANDOM_ENABLE
    VirtrngInit();
#endif
#endif

#ifdef LOSCFG_DRIVERS_MEM
    dprintf("mem dev init ...\n");
    extern int mem_dev_register(void);
    mem_dev_register();
#endif

#ifdef LOSCFG_DRIVERS_MMZ_CHAR_DEVICE
    dprintf("DevMmzRegister...\n");
    extern int DevMmzRegister(void);
    DevMmzRegister();
#endif

    dprintf("Date:%s.\n", __DATE__);
    dprintf("Time:%s.\n", __TIME__);

#ifdef LOSCFG_DRIVERS_NETDEV
    dprintf("net init ...\n");
    net_init();
    dprintf("\n************************************************************\n");
#endif

#ifdef LOSCFG_DRIVERS_HDF
    dprintf("DeviceManagerStart start ...\n");
    if (DeviceManagerStart()) {
        PRINT_ERR("No drivers need load by hdf manager!");
    }
    dprintf("DeviceManagerStart end ...\n");
#endif

#ifdef LOSCFG_PLATFORM_ROOTFS
    dprintf("OsMountRootfs start ...\n");
    if (LOS_GetCmdLine()) {
        PRINT_ERR("get cmdline error!\n");
    }
    if (LOS_ParseBootargs()) {
        PRINT_ERR("parse bootargs error!\n");
    }
    if (OsMountRootfs()) {
        PRINT_ERR("mount rootfs error!\n");
    }
    dprintf("OsMountRootfs end ...\n");
#endif

#ifdef LOSCFG_DRIVERS_HDF_PLATFORM_UART
    dprintf("virtual_serial_init start ...\n");
    if (virtual_serial_init(TTY_DEVICE) != 0) {
        PRINT_ERR("virtual_serial_init failed");
    }
    dprintf("virtual_serial_init end ...\n");
    dprintf("system_console_init start ...\n");
    if (system_console_init(SERIAL) != 0) {
        PRINT_ERR("system_console_init failed\n");
    }
    dprintf("system_console_init end ...\n");
#endif
    dprintf("OsUserInitProcess start ...\n");
    if (OsUserInitProcess()) {
        PRINT_ERR("Create user init process faialed!\n");
        return;
    }
    dprintf("OsUserInitProcess end ...\n");
    return;
}
