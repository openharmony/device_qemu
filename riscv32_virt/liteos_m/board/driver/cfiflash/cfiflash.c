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
/*
 * simple CFI flash driver for QEMU riscv 'virt' machine, with:
 *
 * 32M = 1 region * 128 Erase Blocks * 256K(2 banks: 64 pages * 4096B)
 * 32 bits, Intel command set
 */

#include "stdint.h"
#include "stddef.h"
#include "los_arch_context.h"
#include "los_debug.h"
#include "los_compiler.h"
#include "cfiflash.h"
#include "cfiflash_internal.h"

#define BIT_SHIFT8      8
#define BYTE_WORD_SHIFT 2

static uint32_t g_cfiDrvBase[CFIFLASH_MAX_NUM];

static uint8_t *GetCfiDrvPriv(uint32_t pdrv)
{
    uint8_t *ret = NULL;
    if (pdrv >= 0 && pdrv < CFIFLASH_MAX_NUM) {
        ret = (uint8_t *)g_cfiDrvBase[pdrv];
    }

    if (ret == NULL) {
        PRINT_ERR("Get CfiFlash driver base failed!\n");
    }

    return ret;
}

static int SetCfiDrvPriv(uint32_t pdrv, uint32_t priv)
{
    g_cfiDrvBase[pdrv] = priv;
    return FLASH_OK;
}

unsigned CfiFlashSec2Bytes(unsigned sector)
{
    return sector << CFIFLASH_SEC_SIZE_BITS;
}

static inline unsigned CfiFlashPageWordOffset(unsigned wordOffset)
{
    return wordOffset & CFIFLASH_PAGE_WORDS_MASK;
}

static inline unsigned CfiFlashEraseBlkWordAddr(unsigned wordOffset)
{
    return wordOffset & CFIFLASH_ERASEBLK_WORDMASK;
}

static inline unsigned W2B(unsigned words)
{
    return words << BYTE_WORD_SHIFT;
}

static inline unsigned B2W(unsigned bytes)
{
    return bytes >> BYTE_WORD_SHIFT;
}

static inline int CfiFlashQueryQRY(uint8_t *p)
{
    unsigned wordOffset = CFIFLASH_QUERY_QRY;

    if (p[W2B(wordOffset++)] == 'Q') {
        if (p[W2B(wordOffset++)] == 'R') {
            if (p[W2B(wordOffset)] == 'Y') {
                return FLASH_OK;
            }
        }
    }
    return FLASH_ERROR;
}

