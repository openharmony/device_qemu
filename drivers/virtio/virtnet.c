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

/* enable lwip 'netif_add' API */
#define __LWIP__

#include "los_base.h"
#include "los_hw_cpu.h"
#include "los_vm_zone.h"
#include "los_spinlock.h"

/* kernel changed lwip 'netif->client_data' size, so this should be prior */
#include "netinet/if_ether.h"

#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/mem.h"
#include "virtmmio.h"

#define VIRTIO_NET_F_MTU                    (1 << 3)
#define VIRTIO_NET_F_MAC                    (1 << 5)
struct VirtnetConfig {
    uint8_t mac[6];
    uint16_t status;
    uint16_t maxVirtqPairs;
    uint16_t mtu;
};

#define VIRTMMIO_NETIF_NAME                 "virtnet"
#define VIRTMMIO_NETIF_NICK                 "eth0"
#define VIRTMMIO_NETIF_DFT_IP               "10.0.2.15"
#define VIRTMMIO_NETIF_DFT_GW               "10.0.2.2"
#define VIRTMMIO_NETIF_DFT_MASK             "255.255.255.0"
#define VIRTMMIO_NETIF_DFT_RXQSZ            16
#define VIRTMMIO_NETIF_DFT_TXQSZ            32

/* This struct is actually ignored by this simple driver */
struct VirtnetHdr {
    uint8_t flag;
    uint8_t gsoType;
    uint16_t hdrLen;
    uint16_t gsoSize;
    uint16_t csumStart;
    uint16_t csumOffset;
    uint16_t numBuffers;
};

/*
 * We use two queues for Tx/Rx respectively. When Tx/Rx, no dynamic memory alloc/free:
 * output pbuf directly put into queue and freed by tcpip_thread when used; input has
 * some fixed-size buffers just after the queues and released by application when consumed.
 *
 * Tx/Rx queues memory layout:
 *                         Rx queue                                Tx queue             Rx buffers
 * +-----------------+------------------+------------------++------+-------+------++----------------------+
 * | desc: 16B align | avail: 2B align  | used: 4B align   || desc | avail | used || 4B align             |
 * | 16∗(Queue Size) | 4+2∗(Queue Size) | 4+8∗(Queue Size) ||      |       |      || 1528*(Rx Queue Size) |
 * +-----------------+------------------+------------------++------+-------+------++----------------------+
 */
#define VIRTQ_NUM_NET       2
#define VIRTQ_RXBUF_ALIGN   4
#define VIRTQ_RXBUF_SIZE    ALIGN(sizeof(struct VirtnetHdr) + ETH_FRAME_LEN, VIRTQ_RXBUF_ALIGN)

struct RbufRecord {
    struct pbuf_custom  cbuf;
    struct VirtNetif    *nic;
    uint16_t            id;     /* index to Rx vq[0].desc[] */
};

struct TbufRecord {
    struct pbuf         *head;  /* first pbuf address of this pbuf chain */
    uint16_t            count;  /* occupied desc entries, including VirtnetHdr */
    uint16_t            tail;   /* tail pbuf's index to Tx vq[1].desc[] */
};

struct VirtNetif {
    struct VirtmmioDev  dev;

    struct RbufRecord   *rbufRec;
    SPIN_LOCK_S         recvLock;

    uint16_t            tFreeHdr;   /* head of Tx free desc entries list */
    uint16_t            tFreeNum;
    struct TbufRecord   *tbufRec;
    SPIN_LOCK_S         transLock;

    struct VirtnetHdr   vnHdr;
};

static bool Feature0(uint32_t features, uint32_t *supported, void *dev)
{
    struct netif *netif = dev;
    struct VirtNetif *nic = netif->state;
    struct VirtnetConfig *conf = (struct VirtnetConfig *)(nic->dev.base + VIRTMMIO_REG_CONFIG);
    int i;

    if (features & VIRTIO_NET_F_MTU) {
        if (conf->mtu > ETH_DATA_LEN) {
            PRINT_ERR("unsupported backend net MTU: %u\n", conf->mtu);
            return false;
        }
        netif->mtu = conf->mtu;
        *supported |= VIRTIO_NET_F_MTU;
    } else {
        netif->mtu = ETH_DATA_LEN;
    }

    LOS_ASSERT(features & VIRTIO_NET_F_MAC);
    for (i = 0; i < ETHARP_HWADDR_LEN; i++) {
        netif->hwaddr[i] = conf->mac[i];
    }
    netif->hwaddr_len = ETHARP_HWADDR_LEN;
    *supported |= VIRTIO_NET_F_MAC;

    return true;
}

