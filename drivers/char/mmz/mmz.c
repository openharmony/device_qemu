/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "mmz.h"
#include "fcntl.h"
#include "fs/driver.h"
#include "los_vm_map.h"
#include "los_vm_phys.h"
#include "los_vm_lock.h"
#include "los_hw.h"
#include "los_atomic.h"
#include "los_vm_common.h"
#include "los_process_pri.h"
#include "target_config.h"

static int MmzOpen(struct file *filep)
{
    return 0;
}

static int MmzClose(struct file *filep)
{
    return 0;
}

static ssize_t MmzAlloc(int cmd, unsigned long arg)
{
    UINT32 vmFlags = VM_MAP_REGION_FLAG_PERM_USER |
                     VM_MAP_REGION_FLAG_PERM_READ |
                     VM_MAP_REGION_FLAG_PERM_WRITE;
    STATUS_T status;
    MmzMemory *mmzm = (MmzMemory *)arg;
    LosVmSpace *curVmSpace = OsCurrProcessGet()->vmSpace;
    LosVmMapRegion *vmRegion;
    VOID *kvaddr;
    PADDR_T paddr;
    VADDR_T vaddr;
    UINT32 size = ROUNDUP(mmzm->size, PAGE_SIZE);
    LosVmPage *vmPage = NULL;

    switch (cmd) {
        case MMZ_CACHE_TYPE:
            vmFlags |= VM_MAP_REGION_FLAG_CACHED;
            break;
        case MMZ_NOCACHE_TYPE:
            vmFlags |= VM_MAP_REGION_FLAG_UNCACHED;
            break;
        default:
            PRINT_ERR("%s %d: %d\n", __func__, __LINE__, cmd);
            return -EINVAL;
    }
    vmRegion = LOS_RegionAlloc(curVmSpace, 0, size, vmFlags, 0);
    if (vmRegion == NULL) {
        PRINT_ERR("cmd: %d, size: %#x vaddr alloc failed\n", cmd, size);
        return -ENOMEM;
    }

    kvaddr = (void *)LOS_PhysPagesAllocContiguous(size >> PAGE_SHIFT);
    if (kvaddr == NULL) {
        LOS_RegionFree(curVmSpace, vmRegion);
        PRINT_ERR("size: %#x paddr alloc failed\n", size);
        return -ENOMEM;
    }

    paddr = LOS_PaddrQuery(kvaddr);
    mmzm->paddr = paddr;
    vaddr = vmRegion->range.base;
    while (size > 0) {
        vmPage = LOS_VmPageGet(paddr);
        if (vmPage == NULL) {
            LOS_RegionFree(curVmSpace, vmRegion);
            VM_ERR("Page is NULL");
            return -EINVAL;
        }
        LOS_AtomicInc(&vmPage->refCounts);

        status = LOS_ArchMmuMap(&curVmSpace->archMmu, vaddr, paddr, 1, vmFlags);
        if (status <= 0) {
            VM_ERR("LOS_ArchMmuMap failed: %d", status);
            LOS_RegionFree(curVmSpace, vmRegion);
            return status;
        }

        paddr += PAGE_SIZE;
        vaddr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    mmzm->vaddr = (void *)vmRegion->range.base;
    return LOS_OK;
}

static ssize_t MmzMap(int cmd, unsigned long arg)
{
    UINT32 vmFlags = VM_MAP_REGION_FLAG_PERM_USER |
                     VM_MAP_REGION_FLAG_PERM_READ |
                     VM_MAP_REGION_FLAG_PERM_WRITE;
    MmzMemory *mmzm = (MmzMemory *)arg;
    LosVmSpace *curVmSpace = OsCurrProcessGet()->vmSpace;
    LosVmMapRegion *vmRegion = NULL;
    STATUS_T status = LOS_OK;
    PADDR_T paddr = (PADDR_T)mmzm->paddr;
    UINT32 size = ROUNDUP(mmzm->size, PAGE_SIZE);
    VADDR_T vaddr;
    LosVmPage *vmPage = NULL;

    switch (cmd) {
        case MMZ_MAP_CACHE_TYPE:
            vmFlags |= VM_MAP_REGION_FLAG_CACHED;
            break;
        case MMZ_MAP_NOCACHE_TYPE:
            vmFlags |= VM_MAP_REGION_FLAG_UNCACHED;
            break;
        default:
            PRINT_ERR("%s %d: %d\n", __func__, __LINE__, cmd);
            return -EINVAL;
    }

    vmRegion = LOS_RegionAlloc(curVmSpace, 0, size, vmFlags, 0);
    if (vmRegion == NULL) {
        PRINT_ERR("cmd: %d, size: %#x vaddr alloc failed\n", cmd, size);
        return -ENOMEM;
    }

    mmzm->vaddr = (void *)vmRegion->range.base;
    vaddr = vmRegion->range.base;
    while (size > 0) {
        vmPage = LOS_VmPageGet(paddr);
        if (vmPage == NULL) {
            LOS_RegionFree(curVmSpace, vmRegion);
            VM_ERR("Page is NULL");
            return -EINVAL;
        }
        LOS_AtomicInc(&vmPage->refCounts);

        status = LOS_ArchMmuMap(&curVmSpace->archMmu, vaddr, paddr, 1,
                vmRegion->regionFlags);
        if (status <= 0) {
            VM_ERR("LOS_ArchMmuMap failed: %d", status);
            LOS_RegionFree(curVmSpace, vmRegion);
            return status;
        }

        paddr += PAGE_SIZE;
        vaddr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }

    return status;
}

static ssize_t MmzUnMap(unsigned long arg)
{
    LosVmSpace *curVmSpace = OsCurrProcessGet()->vmSpace;
    MmzMemory *mmzm = (MmzMemory *)arg;
    return OsUnMMap(curVmSpace, (VADDR_T)mmzm->vaddr, mmzm->size);
}

static ssize_t MmzFree(unsigned long arg)
{
    MmzMemory *mmzm = (MmzMemory *)arg;
    LosVmSpace *curVmSpace = OsCurrProcessGet()->vmSpace;
    LosVmMapRegion *region = NULL;
    STATUS_T ret = LOS_OK;

    (VOID)LOS_MuxAcquire(&curVmSpace->regionMux);
    if ((PADDR_T)mmzm->paddr != LOS_PaddrQuery(mmzm->vaddr)) {
        PRINT_ERR("vaddr is not equal to paddr");
        ret = -EINVAL;
        goto DONE;
    }

    region = LOS_RegionFind(curVmSpace, (VADDR_T)(unsigned int)mmzm->vaddr);
    if (region == NULL) {
        PRINT_ERR("find region failed");
        ret = -EINVAL;
        goto DONE;
    }

    ret = LOS_RegionFree(curVmSpace, region);
    if (ret) {
        PRINT_ERR("free region failed, ret = %d", ret);
        ret = -EINVAL;
    }

DONE:
    (VOID)LOS_MuxRelease(&curVmSpace->regionMux);
    return ret;
}

static ssize_t MmzFlush(unsigned long arg)
{
    MmzMemory *mmzm = (MmzMemory *)arg;
    DCacheFlushRange((unsigned int)mmzm->vaddr, (UINT32)mmzm->vaddr + mmzm->size);
    return LOS_OK;
}

static ssize_t MmzInvalidate(unsigned long arg)
{
    MmzMemory *mmzm = (MmzMemory *)arg;
    DCacheInvRange((unsigned int)mmzm->vaddr, (UINT32)mmzm->vaddr + mmzm->size);
    return LOS_OK;
}

static ssize_t MmzIoctl(struct file *filep, int cmd, unsigned long arg)
{
    switch (cmd) {
        case MMZ_CACHE_TYPE:
        case MMZ_NOCACHE_TYPE:
            return MmzAlloc(cmd, arg);
        case MMZ_FREE_TYPE:
            return MmzFree(arg);
        case MMZ_MAP_CACHE_TYPE:
        case MMZ_MAP_NOCACHE_TYPE:
            return MmzMap(cmd, arg);
        case MMZ_UNMAP_TYPE:
            return MmzUnMap(arg);
        case MMZ_FLUSH_CACHE_TYPE:
        case MMZ_FLUSH_NOCACHE_TYPE:
            return MmzFlush(arg);
        case MMZ_INVALIDATE_TYPE:
            return MmzInvalidate(arg);
        default:
            PRINT_ERR("%s %d: %d\n", __func__, __LINE__, cmd);
            return -EINVAL;
    }
    return LOS_OK;
}

static const struct file_operations_vfs g_mmzDevOps = {
    .open = MmzOpen, /* open */
    .close = MmzClose, /* close */
    .ioctl = MmzIoctl, /* ioctl */
};

int DevMmzRegister(void)
{
    return register_driver(MMZ_NODE, &g_mmzDevOps, 0666, 0); /* 0666: file mode */
}
