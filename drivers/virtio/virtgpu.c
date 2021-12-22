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
 * Simple virtio-mmio gpu driver, without hardware accelarator.
 * Using only synchronous request/response, no IRQ.
 */

#include "osal.h"
#include "osal_io.h"
#include "hdf_device_desc.h"
#include "fb.h"
#include "los_vm_phys.h"
#include "los_vm_iomap.h"
#include "virtmmio.h"

#define VIRTIO_GPU_F_EDID   (1 << 1)

#define VIRTQ_CONTROL_QSZ   4
#define VIRTQ_CURSOR_QSZ    2
#define NORMAL_CMD_ENTRIES  2

#define FB_WIDTH_DFT        800
#define FB_HEIGHT_DFT       480
#define GPU_DFT_RATE        (1000 / 30)    /* ms, 30Hz */
#define PIXEL_BYTES         4

#define RESOURCEID_FB      1

enum VirtgpuCtrlType {
    /* 2d commands */
    VIRTIO_GPU_CMD_GET_DISPLAY_INFO = 0x0100,
    VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
    VIRTIO_GPU_CMD_RESOURCE_UNREF,
    VIRTIO_GPU_CMD_SET_SCANOUT,
    VIRTIO_GPU_CMD_RESOURCE_FLUSH,
    VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
    VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
    VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING,
    VIRTIO_GPU_CMD_GET_CAPSET_INFO,
    VIRTIO_GPU_CMD_GET_CAPSET,
    VIRTIO_GPU_CMD_GET_EDID,
    /* cursor commands */
    VIRTIO_GPU_CMD_UPDATE_CURSOR = 0x0300,
    VIRTIO_GPU_CMD_MOVE_CURSOR,
    /* success responses */
    VIRTIO_GPU_RESP_OK_NODATA = 0x1100,
    VIRTIO_GPU_RESP_OK_DISPLAY_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET_INFO,
    VIRTIO_GPU_RESP_OK_CAPSET,
    VIRTIO_GPU_RESP_OK_EDID,
    /* error responses */
    VIRTIO_GPU_RESP_ERR_UNSPEC = 0x1200,
    VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY,
    VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID,
    VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER,
};

enum VirtgpuFormats {
    VIRTIO_GPU_FORMAT_B8G8R8A8_UNORM = 1,
    VIRTIO_GPU_FORMAT_B8G8R8X8_UNORM,
    VIRTIO_GPU_FORMAT_A8R8G8B8_UNORM,
    VIRTIO_GPU_FORMAT_X8R8G8B8_UNORM,

    VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM = 67,
    VIRTIO_GPU_FORMAT_X8B8G8R8_UNORM,

    VIRTIO_GPU_FORMAT_A8B8G8R8_UNORM = 121,
    VIRTIO_GPU_FORMAT_R8G8B8X8_UNORM = 134,
};

struct VirtgpuCtrlHdr {
    uint32_t type;
#define VIRTIO_GPU_FLAG_FENCE (1 << 0)
    uint32_t flags;
    uint64_t fenceId;
    uint32_t ctxId;
    uint32_t padding;
};

struct VirtgpuRect {
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

struct VirtgpuResourceFlush {
    struct VirtgpuCtrlHdr hdr;
    struct VirtgpuRect r;
    uint32_t resourceId;
    uint32_t padding;
};

struct VirtgpuTransferToHost2D {
    struct VirtgpuCtrlHdr hdr;
    struct VirtgpuRect r;
    uint64_t offset;
    uint32_t resourceId;
    uint32_t padding;
};

struct Virtgpu {
    struct VirtmmioDev      dev;
    OSAL_DECLARE_TIMER(timer);          /* refresh timer */

    struct VirtgpuRect      screen;
    uint8_t                 *fb;        /* frame buffer */
    bool                    edid;

