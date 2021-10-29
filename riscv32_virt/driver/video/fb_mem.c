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

#include "fb_mem.h"

#include "key_cache.h"


static UINT32 g_vnodeMux;

int FbMemInit(void)
{
    int retval = LOS_MuxCreate(&g_vnodeMux);
    if (retval != LOS_OK) {
        PRINT_ERR("Create mutex for fbmem fail, status: %d", retval);
        return retval;
    }
    return LOS_OK;
}

int FbMemHold(void)
{
    int ret = LOS_MuxPend(g_vnodeMux, LOS_WAIT_FOREVER);
    if (ret != LOS_OK) {
        PRINT_ERR("FbMemHold lock failed !\n");
    }
    return ret;
}

int FbMemDrop(void)
{
    int ret = LOS_MuxPost(g_vnodeMux);
    if (ret != LOS_OK) {
        PRINT_ERR("FbMemDrop unlock failed !\n");
    }
    return ret;
}

int FbMemLookup(const char *key, struct fb_mem **result, uint32_t flags)
{
    struct fb_mem *fbmem = NULL;
    size_t len = strlen(key);
    int ret = 0;

    ret = KeyCacheLookup(key, len, &fbmem);
    if (ret == LOS_OK) {
        *result = fbmem;
    } else {
        if (flags & V_DUMMY) {
            fbmem = (struct fb_mem *)zalloc(sizeof(struct fb_mem));
            if (fbmem == NULL) {
                return LOS_NOK;
            }
            (void)KeyCacheAlloc(fbmem, key, len);
            *result = fbmem;
            ret = LOS_OK;
        } else {
            ret = -ENXIO;
        }
    }
    return ret;
}

int register_driver(const char *key, void *prev)
{
    struct fb_mem *fbmem = NULL;
    int ret;
    if (key == NULL || strlen(key) >= PATH_MAX) {
        return -EINVAL;
    }
    FbMemHold();
    ret = FbMemLookup(key, &fbmem, 0);
    if (ret == 0) {
        FbMemDrop();
        return -EEXIST;
    }

    ret = FbMemLookup(key, &fbmem, V_CREATE | V_DUMMY);
    if (ret == OK) {
        fbmem->data = prev;
    }
    FbMemDrop();
    return ret;
}

int unregister_driver(const char *key)
{
    struct fb_mem *fbmem = NULL;
    int ret;
    if (key == NULL || strlen(key) >= PATH_MAX) {
        return -EINVAL;
    }
    FbMemHold();
    size_t len = strlen(key);
    ret = KeyCacheLookup(key, len, &fbmem);
    if (ret == LOS_OK) {
        FbMemKeyCacheFree(fbmem);
        free(fbmem);
    }
    FbMemDrop();
    return ret;
}