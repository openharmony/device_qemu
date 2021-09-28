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

#include "hal_littlefs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lfs.h"
#include "errno.h"
#include "fcntl.h"
#include "sys/stat.h"
#include "hal_vfs.h"

typedef struct {
    lfs_t *lfs_fs;
    lfs_file_t lfs_file[LOS_MAX_FILES];
} LFS_P;

LFS_P g_lfs_p;

static lfs_t *littlefs_ptr;

static int RetToErrno(int result)
{
    int err = 0;
    switch (result) {
        case LFS_ERR_OK:
            return 0;
        case LFS_ERR_IO:
            err = EIO;
            break;
        case LFS_ERR_NOENT:
            err = ENOENT;
            break;
        case LFS_ERR_EXIST:
            err = EEXIST;
            break;
        case LFS_ERR_NOTDIR:
            err = ENOTDIR;
            break;
        case LFS_ERR_ISDIR:
            err = EISDIR;
            break;
        case LFS_ERR_NOTEMPTY:
            err = ENOTEMPTY;
            break;
        case LFS_ERR_BADF:
            err = EBADF;
            break;
        case LFS_ERR_INVAL:
            err = EINVAL;
            break;
        case LFS_ERR_NOSPC:
            err = ENOSPC;
            break;
        case LFS_ERR_NOMEM:
            err = ENOMEM;
            break;
        case LFS_ERR_CORRUPT:
            err = ELIBBAD;
            break;
        default:
            err = EIO;
            break;
    }
    VFS_ERRNO_SET(err);

    return -err;
}

static int LittlefsFlagsGet(int oflags)
{
    int flags = 0;
    switch (oflags & O_ACCMODE) {
        case O_RDONLY:
            flags |= LFS_O_RDONLY;
            break;
        case O_WRONLY:
            flags |= LFS_O_WRONLY;
            break;
        case O_RDWR:
            flags |= LFS_O_RDWR;
            break;
        default:
            break;
    }

    if (oflags & O_CREAT) {
        flags |= LFS_O_CREAT;
    }

    if ((oflags & O_EXCL)) {
        flags |= LFS_O_EXCL;
    }

    if (oflags & O_TRUNC) {
        flags |= LFS_O_TRUNC;
    }

    if (oflags & O_APPEND) {
        flags |= LFS_O_APPEND;
    }

    return flags;
}

static int LittlefsOperationOpen(struct file *file, const char *pathInMp, int flags)
{
    LFS_P *p = (LFS_P *)file->f_mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    lfs_file_t *f = &p->lfs_file[FileToFd(file)];
    if (f == lfs->mlist) {
        lfs->mlist = NULL;
    }

    int ret = lfs_file_open(lfs, f, pathInMp, LittlefsFlagsGet(flags));
    if (ret == LFS_ERR_OK) {
        file->f_data = (void *)&f;
    }

    return RetToErrno(ret);
}

static int LittlefsOperationClose(struct file *file)
{
    LFS_P *p = (LFS_P *)file->f_mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    lfs_file_t *f = &p->lfs_file[FileToFd(file)];
    int ret = lfs_file_close(lfs, f);

    return RetToErrno(ret);
}

static ssize_t LittlefsOperationRead(struct file *file, char *buff, size_t bytes)
{
    LFS_P *p = (LFS_P *)file->f_mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    lfs_file_t *f = &p->lfs_file[FileToFd(file)];
    lfs_ssize_t ret;
    ret = lfs_file_read(lfs, f, (void *)buff, (lfs_size_t)bytes);
    if (ret < 0) {
        printf("Failed to read, read size=%d\n", (int)ret);
        return RetToErrno((int)ret);
    }
    return (ssize_t)ret;
}

static ssize_t LittlefsOperationWrite(struct file *file, const char *buff, size_t bytes)
{
    LFS_P *p = (LFS_P *)file->f_mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    lfs_file_t *f = &p->lfs_file[FileToFd(file)];

    lfs_ssize_t ret;
    if ((buff == NULL) || (bytes == 0) || (lfs == NULL) || (f == NULL)) {
        return -EINVAL;
    }

    ret = lfs_file_write(lfs, f, (const void *)buff, (lfs_size_t)bytes);
    if (ret < 0) {
        return RetToErrno((int)ret);
    }

    file->f_offset = f->pos;
    return (ssize_t)ret;
}

