/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
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

#include "display_gralloc_gbm.h"
#include <cstdio>
#include <unistd.h>
#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <securec.h>
#include <linux/dma-buf.h>
#include "drm_fourcc.h"
#include "hisilicon_drm.h"
#include "hi_gbm.h"
#include "hdf_dlist.h"
#include "display_gralloc_private.h"
#include "display_log.h"
#include "v1_0/display_composer_type.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer::V1_0;
using namespace OHOS::HDI::Display::Buffer::V1_0;

const char *g_drmFileNode = "/dev/dri/card0";
static GrallocManager *g_grallocManager = nullptr;
static pthread_mutex_t g_lock;

using PixelFormatConvertTbl = struct {
    uint32_t drmFormat;
    PixelFormat pixFormat;
};

using ValueStrMap = struct {
    uint32_t value;
    const char *str;
};

static GrallocManager *GetGrallocManager(void)
{
    if (g_grallocManager == nullptr) {
        g_grallocManager = (GrallocManager *)malloc(sizeof(GrallocManager));
        errno_t eok = memset_s(g_grallocManager, sizeof(GrallocManager), 0, sizeof(GrallocManager));
        if (eok != EOK) {
            DISPLAY_LOGE("memset_s failed");
        }
        if (g_grallocManager == nullptr) {
            DISPLAY_LOGE("gralloc manager malloc failed");
        }
    }
    return g_grallocManager;
}

const char *GetPixelFmtStr(PixelFormat format)
{
    static const ValueStrMap pixelStrMaps[] = {
        {PIXEL_FMT_CLUT8, "PIXEL_FMT_CLUT8"}, {PIXEL_FMT_CLUT1, "PIXEL_FMT_CLUT1"},
        {PIXEL_FMT_CLUT4, "PIXEL_FMT_CLUT4"}, {PIXEL_FMT_RGB_565, "PIXEL_FMT_RGB_565"},
        {PIXEL_FMT_RGBA_5658, "IXEL_FMT_RGBA_5658"}, {PIXEL_FMT_RGBX_4444, "PIXEL_FMT_RGBX_4444"},
        {PIXEL_FMT_RGBA_4444, "PIXEL_FMT_RGBA_4444"}, {PIXEL_FMT_RGB_444, "PIXEL_FMT_RGB_444"},
        {PIXEL_FMT_RGBX_5551, "PIXEL_FMT_RGBX_5551"}, {PIXEL_FMT_RGBA_5551, "PIXEL_FMT_RGBA_5551"},
        {PIXEL_FMT_RGB_555, "PIXEL_FMT_RGB_555"}, {PIXEL_FMT_RGBX_8888, "PIXEL_FMT_RGBX_8888"},
        {PIXEL_FMT_RGBA_8888, "PIXEL_FMT_RGBA_8888"}, {PIXEL_FMT_RGB_888, "PIXEL_FMT_RGB_888"},
        {PIXEL_FMT_BGR_565, "PIXEL_FMT_BGR_565"}, {PIXEL_FMT_BGRX_4444, "PIXEL_FMT_BGRX_4444"},
        {PIXEL_FMT_BGRA_4444, "PIXEL_FMT_BGRA_4444"}, {PIXEL_FMT_BGRX_5551, "PIXEL_FMT_BGRX_5551"},
        {PIXEL_FMT_BGRA_5551, "PIXEL_FMT_BGRA_5551"}, {PIXEL_FMT_BGRX_8888, "PIXEL_FMT_BGRX_8888"},
        {PIXEL_FMT_BGRA_8888, "PIXEL_FMT_BGRA_8888"}, {PIXEL_FMT_YUV_422_I, "PIXEL_FMT_YUV_422_I"},
        {PIXEL_FMT_YUV_422_I, "PIXEL_FMT_YUV_422_I"}, {PIXEL_FMT_YCBCR_422_SP, "PIXEL_FMT_YCBCR_422_SP"},
        {PIXEL_FMT_YCRCB_422_SP, "PIXEL_FMT_YCRCB_422_SP"}, {PIXEL_FMT_YCBCR_420_SP, "PIXEL_FMT_YCBCR_420_SP"},
        {PIXEL_FMT_YCRCB_420_SP, "PIXEL_FMT_YCRCB_420_SP"}, {PIXEL_FMT_YCBCR_422_P, "PIXEL_FMT_YCBCR_422_P"},
        {PIXEL_FMT_YCRCB_422_P, "PIXEL_FMT_YCRCB_422_P"}, {PIXEL_FMT_YCBCR_420_P, "PIXEL_FMT_YCBCR_420_P"},
        {PIXEL_FMT_YCRCB_420_P, "PIXEL_FMT_YCRCB_420_P"}, {PIXEL_FMT_YUYV_422_PKG, "PIXEL_FMT_YUYV_422_PKG"},
        {PIXEL_FMT_UYVY_422_PKG, "PIXEL_FMT_UYVY_422_PKG"}, {PIXEL_FMT_YVYU_422_PKG, "PIXEL_FMT_YVYU_422_PKG"},
        {PIXEL_FMT_VYUY_422_PKG, "PIXEL_FMT_VYUY_422_PKG"}, {PIXEL_FMT_BUTT, "PIXEL_FMT_BUTT"},
    };
    static const char *unknown = "unknown format";
    for (uint32_t i = 0; i < sizeof(pixelStrMaps) / sizeof(pixelStrMaps[0]); i++) {
        if (pixelStrMaps[i].value == format) {
            return pixelStrMaps[i].str;
        }
    }
    DISPLAY_LOGE("GetPixelFmtStr unknown format %{public}d", format);
    return unknown;
}

