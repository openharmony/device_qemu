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

#include "hal_vfs.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include "errno.h"
#include "fcntl.h"
#include "los_mux.h"
#include "los_task.h"
#include "los_debug.h"
#include "limits.h"
#include "fs_config.h"

#ifdef LOSCFG_COMPONENTS_NET_LWIP
#include "lwipopts.h"
#include "lwip/sockets.h"
#endif

#ifndef CONFIG_NFILE_DESCRIPTORS
#define CONFIG_NFILE_DESCRIPTORS 256
#endif

#define LOS_FCNTL   (O_NONBLOCK | O_NDELAY | O_APPEND | O_SYNC | FASYNC)
#define IOV_MAX_CNT 4
#define BASE_PATH   10

struct file g_files[LOS_MAX_FILES];
uint32_t g_fsMutex;
uint32_t g_openfiles;
struct mount_point *g_mountPoints = NULL;
struct file_system *g_fileSystems = NULL;

int FileToFd(struct file *file)
{
    if (file == NULL) {
        return -1;
    }
    return file - g_files;
}

static struct file *FdToFile(int fd)
{
    if (fd > LOS_MAX_FD) {
        return NULL;
    }
    return &g_files[fd];
}

static struct file *LOS_FileGet(void)
{
    int i;
    /* protected by g_fsMutex */
    for (i = LOS_FD_OFFSET - 1; i < LOS_MAX_FILES; i++) {
        if (g_files[i].f_status == FILE_STATUS_NOT_USED) {
            g_files[i].f_status = FILE_STATUS_INITING;
            return &g_files[i];
        }
    }

    return NULL;
}

static struct file *LOS_FileGetNew(int fd)
{
    if (fd > LOS_MAX_FD) {
        return NULL;
    }
    if (g_files[fd].f_status == FILE_STATUS_NOT_USED) {
        g_files[fd].f_status = FILE_STATUS_INITING;
        return &g_files[fd];
    }

    return NULL;
}

static void LOS_FilePut(struct file *file)
{
    if (file == NULL) {
        return;
    }

    file->f_flags = 0;
    file->f_fops = NULL;
    file->f_data = NULL;
    file->f_mp = NULL;
    file->f_offset = 0;
    file->f_owner = (uint32_t)-1;
    file->full_path = NULL;
    file->f_status = FILE_STATUS_NOT_USED;
}

static struct mount_point *LOS_MpFind(const char *path, const char **pathInMp)
{
    struct mount_point *mp = g_mountPoints;
    struct mount_point *bestMp = NULL;
    int bestMatches = 0;
    if (path == NULL) {
        return NULL;
    }
    if (pathInMp != NULL) {
        *pathInMp = NULL;
    }
    while ((mp != NULL) && (mp->m_path != NULL)) {
        const char *m_path = mp->m_path;
        const char *i_path = path;
        const char *t = NULL;
        int matches = 0;
        do {
            while ((*m_path == '/') && (*(m_path + 1) != '/')) {
                m_path++;
            }
            while ((*i_path == '/') && (*(i_path + 1) != '/')) {
                i_path++;
            }

            t = strchr(m_path, '/');
            if (t == NULL) {
                t = strchr(m_path, '\0');
            }
            if ((t == m_path) || (t == NULL)) {
                break;
            }
            if (strncmp(m_path, i_path, (size_t)(t - m_path)) != 0) {
                goto next; /* this mount point do not match, check next */
            }

            i_path += (t - m_path);
            if ((*i_path != '\0') && (*i_path != '/')) {
                goto next;
            }

            matches += (t - m_path);
            m_path += (t - m_path);
        } while (*m_path != '\0');

        if (matches > bestMatches) {
            bestMatches = matches;
            bestMp = mp;

            while ((*i_path == '/') && (*(i_path + 1) != '/')) {
                i_path++;
            }

            if (pathInMp != NULL) {
                *pathInMp = i_path;
            }
        }
    next:
        mp = mp->m_next;
    }
    return bestMp;
}

static int LOS_FileOpened(const char *path)
{
    for (int i = LOS_FD_OFFSET; i < g_openfiles; i++) {
        if (g_files[i].f_status == FILE_STATUS_READY) {
            if (strcmp(g_files[i].full_path, path) == 0) {
                return 1;
            }
        }
    }
    return 0;
}

