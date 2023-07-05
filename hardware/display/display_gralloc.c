/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "display_gralloc.h"
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <securec.h>
#include "buffer_handle.h"
#include "display_type.h"
#include "disp_common.h"
#include "hdf_log.h"

#define DEFAULT_READ_WRITE_PERMISSIONS   0666
#define MAX_MALLOC_SIZE                  0x10000000L
#define SHM_MAX_KEY                      10000
#define SHM_START_KEY                    1
#define INVALID_SHMID -1
#define BITS_PER_BYTE 8

#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define ALIGN_UP(x, a) ((((x) + ((a)-1)) / (a)) * (a))
#define HEIGHT_ALIGN 2U
#define WIDTH_ALIGN 8U
#define MAX_PLANES 3

typedef struct {
    BufferHandle hdl;
    int32_t shmid;
} PriBufferHandle;

typedef struct {
    uint32_t numPlanes;
    uint32_t radio[MAX_PLANES];
} PlaneLayoutInfo;

typedef struct {
    uint32_t format;
    uint32_t bitsPerPixel; // bits per pixel for first plane
    const PlaneLayoutInfo *planes;
} FormatInfo;

static const PlaneLayoutInfo g_yuv420SPLayout = {
    .numPlanes = 2,
    .radio = { 4, 2 },
};

static const PlaneLayoutInfo g_yuv420PLayout = {
    .numPlanes = 3,
    .radio = { 4, 1, 1 },
};

static const FormatInfo *GetFormatInfo(uint32_t format)
{
    static const FormatInfo fmtInfos[] = {
        {PIXEL_FMT_RGBX_8888,  32, NULL},  {PIXEL_FMT_RGBA_8888, 32,  NULL},
        {PIXEL_FMT_BGRX_8888,  32, NULL},  {PIXEL_FMT_BGRA_8888, 32,  NULL},
        {PIXEL_FMT_RGB_888,    24, NULL},  {PIXEL_FMT_BGR_565,   16,  NULL},
        {PIXEL_FMT_RGBA_5551,  16, NULL},  {PIXEL_FMT_RGB_565,   16,  NULL},
        {PIXEL_FMT_BGRX_4444,  16, NULL},  {PIXEL_FMT_BGRA_4444, 16,  NULL},
        {PIXEL_FMT_RGBA_4444,  16, NULL},  {PIXEL_FMT_RGBX_4444, 16,  NULL},
        {PIXEL_FMT_BGRX_5551,  16, NULL},  {PIXEL_FMT_BGRA_5551, 16,  NULL},
        {PIXEL_FMT_YCBCR_420_SP, 8, &g_yuv420SPLayout}, {PIXEL_FMT_YCRCB_420_SP, 8, &g_yuv420SPLayout},
        {PIXEL_FMT_YCBCR_420_P, 8, &g_yuv420PLayout}, {PIXEL_FMT_YCRCB_420_P, 8, &g_yuv420PLayout},
    };

    for (uint32_t i = 0; i < sizeof(fmtInfos) / sizeof(FormatInfo); i++) {
        if (fmtInfos[i].format == format) {
            return &fmtInfos[i];
        }
    }
    HDF_LOGE("the format can not support %d %d", format, PIXEL_FMT_RGBA_8888);
    return NULL;
}

static uint32_t AdjustStrideFromFormat(uint32_t format, uint32_t width)
{
    const FormatInfo *fmtInfo = GetFormatInfo(format);
    if ((fmtInfo != NULL) && (fmtInfo->planes != NULL)) {
        uint32_t sum = fmtInfo->planes->radio[0];
        for (uint32_t i = 1; (i < fmtInfo->planes->numPlanes) && (i < MAX_PLANES); i++) {
            sum += fmtInfo->planes->radio[i];
        }
        if (sum > 0) {
            width = DIV_ROUND_UP((width * sum), fmtInfo->planes->radio[0]);
        }
    }
    return width;
}

static int32_t InitBufferHandle(PriBufferHandle* buffer, const AllocInfo* info)
{
    int32_t size;
    int32_t stride;
    int32_t h = ALIGN_UP(info->height, HEIGHT_ALIGN);
    const FormatInfo *fmtInfo = GetFormatInfo(info->format);
    if (fmtInfo == NULL) {
        HDF_LOGE("can not get format information : %d", buffer->hdl.format);
        return DISPLAY_FAILURE;
    }

    stride = ALIGN_UP(AdjustStrideFromFormat(info->format, info->width), WIDTH_ALIGN) *
        fmtInfo->bitsPerPixel / BITS_PER_BYTE;
    size = h * stride;
    buffer->hdl.width = info->width;
    buffer->hdl.stride = stride;
    buffer->hdl.height = info->height;
    buffer->hdl.size = size;
    buffer->hdl.usage = info->usage;
    buffer->hdl.fd = -1;
    buffer->shmid = INVALID_SHMID;
    buffer->hdl.format = info->format;
    buffer->hdl.reserveInts = (sizeof(PriBufferHandle) - sizeof(BufferHandle) -
        buffer->hdl.reserveFds * sizeof(uint32_t)) / sizeof(uint32_t);
    return DISPLAY_SUCCESS;
}

