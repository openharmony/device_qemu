/*
 * Copyright (c) 2013-2019 Huawei Technologies Co., Ltd. All rights reserved.
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd. All rights reserved.
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

#ifndef __MMZ_H__
#define __MMZ_H__

#include "los_typedef.h"
#include "sys/ioctl.h"

typedef enum {
    MMZ_CACHE = 1,                  /* allocate mmz with cache attribute */
    MMZ_NOCACHE,                    /* allocate mmz with nocache attribute */
    MMZ_FREE,                       /* free mmz */
    MAP_CACHE,
    MAP_NOCACHE,
    UNMAP,
    FLUSH_CACHE,
    FLUSH_NOCACHE,
    INVALIDATE,
    MMZ_MAX
} MMZ_TYPE;

typedef struct {
    void *vaddr;
    uint64_t paddr;
    int32_t size;
} MmzMemory;

#define MMZ_IOC_MAGIC               'M'
#define MMZ_CACHE_TYPE              _IOR(MMZ_IOC_MAGIC, MMZ_CACHE, MmzMemory)
#define MMZ_NOCACHE_TYPE            _IOR(MMZ_IOC_MAGIC, MMZ_NOCACHE, MmzMemory)
#define MMZ_FREE_TYPE               _IOR(MMZ_IOC_MAGIC, MMZ_FREE, MmzMemory)
#define MMZ_MAP_CACHE_TYPE          _IOR(MMZ_IOC_MAGIC, MAP_CACHE, MmzMemory)
#define MMZ_MAP_NOCACHE_TYPE        _IOR(MMZ_IOC_MAGIC, MAP_NOCACHE, MmzMemory)
#define MMZ_UNMAP_TYPE              _IOR(MMZ_IOC_MAGIC, UNMAP, MmzMemory)
#define MMZ_FLUSH_CACHE_TYPE        _IOR(MMZ_IOC_MAGIC, FLUSH_CACHE, MmzMemory)
#define MMZ_FLUSH_NOCACHE_TYPE      _IOR(MMZ_IOC_MAGIC, FLUSH_NOCACHE, MmzMemory)
#define MMZ_INVALIDATE_TYPE         _IOR(MMZ_IOC_MAGIC, INVALIDATE, MmzMemory)

#define MMZ_NODE                    "/dev/mmz"

int DevMmzRegister(void);

#endif
