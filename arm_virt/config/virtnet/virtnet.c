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

/* enable lwip `netif_add` API */
#define __LWIP__

#include "los_base.h"
#include "los_hw_cpu.h"
#include "los_printf.h"
#include "los_vm_zone.h"
#include "los_vm_phys.h"

/* kernel header changed some lwip behavior, so this should be prior to them */
#include "virtnet.h"

#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/tcpip.h"
#include "lwip/mem.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

static inline VADDR_T RegVaddr(const struct VirtNetif *nic, unsigned reg)
{
    return nic->base + reg;
}

static inline uint32_t VirtioGetStatus(const struct VirtNetif *nic)
{
    VADDR_T v = RegVaddr(nic, VIRTMMIO_REG_STATUS);
    return GET_UINT32(v) & VIRTIO_STATUS_MASK;
}

static inline void VirtioAddStatus(const struct VirtNetif *nic, uint32_t val)
{
    VADDR_T v = RegVaddr(nic, VIRTMMIO_REG_STATUS);
    WRITE_UINT32(VirtioGetStatus(nic) | val, v);
}

static inline void VirtioResetStatus(const struct VirtNetif *nic)
{
    VADDR_T v = RegVaddr(nic, VIRTMMIO_REG_STATUS);
    WRITE_UINT32(VIRTIO_STATUS_RESET, v);
}

static err_t DiscoverVirtnet(struct VirtNetif *nic)
{
    VADDR_T base;
    int i;
    uint32_t *p = NULL;

    base = IO_DEVICE_ADDR(VIRTMMIO_BASE_ADDR) + VIRTMMIO_BASE_SIZE * (NUM_VIRTIO_TRANSPORTS - 1);
    for (i = NUM_VIRTIO_TRANSPORTS - 1; i >= 0; i--) {
        p = (uint32_t *)base;
        if ((*p == VIRTMMIO_MAGIC) &&
            (*(p + 1) == VIRTMMIO_VERSION_LEGACY) &&
            (*(p + 2) == VIRTMMIO_DEVICE_ID_NET)) {
            nic->base = base;
            nic->irq = IRQ_SPI_BASE + VIRTMMIO_BASE_IRQ + i;
            return ERR_OK;
        }

        base -= VIRTMMIO_BASE_SIZE;
    }

    return ERR_IF;
}

static err_t NegotiateFeature(struct netif *netif)
{
    uint32_t features, supported, i;
    struct VirtNetif *nic = netif->state;

    WRITE_UINT32(VIRTIO_FEATURE_WORD, RegVaddr(nic, VIRTMMIO_REG_DEVFEATURESEL));
    features = GET_UINT32(RegVaddr(nic, VIRTMMIO_REG_DEVFEATURE));
    supported = 0;

    if (features & VIRTIO_NET_F_MAC) {
        for (i = 0; i < ETHARP_HWADDR_LEN; i++) {
            netif->hwaddr[i] = GET_UINT8(RegVaddr(nic, VIRTMMIO_REG_CONFIG + i));
        }
        netif->hwaddr_len = ETHARP_HWADDR_LEN;
        supported |= VIRTIO_NET_F_MAC;
    } else {
        PRINT_ERR("no MAC found\n");
        return ERR_IF;
    }

    WRITE_UINT32(VIRTIO_FEATURE_WORD, RegVaddr(nic, VIRTMMIO_REG_DRVFEATURESEL));
    WRITE_UINT32(supported, RegVaddr(nic, VIRTMMIO_REG_DRVFEATURE));

    VirtioAddStatus(nic, VIRTIO_STATUS_FEATURES_OK);
    if ((VirtioGetStatus(nic) & VIRTIO_STATUS_FEATURES_OK) == 0) {
        PRINT_ERR("negotiate feature %#08x failed\n", supported);
        return ERR_IF;
    }

    return ERR_OK;
}

static err_t InitTxFreelist(struct VirtNetif *nic)
{
    int i;

    LOS_SpinInit(&nic->transLock);
    nic->tbufRec = (struct TbufRecord *)mem_malloc(sizeof(struct TbufRecord) * nic->trans.qsz);
    if (nic->tbufRec == NULL) {
        PRINT_ERR("alloc nic->tbufRec memory failed\n");
        return ERR_MEM;
    }

    for (i = 0; i < nic->trans.qsz - 1; i++) {
        nic->trans.desc[i].flag = VIRTQ_DESC_F_NEXT;
        nic->trans.desc[i].next = i + 1;
    }
    nic->tFreeHdr = 0;
    nic->tFreeNum = nic->trans.qsz;

    return ERR_OK;
}

