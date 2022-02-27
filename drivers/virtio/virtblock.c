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
 * Simple virtio-mmio block driver emulating MMC device (spec 4.3).
 */

#include "los_vm_iomap.h"
#include "mmc_block.h"
#include "dmac_core.h"
#include "osal.h"
#include "osal/osal_io.h"
#include "virtmmio.h"

/*
 * Kernel take care lock & cache(bcache), we only take care I/O.
 * When I/O is carrying, we must wait for completion. Since any
 * time there is only one I/O request come here, we can use the
 * most simple virt-queue -- only have 4 descriptors: one for
 * "request header", one for "I/O buffer", one for "response",
 * and one left unused. That is, the driver and the device are
 * always in synchonous mode!
 */
#define VIRTQ_REQUEST_QSZ       4

#define VIRTIO_BLK_F_RO         (1 << 5)
#define VIRTIO_BLK_F_BLK_SIZE   (1 << 6)
#define VIRTMMIO_BLK_NAME       "virtblock"
#define VIRTBLK_DRIVER          "/dev/mmcblk"
#define VIRTBLK_DEF_BLK_SIZE    8192

struct VirtblkConfig {
    uint64_t capacity;
    uint32_t segMax;
    struct VirtblkGeometry {
        uint16_t cylinders;
        uint8_t heads;
        uint8_t sectors;
    } geometry;
    uint32_t blkSize;
    uint8_t otherFieldsOmitted[0];
};

/* request type: only support IN, OUT */
#define VIRTIO_BLK_T_IN             0
#define VIRTIO_BLK_T_OUT            1
#define VIRTIO_BLK_T_FLUSH          4
#define VIRTIO_BLK_T_DISCARD        11
#define VIRTIO_BLK_T_WRITE_ZEROES   13

/* response status */
#define VIRTIO_BLK_S_OK             0
#define VIRTIO_BLK_S_IOERR          1
#define VIRTIO_BLK_S_UNSUPP         2

struct VirtblkReq {
    uint32_t type;
    uint32_t reserved;
    uint64_t startSector;
};

struct Virtblk {
    struct VirtmmioDev dev;

    uint64_t capacity;      /* in 512-byte-sectors */
    uint32_t blkSize;       /* block(cluster) size */
    struct VirtblkReq req;  /* static memory for request */
    uint8_t resp;           /*              and response */
    DmacEvent event;        /* for waiting I/O completion */
};

#define FAT32_MAX_CLUSTER_SECS  128

static bool Feature0(uint32_t features, uint32_t *supported, void *dev)
{
    struct Virtblk *blk = dev;
    struct VirtblkConfig *conf = (void *)(blk->dev.base + VIRTMMIO_REG_CONFIG);
    uint32_t bs;

    if (features & VIRTIO_BLK_F_RO) {
        HDF_LOGE("[%s]not support readonly device", __func__);
        return false;
    }

    blk->blkSize = VIRTBLK_DEF_BLK_SIZE;
    if (features & VIRTIO_BLK_F_BLK_SIZE) {
        bs = conf->blkSize;
        if ((bs >= MMC_SEC_SIZE) && (bs <= FAT32_MAX_CLUSTER_SECS * MMC_SEC_SIZE) &&
            ((bs & (bs - 1)) == 0)) {
            blk->blkSize = bs;
            *supported |= VIRTIO_BLK_F_BLK_SIZE;
        }
    }

    blk->capacity = conf->capacity;
    return true;
}

static bool Feature1(uint32_t features, uint32_t *supported, void *dev)
{
    (void)dev;
    if (features & VIRTIO_F_VERSION_1) {
        *supported |= VIRTIO_F_VERSION_1;
    } else {
        HDF_LOGE("[%s]virtio-mmio block has no VERSION_1 feature", __func__);
        return false;
    }

    return true;
}

static void PopulateRequestQ(const struct Virtblk *blk)
{
    const struct Virtq *q = &blk->dev.vq[0];
    int i = 0;

    q->desc[i].pAddr = VMM_TO_DMA_ADDR((VADDR_T)&blk->req);
    q->desc[i].len = sizeof(struct VirtblkReq);
    q->desc[i].flag = VIRTQ_DESC_F_NEXT;
    q->desc[i].next = i + 1;

    i++;
    q->desc[i].next = i + 1;

    i++;
    q->desc[i].pAddr = VMM_TO_DMA_ADDR((VADDR_T)&blk->resp);
    q->desc[i].len = sizeof(uint8_t);
    q->desc[i].flag = VIRTQ_DESC_F_WRITE;
}

