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
#include "hdf_log.h"
#include "hdf_base.h"
#include "hdf_device_desc.h"
#include "device_resource_if.h"
#include "los_vm_zone.h"
#include "cfiflash_internal.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static struct block_operations g_cfiBlkops = {
    NULL,           /* int     (*open)(struct Vnode *vnode); */
    NULL,           /* int     (*close)(struct Vnode *vnode); */
    CfiBlkRead,
    CfiBlkWrite,
    CfiBlkGeometry,
    NULL,           /* int     (*ioctl)(struct Vnode *vnode, int cmd, unsigned long arg); */
    NULL,           /* int     (*unlink)(struct Vnode *vnode); */
};

struct block_operations *GetCfiBlkOps()
{
    return &g_cfiBlkops;
}

static int HdfCfiMapInit(const struct DeviceResourceNode *node)
{
    int ret;
    uint32_t pbase;
    struct DeviceResourceIface *p = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if ((ret = p->GetUint32(node, "pbase", &pbase, 0))) {
        HDF_LOGE("[%s]GetUint32 error:%d", __func__, ret);
        return HDF_FAILURE;
    }

    g_cfiFlashBase = (uint32_t *)IO_DEVICE_ADDR(pbase);

    return HDF_SUCCESS;
}

int HdfCfiDriverInit(struct HdfDeviceObject *deviceObject)
{
    if (deviceObject == NULL ||  deviceObject->property == NULL) {
        HDF_LOGE("[%s]deviceObject or property is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if (HdfCfiMapInit(deviceObject->property) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    if(CfiFlashInit()) {
        return HDF_ERR_NOT_SUPPORT;
    }

    return HDF_SUCCESS;
}

struct HdfDriverEntry g_cfiDriverEntry = {
    .moduleVersion = 1,
    .moduleName = "cfi_flash_driver",
    .Bind = NULL,
    .Init = HdfCfiDriverInit,
    .Release = NULL,
};

HDF_INIT(g_cfiDriverEntry);

static struct MtdDev g_cfiMtdDev = {
    .priv = NULL,
    .type = MTD_NORFLASH,
    .size = CFIFLASH_CAPACITY,
    .eraseSize = CFIFLASH_ERASEBLK_SIZE,
    .erase = CfiMtdErase,
    .read = CfiMtdRead,
    .write = CfiMtdWrite,
};

struct MtdDev *GetCfiMtdDev()
{
    return &g_cfiMtdDev;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
