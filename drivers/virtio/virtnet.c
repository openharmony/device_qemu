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
 * Simple virtio-net driver using HDF WIFI framework without real WIFI functions.
 */

#include "los_hw_cpu.h"
#include "los_vm_iomap.h"
#include "los_vm_zone.h"
#include "netinet/if_ether.h"
#include "arpa/inet.h"
#include "core/hdf_device_desc.h"
#include "wifi/hdf_wlan_chipdriver_manager.h"
#include "wifi/wifi_mac80211_ops.h"
#include "osal.h"
#include "osal/osal_io.h"
#include "eapol.h"
#include "virtmmio.h"

#define HDF_LOG_TAG HDF_VIRTIO_NET

#define VIRTIO_NET_F_MTU                    (1 << 3)
#define VIRTIO_NET_F_MAC                    (1 << 5)
struct VirtnetConfig {
    uint8_t mac[6];
    uint16_t status;
    uint16_t maxVirtqPairs;
    uint16_t mtu;
};

#define VIRTMMIO_NETIF_NAME                 "virtnet"
#define VIRTMMIO_NETIF_DFT_GW               "10.0.2.2"
#define VIRTMMIO_NETIF_DFT_MASK             "255.255.255.0"

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
 * We use two queues for Tx/Rx respectively. When Tx, we record outgoing NetBuf
 * and free it when QEMU done. When Rx, we use fixed buffers, then allocate &
 * copy to a NetBuf, and then HDF will consume & free the NetBuf.
 * Every NetBuf is a solo packet, no chaining like LWIP pbuf. So every outgoing
 * packet always occupy two desc items: one for VirtnetHdr, the other for NetBuf.
 * Tx/Rx queues memory layout:
 *                         Rx queue                                Tx queue
 * +-----------------+------------------+------------------++------+-------+------+
 * | desc: 16B align | avail: 2B align  | used: 4B align   || desc | avail | used |
 * | 16∗(Queue Size) | 4+2∗(Queue Size) | 4+8∗(Queue Size) ||      |       |      |
 * +-----------------+------------------+------------------++------+-------+------+
 */
#define VIRTQ_RX_QSZ        16
#define VIRTQ_TX_QSZ        32
#define PER_TX_ENTRIES      2
#define PER_RXBUF_SIZE      (sizeof(struct VirtnetHdr) + ETH_FRAME_LEN)

struct VirtNetif {
    struct VirtmmioDev  dev;

    uint16_t            tFreeHdr;   /* head of Tx free desc entries list */
    uint16_t            tFreeNum;
    NetBuf*             tbufRec[VIRTQ_TX_QSZ];
    OSAL_DECLARE_SPINLOCK(transLock);

    uint8_t             rbuf[VIRTQ_RX_QSZ][PER_RXBUF_SIZE];

    struct VirtnetHdr   vnHdr;
};

static inline struct VirtNetif *GetVirtnetIf(const NetDevice *netDev)
{
    return (struct VirtNetif *)GET_NET_DEV_PRIV(netDev);
}

static bool Feature0(uint32_t features, uint32_t *supported, void *dev)
{
    NetDevice *netDev = dev;
    struct VirtNetif *nic = GetVirtnetIf(netDev);
    struct VirtnetConfig *conf = (struct VirtnetConfig *)(nic->dev.base + VIRTMMIO_REG_CONFIG);
    int i;

    if (features & VIRTIO_NET_F_MTU) {
        if (conf->mtu > WLAN_MAX_MTU || conf->mtu < WLAN_MIN_MTU) {
            HDF_LOGE("[%s]unsupported backend net MTU: %u", __func__, conf->mtu);
            return false;
        }
        netDev->mtu = conf->mtu;
        *supported |= VIRTIO_NET_F_MTU;
    } else {
        netDev->mtu = DEFAULT_MTU;
    }

    if ((features & VIRTIO_NET_F_MAC) == 0) {
        HDF_LOGE("[%s]no MAC feature found", __func__);
        return false;
    }
    for (i = 0; i < MAC_ADDR_SIZE; i++) {
        netDev->macAddr[i] = conf->mac[i];
    }
    netDev->addrLen = MAC_ADDR_SIZE;
    *supported |= VIRTIO_NET_F_MAC;

    return true;
}

