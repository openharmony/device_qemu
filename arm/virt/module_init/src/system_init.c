/*
 * Copyright (c) 2020 HiSilicon (Shanghai) Technologies CO., LIMITED.
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
#include "sys_config.h"
#include "board.h"
#include "los_typedef.h"

#include "stdlib.h"
#include "stdio.h"
#include "los_process_pri.h"
#ifdef LOSCFG_FS_VFS
#include "disk.h"
#endif
#include "los_rootfs.h"
#ifdef LOSCFG_SHELL
#include "shell.h"
#include "shcmd.h"
#endif

#ifdef LOSCFG_DRIVERS_HDF_PLATFORM_UART
#include "console.h"
#include "hisoc/uart.h"
#endif

#ifdef LOSCFG_DRIVERS_HDF
#include "devmgr_service_start.h"
#endif

#ifdef LOSCFG_DRIVERS_NETDEV
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"

#define SLEEP_TIME_MS 60
#define NETIF_SETUP_OVERTIME 100

void net_init(void)
{
extern void tcpip_init(tcpip_init_done_fn initfunc, void *arg);
    dprintf("\ntcpip_init start\n");
    tcpip_init(NULL, NULL);
    dprintf("\ntcpip_init end\n");

    PRINTK("Ethernet start.");

#ifdef LOSCFG_DRIVERS_HIGMAC
    extern int ethnet_higmac_init(void);
    (void)ethnet_higmac_init();
#endif

#ifdef LOSCFG_DRIVERS_HIETH_SF
    extern int ethnet_hieth_init(void);
    (void)ethnet_hieth_init();
#endif

    /* TODO: Re-enable this when Netif driver is implemented */
#if 0
    struct netif *pnetif;

    extern void get_defaultNetif(struct netif **pnetif);
    get_defaultNetif(&pnetif);

    (void)netifapi_netif_set_up(pnetif);
    unsigned int overtime = 0;
    do {
        LOS_Msleep(SLEEP_TIME_MS);
        overtime++;
        if (overtime > NETIF_SETUP_OVERTIME) {
            PRINTK("netif_is_link_up overtime!\n");
            break;
        }
    } while (netif_is_link_up(pnetif) == 0);
#endif
}
#endif

void SystemInit(void)
{

#ifdef LOSCFG_FS_PROC
    dprintf("proc fs init ...\n");
    extern void ProcFsInit(void);
    ProcFsInit();
#endif

#ifdef LOSCFG_DRIVERS_MEM
    dprintf("mem dev init ...\n");
    extern int mem_dev_register(void);
    mem_dev_register();
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
    OsMountRootfs();
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
