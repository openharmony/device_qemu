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

#include "uart.h"
#include "arm_uart_drv.h"
#include "stdio.h"
#include "los_config.h"
#include "los_reg.h"
#include "los_interrupt.h"
#include "los_event.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

static const struct arm_uart_dev_cfg_t g_uartCfg = {UART0_BASE, 115200};
static struct arm_uart_dev_data_t g_uartData = {0};
struct arm_uart_dev_t g_uartDev;

INT32 UartGetc()
{
    UINT8 c;
    if (arm_uart_read(&g_uartDev, &c) == ARM_UART_ERR_NOT_READY) {
        return 0;
    }

    return c;
}

INT32 UartPutc(INT32 c, VOID *file)
{
    return arm_uart_write(&g_uartDev, (UINT8)c);
}

VOID UartReciveHandler(VOID)
{
    if (arm_uart_get_interrupt_status(&g_uartDev) == ARM_UART_IRQ_RX) {
        (void)LOS_EventWrite(&g_shellInputEvent, 0x1);
        (void)arm_uart_clear_interrupt(&g_uartDev, ARM_UART_IRQ_RX);
    }
    return;
}

VOID UartInit()
{
    g_uartDev.cfg = &g_uartCfg;
    g_uartDev.data = &g_uartData;
    (void)arm_uart_init(&g_uartDev, UART0_CLK_FREQ);
    return;
}

VOID Uart0RxIrqRegister()
{
    (void)arm_uart_irq_rx_enable(&g_uartDev);
    (void)LOS_HwiCreate(Uart0_Rx_IRQn, 0, 0, (HWI_PROC_FUNC)UartReciveHandler, 0);
    return;
}
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