static uint8_t VirtblkIO(struct Virtblk *blk, uint32_t cmd, uint64_t startSector,
                         uint8_t *buf, uint32_t sectors)
{
    uint32_t ret;
    struct Virtq *q = &blk->dev.vq[0];

    /* fill in and notify virt queue */
    blk->req.type = cmd;
    blk->req.startSector = startSector;
    q->desc[1].pAddr = VMM_TO_DMA_ADDR((VADDR_T)buf);
    q->desc[1].len = sectors * MMC_SEC_SIZE;
    if (cmd == VIRTIO_BLK_T_IN) {
        q->desc[1].flag = VIRTQ_DESC_F_NEXT | VIRTQ_DESC_F_WRITE;
    } else { /* must be VIRTIO_BLK_T_OUT */
        q->desc[1].flag = VIRTQ_DESC_F_NEXT;
    }
    q->avail->ring[q->avail->index % q->qsz] = 0;
    DSB;
    q->avail->index++;
    OSAL_WRITEL(0, blk->dev.base + VIRTMMIO_REG_QUEUENOTIFY);

    /* wait for completion */
    if ((ret = DmaEventWait(&blk->event, 1, HDF_WAIT_FOREVER)) != 1) {
        HDF_LOGE("[%s]FATAL: wait event failed: %u", __func__, ret);
        return VIRTIO_BLK_S_IOERR;
    }

    return blk->resp;
}

static uint32_t VirtblkIRQhandle(uint32_t swIrq, void *dev)
{
    (void)swIrq;
    struct Virtblk *blk = dev;
    struct Virtq *q = &blk->dev.vq[0];

    if (!(OSAL_READL(blk->dev.base + VIRTMMIO_REG_INTERRUPTSTATUS) & VIRTMMIO_IRQ_NOTIFY_USED)) {
        return 1;
    }

    (void)DmaEventSignal(&blk->event, 1);
    q->last++;

    OSAL_WRITEL(VIRTMMIO_IRQ_NOTIFY_USED, blk->dev.base + VIRTMMIO_REG_INTERRUPTACK);
    return 0;
}

static void VirtblkDeInit(struct Virtblk *blk)
{
    if (blk->dev.irq & ~_IRQ_MASK) {
        OsalUnregisterIrq(blk->dev.irq & _IRQ_MASK, blk);
    }
    LOS_DmaMemFree(blk);
}

static struct Virtblk *VirtblkInitDev(void)
{
    struct Virtblk *blk = NULL;
    VADDR_T base;
    uint16_t qsz;
    int len, ret;

    len = sizeof(struct Virtblk) + VirtqSize(VIRTQ_REQUEST_QSZ);
    if ((blk = LOS_DmaMemAlloc(NULL, len, sizeof(void *), DMA_CACHE)) == NULL) {
        HDF_LOGE("[%s]alloc virtio-block memory failed", __func__);
        return NULL;
    }
    memset_s(blk, len, 0, len);

    if (!VirtmmioDiscover(VIRTMMIO_DEVICE_ID_BLK, &blk->dev)) {
        goto ERR_OUT;
    }

    VirtmmioInitBegin(&blk->dev);
    if (!VirtmmioNegotiate(&blk->dev, Feature0, Feature1, blk)) {
        goto ERR_OUT1;
    }
    base = ALIGN((VADDR_T)blk + sizeof(struct Virtblk), VIRTQ_ALIGN_DESC);
    qsz = VIRTQ_REQUEST_QSZ;
    if (VirtmmioConfigQueue(&blk->dev, base, &qsz, 1) == 0) {
        goto ERR_OUT1;
    }