static void FreeTxEntry(struct VirtNetif *nic, uint16_t head)
{
    uint32_t count, idx, tail;
    struct pbuf *phead = NULL;
    struct Virtq *q = &nic->trans;

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

static void ReleaseRvEntry(struct pbuf *p)
{
    struct RbufRecord *pc = (struct RbufRecord *)p;
    struct VirtNetif *nic = pc->nic;
    uint32_t intSave;

    LOS_SpinLockSave(&nic->recvLock, &intSave);
    nic->recv.avail->ring[nic->recv.avail->index % nic->recv.qsz] = pc->id;
    DSB;
    nic->recv.avail->index++;
    LOS_SpinUnlockRestore(&nic->recvLock, intSave);

    if (nic->recv.used->flag != VIRTQ_USED_F_NO_NOTIFY) {
        WRITE_UINT32(VIRTQ_IDX_RECV, RegVaddr(nic, VIRTMMIO_REG_QUEUENOTIFY));
    }
}

static err_t ConfigRvBuffer(struct VirtNetif *nic)
{
    uint32_t i;
    PADDR_T paddr;
    struct Virtq *q = &nic->recv;

    LOS_SpinInit(&nic->recvLock);
    nic->rbufRec = (struct RbufRecord *)mem_calloc(q->qsz, sizeof(struct RbufRecord));
    if (nic->rbufRec == NULL) {
        PRINT_ERR("alloc nic->rbufRec memory failed\n");
        return ERR_MEM;
    }

    /* receive buffer begin after queues */
    nic->packetBase = (VADDR_T)q->desc + PAGE_SIZE * VIRTQ_NUMS * VIRTQ_PAGES;
    paddr = OsVmVaddrToPage((void *)nic->packetBase)->physAddr;

    /* one buffer receive one packet(no more than 1514B) */
    for (i = 0; i < q->qsz; i++) {
        q->desc[i].pAddr = paddr;
        q->desc[i].len = VIRTQ_RECV_BUF_SIZE;
        q->desc[i].flag = VIRTQ_DESC_F_WRITE;
        paddr += VIRTQ_RECV_BUF_SIZE;

        q->avail->ring[i] = i;

        nic->rbufRec[i].cbuf.custom_free_function = ReleaseRvEntry;
        nic->rbufRec[i].nic = nic;
        nic->rbufRec[i].id = i;
    }

    /* now let bypass `VirtnetHdr` and point to packet base */
    nic->packetBase += sizeof(struct VirtnetHdr);
    return ERR_OK;
}

static err_t ConfigQueue(struct VirtNetif *nic)
{
    uint32_t i, num, pfn, qsz;
    void *firstPage = NULL;
    struct Virtq *q = NULL;
    err_t ret;

    num = VIRTQ_NUMS * VIRTQ_PAGES + VIRTQ_RECV_BUF_PAGES;
    firstPage = LOS_PhysPagesAllocContiguous(num);
    if (firstPage == NULL) {
        PRINT_ERR("alloc queue memory failed\n");
        return ERR_MEM;
    }
    memset_s(firstPage, num * PAGE_SIZE, 0, num * PAGE_SIZE);
    pfn = OsVmVaddrToPage(firstPage)->physAddr >> PAGE_SHIFT;

    WRITE_UINT32(PAGE_SIZE, RegVaddr(nic, VIRTMMIO_REG_GUESTPAGESIZE));

    for (i = 0; i < VIRTQ_NUMS; i++) {
        if (i == 0) {
            q = &nic->recv;
            qsz = VIRTQ_RECV_QSZ;
        } else {    /* not support for VIRTQ_NUMS>2 yet */
            q = &nic->trans;
            qsz = VIRTQ_TRANS_QSZ;
        }

        /* put here mainly for Deinit when error */
        q->desc = (struct VirtqDesc *)((UINTPTR)firstPage + i * VIRTQ_PAGES * PAGE_SIZE);

        WRITE_UINT32(i, RegVaddr(nic, VIRTMMIO_REG_QUEUESEL));

        num = GET_UINT32(RegVaddr(nic, VIRTMMIO_REG_QUEUEPFN));
        if (num != 0) {
            PRINT_ERR("queue %#x inused\n", num << PAGE_SHIFT);
            return ERR_IF;
        }
        num = GET_UINT32(RegVaddr(nic, VIRTMMIO_REG_QUEUENUMMAX));
        if (num < qsz) {
            PRINT_ERR("queue %u not available: max qsz=%d\n", i, num);
            return ERR_IF;
        }

        q->qsz = qsz;
        q->avail = (struct VirtqAvail *)((UINTPTR)q->desc + sizeof(struct VirtqDesc) * qsz);
        q->used = (struct VirtqUsed *)((UINTPTR)q->desc + VIRTQ_PAGES / VIRTQ_NUMS * PAGE_SIZE);

        WRITE_UINT32(qsz, RegVaddr(nic, VIRTMMIO_REG_QUEUENUM));
        WRITE_UINT32(PAGE_SIZE, RegVaddr(nic, VIRTMMIO_REG_QUEUEALIGN));
        WRITE_UINT32(pfn + i * VIRTQ_PAGES, RegVaddr(nic, VIRTMMIO_REG_QUEUEPFN));

        if (i == 0) {
            if ((ret = ConfigRvBuffer(nic)) != ERR_OK) {
                return ret;
            }
        } else {
            if ((ret = InitTxFreelist(nic)) != ERR_OK) {
                return ret;
            }
        }
    }

    return ERR_OK;
}

static uint32_t GetTxFreeEntry(struct VirtNetif *nic, uint32_t count)
{
    uint32_t intSave, head, tail, idx;
    uint32_t tmp = count;

RETRY:
    LOS_SpinLockSave(&nic->transLock, &intSave);
    if (count > nic->tFreeNum) {
        LOS_SpinUnlockRestore(&nic->transLock, intSave);
        LOS_TaskYield();
        goto RETRY;
    }

    head = nic->tFreeHdr;
    idx = head;
    while (tmp--) {
        tail = idx;
        idx = nic->trans.desc[idx].next;
    }
    nic->tFreeHdr = idx;   /* may be invalid if empty, but tFreeNum must be valid: 0 */
    nic->tFreeNum -= count;
    LOS_SpinUnlockRestore(&nic->transLock, intSave);
    nic->trans.desc[tail].flag &= ~VIRTQ_DESC_F_NEXT;

    return head;
}

static err_t LowLevelOutput(struct netif *netif, struct pbuf *p)
{
    uint32_t add, idx, head, tmp;
    struct pbuf *q = NULL;
    struct VirtNetif *nic = netif->state;
    struct Virtq *trans = &nic->trans;

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
    trans->desc[head].pAddr = nic->vnHdrPaddr;
    trans->desc[head].len = sizeof(struct VirtnetHdr);
    idx = trans->desc[head].next;
    tmp = head;
    q = p;
    while (q != NULL) {
        tmp = trans->desc[tmp].next;
        trans->desc[tmp].pAddr = (PADDR_T)q->payload - KERNEL_ASPACE_BASE + SYS_MEM_BASE;
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
    WRITE_UINT32(VIRTQ_IDX_TRANS, RegVaddr(nic, VIRTMMIO_REG_QUEUENOTIFY));

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

    payload = nic->packetBase + VIRTQ_RECV_BUF_SIZE * e->id;
#if ETH_PAD_SIZE
    payload -= ETH_PAD_SIZE;
#endif
    pbuf_alloced_custom(PBUF_RAW, ETH_FRAME_LEN, PBUF_ROM | PBUF_ALLOC_FLAG_RX,
                        &nic->rbufRec[e->id].cbuf, (void *)payload, ETH_FRAME_LEN);

    len = e->len - sizeof(struct VirtnetHdr);
    if ((len > ETH_FRAME_LEN) || (nic->recv.desc[e->id].flag & VIRTQ_DESC_F_NEXT)) {
        PRINT_ERR("packet error: len=%u, queue flag=%u\n", len, nic->recv.desc[e->id].flag);
        return NULL;
    }
#if ETH_PAD_SIZE
    len += ETH_PAD_SIZE;
#endif

    p = &nic->rbufRec[e->id].cbuf.pbuf;
    p->len = len;
    p->tot_len = p->len;
    return p;
}

static void VirtnetRvHandle(struct netif *netif)
{
    struct VirtNetif *nic = netif->state;
    struct Virtq *q = &nic->recv;
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
            ReleaseRvEntry(buf);
        }

        q->last++;
    }
}