static off_t LittlefsOperationLseek(struct file *file, off_t off, int whence)
{
    LFS_P *p = (LFS_P *)file->f_mp->m_data;
    lfs_soff_t ret;
    lfs_t *lfs = p->lfs_fs;
    lfs_file_t *f = &p->lfs_file[FileToFd(file)];
    if (f == NULL) {
        return -EINVAL;
    }
    if ((off > 0 && f->ctz.size < off) || (off < 0 && f->ctz.size < -off)) {
        return -EINVAL;
    }
    ret = lfs_file_seek(lfs, f, (lfs_soff_t)off, whence);
    if (ret < 0) {
        return RetToErrno((int)ret);
    }
    return (off_t)ret;
}

static off64_t LittlefsOperationLseek64(struct file *file, off64_t off, int whence)
{
    return (off64_t)LittlefsOperationLseek(file, (off_t)off, whence);
}

int LittlefsOperationStat(struct mount_point *mp, const char *pathInMp, struct stat *stat)
{
    struct lfs_info info;
    (void)memset_s(stat, sizeof(struct stat), 0, sizeof(struct stat));
    LFS_P *p = (LFS_P *)mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    int ret;
    ret = lfs_stat(lfs, pathInMp, &info);
    if (ret == LFS_ERR_OK) {
        stat->st_size = info.size;
        stat->st_mode = ((info.type == LFS_TYPE_DIR) ? S_IFDIR : S_IFREG);
    }
    return RetToErrno(ret);
}

static int LittlefsOperationUlink(struct mount_point *mp, const char *pathInMp)
{
    LFS_P *p = (LFS_P *)mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    int ret = lfs_remove(lfs, pathInMp);
    return RetToErrno(ret);
}

static int LittlefsOperationRename(struct mount_point *mp, const char *pathInMpOld, const char *pathInMpNew)
{
    LFS_P *p = (LFS_P *)mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    int ret = lfs_rename(lfs, pathInMpOld, pathInMpNew);
    return RetToErrno(ret);
}

static int LittlefsOperationSync(struct file *file)
{
    int ret;
    LFS_P *p = (LFS_P *)file->f_mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    lfs_file_t *f = &p->lfs_file[FileToFd(file)];
    if ((lfs == NULL) || (f == NULL)) {
        return -EINVAL;
    }

    ret = lfs_file_sync(lfs, f);
    return RetToErrno(ret);
}

static int LittlefsOperationOpendir(struct dir *dir, const char *path)
{
    int ret;
    LFS_P *p = NULL;
    lfs_dir_t *lfs_dir = NULL;
    lfs_t *lfs = NULL;

    if (dir == NULL) {
        printf("Dir is null, open failed.\n");
        return -ENOMEM;
    }

    lfs_dir = (lfs_dir_t *)malloc(sizeof(lfs_dir_t));
    (void)memset_s(lfs_dir, sizeof(lfs_dir_t), 0, sizeof(lfs_dir_t));
    p = (LFS_P *)dir->d_mp->m_data;
    lfs = p->lfs_fs;

    ret = lfs_dir_open(lfs, lfs_dir, path);
    if (ret != LFS_ERR_OK) {
        (void)free(lfs_dir);
        return RetToErrno(ret);
    }
    dir->d_data = (void *)lfs_dir;
    dir->d_offset = 0;
    return LFS_ERR_OK;
}

static int LittlefsOperationReaddir(struct dir *dir, struct dirent *dent)
{
    LFS_P *p = (LFS_P *)dir->d_mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    lfs_dir_t *lfs_dir = (lfs_dir_t *)dir->d_data;
    struct lfs_info info;
    int ret;
    (void)memset_s(&info, sizeof(struct lfs_info), 0, sizeof(struct lfs_info));
    if (lfs_dir == NULL) {
        return -EINVAL;
    }
    do {
        ret = lfs_dir_read(lfs, lfs_dir, &info);
    } while ((ret >= 0) && ((strcmp(info.name, ".") == 0) || (strcmp(info.name, "..") == 0)));
    if (ret < 0) {
        return ENOENT;
    } else {
        strncpy_s((char *)dent->name, LOS_MAX_DIR_NAME_LEN, (const char *)info.name, LOS_MAX_FILE_NAME_LEN - 1);
        dent->name[LOS_MAX_FILE_NAME_LEN - 1] = '\0';
        dent->size = info.size;
        if (info.type == LFS_TYPE_DIR) {
            dent->type = VFS_TYPE_DIR;
        } else {
            dent->type = VFS_TYPE_FILE;
        }
    }
    return LOS_OK;
}

