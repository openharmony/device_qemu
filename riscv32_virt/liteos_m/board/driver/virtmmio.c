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
#include "los_arch_interrupt.h"
#include "los_interrupt.h"
#include "lwip/mem.h"
#include "virtmmio.h"

#define IRQF_SHARED 0

static inline uint32_t VirtioGetStatus(const struct VirtmmioDev *dev)
{
    return GET_UINT32(dev->base + VIRTMMIO_REG_STATUS);
}

static inline void VirtioAddStatus(const struct VirtmmioDev *dev, uint32_t val)
{
    FENCE_WRITE_UINT32(VirtioGetStatus(dev) | val, dev->base + VIRTMMIO_REG_STATUS);
}

static inline void VirtioResetStatus(const struct VirtmmioDev *dev)
{
    FENCE_WRITE_UINT32(VIRTIO_STATUS_RESET, dev->base + VIRTMMIO_REG_STATUS);
}

bool VirtmmioDiscover(uint32_t devId, struct VirtmmioDev *dev)
{
    VADDR_T base;
    int i;

    base = IO_DEVICE_ADDR(VIRTMMIO_BASE_ADDR) + VIRTMMIO_BASE_SIZE * (NUM_VIRTIO_TRANSPORTS - 1);
    for (i = NUM_VIRTIO_TRANSPORTS - 1; i >= 0; i--) {
        if ((GET_UINT32(base + VIRTMMIO_REG_MAGICVALUE) == VIRTMMIO_MAGIC) &&
            (GET_UINT32(base + VIRTMMIO_REG_VERSION) == VIRTMMIO_VERSION) &&
            (GET_UINT32(base + VIRTMMIO_REG_DEVICEID) == devId)) {
            dev->base = base;
            dev->irq = IRQ_SPI_BASE + VIRTMMIO_BASE_IRQ + i;
            return true;
        }

        base -= VIRTMMIO_BASE_SIZE;
    }

    PRINT_ERR("virtio-mmio ID=%u device not found\n", devId);
    return false;
}

unsigned VirtqSize(uint16_t qsz)
{
           /* pretend we do not have an aligned start address */
    return VIRTQ_ALIGN_DESC - 1 +
           ALIGN(sizeof(struct VirtqDesc) * qsz, VIRTQ_ALIGN_AVAIL) +
           ALIGN(sizeof(struct VirtqAvail) + sizeof(uint16_t) * qsz, VIRTQ_ALIGN_USED) +
           sizeof(struct VirtqUsed) + sizeof(struct VirtqUsedElem) * qsz;
}

void VirtmmioInitBegin(const struct VirtmmioDev *dev)
{
    VirtioResetStatus(dev);
    VirtioAddStatus(dev, VIRTIO_STATUS_ACK);
    VirtioAddStatus(dev, VIRTIO_STATUS_DRIVER);
    while ((VirtioGetStatus(dev) & VIRTIO_STATUS_DRIVER) == 0) { }
}

void VritmmioInitEnd(const struct VirtmmioDev *dev)
{
    VirtioAddStatus(dev, VIRTIO_STATUS_DRIVER_OK);
}

void VirtmmioInitFailed(const struct VirtmmioDev *dev)
{
    VirtioAddStatus(dev, VIRTIO_STATUS_FAILED);
}

static bool Negotiate(struct VirtmmioDev *baseDev, uint32_t nth, VirtioFeatureFn fn, void *dev)
{
    uint32_t features, supported, before, after;

    FENCE_WRITE_UINT32(nth, baseDev->base + VIRTMMIO_REG_DEVFEATURESEL);
    features = GET_UINT32(baseDev->base + VIRTMMIO_REG_DEVFEATURE);

    do {
        before = GET_UINT32(baseDev->base + VIRTMMIO_REG_CONFIGGENERATION);

        supported = 0;
        if (!fn(features, &supported, dev)) {
            return false;
        }

        after = GET_UINT32(baseDev->base + VIRTMMIO_REG_CONFIGGENERATION);
    } while (before != after);

    FENCE_WRITE_UINT32(nth, baseDev->base + VIRTMMIO_REG_DRVFEATURESEL);
    FENCE_WRITE_UINT32(supported, baseDev->base + VIRTMMIO_REG_DRVFEATURE);
    return true;
}

