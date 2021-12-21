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

#include "hal_file.h"
#include "securec.h"
#include "sys/stat.h"
#include "fs_config.h"

int HalFileOpen(const char* path, int oflag, int mode)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN] = {0};
    (void)snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "/littlefs/%s", path);
    return open(tmpPath, oflag, mode);
}

int HalFileClose(int fd)
{
    return close(fd);
}

int HalFileRead(int fd, char *buf, unsigned int len)
{
    return read(fd, buf, len);
}

int HalFileWrite(int fd, const char *buf, unsigned int len)
{
    return write(fd, buf, len);
}

int HalFileDelete(const char *path)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN] = {0};
    (void)snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "/littlefs/%s", path);
    return unlink(path);
}

int HalFileStat(const char *path, unsigned int *fileSize)
{
    char tmpPath[LITTLEFS_MAX_LFN_LEN] = {0};
    struct stat halStat ;
    int ret = 0;
    (void)snprintf_s(tmpPath, LITTLEFS_MAX_LFN_LEN, LITTLEFS_MAX_LFN_LEN, "/littlefs/%s", path);
    ret = stat(tmpPath, &halStat);
    *fileSize = halStat.st_size;
    return ret;
}

int HalFileSeek(int fd, int offset, unsigned int whence)
{
    return lseek(fd, (off_t)offset, whence);
}