static bool Feature1(uint32_t features, uint32_t *supported, void *dev)
{
    if (features & VIRTIO_F_VERSION_1) {
        *supported |= VIRTIO_F_VERSION_1;
    } else {
        PRINT_ERR("net device has no VERSION_1 feature\n");
        return false;
    }

    return true;
}

static err_t InitTxFreelist(struct VirtNetif *nic)
{
    int i;

    LOS_SpinInit(&nic->transLock);
    nic->tbufRec = malloc(sizeof(struct TbufRecord) * nic->dev.vq[1].qsz);
    if (nic->tbufRec == NULL) {
        PRINT_ERR("alloc nic->tbufRec memory failed\n");
        return ERR_MEM;
    }

    for (i = 0; i < nic->dev.vq[1].qsz - 1; i++) {
        nic->dev.vq[1].desc[i].flag = VIRTQ_DESC_F_NEXT;
        nic->dev.vq[1].desc[i].next = i + 1;
    }
    nic->tFreeHdr = 0;
    nic->tFreeNum = nic->dev.vq[1].qsz;

    return ERR_OK;
}

static void FreeTxEntry(struct VirtNetif *nic, uint16_t head)
{
    uint16_t count, idx, tail;
    struct pbuf *phead = NULL;
    struct Virtq *q = &nic->dev.vq[1];

    idx = q->desc[head].next;
    phead = nic->tbufRec[idx].head;
    count = nic->tbufRec[idx].count;
    tail = nic->tbufRec[idx].tail;

    LOS_SpinLock(&nic->transLock);
    if (nic->tFreeNum > 0) {
        q->desc[tail].next = nic->tFreeHdr;
        q->desc[tail].flag = VIRTQ_DESC_F_NEXT;
    }
    nic->tFreeNum += count;
    nic->tFreeHdr = head;
    LOS_SpinUnlock(&nic->transLock);

    pbuf_free_callback(phead);
}

static void ReleaseRxEntry(struct pbuf *p)
{
    struct RbufRecord *pr = (struct RbufRecord *)p;
    struct VirtNetif *nic = pr->nic;
    uint32_t intSave;

    LOS_SpinLockSave(&nic->recvLock, &intSave);
    nic->dev.vq[0].avail->ring[nic->dev.vq[0].avail->index % nic->dev.vq[0].qsz] = pr->id;
    DSB;
    nic->dev.vq[0].avail->index++;
    LOS_SpinUnlockRestore(&nic->recvLock, intSave);

    if (nic->dev.vq[0].used->flag != VIRTQ_USED_F_NO_NOTIFY) {
        WRITE_UINT32(0, nic->dev.base + VIRTMMIO_REG_QUEUENOTIFY);
    }
}

static err_t ConfigRxBuffer(struct VirtNetif *nic, VADDR_T buf)
{
    uint32_t i;
    PADDR_T paddr;
    struct Virtq *q = &nic->dev.vq[0];

    LOS_SpinInit(&nic->recvLock);
    nic->rbufRec = calloc(q->qsz, sizeof(struct RbufRecord));
    if (nic->rbufRec == NULL) {
        PRINT_ERR("alloc nic->rbufRec memory failed\n");
        return ERR_MEM;
    }

    paddr = VMM_TO_DMA_ADDR(buf);

    for (i = 0; i < q->qsz; i++) {
        q->desc[i].pAddr = paddr;
        q->desc[i].len = sizeof(struct VirtnetHdr) + ETH_FRAME_LEN;
        q->desc[i].flag = VIRTQ_DESC_F_WRITE;
        paddr += VIRTQ_RXBUF_SIZE;

        q->avail->ring[i] = i;

        nic->rbufRec[i].cbuf.custom_free_function = ReleaseRxEntry;
        nic->rbufRec[i].nic = nic;
        nic->rbufRec[i].id = i;
    }

    return ERR_OK;
}

