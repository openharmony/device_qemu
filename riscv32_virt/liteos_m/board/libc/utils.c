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

#include <sys/stat.h>
#include "stdlib.h"
#include "ohos_mem_pool.h"
#include "hiview_log.h"
#include "los_debug.h"
#if (LOSCFG_SUPPORT_FATFS == 1)
#include "fatfs.h"
#endif /* LOSCFG_SUPPORT_FATFS == 1 */
#if (LOSCFG_SUPPORT_LITTLEFS == 1)
#include "lfs.h"
#endif /* LOSCFG_SUPPORT_LITTLEFS == 1 */

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

#if (LOSCFG_SUPPORT_FATFS == 1)
void SetupDefaultVolToPartTable(void)
{
    int i = 0;
    PARTITION defaultVToP[FF_VOLUMES] = {
        {0, 1, 1},     /* "0:" ==> Physical drive 1, 1st partition */
        {1, 1, 2},     /* "1:" ==> Physical drive 1, 2nd partition */
        {2, 1, 3},     /* "2:" ==> Physical drive 1, 3rd partition */
        {3, 1, 4}      /* "3:" ==> Physical drive 1, 4th partition */
    };

    for (i = 0; i < FF_VOLUMES; i++) {
        SetupVolToPartTable(i, defaultVToP[i], GetCfiBlkOps());
    }
}
#endif /* LOSCFG_SUPPORT_FATFS == 1 */

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

#if (LOSCFG_SUPPORT_LITTLEFS == 1)
    if (LittlefsDriverInit(1) == 0) {
        struct lfs_config *littlefsConfig = GetCfiLfsCfg();
        ret = mount(NULL, "/data", "littlefs", 0, littlefsConfig);
        if (ret != LOS_OK) {
            PRINT_ERR("Mount Littlefs error:0x%x\n", ret);
        }
    }
#elif (LOSCFG_SUPPORT_FATFS == 1)
    SetupDefaultVolToPartTable();
    /* mount fs in fat.img，for font.ttf used by graphic */
    if (ret = mount("p0", "/data", "vfat", 0, NULL)) {
        PRINT_ERR("Mount Fatfs error:0x%x\n", ret);
    }
#endif /* LOSCFG_SUPPORT_FATFS == 1 */

    /* bootstrap init */
    OHOS_SystemInit();

    /* register hilog output func for mini */
    HiviewRegisterHilogProc(HilogProc_Impl);
}

/**
 * @brief implement for ohos_mem_pool.h
 */
void *OhosMalloc(MemType type, uint32 size)
{
    if (size == 0) {
        return NULL;
    }
    return malloc(size);
}

void OhosFree(void *ptr)
{
    free(ptr);
}

/* implementation for js_app_host.h: LP_TaskBegin */
void LP_TaskBegin(void)
{
}

/* implementation for js_app_host.h: LP_TaskEnd */
void LP_TaskEnd(void)
{
}

/**
 * @brief adapter for js_ability.cpp
 * #ifdef OHOS_ACELITE_PRODUCT_WATCH
 */
void RestoreSystemWrapper(const char *crashMessage)
{
    printf("%s\n", crashMessage);
}

int access(const char *pathname, int mode)
{
    struct stat f_info;

    if (stat(pathname, &f_info) == 0) {
        if (f_info.st_mode & S_IFDIR) {
            return 0;
        } else if (f_info.st_mode & S_IFREG) {
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }

    return 0;
}
