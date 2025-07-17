/*
  Copyright (C) 2009-2016  Brazil
  Copyright (C) 2019-2024  Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_ctx.h"
#include "grn_hash.h"
#include "grn_io.h"

#ifdef __cplusplus
extern "C" {
#endif

/**** fixed sized elements ****/

typedef struct _grn_ra grn_ra;

struct _grn_ra {
  grn_db_obj obj;
  grn_io *io;
  int element_width;
  int element_mask;
  struct grn_ra_header *header;
  grn_raw_string generator;
  grn_obj *parsed_generator;
};

struct grn_ra_header {
  uint32_t element_size;
  uint32_t nrecords; /* nrecords is not maintained by default */
  grn_column_flags flags;
  uint64_t wal_id;
  uint32_t reserved[7];
};

grn_ra *
grn_ra_create(grn_ctx *ctx,
              const char *path,
              uint32_t element_size,
              grn_column_flags flags);
grn_ra *
grn_ra_open(grn_ctx *ctx, const char *path);
grn_rc
grn_ra_info(grn_ctx *ctx, grn_ra *ra, uint32_t *element_size);
grn_column_flags
grn_ra_get_flags(grn_ctx *ctx, grn_ra *ra);
grn_rc
grn_ra_close(grn_ctx *ctx, grn_ra *ra);
grn_rc
grn_ra_remove(grn_ctx *ctx, const char *path);
void *
grn_ra_ref(grn_ctx *ctx, grn_ra *ra, grn_id id);
grn_rc
grn_ra_unref(grn_ctx *ctx, grn_ra *ra, grn_id id);
grn_rc
grn_ra_set_value(
  grn_ctx *ctx, grn_ra *ra, grn_id id, grn_obj *value, int flags);
grn_obj *
grn_ra_cast_value(
  grn_ctx *ctx, grn_ra *ra, grn_obj *value, grn_obj *buffer, int set_flags);
grn_rc
grn_ra_wal_recover(grn_ctx *ctx, grn_ra *ra);
grn_rc
grn_ra_warm(grn_ctx *ctx, grn_ra *ra);

typedef struct _grn_ra_cache grn_ra_cache;

struct _grn_ra_cache {
  void *p;
  int32_t seg;
};

#define GRN_RA_CACHE_INIT(ra, c)                                               \
  do {                                                                         \
    (c)->p = NULL;                                                             \
    (c)->seg = -1;                                                             \
  } while (0)

#define GRN_RA_CACHE_FIN(ctx, ra, c)                                           \
  do {                                                                         \
    if ((c)->seg != -1) {                                                      \
      grn_io_seg_unref(ctx, (ra)->io, (uint32_t)((c)->seg));                   \
    }                                                                          \
  } while (0);

void *
grn_ra_ref_cache(grn_ctx *ctx, grn_ra *ra, grn_id id, grn_ra_cache *cache);

grn_rc
grn_ra_set_generator(grn_ctx *ctx, grn_ra *ja, grn_raw_string generator);

/**** variable sized elements ****/

typedef struct _grn_ja grn_ja;

struct _grn_ja {
  grn_db_obj obj;
  grn_io *io;
  struct grn_ja_header *header;
  grn_raw_string generator;
  grn_obj *parsed_generator;
};

void
grn_ja_init_from_env(void);
const char *
grn_ja_segment_info_type_name(grn_ctx *ctx, uint32_t info);
uint32_t
grn_ja_segment_info_value(grn_ctx *ctx, uint32_t info);

GRN_API grn_ja *
grn_ja_create(grn_ctx *ctx,
              const char *path,
              uint32_t max_element_size,
              uint32_t flags);
grn_ja *
grn_ja_open(grn_ctx *ctx, const char *path);
grn_rc
grn_ja_info(grn_ctx *ctx, grn_ja *ja, unsigned int *max_element_size);
grn_column_flags
grn_ja_get_flags(grn_ctx *ctx, grn_ja *ja);
void
grn_ja_set_visibility(grn_ctx *ctx, grn_ja *ja, bool is_visible);
GRN_API grn_rc
grn_ja_close(grn_ctx *ctx, grn_ja *ja);
grn_rc
grn_ja_remove(grn_ctx *ctx, const char *path);
grn_rc
grn_ja_put(grn_ctx *ctx,
           grn_ja *ja,
           grn_id id,
           void *value,
           uint32_t value_len,
           int flags,
           uint64_t *cas);
grn_obj *
grn_ja_cast_value(
  grn_ctx *ctx, grn_ja *ja, grn_obj *value, grn_obj *buffer, int set_flags);
grn_rc
grn_ja_pack_value(grn_ctx *ctx, grn_ja *ja, grn_obj *value, int set_flags);
int
grn_ja_at(grn_ctx *ctx, grn_ja *ja, grn_id id, void *valbuf, int buf_size);