    /*
     * Normal operations(timer refresh) request/response buffers.
     * We do not wait for their completion, so they must be static memory.
     * When an operation happened, the last one must already done.
     * Response is shared and ignored.
     *
     * control queue 4 descs: 0-trans_req 1-trans_resp 2-flush_req 3-flush_resp
     *                        0-... (30Hz is enough to avoid override)
     */
    struct VirtgpuResourceFlush     flushReq;
    struct VirtgpuTransferToHost2D  transReq;
    struct VirtgpuCtrlHdr           resp;
};
static struct Virtgpu *g_virtGpu;   /* fb module need this data, using global for simplicity */

static const char *ErrString(int err)
{
    switch (err) {
        case VIRTIO_GPU_RESP_ERR_UNSPEC: return "unspec";
        case VIRTIO_GPU_RESP_ERR_OUT_OF_MEMORY: return "out of memory";
        case VIRTIO_GPU_RESP_ERR_INVALID_SCANOUT_ID: return "invalid scanout ID";
        case VIRTIO_GPU_RESP_ERR_INVALID_RESOURCE_ID: return "invalid resource ID";
        case VIRTIO_GPU_RESP_ERR_INVALID_CONTEXT_ID: return "invalid context ID";
        case VIRTIO_GPU_RESP_ERR_INVALID_PARAMETER: return "invalid parameter";
        default: break;
    }
    return "unknown error";
}

static bool Feature0(uint32_t features, uint32_t *supported, void *dev)
{
    struct Virtgpu *gpu = dev;

    if (features & VIRTIO_GPU_F_EDID) {
        *supported |= VIRTIO_GPU_F_EDID;
        gpu->edid = true;
    }

    return true;
}

static bool Feature1(uint32_t features, uint32_t *supported, void *dev)
{
    (void)dev;
    if (features & VIRTIO_F_VERSION_1) {
        *supported |= VIRTIO_F_VERSION_1;
    } else {
        HDF_LOGE("[%s]virtio-gpu has no VERSION_1 feature", __func__);
        return false;
    }

    return true;
}

static bool NotifyAndWaitResponse(unsigned queue, struct Virtq *q, const void *req, volatile void *resp)
{
    const struct VirtgpuCtrlHdr *a = req;
    volatile struct VirtgpuCtrlHdr *b = resp;

    /* always use desc[0] [1] ([2]) for request-wait-response */
    q->avail->ring[q->avail->index % q->qsz] = 0;
    DSB;
    q->avail->index++;
    OSAL_WRITEL(queue, g_virtGpu->dev.base + VIRTMMIO_REG_QUEUENOTIFY);

    /* spin for response */
    while ((q->last == q->used->index) ||
           ((a->flags == VIRTIO_GPU_FLAG_FENCE) && (a->fenceId != b->fenceId))) {
        DSB;
    }
    q->last++;

    if ((b->type < VIRTIO_GPU_RESP_OK_NODATA) || (b->type > VIRTIO_GPU_RESP_OK_EDID)) {
        HDF_LOGE("[%s]virtio-gpu command=0x%x error=0x%x: %s", __func__, a->type, b->type, ErrString(b->type));
        return false;
    }

    return true;
}

static bool RequestResponse(unsigned queue, const void *req, size_t reqSize, volatile void *resp, size_t respSize)
{
    struct Virtq *q = &g_virtGpu->dev.vq[queue];
    uint16_t idx = 0;

    /* NOTE: We need these data physical continuous. They came from kernel stack, so they must. */
    q->desc[idx].pAddr = VMM_TO_DMA_ADDR((VADDR_T)req);
    q->desc[idx].len = reqSize;
    q->desc[idx].flag = VIRTQ_DESC_F_NEXT;
    q->desc[idx].next = idx + 1;
    idx++;
    q->desc[idx].pAddr = VMM_TO_DMA_ADDR((VADDR_T)resp);
    q->desc[idx].len = respSize;
    q->desc[idx].flag = VIRTQ_DESC_F_WRITE;

    return NotifyAndWaitResponse(queue, q, req, resp);
}

static bool RequestDataResponse(const void *req, size_t reqSize, const void *data,
                                size_t dataSize, volatile void *resp, size_t respSize)
{
    struct Virtq *q = &g_virtGpu->dev.vq[0];
    uint16_t idx = 0;

    q->desc[idx].pAddr = VMM_TO_DMA_ADDR((VADDR_T)req);
    q->desc[idx].len = reqSize;
    q->desc[idx].flag = VIRTQ_DESC_F_NEXT;
    q->desc[idx].next = idx + 1;
    idx++;
    q->desc[idx].pAddr = VMM_TO_DMA_ADDR((VADDR_T)data);
    q->desc[idx].len = dataSize;
    q->desc[idx].flag = VIRTQ_DESC_F_NEXT;
    q->desc[idx].next = idx + 1;
    idx++;
    q->desc[idx].pAddr = VMM_TO_DMA_ADDR((VADDR_T)resp);
    q->desc[idx].len = respSize;
    q->desc[idx].flag = VIRTQ_DESC_F_WRITE;

    return NotifyAndWaitResponse(0, q, req, resp);
}

/* For normal display refresh, do not wait response */
static void RequestNoResponse(unsigned queue, const void *req, size_t reqSize, bool notify)
{
    struct Virtq *q = &g_virtGpu->dev.vq[queue];
    uint16_t head = q->last % q->qsz;   /* `last` record next writable desc entry for request */

    /* QEMU is busy for the full queue, give up this request */
    if (abs(q->avail->index - (volatile uint16_t)q->used->index) >= VIRTQ_CONTROL_QSZ) {
        return;
    }

    /* other fields initiated by PopulateVirtQ */
    q->desc[head].pAddr = VMM_TO_DMA_ADDR((VADDR_T)req);
    q->desc[head].len = reqSize;
    q->last += NORMAL_CMD_ENTRIES;

    q->avail->ring[q->avail->index % q->qsz] = head;
    DSB;
    q->avail->index++;

    if (notify) {
        OSAL_WRITEL(queue, g_virtGpu->dev.base + VIRTMMIO_REG_QUEUENOTIFY);
    }
}

#define VIRTIO_GPU_MAX_SCANOUTS 16
struct VirtgpuRespDisplayInfo {
    struct VirtgpuCtrlHdr hdr;
    struct {
        struct VirtgpuRect r;
        uint32_t enabled;
        uint32_t flags;
    } pmodes[VIRTIO_GPU_MAX_SCANOUTS];
};
static void CMDGetDisplayInfo(void)
{
    struct VirtgpuCtrlHdr req = {
        .type = VIRTIO_GPU_CMD_GET_DISPLAY_INFO
    };
    struct VirtgpuRespDisplayInfo resp = { 0 };

    if (!RequestResponse(0, &req, sizeof(req), &resp, sizeof(resp))) {
        goto DEFAULT;
    }

    if (resp.pmodes[0].enabled) {
        g_virtGpu->screen = resp.pmodes[0].r;
        return;
    } else {
        HDF_LOGE("[%s]scanout 0 not enabled", __func__);
    }

DEFAULT:
    g_virtGpu->screen.x = g_virtGpu->screen.y = 0;
    g_virtGpu->screen.width = FB_WIDTH_DFT;
    g_virtGpu->screen.height = FB_HEIGHT_DFT;
}

/* reserved for future use */
struct VirtgpuGetEdid {
    struct VirtgpuCtrlHdr hdr;
    uint32_t scanout;
    uint32_t padding;
};
struct VirtgpuRespEdid {
    struct VirtgpuCtrlHdr hdr;
    uint32_t size;
    uint32_t padding;
    uint8_t edid[1024];
};
static void CMDGetEdid(void)
{
    struct VirtgpuGetEdid req = {
        .hdr.type = VIRTIO_GPU_CMD_GET_EDID
    };
    struct VirtgpuRespEdid resp = { 0 };

    if (!RequestResponse(0, &req, sizeof(req), &resp, sizeof(resp))) {
        goto DEFAULT;
    }

DEFAULT:
    return;
}

struct VirtgpuResourceCreate2D {
    struct VirtgpuCtrlHdr hdr;
    uint32_t resourceId;
    uint32_t format;
    uint32_t width;
    uint32_t height;
};
static bool CMDResourceCreate2D(uint32_t resourceId)
{
    struct VirtgpuResourceCreate2D req = {
        .hdr.type = VIRTIO_GPU_CMD_RESOURCE_CREATE_2D,
        .resourceId = resourceId,
        .format = VIRTIO_GPU_FORMAT_R8G8B8A8_UNORM, /* sRGB, byte order: RGBARGBA... */
        .width = (resourceId == RESOURCEID_FB) ? g_virtGpu->screen.width : 0,
        .height = (resourceId == RESOURCEID_FB) ? g_virtGpu->screen.height : 0
    };
    struct VirtgpuCtrlHdr resp = { 0 };

    return RequestResponse(0, &req, sizeof(req), &resp, sizeof(resp));
}

struct VirtgpuSetScanout {
    struct VirtgpuCtrlHdr hdr;
    struct VirtgpuRect r;
    uint32_t scanoutId;
    uint32_t resourceId;
};
static bool CMDSetScanout(const struct VirtgpuRect *r)
{
    struct VirtgpuSetScanout req = {
        .hdr.type = VIRTIO_GPU_CMD_SET_SCANOUT,
        .r = *r,
        .resourceId = RESOURCEID_FB
    };
    struct VirtgpuCtrlHdr resp = { 0 };

    return RequestResponse(0, &req, sizeof(req), &resp, sizeof(resp));
}

static bool CMDTransferToHost(uint32_t resourceId, const struct VirtgpuRect *r)
{
    struct VirtgpuTransferToHost2D req = {
        .hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D,
        .hdr.flags = VIRTIO_GPU_FLAG_FENCE,
        .hdr.fenceId = r->x + r->y + r->width + r->height,
        .r = *r,
        .resourceId = resourceId,
    };
    struct VirtgpuCtrlHdr resp = { 0 };

    return RequestResponse(0, &req, sizeof(req), &resp, sizeof(resp));
}

static bool CMDResourceFlush(void)
{
    struct VirtgpuResourceFlush req = {
        .hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH,
        .r = g_virtGpu->screen,
        .resourceId = RESOURCEID_FB,
    };
    struct VirtgpuCtrlHdr resp = { 0 };

    return RequestResponse(0, &req, sizeof(req), &resp, sizeof(resp));
}

struct VirtgpuResourceAttachBacking {
    struct VirtgpuCtrlHdr hdr;
    uint32_t resourceId;
    uint32_t nrEntries;
};
struct VirtgpuMemEntry {
    uint64_t addr;
    uint32_t length;
    uint32_t padding;
};                                  /* vaddr's physical address should be continuos */
static bool CMDResourceAttachBacking(uint32_t resourceId, uint64_t vaddr, uint32_t len)
{
    struct VirtgpuResourceAttachBacking req = {
        .hdr.type = VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING,
        .resourceId = resourceId,
        .nrEntries = 1
    };
    struct VirtgpuMemEntry data = {
        .addr = VMM_TO_DMA_ADDR(vaddr),
        .length = len,
    };
    struct VirtgpuCtrlHdr resp = { 0 };

    return RequestDataResponse(&req, sizeof(req), &data, sizeof(data), &resp, sizeof(resp));
}

static void NormOpsRefresh(uintptr_t arg)
{
    (void)arg;
    RequestNoResponse(0, &g_virtGpu->transReq, sizeof(g_virtGpu->transReq), false);
    RequestNoResponse(0, &g_virtGpu->flushReq, sizeof(g_virtGpu->flushReq), true);
}

/* fit user-space page size mmap */
static inline size_t VirtgpuFbPageSize(void)
{
    return ALIGN(g_virtGpu->screen.width * g_virtGpu->screen.height * PIXEL_BYTES, PAGE_SIZE);
}

static void PopulateVirtQ(void)
{
    struct Virtq *q = NULL;
    int i, n;
    uint16_t qsz;

    for (n = 0; n < VIRTQ_NUM; n++) {
        if (n) {
            qsz = VIRTQ_CURSOR_QSZ;
        } else {
            qsz = VIRTQ_CONTROL_QSZ;
        }
        q = &g_virtGpu->dev.vq[n];

        for (i = 0; i < qsz; i += NORMAL_CMD_ENTRIES) {
            q->desc[i].flag = VIRTQ_DESC_F_NEXT;
            q->desc[i].next = i + 1;
            q->desc[i + 1].pAddr = VMM_TO_DMA_ADDR((VADDR_T)&g_virtGpu->resp);
            q->desc[i + 1].len = sizeof(g_virtGpu->resp);
            q->desc[i + 1].flag = VIRTQ_DESC_F_WRITE;
        }
        /* change usage to record our next writable index */
        q->last = 0;
    }

    g_virtGpu->transReq.hdr.type = VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D;
    g_virtGpu->transReq.r = g_virtGpu->screen;
    g_virtGpu->transReq.resourceId = RESOURCEID_FB;

    g_virtGpu->flushReq.hdr.type = VIRTIO_GPU_CMD_RESOURCE_FLUSH;
    g_virtGpu->flushReq.r = g_virtGpu->screen;
    g_virtGpu->flushReq.resourceId = RESOURCEID_FB;
}

static bool VirtgpuBeginNormDisplay(void)
{
    int32_t ret;

    if (!CMDTransferToHost(RESOURCEID_FB, &g_virtGpu->screen)) {
        return false;
    }
    if (!CMDResourceFlush()) {
        return false;
    }

    /* now we can fix queue entries to avoid redundant when do normal OPs */
    PopulateVirtQ();

    if ((ret = OsalTimerStartLoop(&g_virtGpu->timer)) != HDF_SUCCESS) {
        HDF_LOGE("[%s]start timer failed: %d\n", __func__, ret);
        return false;
    }
    return true;
}

/* unified DeInit for InitDev, HDF and fb */
static void VirtgpuDeInit(struct Virtgpu *gpu)
{
    if (gpu->timer.realTimer) {
        OsalTimerDelete(&gpu->timer);
    }
    if (gpu->fb) {
        LOS_PhysPagesFreeContiguous(gpu->fb, VirtgpuFbPageSize() / PAGE_SIZE);
    }
    LOS_DmaMemFree(gpu);
    g_virtGpu = NULL;
}

static struct Virtgpu *VirtgpuInitDev(void)
{
    struct Virtgpu *gpu = NULL;
    VADDR_T base;
    uint16_t qsz[VIRTQ_NUM];
    int32_t ret, len;

    /* NOTE: For simplicity, alloc all these data from physical continuous memory. */
    len = sizeof(struct Virtgpu) + VirtqSize(VIRTQ_CONTROL_QSZ) + VirtqSize(VIRTQ_CURSOR_QSZ);
    gpu = LOS_DmaMemAlloc(NULL, len, sizeof(void *), DMA_CACHE);
    if (gpu == NULL) {
        HDF_LOGE("[%s]alloc gpu memory failed", __func__);
        return NULL;
    }

    if (!VirtmmioDiscover(VIRTMMIO_DEVICE_ID_GPU, &gpu->dev)) {
        goto ERR_OUT;
    }

    VirtmmioInitBegin(&gpu->dev);

    if (!VirtmmioNegotiate(&gpu->dev, Feature0, Feature1, gpu)) {
        goto ERR_OUT1;
    }

    base = ALIGN((VADDR_T)gpu + sizeof(struct Virtgpu), VIRTQ_ALIGN_DESC);
    qsz[0] = VIRTQ_CONTROL_QSZ;
    qsz[1] = VIRTQ_CURSOR_QSZ;
    if (VirtmmioConfigQueue(&gpu->dev, base, qsz, VIRTQ_NUM) == 0) {
        goto ERR_OUT1;
    }

    /* framebuffer can be modified at any time, so we need a full screen refresh timer */
    ret = OsalTimerCreate(&gpu->timer, GPU_DFT_RATE, NormOpsRefresh, 0);
    if (ret != HDF_SUCCESS) {
        HDF_LOGE("[%s]create timer failed: %d", __func__, ret);
        goto ERR_OUT1;
    }

    for (int i = 0; i < VIRTQ_NUM; i++) {   /* hint device not using IRQ */
        gpu->dev.vq[i].avail->flag = VIRTQ_AVAIL_F_NO_INTERRUPT;
    }

    VritmmioInitEnd(&gpu->dev);             /* now virt queue can be used */
    return gpu;

ERR_OUT1:
    VirtmmioInitFailed(&gpu->dev);
ERR_OUT:
    VirtgpuDeInit(gpu);
    return NULL;
}

static int32_t HdfVirtgpuInit(struct HdfDeviceObject *device)
{
    int32_t ret;

    if (device == NULL) {
        HDF_LOGE("[%s]device is null", __func__);
        return HDF_ERR_INVALID_PARAM;
    }

    g_virtGpu = VirtgpuInitDev();
    if (g_virtGpu == NULL) {
        return HDF_FAILURE;
    }
    device->priv = g_virtGpu;

    /* frame buffer resource are initiated here, using virt queue mechanism */
    if ((ret = fb_register(0, 0)) != 0) {
        HDF_LOGE("[%s]framebuffer register failed: %d", __func__, ret);
        return HDF_FAILURE;
    }

    if (!VirtgpuBeginNormDisplay()) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

static void HdfVirtgpuRelease(struct HdfDeviceObject *deviceObject)
{
    if (deviceObject) {
        if (deviceObject->priv) {
            VirtgpuDeInit(deviceObject->priv);
        }
    }
}

struct HdfDriverEntry g_virtGpuEntry = {
    .moduleVersion = 1,
    .moduleName = "HDF_VIRTIO_GPU",
    .Init = HdfVirtgpuInit,
    .Release = HdfVirtgpuRelease,
};

HDF_INIT(g_virtGpuEntry);


/*
 * video/fb.h interface implementation
 */

static bool VirtgpuInitResourceHelper(uint32_t resourceId)
{
    uint64_t va;
    uint32_t len, w, h;

    if (!CMDResourceCreate2D(resourceId)) {
        return false;
    }

    if (resourceId == RESOURCEID_FB) {
        va = (uint64_t)g_virtGpu->fb;
        w = g_virtGpu->screen.width;
        h = g_virtGpu->screen.height;
    } else {
        HDF_LOGE("[%s]error resource ID: %u", __func__, resourceId);
        return false;
    }
    len = w * h * PIXEL_BYTES;
    if (!CMDResourceAttachBacking(resourceId, va, len)) {
        return false;
    }

    if (resourceId == RESOURCEID_FB) {
        struct VirtgpuRect r = { 0, 0, w, h };
        return CMDSetScanout(&r);
    }
    return true;
}

static bool VirtgpuInitResource(void)
{
    /* Framebuffer must be physical continuous. fb_register will zero the buffer */
    g_virtGpu->fb = LOS_PhysPagesAllocContiguous(VirtgpuFbPageSize() / PAGE_SIZE);
    if (g_virtGpu->fb == NULL) {
        HDF_LOGE("[%s]alloc framebuffer memory fail", __func__);
        return false;
    }
    if (!VirtgpuInitResourceHelper(RESOURCEID_FB)) {
        return false;
    }

    return true;
}

int up_fbinitialize(int display)
{
    if (display != 0) {
        return -1;
    }

    CMDGetDisplayInfo();
    if (g_virtGpu->edid) {
        CMDGetEdid();
    }

    if (!VirtgpuInitResource()) {
        return -1;
    }

    return 0;
}

static int FbGetVideoInfo(struct fb_vtable_s *vtable, struct fb_videoinfo_s *vinfo)
{
    (void)vtable;
    vinfo->fmt = FB_FMT_RGB32;  /* sRGB */
    vinfo->xres = g_virtGpu->screen.width;
    vinfo->yres = g_virtGpu->screen.height;
    vinfo->nplanes = 1;
    return 0;
}

#define BYTE_BITS   8
static int FbGetPlaneInfo(struct fb_vtable_s *vtable, int planeno, struct fb_planeinfo_s *pinfo)
{
    if (planeno != 0) {
        return -1;
    }
    (void)vtable;

    pinfo->fbmem = g_virtGpu->fb;
    pinfo->stride = g_virtGpu->screen.width * PIXEL_BYTES;
    pinfo->fblen = pinfo->stride * g_virtGpu->screen.height;
    pinfo->display = 0;
    pinfo->bpp = PIXEL_BYTES * BYTE_BITS;
    return 0;
}

#ifdef CONFIG_FB_OVERLAY
static int FbGetOverlayInfo(struct fb_vtable_s *v, int overlayno, struct fb_overlayinfo_s *info)
{
    (void)v;
    if (overlayno != 0) {
        return -1;
    }

    info->fbmem = g_virtGpu->fb;
    info->memphys = (void *)VMM_TO_DMA_ADDR((VADDR_T)g_virtGpu->fb);
    info->stride = g_virtGpu->screen.width * PIXEL_BYTES;
    info->fblen = info->stride * g_virtGpu->screen.height;
    info->overlay = 0;
    info->bpp = PIXEL_BYTES * BYTE_BITS;
    info->accl = 0;
    return 0;
}
#endif

/* expect windows manager deal with concurrent access */
static int FbOpen(struct fb_vtable_s *vtable)
{
    (void)vtable;
    return 0;
}

static int FbRelease(struct fb_vtable_s *vtable)
{
    (void)vtable;
    return 0;
}

static ssize_t FbMmap(struct fb_vtable_s *vtable, LosVmMapRegion *region)
{
    (void)vtable;
    int n;

    if ((region->range.size + (region->pgOff << PAGE_SHIFT)) > VirtgpuFbPageSize()) {
        HDF_LOGE("[%s]mmap size + pgOff exceed framebuffer size", __func__);
        return -1;
    }
    if (region->regionFlags & VM_MAP_REGION_FLAG_PERM_EXECUTE) {
        HDF_LOGE("[%s]cannot set execute flag", __func__);
        return -1;
    }

    region->regionFlags |= VM_MAP_REGION_FLAG_UNCACHED;
    n = LOS_ArchMmuMap(&region->space->archMmu, region->range.base,
                        VMM_TO_DMA_ADDR((VADDR_T)g_virtGpu->fb + (region->pgOff << PAGE_SHIFT)),
                        region->range.size >> PAGE_SHIFT, region->regionFlags);
    if (n != (region->range.size >> PAGE_SHIFT)) {
        HDF_LOGE("[%s]mmu map error: %d", __func__, n);
        return -1;
    }

    return 0;
}

/* used to happy video/fb.h configure */
static int FbDummy(struct fb_vtable_s *v, int *s)
{
    (void)v;
    (void)s;
    HDF_LOGE("[%s]unsupported method", __func__);
    return -1;
}

static struct fb_vtable_s g_virtGpuFbOps = {
    .getvideoinfo = FbGetVideoInfo,
    .getplaneinfo = FbGetPlaneInfo,
    .fb_open = FbOpen,
    .fb_release = FbRelease,
#ifdef CONFIG_FB_CMAP
    .getcmap = (int (*)(struct fb_vtable_s *, struct fb_cmap_s *))FbDummy,
    .putcmap = (int (*)(struct fb_vtable_s *, const struct fb_cmap_s *))FbDummy,
#endif
#ifdef CONFIG_FB_OVERLAY
    .getoverlayinfo = FbGetOverlayInfo,
    .settransp = (int (*)(struct fb_vtable_s *, const struct fb_overlayinfo_s *))FbDummy,
    .setchromakey = (int (*)(struct fb_vtable_s *, const struct fb_overlayinfo_s *))FbDummy,
    .setcolor = (int (*)(struct fb_vtable_s *, const struct fb_overlayinfo_s *))FbDummy,
    .setblank = (int (*)(struct fb_vtable_s *, const struct fb_overlayinfo_s *))FbDummy,
    .setarea = (int (*)(struct fb_vtable_s *, const struct fb_overlayinfo_s *))FbDummy,
# ifdef CONFIG_FB_OVERLAY_BLIT
    .blit = (int (*)(struct fb_vtable_s *, const struct fb_overlayblit_s *))FbDummy,
    .blend = (int (*)(struct fb_vtable_s *, const struct fb_overlayblend_s *))FbDummy,
# endif
    .fb_pan_display = (int (*)(struct fb_vtable_s *, struct fb_overlayinfo_s *))FbDummy,
#endif
    .fb_mmap = FbMmap
};

struct fb_vtable_s *up_fbgetvplane(int display, int vplane)
{
    if ((display != 0) || (vplane != 0)) {
        return NULL;
    }
    return &g_virtGpuFbOps;
}

void up_fbuninitialize(int display)
{
    if (display != 0) {
        return;
    }

    if (g_virtGpu) {
        VirtgpuDeInit(g_virtGpu);
    }
}

uint32_t VirtgpuGetXres(void)
{
    return g_virtGpu->screen.width;
}

uint32_t VirtgpuGetYres(void)
{
    return g_virtGpu->screen.height;
}