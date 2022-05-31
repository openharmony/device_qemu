/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
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
#ifndef __CFIFLASH_H__
#define __CFIFLASH_H__

#include "stdint.h"

#define CFIFLASH_MAX_NUM            2
#define CFIFLASH_CAPACITY           (32 * 1024 * 1024)
#define CFIFLASH_ERASEBLK_SIZE      (128 * 1024 * 2)    /* fit QEMU of 2 banks  */
#define CFIFLASH_ERASEBLK_WORDS     (CFIFLASH_ERASEBLK_SIZE / sizeof(uint32_t))
#define CFIFLASH_ERASEBLK_WORDMASK  (~(CFIFLASH_ERASEBLK_WORDS - 1))

/* Results of Flash Functions */
typedef enum {
    FLASH_OK = 0,     /* 0: Successful */
    FLASH_ERROR       /* 1: R/W Error */
} FLASH_DRESULT;

unsigned CfiFlashSec2Bytes(unsigned sector);

int CfiFlashQuery(uint32_t pdrv);
int32_t CfiFlashRead(uint32_t pdrv, uint32_t *buffer, uint32_t offset, uint32_t nbytes);
int32_t CfiFlashWrite(uint32_t pdrv, const uint32_t *buffer, uint32_t offset, uint32_t nbytes);
int32_t CfiFlashErase(uint32_t pdrv, uint32_t offset);

#if (LOSCFG_SUPPORT_FATFS == 1)
#include "fatfs.h"
#include "ff_gen_drv.h"

DiskioDrvTypeDef *GetCfiBlkOps(void);
#endif /* LOSCFG_SUPPORT_FATFS == 1 */

#if (LOSCFG_SUPPORT_LITTLEFS == 1)
#include "lfs.h"

int LittlefsDriverInit(void);
struct lfs_config* GetCfiLfsCfg(void);
#endif /* LOSCFG_SUPPORT_LITTLEFS == 1 */

#endif /* __CFIFLASH_H__ */