static bool Feature1(uint32_t features, uint32_t *supported, void *dev)
{
    (void)dev;
    if (features & VIRTIO_F_VERSION_1) {
        *supported |= VIRTIO_F_VERSION_1;
    } else {
        HDF_LOGE("[%s]net device has no VERSION_1 feature", __func__);
        return false;
    }

    return true;
}

static int32_t InitTxFreelist(struct VirtNetif *nic)
{
    int i;

    for (i = 0; i < nic->dev.vq[1].qsz - 1; i++) {
        nic->dev.vq[1].desc[i].flag = VIRTQ_DESC_F_NEXT;
        nic->dev.vq[1].desc[i].next = i + 1;
    }
    nic->tFreeHdr = 0;
    nic->tFreeNum = nic->dev.vq[1].qsz;

    return OsalSpinInit(&nic->transLock);
}

static void FreeTxEntry(struct VirtNetif *nic, uint16_t head)
{
    struct Virtq *q = &nic->dev.vq[1];
    uint16_t idx = q->desc[head].next;
    NetBuf *nb = NULL;

    /* keep track of virt queue free entries */
    OsalSpinLock(&nic->transLock);
    if (nic->tFreeNum > 0) {
        q->desc[idx].next = nic->tFreeHdr;
        q->desc[idx].flag = VIRTQ_DESC_F_NEXT;
    }
    nic->tFreeNum += PER_TX_ENTRIES;
    nic->tFreeHdr = head;
    nb = nic->tbufRec[idx];
    OsalSpinUnlock(&nic->transLock);

    /* We free upstream Tx NetBuf! */
    NetBufFree(nb);
}

static void PopulateRxBuffer(struct VirtNetif *nic)
{
    uint32_t i;
    PADDR_T paddr;
    struct Virtq *q = &nic->dev.vq[0];

    for (i = 0; i < q->qsz; i++) {
        paddr = VMM_TO_DMA_ADDR((VADDR_T)nic->rbuf[i]);

        q->desc[i].pAddr = paddr;
        q->desc[i].len = PER_RXBUF_SIZE;
        q->desc[i].flag = VIRTQ_DESC_F_WRITE;

        q->avail->ring[i] = i;
    }
}

static int32_t ConfigQueue(struct VirtNetif *nic)
{
    VADDR_T base;
    uint16_t qsz[VIRTQ_NUM];

    base = ALIGN((VADDR_T)nic + sizeof(struct VirtNetif), VIRTQ_ALIGN_DESC);
    qsz[0] = VIRTQ_RX_QSZ;
    qsz[1] = VIRTQ_TX_QSZ;
    if (VirtmmioConfigQueue(&nic->dev, base, qsz, VIRTQ_NUM) == 0) {
        return HDF_DEV_ERR_DEV_INIT_FAIL;
    }

    PopulateRxBuffer(nic);

    return InitTxFreelist(nic);
}

static uint16_t GetTxFreeEntry(struct VirtNetif *nic)
{
    uint32_t intSave;
    uint16_t head, idx;
    bool logged = false;

RETRY:
    OsalSpinLockIrqSave(&nic->transLock, &intSave);
    if (PER_TX_ENTRIES > nic->tFreeNum) {
        OsalSpinUnlockIrqRestore(&nic->transLock, &intSave);
        if (!logged) {
            HDF_LOGW("[%s]transmit queue is full", __func__);
            logged = true;
        }
        LOS_TaskYield();
        goto RETRY;
    }

    nic->tFreeNum -= PER_TX_ENTRIES;
    head = nic->tFreeHdr;
    idx = nic->dev.vq[1].desc[head].next;
    /* new tFreeHdr may be invalid if list is empty, but tFreeNum must be valid: 0 */
    nic->tFreeHdr = nic->dev.vq[1].desc[idx].next;
    OsalSpinUnlockIrqRestore(&nic->transLock, &intSave);
    nic->dev.vq[1].desc[idx].flag &= ~VIRTQ_DESC_F_NEXT;

    return head;
}

