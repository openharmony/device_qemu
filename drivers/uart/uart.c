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

#include "los_event.h"
#include "device_resource_if.h"
#include "hdf_base.h"
#include "hdf_log.h"
#include "osal_io.h"
#include "osal_mem.h"
#include "osal_time.h"
#include "uart_core.h"
#include "uart_dev.h"
#include "uart_if.h"
#include "uart_pl011.h"

#define HDF_LOG_TAG uart_hi35xx

static int32_t Hi35xxRead(struct UartHost *host, uint8_t *data, uint32_t size)
{
    int32_t ret;
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    udd = (struct UartDriverData *)host->priv;
    if (udd->state != UART_STATE_USEABLE) {
        return HDF_FAILURE;
    }
    if ((udd->flags & UART_FLG_RD_BLOCK) && (PL011UartRxBufEmpty(udd))) {
        (void)LOS_EventRead(&udd->wait.stEvent, 0x1, LOS_WAITMODE_OR, LOS_WAIT_FOREVER);
    }
    ret = Pl011Read(udd, (char *)data, size);
    if ((udd->flags & UART_FLG_RD_BLOCK) && (PL011UartRxBufEmpty(udd))) {
        (void)LOS_EventClear(&udd->wait.stEvent, ~(0x1));
    }
    return ret;
}

static int32_t Hi35xxWrite(struct UartHost *host, uint8_t *data, uint32_t size)
{
    int32_t ret;
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    udd = (struct UartDriverData *)host->priv;
    if (udd->state != UART_STATE_USEABLE) {
        return HDF_FAILURE;
    }
    if (udd->ops->StartTx != NULL) {
        ret = udd->ops->StartTx(udd, (char *)data, size);
    } else {
        ret = HDF_ERR_NOT_SUPPORT;
        HDF_LOGE("%s: not support", __func__);
    }
    return ret;
}

static int32_t Hi35xxGetBaud(struct UartHost *host, uint32_t *baudRate)
{
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL || baudRate == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udd = (struct UartDriverData *)host->priv;
    if (udd->state != UART_STATE_USEABLE) {
        return HDF_FAILURE;
    }
    *baudRate = udd->baudrate;
    return HDF_SUCCESS;
}

