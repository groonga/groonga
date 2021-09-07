/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>

#include "grn_ctx.h"
#include "grn_io.h"
#include "grn_plugin.h"
#include "grn_hash.h"
#include "grn_ctx_impl.h"
#include "grn_util.h"

#ifdef WIN32
# include <io.h>
# include <share.h>
#endif /* WIN32 */

#define GRN_IO_IDSTR "GROONGA:IO:00001"
#define GRN_IO_IDSTR_LEN (sizeof(GRN_IO_IDSTR) - 1)

#define GRN_IO_VERSION_DEFAULT 1

#define GRN_IO_FILE_SIZE_V1 1073741824UL

#ifdef WIN32
# define GRN_IO_FILE_SIZE_V0  134217728L
#else /* WIN32 */
# define GRN_IO_FILE_SIZE_V0  GRN_IO_FILE_SIZE_V1
#endif /* WIN32 */

#ifndef WIN32
# ifdef HAVE_FUTIMENS
#  define grn_futimens(fd, time_values, time_specs) futimens((fd), (time_specs))
#  define grn_futimens_name "futimens"
# elif defined(HAVE_FUTIMES) /* HAVE_FUTIMENS */
#  define grn_futimens(fd, time_values, time_specs) futimes((fd), (time_values))
#  define grn_futimens_name "futimes"
# else /* HAVE_FUTIMENS */
#  error Support for either the futimens() or futimens() function is required
# endif /* HAVE_FUTIMENS */
#endif /* WIN32 */

typedef struct _grn_io_fileinfo {
  char *path;
#ifdef WIN32
  HANDLE fh;
  HANDLE fmo;
  grn_critical_section cs;
#else /* WIN32 */
  int fd;
  dev_t dev;
  ino_t inode;
#endif /* WIN32 */
} fileinfo;

#define IO_HEADER_SIZE 64

static uint32_t grn_io_version_default = GRN_IO_VERSION_DEFAULT;

grn_inline static grn_rc grn_fileinfo_open(grn_ctx *ctx, fileinfo *fi,
                                       const char *path, int flags);
grn_inline static void grn_fileinfo_init(fileinfo *fis, int nfis);
grn_inline static int grn_fileinfo_opened(fileinfo *fi);
grn_inline static grn_rc grn_fileinfo_close(grn_ctx *ctx, fileinfo *fi);
#ifdef WIN32
grn_inline static void *grn_mmap(grn_ctx *ctx,
                                 grn_ctx *owner_ctx,
                                 grn_io *io,
                                 HANDLE *fmo,
                                 fileinfo *fi,
                                 int64_t offset,
                                 size_t length,
                                 const char *file,
                                 int line,
                                 const char *func);
grn_inline static int grn_munmap(grn_ctx *ctx, grn_ctx *owner_ctx,
                                 grn_io *io, HANDLE *fmo, fileinfo *fi,
                                 void *start, size_t length);
# define GRN_MMAP(ctx,owner_ctx,io,fmo,fi,offset,length)\
  (grn_mmap((ctx), (owner_ctx), (io), (fmo), (fi), (offset), (length),\
            __FILE__, __LINE__, __FUNCTION__))
# define GRN_MUNMAP(ctx,owner_ctx,io,fmo,fi,start,length)\
  (grn_munmap((ctx), (owner_ctx), (io), (fmo), (fi), (start), (length)))
#else /* WIN32 */
grn_inline static void *grn_mmap(grn_ctx *ctx,
                                 grn_ctx *owner_ctx,
                                 grn_io *io,
                                 fileinfo *fi,
                                 int64_t offset,
                                 size_t length,
                                 const char *file,
                                 int line,
                                 const char *func);
grn_inline static int grn_munmap(grn_ctx *ctx, grn_ctx *owner_ctx,
                                 grn_io *io, fileinfo *fi,
                                 void *start, size_t length);
# define GRN_MMAP(ctx,owner_ctx,io,fmo,fi,offset,length)\
  (grn_mmap((ctx), (owner_ctx), (io), (fi), (offset), (length),\
            __FILE__, __LINE__, __FUNCTION__))
# define GRN_MUNMAP(ctx,owner_ctx,io,fmo,fi,start,length) \
  (grn_munmap((ctx), (owner_ctx), (io), (fi), (start), (length)))
#endif  /* WIN32 */

grn_inline static int grn_msync(grn_ctx *ctx,
                                fileinfo *fi,
                                void *start,
                                size_t length);
# define GRN_MSYNC(ctx,fi,start,length) \
  (grn_msync((ctx), (fi), (start), (length)))

grn_inline static grn_rc grn_pread(grn_ctx *ctx, fileinfo *fi, void *buf,
                                   size_t count, off_t offset);
grn_inline static grn_rc grn_pwrite(grn_ctx *ctx, fileinfo *fi, void *buf,
                                    size_t count, off_t offset);

void
grn_io_init_from_env(void)
{
  {
    char version_env[GRN_ENV_BUFFER_SIZE];

    grn_getenv("GRN_IO_VERSION",
               version_env,
               GRN_ENV_BUFFER_SIZE);
    if (version_env[0]) {
      grn_io_version_default = atoi(version_env);
    }
  }
}

static grn_inline uint32_t
grn_io_compute_base(uint32_t header_size)
{
  uint32_t total_header_size;
  total_header_size = IO_HEADER_SIZE + header_size;
  return (total_header_size + grn_pagesize - 1) & ~(grn_pagesize - 1);
}

static grn_inline uint32_t
grn_io_compute_base_segment(uint32_t base, uint32_t segment_size)
{
  return (base + segment_size - 1) / segment_size;
}

static uint32_t
grn_io_compute_max_n_files(uint32_t segment_size, uint32_t max_segment,
                           unsigned int base_segument, unsigned long file_size)
{
  uint64_t last_segment_end;
  last_segment_end = ((uint64_t)segment_size) * (max_segment + base_segument);
  return (uint32_t)((last_segment_end + file_size - 1) / file_size);
}

static grn_inline unsigned long
grn_io_compute_file_size(uint32_t version)
{
  if (version == 0) {
    return GRN_IO_FILE_SIZE_V0;
  } else {
    return GRN_IO_FILE_SIZE_V1;
  }
}

static grn_inline uint32_t
grn_io_max_segment(grn_io *io)
{
  if (io->header->segment_tail) {
    return io->header->segment_tail;
  } else {
    return io->header->max_segment;
  }
}

static uint32_t
grn_io_max_n_files(grn_io *io)
{
  unsigned long file_size;

  file_size = grn_io_compute_file_size(io->header->version);
  return grn_io_compute_max_n_files(io->header->segment_size,
                                    grn_io_max_segment(io),
                                    io->base_seg,
                                    file_size);
}

static grn_inline uint32_t
grn_io_compute_nth_file_info(grn_io *io, uint32_t nth_segment)
{
  uint32_t segment_size;
  unsigned long file_size;
  uint32_t segments_per_file;
  uint32_t resolved_nth_segment;

  segment_size = io->header->segment_size;
  file_size = grn_io_compute_file_size(io->header->version);
  segments_per_file = file_size / segment_size;
  resolved_nth_segment = nth_segment + io->base_seg;
  return resolved_nth_segment / segments_per_file;
}

static grn_io *
grn_io_create_tmp(grn_ctx *ctx, uint32_t header_size, uint32_t segment_size,
                  uint32_t max_segment, grn_io_mode mode, uint32_t flags)
{
  grn_io *io;
  uint32_t b;
  struct _grn_io_header *header;
  b = grn_io_compute_base(header_size);
  header = (struct _grn_io_header *)GRN_MMAP(ctx, &grn_gctx, NULL, NULL, NULL,
                                             0, b);
  if (header) {
    header->version = grn_io_version_default;
    header->header_size = header_size;
    header->segment_size = segment_size;
    header->max_segment = max_segment;
    header->n_arrays = 0;
    header->flags = flags;
    header->lock = 0;
    grn_memcpy(header->idstr, GRN_IO_IDSTR, 16);
    if ((io = GRN_CALLOC(sizeof(grn_io)))) {
      grn_io_mapinfo *maps = NULL;
      if ((maps = GRN_CALLOC(sizeof(grn_io_mapinfo) * max_segment))) {
        io->header = header;
        io->user_header = (((byte *) header) + IO_HEADER_SIZE);
        io->maps = maps;
        io->base = b;
        io->base_seg = 0;
        io->mode = mode;
        io->header->curr_size = b;
        io->fis = NULL;
        io->ainfo = NULL;
        io->max_map_seg = 0;
        io->nmaps = 0;
        io->count = 0;
        io->flags = GRN_IO_TEMPORARY;
        io->lock = &header->lock;
        io->path[0] = '\0';
        return io;
      }
      GRN_FREE(io);
    }
    GRN_MUNMAP(ctx, &grn_gctx, NULL, NULL, NULL, header, b);
  }
  return NULL;
}

static void
grn_io_register(grn_ctx *ctx, grn_io *io)
{
  if (io->fis && (io->flags & (GRN_IO_EXPIRE_GTICK|GRN_IO_EXPIRE_SEGMENT))) {
    grn_bool succeeded = GRN_FALSE;
    CRITICAL_SECTION_ENTER(grn_glock);
    if (grn_gctx.impl && grn_gctx.impl->ios &&
        grn_hash_add(&grn_gctx, grn_gctx.impl->ios, io->path, strlen(io->path),
                     (void **)&io, NULL)) {
      succeeded = GRN_TRUE;
    }
    CRITICAL_SECTION_LEAVE(grn_glock);
    if (!succeeded) {
      GRN_LOG(ctx, GRN_LOG_WARNING,
              "grn_io_register(%s) failed", io->path);
    }
  }
}

static void
grn_io_unregister(grn_ctx *ctx, grn_io *io)
{
  if (io->fis && (io->flags & (GRN_IO_EXPIRE_GTICK|GRN_IO_EXPIRE_SEGMENT))) {
    grn_bool succeeded = GRN_FALSE;
    CRITICAL_SECTION_ENTER(grn_glock);
    if (grn_gctx.impl && grn_gctx.impl->ios) {
      grn_hash_delete(&grn_gctx, grn_gctx.impl->ios,
                      io->path, strlen(io->path), NULL);
      succeeded = GRN_TRUE;
    }
    CRITICAL_SECTION_LEAVE(grn_glock);
    if (!succeeded) {
      GRN_LOG(ctx, GRN_LOG_WARNING,
              "grn_io_unregister(%s) failed", io->path);
    }
  }
}