static NetDevTxResult LowLevelOutput(NetDevice *netDev, NetBuf *p)
{
    uint16_t idx, head;
    struct VirtNetif *nic = GetVirtnetIf(netDev);
    struct Virtq *trans = &nic->dev.vq[1];

    head = GetTxFreeEntry(nic);
    trans->desc[head].pAddr = VMM_TO_DMA_ADDR((PADDR_T)&nic->vnHdr);
    trans->desc[head].len = sizeof(struct VirtnetHdr);
    idx = trans->desc[head].next;
    trans->desc[idx].pAddr = LOS_PaddrQuery(NetBufGetAddress(p, E_DATA_BUF));
    trans->desc[idx].len = NetBufGetDataLen(p);

    nic->tbufRec[idx] = p;

    trans->avail->ring[trans->avail->index % trans->qsz] = head;
    DSB;
    trans->avail->index++;
    if (trans->used->flag != VIRTQ_USED_F_NO_NOTIFY) {
        OSAL_WRITEL(1, nic->dev.base + VIRTMMIO_REG_QUEUENOTIFY);
    }

    return NETDEV_TX_OK;
}

static NetBuf *LowLevelInput(const NetDevice *netDev, const struct VirtqUsedElem *e)
{
    struct VirtNetif *nic = GetVirtnetIf(netDev);
    uint16_t len;
    uint8_t *payload = NULL;
    NetBuf *nb = NULL;

    /* we allocate Rx NetBuf & fill in received packet */
    len = e->len - sizeof(struct VirtnetHdr);
    nb = NetBufDevAlloc(netDev, len);
    if (nb == NULL) {
        HDF_LOGE("[%s]allocate NetBuf failed, drop 1 packet", __func__);
        return NULL;
    }
    payload = NetBufPush(nb, E_DATA_BUF, len);  /* here always succeed */
    (void)memcpy_s(payload, len, nic->rbuf[e->id] + sizeof(struct VirtnetHdr), len);

    return nb;
}

static void VirtnetRxHandle(NetDevice *netDev)
{
    struct VirtNetif *nic = GetVirtnetIf(netDev);
    struct Virtq *q = &nic->dev.vq[0];
    NetBuf *nb = NULL;
    struct VirtqUsedElem *e = NULL;
    uint16_t add = 0;

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
        nb = LowLevelInput(netDev, e);
        if (nb && NetIfRx(netDev, nb) != 0) {   /* Upstream free Rx NetBuf! */
            HDF_LOGE("[%s]NetIfRx failed, drop 1 packet", __func__);
            NetBufFree(nb);
        }

        /*
         * Our fixed receive buffers always sit in the appropriate desc[].
         * We only need to update the available ring to QEMU.
         */
        q->avail->ring[(q->avail->index + add++) % q->qsz] = e->id;
        q->last++;
    }
    DSB;
    q->avail->index += add;

    if (q->used->flag != VIRTQ_USED_F_NO_NOTIFY) {
        OSAL_WRITEL(0, nic->dev.base + VIRTMMIO_REG_QUEUENOTIFY);
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
    NetDevice *netDev = pDevId;
    struct VirtNetif *nic = GetVirtnetIf(netDev);

    if (!(OSAL_READL(nic->dev.base + VIRTMMIO_REG_INTERRUPTSTATUS) & VIRTMMIO_IRQ_NOTIFY_USED)) {
        return;
    }

    VirtnetRxHandle(netDev);

    VirtnetTxHandle(nic);

    OSAL_WRITEL(VIRTMMIO_IRQ_NOTIFY_USED, nic->dev.base + VIRTMMIO_REG_INTERRUPTACK);
}

/*
 * The whole initialization is complex, here is the main point.
 *  -factory-    FakeWifiInit/Release: alloc, set & register HdfChipDriverFactory
 *  -chip-       FakeFactoryInitChip/Release: alloc & set HdfChipDriver
 *  -NetDevice-  VirtNetDeviceInit/DeInit: set & add NetDevice
 *  -virtnet-    VirtnetInit/DeInit: virtio-net driver
 */

