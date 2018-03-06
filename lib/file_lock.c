/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017-2018 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn_file_lock.h"
#include "grn_ctx.h"

#include <sys/stat.h>

#ifdef WIN32
# include <io.h>
# include <share.h>
# define TESTSTRLEN 11 // see http://bit.ly/2oRb3Ok
#else /* WIN32 */
# include <sys/types.h>
# include <sys/file.h>
# include <fcntl.h>
#endif /* WIN32 */

#ifdef WIN32
# define GRN_FILE_LOCK_IS_INVALID(file_lock)       \
  ((file_lock)->handle == INVALID_HANDLE_VALUE)
#else /* WIN32 */
# define GRN_FILE_LOCK_IS_INVALID(file_lock)    \
  ((file_lock)->fd == -1)
#endif /* WIN32 */

void
grn_file_lock_init(grn_ctx *ctx,
                   grn_file_lock *file_lock,
                   const char *path)
{
  grn_strcpy(file_lock->path, PATH_MAX, path);
#ifdef WIN32
  file_lock->handle = INVALID_HANDLE_VALUE;
#else /* WIN32 */
  file_lock->fd = -1;
#endif /* WIN32 */
}

grn_bool
grn_file_lock_acquire(grn_ctx *ctx,
                      grn_file_lock *file_lock,
                      int timeout,
                      const char *error_message_tag)
{
  int i;
  int n_lock_tries = timeout;

  for (i = 0; i < n_lock_tries; i++) {
#ifdef WIN32
    file_lock->handle = CreateFile(file_lock->path,
                                   GENERIC_READ | GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL,
                                   NULL);
#else /* WIN32 */
    file_lock->fd = open(file_lock->path,
        O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
#endif
    if (!GRN_FILE_LOCK_IS_INVALID(file_lock)) {
      break;
    }
    grn_nanosleep(GRN_LOCK_WAIT_TIME_NANOSECOND);
  }

  if (GRN_FILE_LOCK_IS_INVALID(file_lock)) {
    ERR(GRN_NO_LOCKS_AVAILABLE,
        "%s failed to acquire lock: <%s>",
        error_message_tag, file_lock->path);
    return GRN_FALSE;
  } else {
    return GRN_TRUE;
  }
}

void
grn_file_lock_release(grn_ctx *ctx, grn_file_lock *file_lock)
{
  if (GRN_FILE_LOCK_IS_INVALID(file_lock)) {
    return;
  }

#ifdef WIN32
  CloseHandle(file_lock->handle);
  DeleteFile(file_lock->path);

  file_lock->handle = INVALID_HANDLE_VALUE;
#else /* WIN32 */
  close(file_lock->fd);
  unlink(file_lock->path);

  file_lock->fd = -1;
#endif /* WIN32 */
  grn_strcpy(file_lock->path, PATH_MAX, "");
}

void
grn_file_lock_fin(grn_ctx *ctx, grn_file_lock *file_lock)
{
  if (!GRN_FILE_LOCK_IS_INVALID(file_lock)) {
    grn_file_lock_release(ctx, file_lock);
  }
}

grn_bool
grn_file_lock_exist(grn_ctx *ctx, grn_file_lock *file_lock)
{
#ifdef WIN32
  return GetFileAttributes(file_lock->path) != INVALID_FILE_ATTRIBUTES;
#else
  return access(file_lock->path, F_OK) == 0;
#endif
}

grn_bool
grn_file_lock_takeover(grn_ctx *ctx, grn_file_lock *file_lock)
{
#ifdef WIN32
  file_lock->handle = OpenFile(file_lock->path, NULL, OF_READWRITE);
#else
  file_lock->fd = open(file_lock->path, O_RDWR);
#endif
  if (GRN_FILE_LOCK_IS_INVALID(file_lock)) return GRN_FALSE;
#ifdef WIN32
  if (!LockFileEx(file_lock->handle,
      LOCKFILE_EXCLUSIVE_LOCK|LOCKFILE_FAIL_IMMEDIATELY,
      0, TESTSTRLEN, 0, 0)) {
#else
  if (flock(file_lock->fd, LOCK_EX|LOCK_NB) != 0) {
#endif
    grn_file_lock_abandon(ctx, file_lock);
    return GRN_FALSE;
  }
  return GRN_TRUE;
}

void
grn_file_lock_abandon(grn_ctx *ctx, grn_file_lock *file_lock)
{
  if (GRN_FILE_LOCK_IS_INVALID(file_lock)) {
    return;
  }
#ifdef WIN32
  CloseHandle(file_lock->handle);
  file_lock->handle = INVALID_HANDLE_VALUE;
#else /* WIN32 */
  close(file_lock->fd);
  file_lock->fd = -1;
#endif /* WIN32 */
  grn_strcpy(file_lock->path, PATH_MAX, "");
}

void
grn_file_lock_exclusive(grn_ctx *ctx, grn_file_lock *file_lock)
{
#ifdef WIN32
  LockFileEx(file_lock->handle,
      LOCKFILE_EXCLUSIVE_LOCK, 0, TESTSTRLEN, 0, 0);
#else
  flock(file_lock->fd, LOCK_EX);
#endif
}

grn_bool
grn_file_lock_read(grn_ctx *ctx, grn_file_lock *file_lock,
    void *buf, size_t len)
{
#ifdef WIN32
  SetFilePointer(file_lock->hFile, 0, NULL, FILE_BEGIN);
#else
  lseek(file_lock->fd, 0, SEEK_SET);
#endif
  while (len > 0) {
#ifdef WIN32
    DWORD dwLen;
    if (!ReadFile(file_lock->hFile, buf, len, &dwLen, NULL)) {
      return GRN_FALSE;
    }
    ssize_t ret = dwLen;
#else
    ssize_t ret = read(file_lock->fd, buf, len);
    if (ret == EAGAIN) continue;
#endif
    if (ret > 0) {
      len -= ret;
      buf = (char*)buf + ret;
    } else {
      return GRN_FALSE;
    }
  }
  return GRN_TRUE;
}

grn_bool
grn_file_lock_write(grn_ctx *ctx, grn_file_lock *file_lock,
    void *buf, size_t len)
{
#ifdef WIN32
  SetFilePointer(file_lock->hFile, 0, NULL, FILE_BEGIN);
#else
  lseek(file_lock->fd, 0, SEEK_SET);
#endif
  while (len > 0) {
#ifdef WIN32
    if (!WriteFile(file_lock->hFile, buf, len, &dwLen, NULL)) {
      return GRN_FALSE;
    }
    ssize_t ret = dwLen;
#else
    ssize_t ret = write(file_lock->fd, buf, len);
    if (ret == EAGAIN) continue;
#endif
    if (ret > 0) {
      len -= ret;
      buf = (char*)buf + ret;
    } else {
      return GRN_FALSE;
    }
  }
#ifdef WIN32
  FlushFileBuffers(file_lock->hFile);
#else
  fsync(file_lock->fd);
#endif
  return GRN_TRUE;
}
