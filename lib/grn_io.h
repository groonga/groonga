/*
  Copyright (C) 2009-2017  Brazil
  Copyright (C) 2021-2022  Sutou Kouhei <kou@clear-code.com>

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

#pragma once

#include "grn.h"
#include "grn_ctx.h"
#include "grn_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
# define GRN_IO_FILE_CREATE_MODE (GENERIC_READ | GENERIC_WRITE)
#else /* WIN32 */
# define GRN_IO_FILE_CREATE_MODE 0644
#endif /* WIN32 */

typedef enum {
  GRN_IO_RDONLY,
  GRN_IO_WRONLY,
  GRN_IO_RDWR
} grn_io_rw_mode;

typedef enum {
  GRN_IO_AUTO,
  GRN_IO_MANUAL
} grn_io_mode;

/**** grn_io ****/

typedef struct _grn_io grn_io;

typedef struct {
  grn_io *io;
  grn_ctx *ctx;
  uint8_t mode;
  uint8_t tiny_p;
  uint32_t pseg;
  uint32_t segment;
  uint32_t offset;
  uint32_t size;
  uint32_t nseg;
  void *addr;
  uint32_t diff;
  int32_t cached;
#ifdef WIN32
  HANDLE fmo;
#endif /* WIN32 */
  void *uncompressed_value;
} grn_io_win;

typedef struct {
  void *map;
  uint32_t nref;
  uint32_t count;
#ifdef WIN32
  HANDLE fmo;
#endif /* WIN32 */
} grn_io_mapinfo;

typedef struct _grn_io_array_info grn_io_array_info;

struct _grn_io_header {
  char idstr[16];
  uint32_t type;
  uint32_t version;
  uint32_t flags;
  uint32_t header_size;
  uint32_t segment_size;
  uint32_t max_segment;
  uint32_t n_arrays;
  uint32_t lock;
  uint64_t curr_size;
  uint32_t segment_tail;
  uint32_t last_modified;
};

struct _grn_io {
  char path[PATH_MAX];
  struct _grn_io_header *header;
  byte *user_header;
  grn_io_mapinfo *maps;
  uint32_t base;
  uint32_t base_seg;
  grn_io_mode mode;
  struct _grn_io_fileinfo *fis;
  grn_io_array_info *ainfo;
  uint32_t max_map_seg;
  uint32_t nmaps;
  uint32_t nref;
  uint32_t count;
  uint32_t flags;
  uint32_t *lock;
};

GRN_API grn_io *grn_io_create(grn_ctx *ctx, const char *path,
                              uint32_t header_size, uint32_t segment_size,
                              uint32_t max_segment, grn_io_mode mode,
                              unsigned int flags);
grn_io *grn_io_open(grn_ctx *ctx, const char *path, grn_io_mode mode);
GRN_API grn_rc grn_io_close(grn_ctx *ctx, grn_io *io);
grn_rc grn_io_remove(grn_ctx *ctx, const char *path);
grn_rc grn_io_remove_if_exist(grn_ctx *ctx, const char *path);
grn_rc grn_io_size(grn_ctx *ctx, grn_io *io, uint64_t *size);
grn_rc grn_io_rename(grn_ctx *ctx, const char *old_name, const char *new_name);
GRN_API void *grn_io_header(grn_io *io);

void *grn_io_win_map(grn_ctx *ctx,
                     grn_io *io,
                     grn_io_win *iw,
                     uint32_t segment,
                     uint32_t offset,
                     uint32_t size,
                     grn_io_rw_mode mode);
grn_rc grn_io_win_unmap(grn_ctx *ctx, grn_io_win *iw);

typedef struct _grn_io_ja_einfo grn_io_ja_einfo;
typedef struct _grn_io_ja_ehead grn_io_ja_ehead;

struct _grn_io_ja_einfo {
  uint32_t pos;
  uint32_t size;
};

struct _grn_io_ja_ehead {
  uint32_t size;
  uint32_t key;
};

grn_rc grn_io_read_ja(grn_io *io, grn_ctx *ctx, grn_io_ja_einfo *einfo, uint32_t epos,
                      uint32_t key, uint32_t segment, uint32_t offset,
                      void **value, uint32_t *value_len);