grn_io *
grn_io_create(grn_ctx *ctx, const char *path, uint32_t header_size,
              uint32_t segment_size, uint32_t max_segment, grn_io_mode mode,
              uint32_t flags)
{
  grn_io *io;
  fileinfo *fis;
  size_t path_length;
  uint32_t b, max_nfiles;
  uint32_t bs;
  struct _grn_io_header *header;
  uint32_t version = grn_io_version_default;
  unsigned long file_size;

  if (!path) {
    return grn_io_create_tmp(ctx, header_size, segment_size, max_segment,
                             mode, flags);
  }
  path_length = strlen(path);
  if (path_length == 0 || (path_length > PATH_MAX - 4)) { return NULL; }
  b = grn_io_compute_base(header_size);
  bs = grn_io_compute_base_segment(b, segment_size);
  file_size = grn_io_compute_file_size(version);
  max_nfiles = grn_io_compute_max_n_files(segment_size, max_segment,
                                          bs, file_size);
  if ((fis = GRN_MALLOCN(fileinfo, max_nfiles))) {
    grn_fileinfo_init(fis, max_nfiles);
    if (!grn_fileinfo_open(ctx, fis, path, O_RDWR|O_CREAT|O_EXCL)) {
      header = (struct _grn_io_header *)GRN_MMAP(ctx, &grn_gctx, NULL,
                                                 &fis->fmo, fis, 0, b);
      if (header) {
        header->version = version;
        header->header_size = header_size;
        header->segment_size = segment_size;
        header->max_segment = max_segment;
        header->n_arrays = 0;
        header->flags = flags;
        header->lock = 0;
        grn_memcpy(header->idstr, GRN_IO_IDSTR, 16);
        GRN_MSYNC(ctx, &(fis[0]), header, b);
        if ((io = GRN_CALLOC(sizeof(grn_io)))) {
          grn_io_mapinfo *maps = NULL;
          if ((maps = GRN_CALLOC(sizeof(grn_io_mapinfo) * max_segment))) {
            grn_strncpy(io->path, PATH_MAX, path, path_length + 1);
            io->header = header;
            io->user_header = (((byte *) header) + IO_HEADER_SIZE);
            io->maps = maps;
            io->base = b;
            io->base_seg = bs;
            io->mode = mode;
            io->header->curr_size = b;
            io->fis = fis;
            io->ainfo = NULL;
            io->max_map_seg = 0;
            io->nmaps = 0;
            io->count = 0;
            io->flags = flags;
            io->lock = &header->lock;
            grn_io_register(ctx, io);
            return io;
          }
          GRN_FREE(io);
        }
        GRN_MUNMAP(ctx, &grn_gctx, NULL, &fis->fmo, fis, header, b);
      }
      grn_fileinfo_close(ctx, fis);
      if (grn_unlink(path) == 0) {
        GRN_LOG(ctx, GRN_LOG_INFO,
                "[io][create][error] removed path: <%s>", path);
      } else {
        ERRNO_ERR("[io][create][error] failed to remove path: <%s>", path);
      }
    }
    GRN_FREE(fis);
  }
  return NULL;
}

static grn_rc
array_init_(grn_ctx *ctx, grn_io *io, int n_arrays, size_t hsize, size_t msize)
{
  int i;
  uint32_t ws;
  byte *hp, *mp;
  grn_io_array_spec *array_specs = (grn_io_array_spec *)io->user_header;
  hp = io->user_header;
  if (!(mp = GRN_CALLOC(msize))) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  io->ainfo = (grn_io_array_info *)mp;
  hp += sizeof(grn_io_array_spec) * n_arrays;
  mp += sizeof(grn_io_array_info) * n_arrays;
  for (ws = 0; (1 << ws) < io->header->segment_size; ws++);
  for (i = 0; i < n_arrays; i++) {
    uint32_t we = ws - array_specs[i].w_of_element;
    io->ainfo[i].w_of_elm_in_a_segment = we;
    io->ainfo[i].elm_mask_in_a_segment = (1 << we) - 1;
    io->ainfo[i].max_n_segments = array_specs[i].max_n_segments;
    io->ainfo[i].element_size = 1 << array_specs[i].w_of_element;
    io->ainfo[i].segments = (uint32_t *)hp;
    io->ainfo[i].addrs = (void **)mp;
    hp += sizeof(uint32_t) * array_specs[i].max_n_segments;
    mp += sizeof(void *) * array_specs[i].max_n_segments;
  }
  io->user_header += hsize;
  return GRN_SUCCESS;
}

static grn_rc
array_init(grn_ctx *ctx, grn_io *io, int n_arrays)
{
  if (n_arrays) {
    int i;
    grn_io_array_spec *array_specs = (grn_io_array_spec *)io->user_header;
    size_t hsize = sizeof(grn_io_array_spec) * n_arrays;
    size_t msize = sizeof(grn_io_array_info) * n_arrays;
    for (i = 0; i < n_arrays; i++) {
      hsize += sizeof(uint32_t) * array_specs[i].max_n_segments;
      msize += sizeof(void *) * array_specs[i].max_n_segments;
    }
    return array_init_(ctx, io, n_arrays, hsize, msize);
  }
  return GRN_SUCCESS;
}

grn_io *
grn_io_create_with_array(grn_ctx *ctx, const char *path,
                         uint32_t header_size, uint32_t segment_size,
                         grn_io_mode mode, int n_arrays,
                         grn_io_array_spec *array_specs)
{
  if (n_arrays) {
    int i;
    grn_io *io;
    byte *hp;
    uint32_t nsegs = 0;
    size_t hsize = sizeof(grn_io_array_spec) * n_arrays;
    size_t msize = sizeof(grn_io_array_info) * n_arrays;
    for (i = 0; i < n_arrays; i++) {
      nsegs += array_specs[i].max_n_segments;
      hsize += sizeof(uint32_t) * array_specs[i].max_n_segments;
      msize += sizeof(void *) * array_specs[i].max_n_segments;
    }
    if ((io = grn_io_create(ctx, path, header_size + hsize,
                            segment_size, nsegs, mode, GRN_IO_EXPIRE_GTICK))) {
      grn_rc rc;
      hp = io->user_header;
      grn_memcpy(hp, array_specs, sizeof(grn_io_array_spec) * n_arrays);
      io->header->n_arrays = n_arrays;
      io->header->segment_tail = 1;
      rc = array_init_(ctx, io, n_arrays, hsize, msize);
      if (rc == GRN_SUCCESS) {
        return io;
      }
      ERR(GRN_NO_MEMORY_AVAILABLE, "grn_io_create_with_array failed");
      grn_io_close(ctx, io);
    }
  }
  return NULL;
}

grn_inline static uint32_t
segment_alloc(grn_ctx *ctx, grn_io *io)
{
  uint32_t n, s;
  grn_io_array_info *ai;
  if (io->header->segment_tail) {
    if (io->header->segment_tail > io->header->max_segment) {
      s = 0;
    } else {
      s = io->header->segment_tail++;
    }
  } else {
    char *used = GRN_CALLOC(io->header->max_segment + 1);
    if (!used) { return 0; }
    for (n = io->header->n_arrays, ai = io->ainfo; n; n--, ai++) {
      for (s = 0; s < ai->max_n_segments; s++) {
        used[ai->segments[s]] = 1;
      }
    }
    for (s = 1; ; s++) {
      if (s > io->header->max_segment) {
        io->header->segment_tail = s;
        s = 0;
        break;
      }
      if (!used[s]) {
        io->header->segment_tail = s + 1;
        break;
      }
    }
    GRN_FREE(used);
  }
  return s;
}

void
grn_io_segment_alloc(grn_ctx *ctx, grn_io *io, grn_io_array_info *ai,
                     uint32_t lseg, int *flags, void **p)
{
  uint32_t *sp = &ai->segments[lseg];
  if (!*sp) {
    if ((*flags & GRN_TABLE_ADD)) {
      if ((*sp = segment_alloc(ctx, io))) {
        *flags |= GRN_TABLE_ADDED;
      }
    }
  }
  if (*sp) {
    uint32_t pseg = *sp - 1;
    *p = grn_io_seg_ref(ctx, io, pseg);
    if (*p) { grn_io_seg_unref(ctx, io, pseg); };
  }
}

uint32_t
grn_io_detect_type(grn_ctx *ctx, const char *path)
{
  struct _grn_io_header h;
  uint32_t res = 0;
  int fd;
  grn_open(fd, path, O_RDONLY | GRN_OPEN_FLAG_BINARY);
  if (fd != -1) {
    struct stat s;
    if (fstat(fd, &s) != -1 && s.st_size >= sizeof(struct _grn_io_header)) {
      if (grn_read(fd, &h, sizeof(struct _grn_io_header)) ==
          sizeof(struct _grn_io_header)) {
        if (!memcmp(h.idstr, GRN_IO_IDSTR, GRN_IO_IDSTR_LEN)) {
          res = h.type;
        } else {
          ERR(GRN_INCOMPATIBLE_FILE_FORMAT,
              "failed to detect type: format ID is different: <%s>: <%.*s>",
              path,
              (int)GRN_IO_IDSTR_LEN, GRN_IO_IDSTR);
        }
      } else {
        SERR("failed to read enough data for detecting type: <%s>",
             path);
      }
    } else {
      ERR(GRN_INVALID_FORMAT, "grn_io_detect_type failed");
    }
    grn_close(fd);
  } else {
    ERRNO_ERR("failed to open path for detecting type: <%s>",
              path);
  }
  return res;
}

