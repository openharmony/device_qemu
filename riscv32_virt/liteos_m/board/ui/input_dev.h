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

#ifndef INPUT_DEV_H
#define INPUT_DEV_H

#include "dock/pointer_input_device.h"
#include "input_manager.h"
#include "cmsis_os2.h"
#include "graphic_locker.h"

namespace OHOS {
class InputDev : public PointerInputDevice {
public:
    InputDev() {}
    virtual ~InputDev() {}
    static InputDev* GetInstance();
    bool Read(DeviceData& data) override;

    void GetEventData(DeviceData* data)
    {
        GraphicLocker lock(eventLock_);
        if (data != nullptr) {
            *data = deviceData_;
        }
    }

    void SetEventData(DeviceData data)
    {
        GraphicLocker lock(eventLock_);
        deviceData_.point.x = data.point.x;
        deviceData_.point.y = data.point.y;
        deviceData_.state = data.state;
    }

private:
    static void ReportEventPkgCallback(const EventPackage **pkgs, uint32_t count, uint32_t devIndex);
    bool init = false;
    IInputInterface *inputInterface = nullptr;
    InputEventCb inputEventCb = {0};
    DeviceData deviceData_;
    pthread_mutex_t eventLock_;
};
} // namespace OHOS
#endif // INPUT_DEV_H