grn_rc grn_io_write_ja(grn_io *io, grn_ctx *ctx,
                       uint32_t key, uint32_t segment, uint32_t offset,
                       void *value, uint32_t value_len);

grn_rc grn_io_write_ja_ehead(grn_io *io, grn_ctx *ctx, uint32_t key,
                             uint32_t segment, uint32_t offset, uint32_t value_len);

#define GRN_TABLE_ADD                  (0x01<<6)
#define GRN_TABLE_ADDED                (0x01<<7)

#define GRN_IO_MAX_RETRY               (0x10000)
#define GRN_IO_MAX_REF                 (0x80000000)

#define GRN_IO_EXPIRE_GTICK            (0x01)
#define GRN_IO_EXPIRE_SEGMENT          (0x02)
#define GRN_IO_TEMPORARY               (0x04)

void grn_io_seg_map_(grn_ctx *ctx, grn_io *io, uint32_t segno, grn_io_mapinfo *info);

/*
 * io mustn't be NULL;
 */
static grn_inline void *
grn_io_seg_ref(grn_ctx *ctx, grn_io *io, uint32_t segno)
{
  const char *tag = "[io][seg][ref]";
  grn_io_mapinfo *info = NULL;
  if (segno >= io->header->max_segment) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s too large segment ID: "
        "id:%u, "
        "max:%u, "
        "path:<%s>",
        tag,
        segno,
        io->header->max_segment,
        io->path);
    return NULL;
  }
  info = &(io->maps[segno]);
  uint32_t nref, retry, *pnref = &info->nref;
  if (io->flags & GRN_IO_EXPIRE_SEGMENT) {
    if (io->flags & GRN_IO_EXPIRE_GTICK) {
      for (retry = 0; !info->map || info->count != grn_gtick; retry++) {
        GRN_ATOMIC_ADD_EX(pnref, 1, nref);
        if (nref) {
          GRN_ATOMIC_ADD_EX(pnref, -1, nref);
          if (retry >= GRN_IO_MAX_RETRY) {
            GRN_LOG(ctx, GRN_LOG_CRIT,
                    "deadlock detected! in grn_io_seg_ref(%p, %u)", io, segno);
            break;
          }
          GRN_FUTEX_WAIT(pnref);
        } else {
          info->count = grn_gtick;
          if (!info->map) {
            grn_io_seg_map_(ctx, io, segno, info);
            if (!info->map) {
              CRIT(GRN_NO_MEMORY_AVAILABLE,
                   "%s failed to mmap with expire-segment & expire-gtick: "
                   "id:%u, "
                   "max:%u, "
                   "path:<%s>, "
                   "message:%s",
                   tag,
                   segno,
                   io->header->max_segment,
                   io->path,
                   grn_error_get_current_system_message());
            }
          }
          GRN_ATOMIC_ADD_EX(pnref, -1, nref);
          GRN_FUTEX_WAKE(pnref);
          break;
        }
      }
    } else {
      for (retry = 0;; retry++) {
        GRN_ATOMIC_ADD_EX(pnref, 1, nref);
        if (nref >= GRN_IO_MAX_REF) {
          GRN_ATOMIC_ADD_EX(pnref, -1, nref);
          if (retry >= GRN_IO_MAX_RETRY) {
            GRN_LOG(ctx, GRN_LOG_CRIT,
                    "deadlock detected!! in grn_io_seg_ref(%p, %u, %u)",
                    io, segno, nref);
            *pnref = 0; /* force reset */
            break;
          }
          GRN_FUTEX_WAIT(pnref);
          continue;
        }
        if (nref >= 0x40000000) {
          ALERT("strange nref value!! in grn_io_seg_ref(%p, %u, %u)",
                io, segno, nref);
        }
        if (!info->map) {
          if (nref) {
            GRN_ATOMIC_ADD_EX(pnref, -1, nref);
            if (retry >= GRN_IO_MAX_RETRY) {
              GRN_LOG(ctx, GRN_LOG_CRIT,
                      "deadlock detected!!! in grn_io_seg_ref(%p, %u, %u)",
                      io, segno, nref);
              break;
            }
            GRN_FUTEX_WAIT(pnref);
            continue;
          } else {
            grn_io_seg_map_(ctx, io, segno, info);
            if (!info->map) {
              GRN_ATOMIC_ADD_EX(pnref, -1, nref);
              CRIT(GRN_NO_MEMORY_AVAILABLE,
                   "%s failed to mmap with expire-segment: "
                   "id:%u, "
                   "nref:%u, "
                   "max:%u, "
                   "path:<%s>, "
                   "message:%s",
                   tag,
                   segno,
                   nref,
                   io->header->max_segment,
                   io->path,
                   grn_error_get_current_system_message());
            }
            GRN_FUTEX_WAKE(pnref);
          }
        }
        break;
      }
      info->count = grn_gtick;
    }
  } else {
    for (retry = 0; !info->map; retry++) {
      GRN_ATOMIC_ADD_EX(pnref, 1, nref);
      if (nref) {
        GRN_ATOMIC_ADD_EX(pnref, -1, nref);
        if (retry >= GRN_IO_MAX_RETRY) {
          GRN_LOG(ctx, GRN_LOG_CRIT,
                  "deadlock detected!!!! in grn_io_seg_ref(%p, %u)",
                  io, segno);
          break;
        }
        GRN_FUTEX_WAIT(pnref);
      } else {
        if (!info->map) {
          grn_io_seg_map_(ctx, io, segno, info);
          if (!info->map) {
            CRIT(GRN_NO_MEMORY_AVAILABLE,
                 "%s failed to mmap: "
                 "id:%u, "
                 "max:%u, "
                 "path:<%s>, "
                 "message:%s",
                 tag,
                 segno,
                 io->header->max_segment,
                 io->path,
                 grn_error_get_current_system_message());
          }
        }
        GRN_ATOMIC_ADD_EX(pnref, -1, nref);
        GRN_FUTEX_WAKE(pnref);
        break;
      }
    }
    info->count = grn_gtick;
  }
  return info->map;
}