grn_io *
grn_io_open(grn_ctx *ctx, const char *path, grn_io_mode mode)
{
  size_t max_path_len = PATH_MAX - 4;
  size_t path_length;
  struct stat s;
  fileinfo fi;
  uint32_t flags = 0;
  uint32_t b;
  uint32_t header_size = 0, segment_size = 0, max_segment = 0, bs;
  if (!path || !*path) {
    ERR(GRN_INVALID_ARGUMENT, "[io][open] path is missing");
    return NULL;
  }
  path_length = strlen(path);
  if (path_length > max_path_len) {
    int truncate_length = 10;
    ERR(GRN_INVALID_ARGUMENT,
        "[io][open] path is too long: "
        "<%" GRN_FMT_SIZE ">(max: %" GRN_FMT_SIZE "): <%.*s...>",
        path_length,
        max_path_len,
        truncate_length,
        path);
    return NULL;
  }
  {
    struct _grn_io_header h;
    int fd;
    ssize_t read_bytes;
    grn_open(fd, path, O_RDWR | GRN_OPEN_FLAG_BINARY);
    if (fd == -1) {
      ERRNO_ERR("[io][open] failed to open path: <%s>",
                path);
      return NULL;
    }
    if (fstat(fd, &s) == -1) {
      ERRNO_ERR("[io][open] failed to file status: <%s>",
                path);
      grn_close(fd);
      return NULL;
    }
    if (s.st_size < sizeof(struct _grn_io_header)) {
      ERR(GRN_INCOMPATIBLE_FILE_FORMAT,
          "[io][open] file size is too small: "
          "<%" GRN_FMT_INT64D ">(required: >= %" GRN_FMT_SIZE "): <%s>",
          (int64_t)(s.st_size),
          sizeof(struct _grn_io_header),
          path);
      grn_close(fd);
      return NULL;
    }
    read_bytes = grn_read(fd, &h, sizeof(struct _grn_io_header));
    if (read_bytes != sizeof(struct _grn_io_header)) {
      ERRNO_ERR("[io][open] failed to read header data: "
                "<%" GRN_FMT_SSIZE ">(expected: %" GRN_FMT_SSIZE "): <%s>",
                read_bytes,
                sizeof(struct _grn_io_header),
                path);
      grn_close(fd);
      return NULL;
    }
    if (memcmp(h.idstr, GRN_IO_IDSTR, GRN_IO_IDSTR_LEN) != 0) {
      ERR(GRN_INCOMPATIBLE_FILE_FORMAT,
          "[io][open] failed to open: format ID is different: <%s>: <%.*s>",
          path,
          (int)GRN_IO_IDSTR_LEN, GRN_IO_IDSTR);
      grn_close(fd);
      return NULL;
    }
    header_size = h.header_size;
    segment_size = h.segment_size;
    max_segment = h.max_segment;
    flags = h.flags;
    grn_close(fd);
    if (segment_size == 0) {
      ERR(GRN_INCOMPATIBLE_FILE_FORMAT,
          "[io][open] failed to open: segment size is 0: <%s>",
          path);
      return NULL;
    }
  }
  b = grn_io_compute_base(header_size);
  bs = grn_io_compute_base_segment(b, segment_size);
  grn_fileinfo_init(&fi, 1);
  grn_rc rc = grn_fileinfo_open(ctx, &fi, path, O_RDWR);
  if (rc != GRN_SUCCESS) {
    ERR(rc,
        "[io][open] failed to open fileinfo: <%s>",
        path);
    return NULL;
  }

  struct _grn_io_header *header;
  header = GRN_MMAP(ctx, &grn_gctx, NULL, &(fi.fmo), &fi, 0, b);
  if (!header) {
    grn_fileinfo_close(ctx, &fi);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[io][open] failed to map: <%s>",
        path);
    return NULL;
  }

  unsigned long file_size = grn_io_compute_file_size(header->version);
  unsigned int max_nfiles = grn_io_compute_max_n_files(segment_size,
                                                       max_segment,
                                                       bs,
                                                       file_size);
  fileinfo *fis = GRN_MALLOCN(fileinfo, max_nfiles);
  if (!fis) {
    GRN_MUNMAP(ctx, &grn_gctx, NULL, &(fi.fmo), &fi, header, b);
    grn_fileinfo_close(ctx, &fi);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[io][open] failed to allocate fileinfo: <%s>",
        path);
    return NULL;
  }

  grn_fileinfo_init(fis, max_nfiles);
  grn_memcpy(fis, &fi, sizeof(fileinfo));
  grn_io *io = GRN_CALLOC(sizeof(grn_io));
  if (!io) {
    GRN_FREE(fis);
    GRN_MUNMAP(ctx, &grn_gctx, NULL, &(fi.fmo), &fi, header, b);
    grn_fileinfo_close(ctx, &fi);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[io][open] failed to allocate grn_io: <%s>",
        path);
    return NULL;
  }

  grn_io_mapinfo *maps = GRN_CALLOC(sizeof(grn_io_mapinfo) * max_segment);
  if (!maps) {
    GRN_FREE(io);
    GRN_FREE(fis);
    GRN_MUNMAP(ctx, &grn_gctx, NULL, &(fi.fmo), &fi, header, b);
    grn_fileinfo_close(ctx, &fi);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[io][open] failed to allocate grn_io_mapinfo: <%s>",
        path);
    return NULL;
  }

  grn_strncpy(io->path, PATH_MAX, path, path_length + 1);
  io->header = header;
  io->user_header = (((byte *) header) + IO_HEADER_SIZE);
  io->maps = maps;
  io->base = b;
  io->base_seg = bs;
  io->mode = mode;
  io->fis = fis;
  io->ainfo = NULL;
  io->max_map_seg = 0;
  io->nmaps = 0;
  io->count = 0;
  io->flags = header->flags;
  io->lock = &header->lock;
  rc = array_init(ctx, io, io->header->n_arrays);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(io->maps);
    GRN_FREE(io);
    GRN_FREE(fis);
    GRN_MUNMAP(ctx, &grn_gctx, NULL, &(fi.fmo), &fi, header, b);
    grn_fileinfo_close(ctx, &fi);
    ERR(rc,
        "[io][open] failed to initialize array: <%s>",
        path);
    return NULL;
  }

  grn_io_register(ctx, io);
  return io;
}

grn_rc
grn_io_close(grn_ctx *ctx, grn_io *io)
{
  uint32_t max_nfiles;

  max_nfiles = grn_io_max_n_files(io);
  grn_io_unregister(ctx, io);
  if (io->ainfo) { GRN_FREE(io->ainfo); }
  if (io->maps) {
    int i;
    uint32_t max_segment;
    uint32_t segment_size;
    unsigned long file_size;
    uint32_t segments_per_file;

    max_segment = grn_io_max_segment(io);
    segment_size = io->header->segment_size;
    file_size = grn_io_compute_file_size(io->header->version);
    segments_per_file = file_size / segment_size;
    for (i = 0; i < max_segment; i++) {
      grn_io_mapinfo *mi;
      mi = &(io->maps[i]);
      if (mi->map) {
        fileinfo *fi = NULL;
        /* if (atomic_read(mi->nref)) { return STILL_IN_USE ; } */
        if (io->fis) {
          uint32_t bseg = i + io->base_seg;
          uint32_t fno = bseg / segments_per_file;
          fi = &io->fis[fno];
        }
        GRN_MUNMAP(ctx, &grn_gctx, io, &mi->fmo, fi, mi->map, segment_size);
      }
    }
    GRN_FREE(io->maps);
  }
  GRN_MUNMAP(ctx, &grn_gctx, io, (io->fis ? &io->fis->fmo : NULL),
             io->fis, io->header, io->base);
  if (io->fis) {
    int i;
    for (i = 0; i < max_nfiles; i++) {
      fileinfo *fi = &(io->fis[i]);
      grn_fileinfo_close(ctx, fi);
    }
    GRN_FREE(io->fis);
  }
  GRN_FREE(io);
  return GRN_SUCCESS;
}

uint32_t
grn_io_base_seg(grn_io *io)
{
  return io->base_seg;
}

const char *
grn_io_path(grn_io *io)
{
  return io->path;
}

void *
grn_io_header(grn_io *io)
{
  return io->user_header;
}

grn_rc
grn_io_set_type(grn_io *io, uint32_t type)
{
  if (!io || !io->header) {
    return GRN_INVALID_ARGUMENT;
  }
  io->header->type = type;
  return GRN_SUCCESS;
}

uint32_t
grn_io_get_type(grn_io *io)
{
  if (!io || !io->header) { return GRN_VOID; }
  return io->header->type;
}

grn_inline static void
gen_pathname(const char *path, char *buffer, int fno)
{
  size_t len = strlen(path);
  grn_memcpy(buffer, path, len);
  if (fno) {
    buffer[len] = '.';
    grn_itoh(fno, buffer + len + 1, 3);
    buffer[len + 4] = '\0';
  } else {
    buffer[len] = '\0';
  }
}

static uint32_t
grn_io_n_files(grn_ctx *ctx, grn_io *io)
{
  unsigned long file_size;
  file_size = grn_io_compute_file_size(io->header->version);
  return ((io->header->curr_size + file_size - 1) / file_size);
}

grn_rc
grn_io_size(grn_ctx *ctx, grn_io *io, uint64_t *size)
{
  int fno;
  struct stat s;
  uint64_t tsize = 0;
  char buffer[PATH_MAX];
  uint32_t n_files;

  n_files = grn_io_n_files(ctx, io);
  for (fno = 0; fno < n_files; fno++) {
    gen_pathname(io->path, buffer, fno);
    if (stat(buffer, &s)) {
      SERR("failed to stat path to compute size: <%s>",
           buffer);
    } else {
      tsize += s.st_size;
    }
  }
  *size = tsize;
  return GRN_SUCCESS;
}

grn_rc
grn_io_remove_raw(grn_ctx *ctx, const char *path)
{
  grn_rc rc = GRN_SUCCESS;
  int fno;
  char buffer[PATH_MAX];

  if (grn_unlink(path) != 0) {
    ERRNO_ERR("[io][remove] failed to remove path: <%s>",
              path);
    return ctx->rc;
  }
  GRN_LOG(ctx, GRN_LOG_INFO, "[io][remove] removed path: <%s>", path);

  for (fno = 1; ; fno++) {
    struct stat s;
    gen_pathname(path, buffer, fno);
    if (stat(buffer, &s) != 0) {
      break;
    }
    if (grn_unlink(buffer) == 0) {
      GRN_LOG(ctx, GRN_LOG_INFO,
              "[io][remove] removed numbered path: <%d>: <%s>", fno, buffer);
    } else {
      ERRNO_ERR("[io][remove] failed to remove numbered path: <%d>: <%s>",
                fno, buffer);
      rc = ctx->rc;
    }
  }
  return rc;
}

