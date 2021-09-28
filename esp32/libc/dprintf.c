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

#include <stdarg.h>
#include <stdio.h>
#include "los_interrupt.h"

#define PLACEHOLDER_OFFSET  2
#define HEXADECIMAL_NUM     16
#define DECIMALISM_NUM      10
#define BOUNDARY_NUM        9

VOID UartPutc(CHAR c)
{
    (VOID)uart_tx_one_char(c);
}

VOID MatchedOut(CHAR data, va_list ap)
{
    CHAR c = 0;
    UINT32 v, n;
    UINT32 val[100];
    CHAR *s = NULL;
    INT32 i = 0;
    INT32 j = 0;

    switch (data) {
        case 'c':
            c = va_arg(ap, UINT32);
            UartPutc(c);
            break;
        case 's':
            s = va_arg(ap, CHAR*);
            while (*s) {
                UartPutc(*s++);
            }
            break;
        case 'd':
            v = va_arg(ap, UINT32);
            i = 0;
            while (v) {
                n = v % DECIMALISM_NUM;
                val[i] = n;
                v = v / DECIMALISM_NUM;
                i++;
            }
            if (i == 0) {
                UartPutc('0');
            }
            for (j = i - 1; j >= 0; j--) {
                UartPutc('0' + val[j]);
            }
            break;
        case 'x':
            v = va_arg(ap, UINT32);
            i = 0;
            if (v == 0) {
                val[i] = 0;
                i++;
            }
            while (v) {
                n = v % HEXADECIMAL_NUM;
                val[i] = n;
                v = v / HEXADECIMAL_NUM;
                i++;
            }
            if (i == 0) {
                UartPutc('0');
            }
            for (j = i - 1; j >= 0; j--) {
                if (val[j] > BOUNDARY_NUM) {
                    UartPutc('a' + val[j] - DECIMALISM_NUM);
                } else {
                    UartPutc('0' + val[j]);
                }
            }
            break;
        case 'p':
            v = va_arg(ap, UINT32);
            i = 0;
            if (v == 0) {
                val[i] = 0;
                i++;
            }
            while (v) {
                n = v % HEXADECIMAL_NUM;
                val[i] = n;
                v = v / HEXADECIMAL_NUM;
                i++;
            }
            UartPutc('0');
            UartPutc('x');
            if (i == 0) {
                UartPutc('0');
            }
            for (j = i - 1; j >= 0; j--) {
                if (val[j] > BOUNDARY_NUM) {
                    UartPutc('a' + val[j] - DECIMALISM_NUM);
                } else {
                    UartPutc('0' + val[j]);
                }
            }
            break;
        default:
            break;
    }
}

INT32 printf(const CHAR *fmt, ...)
{
    va_list ap;
    UINT32 intSave = LOS_IntLock();
    (VOID)va_start(ap, fmt);
    while (*fmt != '\0') {
        switch (*fmt) {
            case '%':
                MatchedOut(fmt[1], ap);
                fmt += PLACEHOLDER_OFFSET;
                break;
            case '\n':
                UartPutc('\r');
                UartPutc('\n');
                fmt++;
                break;
            case '\r':
                UartPutc('\r');
                UartPutc('\n');
                fmt++;
                break;
            default:
                UartPutc(*fmt);
                fmt++;
                break;
        }
    }
    (VOID)va_end(ap);
    (VOID)LOS_IntRestore(intSave);
    return 0;
}