static err_t ConfigQueue(struct VirtNetif *nic)
{
    VADDR_T buf, pad;
    void *base = NULL;
    err_t ret;
    size_t size;
    uint16_t qsz[VIRTQ_NUM_NET];

    /*
     * lwip request (packet address - ETH_PAD_SIZE) must align with 4B.
     * We pad before the first Rx buf to happy it. Rx buf = VirtnetHdr + packet,
     * then (buf base + pad + VirtnetHdr - ETH_PAD_SIZE) should align with 4B.
     * When allocating memory, VIRTQ_RXBUF_ALIGN - 1 is enough for padding.
     */
    qsz[0] = VIRTMMIO_NETIF_DFT_RXQSZ;
    qsz[1] = VIRTMMIO_NETIF_DFT_TXQSZ;
    size = VirtqSize(qsz[0]) + VirtqSize(qsz[1]) + VIRTQ_RXBUF_ALIGN - 1 + qsz[0] * VIRTQ_RXBUF_SIZE;

    base = calloc(1, size);
    if (base == NULL) {
        PRINT_ERR("alloc queues memory failed\n");
        return ERR_MEM;
    }

    buf = VirtmmioConfigQueue(&nic->dev, (VADDR_T)base, qsz, VIRTQ_NUM_NET);
    if (buf == 0) {
        return ERR_IF;
    }

    pad = (buf + sizeof(struct VirtnetHdr) - ETH_PAD_SIZE) % VIRTQ_RXBUF_ALIGN;
    if (pad) {
        pad = VIRTQ_RXBUF_ALIGN - pad;
    }
    buf += pad;
    if ((ret = ConfigRxBuffer(nic, buf)) != ERR_OK) {
        return ret;
    }

    if ((ret = InitTxFreelist(nic)) != ERR_OK) {
        return ret;
    }

    return ERR_OK;
}

static uint16_t GetTxFreeEntry(struct VirtNetif *nic, uint16_t count)
{
    uint32_t intSave;
    uint16_t head, tail, idx;

RETRY:
    LOS_SpinLockSave(&nic->transLock, &intSave);
    if (count > nic->tFreeNum) {
        LOS_SpinUnlockRestore(&nic->transLock, intSave);
        LOS_TaskYield();
        goto RETRY;
    }

    nic->tFreeNum -= count;
    head = nic->tFreeHdr;
    idx = head;
    while (count--) {
        tail = idx;
        idx = nic->dev.vq[1].desc[idx].next;
    }
    nic->tFreeHdr = idx;   /* may be invalid if empty, but tFreeNum must be valid: 0 */
    LOS_SpinUnlockRestore(&nic->transLock, intSave);
    nic->dev.vq[1].desc[tail].flag &= ~VIRTQ_DESC_F_NEXT;

    return head;
}

static err_t LowLevelOutput(struct netif *netif, struct pbuf *p)
{
    uint16_t add, idx, head, tmp;
    struct pbuf *q = NULL;
    struct VirtNetif *nic = netif->state;
    struct Virtq *trans = &nic->dev.vq[1];

#if ETH_PAD_SIZE
    pbuf_header(p, -ETH_PAD_SIZE); /* drop the padding word */
#endif

    /* plus 1 for VirtnetHdr */
    add = pbuf_clen(p) + 1;
    if (add > trans->qsz) {
        PRINT_ERR("packet pbuf_clen %u larger than supported %u\n", add - 1, trans->qsz - 1);
        return ERR_IF;
    }

    head = GetTxFreeEntry(nic, add);
    trans->desc[head].pAddr = VMM_TO_DMA_ADDR((PADDR_T)&nic->vnHdr);
    trans->desc[head].len = sizeof(struct VirtnetHdr);
    idx = trans->desc[head].next;
    tmp = head;
    q = p;
    while (q != NULL) {
        tmp = trans->desc[tmp].next;
        trans->desc[tmp].pAddr = VMM_TO_DMA_ADDR((PADDR_T)q->payload);
        trans->desc[tmp].len = q->len;
        q = q->next;
    }

    nic->tbufRec[idx].head = p;
    nic->tbufRec[idx].count = add;
    nic->tbufRec[idx].tail = tmp;
    pbuf_ref(p);

    trans->avail->ring[trans->avail->index % trans->qsz] = head;
    DSB;
    trans->avail->index++;
    WRITE_UINT32(1, nic->dev.base + VIRTMMIO_REG_QUEUENOTIFY);

#if ETH_PAD_SIZE
    pbuf_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    return ERR_OK;
}