grn_rc
grn_io_remove(grn_ctx *ctx, const char *path)
{
  struct stat s;

  if (stat(path, &s) != 0) {
    SERR("failed to stat: <%s>", path);
    return ctx->rc;
  }

  return grn_io_remove_raw(ctx, path);
}

grn_rc
grn_io_remove_if_exist(grn_ctx *ctx, const char *path)
{
  struct stat s;
  if (stat(path, &s) == 0) {
    return grn_io_remove_raw(ctx, path);
  }
  return GRN_SUCCESS;
}

grn_rc
grn_io_rename(grn_ctx *ctx, const char *old_name, const char *new_name)
{
  const char *tag = "[io][rename]";
  struct stat s;
  if (stat(old_name, &s)) {
    SERR("%s failed to stat path to be renamed: <%s>",
         tag, old_name);
    return ctx->rc;
  } else if (rename(old_name, new_name)) {
    SERR("%s failed to rename path: <%s> -> <%s>",
         tag, old_name, new_name);
    return ctx->rc;
  } else {
    int fno;
    char old_buffer[PATH_MAX];
    char new_buffer[PATH_MAX];
    for (fno = 1; ; fno++) {
      gen_pathname(old_name, old_buffer, fno);
      if (stat(old_buffer, &s) == 0) {
        break;
      }
      gen_pathname(new_name, new_buffer, fno);
      if (rename(old_buffer, new_buffer)) {
        SERR("%s failed to rename path: <%s> -> <%s>",
             tag, old_buffer, new_buffer);
      }
    }
    return GRN_SUCCESS;
  }
}

typedef struct {
  grn_io_ja_ehead head;
  char body[256];
} ja_element;

grn_rc
grn_io_read_ja(grn_io *io, grn_ctx *ctx, grn_io_ja_einfo *einfo, uint32_t epos,
               uint32_t key, uint32_t segment, uint32_t offset, void **value,
               uint32_t *value_len)
{
  uint32_t rest = 0, size = *value_len + sizeof(grn_io_ja_ehead);
  uint32_t segment_size = io->header->segment_size;
  unsigned long file_size = grn_io_compute_file_size(io->header->version);
  uint32_t segments_per_file = file_size / segment_size;
  uint32_t bseg = segment + io->base_seg;
  int fno = bseg / segments_per_file;
  fileinfo *fi = &io->fis[fno];
  off_t base = fno ? 0 : io->base - (uint64_t)segment_size * io->base_seg;
  off_t pos = (uint64_t)segment_size * (bseg % segments_per_file) + offset + base;
  ja_element *v = GRN_CALLOC(size);
  if (!v) {
    *value = NULL;
    *value_len = 0;
    return GRN_NO_MEMORY_AVAILABLE;
  }
  if (pos + size > file_size) {
    rest = pos + size - file_size;
    size = file_size - pos;
  }
  if (!grn_fileinfo_opened(fi)) {
    char path[PATH_MAX];
    gen_pathname(io->path, path, fno);
    if (grn_fileinfo_open(ctx, fi, path, O_RDWR|O_CREAT)) {
      *value = NULL;
      *value_len = 0;
      GRN_FREE(v);
      return ctx->rc;
    }
  }
  if (grn_pread(ctx, fi, v, size, pos)) {
    *value = NULL;
    *value_len = 0;
    GRN_FREE(v);
    return ctx->rc;
  }
  if (einfo->pos != epos) {
    GRN_LOG(ctx, GRN_LOG_WARNING,
            "einfo pos changed %x => %x", einfo->pos, epos);
    *value = NULL;
    *value_len = 0;
    GRN_FREE(v);
    return GRN_FILE_CORRUPT;
  }
  if (einfo->size != *value_len) {
    GRN_LOG(ctx, GRN_LOG_WARNING,
            "einfo size changed %d => %d", einfo->size, *value_len);
    *value = NULL;
    *value_len = 0;
    GRN_FREE(v);
    return GRN_FILE_CORRUPT;
  }
  if (v->head.key != key) {
    GRN_LOG(ctx, GRN_LOG_ERROR,
            "ehead key unmatch %x => %x", key, v->head.key);
    *value = NULL;
    *value_len = 0;
    GRN_FREE(v);
    return GRN_INVALID_FORMAT;
  }
  if (v->head.size != *value_len) {
    GRN_LOG(ctx, GRN_LOG_ERROR,
            "ehead size unmatch %d => %d", *value_len, v->head.size);
    *value = NULL;
    *value_len = 0;
    GRN_FREE(v);
    return GRN_INVALID_FORMAT;
  }
  if (rest) {
    byte *vr = (byte *)v + size;
    do {
      fi = &io->fis[++fno];
      if (!grn_fileinfo_opened(fi)) {
        char path[PATH_MAX];
        gen_pathname(io->path, path, fno);
        if (grn_fileinfo_open(ctx, fi, path, O_RDWR|O_CREAT)) {
          *value = NULL;
          *value_len = 0;
          GRN_FREE(v);
          return ctx->rc;
        }
      }
      size = rest > file_size ? file_size : rest;
      if (grn_pread(ctx, fi, vr, size, 0)) {
        *value = NULL;
        *value_len = 0;
        GRN_FREE(v);
        return ctx->rc;
      }
      vr += size;
      rest -= size;
    } while (rest);
  }
  *value = v->body;
  return GRN_SUCCESS;
}

grn_rc
grn_io_write_ja(grn_io *io, grn_ctx *ctx, uint32_t key,
                uint32_t segment, uint32_t offset, void *value,
                uint32_t value_len)
{
  grn_rc rc;
  uint32_t rest = 0, size = value_len + sizeof(grn_io_ja_ehead);
  uint32_t segment_size = io->header->segment_size;
  unsigned long file_size = grn_io_compute_file_size(io->header->version);
  uint32_t segments_per_file = file_size / segment_size;
  uint32_t bseg = segment + io->base_seg;
  int fno = bseg / segments_per_file;
  fileinfo *fi = &io->fis[fno];
  off_t base = fno ? 0 : io->base - (uint64_t)segment_size * io->base_seg;
  off_t pos = (uint64_t)segment_size * (bseg % segments_per_file) + offset + base;
  if (pos + size > file_size) {
    rest = pos + size - file_size;
    size = file_size - pos;
  }
  if (!grn_fileinfo_opened(fi)) {
    char path[PATH_MAX];
    gen_pathname(io->path, path, fno);
    if ((rc = grn_fileinfo_open(ctx, fi, path, O_RDWR|O_CREAT))) { return rc; }
  }
  if (value_len <= 256) {
    ja_element je;
    je.head.size = value_len;
    je.head.key = key;
    grn_memcpy(je.body, value, value_len);
    rc = grn_pwrite(ctx, fi, &je, size, pos);
  } else {
    grn_io_ja_ehead eh;
    eh.size = value_len;
    eh.key =  key;
    if ((rc = grn_pwrite(ctx, fi, &eh, sizeof(grn_io_ja_ehead), pos))) {
      return rc;
    }
    pos += sizeof(grn_io_ja_ehead);
    rc = grn_pwrite(ctx, fi, value, size - sizeof(grn_io_ja_ehead), pos);
  }
  if (rc) { return rc; }
  if (rest) {
    byte *vr = (byte *)value + size - sizeof(grn_io_ja_ehead);
    do {
      fi = &io->fis[++fno];
      if (!grn_fileinfo_opened(fi)) {
        char path[PATH_MAX];
        gen_pathname(io->path, path, fno);
        if ((rc = grn_fileinfo_open(ctx, fi, path, O_RDWR|O_CREAT))) {
          return rc;
        }
      }
      size = rest > file_size ? file_size : rest;
      if ((rc = grn_pwrite(ctx, fi, vr, size, 0))) { return rc; }
      vr += size;
      rest -= size;
    } while (rest);
  }
  return rc;
}

grn_rc
grn_io_write_ja_ehead(grn_io *io, grn_ctx *ctx, uint32_t key,
                      uint32_t segment, uint32_t offset, uint32_t value_len)
{
  grn_rc rc;
  uint32_t segment_size = io->header->segment_size;
  unsigned long file_size = grn_io_compute_file_size(io->header->version);
  uint32_t segments_per_file = file_size / segment_size;
  uint32_t bseg = segment + io->base_seg;
  int fno = bseg / segments_per_file;
  fileinfo *fi = &io->fis[fno];
  off_t base = fno ? 0 : io->base - (uint64_t)segment_size + io->base_seg;
  off_t pos = (uint64_t)segment_size * (bseg % segments_per_file) + offset + base;
  if (!grn_fileinfo_opened(fi)) {
    char path[PATH_MAX];
    gen_pathname(io->path, path, fno);
    if ((rc = grn_fileinfo_open(ctx, fi, path, O_RDWR|O_CREAT))) { return rc; }
  }
  {
    grn_io_ja_ehead eh;
    eh.size = value_len;
    eh.key =  key;
    return grn_pwrite(ctx, fi, &eh, sizeof(grn_io_ja_ehead), pos);
  }
}