    if ((ret = DmaEventInit(&blk->event)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]initialize event control block failed: %#x", __func__, ret);
        goto ERR_OUT1;
    }
    ret = OsalRegisterIrq(blk->dev.irq, OSAL_IRQF_TRIGGER_NONE, (OsalIRQHandle)VirtblkIRQhandle,
                          VIRTMMIO_BLK_NAME, blk);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]register IRQ failed: %d", __func__, ret);
        goto ERR_OUT1;
    }
    blk->dev.irq |= ~_IRQ_MASK;

    PopulateRequestQ(blk);
    VritmmioInitEnd(&blk->dev);  /* now virt queue can be used */
    return blk;

ERR_OUT1:
    VirtmmioInitFailed(&blk->dev);
ERR_OUT:
    VirtblkDeInit(blk);
    return NULL;
}


/*
 * MMC code
 *
 * HDF MmcCntlr act like an application adapter, they discover MMC device,
 * send I/O request, receive response data.
 * We act like a card adapter, receive requests, control MMC bus, drive 'card'
 * execution, and report result. Yes, we are part of MmcCntlr -- MMC controller!
 * Every hardware internal infomation are in our scope, such as state, CRC, RCA.
 * So, we COULD(SHOULD) safely ignore them!
 */

#define OCR_LE_2G       (0x00FF8080 | MMC_CARD_BUSY_STATUS)
#define OCR_GT_2G       (0x40FF8080 | MMC_CARD_BUSY_STATUS)
#define CAPACITY_2G     (0x80000000 / 512)
#define READ_BL_LEN     11
#define C_SIZE_MULT     7
#define U32_BITS        32

/*
 * Example bits: start=62 bits=4 value=0b1011
 *
 *             bit127
 *  resp[0]
 *  resp[1]                         1   0
 *  resp[2]      1    1
 *  resp[3]
 *                                     bit0
 *
 * NOTE: no error check, related 'resp' bits must be zeroed and set only once.
 */
static void FillCidCsdBits(uint32_t *resp, int start, int bits, uint32_t value)
{
    uint32_t index, lsb;

    index = CID_LEN - 1 - start / U32_BITS; /* CSD has the same length */
    lsb = start % U32_BITS;
    resp[index] |= value << lsb;

    if (lsb + bits > U32_BITS) {
        resp[index - 1] |= value >> (U32_BITS - lsb);
    }
}

#define MMC_CID_CBX_SBIT    112
#define MMC_CID_CBX_WIDTH   2
#define MMC_CID_PNM_SBYTE   3
#define MMC_CID_PNM_BYTES   6
#define MMC_CID_PSN_SBYTE   10
static void VirtMmcFillRespCid(struct MmcCmd *cmd, const struct Virtblk *blk)
{
    uint8_t *b = (uint8_t *)cmd->resp;

    /* removable card, so OHOS will auto-detect partitions */
    FillCidCsdBits(cmd->resp, MMC_CID_CBX_SBIT, MMC_CID_CBX_WIDTH, 0);

    (void)memcpy_s(&b[MMC_CID_PNM_SBYTE], MMC_CID_PNM_BYTES, VIRTMMIO_BLK_NAME, MMC_CID_PNM_BYTES);
    *(uint32_t *)&b[MMC_CID_PSN_SBYTE] = (uint32_t)blk; /* unique sn */
    /* leave other fields random */
}

#define MMC_CSD_STRUCT_SBIT     126
#define MMC_CSD_STRUCT_WIDTH    2

#define MMC_CSD_VERS_SBIT       122
#define MMC_CSD_VERS_WIDTH      4

#define MMC_CSD_CCC_SBIT        84
#define MMC_CSD_CCC_WIDTH       12

#define MMC_CSD_RBLEN_SBIT      80
#define MMC_CSD_RBLEN_WIDTH     4

#define MMC_CSD_RBPART_SBIT     79

#define MMC_CSD_WBMISALIGN_SBIT 78

#define MMC_CSD_RBMISALIGN_SBIT 77

#define MMC_CSD_DSRIMP_SBIT     76

#define MMC_CSD_CSIZE_SBIT      62
#define MMC_CSD_CSIZE_WIDTH     12
#define MMC_CSD_CSIZE_VAL       0xFFF

#define MMC_CSD_CSIZEMUL_SBIT   47
#define MMC_CSD_CSIZEMUL_WIDTH  3

#define MMC_CSD_EGRPSIZE_SBIT   42
#define MMC_CSD_EGRPSIZE_WIDTH  5
#define MMC_CSD_EGRPSIZE_VAL    31

