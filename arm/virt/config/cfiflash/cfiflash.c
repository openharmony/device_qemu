/*
  simple CFI flash driver for QEMU arm 'virt' machine, with:

  * 64M = 2 bank * 1 region * 256 Erase Blocks * 128K(64 pages * 2048B)
  * 32 bits, Intel command set
*/

#include "sys/param.h"
#include "user_copy.h"
#include "cfiflash.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

/* volatile disable compiler optimization */
volatile uint32_t *g_cfiFlashBase;

#define CFIFLASH_QUERY_CMD(p) do { p[0x55<<2] = 0x98; } while(0)
#define CFIFLASH_QUERY_QRY(p) do {                                          \
        if (p[0x10<<2] != 'Q' || p[0x11<<2] != 'R' || p[0x12<<2] != 'Y') {  \
            dprintf("[CFIFLASH_QUERY]0x10~12: %#02x %#02x %#02x\n",         \
                                p[0x10<<2], p[0x11<<2], p[0x12<<2]);        \
            goto ERR_OUT;                                                   \
        }                                                                   \
    } while(0)
#define CFIFLASH_QUERY_INTEL(p) do {                                \
        if (((p[0x14<<2] << 8) + p[0x13<<2]) != 1) {                \
            dprintf("[CFIFLASH_QUERY]0x13~14: %#02x %#02x\n",       \
                                p[0x13<<2], p[0x14<<2]);            \
            goto ERR_OUT;                                           \
        }                                                           \
    } while(0)
/* TODO: let QEMU response one-64M-bank other than two-32M-bank */
#define CFIFLASH_QUERY_SIZE(p) do {                                 \
        if (p[0x27<<2] != CFIFLASH_ONE_BANK_BITS) {                 \
            dprintf("[CFIFLASH_QUERY]0x27: %#02x\n", p[0x27<<2]);   \
            goto ERR_OUT;                                           \
        }                                                           \
    } while(0)
#define CFIFLASH_QUERY_PAGE_BITS(p) do {                            \
        if (((p[0x2B<<2] << 8) + p[0x2A<<2]) != 11) {               \
            dprintf("[CFIFLASH_QUERY]0x2A~2B: %#02x %#02x\n",       \
                                p[0x2A<<2], p[0x2B<<2]);            \
            goto ERR_OUT;                                           \
        }                                                           \
    } while(0)
#define CFIFLASH_QUERY_ERASE_REGIONS(p) do {                        \
        if (p[0x2C<<2] != 1) {                                      \
            dprintf("[CFIFLASH_QUERY]0x2C: %#02x\n", p[0x2C<<2]);   \
            goto ERR_OUT;                                           \
        }                                                           \
    } while(0)
#define CFIFLASH_QUERY_BLOCK_COUNT(p) do { /* y+1: # of blocks */   \
        if (((p[0x2E<<2] << 8) + p[0x2D<<2]) != 255) {              \
            dprintf("[CFIFLASH_QUERY]0x2D~2E: %#02x %#02x\n",       \
                                p[0x2D<<2], p[0x2E<<2]);            \
            goto ERR_OUT;                                           \
        }                                                           \
    } while (0)
#define CFIFLASH_QUERY_BLOCK_SIZE(p) do { /* z*256: block size */   \
        if (((p[0x30<<2] << 8) + p[0x2F<<2]) != 512) {              \
            dprintf("[CFIFLASH_QUERY]0x2F~30: %#02x %#02x\n",       \
                                p[0x2F<<2], p[0x30<<2]);            \
            goto ERR_OUT;                                           \
        }                                                           \
    } while (0)

#define CFIFLASH_READ_BYTE(offset) ({ ((char *)g_cfiFlashBase)[offset]; })
#define CFIFLASH_READ_WORD(offset) ({ g_cfiFlashBase[offset]; })

#define CFIFLASH_WRITE_WORD(offset, value) do { g_cfiFlashBase[offset] = value; } while(0)

#define CFIFLASH_RESET(offset) do { g_cfiFlashBase[offset] = 0xFF; } while(0)

#define CFIFLASH_STATUS_IS_READY(offset) ({     \
    g_cfiFlashBase[offset] = 0x70;              \
    g_cfiFlashBase[offset] & 0x80;              \
})

#define CFIFLASH_CLEAR_STATUS() do { *g_cfiFlashBase = 0x50; } while(0)