void *
grn_io_win_map(grn_ctx *ctx,
               grn_io *io,
               grn_io_win *iw,
               uint32_t segment,
               uint32_t offset,
               uint32_t size,
               grn_io_rw_mode mode)
{
  uint32_t nseg, segment_size = io->header->segment_size;
  if (offset >= segment_size) {
    segment += offset / segment_size;
    offset = offset % segment_size;
  }
  nseg = (offset + size + segment_size - 1) / segment_size;
  if (!size || !ctx || segment + nseg > io->header->max_segment) {
    return NULL;
  }
  iw->ctx = ctx;
  iw->diff = 0;
  iw->io = io;
  iw->mode = mode;
  iw->tiny_p = 0;
  iw->segment = segment;
  iw->offset = offset;
  iw->nseg = nseg;
  iw->size = size;
  if (nseg == 1) {
    byte *addr = grn_io_seg_ref(ctx, io, segment);
    if (!addr) { return NULL; }
    iw->cached = 1;
    iw->addr = addr + offset;
  } else {
    if (!(iw->addr = GRN_CALLOC(size))) { return NULL; }
    iw->cached = 0;
    switch (mode) {
    case GRN_IO_RDONLY:
    case GRN_IO_RDWR:
      {
        byte *p, *q = NULL;
        uint32_t s, r;
        for (p = iw->addr, r = size; r; p += s, r -= s, segment++, offset = 0) {
          q = grn_io_seg_ref(ctx, io, segment);
          if (!q) {
            GRN_FREE(iw->addr);
            return NULL;
          }
          s = (offset + r > segment_size) ? segment_size - offset : r;
          grn_memcpy(p, q + offset, s);
          grn_io_seg_unref(ctx, io, segment);
        }
      }
      break;
    case GRN_IO_WRONLY:
      break;
    default :
      return NULL;
    }
  }
  return iw->addr;
}

grn_rc
grn_io_win_unmap(grn_ctx *ctx, grn_io_win *iw)
{
  if (!iw || !iw->io ||!iw->ctx) { return GRN_INVALID_ARGUMENT; }
  if (iw->cached) {
    if (!iw->tiny_p) { grn_io_seg_unref(ctx, iw->io, iw->segment); }
    return GRN_SUCCESS;
  }
  {
    grn_io *io = iw->io;
    grn_ctx *ctx = iw->ctx;
    switch (iw->mode) {
    case GRN_IO_RDONLY:
      if (!iw->addr) { return GRN_INVALID_ARGUMENT; }
      GRN_FREE(iw->addr);
      return GRN_SUCCESS;
    case GRN_IO_RDWR:
    case GRN_IO_WRONLY:
      {
        byte *p, *q = NULL;
        uint32_t segment_size = io->header->segment_size;
        uint32_t s, r, offset = iw->offset, segment = iw->segment;
        for (p = iw->addr, r = iw->size; r;
             p += s, r -= s, segment++, offset = 0) {
          q = grn_io_seg_ref(ctx, io, segment);
          if (!q) { return GRN_NO_MEMORY_AVAILABLE; }
          s = (offset + r > segment_size) ? segment_size - offset : r;
          grn_memcpy(q + offset, p, s);
          grn_io_seg_unref(ctx, io, segment);
        }
      }
      GRN_FREE(iw->addr);
      return GRN_SUCCESS;
    default :
      return GRN_INVALID_ARGUMENT;
    }
  }
}

#define DO_MAP(io,fmo,fi,pos,size,segno,res) do {\
  (res) = GRN_MMAP(ctx, &grn_gctx, (io), (fmo), (fi), (pos), (size));\
  if ((res)) {\
    uint32_t nmaps;\
    if (io->max_map_seg < segno) { io->max_map_seg = segno; }\
    GRN_ATOMIC_ADD_EX(&io->nmaps, 1, nmaps);\
    {\
      uint64_t tail = io->base + (uint64_t)(size) * ((segno) + 1);\
      if (tail > io->header->curr_size) { io->header->curr_size = tail; }\
    }\
  }\
} while (0)

void
grn_io_seg_map_(grn_ctx *ctx, grn_io *io, uint32_t segno, grn_io_mapinfo *info)
{
  uint32_t segment_size = io->header->segment_size;
  if ((io->flags & GRN_IO_TEMPORARY)) {
    DO_MAP(io, &info->fmo, NULL, 0, segment_size, segno, info->map);
  } else {
    unsigned long file_size = grn_io_compute_file_size(io->header->version);
    uint32_t segments_per_file = file_size / segment_size;
    uint32_t bseg = segno + io->base_seg;
    uint32_t fno = bseg / segments_per_file;
    off_t base = fno ? 0 : io->base - (uint64_t)segment_size * io->base_seg;
    off_t pos = (uint64_t)segment_size * (bseg % segments_per_file) + base;
    fileinfo *fi = &io->fis[fno];
    if (!grn_fileinfo_opened(fi)) {
      char path[PATH_MAX];
      grn_bool path_exist = GRN_TRUE;
      gen_pathname(io->path, path, fno);
      path_exist = grn_path_exist(path);
      if (!grn_fileinfo_open(ctx, fi, path, O_RDWR|O_CREAT)) {
        DO_MAP(io, &info->fmo, fi, pos, segment_size, segno, info->map);
        if (!info->map && !path_exist) {
          grn_fileinfo_close(ctx, fi);
          if (grn_unlink(path) == 0) {
            GRN_LOG(ctx, GRN_LOG_INFO,
                    "[io][map][error] memory mapping is failed and then "
                    "removed created map file: <%s>", path);
          } else {
            ERRNO_ERR("[io][map][error] memory mapping is failed and then "
                      "failed to remove created map file: <%s>", path);
          }
        }
      }
    } else {
      DO_MAP(io, &info->fmo, fi, pos, segment_size, segno, info->map);
    }
  }
}

grn_rc
grn_io_seg_expire(grn_ctx *ctx, grn_io *io, uint32_t segno, uint32_t nretry)
{
  uint32_t retry, *pnref;
  grn_io_mapinfo *info;
  if (!io->maps || segno >= io->header->max_segment) { return GRN_INVALID_ARGUMENT; }
  info = &io->maps[segno];
  if (!info->map) { return GRN_INVALID_ARGUMENT; }
  pnref = &info->nref;
  for (retry = 0;; retry++) {
    uint32_t nref;
    GRN_ATOMIC_ADD_EX(pnref, 1, nref);
    if (nref) {
      GRN_ATOMIC_ADD_EX(pnref, -1, nref);
      if (retry >= GRN_IO_MAX_RETRY) {
        GRN_LOG(ctx, GRN_LOG_CRIT,
                "deadlock detected! in grn_io_seg_expire(%p, %u, %u)",
                io, segno, nref);
        return GRN_RESOURCE_DEADLOCK_AVOIDED;
      }
    } else {
      GRN_ATOMIC_ADD_EX(pnref, GRN_IO_MAX_REF, nref);
      if (nref > 1) {
        GRN_ATOMIC_ADD_EX(pnref, -(GRN_IO_MAX_REF + 1), nref);
        GRN_FUTEX_WAKE(pnref);
        if (retry >= GRN_IO_MAX_RETRY) {
          GRN_LOG(ctx, GRN_LOG_CRIT,
                  "deadlock detected!! in grn_io_seg_expire(%p, %u, %u)",
                  io, segno, nref);
          return GRN_RESOURCE_DEADLOCK_AVOIDED;
        }
      } else {
        uint32_t nmaps;
        fileinfo *fi = &(io->fis[segno]);
        GRN_MUNMAP(ctx, &grn_gctx, io, &info->fmo, fi,
                   info->map, io->header->segment_size);
        info->map = NULL;
        GRN_ATOMIC_ADD_EX(pnref, -(GRN_IO_MAX_REF + 1), nref);
        GRN_ATOMIC_ADD_EX(&io->nmaps, -1, nmaps);
        GRN_FUTEX_WAKE(pnref);
        return GRN_SUCCESS;
      }
    }
    if (retry >= nretry) { return GRN_RESOURCE_DEADLOCK_AVOIDED; }
    GRN_FUTEX_WAIT(pnref);
  }
}

uint32_t
grn_io_expire(grn_ctx *ctx, grn_io *io, int count_thresh, uint32_t limit)
{
  uint32_t m, n = 0, ln = io->nmaps;
  switch ((io->flags & (GRN_IO_EXPIRE_GTICK|GRN_IO_EXPIRE_SEGMENT))) {
  case GRN_IO_EXPIRE_GTICK :
    {
      uint32_t nref, nmaps, *pnref = &io->nref;
      GRN_ATOMIC_ADD_EX(pnref, 1, nref);
      if (!nref && grn_gtick - io->count > count_thresh) {
        {
          uint32_t i = io->header->n_arrays;
          grn_io_array_spec *array_specs = (grn_io_array_spec *)io->user_header;
          while (i--) {
            memset(io->ainfo[i].addrs, 0,
                   sizeof(void *) * array_specs[i].max_n_segments);
          }
        }
        {
          uint32_t fno;
          for (fno = 0; fno < io->max_map_seg; fno++) {
            grn_io_mapinfo *info = &(io->maps[fno]);
            if (info->map) {
              fileinfo *fi = &(io->fis[fno]);
              GRN_MUNMAP(ctx, &grn_gctx, io, &info->fmo, fi,
                         info->map, io->header->segment_size);
              info->map = NULL;
              info->nref = 0;
              info->count = grn_gtick;
              GRN_ATOMIC_ADD_EX(&io->nmaps, -1, nmaps);
              n++;
            }
          }
        }
      }
      GRN_ATOMIC_ADD_EX(pnref, -1, nref);
    }
    break;
  case GRN_IO_EXPIRE_SEGMENT :
    for (m = io->max_map_seg; n < limit && m; m--) {
      if (!grn_io_seg_expire(ctx, io, m, 0)) { n++; }
    }
    break;
  case (GRN_IO_EXPIRE_GTICK|GRN_IO_EXPIRE_SEGMENT) :
    {
      grn_io_mapinfo *info = io->maps;
      for (m = io->max_map_seg; n < limit && m; info++, m--) {
        if (info->map && (grn_gtick - info->count) > count_thresh) {
          uint32_t nmaps, nref, *pnref = &info->nref;
          GRN_ATOMIC_ADD_EX(pnref, 1, nref);
          if (!nref && info->map && (grn_gtick - info->count) > count_thresh) {
            GRN_MUNMAP(ctx, &grn_gctx, io, &info->fmo, NULL,
                       info->map, io->header->segment_size);
            GRN_ATOMIC_ADD_EX(&io->nmaps, -1, nmaps);
            info->map = NULL;
            info->count = grn_gtick;
            n++;
          }
          GRN_ATOMIC_ADD_EX(pnref, -1, nref);
        }
      }
    }
    break;
  }
  if (n) {
    GRN_LOG(ctx, GRN_LOG_DEBUG, "<%p:%x> expired i=%p max=%d (%d/%d)",
            ctx, grn_gtick, io, io->max_map_seg, n, ln);
  }
  return n;
}