const char *GetDrmFmtStr(uint32_t format)
{
    static const ValueStrMap formatStrMaps[] = {
        {DRM_FORMAT_C8, "DRM_FORMAT_C8" }, {DRM_FORMAT_R8, "DRM_FORMAT_R8" },
        {DRM_FORMAT_R16, "DRM_FORMAT_R16"}, {DRM_FORMAT_RG88, "DRM_FORMAT_RG88"},
        {DRM_FORMAT_GR88, "DRM_FORMAT_GR88"}, {DRM_FORMAT_RG1616, "DRM_FORMAT_RG1616"},
        {DRM_FORMAT_GR1616, "DRM_FORMAT_GR1616"}, {DRM_FORMAT_RGB332, "DRM_FORMAT_RGB332"},
        {DRM_FORMAT_BGR233, "DRM_FORMAT_BGR233"}, {DRM_FORMAT_XRGB4444, "DRM_FORMAT_XRGB4444"},
        {DRM_FORMAT_XBGR4444, "DRM_FORMAT_XBGR4444"}, {DRM_FORMAT_RGBX4444, "DRM_FORMAT_RGBX4444"},
        {DRM_FORMAT_BGRX4444, "DRM_FORMAT_BGRX4444"}, {DRM_FORMAT_ARGB4444, "DRM_FORMAT_ARGB4444"},
        {DRM_FORMAT_ABGR4444, "DRM_FORMAT_ABGR4444"}, {DRM_FORMAT_RGBA4444, "DRM_FORMAT_RGBA4444"},
        {DRM_FORMAT_BGRA4444, "DRM_FORMAT_BGRA4444"}, {DRM_FORMAT_XRGB1555, "DRM_FORMAT_XRGB1555"},
        {DRM_FORMAT_XBGR1555, "DRM_FORMAT_XBGR1555"}, {DRM_FORMAT_RGBX5551, "DRM_FORMAT_RGBX5551"},
        {DRM_FORMAT_BGRX5551, "DRM_FORMAT_BGRX5551"}, {DRM_FORMAT_ARGB1555, "DRM_FORMAT_ARGB1555"},
        {DRM_FORMAT_ABGR1555, "DRM_FORMAT_ABGR1555"}, {DRM_FORMAT_RGBA5551, "DRM_FORMAT_RGBA5551"},
        {DRM_FORMAT_BGRA5551, "DRM_FORMAT_BGRA5551"}, {DRM_FORMAT_RGB565, "DRM_FORMAT_RGB565"},
        {DRM_FORMAT_BGR565, "DRM_FORMAT_BGR565"}, {DRM_FORMAT_RGB888, "DRM_FORMAT_RGB888"},
        {DRM_FORMAT_BGR888, "DRM_FORMAT_BGR888"}, {DRM_FORMAT_XRGB8888, "DRM_FORMAT_XRGB8888"},
        {DRM_FORMAT_XBGR8888, "DRM_FORMAT_XBGR8888"}, {DRM_FORMAT_RGBX8888, "DRM_FORMAT_RGBX8888"},
        {DRM_FORMAT_BGRX8888, "DRM_FORMAT_BGRX8888"}, {DRM_FORMAT_ARGB8888, "DRM_FORMAT_ARGB8888"},
        {DRM_FORMAT_ABGR8888, "DRM_FORMAT_ABGR8888"}, {DRM_FORMAT_RGBA8888, "DRM_FORMAT_RGBA8888"},
        {DRM_FORMAT_BGRA8888, "DRM_FORMAT_BGRA8888"}, {DRM_FORMAT_XRGB2101010, "DRM_FORMAT_XRGB2101010"},
        {DRM_FORMAT_BGRX1010102, "DRM_FORMAT_BGRX1010102"}, {DRM_FORMAT_ARGB2101010, "DRM_FORMAT_ARGB2101010"},
        {DRM_FORMAT_ABGR2101010, "DRM_FORMAT_ABGR2101010"}, {DRM_FORMAT_RGBA1010102, "DRM_FORMAT_RGBA1010102"},
        {DRM_FORMAT_YVYU, "DRM_FORMAT_YVYU"}, {DRM_FORMAT_UYVY, "DRM_FORMAT_UYVY"},
        {DRM_FORMAT_VYUY, "DRM_FORMAT_VYUY"}, {DRM_FORMAT_AYUV, "DRM_FORMAT_AYUV"},
        {DRM_FORMAT_NV12, "DRM_FORMAT_NV12"}, {DRM_FORMAT_NV21, "DRM_FORMAT_NV21"},
        {DRM_FORMAT_NV16, "DRM_FORMAT_NV16"}, {DRM_FORMAT_NV61, "DRM_FORMAT_NV61"},
        {DRM_FORMAT_NV24, "DRM_FORMAT_NV24"}, {DRM_FORMAT_NV42, "DRM_FORMAT_NV42"},
        {DRM_FORMAT_YUV410, "DRM_FORMAT_YUV410"}, {DRM_FORMAT_YVU410, "DRM_FORMAT_YVU410"},
        {DRM_FORMAT_YUV411, "DRM_FORMAT_YUV411"}, {DRM_FORMAT_YVU411, "DRM_FORMAT_YVU411"},
        {DRM_FORMAT_YUV420, "DRM_FORMAT_YUV420"}, {DRM_FORMAT_YVU420, "DRM_FORMAT_YVU420"},
        {DRM_FORMAT_YUV422, "DRM_FORMAT_YUV422"}, {DRM_FORMAT_YVU422, "DRM_FORMAT_YVU422"},
        {DRM_FORMAT_YUV444, "DRM_FORMAT_YUV444"}, {DRM_FORMAT_YVU444, "DRM_FORMAT_YVU444"},
    };

    static const char *unknown = "unknown drm format";
    for (uint32_t i = 0; i < sizeof(formatStrMaps) / sizeof(formatStrMaps[0]); i++) {
        if (formatStrMaps[i].value == format) {
            return formatStrMaps[i].str;
        }
    }
    DISPLAY_LOGE("GetDrmFmtStr unknown format %{public}d", format);
    return unknown;
}