#define CFIFLASH_CMD_BUFWRITE_BEGIN(blk)   do { CFIFLASH_WRITE_WORD(blk, 0xE8); } while(0)
#define CFIFLASH_CMD_BUFWRITE_CONFIRM(blk) do { CFIFLASH_WRITE_WORD(blk, 0xD0); } while(0)
#define CFIFLASH_CMD_BUFWRITE_COUNT(blk, cnt) do { CFIFLASH_WRITE_WORD(blk, (cnt)-1); } while(0) /* 0-based */

/* all in word(4 bytes) measure */
static void CfiFlashWriteBuf(int offset, uint32_t *buffer, size_t buflen)
{
    uint32_t i, blkaddr, wordcount;

    /* first write might not be Page aligned */
    i = CFIFLASH_PAGE_WORDS - CFIFLASH_PAGE_WORDOFFSET(offset);
    wordcount = (i > buflen) ? buflen : i;

    while (buflen) {
        /* command buffer-write begin to Erase Block address */
        blkaddr = CFIFLASH_ERASEBLK_WORDADDR(offset);
        CFIFLASH_CMD_BUFWRITE_BEGIN(blkaddr);

        /* write words count */
        CFIFLASH_CMD_BUFWRITE_COUNT(blkaddr, wordcount);

        /* program word data to actual address */
        for (i = 0; i < wordcount; i++, offset++, buffer++) {
            CFIFLASH_WRITE_WORD(offset, *buffer);
        }

        /* command buffer-write end to Erase Block address */
        CFIFLASH_CMD_BUFWRITE_CONFIRM(blkaddr);
        while (!CFIFLASH_STATUS_IS_READY(blkaddr));

        buflen -= wordcount;
        wordcount = (buflen >= CFIFLASH_PAGE_WORDS) ? CFIFLASH_PAGE_WORDS : buflen;
    }

    CFIFLASH_CLEAR_STATUS();
    CFIFLASH_RESET(offset);
}

static int CfiFlashQuery(void)
{
    uint8_t *p = (uint8_t *)g_cfiFlashBase;

    CFIFLASH_QUERY_CMD(p);

    CFIFLASH_QUERY_QRY(p);

    CFIFLASH_QUERY_INTEL(p);

    CFIFLASH_QUERY_SIZE(p);

    CFIFLASH_QUERY_PAGE_BITS(p);

    CFIFLASH_QUERY_ERASE_REGIONS(p);

    CFIFLASH_QUERY_BLOCK_COUNT(p);

    CFIFLASH_QUERY_BLOCK_SIZE(p);

    CFIFLASH_RESET(0);
    return 0;

ERR_OUT:
    dprintf("[%s]not support CFI flash\n", __FUNCTION__);
    return -1;
}

int CfiFlashInit(void)
{
    dprintf("[%s]CFI flash init start ...\n", __FUNCTION__);
    if (CfiFlashQuery()) {
        return -1;
    }
    dprintf("[%s]CFI flash init end ...\n", __FUNCTION__);
    return 0;
}

static ssize_t CfiPreRead(void *buffer, unsigned int bytesize, void **newbuf)
{
    if (LOS_IsUserAddressRange((VADDR_T)buffer, bytesize)) {
        *newbuf = LOS_MemAlloc(m_aucSysMem0, bytesize);
        if (*newbuf == NULL) {
            dprintf("[%s]fatal memory allocation error\n", __FUNCTION__);
            return -ENOMEM;
        }
    } else if ((VADDR_T)buffer + bytesize < (VADDR_T)buffer) {
        dprintf("[%s]invalid argument: buffer=%#x, size=%#x\n", __FUNCTION__, buffer, bytesize);
        return -EFAULT;
    } else {
        *newbuf = buffer;
    }
    return 0;
}

