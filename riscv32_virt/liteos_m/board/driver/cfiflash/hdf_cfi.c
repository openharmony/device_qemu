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

#include "los_debug.h"
#include "hdf_log.h"
#include "hdf_device_desc.h"
#include "device_resource_if.h"
#include "cfiflash_internal.h"

int HdfCfiDriverInit(struct HdfDeviceObject *deviceObject)
{
    int ret, cfi_drv_idx = 0;
    uint32_t pbase;
    struct DeviceResourceIface *p = DeviceResourceGetIfaceInstance(HDF_CONFIG_SOURCE);

    if (deviceObject == NULL ||  deviceObject->property == NULL) {
        HDF_LOGE("[%s]deviceObject or property is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    if ((ret = p->GetUint32(deviceObject->property, "pbase0", &pbase, 0))) {
        HDF_LOGE("[%s]GetUint32 get cfi0 basee error:%d", __func__, ret);
        return HDF_FAILURE;
    }

    if (CfiFlashInit(cfi_drv_idx, pbase)) {
        return HDF_ERR_NOT_SUPPORT;
    }

    if ((ret = p->GetUint32(deviceObject->property, "pbase1", &pbase, 0))) {
        HDF_LOGE("[%s]GetUint32 get cfi1 basee error:%d", __func__, ret);
        return HDF_FAILURE;
    }

    cfi_drv_idx++;
    if (CfiFlashInit(cfi_drv_idx, pbase)) {
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