static uint32_t ConvertFormatToDrm(PixelFormat fmtIn)
{
    static const PixelFormatConvertTbl convertTable[] = {
        {DRM_FORMAT_XRGB8888, PIXEL_FMT_RGBX_8888}, {DRM_FORMAT_XRGB8888, PIXEL_FMT_RGBA_8888},
        {DRM_FORMAT_RGB888, PIXEL_FMT_RGB_888}, {DRM_FORMAT_RGB565, PIXEL_FMT_BGR_565},
        {DRM_FORMAT_BGRX4444, PIXEL_FMT_BGRX_4444}, {DRM_FORMAT_BGRA4444, PIXEL_FMT_BGRA_4444},
        {DRM_FORMAT_RGBA4444, PIXEL_FMT_RGBA_4444}, {DRM_FORMAT_RGBX4444, PIXEL_FMT_RGBX_4444},
        {DRM_FORMAT_BGRX5551, PIXEL_FMT_BGRX_5551}, {DRM_FORMAT_BGRA5551, PIXEL_FMT_BGRA_5551},
        {DRM_FORMAT_XRGB8888, PIXEL_FMT_BGRX_8888}, {DRM_FORMAT_XRGB8888, PIXEL_FMT_BGRA_8888},
        {DRM_FORMAT_NV12, PIXEL_FMT_YCBCR_420_SP}, {DRM_FORMAT_NV21, PIXEL_FMT_YCRCB_420_SP},
        {DRM_FORMAT_YUV420, PIXEL_FMT_YCBCR_420_P}, {DRM_FORMAT_YVU420, PIXEL_FMT_YCRCB_420_P},
        {DRM_FORMAT_NV16, PIXEL_FMT_YCBCR_422_SP}, {DRM_FORMAT_NV61, PIXEL_FMT_YCRCB_422_SP},
        {DRM_FORMAT_YUV422, PIXEL_FMT_YCBCR_422_P}, {DRM_FORMAT_YVU422, PIXEL_FMT_YCRCB_422_P},
    };
    uint32_t fmtOut = 0;
    for (uint32_t i = 0; i < sizeof(convertTable) / sizeof(convertTable[0]); i++) {
        if (convertTable[i].pixFormat == fmtIn) {
            fmtOut = convertTable[i].drmFormat;
        }
    }
    DISPLAY_LOGD("fmtIn %{public}d : %{public}s, outFmt %{public}d : %{public}s", fmtIn,
        GetPixelFmtStr(fmtIn), fmtOut, GetDrmFmtStr(fmtOut));
    return fmtOut;
}

