/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd. All rights reserved.
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

#include <string.h>
#include <stdlib.h>
#include "stdio.h"
#include "pthread.h"
#include "securec.h"
#include "los_interrupt.h"
#include "ram_virt_flash.h"

/* Logic partition on flash devices, if open at the same time, needed modify by user */
HalLogicPartition g_halPartitions[] = {
    [FLASH_PARTITION_DATA0] = {
        .partitionDescription = "littlefs",
        .partitionStartAddr = 0x21800000,
        .partitionLength = 0x800000, // size is 8M
    },
    [FLASH_PARTITION_DATA1] = {
        .partitionDescription = "vfat",
        .partitionStartAddr = 0x21800000,
        .partitionLength = 0x800000, // size is 8M
    },
};

INT32 virt_flash_erase(HalPartition pdrv, UINT32 offSet, UINT32 size)
{
    UINT32 startAddr;
    UINT32 partitionEnd;

    startAddr = g_halPartitions[pdrv].partitionStartAddr + offSet;
    partitionEnd = g_halPartitions[pdrv].partitionStartAddr + g_halPartitions[pdrv].partitionLength;
    if (startAddr >= partitionEnd) {
        printf("flash over erase, 0x%x, 0x%x\r\n", startAddr, partitionEnd);
        printf("flash over write\r\n");
        return -1;
    }
    if ((startAddr + size) > partitionEnd) {
        size = partitionEnd - startAddr;
        printf("flash over write, new len is %d\r\n", size);
    }

    UINT32 intSave = LOS_IntLock();
    (VOID)memset_s((VOID *)startAddr, size, 0, size);
    LOS_IntRestore(intSave);
    return 0;
}

INT32 virt_flash_write(HalPartition pdrv, UINT32 *offSet, const VOID *buf, UINT32 bufLen)
{
    UINT32 startAddr;
    UINT32 partitionEnd;

    startAddr = g_halPartitions[pdrv].partitionStartAddr + *offSet;
    partitionEnd = g_halPartitions[pdrv].partitionStartAddr + g_halPartitions[pdrv].partitionLength;
    if (startAddr >= partitionEnd) {
        printf("flash over write, 0x%x, 0x%x\r\n", startAddr, partitionEnd);
        return -1;
    }
    if ((startAddr + bufLen) > partitionEnd) {
        bufLen = partitionEnd - startAddr;
        printf("flash over write, new len is %d\r\n", bufLen);
    }

    UINT32 intSave = LOS_IntLock();
    (VOID)memcpy_s((VOID *)startAddr, bufLen, buf, bufLen);
    LOS_IntRestore(intSave);

    return 0;
}

INT32 virt_flash_erase_write(HalPartition pdrv, UINT32 *offSet, const VOID *buf, UINT32 bufLen)
{
    UINT32 startAddr;
    UINT32 partitionEnd;

    startAddr = g_halPartitions[pdrv].partitionStartAddr + *offSet;
    partitionEnd = g_halPartitions[pdrv].partitionStartAddr + g_halPartitions[pdrv].partitionLength;
    if (startAddr >= partitionEnd) {
        printf("flash over e&w, 0x%x, 0x%x\r\n", startAddr, partitionEnd);
        return -1;
    }
    if ((startAddr + bufLen) > partitionEnd) {
        bufLen = partitionEnd - startAddr;
        printf("flash over erase&write, new len is %d\r\n", bufLen);
    }

    UINT32 intSave = LOS_IntLock();
    (VOID)memset_s((VOID *)startAddr, bufLen, 0, bufLen);
    (VOID)memcpy_s((VOID *)startAddr, bufLen, buf, bufLen);
    LOS_IntRestore(intSave);
    return 0;
}

INT32 virt_flash_read(HalPartition pdrv, UINT32 *offSet, VOID *buf, UINT32 bufLen)
{
    UINT32 startAddr;
    UINT32 partitionEnd;
    startAddr = g_halPartitions[pdrv].partitionStartAddr + *offSet;
    partitionEnd = g_halPartitions[pdrv].partitionStartAddr + g_halPartitions[pdrv].partitionLength;
    if (startAddr >= partitionEnd) {
        printf("flash over read, 0x%x, 0x%x\r\n", startAddr, partitionEnd);
        return -1;
    }
    if ((startAddr + bufLen) > partitionEnd) {
        bufLen = partitionEnd - startAddr;
        printf("flash over read, new len is %d\r\n", bufLen);
    }

    UINT32 intSave = LOS_IntLock();
    (VOID)memcpy_s(buf, bufLen, (VOID *)startAddr, bufLen);
    LOS_IntRestore(intSave);
    return 0;
}
