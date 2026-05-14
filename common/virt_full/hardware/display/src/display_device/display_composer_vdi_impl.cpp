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

#include "display_composer_vdi_impl.h"
#include <hdf_base.h>
#include "display_log.h"
#include "hdf_log.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
DisplayComposerVdiImpl::DisplayComposerVdiImpl()
{
}

DisplayComposerVdiImpl::~DisplayComposerVdiImpl()
{
}

int32_t DisplayComposerVdiImpl::RegHotPlugCallback(HotPlugCallback cb, void* data)
{
    HdiSession::GetInstance().RegHotPlugCallback(cb, data);
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayCapability(uint32_t devId, DisplayCapability& info)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCapability, &info);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplaySupportedModes(uint32_t devId, std::vector<DisplayModeInfo>& modes)
{
    std::lock_guard<std::mutex> lock(mMutex);
    DisplayModeInfo* placeHoler = nullptr;
    uint32_t num = 0;
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplaySupportedModes,
        &num, placeHoler);
    if (ec != DISPLAY_SUCCESS) {
        DISPLAY_LOGE("failed, ec=%{public}d", ec);
        return HDF_FAILURE;
    }
    if (num != 0) {
        modes.resize(num);
        ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplaySupportedModes, &num,
            modes.data());
    }
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayMode(uint32_t devId, uint32_t& modeId)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayMode, &modeId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayMode(uint32_t devId, uint32_t modeId)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayMode, modeId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayPowerStatus(uint32_t devId, DispPowerStatus& status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayPowerStatus, &status);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayPowerStatus(uint32_t devId, DispPowerStatus status)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayPowerStatus, status);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayBacklight(uint32_t devId, uint32_t& level)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayBacklight, &level);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayBacklight(uint32_t devId, uint32_t level)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayBacklight, level);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t& value)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::GetDisplayCompChange(uint32_t devId, std::vector<uint32_t>& layers,
    std::vector<int32_t>& types)
{
    std::lock_guard<std::mutex> lock(mMutex);
    uint32_t* layersHoler = nullptr;
    int32_t* typesHoler = nullptr;
    uint32_t num = 0;
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCompChange, &num,
        layersHoler, typesHoler);
    if (ec == HDF_SUCCESS && num != 0) {
        layers.resize(num);
        types.resize(num);
        ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayCompChange, &num,
            layers.data(), types.data());
    }
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayClientCrop(uint32_t devId, const IRect& rect)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetDisplayClientBuffer(uint32_t devId, const BufferHandle& buffer, int32_t fence)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayClientBuffer, &buffer,
        fence);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetDisplayClientDamage(uint32_t devId, std::vector<IRect>& rects)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetDisplayVsyncEnabled(uint32_t devId, bool enabled)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetDisplayVsyncEnabled, enabled);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::RegDisplayVBlankCallback(uint32_t devId, VBlankCallback cb, void* data)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::RegDisplayVBlankCallback, cb, data);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::GetDisplayReleaseFence(uint32_t devId, std::vector<uint32_t>& layers,
    std::vector<int32_t>& fences)
{
    std::lock_guard<std::mutex> lock(mMutex);
    uint32_t* layersHoler = nullptr;
    int32_t* typesHoler = nullptr;
    uint32_t num = 0;
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayReleaseFence, &num,
        layersHoler, typesHoler);
    if (ec == HDF_SUCCESS && num != 0) {
        layers.resize(num);
        fences.resize(num);
        ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::GetDisplayReleaseFence, &num,
            layers.data(), fences.data());
    }
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::DestroyVirtualDisplay(uint32_t devId)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetVirtualDisplayBuffer(uint32_t devId, const BufferHandle& buffer, const int32_t fence)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::Commit(uint32_t devId, int32_t& fence)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::Commit, &fence);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::CreateLayer(uint32_t devId, const LayerInfo& layerInfo, uint32_t& layerId)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::CreateLayer, &layerInfo, &layerId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::DestroyLayer(uint32_t devId, uint32_t layerId)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::DestroyLayer, layerId);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::PrepareDisplayLayers(uint32_t devId, bool& needFlushFb)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::PrepareDisplayLayers, &needFlushFb);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerAlpha(uint32_t devId, uint32_t layerId, const LayerAlpha& alpha)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerAlpha,
        const_cast<LayerAlpha*>(&alpha));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerRegion(uint32_t devId, uint32_t layerId, const IRect& rect)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerRegion,
        const_cast<IRect*>(&rect));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerCrop(uint32_t devId, uint32_t layerId, const IRect& rect)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerCrop,
        const_cast<IRect*>(&rect));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallDisplayFunction(devId, &HdiDisplay::SetLayerZorder, layerId, zorder);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerPreMulti, preMul);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerTransformMode(uint32_t devId, uint32_t layerId, TransformType type)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerTransformMode, type);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, const std::vector<IRect>& rects)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerDirtyRegion,
        const_cast<IRect*>(rects.data()));
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, std::vector<IRect>& rects)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetLayerBuffer(uint32_t devId, uint32_t layerId, const BufferHandle& buffer,
    int32_t fence)
{
    std::lock_guard<std::mutex> lock(mMutex);
    const BufferHandle* holder = &buffer;
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerBuffer, holder, fence);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerCompositionType(uint32_t devId, uint32_t layerId, CompositionType type)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerCompositionType, type);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerBlendType(uint32_t devId, uint32_t layerId, BlendType type)
{
    std::lock_guard<std::mutex> lock(mMutex);
    int32_t ec = HdiSession::GetInstance().CallLayerFunction(devId, layerId, &HdiLayer::SetLayerBlendType, type);
    DISPLAY_CHK_RETURN(ec != DISPLAY_SUCCESS, HDF_FAILURE, DISPLAY_LOGE("failed, ec=%{public}d", ec));
    return HDF_SUCCESS;
}