static int LOS_Open(const char *path, int flags)
{
    struct file *file = NULL;
    int fd = -1;
    const char *pathInMp = NULL;
    char *fullPath = NULL;
    struct mount_point *mp = NULL;

    if ((path == NULL) || (path[strlen(path) - 1] == '/') ||
       strlen(path) > LITTLEFS_MAX_LFN_LEN - BASE_PATH) {
        return fd;
    }
    if (LOS_FileOpened(path)) {
        return fd;
    }
    /* prevent fs/mp being removed while opening */
    if (LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        return fd;
    }
    file = LOS_FileGet();
    mp = LOS_MpFind(path, &pathInMp);

    (void)LOS_MuxPost(g_fsMutex);

    if ((file == NULL) || (mp == NULL) || (pathInMp == NULL) || (*pathInMp == '\0') || (mp->m_fs->fs_fops == NULL) ||
        (mp->m_fs->fs_fops->open == NULL)) {
        return fd;
    }

    if ((LOS_MuxPend(mp->m_mutex, LOS_WAIT_FOREVER) != LOS_OK)) {
        LOS_FilePut(file);
        return fd;
    }

    file->f_flags = (uint32_t)flags;
    file->f_offset = 0;
    file->f_data = NULL;
    file->f_fops = mp->m_fs->fs_fops;
    file->f_mp = mp;
    file->f_owner = LOS_CurTaskIDGet();
    file->full_path = path;

    if (file->f_fops->open(file, pathInMp, flags) == 0) {
        mp->m_refs++;
        g_openfiles++;
        fd = FileToFd(file);
        file->f_status = FILE_STATUS_READY; /* file now ready to use */
        fullPath = (char *)malloc(strlen(path) + 1);
        (void)memset_s(fullPath, strlen(path) + 1, 0, strlen(path) + 1);
        (void)memcpy_s(fullPath, strlen(path), path, strlen(path));
        g_files[fd].full_path = fullPath;
    } else {
        LOS_FilePut(file);
    }
    (void)LOS_MuxPost(mp->m_mutex);

    return fd;
}

/* attach to a file and then set new status */

static struct file *LOS_AttachFile(int fd, uint32_t status)
{
    struct file *file = NULL;

    if ((fd < 0) || (fd >= LOS_MAX_FD)) {
        VFS_ERRNO_SET(EBADF);
        return file;
    }

    file = FdToFile(fd);
    /*
     * Prevent file closed after the checking of:
     *
     * if (file->f_status == FILE_STATUS_READY)
     *
     * Because our g_files are not privated to one task, it may be operated
     * by every task.
     * So we should take the mutex of current mount point before operating it,
     * but for now we don't know if this file is valid (FILE_STATUS_READY), if
     * this file is not valid, the f_mp may be incorrect. so
     * we must check the status first, but this file may be closed/removed
     * after the checking if the senquence is not correct.
     *
     * Consider the following code:
     *
     * LOS_AttachFileReady (...)
     * {
     * if (file->f_status == FILE_STATUS_READY)
     * {
     * while (LOS_MuxPend (file->f_mp->m_mutex, LOS_WAIT_FOREVER) != LOS_OK);
     *
     * return file;
     * }
     * }
     *
     * It is not safe:
     *
     * If current task is interrupted by an IRQ just after the checking and then
     * a new task is swapped in and the new task just closed this file.
     *
     * So <g_fsMutex> is acquire first and then check if it is valid: if not, just
     * return NULL (which means fail); If yes, the mutex for current mount point
     * is qcquired. And the close operation will also set task to
     * FILE_STATUS_CLOSING to prevent other tasks operate on this file (and also
     * prevent other tasks pend on the mutex of this mount point for this file).
     * At last <g_fsMutex> is released. And return the file handle (struct file *).
     *
     * As this logic used in almost all the operation routines, this routine is
     * made to reduce the redundant code.
     */
    if ((file == NULL) || (file->f_mp == NULL)) {
        return NULL;
    }
    while (LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER) != LOS_OK) {
    };
    if (file->f_mp == NULL) {
        return file;
    }
    if (file->f_status == FILE_STATUS_READY) {
        while (LOS_MuxPend(file->f_mp->m_mutex, LOS_WAIT_FOREVER) != LOS_OK) {
        };
        if (status != FILE_STATUS_READY) {
            file->f_status = status;
        }
    } else {
        VFS_ERRNO_SET(EBADF);
        file = NULL;
    }

    (void)LOS_MuxPost(g_fsMutex);

    return file;
}

static struct file *LOS_AttachFileReady(int fd)
{
    return LOS_AttachFile(fd, FILE_STATUS_READY);
}

static struct file *LOS_AttachFileWithStatus(int fd, int status)
{
    return LOS_AttachFile(fd, (uint32_t)status);
}

static void LOS_DetachFile(const struct file *file)
{
    if ((file == NULL) || (file->f_mp == NULL)) {
        return;
    }
    (void)LOS_MuxPost(file->f_mp->m_mutex);
}

