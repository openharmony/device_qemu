/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd. All rights reserved.
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

#include "los_config.h"
#include "ram_virt_flash.h"

#if (LOSCFG_SUPPORT_LITTLEFS == 1)
#include "fs_lowlevel.h"

struct fs_cfg {
    CHAR *mount_point;
    struct lfs_config lfs_cfg;
};

STATIC struct fs_cfg fs[LOSCFG_LFS_MAX_MOUNT_SIZE] = {0};

INT32 LfsLowLevelInit()
{
    INT32 ret;

    fs[0].mount_point = "/littlefs";
    fs[0].lfs_cfg.block_size = 4096; /* 4096, lfs block size */
    fs[0].lfs_cfg.block_count = 2048; /* 2048, lfs block count */
    fs[0].lfs_cfg.context = FLASH_PARTITION_DATA0;
    fs[0].lfs_cfg.read = littlefs_block_read;
    fs[0].lfs_cfg.prog = littlefs_block_write;
    fs[0].lfs_cfg.erase = littlefs_block_erase;
    fs[0].lfs_cfg.sync = littlefs_block_sync;

    fs[0].lfs_cfg.read_size = 256; /* 256, lfs read size */
    fs[0].lfs_cfg.prog_size = 256; /* 256, lfs prog size */
    fs[0].lfs_cfg.cache_size = 256; /* 256, lfs cache size */
    fs[0].lfs_cfg.lookahead_size = 16; /* 16, lfs lookahead size */
    fs[0].lfs_cfg.block_cycles = 1000; /* 1000, lfs block cycles */

    ret = mount(NULL, fs[0].mount_point, "littlefs", 0, &fs[0].lfs_cfg);
    printf("%s: mount fs on '%s' %s\n", __func__, fs[0].mount_point, (ret == 0) ? "succeed" : "failed");
    if (ret != 0) {
        return -1;
    }
    ret = mkdir(fs[0].mount_point);
    printf("%s: mkdir '%s' %s\n", __func__, fs[0].mount_point, (ret == 0) ? "succeed" : "failed");
    if (ret != 0) {
        return -1;
    }
    return 0;
}
#endif
