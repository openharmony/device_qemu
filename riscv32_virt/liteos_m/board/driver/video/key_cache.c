/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd. All rights reserved.
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
#include "los_config.h"
#include "los_debug.h"
#include "stdlib.h"
#include "limits.h"
#include "key_cache.h"

#define PATH_CACHE_HASH_MASK (LOSCFG_MAX_PATH_CACHE_SIZE - 1)
LIST_HEAD g_keyCacheHashEntrys[LOSCFG_MAX_PATH_CACHE_SIZE];
#ifdef LOSCFG_DEBUG_VERSION
static int g_totalKeyCacheHit = 0;
static int g_totalKeyCacheTry = 0;
#define TRACE_TRY_CACHE() do { g_totalKeyCacheTry++; } while (0)
#define TRACE_HIT_CACHE(kc) do { kc->hit++; g_totalKeyCacheHit++; } while (0)

void ResetKeyCacheHitInfo(int *hit, int *try)
{
    *hit = g_totalKeyCacheHit;
    *try = g_totalKeyCacheTry;
    g_totalKeyCacheHit = 0;
    g_totalKeyCacheTry = 0;
}
#else
#define TRACE_TRY_CACHE()
#define TRACE_HIT_CACHE(kc)
#endif

int KeyCacheInit(void)
{
    for (int i = 0; i < LOSCFG_MAX_PATH_CACHE_SIZE; i++) {
        LOS_ListInit(&g_keyCacheHashEntrys[i]);
    }
    return LOS_OK;
}

void KeyCacheDump(void)
{
    PRINTK("-------->keyCache dump in\n");
    for (int i = 0; i < LOSCFG_MAX_PATH_CACHE_SIZE; i++) {
        struct KeyCache *kc = NULL;
        LIST_HEAD *nhead = &g_keyCacheHashEntrys[i];

        LOS_DL_LIST_FOR_EACH_ENTRY(kc, nhead, struct KeyCache, hashEntry) {
            PRINTK("    keyCache dump hash %d item %s %p %d\n", i,
                kc->name, kc->fbmem, kc->nameLen);
        }
    }
    PRINTK("-------->keyCache dump out\n");
}

void KeyCacheMemoryDump(void)
{
    int keyCacheNum = 0;
    int nameSum = 0;
    for (int i = 0; i < LOSCFG_MAX_PATH_CACHE_SIZE; i++) {
        LIST_HEAD *dhead = &g_keyCacheHashEntrys[i];
        struct KeyCache *dent = NULL;

        LOS_DL_LIST_FOR_EACH_ENTRY(dent, dhead, struct KeyCache, hashEntry) {
            keyCacheNum++;
            nameSum += dent->nameLen;
        }
    }
    PRINTK("keyCache number = %d\n", keyCacheNum);
    PRINTK("keyCache memory size = %d(B)\n", keyCacheNum * sizeof(struct KeyCache) + nameSum);
}

static uint32_t NameHash(const char *name, int len)
{
    uint32_t hash;
    hash = LOS_HashFNV32aBuf(name, len, FNV1_32A_INIT);
    return hash;
}

static void KeyCacheInsert(struct KeyCache *cache, const char* name, int len)
{
    int hash = NameHash(name, len) & PATH_CACHE_HASH_MASK;
    LOS_ListAdd(&g_keyCacheHashEntrys[hash], &cache->hashEntry);
}

struct KeyCache *KeyCacheAlloc(struct fb_mem *fbmem, const char *name, uint8_t len)
{
    struct KeyCache *kc = NULL;
    size_t keyCacheSize;
    int ret;

    if (name == NULL || len > NAME_MAX || fbmem == NULL) {
        return NULL;
    }
    keyCacheSize = sizeof(struct KeyCache) + len + 1;
    kc = (struct KeyCache*)zalloc(keyCacheSize);
    if (kc == NULL) {
        PRINT_ERR("keyCache alloc failed, no memory!\n");
        return NULL;
    }

    ret = strncpy_s(kc->name, len + 1, name, len);
    if (ret != LOS_OK) {
        return NULL;
    }

    kc->nameLen = len;
    kc->fbmem = fbmem;

    KeyCacheInsert(kc, name, len);

    return kc;
}

int KeyCacheFree(struct KeyCache *kc)
{
    if (kc == NULL) {
        PRINT_ERR("keyCache free: invalid keyCache\n");
        return -ENOENT;
    }
    LOS_ListDelete(&kc->hashEntry);
    free(kc);
    return LOS_OK;
}

int KeyCacheLookup(const char *name, int len, struct fb_mem **fbmem)
{
    struct KeyCache *kc = NULL;
    int hash = NameHash(name, len) & PATH_CACHE_HASH_MASK;
    LIST_HEAD *dhead = &g_keyCacheHashEntrys[hash];

    TRACE_TRY_CACHE();
    LOS_DL_LIST_FOR_EACH_ENTRY(kc, dhead, struct KeyCache, hashEntry) {
        if (kc->nameLen == len && !strncmp(kc->name, name, len)) {
            *fbmem = kc->fbmem;
            TRACE_HIT_CACHE(kc);
            return LOS_OK;
        }
    }
    return -ENOENT;
}

void FbMemKeyCacheFree(struct fb_mem *fbmem) {
    for (int i = 0; i < LOSCFG_MAX_PATH_CACHE_SIZE; i++) {
        struct KeyCache *kc = NULL;
        LIST_HEAD *nhead = &g_keyCacheHashEntrys[i];

        LOS_DL_LIST_FOR_EACH_ENTRY(kc, nhead, struct KeyCache, hashEntry) {
           if (kc->fbmem == fbmem) {
                KeyCacheFree(kc);
                return;
            }
        }
    }
}

LIST_HEAD* GetKeyCacheList()
{
    return g_keyCacheHashEntrys;
}
