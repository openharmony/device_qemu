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

#include <cstdio>
#include "gfx_utils/graphic_log.h"
#include "graphic_config.h"
#include "input_dev.h"

namespace OHOS {
constexpr const uint8_t INPUT_DEV_ID = 2;
constexpr const uint32_t MESSAGE_QUEUE_SIZE = 8;

InputDev *InputDev::GetInstance()
{
    static InputDev instance;
    if (!instance.init) {
        uint32_t ret = GetInputInterface(&instance.inputInterface);
        if (ret != INPUT_SUCCESS) {
            GRAPHIC_LOGE("Get input interfaces failed");
            return nullptr;
        }

        ret = instance.inputInterface->iInputManager->OpenInputDevice(INPUT_DEV_ID);
        if (ret != INPUT_SUCCESS) {
            GRAPHIC_LOGE("Open input device failed");
            return nullptr;
        }

        instance.inputEventCb.EventPkgCallback = ReportEventPkgCallback;
        ret = instance.inputInterface->iInputReporter->RegisterReportCallback(INPUT_DEV_ID, &instance.inputEventCb);
        if (ret != INPUT_SUCCESS) {
            GRAPHIC_LOGE("Register input device report callback failed");
            return nullptr;
        }

        instance.init = true;
    }
    return &instance;
}

void InputDev::ReportEventPkgCallback(const InputEventPackage **pkgs, uint32_t count, uint32_t devIndex)
{
    if (pkgs == NULL) {
        return;
    }

    DeviceData data = {0};
    GetInstance()->GetEventData(&data);

    for (uint32_t i = 0; i < count; i++) {
        if (pkgs[i]->type == EV_REL) {
            if (pkgs[i]->code == REL_X)
                data.point.x += pkgs[i]->value;
            else if (pkgs[i]->code == REL_Y)
                data.point.y += pkgs[i]->value;
        } else if (pkgs[i]->type == EV_ABS) {
            if (pkgs[i]->code == ABS_X)
                data.point.x =pkgs[i]->value / (0x7fff / HORIZONTAL_RESOLUTION);
            else if (pkgs[i]->code == ABS_Y)
                data.point.y =pkgs[i]->value / (0x7fff / VERTICAL_RESOLUTION);
        } else if (pkgs[i]->type == EV_KEY) {
            if (pkgs[i]->code == BTN_MOUSE || pkgs[i]->code == BTN_TOUCH) {
                if (pkgs[i]->value == 0)
                    data.state = 0;
                else if (pkgs[i]->value == 1)
                    data.state = 1;
            }
        } else if (pkgs[i]->type == EV_SYN) {
            if (pkgs[i]->code == SYN_REPORT) {
                break;
            }
        }
    }

    GetInstance()->SetEventData(data);
}

bool InputDev::Read(DeviceData &data)
{
    GetEventData(&data);
    return false;
}
} // namespace OHOS