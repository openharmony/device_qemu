/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "disp_dev.h"
#include "components/root_view.h"
#include "draw/draw_utils.h"
#include "fbdev.h"

namespace OHOS {
DispDev *DispDev::GetInstance()
{
    static DispDev instance;
    if (!instance.isRegister_) {
        FbdevInit();
        instance.isRegister_ = true;
    }
    return &instance;
}

BufferInfo *DispDev::GetFBBufferInfo()
{
    const int DEFAULT_FBUF_COLOR = 0x44;

    static BufferInfo bufferInfo;
    LiteSurfaceData *surfaceData = GetDevSurfaceData();
    bufferInfo.rect = {0, 0, HORIZONTAL_RESOLUTION - 1, VERTICAL_RESOLUTION - 1};
    bufferInfo.mode = ARGB8888;
    bufferInfo.color = DEFAULT_FBUF_COLOR;
    bufferInfo.phyAddr = surfaceData->phyAddr;
    bufferInfo.virAddr = surfaceData->virAddr;
    bufferInfo.stride = HORIZONTAL_RESOLUTION * (DrawUtils::GetByteSizeByColorMode(bufferInfo.mode));
    bufferInfo.width = HORIZONTAL_RESOLUTION;
    bufferInfo.height = VERTICAL_RESOLUTION;
    this->fbAddr = surfaceData->phyAddr;
    return &bufferInfo;
}

void DispDev::UpdateFBBuffer()
{
    BufferInfo *bufferInfo = DispDev::GetInstance()->GetFBBufferInfo();
    if (this->fbAddr != bufferInfo->phyAddr) {
        this->fbAddr = bufferInfo->phyAddr;
        RootView::GetInstance()->UpdateBufferInfo(bufferInfo);
    }
}

void DispDev::Flush(const OHOS::Rect& flushRect)
{
    FbdevFlush();
}
} // namespace OHOS
