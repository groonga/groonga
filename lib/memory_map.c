/*
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"

#include "grn_ctx.h"
#include "grn_error.h"

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/mman.h>
#  include <sys/stat.h>
#  include <sys/types.h>
#endif

struct grn_memory_map {
  char *path;
  void *address;
  uint64_t offset;
  size_t length;
#ifdef _WIN32
  HANDLE file;
  HANDLE mapping;
#else
  int fd;
#endif
};

#ifdef _WIN32
/* TODO: Not tested. */
grn_memory_map *
grn_memory_map_open(grn_ctx *ctx,
                    const char *path,
                    grn_memory_map_flags flags,
                    uint64_t offset,
                    size_t length)
{
  const char *tag = "[memory-map][open]";

  grn_memory_map *map = GRN_MALLOC(sizeof(grn_memory_map));
  if (!map) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
        "): %s",
        tag,
        path,
        length,
        offset,
        message);
    GRN_API_RETURN(NULL);
  }

  map->offset = offset;
  map->length = length;

  {
    /* CreateFileMapping() doesn't provide write only mode. We always
     * need read access. */
    DWORD dwDesiredAccess = GENERIC_READ;
    if (flags & GRN_MEMORY_MAP_WRITE) {
      dwDesiredAccess |= GENERIC_WRITE;
    }
    DWORD dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
    DWORD dwCreationDisposition;
    if ((flags & GRN_MEMORY_MAP_WRITE) && (flags & GRN_MEMORY_MAP_EXCLUDE)) {
      dwCreationDisposition = CREATE_NEW;
    } else {
      dwCreationDisposition = OPEN_ALWAYS;
    }
    DWORD dwFlagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
    map->file = CreateFile(path,
                           dwDesiredAccess,
                           dwShareMode,
                           NULL,
                           dwCreationDisposition,
                           dwFlagsAndAttributes,
                           NULL);
    if (map->file == INVALID_HANDLE_VALUE) {
      SERR("%s failed to open: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
           "): %cread|%cwrite|%cexclude",
           tag,
           path,
           length,
           offset,
           (flags & GRN_MEMORY_MAP_READ) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_WRITE) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_EXCLUDE) ? '+' : '-');
      GRN_FREE(map);
      GRN_API_RETURN(NULL);
    }
  }

  {
    DWORD flProtect = 0;
    if (flags & GRN_MEMORY_MAP_WRITE) {
      flProtect = PAGE_READWRITE;
    } else {
      flProtect = PAGE_READONLY;
    }
    size_t size = offset + length;
    DWORD dwMaximumSizeHigh = size >> 32;
    DWORD dwMaximumSizeLow = size & UINT32_MAX;
    map->mapping = CreateFileMapping(map->file,
                                     NULL,
                                     flProtect,
                                     dwMaximumSizeHigh,
                                     dwMaximumSizeLow,
                                     NULL);
    if (!map->mapping) {
      SERR("%s failed to create mapping: <%s> (%" GRN_FMT_SIZE
           "+%" GRN_FMT_INT64U "): %cread|%cwrite|%cexclude",
           tag,
           path,
           length,
           offset,
           (flags & GRN_MEMORY_MAP_READ) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_WRITE) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_EXCLUDE) ? '+' : '-');
      CloseHandle(map->file);
      GRN_FREE(map);
      GRN_API_RETURN(NULL);
    }
  }

  {
    DWORD dwDesiredAccess = 0;
    if (flags & GRN_MEMORY_MAP_WRITE) {
      dwDesiredAccess |= FILE_MAP_WRITE;
    } else {
      dwDesiredAccess |= FILE_MAP_READ;
    }
    DWORD dwFileOffsetHigh = offset >> 32;
    DWORD dwFileOffsetLow = offset & UINT32_MAX;
    map->address = MapViewOfFile(map->mapping,
                                 dwDesiredAccess,
                                 dwFileOffsetHigh,
                                 dwFileOffsetLow,
                                 length);
    if (!map->address) {
      SERR("%s failed to map: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
           "): %cread|%cwrite|%cexclude",
           tag,
           path,
           length,
           offset,
           (flags & GRN_MEMORY_MAP_READ) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_WRITE) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_EXCLUDE) ? '+' : '-');
      CloseHandle(map->mapping);
      CloseHandle(map->file);
      GRN_FREE(map);
      GRN_API_RETURN(NULL);
    }
  }

  map->path = GRN_STRDUP(path);
  GRN_API_RETURN(map);
}

void
grn_memory_map_close(grn_ctx *ctx, grn_memory_map *map)
{
  const char *tag = "[memory-map][close]";

  GRN_API_ENTER;
  if (!FlushViewOfFile(map->address, map->length)) {
    SERR("%s failed to flush: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U ")",
         tag,
         map->path,
         map->length,
         map->offset);
  }
  if (!UnmapViewOfFile(map->address)) {
    SERR("%s failed to unmap: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U ")",
         tag,
         map->path,
         map->length,
         map->offset);
  }
  if (!CloseHandle(map->mapping)) {
    SERR("%s failed to close mapping: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
         ")",
         tag,
         map->path,
         map->length,
         map->offset);
  }
  if (!CloseHandle(map->file)) {
    SERR("%s failed to close: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U ")",
         tag,
         map->path,
         map->length,
         map->offset);
  }
  GRN_FREE(map->path);
  GRN_FREE(map);
  GRN_API_RETURN();
}
#else
/* TODO: offset must be a multiple of the page size as returned by
 * sysconf(_SC_PAGE_SIZE). */