int32_t DisplayComposerVdiImpl::SetLayerMaskInfo(uint32_t devId, uint32_t layerId, const MaskInfo maskInfo)
{
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t DisplayComposerVdiImpl::SetLayerColor(uint32_t devId, uint32_t layerId, const LayerColor& layerColor)
{
    DISPLAY_LOGE("%s layerColor: r=%{public}d, g=%{public}d, b=%{public}d, a=%{public}d",
        __func__, layerColor.r, layerColor.g, layerColor.b, layerColor.a);
    DISPLAY_LOGE("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t GetDumpInfo(std::string& result)
{
    DISPLAY_LOGI("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

int32_t UpdateConfig(std::string& result)
{
    DISPLAY_LOGI("%s is not supported", __func__);
    return HDF_ERR_NOT_SUPPORT;
}

extern "C" IDisplayComposerVdi *CreateComposerVdi()
{
    return new DisplayComposerVdiImpl();
}

extern "C" void DestroyComposerVdi(IDisplayComposerVdi* vdi)
{
    delete vdi;
}

DisplayComposerVdiImpl& DisplayComposerVdiImpl::GetVdiInstance()
{
    static DisplayComposerVdiImpl vdiImpl;
    return vdiImpl;
}

extern "C" int32_t RegHotPlugCallback(HotPlugCallback cb, void* data)
{
    return DisplayComposerVdiImpl::GetVdiInstance().RegHotPlugCallback(cb, data);
}

extern "C" int32_t GetDisplayCapability(uint32_t devId, V1_0::DisplayCapability& info)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplayCapability(devId, info);
}

extern "C" int32_t GetDisplaySupportedModes(uint32_t devId, std::vector<V1_0::DisplayModeInfo>& modes)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplaySupportedModes(devId, modes);
}

extern "C" int32_t GetDisplayMode(uint32_t devId, uint32_t& modeId)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplayMode(devId, modeId);
}

extern "C" int32_t SetDisplayMode(uint32_t devId, uint32_t modeId)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayMode(devId, modeId);
}

extern "C" int32_t GetDisplayPowerStatus(uint32_t devId, V1_0::DispPowerStatus& status)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplayPowerStatus(devId, status);
}

extern "C" int32_t SetDisplayPowerStatus(uint32_t devId, V1_0::DispPowerStatus status)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayPowerStatus(devId, status);
}

extern "C" int32_t GetDisplayBacklight(uint32_t devId, uint32_t& level)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplayBacklight(devId, level);
}

extern "C" int32_t SetDisplayBacklight(uint32_t devId, uint32_t level)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayBacklight(devId, level);
}

extern "C" int32_t GetDisplayProperty(uint32_t devId, uint32_t id, uint64_t& value)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplayProperty(devId, id, value);
}

extern "C" int32_t GetDisplayCompChange(uint32_t devId, std::vector<uint32_t>& layers, std::vector<int32_t>& types)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplayCompChange(devId, layers, types);
}

extern "C" int32_t SetDisplayClientCrop(uint32_t devId, const V1_0::IRect& rect)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayClientCrop(devId, rect);
}

extern "C" int32_t SetDisplayClientBuffer(uint32_t devId, const BufferHandle& buffer, int32_t fence)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayClientBuffer(devId, buffer, fence);
}

extern "C" int32_t SetDisplayClientDamage(uint32_t devId, std::vector<V1_0::IRect>& rects)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayClientDamage(devId, rects);
}

extern "C" int32_t SetDisplayVsyncEnabled(uint32_t devId, bool enabled)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayVsyncEnabled(devId, enabled);
}

