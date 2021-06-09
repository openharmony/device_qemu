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
#include "disk.h"
#include "cfiflash_internal.h"

static struct block_operations g_cfiBlkops = {
    CfiBlkOpen,
    CfiBlkClose,
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

static void Setup2ndCfi(uint32_t pbase)
{
    uint8_t *vbase = (uint8_t *)IO_DEVICE_ADDR(pbase);
    if(CfiFlashInit((uint32_t *)vbase) == 0) {
        if (*(uint16_t *)&vbase[BS_SIG55AA] == BS_SIG55AA_VALUE) {
            INT32 id = los_alloc_diskid_byname(CFI_BLK_DRIVER);
            (void)los_disk_init(CFI_BLK_DRIVER, &g_cfiBlkops, vbase, id, NULL);
        }
    }
}

int HdfCfiDriverInit(struct HdfDeviceObject *deviceObject)
{
    int ret;
    uint32_t pbase;
    struct DeviceResourceIface *p = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if (deviceObject == NULL ||  deviceObject->property == NULL) {
        HDF_LOGE("[%s]deviceObject or property is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if ((ret = p->GetUint32(deviceObject->property, "pbase0", &pbase, 0))) {
        HDF_LOGE("[%s]GetUint32 error:%d", __func__, ret);
        return HDF_FAILURE;
    }
    g_cfiMtdDev.priv = (VOID *)IO_DEVICE_ADDR(pbase);
    if(CfiFlashInit(g_cfiMtdDev.priv)) {
        return HDF_ERR_NOT_SUPPORT;
    }

    if (p->GetUint32(deviceObject->property, "pbase1", &pbase, 0) == 0) {
        Setup2ndCfi(pbase);
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