static int32_t AllocShm(BufferHandle *buffer)
{
    static int32_t key = SHM_START_KEY;
    int32_t shmid;

    while ((shmid = shmget(key, buffer->size, IPC_CREAT | IPC_EXCL | DEFAULT_READ_WRITE_PERMISSIONS)) < 0) {
        if (errno != EEXIST) {
            HDF_LOGE("%s: fail to alloc the shared memory, errno = %d", __func__, errno);
            return DISPLAY_FAILURE;
        }
        key++;
        if (key >= SHM_MAX_KEY) {
            key = SHM_START_KEY;
        }
    }
    void *pBase = shmat(shmid, NULL, 0);
    if (pBase == ((void *)-1)) {
        HDF_LOGE("%s: Fail to attach the shared memory, errno = %d", __func__, errno);
        if (shmctl(shmid, IPC_RMID, 0) == -1) {
            HDF_LOGE("%s: Fail to free shmid, errno = %d", __func__, errno);
        }
        return DISPLAY_FAILURE;
    }
    buffer->virAddr = pBase;
    buffer->fd = key;
    ((PriBufferHandle*)buffer)->shmid = shmid;
    key++;
    (void)memset_s(pBase, buffer->size, 0x0, buffer->size);
    if (key >= SHM_MAX_KEY) {
        key = SHM_START_KEY;
    }
    return DISPLAY_SUCCESS;
}

typedef enum {
    MMZ_CACHE = 1,                  /* allocate mmz with cache attribute */
    MMZ_NOCACHE,                    /* allocate mmz with nocache attribute */
    MMZ_FREE,                       /* free mmz */
    MAP_CACHE,
    MAP_NOCACHE,
    UNMAP,
    FLUSH_CACHE,
    FLUSH_NOCACHE,
    INVALIDATE,
    MMZ_MAX
} MMZ_TYPE;

typedef struct {
    void *vaddr;
    uint64_t paddr;
    int32_t size;
} MmzMemory;

#define MMZ_IOC_MAGIC               'M'
#define MMZ_CACHE_TYPE              _IOR(MMZ_IOC_MAGIC, MMZ_CACHE, MmzMemory)
#define MMZ_NOCACHE_TYPE            _IOR(MMZ_IOC_MAGIC, MMZ_NOCACHE, MmzMemory)
#define MMZ_FREE_TYPE               _IOR(MMZ_IOC_MAGIC, MMZ_FREE, MmzMemory)
#define MMZ_MAP_CACHE_TYPE          _IOR(MMZ_IOC_MAGIC, MAP_CACHE, MmzMemory)
#define MMZ_MAP_NOCACHE_TYPE        _IOR(MMZ_IOC_MAGIC, MAP_NOCACHE, MmzMemory)
#define MMZ_UNMAP_TYPE              _IOR(MMZ_IOC_MAGIC, UNMAP, MmzMemory)
#define MMZ_FLUSH_CACHE_TYPE        _IOR(MMZ_IOC_MAGIC, FLUSH_CACHE, MmzMemory)
#define MMZ_FLUSH_NOCACHE_TYPE      _IOR(MMZ_IOC_MAGIC, FLUSH_NOCACHE, MmzMemory)
#define MMZ_INVALIDATE_TYPE         _IOR(MMZ_IOC_MAGIC, INVALIDATE, MmzMemory)

#define MMZ_NODE                    "/dev/mmz"

static int SendCmd(int cmd, unsigned long arg)
{
    int fd = open(MMZ_NODE, O_RDONLY);
    if (fd != -1) {
        int ret = ioctl(fd, cmd, arg);
        if (ret == -1) {
            printf("[Init] 1 [ERR] %d!\n", errno);
        }
        close(fd);
        return ret;
    }
    return fd;
}

