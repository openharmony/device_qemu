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

#ifdef LOSCFG_NET_LWIP

#define IFNAMSIZ  IF_NAMESIZE
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/netifapi.h"

#define SLEEP_TIME_MS 60
#define NETIF_SETUP_OVERTIME 100

void net_init(void)
{
    extern void tcpip_init(tcpip_init_done_fn initfunc, void *arg);
    extern struct netif *VirtnetInit(void);

    static unsigned int overtime = 0;

    printf("tcpip_init start\n");
    tcpip_init(NULL, NULL);
    printf("tcpip_init end\n");

    printf("netif init start...\n");
    struct netif *pnetif = VirtnetInit();
    if (pnetif) {
        netif_set_default(pnetif);
        (void)netifapi_netif_set_up(pnetif);
        do {
            LOS_UDelay(SLEEP_TIME_MS);
            overtime++;
            if (overtime > NETIF_SETUP_OVERTIME) {
                PRINT_ERR("netif_is_link_up overtime!\n");
                break;
            }
        } while (netif_is_link_up(pnetif) == 0);
        if (overtime <= NETIF_SETUP_OVERTIME) {
            printf("netif init succeed!\n");
        }
    } else {
        printf("netif init failed!\n");
    }
}
#endif /* LOSCFG_NET_LWIP */

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

    UartInit();
    printf("\n OHOS start \n\r");

    ret = LOS_KernelInit();
    if (ret != LOS_OK) {
        PRINT_ERR("Liteos kernel init failed! ERROR: 0x%x\n", ret);
        goto START_FAILED;
    }

    HalPlicInit();

    Uart0RxIrqRegister();

#if (LOSCFG_USE_SHELL == 1)
    ret = LosShellInit();
    if (ret != LOS_OK) {
        printf("LosShellInit failed! ERROR: 0x%x\n", ret);
    }
#endif

    printf("\nFb init begin...\n");
    if (fb_init() != LOS_OK) {
        PRINT_ERR("Fb init failed!");
    }
    printf("Fb int end\n");

    printf("\nDeviceManagerStart start ...\n");
    if (DeviceManagerStart()) {
        PRINT_ERR("No drivers need load by hdf manager!");
    }
    printf("DeviceManagerStart end ...\n");

#ifdef LOSCFG_NET_LWIP
    net_init();
#endif /* LOSCFG_NET_LWIP */

    ret = LosAppInit();
    if (ret != LOS_OK) {
        PRINT_ERR("LosAppInit failed! ERROR: 0x%x\n", ret);
    }

    LOS_Start();

START_FAILED:
    while (1) {
        __asm volatile("wfi");
    }
}

void *ioremap(uintptr_t paddr, unsigned long size)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
    return paddr;
}

void iounmap(void *vaddr)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
}

int32_t I2cOpen(int16_t number)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
    return 0;
}

int32_t GpioRead(uint16_t gpio, uint16_t *val)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
    return 0;
}

int32_t GpioSetIrq(uint16_t gpio, uint16_t mode, void *func, void *arg)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
    return 0;
}

int32_t GpioSetDir(uint16_t gpio, uint16_t dir)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
    return 0;
}

int32_t GpioEnableIrq(uint16_t gpio)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
    return 0;
}

int32_t GpioDisableIrq(uint16_t gpio)
{
    printf("[WARN] Function to be implemented: %s\n", __FUNCTION__);
    return 0;
}