static ssize_t CfiPostRead(void *buffer, void *newbuf, unsigned int bytesize, ssize_t ret)
{
    if (newbuf != buffer) {
        if (LOS_ArchCopyToUser(buffer, newbuf, bytesize) != 0) {
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

ssize_t CfiBlkRead(struct inode *inode, unsigned char *buffer,
            unsigned long long start_sector, unsigned int nsectors)
{
    unsigned int i, wordoffset, bytesize;
    uint32_t *p;
    ssize_t ret;

    bytesize = CFIFLASH_SEC_TO_BYTES(nsectors);
    wordoffset = CFIFLASH_SEC_TO_BYTES(start_sector) >> 2;

    if ((ret = CfiPreRead(buffer, bytesize, (void **)&p))) {
        return ret;
    }

    for (i = 0; i < (bytesize >> 2); i++) {
        p[i] = CFIFLASH_READ_WORD(wordoffset + i);
    }
    ret = nsectors;
    
    return CfiPostRead(buffer, p, bytesize, ret);
}

static ssize_t CfiPreWrite(void *buffer, unsigned int bytesize, void **newbuf)
{
    if (LOS_IsUserAddressRange((VADDR_T)buffer, bytesize)) {
        *newbuf = LOS_MemAlloc(m_aucSysMem0, bytesize);
        if (*newbuf == NULL) {
            dprintf("[%s]fatal memory allocation error\n", __FUNCTION__);
            return -ENOMEM;
        }
        if (LOS_ArchCopyFromUser(*newbuf, buffer, bytesize)) {
            dprintf("[%s]LOS_ArchCopyFromUser error\n", __FUNCTION__);
            LOS_MemFree(m_aucSysMem0, *newbuf);
            return -EFAULT;
        }
    } else if ((VADDR_T)buffer + bytesize < (VADDR_T)buffer) {
        dprintf("[%s]invalid argument\n", __FUNCTION__);
        return -EFAULT;
    } else {
        *newbuf = buffer;
    } 
    return 0;
}

static ssize_t CfiPostWrite(void *buffer, void *newbuf, ssize_t ret)
{
    if (newbuf != buffer) {
        if (LOS_MemFree(m_aucSysMem0, newbuf) != 0) {
            dprintf("[%s]LOS_MemFree error\n", __FUNCTION__);
            return -EFAULT;
        }
    }
    return ret;
}

ssize_t CfiBlkWrite(struct inode *inode, const unsigned char *buffer,
            unsigned long long start_sector, unsigned int nsectors)
{
    unsigned int wordoffset, bytesize;
    unsigned char *p;
    ssize_t ret;

    bytesize = CFIFLASH_SEC_TO_BYTES(nsectors);
    wordoffset = CFIFLASH_SEC_TO_BYTES(start_sector) >> 2;

    if ((ret = CfiPreWrite((void *)buffer, bytesize, (void **)&p))) {
        return ret;
    }

    CfiFlashWriteBuf(wordoffset, (uint32_t *)p, bytesize >> 2);
    ret = nsectors;

    return CfiPostWrite((void *)buffer, p, ret);
}

int CfiBlkGeometry(struct inode *inode, struct geometry *geometry)
{
    geometry->geo_available = TRUE,
    geometry->geo_mediachanged = FALSE;
    geometry->geo_writeenabled = TRUE;
    geometry->geo_nsectors = CFIFLASH_SECTORS;
    geometry->geo_sectorsize = CFIFLASH_SEC_SIZE;

    return 0;
}

int CfiMtdErase(struct MtdDev *mtd, UINT64 start, UINT64 len, UINT64 *failAddr)
{
    return 0;
}

int CfiMtdRead(struct MtdDev *mtd, UINT64 start, UINT64 len, const char *buf)
{
    UINT64 i;
    char *p;
    ssize_t ret;

    if ((ret = CfiPreRead((void *)buf, len, (void **)&p))) {
        return ret;
    }

    for (i = 0; i < len; i++) {
        p[i] = CFIFLASH_READ_BYTE(start + i);
    }
    ret = (int)len;
    
    return CfiPostRead((void *)buf, p, len, ret);
}

int CfiMtdWrite(struct MtdDev *mtd, UINT64 start, UINT64 len, const char *buf)
{
    unsigned char *p;
    ssize_t ret;

    if ((start & 3) || (len & 3)) {
        dprintf("[%s]parameter not aligned with 4B: start=%#0llx, len=%#0llx\n", __FUNCTION__, start, len);
        return -EINVAL;
    }

    if ((ret = CfiPreWrite((void *)buf, len, (void **)&p))) {
        return ret;
    }

    CfiFlashWriteBuf((int)(start >> 2), (uint32_t *)p, (size_t)(len >> 2));
    ret = (int)len;

    return CfiPostWrite((void *)buf, p, ret);
}

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */
