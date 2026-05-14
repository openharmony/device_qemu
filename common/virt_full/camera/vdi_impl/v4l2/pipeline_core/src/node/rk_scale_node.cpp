/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "rk_scale_node.h"
#include "rk_node_utils.h"
#include <securec.h>
#include "cstdint"
#include "memory"
namespace OHOS::Camera {
RKScaleNode::RKScaleNode(const std::string& name, const std::string& type, const std::string &cameraId)
    : NodeBase(name, type, cameraId)
{
    CAMERA_LOGV("%{public}s enter, type(%{public}s)\n", name_.c_str(), type_.c_str());
}

RKScaleNode::~RKScaleNode()
{
    CAMERA_LOGI("~RKScaleNode Node exit.");
}

RetCode RKScaleNode::Start(const int32_t streamId)
{
    CAMERA_LOGI("RKScaleNode::Start streamId = %{public}d\n", streamId);
    return RC_OK;
}

RetCode RKScaleNode::Stop(const int32_t streamId)
{
    CAMERA_LOGI("RKScaleNode::Stop streamId = %{public}d\n", streamId);
    return RC_OK;
}

RetCode RKScaleNode::Flush(const int32_t streamId)
{
    CAMERA_LOGI("RKScaleNode::Flush streamId = %{public}d\n", streamId);
    return RC_OK;
}

void RKScaleNode::DeliverBuffer(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGE("RKScaleNode::DeliverBuffer frameSpec is null");
        return;
    }

    if (buffer->GetBufferStatus() != CAMERA_BUFFER_STATUS_OK) {
        CAMERA_LOGE("BufferStatus() != CAMERA_BUFFER_STATUS_OK");
        return NodeBase::DeliverBuffer(buffer);
    }

    int32_t id = buffer->GetStreamId();
    CAMERA_LOGI("RKScaleNode::DeliverBuffer, streamId[%{public}d],\
index[%{public}d], %{public}d * %{public}d ==> %{public}d * %{public}d, encodeType = %{public}d",
        buffer->GetStreamId(), buffer->GetIndex(),
        buffer->GetCurWidth(), buffer->GetCurHeight(), buffer->GetWidth(), buffer->GetHeight(),
        buffer->GetEncodeType());

    if (buffer->GetEncodeType() == ENCODE_TYPE_NULL) {
        RkNodeUtils::BufferScaleFormatTransform(buffer);
    }
    NodeBase::DeliverBuffer(buffer);
}

RetCode RKScaleNode::Capture(const int32_t streamId, const int32_t captureId)
{
    CAMERA_LOGV("RKScaleNode::Capture");
    return RC_OK;
}

RetCode RKScaleNode::CancelCapture(const int32_t streamId)
{
    CAMERA_LOGI("RKScaleNode::CancelCapture streamid = %{public}d", streamId);

    return RC_OK;
}

REGISTERNODE(RKScaleNode, {"RKScale"})
} // namespace OHOS::Camera
