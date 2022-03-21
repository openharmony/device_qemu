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

#include "los_arch_interrupt.h"
#include "los_interrupt.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

INT32 UartPutc(INT32 c, VOID *file)
{
    (VOID) file;
    /* wait for Transmit Holding Empty to be set in LSR */
    while ((ReadUartReg(UART_LSR_OFFSET) & UART_LSR_THRE) == 0) {
        ;
    }
    WriteUartReg(UART_THR_OFFSET, (UINT8)c);
    return c;
}

INT32 UartGetc(VOID)
{
    if (ReadUartReg(UART_LSR_OFFSET) & UART_LSR_DR) {
        return ReadUartReg(UART_RHR_OFFSET);
    } else {
        return 0;
    }
}

INT32 UartOut(INT32 c, VOID *file)
{
    (VOID) file;
    return UartGetc();
}

VOID UartInit(VOID)
{
    /* Disable all interrupts */
    WriteUartReg(UART_IER_OFFSET, 0x00);

    /* special mode to set baud rate */
    WriteUartReg(UART_LCR_OFFSET, UART_LCR_DLAB);

    /* Set divisor low byte, LSB for baud rate of 38.4K */
    WriteUartReg(UART_DLL_OFFSET, 0x03);

    /* Set divisor high byte, LSB for baud rate of 38.4K */
    WriteUartReg(UART_DLM_OFFSET, 0x00);

    /* leave set-baud mode, and set word length to 8 bits, no parity */
    WriteUartReg(UART_LCR_OFFSET, UART_LCR_8N1);

    /* reset and enable FIFOs */
    WriteUartReg(UART_FCR_OFFSET, UART_FCR_FIFO_EN | UART_FCR_RXSR | UART_FCR_TXSR);
}

VOID UartReciveHandler(VOID)
{
    if (ReadUartReg(UART_LSR_OFFSET) & UART_LSR_DR) {
        (void)LOS_EventWrite(&g_shellInputEvent, 0x1);
    }
    return;
}

VOID Uart0RxIrqRegister(VOID)
{
    WriteUartReg(UART_IER_OFFSET, ReadUartReg(UART_IER_OFFSET) | UART_IER_RDI);
    uint32_t ret = LOS_HwiCreate(RISCV_UART0_Rx_IRQn, OS_HWI_PRIO_HIGHEST, 0, (HWI_PROC_FUNC)UartReciveHandler, 0);
    if (ret != LOS_OK) {
        return;
    }
    HalIrqEnable(RISCV_UART0_Rx_IRQn);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