GRN_API void *
grn_ja_ref(
  grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len);
grn_obj *
grn_ja_get_value(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_obj *value);

GRN_API grn_rc
grn_ja_unref(grn_ctx *ctx, grn_io_win *iw);
int
grn_ja_defrag(grn_ctx *ctx, grn_ja *ja, int threshold);

GRN_API grn_rc
grn_ja_putv(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_obj *vector, int flags);
GRN_API uint32_t
grn_ja_size(grn_ctx *ctx, grn_ja *ja, grn_id id);
GRN_API bool
grn_ja_is_empty(grn_ctx *ctx, grn_ja *ja, grn_id id);

void
grn_ja_check(grn_ctx *ctx, grn_ja *ja);
grn_rc
grn_ja_wal_recover(grn_ctx *ctx, grn_ja *ja);
grn_rc
grn_ja_warm(grn_ctx *ctx, grn_ja *ja);

grn_rc
grn_ja_set_generator(grn_ctx *ctx, grn_ja *ja, grn_raw_string generator);

#define GRN_JA_READER_INITIAL_REF_SEG_IDS_SIZE 16

/*
 * grn_ja_reader is designed to improve the performance of sequential access.
 */
typedef struct {
  grn_ja *ja;                /* Target jagged array (without ref. count). */
  uint32_t einfo_seg_id;     /* ID of the current header segment. */
  void *einfo_seg_addr;      /* Address of the current header segment. */
  void *einfo;               /* Header of the current value. */
  bool ref_avail;            /* grn_ja_reader_ref() is available or not. */
  uint32_t ref_seg_id;       /* ID of the current referenced segment. */
  void *ref_seg_addr;        /* Address of the current referenced segment. */
  uint32_t *ref_seg_ids;     /* IDs of referenced segments. */
  uint32_t nref_seg_ids;     /* Number of referenced segments. */
  uint32_t ref_seg_ids_size; /* Maximum number of referenced segments. */
  uint32_t body_seg_id;      /* ID of the current body segment. */
  uint32_t body_seg_offset;  /* Offset in the current body segment. */
  void *body_seg_addr;       /* Address of the current body segment. */
  uint32_t value_size;       /* Size of the current value. */
  uint32_t packed_size;      /* Compressed size of the current value. */
  void *packed_buf;          /* Buffer for decompression. */
  uint32_t packed_buf_size;  /* Size of the buffer for decompression. */
  void *stream;              /* Stream of a compression library. */
} grn_ja_reader;

/*
 * grn_ja_reader_init() initializes a reader.
 * An initialized reader must be finalized by grn_ja_reader_fin().
 */
grn_rc
grn_ja_reader_init(grn_ctx *ctx, grn_ja_reader *reader, grn_ja *ja);

/* grn_ja_reader_fin() finalizes a reader. */
grn_rc
grn_ja_reader_fin(grn_ctx *ctx, grn_ja_reader *reader);

/*
 * grn_ja_reader_open() creates a reader.
 * A created reader must be destroyed by grn_ja_reader_close().
 */
grn_rc
grn_ja_reader_open(grn_ctx *ctx, grn_ja *ja, grn_ja_reader **reader);

/* grn_ja_reader_close() destroys a reader. */
grn_rc
grn_ja_reader_close(grn_ctx *ctx, grn_ja_reader *reader);

/*
 * grn_ja_reader_seek() prepares to access a value specified by `id`.
 * On success, `reader->value_size` is set.
 */
grn_rc
grn_ja_reader_seek(grn_ctx *ctx, grn_ja_reader *reader, grn_id id);

/*
 * grn_ja_reader_ref() gets the address to the current value.
 * This function is available if `reader->ref_avail` is true.
 */
grn_rc
grn_ja_reader_ref(grn_ctx *ctx, grn_ja_reader *reader, void **addr);

/* grn_ja_reader_unref() frees references returned by grn_ja_reader_ref(). */
grn_rc
grn_ja_reader_unref(grn_ctx *ctx, grn_ja_reader *reader);

/* grn_ja_reader_read() reads the current value to `buf`. */
grn_rc
grn_ja_reader_read(grn_ctx *ctx, grn_ja_reader *reader, void *buf);

/*
 * grn_ja_reader_pread() reads a part of the current value to `buf`.
 * If `offset` and `size` are invalid, the behavior is undefined.
 * FIXME: Compressed values are not supported yet.
 */
grn_rc
grn_ja_reader_pread(
  grn_ctx *ctx, grn_ja_reader *reader, size_t offset, size_t size, void *buf);

#ifdef __cplusplus
}
#endif
