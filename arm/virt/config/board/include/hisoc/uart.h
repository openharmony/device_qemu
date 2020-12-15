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

#ifndef __HISOC_UART_H__
#define __HISOC_UART_H__

#include "asm/platform.h"
#include "los_typedef.h"
#include "los_base.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define CONFIG_UART0_BAUDRATE   115200
#define CONFIG_UART_CLK_INPUT   (24000000) // 24M or 6M

#define UART0                   0
#define UART0_ENABLE            1
#define UART0_DMA_RX_PERI       4

#define TTYS0                               "/dev/ttyS0"

#define CONSOLE_UART                        UART0

#define CONSOLE_UART_BAUDRATE               115200
#define UART_NUM    1
#if (CONSOLE_UART == UART0)
    #define TTY_DEVICE                "/dev/uartdev-0"
    #define UART_REG_BASE             UART0_REG_BASE
    #define NUM_HAL_INTERRUPT_UART    NUM_HAL_INTERRUPT_UART0
#endif

extern VOID UartPuts(const CHAR *s, UINT32 len, BOOL isLock);

#define UART_WITHOUT_LOCK 0
#define UART_WITH_LOCK    1

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
