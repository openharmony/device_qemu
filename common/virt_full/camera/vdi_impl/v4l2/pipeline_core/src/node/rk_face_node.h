/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef HOS_CAMERA_RKFACE_NODE_H
#define HOS_CAMERA_RKFACE_NODE_H

#include <vector>
#include <condition_variable>
#include <ctime>
#include "device_manager_adapter.h"
#include "utils.h"
#include "camera.h"
#include "source_node.h"

enum FaceRectanglesIndex : int32_t {
    INDEX_0 = 0,
    INDEX_1,
    INDEX_2,
    INDEX_3,
};

namespace OHOS::Camera {
std::vector<uint32_t> FaceDetectMetadataTag = {
    OHOS_STATISTICS_FACE_DETECT_SWITCH,
    OHOS_STATISTICS_FACE_RECTANGLES,
    OHOS_STATISTICS_FACE_IDS
};

class RKFaceNode : public NodeBase {
public:
    RKFaceNode(const std::string &name, const std::string &type, const std::string &cameraId);
    ~RKFaceNode() override;
    RetCode Start(const int32_t streamId) override;
    RetCode Stop(const int32_t streamId) override;
    void DeliverBuffer(std::shared_ptr<IBuffer>& buffer) override;
    virtual RetCode Capture(const int32_t streamId, const int32_t captureId) override;
    RetCode CancelCapture(const int32_t streamId) override;
    RetCode Flush(const int32_t streamId);
    RetCode Config(const int32_t streamId, const CaptureMeta& meta) override;

private:
    RetCode GetFaceDetectMetaData(std::shared_ptr<CameraMetadata> &metadata);
    RetCode GetCameraFaceDetectSwitch(std::shared_ptr<CameraMetadata> &metadata);
    RetCode GetCameraFaceRectangles(std::shared_ptr<CameraMetadata> &metadata);
    RetCode GetCameraFaceIds(std::shared_ptr<CameraMetadata> &metadata);
    RetCode CopyBuffer(unsigned char *sourceBuffer, std::shared_ptr<IBuffer>& outPutBuffer, int32_t dataSize);
    RetCode CopyMetadataBuffer(std::shared_ptr<CameraMetadata> &metadata,
        std::shared_ptr<IBuffer>& outPutBuffer, int32_t dataSize);
    RetCode CreateMetadataInfo();

private:
    std::vector<std::shared_ptr<IPort>> outPutPorts_;
    std::mutex mLock_;
    std::shared_ptr<CameraMetadata> metaData_ = nullptr;
    std::condition_variable cv_;
    int32_t metaDataSize_;
};
} // namespace OHOS::Camera
#endif