bool VirtmmioNegotiate(struct VirtmmioDev *baseDev, VirtioFeatureFn f0, VirtioFeatureFn f1, void *dev)
{
    if(!Negotiate(baseDev, VIRTIO_FEATURE_WORD0, f0, dev)) {
        return false;
    }

    if(!Negotiate(baseDev, VIRTIO_FEATURE_WORD1, f1, dev)) {
        return false;
    }

    VirtioAddStatus(baseDev, VIRTIO_STATUS_FEATURES_OK);
    if ((VirtioGetStatus(baseDev) & VIRTIO_STATUS_FEATURES_OK) == 0) {
        PRINT_ERR("negotiate features failed\n");
        return false;
    }

    return true;
}

#define U32_MASK        0xFFFFFFFF
#define U32_BYTES       4
#define U64_32_SHIFT    32
uint64_t u32_to_u64(uint32_t addr) {
    uint64_t paddr = (uint64_t)addr & U32_MASK;
    return paddr;
}
static void WriteQueueAddr(uint64_t addr, const struct VirtmmioDev *dev, uint32_t regLow)
{
    uint32_t paddr;

    paddr = addr & U32_MASK;
    FENCE_WRITE_UINT32(paddr, dev->base + regLow);
    paddr = addr >> U64_32_SHIFT;
    FENCE_WRITE_UINT32(paddr, dev->base + regLow + U32_BYTES);
}

static bool CompleteConfigQueue(uint32_t queue, const struct VirtmmioDev *dev)
{
    const struct Virtq *q = &dev->vq[queue];
    uint32_t num;

    FENCE_WRITE_UINT32(queue, dev->base + VIRTMMIO_REG_QUEUESEL);

    num = GET_UINT32(dev->base + VIRTMMIO_REG_QUEUEREADY);
    LOS_ASSERT(num == 0);
    num = GET_UINT32(dev->base + VIRTMMIO_REG_QUEUENUMMAX);
    if (num < q->qsz) {
        PRINT_ERR("queue %u not available: max qsz=%u, requested=%u\n", queue, num, q->qsz);
        return false;
    }

    FENCE_WRITE_UINT32(q->qsz, dev->base + VIRTMMIO_REG_QUEUENUM);
    WriteQueueAddr(u32_to_u64(q->desc), dev, VIRTMMIO_REG_QUEUEDESCLOW);
    WriteQueueAddr(u32_to_u64(q->avail), dev, VIRTMMIO_REG_QUEUEDRIVERLOW);
    WriteQueueAddr(u32_to_u64(q->used), dev, VIRTMMIO_REG_QUEUEDEVICELOW);

    FENCE_WRITE_UINT32(1, dev->base + VIRTMMIO_REG_QUEUEREADY);
    return true;
}

static VADDR_T CalculateQueueAddr(VADDR_T base, uint16_t qsz, struct Virtq *q)
{
    base = ALIGN(base, VIRTQ_ALIGN_DESC);
    q->desc = (struct VirtqDesc *)base;
    q->qsz = qsz;
    base = ALIGN(base + sizeof(struct VirtqDesc) * qsz, VIRTQ_ALIGN_AVAIL);
    q->avail = (struct VirtqAvail *)base;
    base = ALIGN(base + sizeof(struct VirtqAvail) + sizeof(uint16_t) * qsz, VIRTQ_ALIGN_USED);
    q->used = (struct VirtqUsed *)base;

    return base + sizeof(struct VirtqUsed) + sizeof(struct VirtqUsedElem) * qsz;
}

VADDR_T VirtmmioConfigQueue(struct VirtmmioDev *dev, VADDR_T base, uint16_t qsz[], int num)
{
    uint32_t i;

    for (i = 0; i < num; i++) {
        base = CalculateQueueAddr(base, qsz[i], &dev->vq[i]);
        if (!CompleteConfigQueue(i, dev)) {
            return 0;
        }
    }

    return base;
}

bool VirtmmioRegisterIRQ(struct VirtmmioDev *dev, HWI_PROC_FUNC handle, void *argDev, const char *devName)
{
    uint32_t ret;
    HwiIrqParam *param = mem_calloc(1, sizeof(HwiIrqParam));
    param->swIrq = dev->irq;
    param->pDevId = argDev;
    param->pName = devName;
    
    ret = LOS_HwiCreate(dev->irq, OS_HWI_PRIO_HIGHEST, IRQF_SHARED, handle, param);  
    if (ret != 0) {
        PRINT_ERR("virtio-mmio %s IRQ register failed: %u\n", devName, ret);
        return false;
    }

    HalIrqEnable(dev->irq);
    return true;
}