static int32_t Hi35xxSetBaud(struct UartHost *host, uint32_t baudRate)
{
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udd = (struct UartDriverData *)host->priv;
    if (udd->state != UART_STATE_USEABLE) {
        return HDF_FAILURE;
    }
    if ((baudRate > 0) && (baudRate <= CONFIG_MAX_BAUDRATE)) {
        udd->baudrate = baudRate;
        if (udd->ops->Config == NULL) {
            HDF_LOGE("%s: not support", __func__);
            return HDF_ERR_NOT_SUPPORT;
        }
        if (udd->ops->Config(udd) != HDF_SUCCESS) {
            HDF_LOGE("%s: config baudrate %d failed", __func__, baudRate);
            return HDF_FAILURE;
        }
    } else {
        HDF_LOGE("%s: invalid baudrate, which is:%d", __func__, baudRate);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t Hi35xxGetAttribute(struct UartHost *host, struct UartAttribute *attribute)
{
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL || attribute == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    udd = (struct UartDriverData *)host->priv;
    if (udd->state != UART_STATE_USEABLE) {
        return HDF_FAILURE;
    }
    attribute->cts = udd->attr.cts;
    attribute->dataBits = udd->attr.dataBits;
    attribute->fifoRxEn = udd->attr.fifoRxEn;
    attribute->fifoTxEn = udd->attr.fifoTxEn;
    attribute->parity = udd->attr.parity;
    attribute->rts = udd->attr.rts;
    attribute->stopBits = udd->attr.stopBits;
    return HDF_SUCCESS;
}

static int32_t Hi35xxSetAttribute(struct UartHost *host, struct UartAttribute *attribute)
{
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL || attribute == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    udd = (struct UartDriverData *)host->priv;
    if (udd->state != UART_STATE_USEABLE) {
        return HDF_FAILURE;
    }
    udd->attr.cts = attribute->cts;
    udd->attr.dataBits = attribute->dataBits;
    udd->attr.fifoRxEn = attribute->fifoRxEn;
    udd->attr.fifoTxEn = attribute->fifoTxEn;
    udd->attr.parity = attribute->parity;
    udd->attr.rts = attribute->rts;
    udd->attr.stopBits = attribute->stopBits;
    if (udd->ops->Config == NULL) {
        HDF_LOGE("%s: not support", __func__);
        return HDF_ERR_NOT_SUPPORT;
    }
    if (udd->ops->Config(udd) != HDF_SUCCESS) {
        HDF_LOGE("%s: config failed", __func__);
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t Hi35xxSetTransMode(struct UartHost *host, enum UartTransMode mode)
{
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    udd = (struct UartDriverData *)host->priv;
    if (udd->state != UART_STATE_USEABLE) {
        return HDF_FAILURE;
    }
    if (mode == UART_MODE_RD_BLOCK) {
        udd->flags |= UART_FLG_RD_BLOCK;
    } else if (mode == UART_MODE_RD_NONBLOCK) {
        udd->flags &= ~UART_FLG_RD_BLOCK;
        (void)LOS_EventWrite(&udd->wait.stEvent, 0x1);
    }
    return HDF_SUCCESS;
}

static int32_t Hi35xxInit(struct UartHost *host)
{
    int32_t ret = 0;
    struct UartDriverData *udd = NULL;
    struct wait_queue_head *wait = NULL;
    if (host == NULL || host->priv == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udd = (struct UartDriverData *)host->priv;
    wait = &udd->wait;
    if (udd->state == UART_STATE_NOT_OPENED) {
        udd->state = UART_STATE_OPENING;
        (void)LOS_EventInit(&wait->stEvent);
        spin_lock_init(&wait->lock);
        LOS_ListInit(&wait->poll_queue);
        udd->rxTransfer = (struct UartTransfer *)OsalMemCalloc(sizeof(struct UartTransfer));
        if (udd->rxTransfer == NULL) {
            HDF_LOGE("%s: alloc transfer failed", __func__);
            return HDF_ERR_MALLOC_FAIL;
        }
        if (udd->ops->StartUp == NULL) {
            HDF_LOGE("%s: not support", __func__);
            ret = HDF_ERR_NOT_SUPPORT;
            goto FREE_TRANSFER;
        }
        if (udd->ops->StartUp(udd) != HDF_SUCCESS) {
            HDF_LOGE("%s: StartUp failed", __func__);
            ret = HDF_FAILURE;
            goto FREE_TRANSFER;
        }
    }
    udd->state = UART_STATE_USEABLE;
    udd->count++;
    return HDF_SUCCESS;

FREE_TRANSFER:
    (void)OsalMemFree(udd->rxTransfer);
    udd->rxTransfer = NULL;
    return ret;
}

static int32_t Hi35xxDeinit(struct UartHost *host)
{
    struct wait_queue_head *wait = NULL;
    struct UartDriverData *udd = NULL;
    if (host == NULL || host->priv == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    udd = (struct UartDriverData *)host->priv;
    if ((--udd->count) != 0) {
        return HDF_SUCCESS;
    }
    wait = &udd->wait;
    if (udd->flags & UART_FLG_DMA_RX) {
        if (udd->ops->DmaShutDown != NULL) {
            udd->ops->DmaShutDown(udd, UART_DMA_DIR_RX);
        }
    }
    if (udd->flags & UART_FLG_DMA_TX) {
        if (udd->ops->DmaShutDown != NULL) {
            udd->ops->DmaShutDown(udd, UART_DMA_DIR_TX);
        }
    }
    LOS_ListDelete(&wait->poll_queue);
    LOS_EventDestroy(&wait->stEvent);
    if (udd->ops->ShutDown != NULL) {
        udd->ops->ShutDown(udd);
    }
    if (udd->rxTransfer != NULL) {
        (void)OsalMemFree(udd->rxTransfer);
        udd->rxTransfer = NULL;
    }
    udd->state = UART_STATE_NOT_OPENED;
    return HDF_SUCCESS;
}

static int32_t Hi35xxPollEvent(struct UartHost *host, void *filep, void *table)
{
    struct UartDriverData *udd = NULL;

    if (host == NULL || host->priv == NULL) {
        HDF_LOGE("%s: host is NULL", __func__);
        return HDF_FAILURE;
    }
    udd = (struct UartDriverData *)host->priv;
    if (UART_STATE_USEABLE != udd->state) {
        return -EFAULT;
    }

    poll_wait((struct file *)filep, &udd->wait, (poll_table *)table);

    if (!PL011UartRxBufEmpty(udd)) {
        return POLLIN | POLLRDNORM;
    }
    return 0;
}

struct UartHostMethod g_uartHostMethod = {
    .Init = Hi35xxInit,
    .Deinit = Hi35xxDeinit,
    .Read = Hi35xxRead,
    .Write = Hi35xxWrite,
    .SetBaud = Hi35xxSetBaud,
    .GetBaud = Hi35xxGetBaud,
    .SetAttribute = Hi35xxSetAttribute,
    .GetAttribute = Hi35xxGetAttribute,
    .SetTransMode = Hi35xxSetTransMode,
    .pollEvent = Hi35xxPollEvent,
};

static int32_t UartGetConfigFromHcs(struct UartPl011Port *port, const struct DeviceResourceNode *node)
{
    uint32_t tmp, regPbase, iomemCount;
    struct UartDriverData *udd = port->udd;
    struct DeviceResourceIface *iface = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE); 
    if (iface == NULL || iface->GetUint32 == NULL) {
        HDF_LOGE("%s: face is invalid", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "num", &udd->num, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read busNum fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "baudrate", &udd->baudrate, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read numCs fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "fifoRxEn", &tmp, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read speed fail", __func__);
        return HDF_FAILURE;
    }
    udd->attr.fifoRxEn = tmp;
    if (iface->GetUint32(node, "fifoTxEn", &tmp, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read fifoSize fail", __func__);
        return HDF_FAILURE;
    }
    udd->attr.fifoTxEn = tmp;
    if (iface->GetUint32(node, "flags", &udd->flags, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read clkRate fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "regPbase", &regPbase, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read mode fail", __func__);
        return HDF_FAILURE;
    }
    if (iface->GetUint32(node, "iomemCount", &iomemCount, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read bitsPerWord fail", __func__);
        return HDF_FAILURE;
    }
    port->physBase = (unsigned long)OsalIoRemap(regPbase, iomemCount);
    if (iface->GetUint32(node, "interrupt", &port->irqNum, 0) != HDF_SUCCESS) {
        HDF_LOGE("%s: read comMode fail", __func__);
        return HDF_FAILURE;
    }
    return 0;
}

static int32_t Hi35xxAttach(struct UartHost *host, struct HdfDeviceObject *device)
{
    int32_t ret;
    struct UartDriverData *udd = NULL;
    struct UartPl011Port *port = NULL;

    if (device->property == NULL) {
        HDF_LOGE("%s: property is null", __func__);
        return HDF_FAILURE;
    }
    udd = (struct UartDriverData *)OsalMemCalloc(sizeof(*udd));
    if (udd == NULL) {
        HDF_LOGE("%s: OsalMemCalloc udd error", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    port = (struct UartPl011Port *)OsalMemCalloc(sizeof(struct UartPl011Port));
    if (port == NULL) {
        HDF_LOGE("%s: OsalMemCalloc port error", __func__);
        (void)OsalMemFree(udd);
        return HDF_ERR_MALLOC_FAIL;
    }
    udd->ops = Pl011GetOps();
    udd->recv = PL011UartRecvNotify;
    udd->count = 0;
    port->udd = udd;
    ret = UartGetConfigFromHcs(port, device->property);
    if (ret != 0 || port->physBase == 0) {
        (void)OsalMemFree(port);
        (void)OsalMemFree(udd);
        return HDF_FAILURE;
    }
    udd->private = port;
    host->priv = udd;
    host->num = udd->num;
    UartAddDev(host);
    return HDF_SUCCESS;
}

static void Hi35xxDetach(struct UartHost *host)
{
    struct UartDriverData *udd = NULL;
    struct UartPl011Port *port = NULL;

    if (host->priv == NULL) {
        HDF_LOGE("%s: invalid parameter", __func__);
        return;
    }
    udd = host->priv;
    if (udd->state != UART_STATE_NOT_OPENED) {
        HDF_LOGE("%s: uart driver data state invalid", __func__);
        return;
    }
    UartRemoveDev(host);
    port = udd->private;
    if (port != NULL) {
        if (port->physBase != 0) {
            OsalIoUnmap((void *)port->physBase);
        }
        (void)OsalMemFree(port);
        udd->private = NULL;
    }
    (void)OsalMemFree(udd);
    host->priv = NULL;
}

static int32_t HdfUartDeviceBind(struct HdfDeviceObject *device)
{
    HDF_LOGI("%s: entry", __func__);
    if (device == NULL) {
        return HDF_ERR_INVALID_OBJECT;
    }
    return (UartHostCreate(device) == NULL) ? HDF_FAILURE : HDF_SUCCESS;
}

int32_t HdfUartDeviceInit(struct HdfDeviceObject *device)
{
    int32_t ret;
    struct UartHost *host = NULL;

    HDF_LOGI("%s: entry", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    host = UartHostFromDevice(device);
    if (host == NULL) {
        HDF_LOGE("%s: host is null", __func__);
        return HDF_FAILURE;
    }
    ret = Hi35xxAttach(host, device);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: attach error", __func__);
        return HDF_FAILURE;
    }
    host->method = &g_uartHostMethod;
    return ret;
}

void HdfUartDeviceRelease(struct HdfDeviceObject *device)
{
    struct UartHost *host = NULL;

    HDF_LOGI("%s: entry", __func__);
    if (device == NULL) {
        HDF_LOGE("%s: device is null", __func__);
        return;
    }
    host = UartHostFromDevice(device);
    if (host == NULL) {
        HDF_LOGE("%s: host is null", __func__);
        return;
    }
    if (host->priv != NULL) {
        Hi35xxDetach(host);
    }
    UartHostDestroy(host);
}

struct HdfDriverEntry g_hdfUartDevice = {
    .moduleVersion = 1,
    .moduleName = "HDF_PLATFORM_UART",
    .Bind = HdfUartDeviceBind,
    .Init = HdfUartDeviceInit,
    .Release = HdfUartDeviceRelease,
};

HDF_INIT(g_hdfUartDevice);