static uint64_t ConvertUsageToGbm(uint64_t inUsage)
{
    uint64_t outUsage = GBM_BO_USE_TEXTURING;
    if (inUsage & HBM_USE_CPU_READ) {
        outUsage |= GBM_BO_USE_SW_READ_OFTEN;
    }
    if (inUsage & HBM_USE_CPU_WRITE) {
        outUsage |= GBM_BO_USE_SW_WRITE_OFTEN;
    }
    DISPLAY_LOGD("outUsage 0x%{public}" PRIx64 "", outUsage);
    return outUsage;
}

static int32_t InitGbmDevice(const char *drmFile, GrallocManager *grallocManager)
{
    DISPLAY_LOGD();
    if (grallocManager->gbmDevice == nullptr) {
	    char path[PATH_MAX] = {0};
        if (realpath(drmFile, path) == nullptr) {
            DISPLAY_LOGE(" drm File : %{public}s is not a realpath, errno: %{public}s", drmFile, strerror(errno));
            return HDF_ERR_INVALID_PARAM;
        }
        int drmFd = open(path, O_RDWR);
        if (drmFd < 0) {
            DISPLAY_LOGE("drm file:%{public}s open failed %{public}s", drmFile, strerror(errno));
            return HDF_ERR_BAD_FD;
        }
        drmDropMaster(drmFd);
        struct gbm_device *gbmDevice = hdi_gbm_create_device(drmFd);
        grallocManager->drmFd = drmFd;
        if (gbmDevice == nullptr) {
            close(drmFd);
            grallocManager->drmFd = -1;
            DISPLAY_LOGE("gbm device create failed");
            return HDF_FAILURE;
        }
        grallocManager->gbmDevice = gbmDevice;
        grallocManager->drmFd = drmFd;
        DListHeadInit(&grallocManager->gbmBoHead);
    }
    return HDF_SUCCESS;
}

static void DeInitGbmDevice(GrallocManager *grallocManager)
{
    DISPLAY_LOGD();
    hdi_gbm_device_destroy(grallocManager->gbmDevice);
    if (grallocManager->drmFd > 0) {
        close(grallocManager->drmFd);
        grallocManager->drmFd = -1;
    }
    grallocManager->gbmDevice = nullptr;
}

static int32_t DmaBufferSync(const BufferHandle *handle, bool start)
{
    DISPLAY_LOGD();
    struct dma_buf_sync syncPrm;
    errno_t eok = memset_s(&syncPrm, sizeof(syncPrm), 0, sizeof(syncPrm));
    DISPLAY_CHK_RETURN((eok != EOK), HDF_ERR_INVALID_PARAM, DISPLAY_LOGE("dma buffer sync memset_s failed"));

    if (handle->usage & HBM_USE_CPU_WRITE) {
        syncPrm.flags |= DMA_BUF_SYNC_WRITE;
    }

    if (handle->usage & HBM_USE_CPU_READ) {
        syncPrm.flags |= DMA_BUF_SYNC_READ;
    }

    if (start) {
        syncPrm.flags |= DMA_BUF_SYNC_START;
    } else {
        syncPrm.flags |= DMA_BUF_SYNC_END;
    }
    int retry = 6;
    int ret;
    do {
        ret = ioctl(handle->fd, DMA_BUF_IOCTL_SYNC, &syncPrm);
    } while ((retry--) && (ret != -EAGAIN) && (ret != -EINTR));

    if (ret < 0) {
        DISPLAY_LOGE("sync failed");
        return HDF_ERR_DEVICE_BUSY;
    }
    return HDF_SUCCESS;
}