static struct pbuf *LowLevelInput(const struct netif *netif, const struct VirtqUsedElem *e)
{
    struct VirtNetif *nic = netif->state;
    struct pbuf *p = NULL;
    uint16_t len;
    VADDR_T payload;

    payload = DMA_TO_VMM_ADDR(nic->dev.vq[0].desc[e->id].pAddr) + sizeof(struct VirtnetHdr);
#if ETH_PAD_SIZE
    payload -= ETH_PAD_SIZE;
#endif
    pbuf_alloced_custom(PBUF_RAW, ETH_FRAME_LEN, PBUF_ROM | PBUF_ALLOC_FLAG_RX,
                        &nic->rbufRec[e->id].cbuf, (void *)payload, ETH_FRAME_LEN);

    len = e->len - sizeof(struct VirtnetHdr);
    LOS_ASSERT(len <= ETH_FRAME_LEN);
#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE;
#endif

    p = &nic->rbufRec[e->id].cbuf.pbuf;
    p->len = len;
    p->tot_len = p->len;
    return p;
}

static void VirtnetRxHandle(struct netif *netif)
{
    struct VirtNetif *nic = netif->state;
    struct Virtq *q = &nic->dev.vq[0];
    struct pbuf *buf = NULL;
    struct VirtqUsedElem *e = NULL;

    q->avail->flag = VIRTQ_AVAIL_F_NO_INTERRUPT;
    while (1) {
        if (q->last == q->used->index) {
            q->avail->flag = 0;
            /* recheck if new one come in between empty ring and enable interrupt */
            DSB;
            if (q->last == q->used->index) {
                break;
            }
            q->avail->flag = VIRTQ_AVAIL_F_NO_INTERRUPT;
        }

        DSB;
        e = &q->used->ring[q->last % q->qsz];
        buf = LowLevelInput(netif, e);
        if (netif->input(buf, netif) != ERR_OK) {
            LWIP_DEBUGF(NETIF_DEBUG, ("IP input error\n"));
            ReleaseRxEntry(buf);
        }

        q->last++;
    }
}

static void VirtnetTxHandle(struct VirtNetif *nic)
{
    struct Virtq *q = &nic->dev.vq[1];
    struct VirtqUsedElem *e = NULL;

    /* Bypass recheck as VirtnetRxHandle */
    q->avail->flag = VIRTQ_AVAIL_F_NO_INTERRUPT;
    while (q->last != q->used->index) {
        DSB;
        e = &q->used->ring[q->last % q->qsz];
        FreeTxEntry(nic, e->id);
        q->last++;
    }
    q->avail->flag = 0;
}

static void VirtnetIRQhandle(int swIrq, void *pDevId)
{
    (void)swIrq;
    struct netif *netif = pDevId;
    struct VirtNetif *nic = netif->state;

    if (!(GET_UINT32(nic->dev.base + VIRTMMIO_REG_INTERRUPTSTATUS) & VIRTMMIO_IRQ_NOTIFY_USED)) {
        return;
    }

    VirtnetRxHandle(netif);

    VirtnetTxHandle(nic);

    WRITE_UINT32(VIRTMMIO_IRQ_NOTIFY_USED, nic->dev.base + VIRTMMIO_REG_INTERRUPTACK);
}

