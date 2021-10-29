/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef HDF_SYSCALL_ADAPTER_H
#define HDF_SYSCALL_ADAPTER_H

#include <osal_mutex.h>
#include <hdf_dlist.h>
#include "hdf_io_service.h"
#include "hdf_device_desc.h"

struct HdfSyscallAdapter;

struct HdfSyscallAdapter {
    struct HdfDeviceObject deviceObject;
    struct HdfIoService super;
    struct HdfDeviceIoClient client;
    struct OsalMutex mutex;
    struct DListHead listenerList;
};

struct HdfDeviceObject *GetHdfDeviceObject();

#endif /* HDF_SYSCALL_ADAPTER_H */