static inline int CfiFlashQueryUint8(unsigned wordOffset, uint8_t expect, uint8_t *p)
{
    if (p[W2B(wordOffset)] != expect) {
        PRINT_ERR("[%s]name:0x%x value:%u expect:%u \n", __func__, wordOffset, p[W2B(wordOffset)], expect);
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

static inline int CfiFlashQueryUint16(unsigned wordOffset, uint16_t expect, uint8_t *p)
{
    uint16_t v;

    v = (p[W2B(wordOffset + 1)] << BIT_SHIFT8) + p[W2B(wordOffset)];
    if (v != expect) {
        PRINT_ERR("[%s]name:0x%x value:%u expect:%u \n", __func__, wordOffset, v, expect);
        return FLASH_ERROR;
    }
    return FLASH_OK;
}

static inline int CfiFlashIsReady(unsigned wordOffset, uint32_t *p)
{
    dsb();
    p[wordOffset] = CFIFLASH_CMD_READ_STATUS;
    dsb();
    return p[wordOffset] & CFIFLASH_STATUS_READY_MASK;
}

/* all in word(4 bytes) measure */
static void CfiFlashWriteBuf(unsigned wordOffset, const uint32_t *buffer, size_t words, uint32_t *p)
{
    unsigned blkAddr = 0;

    /* first write might not be Page aligned */
    unsigned i = CFIFLASH_PAGE_WORDS - CfiFlashPageWordOffset(wordOffset);
    unsigned wordCount = (i > words) ? words : i;

    while (words) {
        /* command buffer-write begin to Erase Block address */
        blkAddr = CfiFlashEraseBlkWordAddr(wordOffset);
        p[blkAddr] = CFIFLASH_CMD_BUFWRITE;

        /* write words count, 0-based */
        dsb();
        p[blkAddr] = wordCount - 1;

        /* program word data to actual address */
        for (i = 0; i < wordCount; i++, wordOffset++, buffer++) {
            p[wordOffset] = *buffer;
        }

        /* command buffer-write end to Erase Block address */
        p[blkAddr] = CFIFLASH_CMD_CONFIRM;
        while (!CfiFlashIsReady(blkAddr, p)) { }

        words -= wordCount;
        wordCount = (words >= CFIFLASH_PAGE_WORDS) ? CFIFLASH_PAGE_WORDS : words;
    }

    p[0] = CFIFLASH_CMD_CLEAR_STATUS;
}

int CfiFlashQuery(uint32_t pdrv)
{
    uint8_t *p = GetCfiDrvPriv(pdrv);
    if (p == NULL) {
        return FLASH_ERROR;
    }
    uint32_t *base = (uint32_t *)p;
    base[CFIFLASH_QUERY_BASE] = CFIFLASH_QUERY_CMD;

    dsb();
    if (CfiFlashQueryQRY(p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : not found QRY\n", __func__, __LINE__);
        return FLASH_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_VENDOR, CFIFLASH_EXPECT_VENDOR, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected VENDOR\n", __func__, __LINE__);
        return FLASH_ERROR;
    }

    if (CfiFlashQueryUint8(CFIFLASH_QUERY_SIZE, CFIFLASH_ONE_BANK_BITS, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected BANK_BITS\n", __func__, __LINE__);
        return FLASH_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_PAGE_BITS, CFIFLASH_EXPECT_PAGE_BITS, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected PAGE_BITS\n", __func__, __LINE__);
        return FLASH_ERROR;
    }

    if (CfiFlashQueryUint8(CFIFLASH_QUERY_ERASE_REGION, CFIFLASH_EXPECT_ERASE_REGION, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected ERASE_REGION\n", __func__, __LINE__);
        return FLASH_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_BLOCKS, CFIFLASH_EXPECT_BLOCKS, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected BLOCKS\n", __func__, __LINE__);
        return FLASH_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_BLOCK_SIZE, CFIFLASH_EXPECT_BLOCK_SIZE, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected BLOCK_SIZE\n", __func__, __LINE__);
        return FLASH_ERROR;
    }

    base[0] = CFIFLASH_CMD_RESET;
    return FLASH_OK;
}

int CfiFlashInit(uint32_t pdrv, uint32_t priv)
{
    return SetCfiDrvPriv(pdrv, priv);
}

int32_t CfiFlashRead(uint32_t pdrv, uint32_t *buffer, uint32_t offset, uint32_t nbytes)
{
    uint32_t i = 0;

    if ((offset + nbytes) > CFIFLASH_CAPACITY) {
        PRINT_ERR("flash over read, offset:%d, nbytes:%d\n", offset, nbytes);
        return FLASH_ERROR;
    }

    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
        return FLASH_ERROR;
    }
    uint32_t *base = (uint32_t *)pbase;

    unsigned int words = B2W(nbytes);
    unsigned int wordOffset = B2W(offset);

    uint32_t intSave = LOS_IntLock();
    for (i = 0; i < words; i++) {
        buffer[i] = base[wordOffset + i];
    }
    LOS_IntRestore(intSave);
    return FLASH_OK;
}

int32_t CfiFlashWrite(uint32_t pdrv, const uint32_t *buffer, uint32_t offset, uint32_t nbytes)
{
    if ((offset + nbytes) > CFIFLASH_CAPACITY) {
        PRINT_ERR("flash over write, offset:%d, nbytes:%d\n", offset, nbytes);
        return FLASH_ERROR;
    }

    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
        return FLASH_ERROR;
    }
    uint32_t *base = (uint32_t *)pbase;

    unsigned int words = B2W(nbytes);
    unsigned int wordOffset = B2W(offset);

    uint32_t intSave = LOS_IntLock();
    CfiFlashWriteBuf(wordOffset, buffer, words, base);
    LOS_IntRestore(intSave);

    return FLASH_OK;
}

int32_t CfiFlashErase(uint32_t pdrv, uint32_t offset)
{
    if (offset > CFIFLASH_CAPACITY) {
        PRINT_ERR("flash over erase, offset:%d\n", offset);
        return FLASH_ERROR;
    }

    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
        return FLASH_ERROR;
    }
    uint32_t *base = (uint32_t *)pbase;

    uint32_t blkAddr = CfiFlashEraseBlkWordAddr(B2W(offset));

    uint32_t intSave = LOS_IntLock();
    base[blkAddr] = CFIFLASH_CMD_ERASE;
    dsb();
    base[blkAddr] = CFIFLASH_CMD_CONFIRM;
    while (!CfiFlashIsReady(blkAddr, base)) { }
    base[0] = CFIFLASH_CMD_CLEAR_STATUS;
    LOS_IntRestore(intSave);

    return FLASH_OK;
}