static void InitBufferHandle(struct gbm_bo *bo, int fd, const AllocInfo *info, PriBufferHandle *buffer)
{
    BufferHandle *bufferHandle = &(buffer->hdl);
    bufferHandle->fd = fd;
    bufferHandle->reserveFds = 0;
    bufferHandle->reserveInts = 0;
    bufferHandle->stride = hdi_gbm_bo_get_stride(bo);
    bufferHandle->width = hdi_gbm_bo_get_width(bo);
    bufferHandle->height = hdi_gbm_bo_get_height(bo);
    bufferHandle->usage = info->usage;
    bufferHandle->format = info->format;
    bufferHandle->virAddr = nullptr;
    bufferHandle->size = hdi_gbm_bo_get_size(bo);
}

int32_t GbmAllocMem(const AllocInfo *info, BufferHandle **buffer)
{
    DISPLAY_CHK_RETURN((info == nullptr), HDF_FAILURE, DISPLAY_LOGE("info is null"));
    DISPLAY_CHK_RETURN((buffer == nullptr), HDF_FAILURE, DISPLAY_LOGE("buffer is null"));
    PriBufferHandle *priBuffer = nullptr;
    uint32_t drmFmt = ConvertFormatToDrm(static_cast<PixelFormat>(info->format));
    DISPLAY_CHK_RETURN((drmFmt == INVALID_PIXEL_FMT), HDF_ERR_NOT_SUPPORT,
        DISPLAY_LOGE("format %{public}d can not support", info->format));
    DISPLAY_LOGD("requeset width %{public}d, heigt %{public}d, format %{public}d",
        info->width, info->height, drmFmt);

        GRALLOC_LOCK();
    GrallocManager *grallocManager = GetGrallocManager();
    DISPLAY_CHK_RETURN((grallocManager == nullptr), HDF_ERR_INVALID_PARAM, DISPLAY_LOGE("gralloc manager failed");
        GRALLOC_UNLOCK());
    struct gbm_bo *bo =
        hdi_gbm_bo_create(grallocManager->gbmDevice, info->width, info->height, drmFmt, ConvertUsageToGbm(info->usage));
    DISPLAY_CHK_RETURN((bo == nullptr), HDF_DEV_ERR_NO_MEMORY, DISPLAY_LOGE("gbm create bo failed"); \
        GRALLOC_UNLOCK());

    int fd = hdi_gbm_bo_get_fd(bo);
    DISPLAY_CHK_RETURN((fd < 0), HDF_ERR_BAD_FD, DISPLAY_LOGE("gbm can not get fd"); \
        hdi_gbm_bo_destroy(bo); \
        GRALLOC_UNLOCK());

    errno_t eok = EOK;
    priBuffer = (PriBufferHandle *)malloc(sizeof(PriBufferHandle));
    if (priBuffer == nullptr) {
        DISPLAY_LOGE("bufferhandle malloc failed");
        goto error;
    }

    eok = memset_s(priBuffer, sizeof(PriBufferHandle), 0, sizeof(PriBufferHandle));
    if (eok != EOK) {
        DISPLAY_LOGE("memset_s failed");
        goto error;
    }
    DISPLAY_CHK_RETURN((eok != EOK), DISPLAY_PARAM_ERR, DISPLAY_LOGE("memset_s failed"); \
        goto error);

    InitBufferHandle(bo, fd, info, priBuffer);
    *buffer = &priBuffer->hdl;
    hdi_gbm_bo_destroy(bo);
    GRALLOC_UNLOCK();
    return HDF_SUCCESS;
error:
    close(fd);
    hdi_gbm_bo_destroy(bo);
    if (priBuffer != nullptr) {
        free(priBuffer);
    }
    GRALLOC_UNLOCK();
    return HDF_FAILURE;
}

