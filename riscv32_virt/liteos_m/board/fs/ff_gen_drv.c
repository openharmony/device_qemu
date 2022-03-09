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

#include "los_debug.h"
#include "fs_config.h"
#include "fatfs.h"
#include "ff_gen_drv.h"

DiskDrvTypeDef g_diskDrv = { { 0 }, { 0 }, { 0 }, { 0 } };

#if (LOSCFG_SUPPORT_FATFS == 1)
#include "cfiflash.h"
/* 
    note: Physical drive 0 for cfiFlash can not be used, may be conflicted with kernel img,
          so we must assign index=1 when use -drive if=pflash in qemu_run.sh 
*/

int SetupVolToPartTable(int index, PARTITION vptable, DiskioDrvTypeDef *cfiDrv)
{
    if (index < 0 || index >= FF_VOLUMES) {
        PRINT_ERR("[%s]the volum index is out of range\n", __func__);
        return RES_ERROR;
    }
    VolToPart[index].di = vptable.di;    /* Physics disk id */
    VolToPart[index].pd = vptable.pd;    /* Physical drive number */
    VolToPart[index].pt = vptable.pt;    /* Partition: 0:Auto detect, 1-4:Forced partition) */
    VolToPart[index].ps = vptable.ps;    /* Partition start sector */
    VolToPart[index].pc = vptable.pc;    /* Partition sector count */

    g_diskDrv.initialized[index] = 0;
    g_diskDrv.drv[index] = cfiDrv;
    g_diskDrv.lun[index] = VolToPart[index].pd;
    return RES_OK;
}

void SetupDefaultVolToPartTable()
{
    int i = 0;
    PARTITION defaultVToP[FF_VOLUMES] = { 
        {0, 1, 1},     /* "0:" ==> Physical drive 1, 1st partition */
        {1, 1, 2},     /* "1:" ==> Physical drive 1, 2nd partition */
        {2, 1, 3},     /* "2:" ==> Physical drive 1, 3rd partition */
        {3, 1, 4}      /* "3:" ==> Physical drive 1, 4th partition */
    };

    for (i = 0; i < FF_VOLUMES; i++) {
        SetupVolToPartTable(i, defaultVToP[i], GetCfiBlkOps());
    }
}
#endif

int DiscDrvInit(void)
{

#if (LOSCFG_SUPPORT_FATFS == 1)
    SetupDefaultVolToPartTable();
#endif

    return RES_OK;
}

