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
/*
 * Simple virtio-rng driver.
 * Any time, only one task can use it to get randoms.
 */

#include "osal.h"
#include "osal_io.h"
#include "dmac_core.h"
#include "los_vm_iomap.h"
#include "los_random.h"
#include "virtmmio.h"

#define VIRTQ_REQUEST_QSZ   1
#define VIRTMMIO_RNG_NAME   "virtrng"

struct Virtrng {
    struct VirtmmioDev      dev;

    OSAL_DECLARE_MUTEX(mutex);
    DmacEvent event;
};
static struct Virtrng *g_virtRng;

static bool Feature0(uint32_t features, uint32_t *supported, void *dev)
{
    (void)features;
    (void)supported;
    (void)dev;

    return true;
}

static bool Feature1(uint32_t features, uint32_t *supported, void *dev)
{
    (void)dev;
    if (features & VIRTIO_F_VERSION_1) {
        *supported |= VIRTIO_F_VERSION_1;
    } else {
        HDF_LOGE("[%s]virtio-rng has no VERSION_1 feature", __func__);
        return false;
    }

    return true;
}

static int VirtrngIO(char *buffer, size_t buflen)
{
    struct Virtq *q = &g_virtRng->dev.vq[0];
    int32_t ret;

    if ((ret = OsalMutexLock(&g_virtRng->mutex)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]acquire mutex failed: %#x", __func__, ret);
        return -1;
    }

    q->desc[0].pAddr = VMM_TO_DMA_ADDR((VADDR_T)buffer);
    q->desc[0].len = buflen;
    q->desc[0].flag = VIRTQ_DESC_F_WRITE;
    q->avail->ring[q->avail->index % q->qsz] = 0;
    DSB;
    q->avail->index++;
    OSAL_WRITEL(0, g_virtRng->dev.base + VIRTMMIO_REG_QUEUENOTIFY);

    if ((ret = DmaEventWait(&g_virtRng->event, 1, HDF_WAIT_FOREVER)) != 1) {
        HDF_LOGE("[%s]wait event failed: %#x", __func__, ret);
        ret = -1;
    } else {
        ret = q->used->ring[0].len;  /* actual randoms acquired */
    }

    (void)OsalMutexUnlock(&g_virtRng->mutex);
    return ret;
}

static uint32_t VirtrngIRQhandle(uint32_t swIrq, void *dev)
{
    (void)swIrq;
    (void)dev;
    struct Virtq *q = &g_virtRng->dev.vq[0];

    if (!(OSAL_READL(g_virtRng->dev.base + VIRTMMIO_REG_INTERRUPTSTATUS) & VIRTMMIO_IRQ_NOTIFY_USED)) {
        return 1;
    }

    (void)DmaEventSignal(&g_virtRng->event, 1);
    q->last++;

    OSAL_WRITEL(VIRTMMIO_IRQ_NOTIFY_USED, g_virtRng->dev.base + VIRTMMIO_REG_INTERRUPTACK);
    return 0;
}

static void VirtrngDeInit(struct Virtrng *rng)
{
    if (rng->dev.irq & ~_IRQ_MASK) {
        OsalUnregisterIrq(rng->dev.irq & _IRQ_MASK, rng);
    }
    if (rng->mutex.realMutex) {
        OsalMutexDestroy(&rng->mutex);
    }
    LOS_DmaMemFree(rng);
    g_virtRng = NULL;
}

static int VirtrngInitDevAux(struct Virtrng *rng)
{
    int32_t ret;

    if ((ret = OsalMutexInit(&rng->mutex)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]initialize mutex failed: %d", __func__, ret);
        return ret;
    }

    if ((ret = DmaEventInit(&rng->event)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]initialize event control block failed: %u", __func__, ret);
        return ret;
    }

    ret = OsalRegisterIrq(rng->dev.irq, OSAL_IRQF_TRIGGER_NONE,
                          (OsalIRQHandle)VirtrngIRQhandle, VIRTMMIO_RNG_NAME, rng);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]register IRQ failed: %d", __func__, ret);
        return ret;
    }
    rng->dev.irq |= ~_IRQ_MASK;

    return HDF_SUCCESS;
}

