/*
 * Copyright (c) 2022 Unionman Co., Ltd.
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

#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "display_gralloc.h"
#include "securec.h"
#include "display_gfx.h"

GrallocFuncs *grallocFucs = NULL;
int32_t InitGfx()
{
    return DISPLAY_SUCCESS;
}

int32_t DeinitGfx()
{
    return DISPLAY_SUCCESS;
}

int32_t FillRect(ISurface *iSurface, IRect *rect, uint32_t color, GfxOpt *opt)
{
    return DISPLAY_SUCCESS;
}

int32_t Blit(ISurface *srcSurface, IRect *srcRect, ISurface *dstSurface, IRect *dstRect, GfxOpt *opt)
{
    return DISPLAY_SUCCESS;
}

int32_t Sync(int32_t timeOut)
{
    return DISPLAY_SUCCESS;
}

int32_t GfxInitialize(GfxFuncs **funcs)
{
    GfxFuncs *gfxFuncs = (GfxFuncs *)malloc(sizeof(GfxFuncs));

    errno_t eok = memset_s((void *)gfxFuncs, sizeof(GfxFuncs), 0, sizeof(GfxFuncs));
    if (eok != EOK) {
        free(gfxFuncs);
        return DISPLAY_FAILURE;
    }
    gfxFuncs->InitGfx = InitGfx;
    gfxFuncs->DeinitGfx = DeinitGfx;
    gfxFuncs->FillRect = FillRect;
    gfxFuncs->Blit = Blit;
    gfxFuncs->Sync = Sync;
    *funcs = gfxFuncs;

    return DISPLAY_SUCCESS;
}

int32_t GfxUninitialize(GfxFuncs *funcs)
{
    free(funcs);
    //DISPLAY_DEBUGLOG("%s: gfx uninitialize success", __func__);
    return DISPLAY_SUCCESS;
}
