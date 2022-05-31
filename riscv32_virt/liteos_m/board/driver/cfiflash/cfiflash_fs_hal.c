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

#include "cfiflash_internal.h"

#if (LOSCFG_SUPPORT_FATFS == 1)
static DSTATUS DiskInit(BYTE pdrv)
{
    if (CfiFlashQuery(pdrv)) {
        return RES_ERROR;
    }

    g_diskDrv.initialized[pdrv] = 1;
    return RES_OK;
}

static DSTATUS DiskStatus(BYTE pdrv)
{
    if (g_diskDrv.initialized[pdrv] != 1) {
        return RES_ERROR;
    }

    return RES_OK;
}

static DSTATUS DisckRead(BYTE pdrv, BYTE *buffer, DWORD startSector, UINT nSectors)
{
    unsigned int bytes = CfiFlashSec2Bytes(nSectors);
    unsigned int byteOffset = CfiFlashSec2Bytes(startSector);

    uint32_t *p = (uint32_t *)buffer;
    return CfiFlashRead(pdrv, p, byteOffset, bytes);
}

static DSTATUS DiskWrite(BYTE pdrv, const BYTE *buffer, DWORD startSector, UINT nSectors)
{
    unsigned int bytes = CfiFlashSec2Bytes(nSectors);
    unsigned int byteOffset = CfiFlashSec2Bytes(startSector);

    uint32_t *p = (uint32_t *)buffer;
    return CfiFlashWrite(pdrv, p, byteOffset, bytes);
}

static DSTATUS DiskIoctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (g_diskDrv.initialized[pdrv] != 1) {
        return RES_ERROR;
    }

    switch (cmd) {
        case CTRL_SYNC:
            break;
        case GET_SECTOR_COUNT:
            *(DWORD *)buff = CFIFLASH_SECTORS;
            break;
        case GET_SECTOR_SIZE:
            *(WORD *)buff = CFIFLASH_SEC_SIZE;
            break;
        case GET_BLOCK_SIZE:
            *(WORD *)buff = CFIFLASH_EXPECT_ERASE_REGION;
            break;
        default:
            return RES_PARERR;
    }

    return RES_OK;
}

static DiskioDrvTypeDef g_cfiBlkops = {
    DiskInit,
    DiskStatus,
    DisckRead,
    DiskWrite,
    DiskIoctl,
};

DiskioDrvTypeDef *GetCfiBlkOps(void)
{
    return &g_cfiBlkops;
}
#endif /* LOSCFG_SUPPORT_FATFS == 1 */

#if (LOSCFG_SUPPORT_LITTLEFS == 1)
static uint32_t g_flashDevId = 1;

#define READ_SIZE      256
#define PROG_SIZE      256
#define BLOCK_SIZE     CFIFLASH_ERASEBLK_SIZE
#define BLOCK_COUNT    CFIFLASH_EXPECT_BLOCKS + 1
#define CACHE_SIZE     256
#define LOOKAHEAD_SIZE 16
#define BLOCK_CYCLES   1000

static int LittlefsRead(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t addr = cfg->block_size * block + off;
    uint32_t devid = *((uint32_t *)cfg->context);

    uint32_t *p = (uint32_t *)buffer;
    return CfiFlashRead(devid, p, addr, size);
}

static int LittlefsProg(const struct lfs_config *cfg, lfs_block_t block,
                        lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t addr = cfg->block_size * block + off;
    uint32_t devid = *((uint32_t *)cfg->context);

    uint32_t *p = (uint32_t *)buffer;
    return CfiFlashWrite(devid, p, addr, size);
}

static int LittlefsErase(const struct lfs_config *cfg, lfs_block_t block)
{
    uint32_t addr = cfg->block_size * block;
    uint32_t devid = *((uint32_t *)cfg->context);
    return CfiFlashErase(devid, addr);
}

static int LittlefsSync(const struct lfs_config *cfg)
{
    return LFS_ERR_OK;
}

static struct lfs_config g_lfsConfig = {
    // block device operations
    .context = &g_flashDevId,
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

int LittlefsDriverInit(void)
{
    uint32_t devid = *((uint32_t *)g_lfsConfig.context);
    return CfiFlashQuery(devid);
}

struct lfs_config* GetCfiLfsCfg(void)
{
    return &g_lfsConfig;
}
#endif /* LOSCFG_SUPPORT_LITTLEFS == 1 */