#define MMC_CSD_EGRPMULT_SBIT   37
#define MMC_CSD_EGRPMULT_WIDTH  5
#define MMC_CSD_EGRPMULT_VAL    15

#define MMC_CSD_WBLEN_SBIT      22
#define MMC_CSD_WBLEN_WIDTH     4

#define MMC_CSD_WBPART_SBIT     21

#define MMC_CSD_FFORMGRP_SBIT   15

#define MMC_CSD_FFORMAT_SBIT    10
#define MMC_CSD_FFORMAT_WIDTH   2
static void VirtMmcFillRespCsd(struct MmcCmd *cmd, const struct Virtblk *blk)
{
    FillCidCsdBits(cmd->resp, MMC_CSD_STRUCT_SBIT, MMC_CSD_STRUCT_WIDTH, MMC_CSD_STRUCTURE_VER_1_2);
    FillCidCsdBits(cmd->resp, MMC_CSD_VERS_SBIT, MMC_CSD_VERS_WIDTH, MMC_CSD_SPEC_VER_4);
    FillCidCsdBits(cmd->resp, MMC_CSD_CCC_SBIT, MMC_CSD_CCC_WIDTH, MMC_CSD_CCC_BASIC |
                                                MMC_CSD_CCC_BLOCK_READ | MMC_CSD_CCC_BLOCK_WRITE);
    FillCidCsdBits(cmd->resp, MMC_CSD_RBPART_SBIT, 1, 0);       /* READ_BL_PARTIAL: no */
    FillCidCsdBits(cmd->resp, MMC_CSD_WBMISALIGN_SBIT, 1, 0);   /* WRITE_BLK_MISALIGN: no */
    FillCidCsdBits(cmd->resp, MMC_CSD_RBMISALIGN_SBIT, 1, 0);   /* READ_BLK_MISALIGN: no */
    FillCidCsdBits(cmd->resp, MMC_CSD_DSRIMP_SBIT, 1, 0);       /* DSR_IMP: no */
    if (blk->capacity > CAPACITY_2G) {
        uint32_t e = U32_BITS - __builtin_clz(blk->blkSize) - 1;
        FillCidCsdBits(cmd->resp, MMC_CSD_RBLEN_SBIT, MMC_CSD_RBLEN_WIDTH, e);  /* READ_BL_LEN */
        FillCidCsdBits(cmd->resp, MMC_CSD_WBLEN_SBIT, MMC_CSD_WBLEN_WIDTH, e);  /* WRITE_BL_LEN */
        FillCidCsdBits(cmd->resp, MMC_CSD_CSIZE_SBIT, MMC_CSD_CSIZE_WIDTH, MMC_CSD_CSIZE_VAL);
    } else {                                /* ensure c_size can up to 2G (512B can't) */
        FillCidCsdBits(cmd->resp, MMC_CSD_RBLEN_SBIT, MMC_CSD_RBLEN_WIDTH, READ_BL_LEN);
        FillCidCsdBits(cmd->resp, MMC_CSD_WBLEN_SBIT, MMC_CSD_WBLEN_WIDTH, READ_BL_LEN);
        uint32_t size = blk->capacity*MMC_SEC_SIZE / (1<<READ_BL_LEN) / (1<<(C_SIZE_MULT+2)) - 1;
        FillCidCsdBits(cmd->resp, MMC_CSD_CSIZE_SBIT, MMC_CSD_CSIZE_WIDTH, size);   /* C_SIZE */
    }
    FillCidCsdBits(cmd->resp, MMC_CSD_CSIZEMUL_SBIT, MMC_CSD_CSIZEMUL_WIDTH, C_SIZE_MULT);
    FillCidCsdBits(cmd->resp, MMC_CSD_EGRPSIZE_SBIT, MMC_CSD_EGRPSIZE_WIDTH, MMC_CSD_EGRPSIZE_VAL);
    FillCidCsdBits(cmd->resp, MMC_CSD_EGRPMULT_SBIT, MMC_CSD_EGRPMULT_WIDTH, MMC_CSD_EGRPMULT_VAL);
    FillCidCsdBits(cmd->resp, MMC_CSD_WBPART_SBIT, 1, 0);   /* WRITE_BL_PARTIAL: no */
    FillCidCsdBits(cmd->resp, MMC_CSD_FFORMGRP_SBIT, 1, 0); /* FILE_FORMAT_GRP */
    FillCidCsdBits(cmd->resp, MMC_CSD_FFORMAT_SBIT, MMC_CSD_FFORMAT_WIDTH, 0);  /* hard disk-like */
    /* leave other fields random */
}

