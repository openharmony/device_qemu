/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "hiview_log.h"
#include "los_debug.h"

/**
 * @brief implement for hilog_lite featured/mini
 * @notice hilog_lite mini converts '%s' to '%p'
 */
boolean HilogProc_Impl(const HiLogContent *hilogContent, uint32 len)
{
    char tempOutStr[LOG_FMT_MAX_LEN] = {0};
    if (LogContentFmt(tempOutStr, sizeof(tempOutStr), hilogContent) > 0) {
        printf(tempOutStr);
    }
    return TRUE;
}

void _init(void) {}
void _fini(void) {}
extern void __libc_fini_array (void);

void SystemAdapterInit(void)
{
    UINT32 ret = 0;

    /* _start in crt0.S have been replaced by los_start.S in riscv32_virt,
     * so call this functions here for initialization and destruction of c++ global var
     */
    if (atexit(__libc_fini_array) != 0) {
        PRINT_ERR("register exit proc error!\n");
    };
    __libc_init_array();

    if (!DiscDrvInit()) {
        /* mount fs in fs_storage.img，for font.ttf used by graphic */
        if (ret = mount("p0", "/data", "vfat", 0, NULL)) {
            PRINT_ERR("Mount error:%d\n", ret);
        }
    } else {
        PRINT_ERR("SetupDiscDrv error\n");
    }

    /* bootstrap init */
    OHOS_SystemInit();

    /* register hilog output func for mini */
    HiviewRegisterHilogProc(HilogProc_Impl);
}