static err_t LowLevelInit(struct netif *netif)
{
    struct VirtNetif *nic = netif->state;
    int ret;

    if (!VirtmmioDiscover(VIRTMMIO_DEVICE_ID_NET, &nic->dev)) {
        return ERR_IF;
    }

    VirtmmioInitBegin(&nic->dev);

    if (!VirtmmioNegotiate(&nic->dev, Feature0, Feature1, netif)) {
        ret = ERR_IF;
        goto ERR_OUT;
    }

    if ((ret = ConfigQueue(nic)) != ERR_OK) {
        goto ERR_OUT;
    }

    if (!VirtmmioRegisterIRQ(&nic->dev, (HWI_PROC_FUNC)VirtnetIRQhandle, netif, VIRTMMIO_NETIF_NAME)) {
        ret = ERR_IF;
        goto ERR_OUT;
    }

    VritmmioInitEnd(&nic->dev);

    /* everything is ready, now notify device the receive buffer */
    nic->dev.vq[0].avail->index += nic->dev.vq[0].qsz;
    WRITE_UINT32(0, nic->dev.base + VIRTMMIO_REG_QUEUENOTIFY);
    return ERR_OK;

ERR_OUT:
    VirtmmioInitFailed(&nic->dev);
    return ret;
}

static err_t EthernetIfInit(struct netif *netif)
{
    struct VirtNetif *nic = NULL;
    size_t i;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    nic = mem_calloc(1, sizeof(struct VirtNetif));
    if (nic == NULL) {
        PRINT_ERR("alloc nic memory failed\n");
        return ERR_MEM;
    }
    netif->state = nic;

#if LWIP_NETIF_HOSTNAME
    netif->hostname = VIRTMMIO_NETIF_NAME;
#endif

    i = sizeof(netif->name) < sizeof(VIRTMMIO_NETIF_NICK) ?
        sizeof(netif->name) : sizeof(VIRTMMIO_NETIF_NICK);
    memcpy_s(netif->name, sizeof(netif->name), VIRTMMIO_NETIF_NICK, i);
    i = sizeof(netif->full_name) < sizeof(VIRTMMIO_NETIF_NICK) ?
        sizeof(netif->full_name) : sizeof(VIRTMMIO_NETIF_NICK);
    memcpy_s(netif->full_name, sizeof(netif->full_name), VIRTMMIO_NETIF_NICK, i);

    netif->output = etharp_output;
    netif->linkoutput = LowLevelOutput;

    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;
    netif->link_layer_type = ETHERNET_DRIVER_IF;

    return LowLevelInit(netif);
}

static void VirtnetDeInit(struct netif *netif)
{
    struct VirtNetif *nic = netif->state;

    if (nic && (nic->dev.irq & ~_IRQ_MASK)) {
        HwiIrqParam param = {0, netif, VIRTMMIO_NETIF_NAME};
        LOS_HwiDelete(nic->dev.irq & _IRQ_MASK, &param);
    }
    if (nic && nic->rbufRec) {
        free(nic->rbufRec);
    }
    if (nic && nic->tbufRec) {
        free(nic->tbufRec);
    }
    if (nic && nic->dev.vq[0].desc) {
        free(nic->dev.vq[0].desc);
    }
    if (nic) {
        mem_free(nic);
    }
    mem_free(netif);
}

struct netif *VirtnetInit(void)
{
    ip4_addr_t ip, mask, gw;
    struct netif *netif = NULL;

    netif = mem_calloc(1, sizeof(struct netif));
    if (netif == NULL) {
        PRINT_ERR("alloc netif memory failed\n");
        return NULL;
    }

    ip.addr = ipaddr_addr(VIRTMMIO_NETIF_DFT_IP);
    mask.addr = ipaddr_addr(VIRTMMIO_NETIF_DFT_MASK);
    gw.addr = ipaddr_addr(VIRTMMIO_NETIF_DFT_GW);
    if (netif_add(netif, &ip, &mask, &gw, netif->state,
                    EthernetIfInit, tcpip_input) == NULL) {
        PRINT_ERR("add virtio-mmio net device failed\n");
        VirtnetDeInit(netif);
        return NULL;
    }

    return netif;
}

