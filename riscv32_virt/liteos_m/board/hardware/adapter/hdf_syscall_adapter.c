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

#include "hdf_syscall_adapter.h"
#include "hdf_log.h"
#include "hdf_sbuf.h"
#include "devsvc_manager_clnt.h"

#define HDF_LOG_TAG hdf_syscall_adapter

int HdfSyscallAdapterDispatch (struct HdfObject *service, int cmdId, struct HdfSBuf *data, struct HdfSBuf *reply)
{
    struct HdfIoService *ioService = (struct HdfIoService*)service;
    struct HdfSyscallAdapter *adapter = NULL;

    if (ioService == NULL || ioService->dispatcher == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    adapter = CONTAINER_OF(ioService, struct HdfSyscallAdapter, super);

    if (adapter->client.device == NULL || adapter->client.device->service == NULL ||
        adapter->client.device->service->Dispatch == NULL) {
            return HDF_ERR_INVALID_OBJECT;
    }

    return adapter->client.device->service->Dispatch(&adapter->client, cmdId, data, reply);
}

static struct HdfSyscallAdapter *HdfSyscallAdapterInstance(struct HdfDeviceObject *deviceObject)
{
    struct HdfSyscallAdapter *adapter = NULL;

    static struct HdfIoDispatcher kDispatcher = {
        .Dispatch = HdfSyscallAdapterDispatch,
    };

    adapter = CONTAINER_OF(deviceObject, struct HdfSyscallAdapter, deviceObject);
    
    if (adapter == NULL) {
        return NULL;
    }

    DListHeadInit(&adapter->listenerList);
    if (OsalMutexInit(&adapter->mutex)) {
        HDF_LOGE("%s: Failed to create mutex", __func__);
        OsalMemFree(adapter);
        return NULL;
    }

    if (deviceObject == NULL) {
        OsalMutexDestroy(&adapter->mutex);
        OsalMemFree(adapter);
        return NULL;
    }

    adapter->client.device = deviceObject;

    if (deviceObject->service != NULL && deviceObject->service->Open != NULL) {
        if (deviceObject->service->Open(&adapter->client) != HDF_SUCCESS) {
            OsalMutexDestroy(&adapter->mutex);
            OsalMemFree(adapter);
            return NULL;
        }
    }

    adapter->super.dispatcher = &kDispatcher;
    return adapter;
}

struct HdfIoService *HdfIoServiceAdapterObtain(const char *serviceName)
{
    struct HdfSyscallAdapter *adapter = NULL;
    struct HdfDeviceObject *deviceObject = NULL;

    int ret;

    if (serviceName == NULL) {
        return NULL;
    }

    ret = HdfLoadDriverByServiceName(serviceName);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("%s: load %s driver failed", __func__, serviceName);
        return NULL;
    }

    deviceObject = DevSvcManagerClntGetDeviceObject(serviceName);
    if (deviceObject == NULL) {
        return NULL;
    }

    adapter = HdfSyscallAdapterInstance(deviceObject);
    if (adapter == NULL) {
        return NULL;
    }

    return &adapter->super;
}

void HdfIoServiceAdapterRecycle(struct HdfIoService *ioService)
{
    struct HdfSyscallAdapter *adapter = NULL;

    if (ioService == NULL) {
        return;
    }

    adapter = CONTAINER_OF(ioService, struct HdfSyscallAdapter, super);
    if (adapter->client.device != NULL && adapter->client.device->service != NULL &&
        adapter->client.device->service->Release != NULL) {
        adapter->client.device->service->Release(&adapter->client);
    }
 
    OsalMutexDestroy(&adapter->mutex);
}

static bool AddListenerToAdapterLocked(struct HdfSyscallAdapter *adapter, struct HdfDevEventlistener *listener)
{
    struct HdfDevEventlistener *it = NULL;
    DLIST_FOR_EACH_ENTRY(it, &adapter->listenerList, struct HdfDevEventlistener, listNode) {
        if (it == listener) {
            HDF_LOGE("Add a listener for duplicate dev-event");
            return false;
        }
    }
    DListInsertTail(&listener->listNode, &adapter->listenerList);
    return true;
}

int32_t HdfDeviceRegisterEventListener(struct HdfIoService *target, struct HdfDevEventlistener *listener)
{
    if (target == NULL || listener == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    if (listener->callBack == NULL && listener->onReceive == NULL) {
        HDF_LOGE("Listener onReceive func not implemented");
        return HDF_ERR_INVALID_OBJECT;
    }

    struct HdfSyscallAdapter *adapter = CONTAINER_OF(target, struct HdfSyscallAdapter, super);
    
    OsalMutexLock(&adapter->mutex);
    if (!AddListenerToAdapterLocked(adapter, listener)) {
        OsalMutexUnlock(&adapter->mutex);
        return HDF_ERR_INVALID_PARAM;
    }
    OsalMutexUnlock(&adapter->mutex);
    return HDF_SUCCESS;
}

int32_t HdfDeviceUnregisterEventListener(struct HdfIoService *target, struct HdfDevEventlistener *listener)
{
    if (target == NULL || listener == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    if (listener->listNode.next == NULL || listener->listNode.prev == NULL) {
        HDF_LOGE("%s: broken listener, may double unregister", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    struct HdfSyscallAdapter *adapter = CONTAINER_OF(target, struct HdfSyscallAdapter, super);
    OsalMutexLock(&adapter->mutex);
    DListRemove(&listener->listNode);
    OsalMutexUnlock(&adapter->mutex);
    return HDF_SUCCESS;
}

int32_t HdfDeviceSendEvent(const struct HdfDeviceObject *deviceObject, uint32_t id, const struct HdfSBuf *data)
{
    const struct HdfSyscallAdapter *adapter = NULL;

    if (deviceObject == NULL || data == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    struct HdfDeviceObject *inputDriverObject = GetHdfDeviceObject();

    if (inputDriverObject == NULL) {
        return HDF_ERR_NOT_SUPPORT;
    }
 
    if (deviceObject == inputDriverObject) {
        adapter = CONTAINER_OF(deviceObject, struct HdfSyscallAdapter, deviceObject);
        struct HdfDevEventlistener *listener = NULL;
        DLIST_FOR_EACH_ENTRY(listener, &adapter->listenerList, struct HdfDevEventlistener, listNode) {
            OsalMutexLock(&adapter->mutex);
            if (listener->onReceive != NULL) {
                (void)listener->onReceive(listener, &adapter->super, id, data);
            } else if (listener->callBack != NULL) {
                (void)listener->callBack(listener->priv, id, data);
            }
            OsalMutexUnlock(&adapter->mutex);
        }        
    }
    return HDF_SUCCESS;
}
