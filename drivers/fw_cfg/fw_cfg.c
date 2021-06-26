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
 * Simple fw_cfg driver for passing customized command line arguments, format:
 * "argName0=oct-dec-or-hex-number argName1='string ..."
 *
 * NOTE error-prone, keep argument expressions as simple as possible.
 *
 * Based on QEMU document:
 * https://gitlab.com/qemu-project/qemu/-/blob/master/docs/specs/fw_cfg.txt
 */
#include "los_base.h"
#include "los_vm_zone.h"
#include "string.h"
#include "endian.h"

#define FW_CFG_REG_CTL      0x09020008  /* Width: 16-bit, big-endian */
#define FW_CFG_REG_DATA     0x09020000  /* Width: 8/16/32/64-bit, read-only, string-preserving */

#define FW_CFG_KEY_SIGNATURE    0x00
#define FW_CFG_KEY_FILEDIR      0x19
#define FW_CFG_SIG_WORD         0x51454d55ULL /* "QEMU" */

#define FW_CFG_ARGS_FILE        "opt/d"
#define FW_CFG_ARGS_LEN         100

#define FW_CFG_MAX_FILE_PATH    56
struct FWCfgFile {
    uint32_t size;              /* size of referenced fw_cfg item, big-endian */
    uint16_t select;            /* selector key of fw_cfg item, big-endian */
    uint16_t reserved;
    char name[FW_CFG_MAX_FILE_PATH];
};
struct FWCfgFiles {
    uint32_t count;             /* number of entries, big-endian */
    struct FWCfgFile f[];
};

static inline void FwcfgSelect(uint16_t beKey)
{
    WRITE_UINT16(beKey, IO_DEVICE_ADDR(FW_CFG_REG_CTL));
}

static inline uint8_t FwcfgGetByte()
{
    return GET_UINT8(IO_DEVICE_ADDR(FW_CFG_REG_DATA));
}

static inline uint32_t FwcfgGetBe32toh()
{
    return be32toh(GET_UINT32(IO_DEVICE_ADDR(FW_CFG_REG_DATA)));
}

static bool FwcfgIsExisted()
{
    FwcfgSelect(htobe16(FW_CFG_KEY_SIGNATURE));

    return FwcfgGetBe32toh() == FW_CFG_SIG_WORD;
}

static bool FwcfgFindFile(struct FWCfgFile *f, const char *name)
{
    int count, i, j;
    char *p = NULL;

    FwcfgSelect(htobe16(FW_CFG_KEY_FILEDIR));
    count = FwcfgGetBe32toh();  /* struct FWCfgFiles.count */

    for (i = 0; i < count; i++) {
        p = (char *)f;
        for (j = 0; j < sizeof(struct FWCfgFile); j++, p++) {
            *p = FwcfgGetByte();
        }

        if (strncmp(f->name, name, strlen(name)) == 0) {
            return true;
        }
    }

    return false;
}

static int FwcfgReadFile(const struct FWCfgFile *f, char *buf, int len)
{
    int j;

    FwcfgSelect(f->select);
    for (j = 0; j < be32toh(f->size) && j < len - 1; j++, buf++) {
        *buf = FwcfgGetByte();
    }
    *buf = '\0';

    return j;
}

static void FwcfgStrtoll(const char *p, void *valBuf, unsigned valLen)
{
    long long v;

    v = strtoll(p, NULL, 0);
    switch (valLen) {
        case sizeof(short):
            *(short *)valBuf = (short)v;
            return;
        case sizeof(int):
            *(int *)valBuf = (int)v;
            return;
        case sizeof(long long):
            *(long long *)valBuf = v;
            return;
        default:
            *(char *)valBuf = (char)v;
    }
}

static void FwcfgArgVal(char *args, const char *argName, void *valBuf, unsigned valLen)
{
    char *p = NULL;
    char *k = NULL;
    char *context = NULL;
    int l = strlen(argName);

    for (p = args; ; p = NULL) {
        if ((k = strtok_s(p, " ", &context)) == NULL) {
            PRINT_ERR("command line: argument %s not found\n", argName);
            return;
        }

        if ((p = strchr(k, '=')) == NULL) {
            PRINT_ERR("command line: argument format error\n");
            return;
        }

        *p++ = '\0';
        if (strncmp(k, argName, l) == 0) {  /* found */
            if (*p == '\'') {
                (void)strncpy_s(valBuf, valLen, p + 1, strlen(p) - 1);
            } else {
                FwcfgStrtoll(p, valBuf, valLen);
            }
            return;
        }
    }
}

/* matched argument value is copied to input buffer, number value type is long */
void GetDebugArgs(const char *argName, void *valBuf, unsigned valLen)
{
    struct FWCfgFile f;
    char args[FW_CFG_ARGS_LEN];

    if (strlen(argName) == 0) {
        PRINT_ERR("argument name is empty\n");
        return;
    }

    if (!FwcfgIsExisted()) {
        PRINT_ERR("fw_cfg not exist\n");
        return;
    }

    if (!FwcfgFindFile(&f, FW_CFG_ARGS_FILE)) {
        PRINT_ERR("file %s not exist\n", FW_CFG_ARGS_FILE);
        return;
    }

    if (FwcfgReadFile(&f, args, FW_CFG_ARGS_LEN) == 0) {
        PRINT_ERR("file %s is empty\n", FW_CFG_ARGS_FILE);
        return;
    }

    FwcfgArgVal(args, argName, valBuf, valLen);
}