void *
grn_io_anon_map(grn_ctx *ctx, grn_io_mapinfo *mi, size_t length)
{
  return (mi->map = GRN_MMAP(ctx, ctx, NULL, &mi->fmo, NULL, 0, length));
}

void
grn_io_anon_unmap(grn_ctx *ctx, grn_io_mapinfo *mi, size_t length)
{
  GRN_MUNMAP(ctx, ctx, NULL, &mi->fmo, NULL, mi->map, length);
}

grn_rc
grn_io_lock(grn_ctx *ctx, grn_io *io, int timeout)
{
  static int _ncalls = 0, _ncolls = 0;
  uint32_t count, count_log_border = 1000;
  uint32_t rc_check_interval = 1000;
  _ncalls++;
  if (!io) { return GRN_INVALID_ARGUMENT; }
  for (count = 0;; count++) {
    uint32_t lock;
    GRN_ATOMIC_ADD_EX(io->lock, 1, lock);
    if (lock) {
      GRN_ATOMIC_ADD_EX(io->lock, -1, lock);
      if (count == count_log_border) {
        GRN_LOG(ctx, GRN_LOG_NOTICE,
                "io(%s) collisions(%d/%d): lock failed %d times",
                io->path, _ncolls, _ncalls, count_log_border);
      }
      if (!timeout || (timeout > 0 && timeout == count)) {
        GRN_LOG(ctx, GRN_LOG_WARNING,
                "[DB Locked] time out(%d): io(%s) collisions(%d/%d)",
                timeout, io->path, _ncolls, _ncalls);
        break;
      }
      if (!(++_ncolls % 1000000) && (_ncolls > _ncalls)) {
        if (_ncolls < 0 || _ncalls < 0) {
          _ncolls = 0; _ncalls = 0;
        } else {
          GRN_LOG(ctx, GRN_LOG_NOTICE,
                  "io(%s) collisions(%d/%d)", io->path, _ncolls, _ncalls);
        }
      }
      if ((count % rc_check_interval) == 0) {
        if (ctx->rc != GRN_SUCCESS) {
          return ctx->rc;
        }
      }
      grn_nanosleep(GRN_LOCK_WAIT_TIME_NANOSECOND);
      continue;
    }
    return GRN_SUCCESS;
  }
  ERR(GRN_RESOURCE_DEADLOCK_AVOIDED, "grn_io_lock failed");
  return ctx->rc;
}

void
grn_io_unlock(grn_io *io)
{
  if (io) {
    uint32_t lock;
    GRN_ATOMIC_ADD_EX(io->lock, -1, lock);
  }
}

void
grn_io_clear_lock(grn_io *io)
{
  if (io) { *io->lock = 0; }
}

uint32_t
grn_io_is_locked(grn_io *io)
{
  return io ? *io->lock : 0;
}

grn_rc
grn_io_flush(grn_ctx *ctx, grn_io *io)
{
  grn_rc rc = GRN_SUCCESS;
  struct _grn_io_header *header;
  uint32_t aligned_header_size;

  if (io->path[0] == '\0') {
    return GRN_SUCCESS;
  }

  header = io->header;
  aligned_header_size = grn_io_compute_base(header->header_size);

  if (GRN_MSYNC(ctx, &(io->fis[0]), header, aligned_header_size) != 0) {
    return ctx->rc;
  }

  if (io->maps) {
    uint32_t i;
    uint32_t max_mapped_segment;
    uint32_t segment_size;

    max_mapped_segment = grn_io_max_segment(io);
    segment_size = header->segment_size;
    for (i = 0; i < max_mapped_segment; i++) {
      grn_io_mapinfo *info = &(io->maps[i]);
      uint32_t nth_file_info;
      uint32_t *pnref;
      uint32_t nref;
      int msync_result;

      if (!info) {
        continue;
      }

      pnref = &info->nref;
      GRN_ATOMIC_ADD_EX(pnref, 1, nref);
      if (nref != 0) {
        GRN_ATOMIC_ADD_EX(pnref, -1, nref);
        continue;
      }

      if (!info->map) {
        GRN_ATOMIC_ADD_EX(pnref, -1, nref);
        GRN_FUTEX_WAKE(pnref);
        continue;
      }

      nth_file_info = grn_io_compute_nth_file_info(io, i);
      msync_result = GRN_MSYNC(ctx,
                               &(io->fis[nth_file_info]),
                               info->map,
                               segment_size);
      GRN_ATOMIC_ADD_EX(pnref, -1, nref);
      GRN_FUTEX_WAKE(pnref);

      if (msync_result != 0) {
        rc = ctx->rc;
        break;
      }
    }
  }

  return rc;
}

grn_bool
grn_io_is_corrupt(grn_ctx *ctx, grn_io *io)
{
  uint32_t i;
  uint32_t n_files;

  if (!io) {
    return GRN_FALSE;
  }

  n_files = grn_io_n_files(ctx, io);
  for (i = 0; i < n_files; i++) {
    char path[PATH_MAX];
    struct stat s;
    gen_pathname(io->path, path, i);
    if (stat(path, &s) != 0) {
      SERR("[io][corrupt] used path doesn't exist: <%s>",
           path);
      return GRN_TRUE;
    }
  }

  return GRN_FALSE;
}

size_t
grn_io_get_disk_usage(grn_ctx *ctx, grn_io *io)
{
  size_t usage = 0;
  uint32_t i;
  uint32_t n_files;

  if (!io) {
    return usage;
  }

  n_files = grn_io_n_files(ctx, io);
  for (i = 0; i < n_files; i++) {
    char path[PATH_MAX];
    struct stat s;
    gen_pathname(io->path, path, i);
    if (stat(path, &s) != 0) {
      continue;
    }
    usage += s.st_size;
  }

  return usage;
}

/** mmap abstraction **/

static size_t mmap_size = 0;

#ifdef WIN32

grn_inline static grn_rc
grn_fileinfo_open_v1(grn_ctx *ctx, fileinfo *fi, const char *path, int flags)
{
  CRITICAL_SECTION_INIT(fi->cs);
  return GRN_SUCCESS;
}

grn_inline static void *
grn_mmap_v1(grn_ctx *ctx, grn_ctx *owner_ctx, HANDLE *fmo, fileinfo *fi,
            int64_t offset, size_t length)
{
  void *res;
  if (!fi) {
    if (fmo) {
      *fmo = NULL;
    }
    /* TODO: Try to support VirtualAlloc() as anonymous mmap in POSIX.
     * If VirtualAlloc() provides better performance rather than malloc(),
     * we'll use it.
     */
    return GRN_CALLOC(length);
  }
  /* CRITICAL_SECTION_ENTER(fi->cs); */
  /* try to create fmo */
  /* TODO: Add support for 32bit over offset by using dwMaximumSizeHigh. */
  *fmo = CreateFileMapping(fi->fh, NULL, PAGE_READWRITE, 0, offset + length, NULL);
  if (!*fmo) {
    SERR("CreateFileMapping(%lu + %" GRN_FMT_SIZE ") failed "
         "<%" GRN_FMT_SIZE ">",
         (DWORD)offset, length,
         mmap_size);
    return NULL;
  }
  res = MapViewOfFile(*fmo, FILE_MAP_WRITE, 0, (DWORD)offset, (SIZE_T)length);
  if (!res) {
    SERR("MapViewOfFile(%lu,%" GRN_FMT_SIZE ") failed <%" GRN_FMT_SIZE ">",
         (DWORD)offset, length, mmap_size);
    return NULL;
  }
  /* CRITICAL_SECTION_LEAVE(fi->cs); */
  mmap_size += length;
  return res;
}

grn_inline static int
grn_munmap_v1(grn_ctx *ctx, grn_ctx *owner_ctx, HANDLE *fmo, fileinfo *fi,
              void *start, size_t length)
{
  int r = 0;

  if (!fi) {
    GRN_FREE(start);
    return r;
  }

  if (!fmo) {
    GRN_FREE(start);
    return r;
  }

  if (*fmo) {
    if (!FlushViewOfFile(start, length)) {
      SERR("FlushViewOfFile(<%p>, <%" GRN_FMT_SIZE ">) failed on unmap",
           start, length);
      r = -1;
    }
    if (UnmapViewOfFile(start)) {
      mmap_size -= length;
    } else {
      SERR("UnmapViewOfFile(%p,%" GRN_FMT_SIZE ") failed <%" GRN_FMT_SIZE ">",
           start, length, mmap_size);
      r = -1;
    }
    if (!CloseHandle(*fmo)) {
      SERR("CloseHandle(%p,%" GRN_FMT_SIZE ") failed <%" GRN_FMT_SIZE ">",
           start, length, mmap_size);
    }
    *fmo = NULL;
  } else {
    GRN_FREE(start);
  }

  return r;
}

grn_inline static grn_rc
grn_fileinfo_open_v0(grn_ctx *ctx, fileinfo *fi, const char *path, int flags)
{
  /* signature may be wrong.. */
  fi->fmo = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, NULL);
  /* open failed */
  if (fi->fmo == NULL) {
    // flock
    /* retry to open */
    fi->fmo = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, NULL);
    /* failed again */
    if (fi->fmo == NULL) {
      /* try to create fmo */
      fi->fmo = CreateFileMapping(fi->fh, NULL, PAGE_READWRITE, 0,
                                  GRN_IO_FILE_SIZE_V0, NULL);
    }
    // funlock
  }
  if (fi->fmo != NULL) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
      CRITICAL_SECTION_INIT(fi->cs);
      return GRN_SUCCESS;
    } else {
      GRN_LOG(ctx, GRN_LOG_ERROR,
              "fmo object already exists! handle=%p", fi->fh);
      CloseHandle(fi->fmo);
    }
  } else {
    GRN_LOG(ctx, GRN_LOG_ALERT,
            "failed to get FileMappingObject #%lu", GetLastError());
  }
  CloseHandle(fi->fh);
  SERR("OpenFileMapping");
  if (ctx->rc != GRN_SUCCESS && fi->path) {
    free(fi->path);
    fi->path = NULL;
  }
  return ctx->rc;
}