static int32_t VirtnetInit(NetDevice *netDev)
{
    int32_t ret, len;
    struct VirtNetif *nic = NULL;

    /* NOTE: For simplicity, alloc all these data from physical continuous memory. */
    len = sizeof(struct VirtNetif) + VirtqSize(VIRTQ_RX_QSZ) + VirtqSize(VIRTQ_TX_QSZ);
    nic = LOS_DmaMemAlloc(NULL, len, sizeof(void *), DMA_CACHE);
    if (nic == NULL) {
        HDF_LOGE("[%s]alloc nic memory failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    memset_s(nic, len, 0, len);
    GET_NET_DEV_PRIV(netDev) = nic;

    if (!VirtmmioDiscover(VIRTMMIO_DEVICE_ID_NET, &nic->dev)) {
        return HDF_DEV_ERR_NO_DEVICE;
    }

    VirtmmioInitBegin(&nic->dev);

    if (!VirtmmioNegotiate(&nic->dev, Feature0, Feature1, netDev)) {
        ret = HDF_DEV_ERR_DEV_INIT_FAIL;
        goto ERR_OUT;
    }

    if ((ret = ConfigQueue(nic)) != HDF_SUCCESS) {
        goto ERR_OUT;
    }

    ret = OsalRegisterIrq(nic->dev.irq, OSAL_IRQF_TRIGGER_NONE, (OsalIRQHandle)VirtnetIRQhandle,
                          VIRTMMIO_NETIF_NAME, netDev);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]register IRQ failed: %d", __func__, ret);
        goto ERR_OUT;
    }
    nic->dev.irq |= ~_IRQ_MASK;

    VritmmioInitEnd(&nic->dev);
    return HDF_SUCCESS;

ERR_OUT:
    VirtmmioInitFailed(&nic->dev);
    return ret;
}

static void VirtnetDeInit(NetDevice *netDev)
{
    struct VirtNetif *nic = GetVirtnetIf(netDev);

    if (nic && (nic->dev.irq & ~_IRQ_MASK)) {
        OsalUnregisterIrq(nic->dev.irq & _IRQ_MASK, netDev);
    }
    if (nic) {
        LOS_DmaMemFree(nic);
    }
    GET_NET_DEV_PRIV(netDev) = NULL;
}

static int32_t VirtNetDeviceSetMacAddr(NetDevice *netDev, void *addr)
{
    uint8_t *p = addr;
    for (int i = 0; i < netDev->addrLen; i++) {
        netDev->macAddr[i] = p[i];
    }
    return HDF_SUCCESS;
}

static struct NetDeviceInterFace g_netDevOps = {
    .setMacAddr = VirtNetDeviceSetMacAddr,
    /*
     * Link layer packet transmission chain:
     *   LWIP netif->linkoutput = driverif_output, in kernel, pbuf
     *       KHDF netif->drv_send = LwipSend, in HDF adapter, NetBuf
     *           NetDevice .xmit = our driver, NetBuf
     */
    .xmit = LowLevelOutput,
};

#define VN_RANDOM_IP_MASK   0xFF    /* 255.255.255.0 subnet */
#define VN_RANDOM_IP_SHF    24      /* network byte order */

/* fake WIFI have to UP interface, assign IP by hand */
static int32_t VirtNetDeviceInitDone(NetDevice *netDev)
{
    IpV4Addr ip, mask, gw;
    uint32_t h, l;
    int32_t ret;

    if ((ret = NetIfSetStatus(netDev, NETIF_UP)) != HDF_SUCCESS) {
        return ret;
    }

    /* odd way hope to get ~distinct~ IP for different guests */
    LOS_GetCpuCycle(&h, &l);
    l &= VN_RANDOM_IP_MASK;
    if (l == 0 || l == (1 << 1)) {          /* avoid 10.0.2.0, 10.0.2.2 */
        l++;
    } else if (l == VN_RANDOM_IP_MASK) {    /* avoid 10.0.2.255 */
        l--;
    }
    l <<= VN_RANDOM_IP_SHF;
    ip.addr = (inet_addr(VIRTMMIO_NETIF_DFT_MASK) & inet_addr(VIRTMMIO_NETIF_DFT_GW)) | l;
    mask.addr = inet_addr(VIRTMMIO_NETIF_DFT_MASK);
    gw.addr = inet_addr(VIRTMMIO_NETIF_DFT_GW);
    return NetIfSetAddr(netDev, &ip, &mask, &gw);
}

