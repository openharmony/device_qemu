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

#include "soc/uart.h"
#include "los_magickey.h"
#include "los_task.h"
#include "hdf_log.h"
#include "osal_io.h"
#include "osal_irq.h"
#include "osal_time.h"
#include "uart_if.h"
#include "uart_pl011.h"

#define HDF_LOG_TAG       uart_pl011
#define FIFO_SIZE         128
#define UART_WAIT_MS      10
#define IBRD_COEFFICIENTS 16
#define FBRD_COEFFICIENTS 8
static uint32_t Pl011Irq(uint32_t irq, void *data)
{
    uint32_t status;
    uint32_t fr;
    char buf[FIFO_SIZE];
    uint32_t count = 0;
    struct UartPl011Port *port = NULL;
    struct UartDriverData *udd = (struct UartDriverData *)data;

    UNUSED(irq);
    if (udd == NULL || udd->private == NULL) {
        HDF_LOGE("%s: invalid parame", __func__);
        return HDF_FAILURE;
    }
    port = (struct UartPl011Port *)udd->private;
    status = OSAL_READW(port->physBase + UART_MIS);
    if (status & (UART_MIS_RX | UART_IMSC_TIMEOUT)) {
        do {
            fr = OSAL_READB(port->physBase + UART_FR);
            if (fr & UART_FR_RXFE) {
                break;
            }
            buf[count++] = OSAL_READB(port->physBase + UART_DR);
            if (udd->num != CONSOLE_UART) {
                continue;
            }
            if (CheckMagicKey(buf[count - 1], CONSOLE_SERIAL)) {
                goto end;
            }
        } while (count < FIFO_SIZE);
        udd->recv(udd, buf, count);
    }
end:
    /* clear all interrupt */
    OSAL_WRITEW(0xFFFF, port->physBase + UART_CLR);
    return HDF_SUCCESS;
}

static void Pl011ConfigBaudrate(const struct UartDriverData *udd, const struct UartPl011Port *port)
{
    uint64_t tmp;
    uint32_t value;
    uint32_t divider;
    uint32_t remainder;
    uint32_t fraction;

    tmp = (uint64_t)IBRD_COEFFICIENTS * (uint64_t)udd->baudrate;
    if (tmp == 0 || tmp > UINT32_MAX) {
        HDF_LOGE("%s: err, baudrate %u is invalid", __func__, udd->baudrate);
        return;
    }

    value = IBRD_COEFFICIENTS * udd->baudrate;
    divider = CONFIG_UART_CLK_INPUT / value;
    remainder = CONFIG_UART_CLK_INPUT % value;
    value = (FBRD_COEFFICIENTS * remainder) / udd->baudrate;
    fraction = (value >> 1) + (value & 1);
    OSAL_WRITEL(divider, port->physBase + UART_IBRD);
    OSAL_WRITEL(fraction, port->physBase + UART_FBRD);
}

static void Pl011ConfigDataBits(const struct UartDriverData *udd, uint32_t *lcrh)
{
    *lcrh &= ~UART_LCR_H_8_BIT;
    switch (udd->attr.dataBits) {
        case UART_ATTR_DATABIT_5:
            *lcrh |= UART_LCR_H_5_BIT;
            break;
        case UART_ATTR_DATABIT_6:
            *lcrh |= UART_LCR_H_6_BIT;
            break;
        case UART_ATTR_DATABIT_7:
            *lcrh |= UART_LCR_H_7_BIT;
            break;
        case UART_ATTR_DATABIT_8:
        default:
            *lcrh |= UART_LCR_H_8_BIT;
            break;
    }
}

