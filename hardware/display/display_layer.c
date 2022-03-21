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

#include "display_layer.h"
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <securec.h>
#include "hdf_log.h"
#include "display_type.h"

#define DEV_ID             0
#define LAYER_ID           0
#define FB_PATH            "/dev/fb0"
#define DISP_WIDTH         960
#define DISP_HEIGHT        480
#define BITS_PER_PIXEL     32
#define BITS_TO_BYTE       8

struct LayerPrivate {
    int32_t  fd;
    uint32_t width;
    uint32_t height;
    int32_t  pitch;
    void     *fbAddr;
    uint32_t fbSize;
    void     *layerAddr;
    PixelFormat pixFmt;
};

static struct LayerPrivate *GetLayerInstance(void)
{
    static struct LayerPrivate layerPriv = {
        .fd = -1,
        .width = DISP_WIDTH,
        .height = DISP_HEIGHT,
        .pixFmt = PIXEL_FMT_RGBA_8888,
    };
    return &layerPriv;
}

static int32_t InitDisplay(uint32_t devId)
{
    if (devId != DEV_ID) {
        HDF_LOGE("%s: devId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

static int32_t DeinitDisplay(uint32_t devId)
{
    if (devId != DEV_ID) {
        HDF_LOGE("%s: devId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

static void SetBackground(void)
{
    struct LayerPrivate *priv = GetLayerInstance();
    uint32_t i;
    uint32_t j;
    uint32_t *framebuffer = (uint32_t *)priv->fbAddr;
    for (j = 0; j < priv->height; j++) {
        for (i = 0; i < priv->width; i++) {
            framebuffer[i + j * priv->width] = 0xFF; // Blue background
        }
    }
}

static int32_t CreateLayer(uint32_t devId, const LayerInfo *layerInfo, uint32_t *layerId)
{
    if (layerInfo == NULL || layerId == NULL) {
        HDF_LOGE("%s: pointer is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    if (devId != DEV_ID) {
        HDF_LOGE("%s: devId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    struct LayerPrivate *priv = GetLayerInstance();
    priv->fd = open(FB_PATH, O_RDWR, 0);
    if (priv->fd < 0) {
        HDF_LOGE("%s: open fb dev failed", __func__);
        return DISPLAY_FD_ERR;
    }
    priv->pitch = layerInfo->width * BITS_PER_PIXEL / BITS_TO_BYTE;
    priv->fbSize = ((priv->pitch * priv->height) + 0xfff) & (~0xfff);
    priv->fbAddr = (void *)mmap(NULL, priv->fbSize, PROT_READ | PROT_WRITE, MAP_SHARED, priv->fd, 0);
    if (priv->fbAddr == NULL) {
        HDF_LOGE("%s: mmap fb address failure, errno: %d", __func__, errno);
        close(priv->fd);
        priv->fd = -1;
        priv->pitch = 0;
        priv->fbSize = 0;
        return DISPLAY_FAILURE;
    }
    SetBackground();
    *layerId = LAYER_ID;
    HDF_LOGI("%s: open layer success", __func__);
    return DISPLAY_SUCCESS;
}

static int32_t CloseLayer(uint32_t devId, uint32_t layerId)
{
    if (devId != DEV_ID) {
        HDF_LOGE("%s: devId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    if (layerId != LAYER_ID) {
        HDF_LOGE("%s: layerId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    struct LayerPrivate *priv = GetLayerInstance();
    if (priv->fd >= 0) {
        close(priv->fd);
    }
    if (priv->layerAddr != NULL) {
        free(priv->layerAddr);
        priv->layerAddr = NULL;
    }
    if (priv->fbAddr != NULL) {
        munmap(priv->fbAddr, priv->fbSize);
    }
    priv->fd = -1;
    return DISPLAY_SUCCESS;
}

static int32_t GetDisplayInfo(uint32_t devId, DisplayInfo *dispInfo)
{
    if (dispInfo == NULL) {
        HDF_LOGE("%s: dispInfo is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    if (devId != DEV_ID) {
        HDF_LOGE("%s: devId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    struct LayerPrivate *priv = GetLayerInstance();
    dispInfo->width = priv->width;
    dispInfo->height = priv->height;
    dispInfo->rotAngle = ROTATE_NONE;
    HDF_LOGD("%s: width = %u, height = %u, rotAngle = %u", __func__, dispInfo->width,
        dispInfo->height, dispInfo->rotAngle);
    return DISPLAY_SUCCESS;
}

static int32_t Flush(uint32_t devId, uint32_t layerId, LayerBuffer *buffer)
{
    int32_t ret;
    if (devId != DEV_ID) {
        HDF_LOGE("%s: devId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    if (layerId != LAYER_ID) {
        HDF_LOGE("%s: layerId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    if (buffer == NULL) {
        HDF_LOGE("%s: buffer is null", __func__);
        return DISPLAY_FAILURE;
    }

    struct LayerPrivate *priv = GetLayerInstance();
    ret = memcpy_s(priv->fbAddr, priv->fbSize, buffer->data.virAddr, priv->fbSize);
    if (ret != EOK) {
        HDF_LOGE("%s: memcpy_s fail, ret %d", __func__, ret);
        return ret;
    }
    return DISPLAY_SUCCESS;
}

static int32_t GetLayerBuffer(uint32_t devId, uint32_t layerId, LayerBuffer *buffer)
{
    if (buffer == NULL) {
        HDF_LOGE("%s: buffer is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    if (devId != DEV_ID) {
        HDF_LOGE("%s: devId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    if (layerId != LAYER_ID) {
        HDF_LOGE("%s: layerId invalid", __func__);
        return DISPLAY_FAILURE;
    }
    struct LayerPrivate *priv = GetLayerInstance();
    if (priv->fd < 0) {
        HDF_LOGE("%s: fd invalid", __func__);
        return DISPLAY_FAILURE;
    }
    buffer->fenceId = 0;
    buffer->width = priv->width;
    buffer->height = priv->height;
    buffer->pixFormat = priv->pixFmt;
    buffer->pitch = priv->pitch;
    buffer->data.virAddr = malloc(priv->fbSize);
    if (buffer->data.virAddr == NULL) {
        HDF_LOGE("%s: malloc failure", __func__);
        return DISPLAY_FAILURE;
    }
    priv->layerAddr = buffer->data.virAddr;
    (void)memset_s(buffer->data.virAddr, priv->fbSize, 0x00, priv->fbSize);
    HDF_LOGD("%s: fenceId = %d, width = %d, height = %d, pixFormat = %d, pitch = %d", __func__, buffer->fenceId,
        buffer->width, buffer->height, buffer->pixFormat, buffer->pitch);
    return DISPLAY_SUCCESS;
}

int32_t LayerInitialize(LayerFuncs **funcs)
{
    if (funcs == NULL) {
        HDF_LOGE("%s: funcs is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    LayerFuncs *lFuncs = (LayerFuncs *)malloc(sizeof(LayerFuncs));
    if (lFuncs == NULL) {
        HDF_LOGE("%s: lFuncs is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    (void)memset_s(lFuncs, sizeof(LayerFuncs), 0, sizeof(LayerFuncs));
    lFuncs->InitDisplay = InitDisplay;
    lFuncs->DeinitDisplay = DeinitDisplay;
    lFuncs->GetDisplayInfo = GetDisplayInfo;
    lFuncs->CreateLayer = CreateLayer;
    lFuncs->CloseLayer = CloseLayer;
    lFuncs->Flush = Flush;
    lFuncs->GetLayerBuffer = GetLayerBuffer;
    *funcs = lFuncs;
    HDF_LOGI("%s: success", __func__);
    return DISPLAY_SUCCESS;
}

int32_t LayerUninitialize(LayerFuncs *funcs)
{
    if (funcs == NULL) {
        HDF_LOGE("%s: funcs is null", __func__);
        return DISPLAY_NULL_PTR;
    }
    free(funcs);
    HDF_LOGI("%s: layer uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