static grn_inline void
grn_io_seg_unref(grn_ctx *ctx, grn_io *io, uint32_t segno)
{
  if (GRN_IO_EXPIRE_SEGMENT ==
      (io->flags & (GRN_IO_EXPIRE_GTICK|GRN_IO_EXPIRE_SEGMENT))) {
    uint32_t nref, *pnref = &(io)->maps[segno].nref;
    GRN_ATOMIC_ADD_EX(pnref, -1, nref);
  }
}

uint32_t grn_io_base_seg(grn_io *io);
const char *grn_io_path(grn_io *io);

typedef struct _grn_io_array_spec grn_io_array_spec;

struct _grn_io_array_spec {
  uint32_t w_of_element;
  uint32_t max_n_segments;
};

struct _grn_io_array_info {
  uint32_t w_of_elm_in_a_segment;
  uint32_t elm_mask_in_a_segment;
  uint32_t max_n_segments;
  uint32_t element_size;
  uint32_t *segments;
  void **addrs;
};

grn_io *grn_io_create_with_array(grn_ctx *ctx, const char *path, uint32_t header_size,
                                 uint32_t segment_size, grn_io_mode mode,
                                 uint32_t n_arrays, grn_io_array_spec *array_specs);

void grn_io_segment_alloc(grn_ctx *ctx, grn_io *io, grn_io_array_info *ai,
                          uint32_t lseg, int *flags, void **p);

GRN_API grn_rc grn_io_lock(grn_ctx *ctx, grn_io *io, int timeout);
GRN_API void grn_io_unlock(grn_ctx *ctx, grn_io *io);
void grn_io_clear_lock(grn_io *io);
uint32_t grn_io_is_locked(grn_io *io);
grn_bool grn_io_is_corrupt(grn_ctx *ctx, grn_io *io);
size_t grn_io_get_disk_usage(grn_ctx *ctx, grn_io *io);

static grn_inline void *
grn_io_array_at(grn_ctx *ctx,
                grn_io *io,
                uint32_t array,
                uint64_t offset,
                int *flags)
{
  grn_io_array_info *ainfo = &(io->ainfo[array]);
  uint32_t lseg = (uint32_t)(offset >> ainfo->w_of_elm_in_a_segment);
  if (lseg >= ainfo->max_n_segments) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[io][array][at] too large offset: %" GRN_FMT_INT64U
        ": max=%" GRN_FMT_INT64D
        ": nth=%u"
        ": path=<%s>",
        offset,
        (int64_t)((ainfo->max_n_segments << ainfo->w_of_elm_in_a_segment) - 1),
        array,
        io->path);
    return NULL;
  }
  void **p_ = &ainfo->addrs[lseg];
  if (!*p_) {
    grn_io_segment_alloc(ctx, io, ainfo, lseg, flags, p_);
    if (!*p_) { return NULL; }
  }
  return (((byte *)*p_) +
          ((offset & ainfo->elm_mask_in_a_segment) * ainfo->element_size));
}