static void Pl011ConfigParity(const struct UartDriverData *udd, uint32_t *lcrh)
{
    switch (udd->attr.parity) {
        case UART_ATTR_PARITY_EVEN:
            *lcrh |= UART_LCR_H_PEN;
            *lcrh |= UART_LCR_H_EPS;
            *lcrh |= UART_LCR_H_FIFO_EN;
            break;
        case UART_ATTR_PARITY_ODD:
            *lcrh |= UART_LCR_H_PEN;
            *lcrh &= ~UART_LCR_H_EPS;
            *lcrh |= UART_LCR_H_FIFO_EN;
            break;
        case UART_ATTR_PARITY_MARK:
            *lcrh |= UART_LCR_H_PEN;
            *lcrh &= ~UART_LCR_H_EPS;
            *lcrh |= UART_LCR_H_FIFO_EN;
            *lcrh |= UART_LCR_H_SPS;
            break;
        case UART_ATTR_PARITY_SPACE:
            *lcrh |= UART_LCR_H_PEN;
            *lcrh |= UART_LCR_H_EPS;
            *lcrh |= UART_LCR_H_FIFO_EN;
            *lcrh |= UART_LCR_H_SPS;
            break;
        case UART_ATTR_PARITY_NONE:
        default:
            *lcrh &= ~UART_LCR_H_PEN;
            *lcrh &= ~UART_LCR_H_SPS;
            break;
    }
}

static void Pl011ConfigStopBits(const struct UartDriverData *udd, uint32_t *lcrh)
{
    switch (udd->attr.stopBits) {
        case UART_ATTR_STOPBIT_2:
            *lcrh |= UART_LCR_H_STP2;
            break;
        case UART_ATTR_STOPBIT_1:
        default:
            *lcrh &= ~UART_LCR_H_STP2;
            break;
    }
}

static void Pl011ConfigLCRH(const struct UartDriverData *udd, const struct UartPl011Port *port, uint32_t lcrh)
{
    Pl011ConfigDataBits(udd, &lcrh);
    lcrh &= ~UART_LCR_H_PEN;
    lcrh &= ~UART_LCR_H_EPS;
    lcrh &= ~UART_LCR_H_SPS;
    Pl011ConfigParity(udd, &lcrh);
    Pl011ConfigStopBits(udd, &lcrh);
    if (udd->attr.fifoRxEn || udd->attr.fifoTxEn) {
        lcrh |= UART_LCR_H_FIFO_EN;
    }
    OSAL_WRITEB(lcrh, port->physBase + UART_LCR_H);
}

