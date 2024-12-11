/*
  Copyright(C) 2017-2018 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

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
#else /* WIN32 */
# include <sys/types.h>
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

bool
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
    file_lock->fd = open(file_lock->path, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
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
    return false;
  }
  return true;
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