#define GRN_IO_ARRAY_BIT_AT(io,array,offset,res) do {\
  int flags_ = 0;\
  uint8_t *ptr_ = grn_io_array_at(ctx, (io), (array), ((offset) >> 3) + 1, &flags_,);\
  res = ptr_ ? ((*ptr_ >> ((offset) & 7)) & 1) : 0;\
} while (0)

#define GRN_IO_ARRAY_BIT_ON(io,array,offset) do {\
  int flags_ = GRN_TABLE_ADD;\
  uint8_t *ptr_ = grn_io_array_at(ctx, (io), (array), ((offset) >> 3) + 1, &flags_);\
  if (ptr_) { *ptr_ |= (1 << ((offset) & 7)); }\
} while (0)

#define GRN_IO_ARRAY_BIT_OFF(io,array,offset) do {\
  int flags_ = GRN_TABLE_ADD;\
  uint8_t *ptr = grn_io_array_at(ctx, (io), (array), ((offset) >> 3) + 1, &flags_);\
  if (ptr_) { *ptr_ &= ~(1 << ((offset) & 7)); }\
} while (0)

#define GRN_IO_ARRAY_BIT_FLIP(io,array,offset) do {\
  int flags_ = GRN_TABLE_ADD;\
  uint8_t *ptr_ = grn_io_array_at((io), (array), ((offset) >> 3) + 1, &flags_);\
  if (ptr_) { *ptr_ ^= (1 << ((offset) & 7)); }\
} while (0)

void *grn_io_anon_map(grn_ctx *ctx, grn_io_mapinfo *mi, size_t length);
void grn_io_anon_unmap(grn_ctx *ctx, grn_io_mapinfo *mi, size_t length);
uint32_t grn_io_detect_type(grn_ctx *ctx, const char *path);
grn_rc grn_io_set_type(grn_io *io, uint32_t type);
uint32_t grn_io_get_type(grn_io *io);

void grn_io_init_from_env(void);

uint32_t grn_io_expire(grn_ctx *ctx, grn_io *io, uint32_t count_thresh, uint32_t limit);

grn_rc grn_io_flush(grn_ctx *ctx, grn_io *io);

bool grn_io_warm_path(grn_ctx *ctx, grn_io *io, const char *path);
grn_rc grn_io_warm(grn_ctx *ctx, grn_io *io);

/* encode/decode */

#define GRN_B_ENC_MAX_SIZE 5

/* B is for ???
 *
 * v: The uint32_t value to be encoded.
 *
 * v < 0x8f:
 *  +--------+
 *  |1000AAAA|
 *  +--------+
 *  A = v
 *
 * 0x8f <= v < 0x408f:
 *  +--------+--------+
 *  |11AAAAAA|BBBBBBBB|
 *  +--------+--------+
 *  A = ((v - 0x8f) >> 8)
 *  B = ((v - 0x8f) & 0xff)
 *
 * 0x408f <= v < 0x20408f:
 *  +--------+--------+--------+
 *  |101AAAAA|BBBBBBBB|CCCCCCCC|
 *  +--------+--------+--------+
 *  A = ((v - 0x408f) >> 16)
 *  B = ((v - 0x408f) >> 8) & 0xff
 *  C = ((v - 0x408f) & 0xff)
 *
 * 0x20408f <= v < 0x1020408f:
 *  +--------+--------+--------+--------+
 *  |1001AAAA|BBBBBBBB|CCCCCCCC|DDDDDDDD|
 *  +--------+--------+--------+--------+
 *  A = ((v - 0x20408f) >> 24)
 *  B = ((v - 0x20408f) >> 16) & 0xff
 *  C = ((v - 0x20408f) >>  8) & 0xff
 *  D = ((v - 0x20408f) & 0xff)
 *
 * 0x1020408f <= v:
 *  +--------+--------+--------+--------+--------+
 *  |10001111|AAAAAAAA|BBBBBBBB|CCCCCCCC|DDDDDDDD|
 *  +--------+--------+--------+--------+--------+
 *  A = (v >> 24) & 0xff
 *  B = (v >> 16) & 0xff
 *  C = (v >>  8) & 0xff
 *  D = (v & 0xff)
 * */
