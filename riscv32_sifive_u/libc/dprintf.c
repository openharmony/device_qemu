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

#include "stdarg.h"
#include <stdio.h>
#include "los_debug.h"
#include "uart.h"

int putchar(int n)
{
    return UartOut(n, NULL);
}

int puts(const char *string)
{
    int count = 0;
    char *s = (char *)string;
    while (*s != '\0') {
        putchar(*s);
        s++;
        count++;
    }

    return count;
}

static int hex2asc(int n)
{
    n &= 15;
    if(n > 9){
        return ('a' - 10) + n;
    } else {
        return '0' + n;
    }
}

static void dputs(char const *s, int (*pFputc)(int n, void *cookie), void *cookie)
{
    while (*s) {
        pFputc(*s++, cookie);
    }
}

void __dprintf(char const *fmt, va_list ap,
        int (*pFputc)(int n, void *cookie),
        void *cookie)
{
    char scratch[256];

    for(;;){
        switch(*fmt){
            case 0:
                va_end(ap);
                return;
            case '%':
                switch(fmt[1]) {
                    case 'c': {
                                  unsigned n = va_arg(ap, unsigned);
                                  pFputc(n, cookie);
                                  fmt += 2;
                                  continue;
                              }
                    case 'h': {
                                  unsigned n = va_arg(ap, unsigned);
                                  pFputc(hex2asc(n >> 12), cookie);
                                  pFputc(hex2asc(n >> 8), cookie);
                                  pFputc(hex2asc(n >> 4), cookie);
                                  pFputc(hex2asc(n >> 0), cookie);
                                  fmt += 2;
                                  continue;
                              }
                    case 'b': {
                                  unsigned n = va_arg(ap, unsigned);
                                  pFputc(hex2asc(n >> 4), cookie);
                                  pFputc(hex2asc(n >> 0), cookie);
                                  fmt += 2;
                                  continue;
                              }
                    case 'p':
                    case 'X':
                    case 'x': {
                                  unsigned n = va_arg(ap, unsigned);
                                  char *p = scratch + 15;
                                  *p = 0;
                                  do {
                                      *--p = hex2asc(n);
                                      n = n >> 4;
                                  } while(n != 0);
                                  while(p > (scratch + 7)) *--p = '0';
                                  dputs(p, pFputc, cookie);
                                  fmt += 2;
                                  continue;
                              }
                    case 'd': {
                                  int n = va_arg(ap, int);
                                  char *p = scratch + 15;
                                  *p = 0;
                                  if(n < 0) {
                                      pFputc('-', cookie);
                                      n = -n;
                                  }
                                  do {
                                      *--p = (n % 10) + '0';
                                      n /= 10;
                                  } while(n != 0);
                                  dputs(p, pFputc, cookie);
                                  fmt += 2;
                                  continue;
                              }
                    case 'u': {
                                  unsigned int n = va_arg(ap, unsigned int);
                                  char *p = scratch + 15;
                                  *p = 0;
                                  do {
                                      *--p = (n % 10) + '0';
                                      n /= 10;
                                  } while(n != 0);
                                  dputs(p, pFputc, cookie);
                                  fmt += 2;
                                  continue;
                              }
                    case 's': {
                                  char *s = va_arg(ap, char*); /*lint !e64*/
                                  if(s == 0) s = "(null)";
                                  dputs(s, pFputc, cookie);
                                  fmt += 2;
                                  continue;
                              }
                }
                pFputc(*fmt++, cookie);
                break;
            case '\n':
                pFputc('\r', cookie);
            default: /*lint !e616*/
                pFputc(*fmt++, cookie);
        }
    }
}

int printf(char const *str, ...)
{
    va_list ap;
    va_start(ap, str);
    __dprintf(str, ap, UartPutc, 0);
    va_end(ap);
    return 0;
}
