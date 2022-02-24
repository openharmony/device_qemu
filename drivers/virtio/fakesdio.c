/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "device_resource_if.h"
#include "mmc_corex.h"
#include "mmc_sdio.h"
#include "gpio_core.h"

#define HDF_LOG_TAG sdio_adapter_c

static int32_t FakeSdioSetBlockSize(struct SdioDevice *dev, uint32_t blockSize)
{
    (void)dev;
    (void)blockSize;
    return HDF_SUCCESS;
}

static int32_t FakeSdioGetCommonInfo(struct SdioDevice *dev, SdioCommonInfo *info, uint32_t infoType)
{
    if (info == NULL) {
        HDF_LOGE("[%s]info is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (infoType != SDIO_FUNC_INFO) {
        HDF_LOGE("[%s]info type %u is not supported", __func__, infoType);
        return HDF_ERR_NOT_SUPPORT;
    }

    info->funcInfo.enTimeout = 0;
    info->funcInfo.funcNum = 1;
    info->funcInfo.irqCap = 0;
    info->funcInfo.data = NULL;
    return HDF_SUCCESS;
}

static int32_t FakeSdioSetCommonInfo(struct SdioDevice *dev, SdioCommonInfo *info, uint32_t infoType)
{
    (void)dev;
    if (info == NULL) {
        HDF_LOGE("[%s]info is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }
    if (infoType != SDIO_FUNC_INFO) {
        HDF_LOGE("[%s]info type %u is not supported", __func__, infoType);
        return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}

static int32_t FakeSdioEnableFunc(struct SdioDevice *dev)
{
    (void)dev;
    return HDF_SUCCESS;
}

static int32_t FakeSdioFindFunc(struct SdioDevice *dev, struct SdioFunctionConfig *configData)
{
    if (dev == NULL || configData == NULL) {
        HDF_LOGE("[%s]dev or configData is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    return HDF_SUCCESS;
}

static struct SdioDeviceOps g_fakeSdioDeviceOps = {
    .setBlockSize = FakeSdioSetBlockSize,
    .getCommonInfo = FakeSdioGetCommonInfo,
    .setCommonInfo = FakeSdioSetCommonInfo,
    .enableFunc = FakeSdioEnableFunc,
    .findFunc = FakeSdioFindFunc,
};

static void FakeSdioDeleteCntlr(struct MmcCntlr *cntlr)
{
    if (cntlr == NULL) {
        return;
    }
    if (cntlr->curDev != NULL) {
        MmcDeviceRemove(cntlr->curDev);
        OsalMemFree(cntlr->curDev);
    }
    MmcCntlrRemove(cntlr);
    OsalMemFree(cntlr);
}

static int32_t FakeSdioCntlrParse(struct MmcCntlr *cntlr, struct HdfDeviceObject *obj)
{
    const struct DeviceResourceNode *node = NULL;
    struct DeviceResourceIface *drsOps = NULL;
    int32_t ret;

    if (obj == NULL || cntlr == NULL) {
        HDF_LOGE("[%s]input para is NULL", __func__);
        return HDF_FAILURE;
    }

    node = obj->property;
    if (node == NULL) {
        HDF_LOGE("[%s]HdfDeviceObject property is NULL", __func__);
        return HDF_FAILURE;
    }
    drsOps = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);
    if (drsOps == NULL || drsOps->GetUint16 == NULL || drsOps->GetUint32 == NULL) {
        HDF_LOGE("[%s]get HDF_CONFIG_SOURCE failed", __func__);
        return HDF_FAILURE;
    }

    ret = drsOps->GetUint16(node, "hostId", &(cntlr->index), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]read hostIndex failed: %d", __func__, ret);
        return ret;
    }
    ret = drsOps->GetUint32(node, "devType", &(cntlr->devType), 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]read devType failed: %d", __func__, ret);
        return ret;
    }

    return HDF_SUCCESS;
}

static int32_t FakeSdioRescan(struct MmcCntlr *cntlr)
{
    if (cntlr == NULL) {
        HDF_LOGE("[%s]cntlr is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }
    return HDF_SUCCESS;
}

static struct MmcCntlrOps g_fakeSdioCntlrOps = {
    .rescanSdioDev = FakeSdioRescan,
};

static int32_t FakeSdioBind(struct HdfDeviceObject *obj)
{
    struct MmcCntlr *cntlr = NULL;
    int32_t ret;

    if (obj == NULL) {
        HDF_LOGE("[%s]HdfDeviceObject is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr = OsalMemCalloc(sizeof(struct MmcCntlr));
    if (cntlr == NULL) {
        HDF_LOGE("[%s]alloc MmcCntlr failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    cntlr->ops = &g_fakeSdioCntlrOps;
    cntlr->hdfDevObj = obj;
    obj->service = &cntlr->service;
    if ((ret = FakeSdioCntlrParse(cntlr, obj)) != HDF_SUCCESS) {
        goto ERR_OUT;
    }

    if ((ret = MmcCntlrAdd(cntlr)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]add MmcCntlr failed: %d", __func__, ret);
        goto ERR_OUT;
    }

    if ((ret = MmcCntlrAllocDev(cntlr, (enum MmcDevType)cntlr->devType)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]alloc dev failed: %d", __func__, ret);
        goto ERR_OUT;
    }
    MmcDeviceAddOps(cntlr->curDev, &g_fakeSdioDeviceOps);

    return HDF_SUCCESS;

ERR_OUT:
    FakeSdioDeleteCntlr(cntlr);
    return ret;
}

static int32_t FakeSdioInit(struct HdfDeviceObject *obj)
{
    (void)obj;
    return HDF_SUCCESS;
}

static void FakeSdioRelease(struct HdfDeviceObject *obj)
{
    if (obj == NULL) {
        return;
    }
    FakeSdioDeleteCntlr((struct MmcCntlr *)obj->service);
}

struct HdfDriverEntry g_fakeSdioEntry = {
    .moduleVersion = 1,
    .Bind = FakeSdioBind,
    .Init = FakeSdioInit,
    .Release = FakeSdioRelease,
    .moduleName = "HDF_PLATFORM_SDIO",
};
HDF_INIT(g_fakeSdioEntry);


/*
 * WIFI depends on SDIO & GPIO. HDF defined HdfWlanConfigSDIO interface,
 * but user must implement it. Also, we add a dummy GPIO controller here.
 */

static int32_t FakeGiopDummyOps0(struct GpioCntlr *cntlr, uint16_t local, uint16_t dir)
{
    (void)cntlr;
    (void)local;
    (void)dir;
    return HDF_SUCCESS;
}

static struct GpioMethod g_fakeGpioOps = {
    .write = FakeGiopDummyOps0,
    .setDir = FakeGiopDummyOps0,
};

int32_t HdfWlanConfigSDIO(uint8_t busId)
{
    (void)busId;
    int32_t ret;
    struct GpioCntlr *cntlr = OsalMemCalloc(sizeof(struct GpioCntlr));

    if (cntlr == NULL) {
        HDF_LOGE("[%s]alloc memory failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    cntlr->count = 1;
    cntlr->ops = &g_fakeGpioOps;
    if ((ret = GpioCntlrAdd(cntlr)) != HDF_SUCCESS) {
        OsalMemFree(cntlr);
    }

    return ret;
}

