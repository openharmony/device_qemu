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
#ifndef __VIRTMMIO_H__
#define __VIRTMMIO_H__

/* NOTE: Only support non-legacy device and little-endian guest. */

#include "stdint.h"
#include "los_base.h"
#include "los_typedef.h"
#include "los_hwi.h"

#define VIRTIO_STATUS_RESET                 0
#define VIRTIO_STATUS_ACK                   1
#define VIRTIO_STATUS_DRIVER                2
#define VIRTIO_STATUS_DRIVER_OK             4
#define VIRTIO_STATUS_FEATURES_OK           8
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET    64
#define VIRTIO_STATUS_FAILED                128

#define VIRTIO_FEATURE_WORD0                0
#define VIRTIO_F_RING_INDIRECT_DESC         (1 << 28)
#define VIRTIO_FEATURE_WORD1                1
#define VIRTIO_F_VERSION_1                  (1 << 0)

#define VIRTMMIO_REG_MAGICVALUE             0x00
#define VIRTMMIO_REG_VERSION                0x04
#define VIRTMMIO_REG_DEVICEID               0x08
#define VIRTMMIO_REG_DEVFEATURE             0x10
#define VIRTMMIO_REG_DEVFEATURESEL          0x14
#define VIRTMMIO_REG_DRVFEATURE             0x20
#define VIRTMMIO_REG_DRVFEATURESEL          0x24
#define VIRTMMIO_REG_QUEUESEL               0x30
#define VIRTMMIO_REG_QUEUENUMMAX            0x34
#define VIRTMMIO_REG_QUEUENUM               0x38
#define VIRTMMIO_REG_QUEUEREADY             0x44
#define VIRTMMIO_REG_QUEUENOTIFY            0x50
#define VIRTMMIO_REG_INTERRUPTSTATUS        0x60
#define VIRTMMIO_REG_INTERRUPTACK           0x64
#define VIRTMMIO_REG_STATUS                 0x70
#define VIRTMMIO_REG_QUEUEDESCLOW           0x80
#define VIRTMMIO_REG_QUEUEDESCHIGH          0x84
#define VIRTMMIO_REG_QUEUEDRIVERLOW         0x90
#define VIRTMMIO_REG_QUEUEDRIVERHIGH        0x94
#define VIRTMMIO_REG_QUEUEDEVICELOW         0xA0
#define VIRTMMIO_REG_QUEUEDEVICEHIGH        0xA4
#define VIRTMMIO_REG_CONFIGGENERATION       0xFC
#define VIRTMMIO_REG_CONFIG                 0x100

#define VIRTQ_ALIGN_DESC                    16
#define VIRTQ_ALIGN_AVAIL                   2
#define VIRTQ_ALIGN_USED                    4

#define VIRTMMIO_MAGIC                      0x74726976
#define VIRTMMIO_VERSION                    2
#define VIRTMMIO_DEVICE_ID_NET              1
#define VIRTMMIO_DEVICE_ID_BLK              2
#define VIRTMMIO_DEVICE_ID_RNG              4
#define VIRTMMIO_DEVICE_ID_GPU              16
#define VIRTMMIO_DEVICE_ID_INPUT            18

/* QEMU 5.2 virtio-mmio */
#define VIRTMMIO_BASE_ADDR                  0x0A000000
#define VIRTMMIO_BASE_SIZE                  0x200
#define VIRTMMIO_BASE_IRQ                   16
#define NUM_VIRTIO_TRANSPORTS               32

#define VIRTMMIO_IRQ_NOTIFY_USED            (1 << 0)

struct VirtqDesc {
    uint64_t pAddr;
    uint32_t len;
#define VIRTQ_DESC_F_NEXT                   (1 << 0)
#define VIRTQ_DESC_F_WRITE                  (1 << 1)
    uint16_t flag;
    uint16_t next;
};

struct VirtqAvail {
#define VIRTQ_AVAIL_F_NO_INTERRUPT          (1 << 0)
    uint16_t flag;
    uint16_t index;
    uint16_t ring[];
    /* We do not use VIRTIO_F_EVENT_IDX, so no other member */
};

struct VirtqUsedElem {
    uint32_t id;    /* u32 for padding purpose */
    uint32_t len;
};

struct VirtqUsed {
#define VIRTQ_USED_F_NO_NOTIFY              (1 << 0)
    uint16_t flag;
    uint16_t index;
    struct VirtqUsedElem ring[];
    /* We do not use VIRTIO_F_EVENT_IDX, so no other member */
};

struct Virtq {
    uint16_t qsz;
    uint16_t last;

    struct VirtqDesc *desc;
    struct VirtqAvail *avail;
    struct VirtqUsed *used;
};

#define VIRTQ_NUM   2

/* common virtio-mmio device structure, should be first member of specific device */
struct VirtmmioDev {
    VADDR_T         base;   /* I/O base address */
#define _IRQ_MASK   0xFF    /* higher bytes as registered flag */
    int             irq;
    struct Virtq    vq[VIRTQ_NUM];
};


/* discover and fill in 'dev' if found given ID device */
bool VirtmmioDiscover(uint32_t devId, struct VirtmmioDev *dev);

void VirtmmioInitBegin(const struct VirtmmioDev *dev);

/* add 'supported'(default 0) according given 'features' */
typedef bool (*VirtioFeatureFn)(uint32_t features, uint32_t *supported, void *dev);

/* negotiate 'baseDev' feature word 0 & 1 through 'f0' & 'f1'. 'dev' is passed to callbacks */
bool VirtmmioNegotiate(struct VirtmmioDev *baseDev, VirtioFeatureFn f0, VirtioFeatureFn f1, void *dev);

/* calculate queue space of size 'qsz', conforming to alignment limits */
unsigned VirtqSize(uint16_t qsz);

/* config pre-allocated continuous memory as two Virtq, started at 'base' with specified queue size */
VADDR_T VirtmmioConfigQueue(struct VirtmmioDev *dev, VADDR_T base, uint16_t qsz[], int len);

bool VirtmmioRegisterIRQ(struct VirtmmioDev *dev, HWI_PROC_FUNC handle, void *argDev, const char *devName);

void VritmmioInitEnd(const struct VirtmmioDev *dev);

void VirtmmioInitFailed(const struct VirtmmioDev *dev);

uint32_t VirtgpuGetXres(void);
uint32_t VirtgpuGetYres(void);

#endif
