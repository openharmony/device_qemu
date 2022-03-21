/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <unistd.h>

#include "hdf_log.h"
#include "input_manager.h"
#include "los_compiler.h"
#include "test_demo.h"

#define DEV_INDEX 1

#define INPUT_CHECK_NULL_POINTER(pointer, ret)      \
    do {                                            \
        if ((pointer) == NULL) {                    \
            HDF_LOGE("%s: null pointer", __func__); \
        }                                           \
    } while (0)

IInputInterface *g_inputInterface;
InputEventCb g_callback;

// Mouse Para
#define MOUSEWIDTH  12
#define MOUSEHIGH   18
#define SCREENHIGH  480
#define SCREENWIDTH 800

enum {
    MOUSE_EVENT_TYPE_CLICK_LEFT = 1,
    MOUSE_EVENT_TYPE_CLICK_RIGHT = 2,
    MOUSE_EVENT_TYPE_MOVE = 3,
};

static int mousePosX = 400;
static int mousePosY = 240;
static int clickEventFlag = 0;
extern DisplayTest g_displayTest;

void ShowGreenScreen(void)
{
    int x = 0;
    int y = 0;

    uint16_t *pBuf = (uint16_t *)g_displayTest.buffer.data.virAddr;

    for (y = 0; y < SCREENHIGH; y++) {
        for (x = 0; x < SCREENWIDTH; x++) {
            *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFF00;
        }
    }
}

void ShowBlueButton(void)
{
    int x = 0;
    int y = 0;

    uint16_t *pBuf = (uint16_t *)g_displayTest.buffer.data.virAddr;

    for (y = 100; y < 140; y++) {
        for (x = 200; x < 280; x++) {
            *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFF0000;
        }
    }
}

void ShowClickEvent(void)
{
    int x = 0;
    int y = 0;

    uint16_t *pBuf = (uint16_t *)g_displayTest.buffer.data.virAddr;

    for (y = 200; y < 280; y++) {
        for (x = 400; x < 480; x++) {
            *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFF;
        }
    }
}

void ShowMouse(int mouseX, int mouseY)
{
    int x = 0;
    int y = 0;
    int i = 0;
    int ret = 0;

    uint16_t *pBuf = (uint16_t *)g_displayTest.buffer.data.virAddr;

    ShowGreenScreen();

    ShowBlueButton();

    if (clickEventFlag == 1) {
        ShowClickEvent();
    }

    for (y = mouseY; y < mouseY + MOUSEHIGH; y++) {
        i++;
        if (i < 12) {
            for (x = mouseX; x < mouseX + i; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
        } else if (i == 12) {
            for (x = mouseX; x < mouseX + 6; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
        } else if (i == 13) {
            for (x = mouseX; x < mouseX + 4; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
            for (x = mouseX + 5; x < mouseX + 7; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
        } else if (i == 14) {
            for (x = mouseX; x < mouseX + 2; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
            for (x = mouseX + 5; x < mouseX + 7; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
        } else if (i == 15) {
            for (x = mouseX; x < mouseX + 1; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
            for (x = mouseX + 6; x < mouseX + 8; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
        } else if (i == 16) {
            for (x = mouseX + 6; x < mouseX + 8; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
        } else if (i == 17 || i == 18) {
            for (x = mouseX + 7; x < mouseX + 9; x++) {
                *(((int *)pBuf) + SCREENWIDTH * y + x) = 0xFFFFFF;
            }
        }
    }

    if (g_displayTest.layerFuncs->Flush != NULL) {
        ret = g_displayTest.layerFuncs->Flush(g_displayTest.devId, g_displayTest.layerId, &g_displayTest.buffer);
        if (ret != DISPLAY_SUCCESS) {
            HDF_LOGE("flush layer failed");
            return;
        }
    }
}

static void GetMousePos(uint16_t eventType, uint16_t code, int value)
{
    if (eventType == MOUSE_EVENT_TYPE_MOVE && code == 0) {
        value = value / (0x7fff / SCREENWIDTH);

        mousePosX = value;

        if (mousePosX < 0) {
            mousePosX = 0;
        } else if (mousePosX >= (SCREENWIDTH - MOUSEWIDTH)) {
            mousePosX = SCREENWIDTH - MOUSEWIDTH;
        }
    } else if (eventType == MOUSE_EVENT_TYPE_MOVE && code == 1) {
        value = value / (0x7fff / SCREENHIGH);

        mousePosY = value;

        if (mousePosY < 0) {
            mousePosY = 0;
        } else if (mousePosY >= (SCREENHIGH - MOUSEHIGH)) {
            mousePosY = SCREENHIGH - MOUSEHIGH;
        }
    }
    // left click
    if (eventType == MOUSE_EVENT_TYPE_CLICK_LEFT && code == 272 && value == 0) {
        if ((200 < mousePosX && mousePosX < 280) && (100 < mousePosY && mousePosY < 140)) {
            clickEventFlag = !clickEventFlag;
        }
    }

    if (eventType == MOUSE_EVENT_TYPE_CLICK_LEFT || eventType == MOUSE_EVENT_TYPE_MOVE) {
        ShowMouse(mousePosX, mousePosY);
    }
}

static void ReportEventPkgCallback(const EventPackage **pkgs, uint32_t count)
{
    if (pkgs == NULL) {
        return;
    }
    for (uint32_t i = 0; i < count; i++) {
        GetMousePos(pkgs[i]->type, pkgs[i]->code, pkgs[i]->value);
    }
}

int InputServiceSample(void)
{
    uint32_t devType = INDEV_TYPE_MOUSE;

    /* 获取Input驱动能力接口 */
    int ret = GetInputInterface(&g_inputInterface);
    if (ret != INPUT_SUCCESS) {
        HDF_LOGE("%s: get input interfaces failed, ret = %d", __func__, ret);
        return ret;
    }

    INPUT_CHECK_NULL_POINTER(g_inputInterface, INPUT_NULL_PTR);
    INPUT_CHECK_NULL_POINTER(g_inputInterface->iInputManager, INPUT_NULL_PTR);
    /* 打开特定的input设备 */
    ret = g_inputInterface->iInputManager->OpenInputDevice(DEV_INDEX);
    if (ret) {
        HDF_LOGE("%s: open input device failed, ret = %d", __func__, ret);
        return ret;
    }

    INPUT_CHECK_NULL_POINTER(g_inputInterface->iInputController, INPUT_NULL_PTR);
    /* 获取对应input设备的类型 */
    ret = g_inputInterface->iInputController->GetDeviceType(DEV_INDEX, &devType);

    if (ret) {
        HDF_LOGE("%s: get device type failed, ret: %d", __FUNCTION__, ret);
        return ret;
    }
    HDF_LOGI("%s: device1's type is %u\n", __FUNCTION__, devType);

    /* 给特定的input设备注册数据上报回调函数 */
    g_callback.EventPkgCallback = ReportEventPkgCallback;
    ret = g_inputInterface->iInputReporter->RegisterReportCallback(DEV_INDEX, &g_callback);
    return 0;
}