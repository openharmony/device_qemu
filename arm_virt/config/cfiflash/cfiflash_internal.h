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
#ifndef __CFIFLASH_INTERNAL_H__
#define __CFIFLASH_INTERNAL_H__

#include "cfiflash.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define HDF_LOG_TAG cfi_flash_driver

#define CFIFLASH_ONE_BANK_BITS          25

#define CFIFLASH_SEC_SIZE               512
#define CFIFLASH_SEC_SIZE_BITS          9
#define CFIFLASH_SECTORS                (CFIFLASH_CAPACITY / CFIFLASH_SEC_SIZE)

#define CFIFLASH_PAGE_SIZE              (2048 * 2)          /* fit QEMU of 2 banks */
#define CFIFLASH_PAGE_WORDS             (CFIFLASH_PAGE_SIZE / sizeof(uint32_t))
#define CFIFLASH_PAGE_WORDS_MASK        (CFIFLASH_PAGE_WORDS - 1)

#define CFIFLASH_QUERY_CMD              0x98
#define CFIFLASH_QUERY_BASE             0x55
#define CFIFLASH_QUERY_QRY              0x10
#define CFIFLASH_QUERY_VENDOR           0x13
#define CFIFLASH_QUERY_SIZE             0x27
#define CFIFLASH_QUERY_PAGE_BITS        0x2A
#define CFIFLASH_QUERY_ERASE_REGION     0x2C
#define CFIFLASH_QUERY_BLOCKS           0x2D
#define CFIFLASH_QUERY_BLOCK_SIZE       0x2F
#define CFIFLASH_EXPECT_VENDOR          1       /* Intel command set */
#define CFIFLASH_EXPECT_PAGE_BITS       11
#define CFIFLASH_EXPECT_BLOCKS          255     /* plus 1: # of blocks */
#define CFIFLASH_EXPECT_BLOCK_SIZE      512     /* times 256: block size */
#define CFIFLASH_EXPECT_ERASE_REGION    1

#define CFIFLASH_CMD_ERASE              0x20
#define CFIFLASH_CMD_CLEAR_STATUS       0x50
#define CFIFLASH_CMD_READ_STATUS        0x70
#define CFIFLASH_CMD_CONFIRM            0xD0
#define CFIFLASH_CMD_BUFWRITE           0xE8
#define CFIFLASH_CMD_RESET              0xFF

#define CFIFLASH_STATUS_READY_MASK      0x80

extern volatile uint32_t *g_cfiFlashBase;

int CfiFlashInit(void);
ssize_t CfiBlkRead(struct Vnode *vnode, unsigned char *buffer,
                   unsigned long long start_sector, unsigned int nsectors);
ssize_t CfiBlkWrite(struct Vnode *vnode, const unsigned char *buffer,
                    unsigned long long start_sector, unsigned int nsectors);
int CfiBlkGeometry(struct Vnode *vnode, struct geometry *geometry);

int CfiMtdErase(struct MtdDev *mtd, UINT64 start, UINT64 bytes, UINT64 *failAddr);
int CfiMtdRead(struct MtdDev *mtd, UINT64 start, UINT64 bytes, const char *buf);
int CfiMtdWrite(struct MtdDev *mtd, UINT64 start, UINT64 bytes, const char *buf);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
