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
#include "stdio.h"
#include "pin.h"
#include "drv_gpio.h"
#include "drv_usart.h"
#include <csi_config.h>
#include "los_compiler.h"

#define UART_QUEUE_SIZE         64
#define UART_QUEUE_BUF_MAX_LEN  1
#define UART_BAUDRATE           115200

usart_handle_t g_consoleHandle;

void uart_early_init(void)
{
    drv_pinmux_config(UART_TXD0, CONSOLE_TXD);
    drv_pinmux_config(UART_RXD0, CONSOLE_RXD);

    g_consoleHandle = csi_usart_initialize(CONSOLE_IDX, NULL);
    csi_usart_config(g_consoleHandle, UART_BAUDRATE, USART_MODE_ASYNCHRONOUS,
                     USART_PARITY_NONE, USART_STOP_BITS_1, USART_DATA_BITS_8);
}

int fputc(int ch, FILE *stream)
{
    (void)stream;
    if (g_consoleHandle == NULL) {
        return -1;
    }
    if (ch == '\n') {
        csi_usart_putchar(g_consoleHandle, '\r');
    }
    csi_usart_putchar(g_consoleHandle, ch);
    return 0;
}

int fgetc(FILE *stream)
{
    uint8_t ch;
    (void)stream;
    if (g_consoleHandle == NULL) {
        return -1;
    }
    csi_usart_getchar(g_consoleHandle, &ch);
    return ch;
}

int os_critical_enter(unsigned int *lock)
{
    (void)lock;
    return 0;
}

int os_critical_exit(unsigned int *lock)
{
    (void)lock;
    return 0;
}