static int LittlefsOperationClosedir(struct dir *dir)
{
    int ret;
    LFS_P *p = (LFS_P *)dir->d_mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    lfs_dir_t *lfs_dir = (lfs_dir_t *)dir->d_data;
    if (lfs_dir == NULL) {
        return -EINVAL;
    }
    ret = lfs_dir_close(lfs, lfs_dir);
    if (ret == LFS_ERR_OK) {
        (void)free(lfs_dir);
    }
    return RetToErrno(ret);
}

static int LittlefsOperationMkdir(struct mount_point *mp, const char *path)
{
    LFS_P *p = (LFS_P *)mp->m_data;
    lfs_t *lfs = p->lfs_fs;
    int ret;

    ret = lfs_mkdir(lfs, path);
    if (ret == LFS_ERR_NOENT) {
        int err;
        VFS_ERRNO_SET(ENOENT);
        err = ENOENT;
        return err;
    }
    return RetToErrno(ret);
}

static struct file_ops g_littlefsOps = {
    LittlefsOperationOpen,
    LittlefsOperationClose,
    LittlefsOperationRead,
    LittlefsOperationWrite,
    LittlefsOperationLseek,
    LittlefsOperationLseek64,
    LittlefsOperationStat,
    LittlefsOperationUlink,
    LittlefsOperationRename,
    NULL, /* ioctl not supported for now */
    LittlefsOperationSync,
    LittlefsOperationOpendir,
    LittlefsOperationReaddir,
    LittlefsOperationClosedir,
    LittlefsOperationMkdir
    };

static struct file_system g_littlefsFs = { "littlefs", &g_littlefsOps, NULL, 0 };

int LittlefsMount(const char *path, const struct lfs_config *lfsConfig)
{
    int ret = -1;
    int err;
    lfs_t *fs = NULL;

    fs = (lfs_t *)malloc(sizeof(lfs_t));

    g_lfs_p.lfs_fs = fs;

    if (fs == NULL) {
        printf("Malloc memory failed.\n");
        goto err_free;
    }

    (void)memset_s(fs, sizeof(lfs_t), 0, sizeof(lfs_t));

    err = lfs_mount(fs, lfsConfig);
    if (err == LFS_ERR_CORRUPT) {
        (void)lfs_format(fs, lfsConfig);
        err = lfs_mount(fs, lfsConfig);
    }
    if (err != LFS_ERR_OK) {
        printf("Format fail.\n");
        goto err_unmount;
    }

    ret = LOS_FsMount("littlefs", path, &g_lfs_p);
    if (ret == LFS_ERR_OK) {
        printf("Littlefs mount at %s done.\n", path);
        littlefs_ptr = fs;
        return LFS_ERR_OK;
    }
    printf("Failed to mount.\n");
err_unmount:
    lfs_unmount(fs);
err_free:
    if (fs != NULL) {
        (void)free(fs);
    }
    return ret;
}

int LittlefsUnmout(const char *path)
{
    int ret = LFS_ERR_OK;
    if (littlefs_ptr != NULL) {
        ret = lfs_unmount(littlefs_ptr);
        free(littlefs_ptr);
        littlefs_ptr = NULL;
    }
    (void)LOS_FsUnmount(path);
    return ret;
}

int LittlefsInit(int needErase, const struct lfs_config *lfsConfig)
{
    int ret;
    static int littlefsInited = FALSE;
    if (littlefsInited) {
        return LOS_OK;
    }
    if (LOS_VfsInit() != LOS_OK) {
        return LOS_NOK;
    }
    if (LOS_FsRegister(&g_littlefsFs) != LOS_OK) {
        printf("Failed to register fs.\n");
        return LOS_NOK;
    }
    printf("Register littlefs done.\n");

    LittlefsDriverInit(needErase);

    ret = LittlefsMount("/littlefs/", lfsConfig);
    if (ret == LFS_ERR_OK) {
        littlefsInited = TRUE;
        return LOS_OK;
    }

    return ret;
}
