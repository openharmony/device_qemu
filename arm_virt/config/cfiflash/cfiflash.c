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
 * simple CFI flash driver for QEMU arm 'virt' machine, with:
 *
 * 64M = 1 region * 256 Erase Blocks * 256K(2 banks: 64 pages * 4096B)
 * 32 bits, Intel command set
 */

#include "user_copy.h"
#include "cfiflash_internal.h"

#define BIT_SHIFT8      8
#define WORD_ALIGN      4
#define BYTE_WORD_SHIFT 2

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

static inline int CfiFlashQueryQRY(uint8_t *p)
{
    unsigned wordOffset = CFIFLASH_QUERY_QRY;

    if (p[W2B(wordOffset++)] == 'Q') {
        if (p[W2B(wordOffset++)] == 'R') {
            if (p[W2B(wordOffset)] == 'Y') {
                return 0;
            }
        }
    }
    return -1;
}

static inline int CfiFlashQueryUint8(unsigned wordOffset, uint8_t expect, uint8_t *p)
{
    if (p[W2B(wordOffset)] != expect) {
        return -1;
    }
    return 0;
}

static inline int CfiFlashQueryUint16(unsigned wordOffset, uint16_t expect, uint8_t *p)
{
    uint16_t v;

    v = (p[W2B(wordOffset + 1)] << BIT_SHIFT8) + p[W2B(wordOffset)];
    if (v != expect) {
        return -1;
    }
    return 0;
}

static inline int CfiFlashIsReady(unsigned wordOffset, uint32_t *p)
{
    DSB;
    p[wordOffset] = CFIFLASH_CMD_READ_STATUS;
    DSB;
    return p[wordOffset] & CFIFLASH_STATUS_READY_MASK;
}

/* all in word(4 bytes) measure */
static void CfiFlashWriteBuf(unsigned wordOffset, const uint32_t *buffer, size_t words, uint32_t *p)
{
    unsigned i, blkAddr, wordCount;

    /* first write might not be Page aligned */
    i = CFIFLASH_PAGE_WORDS - CfiFlashPageWordOffset(wordOffset);
    wordCount = (i > words) ? words : i;

    while (words) {
        /* command buffer-write begin to Erase Block address */
        blkAddr = CfiFlashEraseBlkWordAddr(wordOffset);
        p[blkAddr] = CFIFLASH_CMD_BUFWRITE;

        /* write words count, 0-based */
        DSB;
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

static int CfiFlashQuery(uint8_t *p)
{
    uint32_t *base = (uint32_t *)p;
    base[CFIFLASH_QUERY_BASE] = CFIFLASH_QUERY_CMD;

    if (CfiFlashQueryQRY(p)) {
        goto ERR_OUT;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_VENDOR, CFIFLASH_EXPECT_VENDOR, p)) {
        goto ERR_OUT;
    }

    if (CfiFlashQueryUint8(CFIFLASH_QUERY_SIZE, CFIFLASH_ONE_BANK_BITS, p)) {
        goto ERR_OUT;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_PAGE_BITS, CFIFLASH_EXPECT_PAGE_BITS, p)) {
        goto ERR_OUT;
    }

    if (CfiFlashQueryUint8(CFIFLASH_QUERY_ERASE_REGION, CFIFLASH_EXPECT_ERASE_REGION, p)) {
        goto ERR_OUT;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_BLOCKS, CFIFLASH_EXPECT_BLOCKS, p)) {
        goto ERR_OUT;
    }

    if (CfiFlashQueryUint16(CFIFLASH_QUERY_BLOCK_SIZE, CFIFLASH_EXPECT_BLOCK_SIZE, p)) {
        goto ERR_OUT;
    }

    base[0] = CFIFLASH_CMD_RESET;
    return 0;

ERR_OUT:
    dprintf("[%s]not supported CFI flash\n", __FUNCTION__);
    return -1;
}

int CfiFlashInit(uint32_t *p)
{
    dprintf("[%s]CFI flash init start ...\n", __FUNCTION__);
    if (CfiFlashQuery((uint8_t *)p)) {
        return -1;
    }
    dprintf("[%s]CFI flash init end ...\n", __FUNCTION__);
    return 0;
}

int CfiBlkOpen(struct Vnode *vnode)
{
    return 0;
}

int CfiBlkClose(struct Vnode *vnode)
{
    return 0;
}

static ssize_t CfiPreRead(char *buffer, unsigned bytes, char **newbuf)
{
    if (LOS_IsUserAddressRange((VADDR_T)buffer, bytes)) {
        *newbuf = LOS_MemAlloc(m_aucSysMem0, bytes);
        if (*newbuf == NULL) {
            dprintf("[%s]fatal memory allocation error\n", __FUNCTION__);
            return -ENOMEM;
        }
    } else if ((VADDR_T)buffer + bytes < (VADDR_T)buffer) {
        dprintf("[%s]invalid argument: buffer=%#x, size=%#x\n", __FUNCTION__, buffer, bytes);
        return -EFAULT;
    } else {
        *newbuf = buffer;
    }
    return 0;
}

static ssize_t CfiPostRead(char *buffer, char *newbuf, unsigned bytes, ssize_t ret)
{
    if (newbuf != buffer) {
        if (LOS_ArchCopyToUser(buffer, newbuf, bytes) != 0) {
            dprintf("[%s]LOS_ArchCopyToUser error\n", __FUNCTION__);
            ret = -EFAULT;
        }

        if (LOS_MemFree(m_aucSysMem0, newbuf) != 0) {
            dprintf("[%s]LOS_MemFree error\n", __FUNCTION__);
            ret = -EFAULT;
        }
    }
    return ret;
}