extern "C" int32_t RegDisplayVBlankCallback(uint32_t devId, VBlankCallback cb, void* data)
{
    return DisplayComposerVdiImpl::GetVdiInstance().RegDisplayVBlankCallback(devId, cb, data);
}

extern "C" int32_t GetDisplayReleaseFence(uint32_t devId, std::vector<uint32_t>& layers, std::vector<int32_t>& fences)
{
    return DisplayComposerVdiImpl::GetVdiInstance().GetDisplayReleaseFence(devId, layers, fences);
}

extern "C" int32_t CreateVirtualDisplay(uint32_t width, uint32_t height, int32_t& format, uint32_t& devId)
{
    return DisplayComposerVdiImpl::GetVdiInstance().CreateVirtualDisplay(width, height, format, devId);
}

extern "C" int32_t DestroyVirtualDisplay(uint32_t devId)
{
    return DisplayComposerVdiImpl::GetVdiInstance().DestroyVirtualDisplay(devId);
}

extern "C" int32_t SetVirtualDisplayBuffer(uint32_t devId, const BufferHandle& buffer, const int32_t fence)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetVirtualDisplayBuffer(devId, buffer, fence);
}

extern "C" int32_t SetDisplayProperty(uint32_t devId, uint32_t id, uint64_t value)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetDisplayProperty(devId, id, value);
}

extern "C" int32_t Commit(uint32_t devId, int32_t& fence)
{
    return DisplayComposerVdiImpl::GetVdiInstance().Commit(devId, fence);
}

extern "C" int32_t CreateLayer(uint32_t devId, const V1_0::LayerInfo& layerInfo, uint32_t& layerId)
{
    return DisplayComposerVdiImpl::GetVdiInstance().CreateLayer(devId, layerInfo, layerId);
}

extern "C" int32_t DestroyLayer(uint32_t devId, uint32_t layerId)
{
    return DisplayComposerVdiImpl::GetVdiInstance().DestroyLayer(devId, layerId);
}

extern "C" int32_t PrepareDisplayLayers(uint32_t devId, bool& needFlushFb)
{
    return DisplayComposerVdiImpl::GetVdiInstance().PrepareDisplayLayers(devId, needFlushFb);
}

extern "C" int32_t SetLayerAlpha(uint32_t devId, uint32_t layerId, const V1_0::LayerAlpha& alpha)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerAlpha(devId, layerId, alpha);
}

extern "C" int32_t SetLayerRegion(uint32_t devId, uint32_t layerId, const V1_0::IRect& rect)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerRegion(devId, layerId, rect);
}

extern "C" int32_t SetLayerCrop(uint32_t devId, uint32_t layerId, const V1_0::IRect& rect)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerCrop(devId, layerId, rect);
}

extern "C" int32_t SetLayerZorder(uint32_t devId, uint32_t layerId, uint32_t zorder)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerZorder(devId, layerId, zorder);
}

extern "C" int32_t SetLayerPreMulti(uint32_t devId, uint32_t layerId, bool preMul)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerPreMulti(devId, layerId, preMul);
}

extern "C" int32_t SetLayerTransformMode(uint32_t devId, uint32_t layerId, V1_0::TransformType type)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerTransformMode(devId, layerId, type);
}

extern "C" int32_t SetLayerDirtyRegion(uint32_t devId, uint32_t layerId, const std::vector<V1_0::IRect>& rects)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerDirtyRegion(devId, layerId, rects);
}

extern "C" int32_t SetLayerVisibleRegion(uint32_t devId, uint32_t layerId, std::vector<V1_0::IRect>& rects)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerVisibleRegion(devId, layerId, rects);
}

extern "C" int32_t SetLayerBuffer(uint32_t devId, uint32_t layerId, const BufferHandle& buffer, int32_t fence)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerBuffer(devId, layerId, buffer, fence);
}

extern "C" int32_t SetLayerCompositionType(uint32_t devId, uint32_t layerId, V1_0::CompositionType type)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerCompositionType(devId, layerId, type);
}

extern "C" int32_t SetLayerBlendType(uint32_t devId, uint32_t layerId, V1_0::BlendType type)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerBlendType(devId, layerId, type);
}

extern "C" int32_t SetLayerMaskInfo(uint32_t devId, uint32_t layerId, const V1_0::MaskInfo maskInfo)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerMaskInfo(devId, layerId, maskInfo);
}

extern "C" int32_t SetLayerColor(uint32_t devId, uint32_t layerId, const V1_0::LayerColor& layerColor)
{
    return DisplayComposerVdiImpl::GetVdiInstance().SetLayerColor(devId, layerId, layerColor);
}

} // DISPLAY
} // HDI
} // OHOS