static int32_t Pl011ConfigIn(struct UartDriverData *udd)
{
    uint32_t cr;
    uint32_t lcrh;
    struct UartPl011Port *port = NULL;

    port = (struct UartPl011Port *)udd->private;
    if (port == NULL) {
        HDF_LOGE("%s: port is NULL", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    /* get CR */
    cr = OSAL_READW(port->physBase + UART_CR);
    /* get LCR_H */
    lcrh = OSAL_READW(port->physBase + UART_LCR_H);
    /* uart disable */
    OSAL_WRITEW(0, port->physBase + UART_CR);
    /* config cts/rts */
    if (UART_ATTR_CTS_EN == udd->attr.cts) {
        cr |= UART_CR_CTS;
    } else {
        cr &= ~UART_CR_CTS;
    }
    if (UART_ATTR_RTS_EN == udd->attr.rts) {
        cr |= UART_CR_RTS;
    } else {
        cr &= ~UART_CR_RTS;
    }
    lcrh &= ~UART_LCR_H_FIFO_EN;
    OSAL_WRITEB(lcrh, port->physBase + UART_LCR_H);

    cr &= ~UART_CR_EN;
    OSAL_WRITEW(cr, port->physBase + UART_CR);

    /* set baud rate */
    Pl011ConfigBaudrate(udd, port);

    /* config lcr_h */
    Pl011ConfigLCRH(udd, port, lcrh);

    cr |= UART_CR_EN;
    /* resume CR */
    OSAL_WRITEW(cr, port->physBase + UART_CR);
    return HDF_SUCCESS;
}

static int32_t Pl011StartUp(struct UartDriverData *udd)
{
    int32_t ret;
    uint32_t cr;
    struct UartPl011Port *port = NULL;

    if (udd == NULL) {
        HDF_LOGE("%s: udd is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    port = (struct UartPl011Port *)udd->private;
    if (port == NULL) {
        HDF_LOGE("%s: port is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    /* enable the clock */
    LOS_TaskLock();
    uart_clk_cfg(udd->num, true);
    LOS_TaskUnlock();
    /* uart disable */
    OSAL_WRITEW(0, port->physBase + UART_CR);
    OSAL_WRITEW(0xFF, port->physBase + UART_RSR);
    /* clear all interrupt,set mask */
    OSAL_WRITEW(0xFFFF, port->physBase + UART_CLR);
    /* mask all interrupt */
    OSAL_WRITEW(0x0, port->physBase + UART_IMSC);
    /* interrupt trigger line RX: 4/8, TX 7/8 */
    OSAL_WRITEW(UART_IFLS_RX4_8 | UART_IFLS_TX7_8, port->physBase + UART_IFLS);
    if (!(udd->flags & UART_FLG_DMA_RX)) {
        if (!(port->flags & PL011_FLG_IRQ_REQUESTED)) {
            ret = OsalRegisterIrq(port->irqNum, 0, Pl011Irq, "uart_pl011", udd);
            if (ret == 0) {
                port->flags |= PL011_FLG_IRQ_REQUESTED;
                /* enable rx and timeout interrupt */
                OSAL_WRITEW(UART_IMSC_RX | UART_IMSC_TIMEOUT, port->physBase + UART_IMSC);
            }
        }
    }
    cr = OSAL_READW(port->physBase + UART_CR);
    cr |= UART_CR_EN | UART_CR_RX_EN | UART_CR_TX_EN;
    OSAL_WRITEL(cr, port->physBase + UART_CR);
    ret = Pl011ConfigIn(udd);
    return ret;
}

static int32_t Pl011ShutDown(struct UartDriverData *udd)
{
    uint32_t reg_tmp;
    struct UartPl011Port *port = NULL;

    if (udd == NULL) {
        HDF_LOGE("%s: udd is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    port = (struct UartPl011Port *)udd->private;
    if (port == NULL) {
        HDF_LOGE("%s: port is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    OSAL_WRITEW(0, port->physBase + UART_IMSC);
    OSAL_WRITEW(0xFFFF, port->physBase + UART_CLR);
    if (port->flags & PL011_FLG_IRQ_REQUESTED) {
        OsalUnregisterIrq(port->irqNum, udd);
        port->flags &= ~PL011_FLG_IRQ_REQUESTED;
    }

    reg_tmp = OSAL_READW(port->physBase + UART_CR);
    reg_tmp &= ~UART_CR_TX_EN;
    reg_tmp &= ~UART_CR_RX_EN;
    reg_tmp &= ~UART_CR_EN;
    OSAL_WRITEW(reg_tmp, port->physBase + UART_CR);

    /* disable break and fifo */
    reg_tmp = OSAL_READW(port->physBase + UART_LCR_H);
    reg_tmp &= ~(UART_LCR_H_BREAK);
    reg_tmp &= ~(UART_LCR_H_FIFO_EN);
    OSAL_WRITEW(reg_tmp, port->physBase + UART_LCR_H);

    /* shut down the clock */
    LOS_TaskLock();
    uart_clk_cfg(udd->num, false);
    LOS_TaskUnlock();
    return HDF_SUCCESS;
}

static int32_t Pl011StartTx(struct UartDriverData *udd, const char *buf, size_t count)
{
    struct UartPl011Port *port = NULL;

    if (udd == NULL || buf == NULL || count == 0) {
        HDF_LOGE("%s: invalid parame", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    port = (struct UartPl011Port *)udd->private;
    if (port == NULL) {
        HDF_LOGE("%s: port is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    /* UART_WITH_LOCK: there is a spinlock in the function to write reg in order. */
    (void)UartPutsReg(port->physBase, buf, count, UART_WITH_LOCK);
    return HDF_SUCCESS;
}

static int32_t Pl011Config(struct UartDriverData *udd)
{
    uint32_t fr;
    struct UartPl011Port *port = NULL;

    if (udd == NULL) {
        HDF_LOGE("%s: udd is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    port = (struct UartPl011Port *)udd->private;
    if (port == NULL) {
        HDF_LOGE("%s: port is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    /* wait for send finish */
    do {
        fr = OSAL_READB(port->physBase + UART_FR);
        if (!(fr & UART_FR_BUSY)) {
            break;
        }
        OsalMSleep(UART_WAIT_MS);
    } while (1);
    return Pl011ConfigIn(udd);
}

struct UartOps g_pl011Uops = {
    .StartUp        = Pl011StartUp,
    .ShutDown       = Pl011ShutDown,
    .StartTx        = Pl011StartTx,
    .Config         = Pl011Config,
    .DmaStartUp     = NULL,
    .DmaShutDown    = NULL,
};

int32_t Pl011Read(struct UartDriverData *udd, char *buf, size_t count)
{
    uint32_t wp;
    uint32_t rp;
    uint32_t upperHalf;
    uint32_t lowerHalf = 0;
    unsigned long data;

    if (udd == NULL || buf == NULL || count == 0 || udd->rxTransfer == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    wp = udd->rxTransfer->wp;
    rp = udd->rxTransfer->rp;
    data = (unsigned long)(udd->rxTransfer->data);

    if (rp == wp) {
        return 0; // buffer empty
    }

    if (rp < wp) { // rp behind
        upperHalf = (count > (wp - rp)) ? (wp - rp) : count;
        if (upperHalf > 0 && memcpy_s(buf, upperHalf, (void *)(data + rp), upperHalf) != EOK) {
            return HDF_ERR_IO;
        }
        rp += upperHalf;
    } else { // wp behind
        count = (count > (BUF_SIZE - rp + wp)) ? (BUF_SIZE - rp + wp) : count;
        upperHalf = (count > (BUF_SIZE - rp)) ? (BUF_SIZE - rp) : count;
        lowerHalf = (count > (BUF_SIZE - rp)) ? (count - (BUF_SIZE - rp)) : 0;
        if (upperHalf > 0 && memcpy_s(buf, upperHalf, (void *)(data + rp), upperHalf) != EOK) {
            return HDF_ERR_IO;
        }
        if (lowerHalf > 0 && memcpy_s(buf + upperHalf, lowerHalf, (void *)(data), lowerHalf) != EOK) {
            return HDF_ERR_IO;
        }
        rp += upperHalf;
        if (rp >= BUF_SIZE) {
            rp = lowerHalf;
        }
    }
    udd->rxTransfer->rp = rp;
    return (upperHalf + lowerHalf);
}

static int32_t Pl011Notify(struct wait_queue_head *wait)
{
    if (wait == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    LOS_EventWrite(&wait->stEvent, 0x1);
    notify_poll(wait);
    return HDF_SUCCESS;
}

int32_t PL011UartRecvNotify(struct UartDriverData *udd, const char *buf, size_t count)
{
    uint32_t wp;
    uint32_t rp;
    uint32_t upperHalf;
    uint32_t lowerHalf = 0;
    unsigned long data;

    if (udd == NULL || buf == NULL || count == 0 || udd->rxTransfer == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    wp = udd->rxTransfer->wp;
    rp = udd->rxTransfer->rp;
    data = (unsigned long)(udd->rxTransfer->data);

    if (wp < rp) { // wp behind
        upperHalf = (count > (rp - wp - 1)) ? (rp - wp - 1) : count;
        if (upperHalf > 0 && memcpy_s((void *)(data + wp), upperHalf, buf, upperHalf) != EOK) {
            return HDF_ERR_IO;
        }
        wp += upperHalf;
    } else { // rp behind
        count = (count > ((BUF_SIZE - wp) + rp - 1)) ? (BUF_SIZE - wp) + rp - 1 : count;
        upperHalf = (count > (BUF_SIZE - wp)) ? (BUF_SIZE - wp) : count;
        lowerHalf = (count > (BUF_SIZE - wp)) ? (count - (BUF_SIZE - wp)) : 0;
        if (upperHalf > 0 && memcpy_s((void *)(data + wp), upperHalf, buf, upperHalf) != EOK) {
            return HDF_ERR_IO;
        }
        if (lowerHalf > 0 && memcpy_s((void *)(data), lowerHalf, buf + upperHalf, lowerHalf) != EOK) {
            return HDF_ERR_IO;
        }
        wp += upperHalf;
        if (wp >= BUF_SIZE) {
            wp = lowerHalf;
        }
    }

    if (Pl011Notify(&udd->wait) != HDF_SUCCESS) {
        HDF_LOGE("%s: Pl011 notify err", __func__);
        return HDF_FAILURE;
    }
    udd->rxTransfer->wp = wp;
    return (upperHalf + lowerHalf);
}

bool PL011UartRxBufEmpty(struct UartDriverData *udd)
{
    struct UartTransfer *transfer = udd->rxTransfer;
    return (transfer->wp == transfer->rp);
}

struct UartOps *Pl011GetOps(void)
{
    return &g_pl011Uops;
}