static void VirtnetTxHandle(struct VirtNetif *nic)
{
    struct Virtq *q = &nic->trans;
    struct VirtqUsedElem *e = NULL;

    /* Bypass recheck as VirtnetRvHandle */
    q->avail->flag = VIRTQ_AVAIL_F_NO_INTERRUPT;
    while (q->last != q->used->index) {
        DSB;
        e = &q->used->ring[q->last % q->qsz];
        FreeTxEntry(nic, e->id);
        q->last++;
    }
    q->avail->flag = 0;
}

void VirtnetIRQhandle(int swIrq, void *pDevId)
{
    (void)swIrq;
    struct netif *netif = pDevId;
    struct VirtNetif *nic = netif->state;

    if (!(GET_UINT32(RegVaddr(nic, VIRTMMIO_REG_INTERRUPTSTATUS)) & VIRTMMIO_IRQ_NOTIFY_USED)) {
        return;
    }

    VirtnetRvHandle(netif);

    VirtnetTxHandle(nic);

    WRITE_UINT32(VIRTMMIO_IRQ_NOTIFY_USED, RegVaddr(nic, VIRTMMIO_REG_INTERRUPTACK));
}

static err_t LowLevelInit(struct netif *netif)
{
    struct VirtNetif *nic = netif->state;
    int ret;

    if (DiscoverVirtnet(nic) != ERR_OK) {
        PRINT_ERR("virtio-mmio net device not found\n");
        return ERR_IF;
    }

    VirtioResetStatus(nic);

    VirtioAddStatus(nic, VIRTIO_STATUS_ACK);

    VirtioAddStatus(nic, VIRTIO_STATUS_DRIVER);
    while ((VirtioGetStatus(nic) & VIRTIO_STATUS_DRIVER) == 0) { }

    if ((ret = NegotiateFeature(netif)) != ERR_OK) {
        goto ERR_OUT;
    }

    if ((ret = ConfigQueue(nic)) != ERR_OK) {
        goto ERR_OUT;
    }

    nic->vnHdrPaddr = (PADDR_T)(&nic->vnHdr) - KERNEL_ASPACE_BASE + SYS_MEM_BASE;

    HwiIrqParam param = {0, netif, VIRTMMIO_NETIF_NAME};
    ret = LOS_HwiCreate(nic->irq, OS_HWI_PRIO_HIGHEST, IRQF_SHARED,
                        (HWI_PROC_FUNC)VirtnetIRQhandle, &param);
    if (ret != 0) {
        PRINT_ERR("virtio-mmio net device IRQ register failed: %d\n", ret);
        ret = ERR_IF;
        goto ERR_OUT;
    }
    HalIrqUnmask(nic->irq);
    nic->irq |= ~_IRQ_MASK;

    VirtioAddStatus(nic, VIRTIO_STATUS_DRIVER_OK);

    /* everything is ready, now notify device the receive buffer */
    nic->recv.avail->index += nic->recv.qsz;
    WRITE_UINT32(VIRTQ_IDX_RECV, RegVaddr(nic, VIRTMMIO_REG_QUEUENOTIFY));
    return ERR_OK;

ERR_OUT:
    VirtioAddStatus(nic, VIRTIO_STATUS_FAILED);
    return ret;
}

