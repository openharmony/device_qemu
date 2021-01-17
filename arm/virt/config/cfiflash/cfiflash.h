#ifndef __CFIFLASH_H__
#define __CFIFLASH_H__

#include "fs/fs.h"
#include "mtd_dev.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cplusplus */
#endif /* __cplusplus */

#define HDF_LOG_TAG cfi_flash_driver
#define CFI_DRIVER "/dev/cfiflash"

#define CFIFLASH_CAPACITY       (64 * 1024 * 1024)
#define CFIFLASH_ONE_BANK_BITS  25                  /* 32M */

#define CFIFLASH_SEC_SIZE       512
#define CFIFLASH_SEC_SIZE_BITS  9
#define CFIFLASH_SECTORS        (CFIFLASH_CAPACITY / CFIFLASH_SEC_SIZE)
#define CFIFLASH_SEC_TO_BYTES(sector) ({ sector << CFIFLASH_SEC_SIZE_BITS; })

#define CFIFLASH_PAGE_SIZE                  2048
#define CFIFLASH_PAGE_WORDS                 (CFIFLASH_PAGE_SIZE / sizeof(uint32_t))
#define CFIFLASH_PAGE_WORDS_MASK            (CFIFLASH_PAGE_WORDS - 1)
#define CFIFLASH_PAGE_WORDOFFSET(addr)      ({ (addr) & CFIFLASH_PAGE_WORDS_MASK; })
#define CFIFLASH_ERASEBLK_SIZE              (128 * 1024)
#define CFIFLASH_ERASEBLK_WORDS             (CFIFLASH_ERASEBLK_SIZE / sizeof(uint32_t))
#define CFIFLASH_ERASEBLK_WORDMASK          (~(CFIFLASH_ERASEBLK_WORDS - 1))
#define CFIFLASH_ERASEBLK_WORDADDR(addr)    ({ (addr) & CFIFLASH_ERASEBLK_WORDMASK; })

extern volatile uint32_t *g_cfiFlashBase;

int CfiFlashInit(void);
ssize_t CfiBlkRead(struct inode *inode, unsigned char *buffer,
            unsigned long long start_sector, unsigned int nsectors);
ssize_t CfiBlkWrite(struct inode *inode, const unsigned char *buffer,
            unsigned long long start_sector, unsigned int nsectors);
int CfiBlkGeometry(struct inode *inode, struct geometry *geometry);

int CfiMtdErase(struct MtdDev *mtd, UINT64 start, UINT64 len, UINT64 *failAddr);
int CfiMtdRead(struct MtdDev *mtd, UINT64 start, UINT64 len, const char *buf);
int CfiMtdWrite(struct MtdDev *mtd, UINT64 start, UINT64 len, const char *buf);

#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cplusplus */
#endif /* __cplusplus */

#endif