static void CloseBufferHandle(BufferHandle *handle)
{
    DISPLAY_CHK_RETURN_NOT_VALUE((handle == nullptr), DISPLAY_LOGE("buffer is null"));
    if (handle->fd >= 0) {
        close(handle->fd);
        handle->fd = -1;
    }
    const uint32_t reserveFds = handle->reserveFds;
    for (uint32_t i = 0; i < reserveFds; i++) {
        if (handle->reserve[i] >= 0) {
            close(handle->reserve[i]);
            handle->reserve[i] = -1;
        }
    }
}

void GbmFreeMem(BufferHandle *buffer)
{
    DISPLAY_LOGD();
    DISPLAY_CHK_RETURN_NOT_VALUE((buffer == nullptr), DISPLAY_LOGE("buffer is null"));
    if ((buffer->virAddr != nullptr) && (GbmUnmap(buffer) != HDF_SUCCESS)) {
        DISPLAY_LOGE("freeMem unmap buffer failed");
    }
    CloseBufferHandle(buffer);
    free(buffer);
}

void *GbmMmap(BufferHandle *buffer)
{
    void *virAddr = nullptr;
    DISPLAY_LOGD();
    if (buffer == nullptr) {
        DISPLAY_LOGE("gbmmap the buffer handle is nullptr");
        return nullptr;
    }
    if (buffer->virAddr != nullptr) {
        DISPLAY_LOGD("the buffer has virtual addr");
        return buffer->virAddr;
    }
    virAddr = mmap(nullptr, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, buffer->fd, 0);
    if (virAddr == reinterpret_cast<void *>(MAP_FAILED)) {
        DISPLAY_LOGE("mmap failed errno %{public}s, fd : %{public}d", strerror(errno), buffer->fd);
    }
    buffer->virAddr = virAddr;
    return virAddr;
}

int32_t GbmUnmap(BufferHandle *buffer)
{
    DISPLAY_LOGD();
    if (buffer == nullptr) {
        DISPLAY_LOGE("gbmumap the buffer handle is null");
        return HDF_FAILURE;
    }

    if (buffer->virAddr == nullptr) {
        DISPLAY_LOGE("virAddr is nullptr , has not map the buffer");
        return HDF_ERR_INVALID_PARAM;
    }
    int ret = munmap(buffer->virAddr, buffer->size);
    if (ret != 0) {
        DISPLAY_LOGE("munmap failed err: %{public}s", strerror(errno));
        return HDF_FAILURE;
    }
    buffer->virAddr = nullptr;
    return HDF_SUCCESS;
}

int32_t GbmInvalidateCache(BufferHandle *buffer)
{
    DISPLAY_LOGD();
    return DmaBufferSync(buffer, true);
}

int32_t GbmFlushCache(BufferHandle *buffer)
{
    DISPLAY_LOGD();
    return DmaBufferSync(buffer, false);
}

int32_t GbmGrallocUninitialize(void)
{
    DISPLAY_LOGD();
    GRALLOC_LOCK();
    GrallocManager *grallocManager = GetGrallocManager();
    DISPLAY_CHK_RETURN((grallocManager == nullptr), HDF_ERR_INVALID_PARAM, DISPLAY_LOGE("gralloc manager failed"); \
        GRALLOC_UNLOCK());
    grallocManager->referCount--;
    if (grallocManager->referCount < 0) {
        DeInitGbmDevice(grallocManager);
        free(g_grallocManager);
        g_grallocManager = nullptr;
    }
    GRALLOC_UNLOCK();
    return HDF_SUCCESS;
}

int32_t GbmGrallocInitialize(void)
{
    DISPLAY_LOGD();
    GRALLOC_LOCK();
    GrallocManager *grallocManager = GetGrallocManager();
    DISPLAY_CHK_RETURN((grallocManager == nullptr), HDF_ERR_INVALID_PARAM, DISPLAY_LOGE("gralloc manager failed"); \
        GRALLOC_UNLOCK());
    int ret = InitGbmDevice(g_drmFileNode, grallocManager);
    DISPLAY_CHK_RETURN((ret != HDF_SUCCESS), ret, DISPLAY_LOGE("gralloc manager failed"); \
        GRALLOC_UNLOCK());
    grallocManager->referCount++;
    GRALLOC_UNLOCK();
    return HDF_SUCCESS;
}
} // namespace DISPLAY
} // namespace HDI
} // namespace OHOS
