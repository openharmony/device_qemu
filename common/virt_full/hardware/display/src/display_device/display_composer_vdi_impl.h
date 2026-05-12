/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef _DISPLAY_COMPOSER_VDI_IMPL_H
#define _DISPLAY_COMPOSER_VDI_IMPL_H

#include <vector>
#include <mutex>
#include "hdi_session.h"
#include "idisplay_composer_vdi.h"
#include "v1_0/display_composer_type.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer;
using namespace OHOS::HDI::Display::Composer::V1_0;

class DisplayComposerVdiImpl : public IDisplayComposerVdi {
public:
    static DisplayComposerVdiImpl& GetVdiInstance();
    DisplayComposerVdiImpl();
    virtual ~DisplayComposerVdiImpl();
    virtual int32_t RegHotPlugCallback(HotPlugCallback cb, void* data) override;
    virtual int32_t GetDisplayCapability(uint32_t devId, DisplayCapability& info) override;
    virtual int32_t GetDisplaySupportedModes(uint32_t devId, std::vector<DisplayModeInfo>& modes) override;
    virtual int32_t GetDisplayMode(uint32_t devId, uint32_t& modeId) override;
    virtual int32_t SetDisplayMode(uint32_t devId, uint32_t modeId) override;
    virtual int32_t GetDisplayPowerStatus(uint32_t devId, DispPowerStatus& status) override;
    virtual int32_t SetDisplayPowerStatus(uint32_t devId, DispPowerStatus status) override;
    virtual int32_t GetDisplayBacklight(uint32_t devId, uint32_t& level) override;
    virtual int32_t SetDisplayBacklight(uint32_t devId, uint32_t level) override;
    virtual int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t& value) override;
    virtual int32_t GetDisplayCompChange(uint32_t devId, std::vector<uint32_t>& layers,
        std::vector<int32_t>& types) override;
    virtual int32_t SetDisplayClientCrop(uint32_t devId, const IRect& rect) override;
    virtual int32_t SetDisplayClientBuffer(uint32_t devId, const BufferHandle& buffer, int32_t fence) override;
    virtual int32_t SetDisplayClientDamage(uint32_t devId, std::vector<IRect>& rects) override;
    virtual int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled) override;
    virtual int32_t RegDisplayVBlankCallback(uint32_t devId, VBlankCallback cb, void* data) override;
    virtual int32_t GetDisplayReleaseFence(uint32_t devId, std::vector<uint32_t>& layers,
        std::vector<int32_t>& fences) override;
    virtual int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId) override;
    virtual int32_t DestroyVirtualDisplay(uint32_t devId) override;
    virtual int32_t SetVirtualDisplayBuffer(uint32_t devId, const BufferHandle& buffer, const int32_t fence) override;
    virtual int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value) override;
    virtual int32_t Commit(uint32_t devId, int32_t& fence) override;
    virtual int32_t CreateLayer(uint32_t devId, const LayerInfo& layerInfo, uint32_t& layerId) override;
    virtual int32_t DestroyLayer(uint32_t devId, uint32_t layerId) override;
    virtual int32_t PrepareDisplayLayers(uint32_t devId, bool& needFlushFb) override;
    virtual int32_t SetLayerAlpha(uint32_t devId, uint32_t layerId, const LayerAlpha& alpha) override;
    virtual int32_t SetLayerRegion(uint32_t devId, uint32_t layerId, const IRect& rect) override;
    virtual int32_t SetLayerCrop(uint32_t devId, uint32_t layerId, const IRect& rect) override;
    virtual int32_t SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder) override;
    virtual int32_t SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul) override;
    virtual int32_t SetLayerTransformMode(uint32_t devId, uint32_t layerId, TransformType type) override;
    virtual int32_t SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, const std::vector<IRect>& rects) override;
    virtual int32_t SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, std::vector<IRect>& rects) override;
    virtual int32_t SetLayerBuffer(uint32_t devId, uint32_t layerId,
        const BufferHandle& buffer, int32_t fence) override;
    virtual int32_t SetLayerCompositionType(uint32_t devId, uint32_t layerId, CompositionType type) override;
    virtual int32_t SetLayerBlendType(uint32_t devId, uint32_t layerId, BlendType type) override;
    virtual int32_t SetLayerMaskInfo(uint32_t devId, uint32_t layerId, const MaskInfo maskInfo) override;
    virtual int32_t SetLayerColor(uint32_t devId, uint32_t layerId, const LayerColor& layerColor) override;
private:
    std::mutex mMutex;
};

extern "C" int32_t GetDumpInfo(std::string& result);
extern "C" int32_t UpdateConfig(std::string& result);
} // DISPLAY
}  // HDI
}  // OHOS
#endif // _DISPLAY_COMPOSER_VDI_IMPL_H
