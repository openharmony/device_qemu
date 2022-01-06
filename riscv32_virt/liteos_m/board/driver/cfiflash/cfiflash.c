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
#include "ff_gen_drv.h"
#include "diskio.h"

#define BIT_SHIFT8      8
#define BYTE_WORD_SHIFT 2

static uint32_t g_cfiDrvBase[CFIFLASH_MAX_NUM];

static uint8_t *GetCfiDrvPriv(BYTE pdrv) 
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

static DRESULT SetCfiDrvPriv(BYTE pdrv, uint32_t priv) 
{
    g_cfiDrvBase[pdrv] = priv;
    return RES_OK;
}

static inline unsigned CfiFlashSec2Bytes(unsigned sector)
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

static inline DRESULT CfiFlashQueryQRY(uint8_t *p)
{
    unsigned wordOffset = CFIFLASH_QUERY_QRY;

    if (p[W2B(wordOffset++)] == 'Q') {
        if (p[W2B(wordOffset++)] == 'R') {
            if (p[W2B(wordOffset)] == 'Y') {
                return RES_OK;
            }
        }
    }
    return RES_ERROR;
}

static inline DRESULT CfiFlashQueryUint8(unsigned wordOffset, uint8_t expect, uint8_t *p)
{
    if (p[W2B(wordOffset)] != expect) {
        PRINT_ERR("[%s]name:0x%x value:%u expect:%u \n", __func__, wordOffset, p[W2B(wordOffset)], expect);
        return RES_ERROR;
    }
    return RES_OK;
}

static inline DRESULT CfiFlashQueryUint16(unsigned wordOffset, uint16_t expect, uint8_t *p)
{
    uint16_t v;

    v = (p[W2B(wordOffset + 1)] << BIT_SHIFT8) + p[W2B(wordOffset)];
    if (v != expect) {
        PRINT_ERR("[%s]name:0x%x value:%u expect:%u \n", __func__, wordOffset, v, expect);
        return RES_ERROR;
    }
    return RES_OK;
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

static DSTATUS CfiFlashQuery(uint8_t *p)
{
    uint32_t *base = (uint32_t *)p;
    base[CFIFLASH_QUERY_BASE] = CFIFLASH_QUERY_CMD;

    dsb();
    if (CfiFlashQueryQRY(p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : not found QRY\n", __func__, __LINE__);
        return RES_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_VENDOR, CFIFLASH_EXPECT_VENDOR, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected VENDOR\n", __func__, __LINE__);
        return RES_ERROR;
    }

    if (CfiFlashQueryUint8(CFIFLASH_QUERY_SIZE, CFIFLASH_ONE_BANK_BITS, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected BANK_BITS\n", __func__, __LINE__);
        return RES_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_PAGE_BITS, CFIFLASH_EXPECT_PAGE_BITS, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected PAGE_BITS\n", __func__, __LINE__);
        return RES_ERROR;
    }

    if (CfiFlashQueryUint8(CFIFLASH_QUERY_ERASE_REGION, CFIFLASH_EXPECT_ERASE_REGION, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected ERASE_REGION\n", __func__, __LINE__);
        return RES_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_BLOCKS, CFIFLASH_EXPECT_BLOCKS, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected BLOCKS\n", __func__, __LINE__);
        return RES_ERROR;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_BLOCK_SIZE, CFIFLASH_EXPECT_BLOCK_SIZE, p)) {
        PRINT_ERR("[%s: %d]not supported CFI flash : unexpected BLOCK_SIZE\n", __func__, __LINE__);
        return RES_ERROR;
    }

    base[0] = CFIFLASH_CMD_RESET;
    return RES_OK;
}

DRESULT CfiFlashInit(BYTE pdrv, uint32_t priv)
{
    return SetCfiDrvPriv(pdrv, priv);
}

DSTATUS DiskInit(BYTE pdrv)
{
    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
        return RES_ERROR;
    }

    if (CfiFlashQuery(pbase)) {
        return RES_ERROR;
    }

    g_diskDrv.initialized[pdrv] = 1;
    return RES_OK;
}

DSTATUS DiskStatus(BYTE pdrv)
{
    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
        return RES_ERROR;
    }

    return RES_OK;
}

DSTATUS DisckRead(BYTE pdrv, BYTE *buffer, DWORD startSector, UINT nSectors)
{
    unsigned int i = 0;

    uint32_t *p = (uint32_t *)buffer;

    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
        return RES_ERROR;
    }

    unsigned int bytes = CfiFlashSec2Bytes(nSectors);
    unsigned int wordOffset = B2W(CfiFlashSec2Bytes(startSector));

    uint32_t *base = (uint32_t *)pbase;
    for (i = 0; i < B2W(bytes); i++) {
        p[i] = base[wordOffset + i];
    }

    return RES_OK;
}

DSTATUS DiskWrite(BYTE pdrv, const BYTE *buffer, DWORD startSector, UINT nSectors)
{
    uint32_t *p = (uint32_t *)buffer;

    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
        return RES_ERROR;
    }

    unsigned int bytes = CfiFlashSec2Bytes(nSectors);
    unsigned int wordOffset = B2W(CfiFlashSec2Bytes(startSector));

    uint32_t *base = (uint32_t *)pbase;
    CfiFlashWriteBuf(wordOffset, (uint32_t *)p, B2W(bytes), base);

    return RES_OK;
}

DSTATUS DiskIoctl(BYTE pdrv, BYTE cmd, void *buff)
{
    uint8_t *pbase = GetCfiDrvPriv(pdrv);
    if (pbase == NULL) {
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