static int32_t VirtNetDeviceInit(struct HdfChipDriver *chipDriver, NetDevice *netDev)
{
    (void)chipDriver;
    int32_t ret;

    /* VirtnetInit also set netDev->macAddr, mtu, addrLen */
    if ((ret = VirtnetInit(netDev)) != HDF_SUCCESS) {
        return ret;
    }
    netDev->flags = NET_DEVICE_IFF_RUNNING;
    netDev->neededHeadRoom = 0;
    netDev->neededTailRoom = 0;
    netDev->funType.wlanType = PROTOCOL_80211_IFTYPE_STATION;
    netDev->netDeviceIf = &g_netDevOps;
    if ((ret = NetDeviceAdd(netDev)) != HDF_SUCCESS) {
        goto ERR_OUT;
    }
    if ((ret = CreateEapolData(netDev)) != HDF_SUCCESS) {
        goto ERR_OUT;
    }

    /* everything is ready, now notify device the receive buffers */
    struct VirtNetif *nic = GetVirtnetIf(netDev);
    nic->dev.vq[0].avail->index = nic->dev.vq[0].qsz;
    OSAL_WRITEL(0, nic->dev.base + VIRTMMIO_REG_QUEUENOTIFY);

    return VirtNetDeviceInitDone(netDev);

ERR_OUT:
    VirtnetDeInit(netDev);
    return ret;
}

static int32_t VirtNetDeviceDeInit(struct HdfChipDriver *chipDriver, NetDevice *netDev)
{
    (void)chipDriver;

    DestroyEapolData(netDev);

    if (GetVirtnetIf(netDev)) {
        VirtnetDeInit(netDev);
    }

    return NetDeviceDelete(netDev);
}


/*
 * Followings are mainly fake data & funcs to mimic a wireless card.
 *
 * Here is fake MAC80211 base operations.
 */