static int32_t AllocMmz(BufferHandle *buffer)
{
    int32_t ret;

    MmzMemory mmz = {0};
    mmz.size = buffer->size;
    switch (buffer->usage) {
        case HBM_USE_MEM_MMZ_CACHE:
            printf("req size(%#x), ret:%d \n", buffer->size, SendCmd(MMZ_CACHE_TYPE, (uintptr_t)&mmz));
            printf("vaddr %#x, paddr: %#x\n", mmz.vaddr, mmz.paddr);
            ret = 0;
            break;
        case HBM_USE_MEM_MMZ:
            printf("req size(%#x), ret:%d \n", buffer->size, SendCmd(MMZ_NOCACHE_TYPE, (uintptr_t)&mmz));
            printf("vaddr %#x, paddr: %#x\n", mmz.vaddr, mmz.paddr);
            ret = 0;
            break;
        default:
            HDF_LOGE("%s: not support memory usage: 0x%" PRIx64 "", __func__, buffer->usage);
            return DISPLAY_NOT_SUPPORT;
    }
    if (ret != DISPLAY_SUCCESS) {
        HDF_LOGE("%s: mmzalloc failure, usage = 0x%" PRIx64 ", ret 0x%x", __func__,
            buffer->usage, ret);
        return DISPLAY_FAILURE;
    }
    (void)memset_s(mmz.vaddr, buffer->size, 0x0, buffer->size);
    buffer->phyAddr = mmz.paddr;
    buffer->virAddr = mmz.vaddr;
    return DISPLAY_SUCCESS;
}

static int32_t FreeMmz(uint64_t paddr, void* vaddr)
{
    MmzMemory mmz = {0};
    mmz.vaddr = vaddr;
    mmz.paddr = paddr;
    return SendCmd(MMZ_FREE_TYPE, (uintptr_t)&mmz);
}

static int32_t MmzFlushCache(BufferHandle *buffer)
{
    MmzMemory mmz = {0};
    mmz.paddr = buffer->phyAddr;
    mmz.size = buffer->size;
    mmz.vaddr = buffer->virAddr;
    return SendCmd(MMZ_FLUSH_CACHE_TYPE, (uintptr_t)&mmz);
}

static int32_t MmzInvalidateCache(BufferHandle *buffer)
{
    MmzMemory mmz = {0};
    mmz.paddr = buffer->phyAddr;
    mmz.size = buffer->size;
    mmz.vaddr = buffer->virAddr;
    return SendCmd(MMZ_INVALIDATE_TYPE, (uintptr_t)&mmz);
}

static int32_t AllocMem(const AllocInfo* info, BufferHandle **buffer)
{
    int32_t ret;
    DISPLAY_CHK_RETURN((buffer == NULL), DISPLAY_NULL_PTR, HDF_LOGE("%s: in buffer is null", __func__));
    DISPLAY_CHK_RETURN((info == NULL), DISPLAY_NULL_PTR, HDF_LOGE("%s: in info is null", __func__));
    PriBufferHandle* priBuffer = calloc(1, sizeof(PriBufferHandle));
    DISPLAY_CHK_RETURN((priBuffer == NULL), DISPLAY_NULL_PTR, HDF_LOGE("%s: can not calloc errno : %d",
        __func__, errno));
    ret = InitBufferHandle(priBuffer, info);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, HDF_LOGE("%s: can not init buffe handle",
        __func__); goto OUT);

    BufferHandle *bufferHdl = &priBuffer->hdl;
    DISPLAY_CHK_RETURN(((bufferHdl->size > MAX_MALLOC_SIZE) || (bufferHdl->size == 0)),
        DISPLAY_FAILURE, HDF_LOGE("%s: size is invalid %d ", __func__, bufferHdl->size); goto OUT);

    if (bufferHdl->usage == HBM_USE_MEM_SHARE) {
        ret = AllocShm(bufferHdl);
    } else if ((bufferHdl->usage == HBM_USE_MEM_DMA ) || (bufferHdl->usage == HBM_USE_MEM_MMZ )) {
        ret = AllocMmz(bufferHdl);
    } else {
        HDF_LOGE("%s: not support memory usage: 0x%" PRIx64 "", __func__, bufferHdl->usage);
        ret = DISPLAY_NOT_SUPPORT;
    }

OUT:
    if ((ret != DISPLAY_SUCCESS) && (bufferHdl != NULL)) {
        free(bufferHdl);
        bufferHdl = NULL;
    }
    *buffer = bufferHdl;
    return ret;
}

static void FreeShm(BufferHandle *buffer)
{
    CHECK_NULLPOINTER_RETURN(buffer->virAddr);
    if (shmdt(buffer->virAddr) == -1) {
        HDF_LOGE("%s: Fail to free shared memory, errno = %d", __func__, errno);
    }
    if (shmctl(((PriBufferHandle*)buffer)->shmid, IPC_RMID, 0) == -1) {
        HDF_LOGE("%s: Fail to free shmid, errno = %d", __func__, errno);
    }
}