grn_memory_map *
grn_memory_map_open(grn_ctx *ctx,
                    const char *path,
                    grn_memory_map_flags flags,
                    uint64_t offset,
                    size_t length)
{
  const char *tag = "[memory-map][open]";

  GRN_API_ENTER;

  grn_memory_map *map = GRN_MALLOC(sizeof(grn_memory_map));
  if (!map) {
    char message[GRN_CTX_MSGSIZE];
    grn_strcpy(message, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s failed to allocate: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
        "): %s",
        tag,
        path,
        length,
        offset,
        message);
    GRN_API_RETURN(NULL);
  }

  map->offset = offset;
  map->length = length;

  int open_flags = 0;
  if ((flags & GRN_MEMORY_MAP_READ) && (flags & GRN_MEMORY_MAP_WRITE)) {
    open_flags |= O_RDWR | O_CREAT;
  } else if (flags & GRN_MEMORY_MAP_READ) {
    open_flags |= O_RDONLY;
  } else if (flags & GRN_MEMORY_MAP_WRITE) {
    open_flags |= O_WRONLY | O_CREAT;
  }
  if ((open_flags & O_CREAT) && (flags & GRN_MEMORY_MAP_EXCLUDE)) {
    open_flags |= O_EXCL;
  }
  grn_open(map->fd, path, open_flags);
  if (map->fd == -1) {
    SERR("%s failed to open: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
         "): %cread|%cwrite|%cexclude",
         tag,
         path,
         length,
         offset,
         (flags & GRN_MEMORY_MAP_READ) ? '+' : '-',
         (flags & GRN_MEMORY_MAP_WRITE) ? '+' : '-',
         (flags & GRN_MEMORY_MAP_EXCLUDE) ? '+' : '-');
    GRN_FREE(map);
    GRN_API_RETURN(NULL);
  }

  if (flags & GRN_MEMORY_MAP_WRITE) {
    struct stat s;
    if (fstat(map->fd, &s) == -1) {
      SERR("%s failed to stat: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
           "): %cread|%cwrite|%cexclude",
           tag,
           path,
           length,
           offset,
           (flags & GRN_MEMORY_MAP_READ) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_WRITE) ? '+' : '-',
           (flags & GRN_MEMORY_MAP_EXCLUDE) ? '+' : '-');
      close(map->fd);
      GRN_FREE(map);
      GRN_API_RETURN(NULL);
    }

    off64_t total_size = offset + length;
    if (s.st_size < total_size) {
      if (ftruncate(map->fd, total_size) == -1) {
        SERR("%s failed to expand: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U
             "): %cread|%cwrite|%cexclude",
             tag,
             path,
             length,
             offset,
             (flags & GRN_MEMORY_MAP_READ) ? '+' : '-',
             (flags & GRN_MEMORY_MAP_WRITE) ? '+' : '-',
             (flags & GRN_MEMORY_MAP_EXCLUDE) ? '+' : '-');
        close(map->fd);
        GRN_FREE(map);
        GRN_API_RETURN(NULL);
      }
    }
  }

  int mmap_prot = 0;
  int mmap_flags = MAP_SHARED;
  if (flags & GRN_MEMORY_MAP_READ) {
    mmap_prot |= PROT_READ;
  }
  if (flags & GRN_MEMORY_MAP_WRITE) {
    mmap_prot |= PROT_WRITE;
  }
  map->address = mmap(NULL, length, mmap_prot, mmap_flags, map->fd, offset);
  if (map->address == MAP_FAILED) {
    SERR("%s failed to mmap: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U ")",
         tag,
         path,
         length,
         offset);
    close(map->fd);
    GRN_FREE(map);
    GRN_API_RETURN(NULL);
  }

  map->path = GRN_STRDUP(path);
  GRN_API_RETURN(map);
}

void
grn_memory_map_close(grn_ctx *ctx, grn_memory_map *map)
{
  const char *tag = "[memory-map][close]";

  GRN_API_ENTER;
  if (munmap(map->address, map->length) == -1) {
    SERR("%s failed to munmap: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U ")",
         tag,
         map->path,
         map->length,
         map->offset);
  }
  if (close(map->fd) == -1) {
    SERR("%s failed to close: <%s> (%" GRN_FMT_SIZE "+%" GRN_FMT_INT64U ")",
         tag,
         map->path,
         map->length,
         map->offset);
  }
  GRN_FREE(map->path);
  GRN_FREE(map);
  GRN_API_RETURN();
}
#endif

void *
grn_memory_map_get_address(grn_ctx *ctx, grn_memory_map *map)
{
  return map->address;
}
