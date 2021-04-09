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
#ifndef __VIRTNET_H__
#define __VIRTNET_H__

/*
 * Only legacy device supported and endianness ignored,
 * for virtqueue use native endianness of the guest.
 */

#include "stdint.h"
#include "los_typedef.h"
#include "los_vm_common.h"
#include "los_spinlock.h"
#include "netinet/if_ether.h"
#include "lwip/pbuf.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define VIRTIO_STATUS_RESET                 0
#define VIRTIO_STATUS_ACK                   1
#define VIRTIO_STATUS_DRIVER                2
#define VIRTIO_STATUS_DRIVER_OK             4
#define VIRTIO_STATUS_FEATURES_OK           8
#define VIRTIO_STATUS_DEVICE_NEEDS_RESET    64
#define VIRTIO_STATUS_FAILED                128
#define VIRTIO_STATUS_MASK                  0xFF

#define VIRTIO_FEATURE_WORD                 0
#define VIRTIO_NET_F_MAC                    (1 << 5)

#define VIRTMMIO_REG_DEVFEATURE             0x10
#define VIRTMMIO_REG_DEVFEATURESEL          0x14
#define VIRTMMIO_REG_DRVFEATURE             0x20
#define VIRTMMIO_REG_DRVFEATURESEL          0x24
#define VIRTMMIO_REG_GUESTPAGESIZE          0x28
#define VIRTMMIO_REG_QUEUESEL               0x30
#define VIRTMMIO_REG_QUEUENUMMAX            0x34
#define VIRTMMIO_REG_QUEUENUM               0x38
#define VIRTMMIO_REG_QUEUEALIGN             0x3C
#define VIRTMMIO_REG_QUEUEPFN               0x40
#define VIRTMMIO_REG_QUEUENOTIFY            0x50
#define VIRTMMIO_REG_INTERRUPTSTATUS        0x60
#define VIRTMMIO_REG_INTERRUPTACK           0x64
#define VIRTMMIO_REG_STATUS                 0x70
#define VIRTMMIO_REG_CONFIG                 0x100

/* QEMU 5.2 virtio-mmio */
#define VIRTMMIO_MAGIC                      0x74726976
#define VIRTMMIO_VERSION_LEGACY             1
#define VIRTMMIO_DEVICE_ID_NET              1
#define VIRTMMIO_BASE_ADDR                  0x0A000000
#define VIRTMMIO_BASE_SIZE                  0x200
#define VIRTMMIO_BASE_IRQ                   16
#define NUM_VIRTIO_TRANSPORTS               32

#define VIRTMMIO_NETIF_MTU                  1500
#define VIRTMMIO_NETIF_NAME                 "virtnet"
#define VIRTMMIO_NETIF_NICK0                'v'
#define VIRTMMIO_NETIF_NICK1                'n'
#define VIRTMMIO_NETIF_NICK2                '0'
#define VIRTMMIO_NETIF_DFT_IP               "10.0.2.15"
#define VIRTMMIO_NETIF_DFT_GW               "10.0.2.2"
#define VIRTMMIO_NETIF_DFT_MASK             "255.255.255.0"

#define VIRTQ_DESC_F_NEXT                   1
#define VIRTQ_DESC_F_WRITE                  2
#define VIRTQ_USED_F_NO_NOTIFY              1
#define VIRTQ_AVAIL_F_NO_INTERRUPT          1
#define VIRTMMIO_IRQ_NOTIFY_USED            1

struct VirtqDesc {
    uint64_t pAddr;
    uint32_t len;
    uint16_t flag;
    uint16_t next;
};

struct VirtqAvail {
    uint16_t flag;
    uint16_t index;
    uint16_t ring[];
    /* We do not use VIRTIO_F_EVENT_IDX, so no other member */
};

struct VirtqUsedElem {
    uint32_t id;
    uint32_t len;
};

struct VirtqUsed {
    uint16_t flag;
    uint16_t index;
    struct VirtqUsedElem ring[];
    /* We do not use VIRTIO_F_EVENT_IDX, so no other member */
};

struct Virtq {
    unsigned int qsz;
    unsigned int last;

    struct VirtqDesc *desc;
    struct VirtqAvail *avail;
    struct VirtqUsed *used;
};

/* This struct is actually ignored */
struct VirtnetHdr {
    uint8_t flag;
    uint8_t gsoType;
    uint16_t hdrLen;
    uint16_t gsoSize;
    uint16_t csumStart;
    uint16_t csumOffset;
};

/*
 * We use two queues for Tx/Rv respectively, each has minimal two-page-size.
 * When Tx/Rv, no dynamic memory alloc/free: output pbuf directly put into
 * queue and freed by tcpip_thread when used; input has some fixed-size buffers
 * just after the queues and released by application when consumed.
 */
#define VIRTQ_IDX_RECV          0
#define VIRTQ_IDX_TRANS         1
#define VIRTQ_NUMS              2
#define VIRTQ_PAGES             2
#define VIRTQ_TRANS_QSZ         128
#define VIRTQ_RECV_QSZ          32
#define VIRTQ_RECV_BUF_SIZE     ROUNDUP(sizeof(struct VirtnetHdr) + ETH_FRAME_LEN, 4)
#define VIRTQ_RECV_BUF_PAGES    (ROUNDUP(VIRTQ_RECV_QSZ * VIRTQ_RECV_BUF_SIZE, PAGE_SIZE) / PAGE_SIZE)

struct RbufRecord {
    struct pbuf_custom  cbuf;
    struct VirtNetif    *nic;
    uint32_t            id;     /* index to recv.desc[] */
};

struct TbufRecord {
    struct pbuf         *head;  /* first pbuf address of this pbuf chain */
    uint32_t            count;  /* occupied desc entries, include VirtnetHdr */
    uint32_t            tail;   /* tail pbuf's index to recv.desc[] */
};

struct VirtNetif {
    VADDR_T             base;       /* I/O base address */
#define _IRQ_MASK   0xFF
    int                 irq;        /* higher bytes as HWI registered flag */

    struct Virtq        recv;
    VADDR_T             packetBase; /* actual packet base in receive buffer */
    struct RbufRecord   *rbufRec;
    SPIN_LOCK_S         recvLock;

    struct Virtq        trans;
    unsigned int        tFreeHdr;   /* head of free desc entries list */
    unsigned int        tFreeNum;
    struct TbufRecord   *tbufRec;
    SPIN_LOCK_S         transLock;

    struct VirtnetHdr   vnHdr;
    PADDR_T             vnHdrPaddr;
};

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