#define EMMC_EXT_CSD_CMD_SET_REV    189
#define EMMC_EXT_CSD_CMD_SET        191
#define EMMC_EXT_CSD_ACC_SIZE       225
#define EMMC_EXT_CSD_S_CMD_SET      504
static void VirtMmcFillDataExtCsd(const struct MmcCmd *cmd, const struct Virtblk *blk)
{
    uint8_t *b = (uint8_t *)cmd->data->dataBuffer;

    b[EMMC_EXT_CSD_S_CMD_SET] = 0;      /* standard MMC */
    b[EMMC_EXT_CSD_ACC_SIZE] = blk->blkSize / MMC_SEC_SIZE;
    b[EMMC_EXT_CSD_REL_WR_SEC_C] = blk->blkSize / MMC_SEC_SIZE;
    *(uint32_t*)&b[EMMC_EXT_CSD_SEC_CNT] = blk->capacity;
    b[EMMC_EXT_CSD_CARD_TYPE] = EMMC_EXT_CSD_CARD_TYPE_26 | EMMC_EXT_CSD_CARD_TYPE_52;
    b[EMMC_EXT_CSD_STRUCTURE] = EMMC_EXT_CSD_STRUCTURE_VER_1_2;
    b[EMMC_EXT_CSD_REV] = EMMC_EXT_CSD_REV_1_3;
    b[EMMC_EXT_CSD_CMD_SET] = 0;        /* standard MMC */
    b[EMMC_EXT_CSD_CMD_SET_REV] = 0;    /* v4.0 */
    b[EMMC_EXT_CSD_BUS_WIDTH] = EMMC_EXT_CSD_BUS_WIDTH_1;
    /* leave other fields random */
}

#define MMC_RESP_STATE_BIT  9
static inline void VirtMmcFillRespR1(struct MmcCmd *cmd)
{
    cmd->resp[0] = READY_FOR_DATA | (STATE_READY << MMC_RESP_STATE_BIT);
    cmd->resp[1] = cmd->cmdCode;
}

static int32_t VirtMmcIO(const struct MmcCntlr *cntlr, const struct MmcCmd *cmd)
{
    struct Virtblk *blk = cntlr->priv;
    uint64_t startSector = (uint64_t)cmd->argument;
    uint32_t io, ret;

    if (cntlr->curDev->state.bits.blockAddr == 0) {
        startSector >>= MMC_SEC_SHIFT;
    }

    if (cmd->data->dataFlags == DATA_READ) {
        io = VIRTIO_BLK_T_IN;
    } else {
        io = VIRTIO_BLK_T_OUT;
    }

    ret = VirtblkIO(blk, io, startSector, cmd->data->dataBuffer, cmd->data->blockNum);
    if (ret == VIRTIO_BLK_S_OK) {
        cmd->data->returnError = HDF_SUCCESS;
    } else {
        HDF_LOGE("[%s]QEMU backend I/O error", __func__);
        cmd->data->returnError = HDF_ERR_IO;
    }

    return HDF_SUCCESS;
}

