/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "display_gfx.h"
#include "display_gralloc.h"
#include "display_layer.h"
#include "display_type.h"
#include "hdf_log.h"
#include "test_demo.h"

#define DEVID                  0
#define LAYER_BPP              16
#define LINE_WIDTH             2
#define HIFB_RED_1555          0xFF


DisplayTest g_displayTest;

static int32_t GetDisplayInterfaces(void)
{
    int32_t ret;
    ret = LayerInitialize(&g_displayTest.layerFuncs);
    if (ret != DISPLAY_SUCCESS || g_displayTest.layerFuncs == NULL) {
        HDF_LOGE("initialize layer failed");
        return DISPLAY_FAILURE;
    }
    ret = GrallocInitialize(&g_displayTest.grallocFuncs);
    if (ret != DISPLAY_SUCCESS || g_displayTest.grallocFuncs == NULL) {
        HDF_LOGE("initialize gralloc failed");
        return DISPLAY_FAILURE;
    }
    ret = GfxInitialize(&g_displayTest.gfxFuncs);
    if (ret != DISPLAY_SUCCESS) {
        HDF_LOGE("initialize gfx failed");
        return DISPLAY_FAILURE;
    }
    return DISPLAY_SUCCESS;
}

static int32_t DisplayUninit(void)
{
    LayerUninitialize(g_displayTest.layerFuncs);
    GrallocUninitialize(g_displayTest.grallocFuncs);
    GfxUninitialize(g_displayTest.gfxFuncs);
    return DISPLAY_SUCCESS;
}

static void GetLayerInfo(LayerInfo *layInfo)
{
    layInfo->width = g_displayTest.displayInfo.width;
    layInfo->height = g_displayTest.displayInfo.height;
    layInfo->bpp = LAYER_BPP;
    layInfo->pixFormat = PIXEL_FMT_RGBA_5551;
    layInfo->type = LAYER_TYPE_GRAPHIC;
}

static void WriteDataToBuf(int32_t width, int32_t height, uint16_t *pBuf)
{
    int32_t x;
    int32_t y;

    for (y = ((height / LINE_WIDTH) - LINE_WIDTH); y < ((height / LINE_WIDTH) + LINE_WIDTH); y++) {
        for (x = 0; x < width; x++) {
            *((uint16_t*)pBuf + y * width + x) = HIFB_RED_1555;
        }
    }
    for (y = 0; y < height; y++) {
        for (x = ((width / LINE_WIDTH) - LINE_WIDTH); x < ((width / LINE_WIDTH) + LINE_WIDTH); x++) {
            *((uint16_t*)pBuf + y * width + x) = HIFB_RED_1555;
        }
    }

}

int DisplayServiceSample(void)
{    
    int32_t ret;
    g_displayTest.devId = DEVID;

    /* 获取display驱动接口 */
    ret = GetDisplayInterfaces();
    if (ret != DISPLAY_SUCCESS) {
        HDF_LOGE("get display interfaces ops failed");
        return ret;
    }

    /* 初始化显示设备 */
    if (g_displayTest.layerFuncs->InitDisplay != NULL) {
        ret = g_displayTest.layerFuncs->InitDisplay(g_displayTest.devId);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("initialize display failed");
            return DISPLAY_FAILURE;
        }
    }

    /* 获取显示设备的信息 */
    if (g_displayTest.layerFuncs->GetDisplayInfo != NULL) {
        ret = g_displayTest.layerFuncs->GetDisplayInfo(g_displayTest.devId, &g_displayTest.displayInfo);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("get disp info failed");
            return DISPLAY_FAILURE;
        }
    }

    /* 打开显示设备的特定图层 */
    if (g_displayTest.layerFuncs->CreateLayer != NULL) {
        LayerInfo layInfo;
        GetLayerInfo(&layInfo);
        ret = g_displayTest.layerFuncs->CreateLayer(g_displayTest.devId, &layInfo, &g_displayTest.layerId);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("open layer failed");
            return DISPLAY_FAILURE;
        }
    }

    /* 获取图层buffer并填充buffer */
    if (g_displayTest.layerFuncs->GetLayerBuffer != NULL) {
        ret = g_displayTest.layerFuncs->GetLayerBuffer(g_displayTest.devId, g_displayTest.layerId, &g_displayTest.buffer);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("get layer buffer failed");
            return DISPLAY_FAILURE;
        }
        uint16_t *pBuf = (uint16_t *)g_displayTest.buffer.data.virAddr;
        WriteDataToBuf(g_displayTest.displayInfo.width, g_displayTest.displayInfo.height, pBuf);
    }

    /* 刷新图层数据进行显示 */
    if (g_displayTest.layerFuncs->Flush != NULL) {
        ret = g_displayTest.layerFuncs->Flush(g_displayTest.devId, g_displayTest.layerId, &g_displayTest.buffer);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("flush layer failed");
            return DISPLAY_FAILURE;
        }
    }

    return 0;
}