static struct Virtrng *VirtrngInitDev(void)
{
    struct Virtrng *rng = NULL;
    VADDR_T base;
    uint16_t qsz;
    int32_t len;

    /* NOTE: For simplicity, alloc all these data from physical continuous memory. */
    len = sizeof(struct Virtrng) + VirtqSize(VIRTQ_REQUEST_QSZ);
    rng = LOS_DmaMemAlloc(NULL, len, sizeof(UINTPTR), DMA_CACHE);
    if (rng == NULL) {
        HDF_LOGE("[%s]alloc rng memory failed", __func__);
        return NULL;
    }
    (void)memset_s(rng, len, 0, len);

    if (!VirtmmioDiscover(VIRTMMIO_DEVICE_ID_RNG, &rng->dev)) {
        goto ERR_OUT;
    }

    VirtmmioInitBegin(&rng->dev);

    if (!VirtmmioNegotiate(&rng->dev, Feature0, Feature1, rng)) {
        goto ERR_OUT1;
    }

    base = ALIGN((VADDR_T)rng + sizeof(struct Virtrng), VIRTQ_ALIGN_DESC);
    qsz = VIRTQ_REQUEST_QSZ;
    if (VirtmmioConfigQueue(&rng->dev, base, &qsz, 1) == 0) {
        goto ERR_OUT1;
    }

    if (VirtrngInitDevAux(rng) != HDF_SUCCESS) {
        goto ERR_OUT1;
    }

    VritmmioInitEnd(&rng->dev);
    return rng;

ERR_OUT1:
    VirtmmioInitFailed(&rng->dev);
ERR_OUT:
    VirtrngDeInit(rng);
    return NULL;
}


/*
 * random_hw code
 */

static int VirtrngSupport(void)
{
    return 1;
}

static void VirtrngOpen(void)
{
}

static void VirtrngClose(void)
{
}

static int VirtrngRead(char *buffer, size_t bytes)
{
    char *newbuf = buffer;
    int len;

    if (LOS_IsUserAddressRange((VADDR_T)buffer, bytes)) {
        newbuf = OsalMemAlloc(bytes);
        if (newbuf == NULL) {
            HDF_LOGE("[%s]alloc memory failed", __func__);
            return -1;
        }
    } else if ((VADDR_T)buffer + bytes < (VADDR_T)buffer) {
        HDF_LOGE("[%s]invalid argument: buffer=%p, size=%#x\n", __func__, buffer, bytes);
        return -1;
    }

    len = VirtrngIO(newbuf, bytes);

    if (newbuf != buffer) {
        if ((len > 0) && (LOS_ArchCopyToUser(buffer, newbuf, len)) != 0) {
            HDF_LOGE("[%s]LOS_ArchCopyToUser error\n", __func__);
            len = -1;
        }

        (void)OsalMemFree(newbuf);
    }

    return len;
}

void VirtrngInit(void)
{
    if ((g_virtRng = VirtrngInitDev()) == NULL) {
        return;
    }

    int ret;
    RandomOperations r = {
        .support = VirtrngSupport,
        .init = VirtrngOpen,
        .deinit = VirtrngClose,
        .read = VirtrngRead,
    };
    RandomOperationsInit(&r);
    if ((ret = DevUrandomRegister()) != 0) {
        HDF_LOGE("[%s]register /dev/urandom failed: %#x", __func__, ret);
        VirtrngDeInit(g_virtRng);
    }
}


/*
 * When kernel decoupled with specific devices,
 * these code can be removed.
 */
void HiRandomHwInit(void) {}
void HiRandomHwDeinit(void) {}
int HiRandomHwGetInteger(unsigned *result)
{
    /* kernel call this too early in mount.c */
    if (g_virtRng == NULL) {
        *result = 1;
        return sizeof(unsigned);
    }

    return VirtrngRead((char*)result, sizeof(unsigned));
}
int HiRandomHwGetNumber(char *buffer, size_t buflen)
{
    return VirtrngRead(buffer, buflen);
}
