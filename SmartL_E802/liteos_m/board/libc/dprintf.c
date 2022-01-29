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

#include "dprintf.h"
#include <stdarg.h>
#include <stdio.h>
#include <csi_config.h>
#include "pin.h"
#include "drv_gpio.h"
#include "drv_usart.h"
#include "los_compiler.h"
#include "los_interrupt.h"

#define BUFSIZE             256
#define UART_BAUDRATE       115200

usart_handle_t g_consoleHandle;

void uart_early_init(void)
{
    drv_pinmux_config(UART_TXD0, CONSOLE_TXD);
    drv_pinmux_config(UART_RXD0, CONSOLE_RXD);

    g_consoleHandle = csi_usart_initialize(CONSOLE_IDX, NULL);
    csi_usart_config(g_consoleHandle, UART_BAUDRATE, USART_MODE_ASYNCHRONOUS,
                     USART_PARITY_NONE, USART_STOP_BITS_1, USART_DATA_BITS_8);
}

STATIC VOID UartPutc(CHAR c)
{
    (VOID)csi_usart_putchar(g_consoleHandle, c);
}

STATIC VOID FuncPuts(const CHAR *s, VOID (*funcPutc)(CHAR c))
{
    UINT32 intSave;

    intSave = LOS_IntLock();
    while (*s) {
        funcPutc(*s++);
    }
    LOS_IntRestore(intSave);
}

INT32 printf(const CHAR *fmt, ...)
{
    CHAR buf[BUFSIZE] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf_s(buf, sizeof(buf), BUFSIZE - 1, fmt, ap);
    va_end(ap);
    if (len > 0) {
        FuncPuts(buf, UartPutc);
    } else {
        FuncPuts("printf error!\n", UartPutc);
    }

    return len;
}