static int32_t VirtMmcDoRequest(struct MmcCntlr *cntlr, struct MmcCmd *cmd)
{
    if ((cntlr == NULL) || (cntlr->priv == NULL) || (cmd == NULL)) {
        return HDF_ERR_INVALID_OBJECT;
    }
    struct Virtblk *blk = cntlr->priv;

    cmd->returnError = HDF_SUCCESS;
    switch (cmd->cmdCode) {
        case GO_IDLE_STATE:         // CMD0
            break;
        case SEND_OP_COND:          // CMD1
            if (blk->capacity > CAPACITY_2G) {
                cmd->resp[0] = OCR_GT_2G;
            } else {
                cmd->resp[0] = OCR_LE_2G;
            }
            break;
        case ALL_SEND_CID:          // CMD2, fall through
        case SEND_CID:              // CMD10
            VirtMmcFillRespCid(cmd, blk);
            break;
        case SEND_EXT_CSD:          // CMD8, fall through
            VirtMmcFillDataExtCsd(cmd, blk);
            cmd->data->returnError = HDF_SUCCESS;
        case SET_RELATIVE_ADDR:     // CMD3, fall through
        case SWITCH:                // CMD6, fall through
        case SELECT_CARD:           // CMD7, fall through
        case SEND_STATUS:           // CMD13
            VirtMmcFillRespR1(cmd);
            break;
        case SEND_CSD:              // CMD9
            VirtMmcFillRespCsd(cmd, blk);
            break;
        case READ_SINGLE_BLOCK:     // CMD17, fall through
        case READ_MULTIPLE_BLOCK:   // CMD18, fall through
        case WRITE_BLOCK:           // CMD24, fall through
        case WRITE_MULTIPLE_BLOCK:  // CMD25
            return VirtMmcIO(cntlr, cmd);
        default:
            HDF_LOGE("[%s]unsupported command: %u", __func__, cmd->cmdCode);
            cmd->returnError = HDF_ERR_NOT_SUPPORT;
    }
    return cmd->returnError;
}

static bool VirtMmcPluged(struct MmcCntlr *cntlr)
{
    (void)cntlr;
    return true;
}

static bool VirtMmcBusy(struct MmcCntlr *cntlr)
{
    (void)cntlr;
    return false;
}

static struct MmcCntlrOps g_virtblkOps = {
    .request = VirtMmcDoRequest,
    .devPluged = VirtMmcPluged,
    .devBusy = VirtMmcBusy,
};


/*
 * HDF entry
 */

static void HdfVirtblkRelease(struct HdfDeviceObject *deviceObject)
{
    struct MmcCntlr *cntlr = deviceObject->priv;
    struct Virtblk *blk = cntlr->priv;

    if (blk) {
        VirtblkDeInit(blk);
    }
    if (cntlr->curDev != NULL) {
        MmcDeviceRemove(cntlr->curDev);
        OsalMemFree(cntlr->curDev);
        cntlr->curDev = NULL;
    }
    MmcCntlrRemove(cntlr);
    OsalMemFree(cntlr);
}

static int32_t HdfVirtblkBind(struct HdfDeviceObject *obj)
{
    struct MmcCntlr *cntlr = NULL;
    struct Virtblk *blk = NULL;
    int32_t ret;

    if (obj == NULL) {
        HDF_LOGE("[%s]HdfDeviceObject is NULL", __func__);
        return HDF_ERR_INVALID_OBJECT;
    }

    cntlr = OsalMemCalloc(sizeof(struct MmcCntlr));
    if (cntlr == NULL) {
        HDF_LOGE("[%s]alloc MmcCntlr memory failed", __func__);
        return HDF_ERR_MALLOC_FAIL;
    }

    if ((blk = VirtblkInitDev()) == NULL) {
        OsalMemFree(cntlr);
        return HDF_FAILURE;
    }

    obj->service = &cntlr->service;
    obj->priv = cntlr;
    cntlr->priv = blk;
    cntlr->ops = &g_virtblkOps;
    cntlr->hdfDevObj = obj;
    if ((ret = MmcCntlrParse(cntlr, obj)) != HDF_SUCCESS) {
        goto _ERR;
    }

    if ((ret = MmcCntlrAdd(cntlr)) != HDF_SUCCESS) {
        goto _ERR;
    }
    (void)MmcCntlrAddDetectMsgToQueue(cntlr);

    return HDF_SUCCESS;

_ERR:   /* Bind failure, so we must call release manually. */
    HdfVirtblkRelease(obj);
    return ret;
}

static int32_t HdfVirtblkInit(struct HdfDeviceObject *device)
{
    if (device == NULL) {
        HDF_LOGE("[%s]device is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    return HDF_SUCCESS;
}

struct HdfDriverEntry g_virtBlockEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_VIRTIO_BLOCK",
    .Bind = HdfVirtblkBind,
    .Init = HdfVirtblkInit,
    .Release = HdfVirtblkRelease,
};

HDF_INIT(g_virtBlockEntry);