static err_t EthernetIfInit(struct netif *netif)
{
    struct VirtNetif *nic = NULL;

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

    netif->full_name[0] = netif->name[0] = VIRTMMIO_NETIF_NICK0;
    netif->full_name[1] = netif->name[1] = VIRTMMIO_NETIF_NICK1;
    netif->full_name[2] = VIRTMMIO_NETIF_NICK2;
    netif->full_name[3] = '\0';

    netif->output = etharp_output;
    netif->linkoutput = LowLevelOutput;

    netif->mtu = VIRTMMIO_NETIF_MTU;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    return LowLevelInit(netif);
}

static void VirtnetDeInit(struct netif *netif)
{
    struct VirtNetif *nic = netif->state;

    if (nic && (nic->irq & ~_IRQ_MASK)) {
        HwiIrqParam param = {0, netif, VIRTMMIO_NETIF_NAME};
        LOS_HwiDelete(nic->irq & _IRQ_MASK, &param);
    }
    if (nic && nic->rbufRec) {
        mem_free(nic->rbufRec);
    }
    if (nic && nic->tbufRec) {
        mem_free(nic->tbufRec);
    }
    if (nic && nic->recv.desc) {
        LOS_PhysPagesFreeContiguous(nic->recv.desc, VIRTQ_NUMS * VIRTQ_PAGES + VIRTQ_RECV_BUF_PAGES);
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

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