grn_inline static void *
grn_mmap_v0(grn_ctx *ctx, grn_ctx *owner_ctx, fileinfo *fi, int64_t offset,
            size_t length)
{
  void *res;
  if (!fi) { return GRN_CALLOC(length); }
  /* file must be exceeded to GRN_IO_FILE_SIZE_V0 when FileMappingObject created.
     and, after fmo created, it's not allowed to expand the size of file.
  DWORD tail = (DWORD)(offset + length);
  DWORD filesize = GetFileSize(fi->fh, NULL);
  if (filesize < tail) {
    if (SetFilePointer(fi->fh, tail, NULL, FILE_BEGIN) != tail) {
      grn_log("SetFilePointer failed");
      return NULL;
    }
    if (!SetEndOfFile(fi->fh)) {
      grn_log("SetEndOfFile failed");
      return NULL;
    }
    filesize = tail;
  }
  */
  /* TODO: Add support for 32bit over offset by using dwFileOffsetHigh. */
  res = MapViewOfFile(fi->fmo, FILE_MAP_WRITE, 0, (DWORD)offset, (SIZE_T)length);
  if (!res) {
    MERR("MapViewOfFile failed: <%" GRN_FMT_SIZE ">: %s",
         mmap_size, grn_current_error_message());
    return NULL;
  }
  mmap_size += length;
  return res;
}

grn_inline static int
grn_munmap_v0(grn_ctx *ctx, grn_ctx *owner_ctx, fileinfo *fi, void *start,
              size_t length)
{
  int r = 0;

  if (!fi) {
    GRN_FREE(start);
    return 0;
  }

  if (!FlushViewOfFile(start, length)) {
    SERR("FlushViewOfFile(<%p>, <%" GRN_FMT_SIZE ">) failed on unmap",
         start, length);
    r = -1;
  }
  if (UnmapViewOfFile(start)) {
    mmap_size -= length;
    return r;
  } else {
    SERR("UnmapViewOfFile(%p,%" GRN_FMT_SIZE ") failed <%" GRN_FMT_SIZE ">",
         start, length, mmap_size);
    return -1;
  }
}

