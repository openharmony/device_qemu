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

#ifndef UART_PL011_H
#define UART_PL011_H

#include "console.h"
#include "poll.h"
#include "uart_if.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define DEFAULT_UART0_BAUDRATE 115200
#define DEFAULT_UART1_BAUDRATE 9600
#define DEFAULT_UART2_BAUDRATE 115200
#define DEFAULT_UART3_BAUDRATE 9600
#define DEFAULT_UART4_BAUDRATE 9600
#define CONFIG_MAX_BAUDRATE    921600

#define UART_DR                0x0  /* data register */
#define UART_RSR               0x04
#define UART_FR                0x18 /* flag register */
#define UART_CLR               0x44 /* interrupt clear register */
#define UART_CR                0x30 /* control register */
#define UART_IBRD              0x24 /* integer baudrate register */
#define UART_FBRD              0x28 /* decimal baudrate register */
#define UART_LCR_H             0x2C
#define UART_IFLS              0x34 /* fifo register */
#define UART_IMSC              0x38 /* interrupt mask register */
#define UART_RIS               0x3C /* base interrupt state register */
#define UART_MIS               0x40 /* mask interrupt state register */
#define UART_ICR               0x44
#define UART_DMACR             0x48 /* DMA control register */

/* register define */
#define UART_IFLS_RX1_8        (0x00 << 3)
#define UART_IFLS_RX4_8        (0x02 << 3)
#define UART_IFLS_RX7_8        (0x04 << 3)
#define UART_IFLS_TX1_8        (0x00 << 0)
#define UART_IFLS_TX4_8        (0x02 << 0)
#define UART_IFLS_TX7_8        (0x04 << 0)

#define UART_CR_CTS            (0x01 << 15)
#define UART_CR_RTS            (0x01 << 14)
#define UART_CR_RX_EN          (0x01 << 9)
#define UART_CR_TX_EN          (0x01 << 8)
#define UART_CR_LOOPBACK       (0x01 << 7)
#define UART_CR_EN             (0x01 << 0)

#define UART_FR_TXFE           (0x01 << 7)
#define UART_FR_RXFF           (0x01 << 6)
#define UART_FR_TXFF           (0x01 << 5)
#define UART_FR_RXFE           (0x01 << 4)
#define UART_FR_BUSY           (0x01 << 3)

#define UART_LCR_H_BREAK       (0x01 << 0)
#define UART_LCR_H_PEN         (0x01 << 1)
#define UART_LCR_H_EPS         (0x01 << 2)
#define UART_LCR_H_STP2        (0x01 << 3)
#define UART_LCR_H_FIFO_EN     (0x01 << 4)
#define UART_LCR_H_8_BIT       (0x03 << 5)
#define UART_LCR_H_7_BIT       (0x02 << 5)
#define UART_LCR_H_6_BIT       (0x01 << 5)
#define UART_LCR_H_5_BIT       (0x00 << 5)
#define UART_LCR_H_SPS         (0x01 << 7)

#define UART_RXDMAE            (0x01 << 0)
#define UART_TXDMAE            (0x01 << 1)

#define UART_MIS_TIMEOUT       (0x01 << 6)
#define UART_MIS_TX            (0x01 << 5)
#define UART_MIS_RX            (0x01 << 4)

#define UART_IMSC_OVER         (0x01 << 10)
#define UART_IMSC_BREAK        (0x01 << 9)
#define UART_IMSC_CHK          (0x01 << 8)
#define UART_IMSC_ERR          (0x01 << 7)
#define UART_IMSC_TIMEOUT      (0x01 << 6)
#define UART_IMSC_TX           (0x01 << 5)
#define UART_IMSC_RX           (0x01 << 4)

#define UART_DMACR_RX          (0x01 << 0)
#define UART_DMACR_TX          (0x01 << 1)
#define UART_DMACR_ONERR       (0x01 << 2)
#define UART_INFO              (0x01 << 1)

/* DMA buf size: 4K */
#define RX_DMA_BUF_SIZE        0x1000

/* receive buf default size: 16K */
#define BUF_SIZE               0x4000

struct UartDriverData;

struct UartOps {
    int32_t (*StartUp)(struct UartDriverData *udd);
    int32_t (*ShutDown)(struct UartDriverData *udd);
    int32_t (*DmaStartUp)(struct UartDriverData *udd, int32_t dir);
    int32_t (*DmaShutDown)(struct UartDriverData *udd, int32_t dir);
#define UART_DMA_DIR_RX 0
#define UART_DMA_DIR_TX 1
    int32_t (*StartTx)(struct UartDriverData *udd, const char *buf, size_t count);
    int32_t (*Config)(struct UartDriverData *udd);
    /* private operation */
    int32_t (*PrivOperator)(struct UartDriverData *udd, void *data);
};

struct UartTransfer {
    uint32_t rp;
    uint32_t wp;
    uint32_t flags;
#define BUF_CIRCLED    (1 << 0)
#define BUF_OVERFLOWED (1 << 1)
#define BUF_EMPTIED    (1 << 2)
    char data[BUF_SIZE];
};

typedef int32_t (*RecvNotify)(struct UartDriverData *udd, const char *buf, size_t count);

struct UartDriverData {
    uint32_t num;
    uint32_t baudrate;
    struct UartAttribute attr;
    struct UartTransfer *rxTransfer;
    wait_queue_head_t wait;
    int32_t count;
    int32_t state;
#define UART_STATE_NOT_OPENED 0
#define UART_STATE_OPENING    1
#define UART_STATE_USEABLE    2
#define UART_STATE_SUSPENED   3
    uint32_t flags;
#define UART_FLG_DMA_RX       (1 << 0)
#define UART_FLG_DMA_TX       (1 << 1)
#define UART_FLG_RD_BLOCK     (1 << 2)
    RecvNotify recv;
    struct UartOps *ops;
    void *private;
};

struct UartDmaTransfer {
    /* dma alloced channel */
    uint32_t channel;
    /* dma created task id */
    uint32_t thread_id;
    /* dma receive buf head */
    uint32_t head;
    /* dma receive buf tail */
    uint32_t tail;
    /* dma receive buf cycled flag */
    uint32_t flags;
#define BUF_CIRCLED (1 << 0)
    /* dma receive buf, should be cache aligned */
    char *buf;
};

struct UartPl011Port {
    int32_t enable;
    unsigned long physBase;
    uint32_t irqNum;
    uint32_t defaultBaudrate;
    uint32_t flags;
#define PL011_FLG_IRQ_REQUESTED    (1 << 0)
#define PL011_FLG_DMA_RX_REQUESTED (1 << 1)
#define PL011_FLG_DMA_TX_REQUESTED (1 << 2)
    struct UartDmaTransfer *rxUdt;
    struct UartDriverData *udd;
};

/* read some data from rx_data buf in UartTransfer */
int32_t Pl011Read(struct UartDriverData *udd, char *buf, size_t count);
/* check the buf is empty */
bool PL011UartRxBufEmpty(struct UartDriverData *udd);
int32_t PL011UartRecvNotify(struct UartDriverData *udd, const char *buf, size_t count);
struct UartOps *Pl011GetOps(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif /* UART_PL011_H */