#define GRN_B_ENC(v,p) do {\
  uint8_t *_p = (uint8_t *)p; \
  uint32_t _v = v; \
  if (_v < 0x8f) { \
    *_p++ = (uint8_t)_v; \
  } else if (_v < 0x408f) { \
    _v -= 0x8f; \
    *_p++ = (uint8_t)(0xc0 + (_v >> 8)); \
    *_p++ = _v & 0xff; \
  } else if (_v < 0x20408f) { \
    _v -= 0x408f; \
    *_p++ = (uint8_t)(0xa0 + (_v >> 16)); \
    *_p++ = (_v >> 8) & 0xff; \
    *_p++ = _v & 0xff; \
  } else if (_v < 0x1020408f) { \
    _v -= 0x20408f; \
    *_p++ = (uint8_t)(0x90 + (_v >> 24)); \
    *_p++ = (_v >> 16) & 0xff; \
    *_p++ = (_v >> 8) & 0xff; \
    *_p++ = _v & 0xff; \
  } else { \
    *_p++ = 0x8f; \
    grn_memcpy(_p, &_v, sizeof(uint32_t));\
    _p += sizeof(uint32_t); \
  } \
  p = _p; \
} while (0)

#define GRN_B_ENC_SIZE(v) \
 ((v) < 0x8f ? 1 : ((v) < 0x408f ? 2 : ((v) < 0x20408f ? 3 : ((v) < 0x1020408f ? 4 : 5))))

#define GRN_B_DEC(v,p) do { \
  uint8_t *_p = (uint8_t *)p; \
  uint32_t _v = *_p++; \
  switch (_v >> 4) { \
  case 0x08 : \
    if (_v == 0x8f) { \
      grn_memcpy(&_v, _p, sizeof(uint32_t));\
      _p += sizeof(uint32_t); \
    } \
    break; \
  case 0x09 : \
    _v = (_v - 0x90) * 0x100 + *_p++; \
    _v = _v * 0x100 + *_p++; \
    _v = _v * 0x100 + *_p++ + 0x20408f; \
    break; \
  case 0x0a : \
  case 0x0b : \
    _v = (_v - 0xa0) * 0x100 + *_p++; \
    _v = _v * 0x100 + *_p++ + 0x408f; \
    break; \
  case 0x0c : \
  case 0x0d : \
  case 0x0e : \
  case 0x0f : \
    _v = (_v - 0xc0) * 0x100 + *_p++ + 0x8f; \
    break; \
  } \
  v = _v; \
  p = _p; \
} while (0)

#define GRN_B_SKIP(p) do { \
  uint8_t *_p = (uint8_t *)p; \
  uint32_t _v = *_p++; \
  switch (_v >> 4) { \
  case 0x08 : \
    if (_v == 0x8f) { \
      _p += sizeof(uint32_t); \
    } \
    break; \
  case 0x09 : \
    _p += 3; \
    break; \
  case 0x0a : \
  case 0x0b : \
    _p += 2; \
    break; \
  case 0x0c : \
  case 0x0d : \
  case 0x0e : \
  case 0x0f : \
    _p += 1; \
    break; \
  } \
  p = _p; \
} while (0)

#define GRN_B_COPY(p2,p1) do { \
  uint32_t size = 0, _v = *p1++; \
  *p2++ = _v; \
  switch (_v >> 4) { \
  case 0x08 : \
    size = (_v == 0x8f) ? 4 : 0; \
    break; \
  case 0x09 : \
    size = 3; \
    break; \
  case 0x0a : \
  case 0x0b : \
    size = 2; \
    break; \
  case 0x0c : \
  case 0x0d : \
  case 0x0e : \
  case 0x0f : \
    size = 1; \
    break; \
  } \
  while (size--) { *p2++ = *p1++; } \
} while (0)

#ifdef __cplusplus
}
#endif
