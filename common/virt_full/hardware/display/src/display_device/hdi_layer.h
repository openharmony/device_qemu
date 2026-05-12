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

#ifndef HDI_LAYER_H
#define HDI_LAYER_H
#include <unordered_set>
#include <memory>
#include "buffer_handle.h"
#include "v1_0/display_composer_type.h"
#include "hdi_device_common.h"
#include "hdi_shared_fd.h"

namespace OHOS {
namespace HDI {
namespace DISPLAY {
using namespace OHOS::HDI::Display::Composer::V1_0;
const uint32_t INVALIDE_LAYER_ID = 0xffffffff;
const uint32_t FENCE_TIMEOUT = 3000;
struct HdiLayerBuffer {
public:
    explicit HdiLayerBuffer(const BufferHandle &hdl);
    virtual ~HdiLayerBuffer();
    HdiLayerBuffer &operator = (const BufferHandle &right);
    uint64_t GetPhysicalAddr() const
    {
        return mPhyAddr;
    }
    int32_t GetHeight() const
    {
        return mHeight;
    }
    int32_t GetWight() const
    {
        return mWidth;
    }
    int32_t GetStride() const
    {
        return mStride;
    }
    int32_t GetFormat() const
    {
        return mFormat;
    }
    int GetFb() const
    {
        return mFd;
    }
    BufferHandle mHandle;

private:
    uint64_t mPhyAddr = 0;
    int32_t mHeight = 0;
    int32_t mWidth = 0;
    int32_t mStride = 0;
    int32_t mFormat = 0;
    int mFd = -1;
};

class HdiLayer {
public:
    explicit HdiLayer(LayerType type) : mType(type) {}
    int32_t Init();
    uint32_t GetId() const
    {
        return mId;
    }
    uint32_t GetZorder() const
    {
        return mZorder;
    }
    const IRect &GetLayerDisplayRect() const
    {
        return mDisplayRect;
    }
    const IRect &GetLayerCrop() const
    {
        return mCrop;
    }
    bool GetLayerPreMulti() const
    {
        return mPreMul;
    }
    const LayerAlpha &GetAlpha() const
    {
        return mAlpha;
    }
    LayerType GetType() const
    {
        return mType;
    }
    TransformType GetTransFormType() const
    {
        return mTransformType;
    }
    BlendType GetLayerBlenType() const
    {
        return mBlendType;
    }
    CompositionType GetCompositionType() const
    {
        return mCompositionType;
    }
    void SetDeviceSelect(CompositionType type)
    {
        DISPLAY_LOGD("%{public}d", type);
        mDeviceSelect = type;
    }
    CompositionType GetDeviceSelect() const
    {
        return mDeviceSelect;
    }

    int GetAcquireFenceFd()
    {
        return mAcquireFence.GetFd();
    }
    int GetReleaseFenceFd()
    {
        return mReleaseFence.GetFd();
    }
    void SetReleaseFence(int fd)
    {
        mReleaseFence = fd;
    };
    void ClearColor(uint32_t color);

    void SetPixel(const BufferHandle &handle, int x, int y, uint32_t color);

    void WaitAcquireFence();
    virtual int32_t SetLayerRegion(IRect *rect);
    virtual int32_t SetLayerCrop(IRect *rect);
    virtual void SetLayerZorder(uint32_t zorder);
    virtual int32_t SetLayerPreMulti(bool preMul);
    virtual int32_t SetLayerAlpha(LayerAlpha *alpha);
    virtual int32_t SetLayerTransformMode(TransformType type);
    virtual int32_t SetLayerDirtyRegion(IRect *region);
    virtual int32_t SetLayerVisibleRegion(uint32_t num, IRect *rect);
    virtual int32_t SetLayerBuffer(const BufferHandle *buffer, int32_t fence);
    virtual int32_t SetLayerCompositionType(CompositionType type);
    virtual int32_t SetLayerBlendType(BlendType type);
    virtual HdiLayerBuffer *GetCurrentBuffer()
    {
        return mHdiBuffer.get();
    }
    virtual ~HdiLayer()
    {
        mIdSets.erase(mId);
    }

private:
    static uint32_t GetIdleId();
    static uint32_t mIdleId;
    static std::unordered_set<uint32_t> mIdSets;

    uint32_t mId = 0;
    HdiFd mAcquireFence;
    HdiFd mReleaseFence;
    LayerType mType;

    IRect mDisplayRect;
    IRect mCrop;
    uint32_t mZorder = -1;
    bool mPreMul = false;
    LayerAlpha mAlpha;
    int32_t mFenceTimeOut = FENCE_TIMEOUT;
    TransformType mTransformType = ROTATE_BUTT;
    CompositionType mCompositionType = COMPOSITION_CLIENT;
    CompositionType mDeviceSelect = COMPOSITION_CLIENT;
    BlendType mBlendType;
    std::unique_ptr<HdiLayerBuffer> mHdiBuffer;
};

struct SortLayersByZ {
    bool operator () (const HdiLayer *lhs, const HdiLayer *rhs) const
    {
        if (lhs == nullptr || rhs == nullptr) {
            return (lhs == nullptr) && (rhs == nullptr);
        }
        return lhs->GetZorder() < rhs->GetZorder();
    }
};
} // namespace OHOS
} // namespace HDI
} // namespace DISPLAY

#endif // HDI_LAYER_H
