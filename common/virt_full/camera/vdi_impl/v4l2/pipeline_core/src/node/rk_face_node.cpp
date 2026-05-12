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

#include "rk_face_node.h"
#include <securec.h>
#include "camera_dump.h"

namespace OHOS::Camera {
RKFaceNode::RKFaceNode(const std::string &name, const std::string &type, const std::string &cameraId)
    : NodeBase(name, type, cameraId), metaDataSize_(0)
{
    CAMERA_LOGV("%{public}s enter, type(%{public}s)\n", name_.c_str(), type_.c_str());
}

RKFaceNode::~RKFaceNode()
{
    CAMERA_LOGI("~RKFaceNode Node exit.");
}

RetCode RKFaceNode::Start(const int32_t streamId)
{
    CAMERA_LOGI("RKFaceNode::Start streamId = %{public}d\n", streamId);
    CreateMetadataInfo();
    return RC_OK;
}

RetCode RKFaceNode::Stop(const int32_t streamId)
{
    CAMERA_LOGI("RKFaceNode::Stop streamId = %{public}d\n", streamId);
    std::unique_lock <std::mutex> lock(mLock_);
    metaDataSize_ = 0;
    return RC_OK;
}

RetCode RKFaceNode::Flush(const int32_t streamId)
{
    CAMERA_LOGI("RKFaceNode::Flush streamId = %{public}d\n", streamId);
    return RC_OK;
}

void RKFaceNode::DeliverBuffer(std::shared_ptr<IBuffer>& buffer)
{
    if (buffer == nullptr) {
        CAMERA_LOGE("RKFaceNode::DeliverBuffer frameSpec is null");
        return;
    }

    CameraDumper& dumper = CameraDumper::GetInstance();
    dumper.DumpBuffer("board_RKFaceNode", ENABLE_RKFACE_NODE_CONVERTED, buffer);
    NodeBase::DeliverBuffer(buffer);
}

RetCode RKFaceNode::Config(const int32_t streamId, const CaptureMeta& meta)
{
    (void)meta;
    return RC_OK;
}

RetCode RKFaceNode::Capture(const int32_t streamId, const int32_t captureId)
{
    CAMERA_LOGV("RKFaceNode::Capture");
    return RC_OK;
}

RetCode RKFaceNode::CancelCapture(const int32_t streamId)
{
    CAMERA_LOGI("RKFaceNode::CancelCapture streamid = %{public}d", streamId);
    return RC_OK;
}

RetCode RKFaceNode::GetFaceDetectMetaData(std::shared_ptr<CameraMetadata> &metadata)
{
    GetCameraFaceDetectSwitch(metadata);
    GetCameraFaceRectangles(metadata);
    GetCameraFaceIds(metadata);
    return RC_OK;
}

RetCode RKFaceNode::GetCameraFaceDetectSwitch(std::shared_ptr<CameraMetadata> &metadata)
{
    uint8_t faceDetectSwitch = OHOS_CAMERA_FACE_DETECT_MODE_SIMPLE;
    metadata->addEntry(OHOS_STATISTICS_FACE_DETECT_SWITCH, &faceDetectSwitch, sizeof(uint8_t));
    return RC_OK;
}

RetCode RKFaceNode::GetCameraFaceRectangles(std::shared_ptr<CameraMetadata> &metadata)
{
    constexpr int32_t row = 3;
    constexpr int32_t col = 4;
    constexpr float rect_one_x = 0.0; // dummy data: faceRectangles data
    constexpr float rect_one_y = 0.0;
    constexpr float rect_one_width = 0.2;
    constexpr float rect_one_height = 0.3;

    constexpr float rect_two_x = 0.3; // dummy data: faceRectangles data
    constexpr float rect_two_y = 0.3;
    constexpr float rect_two_width = 0.2;
    constexpr float rect_two_height = 0.3;

    constexpr float rect_three_x = 0.6; // dummy data: faceRectangles data
    constexpr float rect_three_y = 0.6;
    constexpr float rect_three_width = 0.2;
    constexpr float rect_three_height = 0.3;

    float faceRectangles[row][col];
    faceRectangles[INDEX_0][INDEX_0] = rect_one_x;
    faceRectangles[INDEX_0][INDEX_1] = rect_one_y;
    faceRectangles[INDEX_0][INDEX_2] = rect_one_width;
    faceRectangles[INDEX_0][INDEX_3] = rect_one_height;

    faceRectangles[INDEX_1][INDEX_0] = rect_two_x;
    faceRectangles[INDEX_1][INDEX_1] = rect_two_y;
    faceRectangles[INDEX_1][INDEX_2] = rect_two_width;
    faceRectangles[INDEX_1][INDEX_3] = rect_two_height;

    faceRectangles[INDEX_2][INDEX_0] = rect_three_x;
    faceRectangles[INDEX_2][INDEX_1] = rect_three_y;
    faceRectangles[INDEX_2][INDEX_2] = rect_three_width;
    faceRectangles[INDEX_2][INDEX_3] = rect_three_height;
    metadata->addEntry(OHOS_STATISTICS_FACE_RECTANGLES, static_cast<void*>(&faceRectangles[0]),
        row * col);
    return RC_OK;
}

RetCode RKFaceNode::GetCameraFaceIds(std::shared_ptr<CameraMetadata> &metadata)
{
    std::vector<int32_t> vFaceIds;
    constexpr int32_t id_zero = 0;
    constexpr int32_t id_one = 1;
    constexpr int32_t id_two = 2;
    vFaceIds.push_back(id_zero);
    vFaceIds.push_back(id_one);
    vFaceIds.push_back(id_two);
    metadata->addEntry(OHOS_STATISTICS_FACE_IDS, vFaceIds.data(), vFaceIds.size());
    return RC_OK;
}

RetCode RKFaceNode::CopyMetadataBuffer(std::shared_ptr<CameraMetadata> &metadata,
    std::shared_ptr<IBuffer>& outPutBuffer, int32_t dataSize)
{
    int bufferSize = outPutBuffer->GetSize();
    int metadataSize = metadata->get()->size;
    CAMERA_LOGI("outPutBuffer.size=%{public}d  and metadataSize=%{public}d ", bufferSize, metadataSize);
    int ret = 0;
    ret = memset_s(outPutBuffer->GetVirAddress(),  bufferSize, 0,  bufferSize);
    if (ret != RC_OK) {
        CAMERA_LOGE("memset_s failed");
        return RC_ERROR;
    }

    if (memcpy_s(outPutBuffer->GetVirAddress(), metadataSize, static_cast<void*>(metadata->get()),
        metadataSize) != 0) {
        CAMERA_LOGE("memcpy_s failed");
        return RC_ERROR;
    }
    outPutBuffer->SetEsFrameSize(metadataSize);
    return RC_OK;
}

RetCode RKFaceNode::CopyBuffer(unsigned char *sourceBuffer, std::shared_ptr<IBuffer>& outPutBuffer, int32_t dataSize)
{
    if (memcpy_s(outPutBuffer->GetVirAddress(), dataSize, sourceBuffer, dataSize) != 0) {
        CAMERA_LOGE("copy buffer memcpy_s failed");
        return RC_ERROR;
    }
    outPutBuffer->SetEsFrameSize(dataSize);
    return RC_OK;
}

RetCode RKFaceNode::CreateMetadataInfo()
{
    const int ENTRY_CAPACITY = 30; // 30:entry capacity
    const int DATA_CAPACITY = 2000; // 2000:data capacity
    std::unique_lock <std::mutex> lock(mLock_);
    metaData_ = std::make_shared<CameraMetadata>(ENTRY_CAPACITY, DATA_CAPACITY);
    RetCode result = GetFaceDetectMetaData(metaData_);
    if (result  != RC_OK) {
        CAMERA_LOGE("GetFaceDetectMetaData failed\n");
        return RC_ERROR;
    }
    return RC_OK;
}

REGISTERNODE(RKFaceNode, {"RKFace"})
} // namespace OHOS::Camera