static int32_t FakeWalSetMode(NetDevice *netDev, enum WlanWorkMode mode)
{
    (void)netDev;
    if (mode != WLAN_WORKMODE_STA) {
        HDF_LOGE("[%s]unsupported WLAN mode: %u", __func__, mode);
        return HDF_ERR_NOT_SUPPORT;
    }
    return HDF_SUCCESS;
}
static int32_t FakeWalAddKey(NetDevice *netDev, uint8_t keyIndex, bool pairwise, const uint8_t *macAddr,
                             struct KeyParams *params)
{
    (void)netDev;
    (void)keyIndex;
    (void)pairwise;
    (void)macAddr;
    (void)params;
    return HDF_SUCCESS;
}
static int32_t FakeWalDelKey(NetDevice *netDev, uint8_t keyIndex, bool pairwise, const uint8_t *macAddr)
{
    (void)netDev;
    (void)keyIndex;
    (void)pairwise;
    (void)macAddr;
    return HDF_SUCCESS;
}
static int32_t FakeWalSetDefaultKey(NetDevice *netDev, uint8_t keyIndex, bool unicast, bool multicas)
{
    (void)netDev;
    (void)keyIndex;
    (void)unicast;
    (void)multicas;
    return HDF_SUCCESS;
}
static int32_t FakeWalGetDeviceMacAddr(NetDevice *netDev, int32_t type, uint8_t *mac, uint8_t len)
{
    (void)netDev;
    (void)type;

    for (int i = 0; i < len && i < netDev->addrLen; i++) {
        mac[i] = netDev->macAddr[i];
    }

    return HDF_SUCCESS;
}
static int32_t FakeWalSetMacAddr(NetDevice *netDev, uint8_t *mac, uint8_t len)
{
    (void)netDev;

    for (int i = 0; i < len && i < netDev->addrLen; i++) {
        netDev->macAddr[i] = mac[i];
    }

    return HDF_SUCCESS;
}
static int32_t FakeWalSetTxPower(NetDevice *netDev, int32_t power)
{
    (void)netDev;
    (void)power;
    return HDF_SUCCESS;
}
#define FAKE_MAGIC_BAND     20
#define FAKE_MAGIC_FREQ     2412
#define FAKE_MAGIC_CAPS     20
#define FAKE_MAGIC_RATE     100
static int32_t FakeWalGetValidFreqsWithBand(NetDevice *netDev, int32_t band, int32_t *freqs, uint32_t *num)
{
    (void)netDev;

    if (band != FAKE_MAGIC_BAND) {
        HDF_LOGE("[%s]unsupported WLAN band: %dMHz", __func__, band);
        return HDF_ERR_NOT_SUPPORT;
    }

    *freqs = FAKE_MAGIC_FREQ; /* MHz, channel 1 */
    *num = 1;
    return HDF_SUCCESS;
}
struct MemAllocForWlanHwCapability {
    struct WlanHwCapability cap;
    struct WlanBand band;
    struct WlanChannel ch;
    uint16_t rate;
};
static void FakeWalHwCapabilityRelease(struct WlanHwCapability *self)
{
    OsalMemFree(self);
}
static int32_t FakeWalGetHwCapability(NetDevice *netDev, struct WlanHwCapability **capability)
{
    (void)netDev;

    struct MemAllocForWlanHwCapability *p = OsalMemCalloc(sizeof(struct MemAllocForWlanHwCapability));
    if (p == NULL) {
        HDF_LOGE("[%s]alloc memory failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }
    p->cap.Release = FakeWalHwCapabilityRelease;
    p->cap.bands[0] = &p->band;
    p->cap.htCapability = FAKE_MAGIC_CAPS;
    p->cap.supportedRateCount = 1;
    p->cap.supportedRates = &p->rate;
    p->band.channelCount = 1;
    p->ch.channelId = 1;
    p->ch.centerFreq = FAKE_MAGIC_FREQ;
    p->ch.flags = WLAN_CHANNEL_FLAG_NO_IR | WLAN_CHANNEL_FLAG_DFS_UNAVAILABLE;
    p->rate = FAKE_MAGIC_RATE;

    *capability = &p->cap;
    return HDF_SUCCESS;
}
static struct HdfMac80211BaseOps g_fakeBaseOps = {
    .SetMode = FakeWalSetMode,
    .AddKey = FakeWalAddKey,
    .DelKey = FakeWalDelKey,
    .SetDefaultKey = FakeWalSetDefaultKey,
    .GetDeviceMacAddr = FakeWalGetDeviceMacAddr,
    .SetMacAddr = FakeWalSetMacAddr,
    .SetTxPower = FakeWalSetTxPower,
    .GetValidFreqsWithBand = FakeWalGetValidFreqsWithBand,
    .GetHwCapability = FakeWalGetHwCapability
};


/*
 * Fake STA operations.
 */
static int32_t FakeStaConnect(NetDevice *netDev, WlanConnectParams *param)
{
    (void)netDev;
    (void)param;
    return HDF_SUCCESS;
}
static int32_t FakeStaDisonnect(NetDevice *netDev, uint16_t reasonCode)
{
    (void)netDev;
    (void)reasonCode;
    return HDF_SUCCESS;
}
static int32_t FakeStaStartScan(NetDevice *netDev, struct WlanScanRequest *param)
{
    (void)netDev;
    (void)param;
    return HDF_SUCCESS;
}
static int32_t FakeStaAbortScan(NetDevice *netDev)
{
    (void)netDev;
    return HDF_SUCCESS;
}
static int32_t FakeStaSetScanningMacAddress(NetDevice *netDev, unsigned char *mac, uint32_t len)
{
    (void)netDev;
    (void)mac;
    (void)len;
    return HDF_SUCCESS;
}
static struct HdfMac80211STAOps g_fakeStaOps = {
    .Connect = FakeStaConnect,
    .Disconnect = FakeStaDisonnect,
    .StartScan = FakeStaStartScan,
    .AbortScan = FakeStaAbortScan,
    .SetScanningMacAddress = FakeStaSetScanningMacAddress,
};


/*
 * Fake factory & chip functions.
 */
static struct HdfChipDriver *FakeFactoryInitChip(struct HdfWlanDevice *device, uint8_t ifIndex)
{
    struct HdfChipDriver *chipDriver = NULL;
    if (device == NULL || ifIndex > 0) {
        HDF_LOGE("[%s]HdfWlanDevice is NULL or ifIndex>0", __func__);
        return NULL;
    }
    chipDriver = OsalMemCalloc(sizeof(struct HdfChipDriver));
    if (chipDriver == NULL) {
        HDF_LOGE("[%s]alloc memory failed", __func__);
        return NULL;
    }

    if (strcpy_s(chipDriver->name, MAX_WIFI_COMPONENT_NAME_LEN, VIRTMMIO_NETIF_NAME) != EOK) {
        HDF_LOGE("[%s]strcpy_s failed", __func__);
        OsalMemFree(chipDriver);
        return NULL;
    }
    chipDriver->init = VirtNetDeviceInit;
    chipDriver->deinit = VirtNetDeviceDeInit;
    chipDriver->ops = &g_fakeBaseOps;
    chipDriver->staOps = &g_fakeStaOps;

    return chipDriver;
}
static void FakeFactoryReleaseChip(struct HdfChipDriver *chipDriver)
{
    if (chipDriver == NULL) {
        return;
    }
    if (strcmp(chipDriver->name, VIRTMMIO_NETIF_NAME) != 0) {
        HDF_LOGE("[%s]not my driver: %s", __func__, chipDriver->name);
        return;
    }
    OsalMemFree(chipDriver);
}
static uint8_t FakeFactoryGetMaxIFCount(struct HdfChipDriverFactory *factory)
{
    (void)factory;
    return 1;
}
static void FakeFactoryRelease(struct HdfChipDriverFactory *factory)
{
    OsalMemFree(factory);
}
static int32_t FakeFactoryInit(void)
{
    struct HdfChipDriverManager *driverMgr = NULL;
    struct HdfChipDriverFactory *tmpFactory = NULL;
    int32_t ret;

    tmpFactory = OsalMemCalloc(sizeof(struct HdfChipDriverFactory));
    if (tmpFactory == NULL) {
        HDF_LOGE("[%s]alloc memory failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    driverMgr = HdfWlanGetChipDriverMgr();
    if (driverMgr == NULL || driverMgr->RegChipDriver == NULL) {
        HDF_LOGE("[%s]driverMgr or its RegChipDriver is NULL!", __func__);
        OsalMemFree(tmpFactory);
        return HDF_FAILURE;
    }

#define VIRTMMIO_WIFI_NAME  "fakewifi"  /* must match wlan_chip_virtnet.hcs::driverName */
    tmpFactory->driverName = VIRTMMIO_WIFI_NAME;
    tmpFactory->ReleaseFactory = FakeFactoryRelease;
    tmpFactory->Build = FakeFactoryInitChip;
    tmpFactory->Release = FakeFactoryReleaseChip;
    tmpFactory->GetMaxIFCount = FakeFactoryGetMaxIFCount;
    if ((ret = driverMgr->RegChipDriver(tmpFactory)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]register chip driver failed: %d", __func__, ret);
        OsalMemFree(tmpFactory);
        return ret;
    }

    return HDF_SUCCESS;
}


/*
 * HDF entry.
 */

static int32_t FakeWifiInit(struct HdfDeviceObject *device)
{
    struct VirtmmioDev dev;

    if (device == NULL) {
        HDF_LOGE("[%s]device is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    /* only when virtio-net do exist, we go on the other lots of work */
    if (!VirtmmioDiscover(VIRTMMIO_DEVICE_ID_NET, &dev)) {
        return HDF_ERR_INVALID_OBJECT;
    }

    return FakeFactoryInit();
}

static void FakeWifiRelease(struct HdfDeviceObject *deviceObject)
{
    if (deviceObject) {
        (void)ChipDriverMgrDeInit();
    }
}

struct HdfDriverEntry g_fakeWifiEntry = {
    .moduleVersion = 1,
    .Init = FakeWifiInit,
    .Release = FakeWifiRelease,
    .moduleName = "HDF_FAKE_WIFI"
};

HDF_INIT(g_fakeWifiEntry);