grn_inline static bool
grn_fileinfo_open_common(grn_ctx *ctx, fileinfo *fi, const char *path, int flags)
{
  bool success = true;

  fi->path = grn_strdup_raw(path);

  /* may be wrong if flags is just only O_RDWR */
  if ((flags & O_CREAT)) {
    grn_bool exist = GRN_FALSE;
    DWORD dwCreationDisposition;
    const char *flags_description;

    {
      struct stat stat_buffer;
      if (stat(path, &stat_buffer) == 0) {
        exist = GRN_TRUE;
      }
    }

    if (flags & O_EXCL) {
      dwCreationDisposition = CREATE_NEW;
      flags_description = "O_RDWR|O_CREAT|O_EXCL";
    } else {
      dwCreationDisposition = OPEN_ALWAYS;
      flags_description = "O_RDWR|O_CREAT";
    }
    fi->fh = CreateFile(path, GRN_IO_FILE_CREATE_MODE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        dwCreationDisposition, FILE_ATTRIBUTE_NORMAL, 0);
    if (fi->fh == INVALID_HANDLE_VALUE) {
      SERR("CreateFile(<%s>, <%s>) failed",
           path, flags_description);
      success = false;
      if (!exist) {
        struct stat stat_buffer;
        if (stat(path, &stat_buffer) == 0) {
          GRN_LOG(ctx, GRN_LOG_INFO,
                  "[io][open] "
                  "delete a newly created file because of open error: <%s>",
                  path);
          DeleteFile(path);
        }
      }
      goto exit;
    }

    switch (dwCreationDisposition) {
    case CREATE_NEW :
      GRN_LOG(ctx, GRN_LOG_INFO,
              "[io][open] create new file: <%s>", path);
      break;
    case OPEN_ALWAYS :
      if (GetLastError() == ERROR_ALREADY_EXISTS) {
        GRN_LOG(ctx, GRN_LOG_INFO,
                "[io][open] open existing file because it exists: <%s>", path);
      } else {
        GRN_LOG(ctx, GRN_LOG_INFO,
                "[io][open] create new file because it doesn't exist: <%s>",
                path);
      }
      break;
    default :
      break;
    }

    {
      FILE_SET_SPARSE_BUFFER buffer;
      buffer.SetSparse = TRUE;
      DWORD returned_bytes;
      if (!DeviceIoControl(fi->fh,
                           FSCTL_SET_SPARSE,
                           &buffer,
                           sizeof(FILE_SET_SPARSE_BUFFER),
                           NULL,
                           0,
                           &returned_bytes,
                           NULL)) {
        GRN_LOG(ctx, GRN_LOG_INFO,
                "Tried to make file sparse but failed: "
                "DeviceIoControl(FSCTL_SET_SPARSE): "
                "<%s>: <%s>",
                path, grn_current_error_message());
      }
    }

    goto exit;
  }

  if ((flags & O_TRUNC)) {
    CloseHandle(fi->fh);
    /* unable to assign OPEN_ALWAYS and TRUNCATE_EXISTING at once */
    fi->fh = CreateFile(path, GRN_IO_FILE_CREATE_MODE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        NULL,
                        TRUNCATE_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (fi->fh == INVALID_HANDLE_VALUE) {
      SERR("CreateFile(<%s>, <O_RDWR|O_TRUNC>) failed",
           path);
      success = false;
      goto exit;
    }
    GRN_LOG(ctx, GRN_LOG_INFO,
            "[io][open] truncated: <%s>", path);
    goto exit;
  }
  /* O_RDWR only */
  fi->fh = CreateFile(path, GRN_IO_FILE_CREATE_MODE,
                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                      NULL,
                      OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (fi->fh == INVALID_HANDLE_VALUE) {
    SERR("CreateFile(<%s>, <O_RDWR>) failed",
         path);
    success = false;
    goto exit;
  }
  GRN_LOG(ctx, GRN_LOG_INFO,
          "[io][open] open existing file: <%s>", path);

exit :
  if (!success && fi->path) {
    free(fi->path);
    fi->path = NULL;
  }
  return success;
}

grn_inline static grn_rc
grn_fileinfo_open(grn_ctx *ctx, fileinfo *fi, const char *path, int flags)
{
  struct _grn_io_header io_header;
  LARGE_INTEGER file_size;
  int version = grn_io_version_default;

  if (!grn_fileinfo_open_common(ctx, fi, path, flags)) {
    if (fi->fh != INVALID_HANDLE_VALUE) {
      CloseHandle(fi->fh);
      fi->fh = INVALID_HANDLE_VALUE;
    }
    return ctx->rc;
  }

  if (GetFileSizeEx(fi->fh, &file_size) && file_size.QuadPart > 0) {
    DWORD header_size;
    DWORD read_bytes;
    header_size = sizeof(struct _grn_io_header);
    ReadFile(fi->fh, &io_header, header_size, &read_bytes, NULL);
    if (read_bytes == header_size) {
      version = io_header.version;
    }
    SetFilePointer(fi->fh, 0, NULL, FILE_BEGIN);
  }

  if (version == 0) {
    return grn_fileinfo_open_v0(ctx, fi, path, flags);
  } else {
    return grn_fileinfo_open_v1(ctx, fi, path, flags);
  }
}

grn_inline static int
grn_guess_io_version(grn_ctx *ctx, grn_io *io, fileinfo *fi)
{
  if (io) {
    return io->header->version;
  }

  if (fi) {
    if (fi->fmo) {
      return 0;
    } else {
      return 1;
    }
  }

  return grn_io_version_default;
}

grn_inline static void *
grn_mmap(grn_ctx *ctx,
         grn_ctx *owner_ctx,
         grn_io *io,
         HANDLE *fmo,
         fileinfo *fi,
         int64_t offset,
         size_t length,
         const char *file,
         int line,
         const char *func)
{
  if (grn_fail_malloc_should_fail(length, file, line, func)) {
    MERR("[alloc][fail][mmap] <%d>: <%" GRN_FMT_SIZE ">: <%s>: "
         "<%p:%" GRN_FMT_INT64D ":%" GRN_FMT_SIZE ">: "
         "%s:%d: %s",
         grn_alloc_count(),
         mmap_size,
         io ? (io->path[0] ? io->path : "(memory)") : "(null)",
         fi ? fi->fh : NULL,
         offset,
         length,
         file,
         line,
         func);
    return NULL;
  } else {
    int version;

    version = grn_guess_io_version(ctx, io, fi);

    if (version == 0) {
      return grn_mmap_v0(ctx, owner_ctx, fi, offset, length);
    } else {
      return grn_mmap_v1(ctx, owner_ctx, fmo, fi, offset, length);
    }
  }
}

grn_inline static int
grn_munmap(grn_ctx *ctx, grn_ctx *owner_ctx, grn_io *io,
           HANDLE *fmo, fileinfo *fi, void *start, size_t length)
{
  int version;

  version = grn_guess_io_version(ctx, io, fi);

  if (version == 0) {
    return grn_munmap_v0(ctx, owner_ctx, fi, start, length);
  } else {
    return grn_munmap_v1(ctx, owner_ctx, fmo, fi, start, length);
  }
}

grn_inline static grn_rc
grn_fileinfo_close(grn_ctx *ctx, fileinfo *fi)
{
  if (fi->fmo != NULL) {
    CloseHandle(fi->fmo);
    fi->fmo = NULL;
  }
  if (fi->fh != INVALID_HANDLE_VALUE) {
    if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
      GRN_LOG(ctx, GRN_LOG_DEBUG, "[io][close] <%s>", fi->path);
    }
    CloseHandle(fi->fh);
    CRITICAL_SECTION_FIN(fi->cs);
    fi->fh = INVALID_HANDLE_VALUE;
  }
  if (fi->path) {
    free(fi->path);
    fi->path = NULL;
  }
  return GRN_SUCCESS;
}

grn_inline static void
grn_fileinfo_init(fileinfo *fis, int nfis)
{
  for (; nfis--; fis++) {
    fis->path = NULL;
    fis->fh = INVALID_HANDLE_VALUE;
    fis->fmo = NULL;
  }
}

grn_inline static int
grn_fileinfo_opened(fileinfo *fi)
{
  return fi->fh != INVALID_HANDLE_VALUE;
}

grn_inline static int
grn_msync(grn_ctx *ctx, fileinfo *fi, void *start, size_t length)
{
  BOOL succeeded;
  SYSTEMTIME system_time;
  FILETIME file_time;

  succeeded = FlushViewOfFile(start, length);
  if (!succeeded) {
    SERR("FlushViewOfFile(<%p>, <%" GRN_FMT_SIZE ">) failed on sync",
         start, length);
    return -1;
  }

  if (fi->fh == INVALID_HANDLE_VALUE) {
    return 0;
  }

  GetSystemTime(&system_time);
  succeeded = SystemTimeToFileTime(&system_time, &file_time);
  if (!succeeded) {
    SERR("SystemTimeToFileTime(<%04u-%02u-%02uT%02u:%02u:%02u.%03u>) failed",
         system_time.wYear,
         system_time.wMonth,
         system_time.wDay,
         system_time.wHour,
         system_time.wMinute,
         system_time.wSecond,
         system_time.wMilliseconds);
    return -1;
  }

  succeeded = SetFileTime(fi->fh, NULL, NULL, &file_time);
  if (!succeeded) {
    SERR("SetFileTime(<%p>, <%p>, <%" GRN_FMT_SIZE ">) failed",
         fi->fh, start, length);
    return -1;
  }

  return 0;
}

grn_inline static grn_rc
grn_pread(grn_ctx *ctx, fileinfo *fi, void *buf, size_t count, off_t offset)
{
  DWORD r, len;
  CRITICAL_SECTION_ENTER(fi->cs);
  r = SetFilePointer(fi->fh, offset, NULL, FILE_BEGIN);
  if (r == INVALID_SET_FILE_POINTER) {
    SERR("SetFilePointer");
  } else {
    if (!ReadFile(fi->fh, buf, (DWORD)count, &len, NULL)) {
      SERR("ReadFile");
    } else if (len != count) {
      /* todo : should retry ? */
      ERR(GRN_INPUT_OUTPUT_ERROR,
          "ReadFile %" GRN_FMT_SIZE " != %lu",
          count, len);
    }
  }
  CRITICAL_SECTION_LEAVE(fi->cs);
  return ctx->rc;
}

grn_inline static grn_rc
grn_pwrite(grn_ctx *ctx, fileinfo *fi, void *buf, size_t count, off_t offset)
{
  DWORD r, len;
  CRITICAL_SECTION_ENTER(fi->cs);
  r = SetFilePointer(fi->fh, offset, NULL, FILE_BEGIN);
  if (r == INVALID_SET_FILE_POINTER) {
    SERR("SetFilePointer");
  } else {
    if (!WriteFile(fi->fh, buf, (DWORD)count, &len, NULL)) {
      SERR("WriteFile");
    } else if (len != count) {
      /* todo : should retry ? */
      ERR(GRN_INPUT_OUTPUT_ERROR,
          "WriteFile %" GRN_FMT_SIZE " != %lu",
          count, len);
    }
  }
  CRITICAL_SECTION_LEAVE(fi->cs);
  return ctx->rc;
}

#else /* WIN32 */

grn_inline static grn_rc
grn_fileinfo_open(grn_ctx *ctx, fileinfo *fi, const char *path, int flags)
{
  struct stat st;
  fi->path = grn_strdup_raw(path);
  grn_open(fi->fd, path, flags);
  if (fi->fd == -1) {
    ERRNO_ERR("failed to open file info path: <%s>",
              path);
    free(fi->path);
    fi->path = NULL;
    return ctx->rc;
  }
  if (fstat(fi->fd, &st) == -1) {
    ERRNO_ERR("failed to stat file info path: <%s>",
              path);
    grn_close(fi->fd);
    free(fi->path);
    fi->path = NULL;
    return ctx->rc;
  }
  fi->dev = st.st_dev;
  fi->inode = st.st_ino;
  GRN_LOG(ctx, GRN_LOG_DUMP, "[io][open] <%s>", path);
  return GRN_SUCCESS;
}

grn_inline static void
grn_fileinfo_init(fileinfo *fis, int nfis)
{
  for (; nfis--; fis++) {
    fis->path = NULL;
    fis->fd = -1;
  }
}

grn_inline static int
grn_fileinfo_opened(fileinfo *fi)
{
  return fi->fd != -1;
}

grn_inline static grn_rc
grn_fileinfo_close(grn_ctx *ctx, fileinfo *fi)
{
  if (fi->fd != -1) {
    if (grn_close(fi->fd) == -1) {
      SERR("close");
      return ctx->rc;
    }
    fi->fd = -1;
    if (fi->path) {
      GRN_LOG(ctx, GRN_LOG_DUMP, "[io][close] <%s>", fi->path);
    }
  }
  if (fi->path) {
    free(fi->path);
    fi->path = NULL;
  }
  return GRN_SUCCESS;
}

#if defined(MAP_ANON) && !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif

#include <sys/mman.h>

grn_inline static void *
grn_mmap(grn_ctx *ctx,
         grn_ctx *owner_ctx,
         grn_io *io,
         fileinfo *fi,
         int64_t offset,
         size_t length,
         const char *file,
         int line,
         const char *func)
{
  if (grn_fail_malloc_should_fail(length, file, line, func)) {
    MERR("[alloc][fail][mmap] <%d>: <%" GRN_FMT_SIZE ">: <%s>: "
         "<%d:%" GRN_FMT_INT64D ":%" GRN_FMT_SIZE ">: "
         "%s:%d: %s",
         grn_alloc_count(),
         mmap_size,
         io ? (io->path[0] ? io->path : "(memory)") : "(null)",
         fi ? fi->fd : 0,
         offset,
         length,
         file,
         line,
         func);
    return NULL;
  } else {
    void *res;
    int fd, flags;
    if (fi) {
      struct stat s;
      int64_t tail = offset + length;
      fd = fi->fd;
      if ((fstat(fd, &s) == -1) || (s.st_size < tail && ftruncate(fd, tail) == -1)) {
        SERR("fstat");
        return NULL;
      }
      flags = MAP_SHARED;
    } else {
      fd = -1;
      flags = MAP_PRIVATE|MAP_ANONYMOUS;
    }
    res = mmap(NULL, length, PROT_READ|PROT_WRITE, flags, fd, offset);
    if (MAP_FAILED == res) {
      MERR("mmap(%" GRN_FMT_SIZE ",%d,%" GRN_FMT_INT64D ")=%s <%" GRN_FMT_SIZE ">",
           length,
           fd,
           offset,
           strerror(errno),
           mmap_size);
      return NULL;
    }
    mmap_size += length;
    return res;
  }
}

grn_inline static int
grn_msync(grn_ctx *ctx, fileinfo *fi, void *start, size_t length)
{
  int r = msync(start, length, MS_SYNC);
  if (r == -1) {
    SERR("msync");
    return r;
  }

  if (fi->fd > 0) {
    struct timeval time_values[2];
    struct timespec time_specs[2];
    gettimeofday(&(time_values[0]), NULL);
    time_values[1] = time_values[0];
    time_specs[0].tv_sec = time_values[0].tv_sec;
    time_specs[0].tv_nsec = time_values[0].tv_usec * 1000;
    time_specs[1] = time_specs[0];
    r = grn_futimens(fi->fd, time_values, time_specs);
    if (r == -1) {
      SERR("%s failed: <%d>", grn_futimens_name, fi->fd);
    }
  }

  return r;
}

grn_inline static int
grn_munmap(grn_ctx *ctx, grn_ctx *owner_ctx, grn_io *io, fileinfo *fi,
           void *start, size_t length)
{
  int res;
  res = munmap(start, length);
  if (res == 0) {
    mmap_size -= length;
  } else {
    SERR("munmap(%p,%" GRN_FMT_LLU ") failed <%" GRN_FMT_LLU ">",
         start,
         (unsigned long long int)length,
         (unsigned long long int)mmap_size);
  }
  return res;
}

grn_inline static grn_rc
grn_pread(grn_ctx *ctx, fileinfo *fi, void *buf, size_t count, off_t offset)
{
  ssize_t r = pread(fi->fd, buf, count, offset);
  if (r != count) {
    if (r == -1) {
      SERR("pread");
    } else {
      /* todo : should retry ? */
      ERR(GRN_INPUT_OUTPUT_ERROR,
          "pread returned %" GRN_FMT_SSIZE " != %" GRN_FMT_SIZE,
          r,
          count);
    }
    return ctx->rc;
  }
  return GRN_SUCCESS;
}

grn_inline static grn_rc
grn_pwrite(grn_ctx *ctx, fileinfo *fi, void *buf, size_t count, off_t offset)
{
  ssize_t r = pwrite(fi->fd, buf, count, offset);
  if (r != count) {
    if (r == -1) {
      SERR("pwrite");
    } else {
      /* todo : should retry ? */
      ERR(GRN_INPUT_OUTPUT_ERROR,
          "pwrite returned %" GRN_FMT_SSIZE " != %" GRN_FMT_SIZE,
          r,
          count);
    }
    return ctx->rc;
  }
  return GRN_SUCCESS;
}

#endif /* WIN32 */

bool
grn_io_warm_path(grn_ctx *ctx, grn_io *io, const char *path)
{
  FILE *input = grn_fopen(path, "rb");
  if (!input) {
    SERR("[io][warm] failed to open a file: <%s>", path);
    return false;
  }

  GRN_LOG(ctx, GRN_LOG_DUMP, "[io][warm] <%s>", path);
  char buffer[4096];
  while (fread(buffer, 1, sizeof(buffer), input) != 0) {
    /* Do nothing */
  }
  fclose(input);
  return true;
}

grn_rc
grn_io_warm(grn_ctx *ctx, grn_io *io)
{
  if (io->path[0] == '\0') {
    return GRN_SUCCESS;
  }

  char buffer[PATH_MAX];
  uint32_t i;
  uint32_t n_files = grn_io_n_files(ctx, io);
  for (i = 0; i < n_files; i++) {
    gen_pathname(io->path, buffer, i);
    if (!grn_io_warm_path(ctx, io, buffer)) {
      break;
    }
  }
  return ctx->rc;
}
