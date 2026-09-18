/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

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

#pragma once

/* Helpers for the mmap() emulation for WASI. WASI doesn't provide
 * mmap(). We emulate a file backed mapping by reading the target
 * region into a heap buffer on map and writing it back on sync and
 * unmap. */

#ifdef __wasi__

#  include <errno.h>
#  include <stdint.h>
#  include <sys/types.h>
#  include <unistd.h>

/* Reads up to length bytes from offset. A short read is continued
 * until length bytes are read or EOF. Returns the number of read
 * bytes, which is less than length only on EOF, or -1 with errno on
 * error. */
static inline ssize_t
grn_mmap_emulation_pread(int fd, void *start, size_t length, int64_t offset)
{
  char *buffer = (char *)start;
  size_t rest = length;
  while (rest > 0) {
    ssize_t r = pread(fd, buffer, rest, (off_t)offset);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (r == 0) {
      break;
    }
    buffer += r;
    rest -= (size_t)r;
    offset += r;
  }
  return (ssize_t)(length - rest);
}

/* Writes the whole buffer to offset. A short write is
 * continued. Returns 0 on success or -1 with errno on error. errno is
 * EIO if nothing can be written. */
static inline int
grn_mmap_emulation_pwrite(int fd,
                          const void *start,
                          size_t length,
                          int64_t offset)
{
  const char *buffer = (const char *)start;
  size_t rest = length;
  while (rest > 0) {
    ssize_t r = pwrite(fd, buffer, rest, (off_t)offset);
    if (r == -1) {
      if (errno == EINTR) {
        continue;
      }
      return -1;
    }
    if (r == 0) {
      errno = EIO;
      return -1;
    }
    buffer += r;
    rest -= (size_t)r;
    offset += r;
  }
  return 0;
}

#endif /* __wasi__ */
