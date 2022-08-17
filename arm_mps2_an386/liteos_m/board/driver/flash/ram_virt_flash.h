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

#ifndef _RAM_VIRT_FLASH_H_
#define _RAM_VIRT_FLASH_H_

#include "los_compiler.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FLASH_PARTITION_DATA0 = 0,
    FLASH_PARTITION_DATA1 = 1,
    FLASH_PARTITION_MAX,
} HalPartition;

typedef struct {
    const CHAR *partitionDescription;
    UINT32 partitionStartAddr;
    UINT32 partitionLength;
    UINT32 partitionOptions;
} HalLogicPartition;


HalLogicPartition *getPartitionInfo(VOID);
INT32 virt_flash_erase(HalPartition in_partition, UINT32 off_set, UINT32 size);
INT32 virt_flash_write(HalPartition in_partition, UINT32 *off_set, const VOID *in_buf, UINT32 in_buf_len);
INT32 virt_flash_erase_write(HalPartition in_partition, UINT32 *off_set, const VOID *in_buf, UINT32 in_buf_len);
INT32 virt_flash_read(HalPartition in_partition, UINT32 *off_set, VOID *out_buf, UINT32 in_buf_len);

#ifdef __cplusplus
}
#endif

#endif /* _RAM_VIRT_FLASH_H_ */