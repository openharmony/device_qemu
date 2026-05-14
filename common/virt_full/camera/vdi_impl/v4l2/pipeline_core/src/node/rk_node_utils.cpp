/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "rk_node_utils.h"
#include "map"
#include "camera.h"
#include "source_node.h"
#include "mutex"
namespace OHOS::Camera {
using namespace std;
static uint32_t ConvertOhosFormat2RkFormat(uint32_t format)
{
    static map<uint32_t, uint32_t> ohosFormat2AVPixelFormatMap = {
        {CAMERA_FORMAT_RGBA_8888,    RK_FORMAT_RGBA_8888},
        {CAMERA_FORMAT_RGB_888,      RK_FORMAT_RGB_888},
        {CAMERA_FORMAT_YCRCB_420_SP, RK_FORMAT_YCbCr_420_SP},
        {CAMERA_FORMAT_YCRCB_420_P,  RK_FORMAT_YCbCr_420_P},
    };
    auto it = ohosFormat2AVPixelFormatMap.find(format);
    if (it != ohosFormat2AVPixelFormatMap.end()) {
        return it->second;
    }
    return RK_FORMAT_UNKNOWN;
}

static bool CheckIfNeedDoTransform(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGE("BufferScaleFormatTransform Error buffer == nullptr");
        return false;
    }
    CAMERA_LOGD("BufferScaleFormatTransform, streamId[%{public}d], index[%{public}d], \
                %{public}d * %{public}d ==> %{public}d * %{public}d, \
                format: %{public}d ==> %{public}d, encodeType: %{public}d",
        buffer->GetStreamId(), buffer->GetIndex(),
        buffer->GetCurWidth(), buffer->GetCurHeight(), buffer->GetWidth(), buffer->GetHeight(),
        buffer->GetCurFormat(), buffer->GetFormat(), buffer->GetEncodeType());

    if (buffer->GetCurWidth() == buffer->GetWidth()
        && buffer->GetCurHeight() == buffer->GetHeight()
        && buffer->GetCurFormat() == buffer->GetFormat()) {
            CAMERA_LOGE("no need ImageFormatConvert, nothing to do");
            return false;
    }
    if (buffer->GetIsValidDataInSurfaceBuffer()) {
        CAMERA_LOGD("IsValidDataInSurfaceBuffer ture");
        if (memcpy_s(buffer->GetVirAddress(), buffer->GetSize(),
            buffer->GetSuffaceBufferAddr(), buffer->GetSuffaceBufferSize()) != 0) {
            CAMERA_LOGE("BufferScaleFormatTransform Fail,  memcpy_s error");
            return false;
        }
    }

    auto srcRkFmt = ConvertOhosFormat2RkFormat(buffer->GetCurFormat());
    auto dstRkFmt = ConvertOhosFormat2RkFormat(buffer->GetFormat());
    if (srcRkFmt == RK_FORMAT_UNKNOWN || dstRkFmt == RK_FORMAT_UNKNOWN) {
        CAMERA_LOGE("RkNodeUtils::BufferScaleFormatTransform Error, not support format: %{public}d -> %{public}d",
            buffer->GetCurFormat(), buffer->GetFormat());
        return false;
    }
    return true;
}

static void TransformToVirAddress(std::shared_ptr<IBuffer>& buffer, int32_t srcRkFmt, int32_t dstRkFmt)
{
    auto tmpBuffer = malloc(buffer->GetSize());
    if (tmpBuffer == nullptr) {
        CAMERA_LOGE("TransformToVirAddress malloc tmpBuffer fail");
        return;
    }
    memcpy_s(tmpBuffer, buffer->GetSize(), buffer->GetVirAddress(), buffer->GetSize());

    RockchipRga rkRga;
    rga_info_t src = {};
    rga_info_t dst = {};

    src.mmuFlag = 1;
    src.rotation = 0;
    src.virAddr = tmpBuffer;
    src.fd = -1;

    dst.mmuFlag = 1;
    dst.fd = -1;
    dst.virAddr = buffer->GetVirAddress();

    rga_set_rect(&src.rect, 0, 0, buffer->GetCurWidth(), buffer->GetCurHeight(),
        buffer->GetCurWidth(), buffer->GetCurHeight(), srcRkFmt);
    rga_set_rect(&dst.rect, 0, 0, buffer->GetWidth(), buffer->GetHeight(),
        buffer->GetWidth(), buffer->GetHeight(), dstRkFmt);

    rkRga.RkRgaBlit(&src, &dst, NULL);
    rkRga.RkRgaFlush();
    free(tmpBuffer);
    buffer->SetIsValidDataInSurfaceBuffer(false);
}
static void TransformToFd(std::shared_ptr<IBuffer>& buffer, int32_t srcRkFmt, int32_t dstRkFmt)
{
    RockchipRga rkRga;
    rga_info_t src = {};
    rga_info_t dst = {};

    src.mmuFlag = 1;
    src.rotation = 0;
    src.virAddr = buffer->GetVirAddress();
    src.fd = -1;

    dst.mmuFlag = 1;
    dst.fd = buffer->GetFileDescriptor();
    dst.virAddr = 0;

    rga_set_rect(&src.rect, 0, 0, buffer->GetCurWidth(), buffer->GetCurHeight(),
        buffer->GetCurWidth(), buffer->GetCurHeight(), srcRkFmt);
    rga_set_rect(&dst.rect, 0, 0, buffer->GetWidth(), buffer->GetHeight(),
        buffer->GetWidth(), buffer->GetHeight(), dstRkFmt);

    rkRga.RkRgaBlit(&src, &dst, NULL);
    rkRga.RkRgaFlush();
    buffer->SetIsValidDataInSurfaceBuffer(true);
}

void RkNodeUtils::BufferScaleFormatTransform(std::shared_ptr<IBuffer>& buffer, bool flagToFd)
{
    static std::mutex mtx;
    if (!CheckIfNeedDoTransform(buffer)) {
        return;
    }
    auto srcRkFmt = ConvertOhosFormat2RkFormat(buffer->GetCurFormat());
    auto dstRkFmt = ConvertOhosFormat2RkFormat(buffer->GetFormat());

    {
        std::lock_guard<std::mutex> l(mtx);
        if (flagToFd) {
            TransformToFd(buffer, srcRkFmt, dstRkFmt);
        } else {
            TransformToVirAddress(buffer, srcRkFmt, dstRkFmt);
        }
    }

    buffer->SetCurFormat(buffer->GetFormat());
    buffer->SetCurWidth(buffer->GetWidth());
    buffer->SetCurHeight(buffer->GetHeight());
}
};