static void FreeMem(BufferHandle *buffer)
{
    int ret;

    CHECK_NULLPOINTER_RETURN(buffer);
    if ((buffer->size > MAX_MALLOC_SIZE) || (buffer->size == 0)) {
        HDF_LOGE("%s: size is invalid, buffer->size = %d", __func__, buffer->size);
        return;
    }

    switch (buffer->usage) {
        case HBM_USE_MEM_MMZ_CACHE:
        case HBM_USE_MEM_MMZ:
            ret = FreeMmz(buffer->phyAddr, buffer->virAddr);
            if (ret != DISPLAY_SUCCESS) {
                HDF_LOGE("%s: HI_MPI_SYS_MmzFree failure, ret 0x%x", __func__, ret);
            }
            break;
        case HBM_USE_MEM_SHARE:
            FreeShm(buffer);
            break;
        default:
            HDF_LOGE("%s: not support memory usage: 0x%" PRIx64 "", __func__, buffer->usage);
    }
}

static void *MmapShm(BufferHandle *buffer)
{
    int32_t shmid;

    shmid = shmget(buffer->fd, buffer->size, IPC_EXCL | DEFAULT_READ_WRITE_PERMISSIONS);
    if (shmid < 0) {
        HDF_LOGE("%s: Fail to mmap the shared memory, errno = %d", __func__, errno);
        return NULL;
    }
    void *pBase = shmat(shmid, NULL, 0);
    if (pBase == ((void *)-1)) {
        HDF_LOGE("%s: Fail to attach the shared memory, errno = %d", __func__, errno);
        return NULL;
    }
    ((PriBufferHandle*)buffer)->shmid = shmid;
    HDF_LOGI("%s: Mmap shared memory succeed", __func__);
    return pBase;
}

static void *MmapMmzNoCache(uint64_t paddr, int32_t size)
{
    MmzMemory mmz = {0};
    mmz.paddr = paddr;
    mmz.size = size;
    SendCmd(MMZ_MAP_NOCACHE_TYPE, (uintptr_t)&mmz);
    return (void *)mmz.vaddr;
}

static int32_t UnmapMmz(BufferHandle *buffer)
{
    MmzMemory mmz = {0};
    mmz.paddr = buffer->phyAddr;
    mmz.size = buffer->size;
    mmz.vaddr = buffer->virAddr;
    return SendCmd(MMZ_UNMAP_TYPE, (uintptr_t)&mmz);
}

static void *MmapMmzCache(uint64_t paddr, int32_t size)
{
    MmzMemory mmz = {0};
    mmz.paddr = paddr;
    mmz.size = size;
    SendCmd(MMZ_MAP_CACHE_TYPE, (uintptr_t)&mmz);
    return (void *)mmz.vaddr;
}

static void *MmapCache(BufferHandle *buffer)
{
    CHECK_NULLPOINTER_RETURN_VALUE(buffer, NULL);
    if ((buffer->size > MAX_MALLOC_SIZE) || (buffer->size == 0)) {
        HDF_LOGE("%s: size is invalid, buffer->size = %d", __func__, buffer->size);
        return NULL;
    }
    if (buffer->usage == HBM_USE_MEM_MMZ_CACHE) {
        return MmapMmzCache(buffer->phyAddr, buffer->size);
    } else {
        HDF_LOGE("%s: buffer usage error, buffer->usage = 0x%" PRIx64 "", __func__, buffer->usage);
        return NULL;
    }
}

static void *Mmap(BufferHandle *buffer)
{
    CHECK_NULLPOINTER_RETURN_VALUE(buffer, NULL);
    if ((buffer->size > MAX_MALLOC_SIZE) || (buffer->size == 0)) {
        HDF_LOGE("%s: size is invalid, buffer->size = %d", __func__, buffer->size);
        return NULL;
    }

    switch (buffer->usage) {
        case HBM_USE_MEM_MMZ_CACHE:
            return MmapMmzCache(buffer->phyAddr, buffer->size);
        case HBM_USE_MEM_MMZ:
            return MmapMmzNoCache(buffer->phyAddr, buffer->size);
        case HBM_USE_MEM_SHARE:
            return MmapShm(buffer);
        default:
            HDF_LOGE("%s: not support memory usage: 0x%" PRIx64 "", __func__, buffer->usage);
            break;
    }
    return NULL;
}

