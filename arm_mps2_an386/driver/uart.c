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
#include "los_config.h"
#include "los_reg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/******* uart register offset *****/
#define UART_TX_DATA         0x00
#define UART_RX_DATA         0x04
#define UART_TX_CTRL         0x08
#define UART_RX_CTRL         0x0C
#define UART_IN_ENAB         0x10
#define UART_IN_PEND         0x14
#define UART_BR_DIV          0x18

INT32 UartPutc(INT32 c, VOID *file)
{
    (VOID)file;

    while (GET_UINT32(UART0_BASE + UART_TX_DATA) & 0x80000000) {
        ;
    }

    WRITE_UINT32((INT32)(c & 0xFF), UART0_BASE + UART_TX_DATA);

    return c;
}

INT32 UartOut(INT32 c, VOID *file)
{
    if (c == '\n') {
        return UartPutc('\r', file);
    }

    return UartPutc(c, file);
}

VOID UartInit()
{

    UINT32 uartDiv;

    /* Enable TX and RX channels */
    WRITE_UINT32(1 << 0, UART0_BASE + UART_TX_CTRL);
    WRITE_UINT32(1 << 0, UART0_BASE + UART_RX_CTRL);

    /* Set baud rate */
    uartDiv = UART0_CLK_FREQ / UART0_BAUDRAT - 1;
    WRITE_UINT32(uartDiv, UART0_BASE + UART_BR_DIV);

    /* Ensure that uart IRQ is disabled initially */
    WRITE_UINT32(0, UART0_BASE + UART_IN_ENAB);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