static int LOS_Close(int fd)
{
    struct file *file;
    int ret = -1;

    file = LOS_AttachFileWithStatus(fd, FILE_STATUS_CLOSING);
    if (file == NULL) {
        return ret;
    }

    if (file->full_path != NULL) {
        (void)free((void *)file->full_path);
    }
    if ((file->f_fops != NULL) && (file->f_fops->close != NULL)) {
        ret = file->f_fops->close(file);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    if ((ret == 0) && (file->f_mp != NULL)) {
        file->f_mp->m_refs--;
        g_openfiles--;
    }

    LOS_DetachFile(file);

    LOS_FilePut(file);

    return ret;
}

static ssize_t LOS_Read(int fd, char *buff, size_t bytes)
{
    struct file *file = NULL;
    ssize_t ret = (ssize_t)-1;

    if ((buff == NULL) || (bytes == 0)) {
        VFS_ERRNO_SET(EINVAL);
        return ret;
    }

    file = LOS_AttachFileReady(fd);
    if (file == NULL) {
        return ret;
    }

    if ((file->f_flags & O_ACCMODE) == O_WRONLY) {
        VFS_ERRNO_SET(EACCES);
    } else if ((file->f_fops != NULL) && (file->f_fops->read != NULL)) {
        ret = file->f_fops->read(file, buff, bytes);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    /* else ret will be -1 */

    LOS_DetachFile(file);

    return ret;
}

static ssize_t LOS_Write(int fd, const void *buff, size_t bytes)
{
    struct file *file = NULL;
    ssize_t ret = -1;

    if ((buff == NULL) || (bytes == 0)) {
        VFS_ERRNO_SET(EINVAL);
        return ret;
    }

    file = LOS_AttachFileReady(fd);
    if (file == NULL) {
        return ret;
    }

    if ((file->f_flags & O_ACCMODE) == O_RDONLY) {
        VFS_ERRNO_SET(EACCES);
    } else if ((file->f_fops != NULL) && (file->f_fops->write != NULL)) {
        ret = file->f_fops->write(file, buff, bytes);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    /* else ret will be -1 */
    LOS_DetachFile(file);

    return ret;
}

static off_t LOS_Lseek(int fd, off_t off, int whence)
{
    struct file *file;
    off_t ret = -1;

    file = LOS_AttachFileReady(fd);
    if (file == NULL) {
        return ret;
    }

    if ((file->f_fops == NULL) || (file->f_fops->lseek == NULL)) {
        ret = file->f_offset;
    } else {
        ret = file->f_fops->lseek(file, off, whence);
    }

    LOS_DetachFile(file);

    return ret;
}

static off64_t LOS_Lseek64(int fd, off64_t off, int whence)
{
    struct file *file;
    off64_t ret = -1;

    file = LOS_AttachFileReady(fd);
    if ((file == NULL) || (file->f_fops == NULL)) {
        return ret;
    }
    if (file->f_fops->lseek64 == NULL) {
        ret = file->f_offset64;
    } else {
        ret = file->f_fops->lseek64(file, off, whence);
    }

    LOS_DetachFile(file);

    return ret;
}

static int LOS_Stat(const char *path, struct stat *stat)
{
    struct mount_point *mp = NULL;
    const char *pathInMp = NULL;
    int ret = -1;

    if ((path == NULL) || (stat == NULL)) {
        VFS_ERRNO_SET(EINVAL);
        return ret;
    }

    if (LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        VFS_ERRNO_SET(EAGAIN);
        return ret;
    }

    mp = LOS_MpFind(path, &pathInMp);
    if ((mp == NULL) || (pathInMp == NULL) || (*pathInMp == '\0')) {
        VFS_ERRNO_SET(ENOENT);
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    if (mp->m_fs->fs_fops->stat != NULL) {
        ret = mp->m_fs->fs_fops->stat(mp, pathInMp, stat);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    (void)LOS_MuxPost(g_fsMutex);

    return ret;
}

static int LOS_Unlink(const char *path)
{
    struct mount_point *mp = NULL;
    const char *pathInMp = NULL;
    int ret = -1;

    if (path == NULL) {
        VFS_ERRNO_SET(EINVAL);
        return ret;
    }

    (void)LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER); /* prevent the file open/rename */

    mp = LOS_MpFind(path, &pathInMp);
    if ((mp == NULL) || (pathInMp == NULL) || (*pathInMp == '\0') || (mp->m_fs->fs_fops->unlink == NULL)) {
        VFS_ERRNO_SET(ENOENT);
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    ret = mp->m_fs->fs_fops->unlink(mp, pathInMp);

    if(ret != 0) {
        return -1;
    }
    (void)LOS_MuxPost(g_fsMutex);
    return ret;
}

static int LOS_Rename(const char *old, const char *new)
{
    struct mount_point *mpOld = NULL;
    struct mount_point *mpNew = NULL;
    const char *pathInMpOld = NULL;
    const char *pathInMpNew = NULL;
    int ret = -1;

    if ((old == NULL) || (new == NULL)) {
        VFS_ERRNO_SET(EINVAL);
        return ret;
    }

    (void)LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER); /* prevent file open/unlink */

    mpOld = LOS_MpFind(old, &pathInMpOld);

    if (pathInMpOld == NULL) {
        VFS_ERRNO_SET(EINVAL);
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    if ((mpOld == NULL) || (*pathInMpOld == '\0') || (mpOld->m_fs->fs_fops->unlink == NULL)) {
        VFS_ERRNO_SET(EINVAL);
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    mpNew = LOS_MpFind(new, &pathInMpNew);
    if ((mpNew == NULL) || (pathInMpNew == NULL) || (*pathInMpNew == '\0') || (mpNew->m_fs->fs_fops->unlink == NULL)) {
        VFS_ERRNO_SET(EINVAL);
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    if (mpOld != mpNew) {
        VFS_ERRNO_SET(EXDEV);
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    if (mpOld->m_fs->fs_fops->rename != NULL) {
        ret = mpOld->m_fs->fs_fops->rename(mpOld, pathInMpOld, pathInMpNew);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    (void)LOS_MuxPost(g_fsMutex);
    return ret;
}

static int LOS_Ioctl(int fd, int func, ...)
{
    va_list ap;
    unsigned long arg;
    struct file *file = NULL;
    int ret = -1;

    (void)va_start(ap, func);
    arg = va_arg(ap, unsigned long);
    (void)va_end(ap);

    file = LOS_AttachFileReady(fd);
    if (file == NULL) {
        return ret;
    }

    if ((file->f_fops != NULL) && (file->f_fops->ioctl != NULL)) {
        ret = file->f_fops->ioctl(file, func, arg);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    LOS_DetachFile(file);

    return ret;
}

static int LOS_Sync(int fd)
{
    struct file *file;
    int ret = -1;

    file = LOS_AttachFileReady(fd);
    if (file == NULL) {
        return ret;
    }

    if ((file->f_fops != NULL) && (file->f_fops->sync != NULL)) {
        ret = file->f_fops->sync(file);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    LOS_DetachFile(file);

    return ret;
}

static struct dir *LOS_Opendir(const char *path)
{
    struct mount_point *mp = NULL;
    const char *pathInMp = NULL;
    struct dir *dir = NULL;
    uint32_t ret;

    if (path == NULL) {
        VFS_ERRNO_SET(EINVAL);
        return NULL;
    }

    dir = (struct dir *)malloc(sizeof(struct dir));
    if (dir == NULL) {
        printf("fail to malloc memory in VFS, <malloc.c> is needed,"
            "make sure it is added\n");
        VFS_ERRNO_SET(ENOMEM);
        return NULL;
    }

    if (LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        VFS_ERRNO_SET(EAGAIN);
        (void)free(dir);
        return NULL;
    }

    mp = LOS_MpFind(path, &pathInMp);
    if ((mp == NULL) || (pathInMp == NULL)) {
        VFS_ERRNO_SET(ENOENT);
        (void)LOS_MuxPost(g_fsMutex);
        (void)free(dir);
        return NULL;
    }

    ret = LOS_MuxPend(mp->m_mutex, LOS_WAIT_FOREVER);

    (void)LOS_MuxPost(g_fsMutex);

    if ((ret != LOS_OK) || (mp->m_fs->fs_fops->opendir == NULL)) {
        VFS_ERRNO_SET(ENOTSUP);
        (void)LOS_MuxPost(mp->m_mutex);
        (void)free(dir);
        return NULL;
    }

    dir->d_mp = mp;
    dir->d_offset = 0;

    ret = (uint32_t)mp->m_fs->fs_fops->opendir(dir, pathInMp);
    if (ret == 0) {
        mp->m_refs++;
    } else {
        (void)free(dir);
        dir = NULL;
    }

    (void)LOS_MuxPost(mp->m_mutex);

    return dir;
}

static struct dirent *LOS_Readdir(struct dir *dir)
{
    struct mount_point *mp = NULL;
    struct dirent *ret = NULL;

    if ((dir == NULL) || (dir->d_mp == NULL)) {
        VFS_ERRNO_SET(EINVAL);
        return NULL;
    }

    mp = dir->d_mp;

    if (LOS_MuxPend(mp->m_mutex, LOS_WAIT_FOREVER) != LOS_OK) {
        VFS_ERRNO_SET(EAGAIN);
        return NULL;
    }

    if ((dir->d_mp->m_fs != NULL) && (dir->d_mp->m_fs->fs_fops != NULL) &&
        (dir->d_mp->m_fs->fs_fops->readdir != NULL)) {
        if (dir->d_mp->m_fs->fs_fops->readdir(dir, &dir->d_dent) == 0) {
            ret = &dir->d_dent;
        } else {
            VFS_ERRNO_SET(EBADF);
        }
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }

    (void)LOS_MuxPost(mp->m_mutex);

    return ret;
}

static int LOS_Closedir(struct dir *dir)
{
    struct mount_point *mp = NULL;
    int ret = -1;

    if ((dir == NULL) || (dir->d_mp == NULL)) {
        VFS_ERRNO_SET(EBADF);
        return ret;
    }

    mp = dir->d_mp;

    if (LOS_MuxPend(mp->m_mutex, LOS_WAIT_FOREVER) != LOS_OK) {
        VFS_ERRNO_SET(EAGAIN);
        return ret;
    }

    if ((dir->d_mp->m_fs != NULL) && (dir->d_mp->m_fs->fs_fops != NULL) &&
        (dir->d_mp->m_fs->fs_fops->closedir != NULL)) {
        ret = dir->d_mp->m_fs->fs_fops->closedir(dir);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
    }
    (void)LOS_MuxPost(mp->m_mutex);
    if (ret == 0) {
        mp->m_refs--;
    } else {
        VFS_ERRNO_SET(EBADF);
    }
    (void)free(dir);
    dir = NULL;
    return ret;
}

static int LOS_Mkdir(const char *path, int mode)
{
    struct mount_point *mp = NULL;
    const char *pathInMp = NULL;
    int ret = -1;

    (void)mode;

    if (path == NULL) {
        VFS_ERRNO_SET(EINVAL);
        return ret;
    }

    if (LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        VFS_ERRNO_SET(EAGAIN);
        return ret;
    }

    mp = LOS_MpFind(path, &pathInMp);
    if ((mp == NULL) || (pathInMp == NULL) || (*pathInMp == '\0')) {
        VFS_ERRNO_SET(ENOENT);
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    ret = (int)LOS_MuxPend(mp->m_mutex, LOS_WAIT_FOREVER);

    (void)LOS_MuxPost(g_fsMutex);

    if (ret != LOS_OK) {
        VFS_ERRNO_SET(EAGAIN);
        return -1;
    }

    if (mp->m_fs->fs_fops->mkdir != NULL) {
        ret = mp->m_fs->fs_fops->mkdir(mp, pathInMp);
    } else {
        VFS_ERRNO_SET(ENOTSUP);
        ret = -1;
    }

    (void)LOS_MuxPost(mp->m_mutex);

    return ret;
}

static int LOS_Dup(int fd)
{
    int ret;
    struct file *file1 = NULL;
    struct file *file2 = NULL;
    const char *mpath = NULL;
    struct mount_point *mp = NULL;

    if ((fd < 0) || (fd > LOS_MAX_FD)) {
        VFS_ERRNO_SET(EBADF);
        return LOS_NOK;
    }

    file1 = LOS_AttachFileReady(fd);
    if (file1 == NULL) {
        return LOS_NOK;
    }

    file2 = LOS_FileGet();
    if (file2 == NULL) {
        LOS_DetachFile(file1);
        VFS_ERRNO_SET(ENFILE);
        return LOS_NOK;
    }

    file2->f_flags = file1->f_flags;
    file2->f_status = file1->f_status;
    file2->f_offset = file1->f_offset;
    file2->f_owner = file1->f_owner;
    file2->f_data = file1->f_data;
    file2->f_fops = file1->f_fops;
    file2->f_mp = file1->f_mp;

    if ((file1->f_mp == NULL) || (file1->f_mp->m_path == NULL)) {
        LOS_DetachFile(file1);
        return LOS_NOK;
    }
    mp = LOS_MpFind(file1->f_mp->m_path, &mpath);
    if ((mp == NULL) || (file1->f_fops == NULL)) {
        LOS_DetachFile(file1);
        return LOS_NOK;
    }
    ret = file1->f_fops->open(file2, mpath, file1->f_flags);
    LOS_DetachFile(file1);
    return ret;
}

static int LOS_Dup2(int oldFd, int newFd)
{
    struct file *file1 = NULL;
    struct file *file2 = NULL;
    const char *mpath = NULL;
    struct mount_point *mp = NULL;

    file1 = LOS_AttachFileReady(oldFd);
    if (file1 == NULL) {
        return LOS_NOK;
    }

    file2 = LOS_AttachFileReady(newFd);
    if (file2 != NULL) {
        LOS_DetachFile(file1);
        LOS_DetachFile(file2);
        (void)LOS_Close(newFd);
        return LOS_NOK;
    }

    file2 = LOS_FileGetNew(newFd);
    if (file2 == NULL) {
        VFS_ERRNO_SET(ENFILE);
        printf("files no free!\n");
        LOS_DetachFile(file1);
        return LOS_NOK;
    }
    if (oldFd == newFd) {
        LOS_DetachFile(file1);
        return oldFd;
    }

    file2->f_flags = file1->f_flags;
    file2->f_status = file1->f_status;
    file2->f_offset = file1->f_offset;
    file2->f_owner = file1->f_owner;
    file2->f_data = file1->f_data;
    file2->f_fops = file1->f_fops;
    file2->f_mp = file1->f_mp;

    if ((file1->f_mp == NULL) || (file1->f_mp->m_path == NULL)) {
        LOS_DetachFile(file1);
        return LOS_NOK;
    }
    mp = LOS_MpFind(file1->f_mp->m_path, &mpath);
    if ((mp == NULL) || (file1->f_fops == NULL)) {
        LOS_DetachFile(file1);
        return LOS_NOK;
    }
    file1->f_fops->open(file2, mpath, file1->f_flags);
    LOS_DetachFile(file1);

    return newFd;
}

static int LOS_Vfcntl(struct file *filep, int cmd, va_list ap)
{
    int ret;
    int fd;
    uint32_t flags;

    if ((filep == NULL) || (filep->f_fops == NULL)) {
        return -EBADF;
    }

    if (cmd == F_DUPFD) {
        fd = FileToFd(filep);
        ret = LOS_Dup(fd);
    } else if (cmd == F_GETFL) {
        ret = (int)(filep->f_flags);
    } else if (cmd == F_SETFL) {
        flags = (uint32_t)va_arg(ap, int);
        flags &= LOS_FCNTL;
        filep->f_flags &= ~LOS_FCNTL;
        filep->f_flags |= flags;
        ret = LOS_OK;
    } else {
        ret = -ENOSYS;
    }
    return ret;
}

static int LOS_FsNameCheck(const char *name)
{
    char ch;
    int len = 0;

    do {
        ch = *(name++);

        if (ch == '\0') {
            break;
        }

        if (((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || ((ch >= '0') && (ch <= '9')) ||
            (ch == '_') || (ch == '-')) {
            len++;
            if (len == LOS_FS_MAX_NAME_LEN) {
                return LOS_NOK;
            }
            continue;
        }
    } while (1);

    return len == 0 ? LOS_NOK : LOS_OK;
}

static struct file_system *LOS_FsFind(const char *name)
{
    struct file_system *fs;

    for (fs = g_fileSystems; fs != NULL; fs = fs->fs_next) {
        if (strncmp(fs->fs_name, name, LOS_FS_MAX_NAME_LEN) == 0) {
            break;
        }
    }

    return fs;
}

int LOS_FsRegister(struct file_system *fs)
{
    if ((fs == NULL) || (fs->fs_fops == NULL) || (fs->fs_fops->open == NULL)) {
        return LOS_NOK;
    }

    if (LOS_FsNameCheck(fs->fs_name) != LOS_OK) {
        return LOS_NOK;
    }

    if (LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        return LOS_NOK;
    }

    if (LOS_FsFind(fs->fs_name) != NULL) {
        (void)LOS_MuxPost(g_fsMutex);
        return LOS_NOK;
    }

    fs->fs_next = g_fileSystems;
    g_fileSystems = fs;

    (void)LOS_MuxPost(g_fsMutex);

    return LOS_OK;
}

int LOS_FsUnregister(struct file_system *fs)
{
    struct file_system *prev = NULL;
    int ret = LOS_OK;

    if (fs == NULL) {
        return LOS_NOK;
    }

    if (LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER) != LOS_OK) {
        return LOS_NOK;
    }

    if (fs->fs_refs > 0) {
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    if (g_fileSystems == fs) {
        g_fileSystems = fs->fs_next;
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    prev = g_fileSystems;

    while (prev != NULL) {
        if (prev->fs_next == fs) {
            break;
        }

        prev = prev->fs_next;
    }

    if (prev == NULL) {
        ret = LOS_NOK;
    } else {
        prev->fs_next = fs->fs_next;
    }

    (void)LOS_MuxPost(g_fsMutex);
    return ret;
}

int LOS_FsMount(const char *fsname, const char *path, void *data)
{
    struct file_system *fs = NULL;
    struct mount_point *mp = NULL;
    const char *tmp = NULL;
    int ret;
    if ((fsname == NULL) || (path == NULL) || (path[0] != '/')) {
        return LOS_NOK;
    }
    (void)LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER);

    fs = LOS_FsFind(fsname);
    if (fs == NULL) {
        (void)LOS_MuxPost(g_fsMutex);
        return LOS_NOK;
    }

    mp = LOS_MpFind(path, &tmp);
    if ((mp != NULL) && (tmp != NULL) && (*tmp == '\0')) {
        (void)LOS_MuxPost(g_fsMutex);
        return LOS_NOK;
    }

    mp = malloc(sizeof(struct mount_point));
    if (mp == NULL) {
        printf("Fail to malloc memory in vfs\n");
        (void)LOS_MuxPost(g_fsMutex);
        return LOS_NOK;
    }

    ret = memset_s(mp, sizeof(struct mount_point), 0, sizeof(struct mount_point));
    if (ret != LOS_OK) {
        (void)free(mp);
        (void)LOS_MuxPost(g_fsMutex);
        return LOS_NOK;
    }

    mp->m_fs = fs;
    mp->m_path = path;
    mp->m_data = data;
    mp->m_refs = 0;

    if (LOS_MuxCreate(&mp->m_mutex) != LOS_OK) {
        (void)free(mp);
        (void)LOS_MuxPost(g_fsMutex);
        return LOS_NOK;
    }

    mp->m_next = g_mountPoints;
    g_mountPoints = mp;

    fs->fs_refs++;
    (void)LOS_MuxPost(g_fsMutex);

    return LOS_OK;
}

int LOS_FsUnmount(const char *path)
{
    struct mount_point *mp = NULL;
    struct mount_point *prev = NULL;
    const char *tmp = NULL;
    int ret = LOS_NOK;

    if (path == NULL) {
        return ret;
    }

    (void)LOS_MuxPend(g_fsMutex, LOS_WAIT_FOREVER);

    mp = LOS_MpFind(path, &tmp);
    if ((mp == NULL) || (tmp == NULL) || (*tmp != '\0') || (mp->m_refs != 0)) {
        (void)LOS_MuxPost(g_fsMutex);
        return ret;
    }

    if (g_mountPoints == mp) {
        g_mountPoints = mp->m_next;
    } else {
        for (prev = g_mountPoints; prev != NULL; prev = prev->m_next) {
            if (prev->m_next != mp) {
                continue;
            }

            prev->m_next = mp->m_next;
            break;
        }
    }

    (void)LOS_MuxDelete(mp->m_mutex);
    mp->m_fs->fs_refs--;
    (void)free(mp);
    (void)LOS_MuxPost(g_fsMutex);
    return LOS_OK;
}

int LOS_VfsInit(void)
{
    if (LOS_MuxCreate(&g_fsMutex) == LOS_OK) {
        return LOS_OK;
    }

    printf("Vfs init fail!\n");

    return LOS_NOK;
}


static int MapToPosixRet(int ret)
{
    return ((ret) < 0 ? -1 : (ret));
}

int EspOpen(const char *path, int flags, ...)
{
    int ret = LOS_Open(path, flags);
    return MapToPosixRet(ret);
}

int EspClose(int fd)
{
    int ret;
    if (fd < CONFIG_NFILE_DESCRIPTORS) {
        ret = LOS_Close(fd);
    } else {
        ret = -1;
    }
    return MapToPosixRet(ret);
}

ssize_t EspRead(int fd, void *buff, size_t bytes)
{
    ssize_t ret;
    if (fd < CONFIG_NFILE_DESCRIPTORS) {
        ret = LOS_Read(fd, buff, bytes);
    } else {
        ret = -1;
    }
    return MapToPosixRet(ret);
}

ssize_t EspWrite(int fd, const void *buff, size_t bytes)
{
    ssize_t ret;
    if (fd < CONFIG_NFILE_DESCRIPTORS) {
        ret = LOS_Write(fd, buff, bytes);
    } else {
        ret = -1;
    }
    return MapToPosixRet(ret);
}

off_t EspLseek(int fd, off_t off, int whence)
{
    off_t ret = LOS_Lseek(fd, off, whence);
    return MapToPosixRet(ret);
}

off64_t EspLseek64(int fd, off64_t off, int whence)
{
    off64_t ret = LOS_Lseek64(fd, off, whence);
    return MapToPosixRet(ret);
}

int EspStat(const char *path, struct stat *stat)
{
    int ret = LOS_Stat(path, stat);
    return MapToPosixRet(ret);
}

int EspUnlink(const char *path)
{
    int ret = LOS_Unlink(path);
    return MapToPosixRet(ret);
}

int EspRename(const char *oldpath, const char *newpath)
{
    int ret = LOS_Rename(oldpath, newpath);
    return MapToPosixRet(ret);
}

int EspFsync(int fd)
{
    int ret = LOS_Sync(fd);
    return MapToPosixRet(ret);
}

struct dir *EspOpendir(const char *path)
{
    return LOS_Opendir(path);
}

struct dirent *EspReaddir(struct dir *dir)
{
    return LOS_Readdir(dir);
}

int EspClosedir(struct dir *dir)
{
    int ret = LOS_Closedir(dir);
    return MapToPosixRet(ret);
}

int EspMkdir(const char *path, mode_t mode)
{
    int ret = LOS_Mkdir(path, (int)mode);
    return MapToPosixRet(ret);
}

int EspRmdir(const char *path)
{
    int ret = LOS_Unlink(path);
    return MapToPosixRet(ret);
}

int EspDup(int fd)
{
    int ret = LOS_Dup(fd);
    return MapToPosixRet(ret);
}

int EspDup2(int oldFd, int newFd)
{
    int ret = LOS_Dup2(oldFd, newFd);
    return MapToPosixRet(ret);
}

int EspLstat(const char *path, struct stat *buffer)
{
    return stat(path, buffer);
}

int EspFstat(int fd, struct stat *buf)
{
    struct file *filep;
    int ret;
    filep = LOS_AttachFileReady(fd);
    if ((filep == NULL) || (filep->f_mp == NULL) || filep->full_path == NULL) {
        return VFS_ERROR;
    }
    ret = stat(filep->full_path, buf);
    LOS_DetachFile(filep);
    return ret;
}

int EspFcntl(int fd, int cmd, ...)
{
    struct file *filep = NULL;
    va_list ap;
    int ret;
    (void)va_start(ap, cmd);

    if (fd < CONFIG_NFILE_DESCRIPTORS) {
        filep = LOS_AttachFileReady(fd);
        ret = LOS_Vfcntl(filep, cmd, ap);
        LOS_DetachFile(filep);
    } else {
        ret = -EBADF;
    }

    (void)va_end(ap);

    if (ret < 0) {
        ret = VFS_ERROR;
    }
    return ret;
}

int EspIoctl(int fd, int func, ...)
{
    int ret;
    va_list ap;
    (void)va_start(ap, func);
    if (fd < CONFIG_NFILE_DESCRIPTORS) {
        ret = LOS_Ioctl(fd, func, ap);
    } else {
        ret = -EBADF;
    }

    (void)va_end(ap);
    return ret;
}

ssize_t EspReadv(int fd, const struct iovec *iov, int iovcnt)
{
    int i;
    int ret;
    char *buf = NULL;
    char *curBuf = NULL;
    char *readBuf = NULL;
    size_t bufLen = 0;
    size_t bytesToRead;
    ssize_t totalBytesRead;
    size_t totalLen;

    if ((iov == NULL) || (iovcnt <= 0) || (iovcnt > IOV_MAX_CNT)) {
        return VFS_ERROR;
    }

    for (i = 0; i < iovcnt; ++i) {
        if ((SSIZE_MAX - bufLen) < iov[i].iov_len) {
            return VFS_ERROR;
        }
        bufLen += iov[i].iov_len;
    }
    if (bufLen == 0) {
        return VFS_ERROR;
    }
    totalLen = bufLen * sizeof(char);
    buf = (char *)malloc(totalLen);
    if (buf == NULL) {
        return VFS_ERROR;
    }

    totalBytesRead = read(fd, buf, bufLen);
    if ((size_t)totalBytesRead < totalLen) {
        totalLen = (size_t)totalBytesRead;
    }
    curBuf = buf;
    for (i = 0; i < iovcnt; ++i) {
        readBuf = (char *)iov[i].iov_base;
        bytesToRead = iov[i].iov_len;

        size_t lenToRead = totalLen < bytesToRead ? totalLen : bytesToRead;
        ret = memcpy_s(readBuf, bytesToRead, curBuf, lenToRead);
        if (ret != LOS_OK) {
            (void)free(buf);
            return VFS_ERROR;
        }
        if (totalLen < (size_t)bytesToRead) {
            break;
        }
        curBuf += bytesToRead;
        totalLen -= bytesToRead;
    }
    (void)free(buf);
    return totalBytesRead;
}

ssize_t EspWritev(int fd, const struct iovec *iov, int iovcnt)
{
    int i;
    int ret;
    char *buf = NULL;
    char *curBuf = NULL;
    char *writeBuf = NULL;
    size_t bufLen = 0;
    size_t bytesToWrite;
    ssize_t totalBytesWritten;
    size_t totalLen;

    if ((iov == NULL) || (iovcnt <= 0) || (iovcnt > IOV_MAX_CNT)) {
        return VFS_ERROR;
    }

    for (i = 0; i < iovcnt; ++i) {
        if ((SSIZE_MAX - bufLen) < iov[i].iov_len) {
            return VFS_ERROR;
        }
        bufLen += iov[i].iov_len;
    }
    if (bufLen == 0) {
        return VFS_ERROR;
    }
    totalLen = bufLen * sizeof(char);
    buf = (char *)malloc(totalLen);
    if (buf == NULL) {
        return VFS_ERROR;
    }
    curBuf = buf;
    for (i = 0; i < iovcnt; ++i) {
        writeBuf = (char *)iov[i].iov_base;
        bytesToWrite = iov[i].iov_len;
        if (((ssize_t)totalLen <= 0) || ((ssize_t)bytesToWrite <= 0)) {
            continue;
        }
        ret = memcpy_s(curBuf, totalLen, writeBuf, bytesToWrite);
        if (ret != LOS_OK) {
            (void)free(buf);
            return VFS_ERROR;
        }
        curBuf += bytesToWrite;
        totalLen -= bytesToWrite;
    }

    totalBytesWritten = write(fd, buf, bufLen);
    (void)free(buf);

    return totalBytesWritten;
}

int EspIsatty(int fd)
{
    (void)fd;
    return 0;
}

int EspAccess(const char *path, int amode)
{
    int result;
    mode_t mode;
    struct stat buf;

    result = stat(path, &buf);
    if (result != ENOERR) {
        return -1;
    }

    mode = buf.st_mode;
    if ((unsigned int)amode & R_OK) {
        if ((mode & (S_IROTH | S_IRGRP | S_IRUSR)) == 0) {
            return -1;
        }
    }
    if ((unsigned int)amode & W_OK) {
        if ((mode & (S_IWOTH | S_IWGRP | S_IWUSR)) == 0) {
            return -1;
        }
    }
    if ((unsigned int)amode & X_OK) {
        if ((mode & (S_IXOTH | S_IXGRP | S_IXUSR)) == 0) {
            return -1;
        }
    }
    return 0;
}
