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

#include "fs/driver.h"
#include "mtd_dev.h"

#define CFI_DRIVER                  "/dev/cfiflash"
#define CFI_BLK_DRIVER              "/dev/cfiblk"
#define FLASH_TYPE                  "cfi-flash"

#define CFIFLASH_CAPACITY           (64 * 1024 * 1024)
#define CFIFLASH_ERASEBLK_SIZE      (128 * 1024 * 2)    /* fit QEMU of 2 banks  */
#define CFIFLASH_ERASEBLK_WORDS     (CFIFLASH_ERASEBLK_SIZE / sizeof(uint32_t))
#define CFIFLASH_ERASEBLK_WORDMASK  (~(CFIFLASH_ERASEBLK_WORDS - 1))

#define CFIFLASH_ROOT_ADDR          (10 * 1024 * 1024)
#define CFIFLASH_BOOTARGS_ADDR      (CFIFLASH_ROOT_ADDR - CFIFLASH_ERASEBLK_SIZE)

struct block_operations *GetCfiBlkOps(void);
struct MtdDev *GetCfiMtdDev(void);

#endif