ssize_t CfiBlkRead(struct Vnode *vnode, unsigned char *buffer,
                   unsigned long long startSector, unsigned int nSectors)
{
    unsigned int i, wordOffset, bytes;
    uint32_t *p;
    uint32_t *base = ((struct drv_data*)(vnode->data))->priv;
    ssize_t ret;

    bytes = CfiFlashSec2Bytes(nSectors);
    wordOffset = B2W(CfiFlashSec2Bytes(startSector));

    if ((ret = CfiPreRead((char*)buffer, bytes, (char**)&p))) {
        return ret;
    }

    for (i = 0; i < B2W(bytes); i++) {
        p[i] = base[wordOffset + i];
    }
    ret = nSectors;

    return CfiPostRead((char*)buffer, (char*)p, bytes, ret);
}

static ssize_t CfiPreWrite(const char *buffer, unsigned bytes, char **newbuf)
{
    if (LOS_IsUserAddressRange((VADDR_T)buffer, bytes)) {
        *newbuf = LOS_MemAlloc(m_aucSysMem0, bytes);
        if (*newbuf == NULL) {
            dprintf("[%s]fatal memory allocation error\n", __FUNCTION__);
            return -ENOMEM;
        }

        if (LOS_ArchCopyFromUser(*newbuf, buffer, bytes)) {
            dprintf("[%s]LOS_ArchCopyFromUser error\n", __FUNCTION__);
            LOS_MemFree(m_aucSysMem0, *newbuf);
            return -EFAULT;
        }
    } else if ((VADDR_T)buffer + bytes < (VADDR_T)buffer) {
        dprintf("[%s]invalid argument\n", __FUNCTION__);
        return -EFAULT;
    } else {
        *newbuf = (char*)buffer;
    }
    return 0;
}

static ssize_t CfiPostWrite(const char *buffer, char *newbuf, ssize_t ret)
{
    if (newbuf != buffer) {
        if (LOS_MemFree(m_aucSysMem0, newbuf) != 0) {
            dprintf("[%s]LOS_MemFree error\n", __FUNCTION__);
            return -EFAULT;
        }
    }
    return ret;
}

ssize_t CfiBlkWrite(struct Vnode *vnode, const unsigned char *buffer,
                    unsigned long long startSector, unsigned int nSectors)
{
    unsigned int wordOffset, bytes;
    unsigned char *p;
    ssize_t ret;

    bytes = CfiFlashSec2Bytes(nSectors);
    wordOffset = B2W(CfiFlashSec2Bytes(startSector));

    if ((ret = CfiPreWrite((const char*)buffer, bytes, (char**)&p))) {
        return ret;
    }

    CfiFlashWriteBuf(wordOffset, (uint32_t *)p, B2W(bytes), ((struct drv_data*)(vnode->data))->priv);
    ret = nSectors;

    return CfiPostWrite((const char*)buffer, (char*)p, ret);
}

int CfiBlkGeometry(struct Vnode *vnode, struct geometry *geometry)
{
    geometry->geo_available = TRUE,
    geometry->geo_mediachanged = FALSE;
    geometry->geo_writeenabled = TRUE;
    geometry->geo_nsectors = CFIFLASH_SECTORS;
    geometry->geo_sectorsize = CFIFLASH_SEC_SIZE;

    return 0;
}

int CfiMtdErase(struct MtdDev *mtd, UINT64 start, UINT64 bytes, UINT64 *failAddr)
{
    uint32_t blkAddr, count, i;
    uint32_t *p = mtd->priv;

    blkAddr = CfiFlashEraseBlkWordAddr(B2W(start));
    count = (CfiFlashEraseBlkWordAddr(B2W(start + bytes - 1)) - blkAddr) / CFIFLASH_ERASEBLK_WORDS + 1;

    for (i = 0; i < count; i++) {
        p[blkAddr] = CFIFLASH_CMD_ERASE;
        DSB;
        p[blkAddr] = CFIFLASH_CMD_CONFIRM;
        while (!CfiFlashIsReady(blkAddr, p)) { }

        blkAddr += CFIFLASH_ERASEBLK_WORDS;
    }

    p[0] = CFIFLASH_CMD_CLEAR_STATUS;
    return 0;
}

int CfiMtdRead(struct MtdDev *mtd, UINT64 start, UINT64 bytes, const char *buf)
{
    UINT64 i;
    char *p;
    ssize_t ret;
    uint8_t *base = mtd->priv;

    if ((ret = CfiPreRead((char*)buf, bytes, &p))) {
        return ret;
    }

    for (i = 0; i < bytes; i++) {
        p[i] = base[start + i];
    }
    ret = (int)bytes;

    return CfiPostRead((char*)buf, p, bytes, ret);
}

int CfiMtdWrite(struct MtdDev *mtd, UINT64 start, UINT64 bytes, const char *buf)
{
    char *p;
    ssize_t ret;

    if (!IS_ALIGNED(start, WORD_ALIGN) || !IS_ALIGNED(bytes, WORD_ALIGN)) {
        dprintf("[%s]not aligned with 4B: start=%#0llx, bytes=%#0llx\n", __FUNCTION__, start, bytes);
        return -EINVAL;
    }

    if ((ret = CfiPreWrite(buf, bytes, &p))) {
        return ret;
    }

    CfiFlashWriteBuf((int)B2W(start), (uint32_t *)p, (size_t)B2W(bytes), mtd->priv);
    ret = (int)bytes;

    return CfiPostWrite(buf, p, ret);
}
