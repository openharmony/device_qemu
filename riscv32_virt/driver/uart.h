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

#ifndef _UART_H
#define _UART_H

#include "los_compiler.h"
#include "los_event.h"
#include "los_reg.h"
#include "soc.h"

#define RISCV_UART0_Rx_IRQn  (RISCV_SYS_MAX_IRQ + 10)

// the UART control registers.
// some have different meanings for read vs write.
// see http://byterunner.com/16550.html
#define UART_RHR_OFFSET      0 /* receive holding register (for input bytes) */
#define UART_THR_OFFSET      0 /* transmit holding register (for output bytes) */
#define UART_DLL_OFFSET      0 /* Divisor Latch (Least Significant Byte) Register (LSB) */
#define UART_IER_OFFSET      1 /* interrupt enable register */
#define UART_DLM_OFFSET      1 /* Divisor Latch (Most Significant Byte) Register (MSB) */
#define UART_FCR_OFFSET      2 /* FIFO control register */
#define UART_ISR_OFFSET      2 /* interrupt status register */
#define UART_LCR_OFFSET      3 /* line control register */
#define UART_LSR_OFFSET      5 /* line status register */

#define UART_LCR_8N1         0x03 /* useful defaults for LCR */
#define UART_LCR_DLAB        0x80 /* Divisor latch access bit */

#define UART_IER_RDI         0x01 /* Enable receiver data interrupt */
#define UART_IER_THRI        0x02 /* Enable Transmitter holding register int. */

#define UART_FCR_FIFO_EN     0x01 /* FIFO Enable */
#define UART_FCR_RXSR        0x02 /* Receiver soft reset */
#define UART_FCR_TXSR        0x04 /* Transmitter soft reset */

#define UART_LSR_DR          0x01 /* Receiver data ready */
#define UART_LSR_THRE        0x20 /* Transmit-hold-register empty */

#define ReadUartReg(reg)     GET_UINT8((UART0_BASE) + (reg))
#define WriteUartReg(reg, v) WRITE_UINT8((v), (UART0_BASE) + (reg))

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

extern INT32 UartPutc(INT32 c, VOID *file);
extern INT32 UartOut(INT32 c, VOID *file);

extern INT32 UartInit(VOID);
extern INT32 UartGetc(VOID);
extern VOID Uart0RxIrqRegister(VOID);

extern EVENT_CB_S g_shellInputEvent;

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
#endif
