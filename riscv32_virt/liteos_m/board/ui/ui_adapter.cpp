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
#include "input_dev.h"
#include "cmsis_os2.h"
#include "common/graphic_startup.h"
#include "common/image_decode_ability.h"
#include "common/input_device_manager.h"
#include "common/task_manager.h"
#include "engines/gfx/gfx_engine_manager.h"
#include "font/ui_font.h"
#include "font/ui_font_header.h"
#include "font/ui_font_vector.h"
#include "gfx_utils/graphic_log.h"
#include "graphic_config.h"
#include "hal_tick.h"
#include "ui_adapter.h"

#define ENABLE_FPS
#ifdef ENABLE_ACE
#include "product_adapter.h"
#endif

#define FONT_MEM_LEN (512 * 1024)

static uint32_t g_fontMemBaseAddr[OHOS::MIN_FONT_PSRAM_LENGTH / 4];

using namespace OHOS;

static void InitFontEngine(void)
{
    GraphicStartUp::InitFontEngine(
        reinterpret_cast<uintptr_t>(g_fontMemBaseAddr),
        MIN_FONT_PSRAM_LENGTH,
        VECTOR_FONT_DIR,
        DEFAULT_VECTOR_FONT_FILENAME);
}

static void InitImageDecodeAbility()
{
    uint32_t imageType = IMG_SUPPORT_BITMAP | OHOS::IMG_SUPPORT_JPEG | OHOS::IMG_SUPPORT_PNG;
    ImageDecodeAbility::GetInstance().SetImageDecodeAbility(imageType);
}

static void InitHal(void)
{
    DispDev *display = DispDev::GetInstance();
    BaseGfxEngine::InitGfxEngine(display);

    InputDev *input = InputDev::GetInstance();
    InputDeviceManager::GetInstance()->Add(input);
}

void UiAdapterInit(void)
{
    GraphicStartUp::Init();
    InitHal();
    InitFontEngine();
    InitImageDecodeAbility();
}

__attribute__((weak)) void RunApp(void)
{
    GRAPHIC_LOGI("RunApp default");
}

#ifdef ENABLE_ACE
static void RenderTEHandler()
{
}
#endif

static void UiAdapterTask(void *arg)
{
    (void)arg;

    static constexpr uint32_t UI_TASK_DELAY = 100; // 1 sec delay for services init finish
    osDelay(UI_TASK_DELAY);
    
    UiAdapterInit();
    RunApp();

#ifdef ENABLE_ACE
    const ACELite::TEHandlingHooks hooks = {RenderTEHandler, nullptr};
    ACELite::ProductAdapter::RegTEHandlers(hooks);
#endif
#ifdef ENABLE_FPS
    uint32_t cnt = 0;
    uint32_t start = HALTick::GetInstance().GetTime();
#endif

    while (1) {
#ifdef ENABLE_ACE
        // Here render all js app in the same task.
        ACELite::ProductAdapter::DispatchTEMessage();
#endif
        DispDev::GetInstance()->UpdateFBBuffer();
        uint32_t temp = HALTick::GetInstance().GetTime();
        TaskManager* inst = TaskManager::GetInstance();
        inst->TaskHandler();
        uint32_t time = HALTick::GetInstance().GetElapseTime(temp);
        if (time < DEFAULT_TASK_PERIOD) {
            osDelay(DEFAULT_TASK_PERIOD - time);
        }
#ifdef ENABLE_FPS
        const int TICKS_OF_SEC = 1000;
        cnt++;
        time = HALTick::GetInstance().GetElapseTime(start);
        if (time >= TICKS_OF_SEC && time != 0) {
            start = HALTick::GetInstance().GetTime();
            cnt = 0;
        }
#endif
    }
}

void UiAdapterRun(void)
{
    const int UI_TASK_SIZE = (32 * 1024);

    osThreadAttr_t attr = {0};
    attr.stack_size = UI_TASK_SIZE;
    attr.priority = osPriorityNormal;
    attr.name = "UiAdapterThread";
    if (osThreadNew((osThreadFunc_t)UiAdapterTask, NULL, &attr) == NULL) {
        GRAPHIC_LOGE("Failed to create UiAdapterTask");
    }
}
