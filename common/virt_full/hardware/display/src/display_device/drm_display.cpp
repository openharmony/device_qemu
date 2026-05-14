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

#include "drm_display.h"
#include <string>
#include <cerrno>
#include <memory>
#include <sys/ioctl.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include "display_buffer_vdi_impl.h"
#include "display_log.h"
#include "drm_device.h"
#include "drm_vsync_worker.h"
#include "hdi_drm_composition.h"
#include "hdi_gfx_composition.h"
#include "idisplay_buffer_vdi.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Buffer::V1_0;
DrmDisplay::DrmDisplay(std::shared_ptr<DrmConnector> connector, std::shared_ptr<DrmCrtc> crtc,
    std::shared_ptr<DrmDevice> drmDevice)
    : mDrmDevice(drmDevice), mConnector(connector), mCrtc(crtc)
{}

DrmDisplay::~DrmDisplay()
{
    if (mCrtc != nullptr) {
        mCrtc->UnBindDisplay(GetId());
    }
}

int32_t DrmDisplay::Init()
{
    int ret;
    DISPLAY_CHK_RETURN((mCrtc == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("crtc is null"));
    DISPLAY_CHK_RETURN((mConnector == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("connector is null"));
    DISPLAY_CHK_RETURN((mDrmDevice == nullptr), DISPLAY_FAILURE, DISPLAY_LOGE("drmDevice is null"));

    ret = HdiDisplay::Init();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("init failed"));
    auto preComp = std::make_unique<HdiGfxComposition>();
    DISPLAY_CHK_RETURN((preComp == nullptr), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not new HdiGfxComposition errno %{public}d", errno));
    ret = preComp->Init();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not init HdiGfxComposition"));

    auto postComp = std::make_unique<HdiDrmComposition>(mConnector, mCrtc, mDrmDevice);
    DISPLAY_CHK_RETURN((postComp == nullptr), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not new HdiDrmComposition errno %{public}d", errno));
    ret = postComp->Init();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not init HdiDrmComposition"));
    mComposer = std::make_unique<HdiComposer>(std::move(preComp), std::move(postComp));
    ret = mCrtc->BindToDisplay(GetId());
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("bind crtc failed"));

    ret = ChosePreferenceMode();
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("choose preference mode fialed"));
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::GetDisplayCapability(DisplayCapability *info)
{
    mConnector->GetDisplayCap(*info);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::GetDisplaySupportedModes(uint32_t *num, DisplayModeInfo *modes)
{
    mConnector->GetDisplaySupportedModes(num, modes);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::GetDisplayMode(uint32_t *modeId)
{
    DISPLAY_CHK_RETURN((modeId == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("the in modeId is nullptr"));
    *modeId = mCrtc->GetActiveModeId();
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::SetDisplayMode(uint32_t modeId)
{
    return mCrtc->SetActivieMode(modeId);
}

int32_t DrmDisplay::GetDisplayPowerStatus(DispPowerStatus *status)
{
    DISPLAY_CHK_RETURN((status == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("status is nullptr"));
    return ConvertToHdiPowerState(mConnector->GetDpmsState(), *status);
}

int32_t DrmDisplay::SetDisplayPowerStatus(DispPowerStatus status)
{
    DISPLAY_LOGD("the status %{public}d ", status);
    uint32_t drmPowerState = 0;
    int ret = ConvertToDrmPowerState(status, drmPowerState);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_PARAM_ERR,
        DISPLAY_LOGE("unknown power status %{public}d", status));
    mConnector->SetDpmsState(drmPowerState);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::ConvertToHdiPowerState(uint32_t drmPowerState, DispPowerStatus &hdiPowerState)
{
    int32_t ret = DISPLAY_SUCCESS;
    switch (drmPowerState) {
        case DRM_MODE_DPMS_OFF:
            hdiPowerState = POWER_STATUS_OFF;
            break;
        case DRM_MODE_DPMS_ON:
            hdiPowerState = POWER_STATUS_ON;
            break;
        case DRM_MODE_DPMS_STANDBY:
            hdiPowerState = POWER_STATUS_STANDBY;
            break;
        case DRM_MODE_DPMS_SUSPEND:
            hdiPowerState = POWER_STATUS_SUSPEND;
            break;
        default:
            hdiPowerState = POWER_STATUS_BUTT;
            ret = DISPLAY_FAILURE;
            break;
    }
    DISPLAY_LOGD("hdi power state %{public}u", hdiPowerState);
    return ret;
}

int32_t DrmDisplay::ConvertToDrmPowerState(DispPowerStatus hdiPowerState, uint32_t &drmPowerState)
{
    int32_t ret = DISPLAY_SUCCESS;
    switch (hdiPowerState) {
        case POWER_STATUS_OFF:
            drmPowerState = DRM_MODE_DPMS_OFF;
            break;
        case POWER_STATUS_ON:
            drmPowerState = DRM_MODE_DPMS_ON;
            break;
        case POWER_STATUS_STANDBY:
            drmPowerState = DRM_MODE_DPMS_STANDBY;
            break;
        case POWER_STATUS_SUSPEND:
            drmPowerState = DRM_MODE_DPMS_SUSPEND;
            break;
        default:
            ret = DISPLAY_FAILURE;
            break;
    }
    return ret;
}

std::unique_ptr<HdiLayer> DrmDisplay::CreateHdiLayer(LayerType type)
{
    DISPLAY_LOGD();
    return std::make_unique<HdiDrmLayer>(type);
}

int32_t DrmDisplay::WaitForVBlank(uint64_t *ns)
{
    struct timespec curTime = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &curTime);
    usleep(16666);
    // 1000000000 is used for transfer second to nsec
    uint64_t duration = static_cast<uint64_t>(curTime.tv_sec) * 1000000 + static_cast<uint64_t>(curTime.tv_nsec);
    *ns = static_cast<uint64_t>(duration);
    return DISPLAY_SUCCESS;
}

bool DrmDisplay::IsConnected()
{
    DISPLAY_LOGD("conneted %{public}d", mConnector->IsConnected());
    return mConnector->IsConnected();
}

int32_t DrmDisplay::PushFirstFrame()
{
    std::shared_ptr<IDisplayBufferVdi> hdiImpl = std::make_shared<DisplayBufferVdiImpl>();
    int ret = DISPLAY_SUCCESS;
    DrmMode mode;
    ret = mConnector->GetModeFromId(mCrtc->GetActiveModeId(), mode);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE,
        DISPLAY_LOGE("can not get the mode from id %{public}d", mCrtc->GetActiveModeId()));
    const AllocInfo info = {
        .width = mode.GetModeInfoPtr()->hdisplay,
        .height = mode.GetModeInfoPtr()->vdisplay,
        .usage = HBM_USE_MEM_DMA | HBM_USE_CPU_READ | HBM_USE_CPU_WRITE,
        .format = PIXEL_FMT_RGBX_8888
    };

    BufferHandle *buffer = nullptr;
    ret = hdiImpl->AllocMem(info, buffer);
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not alloc memory"));
    mClientLayer->SetLayerBuffer(buffer, -1);

    std::vector<HdiLayer *> layers;
    HdiDrmComposition *drmComp = static_cast<HdiDrmComposition *>(mComposer->GetPostCompostion());
    drmComp->SetLayers(layers, *mClientLayer);
    drmComp->Apply(true);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::ChosePreferenceMode()
{
    int32_t ret;
    int32_t modeId = mConnector->GetPreferenceId();
    if (modeId == INVALID_MODE_ID) {
        uint32_t num = 0;
        ret = GetDisplaySupportedModes(&num, nullptr);
        DISPLAY_CHK_RETURN((num == 0) && (ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("can not get modes"));
        modeId = 0;
    }
    ret = SetDisplayMode(modeId);
    // Push first frame to the drm, for that the vblank must init all the componet.
    DISPLAY_CHK_RETURN((ret != DISPLAY_SUCCESS), DISPLAY_FAILURE, DISPLAY_LOGE("set display mode failed"));
    return PushFirstFrame();
}

int32_t DrmDisplay::RegDisplayVBlankCallback(VBlankCallback cb, void *data)
{
    (void)data;
    std::shared_ptr<VsyncCallBack> vsyncCb = std::make_shared<VsyncCallBack>(cb, data, mCrtc->GetPipe());
    DrmVsyncWorker::GetInstance().ReqesterVBlankCb(vsyncCb);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::SetDisplayVsyncEnabled(bool enabled)
{
    DISPLAY_LOGD("enable %{public}d", enabled);
    DrmVsyncWorker::GetInstance().EnableVsync(enabled);
    return DISPLAY_SUCCESS;
}

int32_t DrmDisplay::GetDisplayBacklight(uint32_t *value)
{
    DISPLAY_CHK_RETURN((value == nullptr), DISPLAY_NULL_PTR, DISPLAY_LOGE("value is nullptr"));
    return mConnector->GetBrightness(*value);
}

int32_t DrmDisplay::SetDisplayBacklight(uint32_t value)
{
    return mConnector->SetBrightness(value);
}
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY
