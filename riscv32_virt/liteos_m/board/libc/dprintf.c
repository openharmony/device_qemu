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
#include "securec.h"
#include "uart.h"
#include "los_debug.h"
#include "los_interrupt.h"

static void dputs(char const *s, int (*pFputc)(int n, FILE *cookie), void *cookie)
{
    unsigned int intSave;

    intSave = LOS_IntLock();
    while (*s) {
        pFputc(*s++, cookie);
    }
    LOS_IntRestore(intSave);
}

#ifdef LOSCFG_LIBC_NEWLIB
int __wrap_printf(char const  *fmt, ...)
#else /* LOSCFG_LIBC_NEWLIB */
int printf(char const  *fmt, ...)
#endif /* LOSCFG_LIBC_NEWLIB */
{
#define BUFSIZE 1024  // fit the length of LOG_BUF_SIZE in hiview_log.c
    char buf[BUFSIZE] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf_s(buf, sizeof(buf), BUFSIZE - 1, fmt, ap);
    va_end(ap);
    if (len > 0) {
        dputs(buf, UartPutc, 0);
    } else {
        dputs("printf error!\n", UartPutc, 0);
    }
    return len;
}

#define HDF_KM_LOGV       6
#define HDF_KM_LOGD       5
#define HDF_KM_LOGI       4
#define HDF_KM_LOGW       2
#define HDF_KM_LOGE       1
#define HDF_KM_LOG_LEVEL  HDF_KM_LOGW

int hal_trace_printf(int attr, const char *fmt, ...)
{
    if (attr > HDF_KM_LOG_LEVEL)
        return 1;

    char buf[BUFSIZE] = { 0 };
    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf_s(buf, sizeof(buf), BUFSIZE - 1, fmt, ap);
    va_end(ap);
    if (len > 0) {
        dputs(buf, UartPutc, 0);
    } else {
        dputs("printf error!\n", UartPutc, 0);
    }
    return len;
}

/* enable hilog output in LOSCFG_BASE_CORE_HILOG mode */
int HiLogWriteInternal(const char *buffer, size_t bufLen)
{
    const int BUFF_TAIL_LEN = 2;

    if (!buffer) {
        return -1;
    }

    // because it's called as HiLogWriteInternal(buf, strlen(buf) + 1)
    if (bufLen < BUFF_TAIL_LEN) {
        return 0;
    }

    if (buffer[bufLen - BUFF_TAIL_LEN] != '\n') {
        printf("%s\n", buffer);
    } else {
        dputs(buffer, UartPutc, 0);
    }
    return 0;
}