static int32_t UnmapShm(BufferHandle *buffer)
{
    if (shmdt(buffer->virAddr) == -1) {
        HDF_LOGE("%s: Fail to unmap shared memory errno =  %d", __func__, errno);
        return DISPLAY_FAILURE;
    }
    int32_t shmid = ((PriBufferHandle*)buffer)->shmid;
    if ((shmid != INVALID_SHMID) && (shmctl(shmid, IPC_RMID, 0) == -1)) {
        HDF_LOGE("%s: Fail to free shmid, errno = %d", __func__, errno);
    }
    return DISPLAY_SUCCESS;
}

static int32_t Unmap(BufferHandle *buffer)
{
    int32_t ret;

    CHECK_NULLPOINTER_RETURN_VALUE(buffer, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(buffer->virAddr, DISPLAY_NULL_PTR);
    if ((buffer->size > MAX_MALLOC_SIZE) || (buffer->size == 0)) {
        HDF_LOGE("%s: size is invalid, buffer->size = %d", __func__, buffer->size);
        return DISPLAY_FAILURE;
    }
    switch (buffer->usage) {
        case HBM_USE_MEM_MMZ_CACHE:
        case HBM_USE_MEM_MMZ:
            ret = UnmapMmz(buffer);
            break;
        case  HBM_USE_MEM_SHARE:
            ret = UnmapShm(buffer);
            break;
        default:
            HDF_LOGE("%s: not support memory usage: 0x%" PRIx64 "", __func__, buffer->usage);
            ret = DISPLAY_FAILURE;
            break;
    }
    return ret;
}

static int32_t FlushCache(BufferHandle *buffer)
{
    int32_t ret;

    CHECK_NULLPOINTER_RETURN_VALUE(buffer, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(buffer->virAddr, DISPLAY_NULL_PTR);
    if ((buffer->size > MAX_MALLOC_SIZE) || (buffer->size == 0)) {
        HDF_LOGE("%s: size is invalid, buffer->size = %d", __func__, buffer->size);
        return DISPLAY_FAILURE;
    }
    if (buffer->usage == HBM_USE_MEM_MMZ_CACHE) {
        ret = MmzFlushCache(buffer);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("%s: MmzFlushCache failure, ret 0x%x", __func__, ret);
            return DISPLAY_FAILURE;
        }
    } else {
        HDF_LOGE("%s: buffer usage error, usage = 0x%" PRIx64"", __func__, buffer->usage);
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

static int32_t InvalidateCache(BufferHandle *buffer)
{
    int32_t ret;

    CHECK_NULLPOINTER_RETURN_VALUE(buffer, DISPLAY_NULL_PTR);
    CHECK_NULLPOINTER_RETURN_VALUE(buffer->virAddr, DISPLAY_NULL_PTR);
    if ((buffer->size > MAX_MALLOC_SIZE) || (buffer->size == 0)) {
        HDF_LOGE("%s: size is invalid, buffer->size = %d", __func__, buffer->size);
        return DISPLAY_FAILURE;
    }
    if (buffer->usage == HBM_USE_MEM_MMZ_CACHE) {
        ret = MmzInvalidateCache(buffer);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("%s: MmzFlushCache failure, ret 0x%x", __func__, ret);
            return DISPLAY_FAILURE;
        }
    } else {
        HDF_LOGE("%s: buffer usage error, usage = 0x%" PRIx64"", __func__, buffer->usage);
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

int32_t GrallocInitialize(GrallocFuncs **funcs)
{
    if (funcs == NULL) {
        HDF_LOGE("%s: funcs is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    GrallocFuncs *gFuncs = (GrallocFuncs *)malloc(sizeof(GrallocFuncs));
    if (gFuncs == NULL) {
        HDF_LOGE("%s: gFuncs is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    (void)memset_s(gFuncs, sizeof(GrallocFuncs), 0, sizeof(GrallocFuncs));
    gFuncs->AllocMem = AllocMem;
    gFuncs->FreeMem = FreeMem;
    gFuncs->Mmap = Mmap;
    gFuncs->MmapCache = MmapCache;
    gFuncs->Unmap = Unmap;
    gFuncs->FlushCache = FlushCache;
    gFuncs->FlushMCache = FlushCache;
    gFuncs->InvalidateCache = InvalidateCache;
    *funcs = gFuncs;
    HDF_LOGI("%s: gralloc initialize success", __func__);
    return DISPLAY_SUCCESS;
}

int32_t GrallocUninitialize(GrallocFuncs *funcs)
{
    if (funcs == NULL) {
        HDF_LOGE("%s: funcs is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    free(funcs);
    HDF_LOGI("%s: gralloc uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
