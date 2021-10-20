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

#include <stdio.h>
#include <string.h>
#include "los_memory.h"
#include "lfs_rambd.h"
#include "lfs.h"

#define LITTLEFS_PHYS_ADDR 0x00
#define LITTLEFS_PHYS_SIZE (64 * 1024)

#define READ_SIZE      16
#define PROG_SIZE      16
#define BLOCK_SIZE     256
#define BLOCK_COUNT    128
#define CACHE_SIZE     16
#define LOOKAHEAD_SIZE 16
#define BLOCK_CYCLES   500

static int LittlefsRead(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, void *buffer, lfs_size_t size)
{
    (void)lfs_rambd_read(cfg, block, off, buffer, size);

    return LFS_ERR_OK;
}

static int LittlefsProg(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    (void)lfs_rambd_prog(cfg, block, off, buffer, size);

    return LFS_ERR_OK;
}

static int LittlefsErase(const struct lfs_config *cfg, lfs_block_t block)
{
    (void)lfs_rambd_erase(cfg, block);

    return LFS_ERR_OK;
}

static int LittlefsSync(const struct lfs_config *cfg)
{
    return LFS_ERR_OK;
}

static struct lfs_config g_lfsConfig = {
    // block device operations
    .context = NULL,
    .read  = LittlefsRead,
    .prog  = LittlefsProg,
    .erase = LittlefsErase,
    .sync  = LittlefsSync,

    // block device configuration
    .read_size = READ_SIZE,
    .prog_size = PROG_SIZE,
    .block_size = BLOCK_SIZE,
    .block_count = BLOCK_COUNT,
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEAD_SIZE,
    .block_cycles = BLOCK_CYCLES,
    .read_buffer = NULL,
    .prog_buffer = NULL,
    .lookahead_buffer = NULL
};

void LittlefsDriverInit(int needErase)
{
    lfs_rambd_t *bd = (lfs_rambd_t *)LOS_MemAlloc(m_aucSysMem0, sizeof(lfs_rambd_t));
    (void)memset_s(bd, sizeof(lfs_rambd_t), 0, sizeof(lfs_rambd_t));
    g_lfsConfig.context = bd;
    (void)lfs_rambd_create(&g_lfsConfig);
}

struct lfs_config* LittlefsConfigGet(void)
{
    return &g_lfsConfig;
}
