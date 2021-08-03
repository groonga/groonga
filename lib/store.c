/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2020-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_token_cursor.h"

#include "grn.h"
#include "grn_str.h"
#include "grn_store.h"
#include "grn_ctx_impl.h"
#include "grn_output.h"
#include "grn_db.h"
#include "grn_vector.h"
#include "grn_wal.h"
#include <string.h>

/* rectangular arrays */

#define GRN_RA_W_SEGMENT    22
#define GRN_RA_SEGMENT_SIZE (1 << GRN_RA_W_SEGMENT)

static grn_ra *
_grn_ra_create(grn_ctx *ctx, grn_ra *ra, const char *path, unsigned int element_size)
{
  grn_io *io;
  int max_segments, n_elm, w_elm;
  struct grn_ra_header *header;
  unsigned int actual_size;
  if (element_size > GRN_RA_SEGMENT_SIZE) {
    GRN_LOG(ctx, GRN_LOG_ERROR, "element_size too large (%d)", element_size);
    return NULL;
  }
  for (actual_size = 1; actual_size < element_size; actual_size *= 2) ;
  max_segments = ((GRN_ID_MAX + 1) / GRN_RA_SEGMENT_SIZE) * actual_size;
  io = grn_io_create(ctx, path, sizeof(struct grn_ra_header),
                     GRN_RA_SEGMENT_SIZE, max_segments, GRN_IO_AUTO,
                     GRN_IO_EXPIRE_SEGMENT);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  grn_io_set_type(io, GRN_COLUMN_FIX_SIZE);
  header->element_size = actual_size;
  n_elm = GRN_RA_SEGMENT_SIZE / header->element_size;
  for (w_elm = GRN_RA_W_SEGMENT; (1 << w_elm) > n_elm; w_elm--);
  ra->io = io;
  ra->header = header;
  ra->element_mask =  n_elm - 1;
  ra->element_width = w_elm;
  return ra;
}

grn_ra *
grn_ra_create(grn_ctx *ctx, const char *path, unsigned int element_size)
{
  grn_ra *ra = (grn_ra *)GRN_CALLOC(sizeof(grn_ra));
  if (!ra) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ra, GRN_COLUMN_FIX_SIZE);
  if (!_grn_ra_create(ctx, ra, path, element_size)) {
    GRN_FREE(ra);
    return NULL;
  }
  return ra;
}

grn_ra *
grn_ra_open(grn_ctx *ctx, const char *path)
{
  grn_io *io;
  int n_elm, w_elm;
  grn_ra *ra = NULL;
  struct grn_ra_header *header;
  uint32_t io_type;
  io = grn_io_open(ctx, path, GRN_IO_AUTO);
  if (!io) { return NULL; }
  header = grn_io_header(io);
  io_type = grn_io_get_type(io);
  if (io_type != GRN_COLUMN_FIX_SIZE) {
    ERR(GRN_INVALID_FORMAT,
        "[column][fix-size] file type must be %#04x: <%#04x>",
        GRN_COLUMN_FIX_SIZE, io_type);
    grn_io_close(ctx, io);
    return NULL;
  }
  ra = GRN_MALLOCN(grn_ra, 1);
  if (!ra) {
    grn_io_close(ctx, io);
    return NULL;
  }
  n_elm = GRN_RA_SEGMENT_SIZE / header->element_size;
  for (w_elm = GRN_RA_W_SEGMENT; (1 << w_elm) > n_elm; w_elm--);
  GRN_DB_OBJ_SET_TYPE(ra, GRN_COLUMN_FIX_SIZE);
  ra->io = io;
  ra->header = header;
  ra->element_mask =  n_elm - 1;
  ra->element_width = w_elm;
  return ra;
}

grn_rc
grn_ra_info(grn_ctx *ctx, grn_ra *ra, unsigned int *element_size)
{
  if (!ra) { return GRN_INVALID_ARGUMENT; }
  if (element_size) { *element_size = ra->header->element_size; }
  return GRN_SUCCESS;
}

grn_rc
grn_ra_close(grn_ctx *ctx, grn_ra *ra)
{
  grn_rc rc;
  if (!ra) { return GRN_INVALID_ARGUMENT; }
  if (ctx->impl->wal.role == GRN_WAL_ROLE_PRIMARY) {
    grn_obj_flush(ctx, (grn_obj *)ra);
  }
  rc = grn_io_close(ctx, ra->io);
  GRN_FREE(ra);
  return rc;
}

grn_rc
grn_ra_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return GRN_INVALID_ARGUMENT; }
  return grn_io_remove(ctx, path);
}

grn_rc
grn_ra_truncate(grn_ctx *ctx, grn_ra *ra)
{
  grn_rc rc;
  const char *io_path;
  char *path;
  unsigned int element_size;
  if ((io_path = grn_io_path(ra->io)) && *io_path != '\0') {
    if (!(path = GRN_STRDUP(io_path))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_path);
      return GRN_NO_MEMORY_AVAILABLE;
    }
  } else {
    path = NULL;
  }
  element_size = ra->header->element_size;
  if (path) {
    grn_wal_clear(ctx, (grn_obj *)ra, "[truncate]");
  }
  if ((rc = grn_io_close(ctx, ra->io))) { goto exit; }
  ra->io = NULL;
  if (path && (rc = grn_io_remove(ctx, path))) { goto exit; }
  if (!_grn_ra_create(ctx, ra, path, element_size)) {
    rc = GRN_UNKNOWN_ERROR;
  }
exit:
  if (path) { GRN_FREE(path); }
  return rc;
}

void *
grn_ra_ref(grn_ctx *ctx, grn_ra *ra, grn_id id)
{
  void *p = NULL;
  uint16_t seg;
  if (id > GRN_ID_MAX) { return NULL; }
  seg = id >> ra->element_width;
  p = grn_io_seg_ref(ctx, ra->io, seg);
  if (!p) { return NULL; }
  return (void *)(((byte *)p) + ((id & ra->element_mask) * ra->header->element_size));
}

grn_rc
grn_ra_unref(grn_ctx *ctx, grn_ra *ra, grn_id id)
{
  uint16_t seg;
  if (id > GRN_ID_MAX) { return GRN_INVALID_ARGUMENT; }
  seg = id >> ra->element_width;
  grn_io_seg_unref(ctx, ra->io, seg);
  return GRN_SUCCESS;
}

void *
grn_ra_ref_cache(grn_ctx *ctx, grn_ra *ra, grn_id id, grn_ra_cache *cache)
{
  void *p = NULL;
  uint16_t seg;
  if (id > GRN_ID_MAX) { return NULL; }
  seg = id >> ra->element_width;
  if (seg == cache->seg) {
    p = cache->p;
  } else {
    if (cache->seg != -1) { grn_io_seg_unref(ctx, ra->io, cache->seg); }
    p = grn_io_seg_ref(ctx, ra->io, seg);
    cache->seg = seg;
    cache->p = p;
  }
  if (!p) { return NULL; }
  return (void *)(((byte *)p) + ((id & ra->element_mask) * ra->header->element_size));
}

grn_rc
grn_ra_cache_fin(grn_ctx *ctx, grn_ra *ra, grn_id id)
{
  uint16_t seg;
  if (id > GRN_ID_MAX) { return GRN_INVALID_ARGUMENT; }
  seg = id >> ra->element_width;
  grn_io_seg_unref(ctx, ra->io, seg);
  return GRN_SUCCESS;
}

static grn_rc
grn_ra_set_value_raw(grn_ctx *ctx,
                     grn_ra *ra,
                     grn_id id,
                     const void *value,
                     size_t size)
{
  void *p = grn_ra_ref(ctx, ra, id);
  if (!p) {
    GRN_DEFINE_NAME(ra);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[column][fix][set-value][%.*s] failed to refer storage",
        name_size, name);
    return ctx->rc;
  }

  uint32_t element_size = ra->header->element_size;
  if (element_size != size) {
    if (size == 0) {
      memset(p, 0, element_size);
    } else {
      void *buffer = GRN_CALLOC(element_size);
      if (buffer) {
        grn_memcpy(buffer, value, size);
        grn_memcpy(p, buffer, element_size);
        GRN_FREE(buffer);
      }
    }
  } else {
    grn_memcpy(p, value, size);
  }
  grn_ra_unref(ctx, ra, id);

  return ctx->rc;
}

#define INCRDECR(range, op)\
  switch (range) {\
  case GRN_DB_INT8 :\
    if (s == sizeof(int8_t)) {\
      int8_t *vp = (int8_t *)p;\
      *vp op *(int8_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_UINT8 :\
    if (s == sizeof(uint8_t)) {\
      uint8_t *vp = (uint8_t *)p;\
      *vp op *(int8_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_INT16 :\
    if (s == sizeof(int16_t)) {\
      int16_t *vp = (int16_t *)p;\
      *vp op *(int16_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_UINT16 :\
    if (s == sizeof(uint16_t)) {\
      uint16_t *vp = (uint16_t *)p;\
      *vp op *(int16_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_INT32 :\
    if (s == sizeof(int32_t)) {\
      int32_t *vp = (int32_t *)p;\
      *vp op *(int32_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_UINT32 :\
    if (s == sizeof(uint32_t)) {\
      uint32_t *vp = (uint32_t *)p;\
      *vp op *(int32_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_INT64 :\
  case GRN_DB_TIME :\
    if (s == sizeof(int64_t)) {\
      int64_t *vp = (int64_t *)p;\
      *vp op *(int64_t *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_FLOAT32 :\
    if (s == sizeof(float)) {\
      float *vp = (float *)p;\
      *vp op *(float *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  case GRN_DB_FLOAT :\
    if (s == sizeof(double)) {\
      double *vp = (double *)p;\
      *vp op *(double *)v;\
      rc = GRN_SUCCESS;\
    } else {\
      rc = GRN_INVALID_ARGUMENT;\
    }\
    break;\
  default :\
    rc = GRN_OPERATION_NOT_SUPPORTED;\
    break;\
  }

static grn_rc
grn_ra_set_value_incrdecr_raw(grn_ctx *ctx,
                              grn_ra *ra,
                              grn_id id,
                              const void *v,
                              size_t s,
                              int flags)
{
  void *p = grn_ra_ref(ctx, ra, id);
  if (!p) {
    GRN_DEFINE_NAME(ra);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[column][fix][set-value][%.*s] failed to refer storage",
        name_size, name);
    return ctx->rc;
  }

  grn_rc rc = GRN_SUCCESS;
  switch (flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_INCR :
    INCRDECR(DB_OBJ(ra)->range, +=);
    break;
  case GRN_OBJ_DECR :
    /* WAL isn't supported */
    INCRDECR(DB_OBJ(ra)->range, -=);
    break;
  default :
    rc = GRN_OPERATION_NOT_SUPPORTED;
    break;
  }
  return rc;
}

grn_rc
grn_ra_set_value(grn_ctx *ctx,
                 grn_ra *ra,
                 grn_id id,
                 grn_obj *value,
                 int flags)
{
  void *v = GRN_BULK_HEAD(value);
  size_t s = GRN_BULK_VSIZE(value);
  grn_rc rc = GRN_SUCCESS;

  switch (flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_SET :
    {
      uint64_t wal_id;
      rc = grn_wal_add_entry(ctx,
                             (grn_obj *)ra,
                             &wal_id,
                             "[column][fix][set-value]",
                             GRN_WAL_KEY_EVENT,
                             GRN_WAL_VALUE_EVENT,
                             GRN_WAL_EVENT_SET,

                             GRN_WAL_KEY_RECORD_ID,
                             GRN_WAL_VALUE_RECORD_ID,
                             id,

                             GRN_WAL_KEY_VALUE,
                             GRN_WAL_VALUE_BINARY,
                             v,
                             s,

                             GRN_WAL_KEY_END);
      if (rc == GRN_SUCCESS) {
        rc = grn_ra_set_value_raw(ctx, ra, id, v, s);
      }
      if (rc == GRN_SUCCESS) {
        ra->header->wal_id = wal_id;
      }
    }
    break;
  case GRN_OBJ_INCR :
  case GRN_OBJ_DECR :
    /* WAL isn't supported */
    rc = grn_ra_set_value_incrdecr_raw(ctx, ra, id, v, s, flags);
    break;
  default :
    rc = GRN_OPERATION_NOT_SUPPORTED;
    break;
  }

  return rc;
}

grn_rc
grn_ra_warm(grn_ctx *ctx, grn_ra *ra)
{
  return grn_io_warm(ctx, ra->io);
}

/**** jagged arrays, ja ****/

/*
 * Summary:
 *
 *   * A jagged array has multiple (JA_N_DATA_SEGMENTS, 65536) segments.
 *   * A segment can store JA_SEGMENT_SIZE (4MiB) raw data.
 *   * Each element has metadata (element info, einfo).
 *   * Element types:
 *     * TINY: Element size is less than 8B. The value is embedded into
 *       the element info.
 *     * HUGE: Element size + sizeof(grn_id) (8B) is larger than segment size
 *       (JA_SEGMENT_SIZE, 4MiB). The value is stored to multiple continuous
 *       segments as is. Start segment ID and element size are stored in
 *       metadata.
 *     * SEQUENTIAL: Element size is larger than (1 << GRN_JA_W_CHUNK_THRESH_V2)
 *       (65536B) and not HUGE. The value is stored to a segment. Segment ID,
 *       start position in the stored segment and element size are stored in
 *       metadata.
 *       FYI: The next segment ID and position are stored in
 *       ja->header->curr_seg and ja->header->curr_pos.
 *       NOTE: Old DB uses GRN_JA_W_CHUNK_THRESH_V1 not _V2.
 *     * CHUNK: Element size is less than or equals to
 *       (1 << GRN_JA_W_CHUNK_THRESH_V2) (65536B). The value is stored to a
 *       segment. The segment has fixed size chunks. Each fixed size is
 *       multiple of 2. Available fixed sizes are 16B, 32B, ...,
 *       (1 << GRN_JA_W_CHUNK_THRESH_V2). Each element uses a segment that
 *       uses best fit fixed size. For example, 31B size element uses a
 *       segment that uses 32B fixed size not 16B/64B fixed size.
 *       NOTE: Old DB uses GRN_JA_W_CHUNK_THRESH_V1 not _V2.
 *   * Each segment has metadata (segment info).
 *   * Segment types:
 *     * HUGE: The segment that stores element values of HUGE element.
 *       Multiple continuous HUGE segments are used for one HUGE element value.
 *       SEG_HUGE tag is stored in metadata.
 *       See also: HUGE element type.
 *     * SEQUENTIAL: The segment that stores element values of SEQUENTIAL
 *       element. Each element value may be different size.
 *       SEG_SEQ tag is stored in metadata.
 *       See also: SEQUENTIAL element type.
 *     * CHUNK: The segment that stores element values of CHUNK element.
 *       Each segment has fixed size chunks. The size is defined for each
 *       segment. SEG_CHUNK tag and chunk size are stored in metadata.
 *       See also: CHUNK element type.
 *     * EINFO: The segment that stores element metadata (element information,
 *       einfo). Each element metadata is fixed size data. Corresponding
 *       element metadata position in segment can be computed from record ID.
 *       The (record_id & JA_M_EINFO_IN_A_SEGMENT)-th grn_ja_einfo is the
 *       corresponding element metadata of the given record ID.
 *       SEG_EINFO tag and index of ja->header->element_segs are stored in
 *       metadata.
 *       See also: EINFO element type and grn_ja_check_segment_einfo_validate().
 *     * GINFO: The segment that stores garbage chunks in CHUNK segments.
 *       This is for reusing freed chunks effectively. SEQUENTIAL element
 *       isn't space effective for massive replace use case. It causes
 *       fragmentation. CHUNK element isn't space effective for one-time set
 *       use case. Because there may be spaces in chunk. If 15B size
 *       element is stored to 16B fixed size chunk, 1B space is wasted.
 *       But CHUNK element is space effective for massive replace use case.
 *       We can reuse each chunk effectively by GINFO segment.
 *   * TODO: How to update element value?
 */

#define GRN_JA_W_CHUNK_THRESH_V1       7
#define GRN_JA_W_CHUNK_THRESH_V2       16
#define GRN_JA_W_CAPACITY              38
#define GRN_JA_W_SEGMENT               22

#define JA_ELEMENT_SEG_VOID            (0xffffffffU)
#define JA_SEGMENT_SIZE                (1U << GRN_JA_W_SEGMENT)
/* EINFO = ELEMENT_INFO */
#define JA_W_EINFO                     3
#define JA_W_SEGMENTS_MAX              (GRN_JA_W_CAPACITY - GRN_JA_W_SEGMENT)
#define JA_W_EINFO_IN_A_SEGMENT        (GRN_JA_W_SEGMENT - JA_W_EINFO)
#define JA_N_EINFO_IN_A_SEGMENT        (1U << JA_W_EINFO_IN_A_SEGMENT)
#define JA_M_EINFO_IN_A_SEGMENT        (JA_N_EINFO_IN_A_SEGMENT - 1)
#define JA_N_GARBAGES_IN_A_SEGMENT     ((1U << (GRN_JA_W_SEGMENT - 3)) - 2)
#define JA_N_ELEMENT_VARIATIONS_V1     (GRN_JA_W_CHUNK_THRESH_V1 - JA_W_EINFO + 1)
#define JA_N_ELEMENT_VARIATIONS_V2     (GRN_JA_W_CHUNK_THRESH_V2 - JA_W_EINFO + 1)
#define JA_N_DATA_SEGMENTS             (1U << JA_W_SEGMENTS_MAX)
#define JA_N_ELEMENT_SEGMENTS          (1U << (GRN_ID_WIDTH - JA_W_EINFO_IN_A_SEGMENT))

typedef struct _grn_ja_einfo grn_ja_einfo;

struct _grn_ja_einfo {
  union {
    struct {
      uint16_t seg;
      uint16_t pos;
      uint16_t size;
      uint8_t c1;
      uint8_t c2;
    } n;
    struct {
      uint32_t size;
      uint16_t seg;
      uint8_t c1;
      uint8_t c2;
    } h;
    uint8_t c[8];
  } u;
};

#define ETINY (0x80)
#define EHUGE (0x40)
#define ETINY_P(e) ((e)->u.c[7] & ETINY)
#define ETINY_ENC(e,_size) ((e)->u.c[7] = (_size) + ETINY)
#define ETINY_DEC(e,_size) ((_size) = (e)->u.c[7] & ~(ETINY|EHUGE))
#define EHUGE_P(e) ((e)->u.c[7] & EHUGE)
#define EHUGE_ENC(e,_seg,_size) do {\
  (e)->u.h.c1 = 0;\
  (e)->u.h.c2 = EHUGE;\
  (e)->u.h.seg = (_seg);\
  (e)->u.h.size = (_size);\
} while (0)
#define EHUGE_DEC(e,_seg,_size) do {\
  (_seg) = (e)->u.h.seg;\
  (_size) = (e)->u.h.size;\
} while (0)
#define EINFO_ENC(e,_seg,_pos,_size) do {\
  (e)->u.n.c1 = (_pos) >> 16;\
  (e)->u.n.c2 = ((_size) >> 16);\
  (e)->u.n.seg = (_seg);\
  (e)->u.n.pos = (_pos);\
  (e)->u.n.size = (_size);\
} while (0)
#define EINFO_DEC(e,_seg,_pos,_size) do {\
  (_seg) = (e)->u.n.seg;\
  (_pos) = ((e)->u.n.c1 << 16) + (e)->u.n.pos;\
  (_size) = ((e)->u.n.c2 << 16) + (e)->u.n.size;\
} while (0)

typedef struct {
  uint32_t seg;
  uint32_t pos;
} ja_pos;

typedef struct {
  uint32_t head;
  uint32_t tail;
  uint32_t nrecs;
  uint32_t next;
  ja_pos recs[JA_N_GARBAGES_IN_A_SEGMENT];
} grn_ja_ginfo;

struct grn_ja_header_v1 {
  uint32_t flags;
  uint32_t curr_seg;
  uint32_t curr_pos;
  uint32_t max_element_size;
  ja_pos free_elements[JA_N_ELEMENT_VARIATIONS_V1];
  uint32_t garbages[JA_N_ELEMENT_VARIATIONS_V1];
  uint32_t n_garbages[JA_N_ELEMENT_VARIATIONS_V1];
  uint32_t segment_infos[JA_N_DATA_SEGMENTS];
  uint32_t element_segs[JA_N_ELEMENT_SEGMENTS];
};

struct grn_ja_header_v2 {
  uint32_t flags;
  uint32_t curr_seg;
  uint32_t curr_pos;
  uint32_t max_element_size;
  ja_pos free_elements[JA_N_ELEMENT_VARIATIONS_V2];
  uint32_t garbages[JA_N_ELEMENT_VARIATIONS_V2];
  uint32_t n_garbages[JA_N_ELEMENT_VARIATIONS_V2];
  uint32_t segment_infos[JA_N_DATA_SEGMENTS];
  uint32_t element_segs[JA_N_ELEMENT_SEGMENTS];
  uint8_t chunk_threshold;
  uint8_t n_element_variations;
};

struct grn_ja_header {
  uint32_t flags;
  uint32_t *curr_seg;
  uint32_t *curr_pos;
  uint32_t max_element_size;
  ja_pos *free_elements;
  uint32_t *garbages;
  uint32_t *n_garbages;
  uint32_t *segment_infos;
  uint32_t *element_segs;
  uint8_t chunk_threshold;
  uint8_t n_element_variations;
};

#define SEG_CHUNK      (0x00000000U)
#define SEG_SEQ        (0x10000000U)
#define SEG_HUGE       (0x20000000U)
#define SEG_EINFO      (0x30000000U)
#define SEG_GINFO      (0x40000000U)
#define SEG_MASK       (0xf0000000U)
#define SEG_TYPE_SHIFT 28

static uint32_t
grn_ja_segment_info_type(grn_ctx *ctx, uint32_t info)
{
  return info & SEG_MASK;
}

static const char *
grn_ja_segment_info_type_name(grn_ctx *ctx, uint32_t info)
{
  switch (grn_ja_segment_info_type(ctx, info)) {
  case SEG_CHUNK :
    return "chunk";
  case SEG_SEQ :
    return "sequential";
  case SEG_HUGE :
    return "huge";
  case SEG_EINFO :
    return "element info";
  case SEG_GINFO :
    return "garbage info";
  default :
    return "unknown";
  }
}

static uint32_t
grn_ja_segment_info_value(grn_ctx *ctx, uint32_t info)
{
  return info & ~SEG_MASK;
}

#define SEGMENT_INFO_AT(ja,seg) ((ja)->header->segment_infos[seg])
#define SEGMENT_CHUNK_ON(ja,seg,width) (SEGMENT_INFO_AT(ja,seg) = SEG_CHUNK|width)
#define SEGMENT_SEQ_ON(ja,seg) (SEGMENT_INFO_AT(ja,seg) = SEG_SEQ)
#define SEGMENT_HUGE_ON(ja,seg) (SEGMENT_INFO_AT(ja,seg) = SEG_HUGE)
#define SEGMENT_EINFO_ON(ja,seg,lseg) (SEGMENT_INFO_AT(ja,seg) = SEG_EINFO|(lseg))
#define SEGMENT_GINFO_ON(ja,seg,width) (SEGMENT_INFO_AT(ja,seg) = SEG_GINFO|(width))
#define SEGMENT_OFF(ja,seg) (SEGMENT_INFO_AT(ja,seg) = 0)

static grn_ja *
_grn_ja_create(grn_ctx *ctx, grn_ja *ja, const char *path,
               unsigned int max_element_size, uint32_t flags)
{
  int i;
  grn_io *io;
  struct grn_ja_header *header;
  struct grn_ja_header_v2 *header_v2;
  io = grn_io_create(ctx, path, sizeof(struct grn_ja_header_v2),
                     JA_SEGMENT_SIZE, JA_N_DATA_SEGMENTS, GRN_IO_AUTO,
                     GRN_IO_EXPIRE_SEGMENT);
  if (!io) { return NULL; }
  grn_io_set_type(io, GRN_COLUMN_VAR_SIZE);

  header_v2 = grn_io_header(io);
  header_v2->flags = flags;
  header_v2->curr_seg = 0;
  header_v2->curr_pos = JA_SEGMENT_SIZE;
  header_v2->max_element_size = max_element_size;
  for (i = 0; i < JA_N_ELEMENT_SEGMENTS; i++) {
    header_v2->element_segs[i] = JA_ELEMENT_SEG_VOID;
  }
  header_v2->chunk_threshold = GRN_JA_W_CHUNK_THRESH_V2;
  header_v2->n_element_variations = JA_N_ELEMENT_VARIATIONS_V2;

  header = GRN_MALLOCN(struct grn_ja_header, 1);
  if (!header) {
    grn_io_close(ctx, io);
    return NULL;
  }
  header->flags                = header_v2->flags;
  header->curr_seg             = &(header_v2->curr_seg);
  header->curr_pos             = &(header_v2->curr_pos);
  header->max_element_size     = header_v2->max_element_size;
  header->free_elements        = header_v2->free_elements;
  header->garbages             = header_v2->garbages;
  header->n_garbages           = header_v2->n_garbages;
  header->segment_infos        = header_v2->segment_infos;
  header->element_segs         = header_v2->element_segs;
  header->chunk_threshold      = header_v2->chunk_threshold;
  header->n_element_variations = header_v2->n_element_variations;

  ja->io = io;
  ja->header = header;
  SEGMENT_EINFO_ON(ja, 0, 0);
  header->element_segs[0] = 0;
  return ja;
}

grn_ja *
grn_ja_create(grn_ctx *ctx, const char *path, unsigned int max_element_size, uint32_t flags)
{
  grn_ja *ja = NULL;
  ja = (grn_ja *)GRN_CALLOC(sizeof(grn_ja));
  if (!ja) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ja, GRN_COLUMN_VAR_SIZE);
  if (!_grn_ja_create(ctx, ja, path, max_element_size, flags)) {
    GRN_FREE(ja);
    return NULL;
  }
  return ja;
}

grn_ja *
grn_ja_open(grn_ctx *ctx, const char *path)
{
  grn_io *io;
  grn_ja *ja = NULL;
  struct grn_ja_header *header;
  struct grn_ja_header_v2 *header_v2;
  uint32_t io_type;
  io = grn_io_open(ctx, path, GRN_IO_AUTO);
  if (!io) { return NULL; }
  header_v2 = grn_io_header(io);
  io_type = grn_io_get_type(io);
  if (io_type != GRN_COLUMN_VAR_SIZE) {
    ERR(GRN_INVALID_FORMAT,
        "[column][var-size] file type must be %#04x: <%#04x>",
        GRN_COLUMN_VAR_SIZE, io_type);
    grn_io_close(ctx, io);
    return NULL;
  }
  if (header_v2->chunk_threshold == 0) {
    header_v2->chunk_threshold = GRN_JA_W_CHUNK_THRESH_V1;
  }
  if (header_v2->n_element_variations == 0) {
    header_v2->n_element_variations = JA_N_ELEMENT_VARIATIONS_V1;
  }
  ja = GRN_MALLOCN(grn_ja, 1);
  if (!ja) {
    grn_io_close(ctx, io);
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ja, GRN_COLUMN_VAR_SIZE);
  header = GRN_MALLOCN(struct grn_ja_header, 1);
  if (!header) {
    grn_io_close(ctx, io);
    GRN_FREE(ja);
    return NULL;
  }

  header->flags                = header_v2->flags;
  header->curr_seg             = &(header_v2->curr_seg);
  header->curr_pos             = &(header_v2->curr_pos);
  header->max_element_size     = header_v2->max_element_size;
  header->chunk_threshold      = header_v2->chunk_threshold;
  header->n_element_variations = header_v2->n_element_variations;
  if (header->chunk_threshold == GRN_JA_W_CHUNK_THRESH_V1) {
    struct grn_ja_header_v1 *header_v1 = (struct grn_ja_header_v1 *)header_v2;
    header->free_elements = header_v1->free_elements;
    header->garbages      = header_v1->garbages;
    header->n_garbages    = header_v1->n_garbages;
    header->segment_infos = header_v1->segment_infos;
    header->element_segs  = header_v1->element_segs;
  } else {
    header->free_elements = header_v2->free_elements;
    header->garbages      = header_v2->garbages;
    header->n_garbages    = header_v2->n_garbages;
    header->segment_infos = header_v2->segment_infos;
    header->element_segs  = header_v2->element_segs;
  }

  ja->io = io;
  ja->header = header;

  return ja;
}

grn_rc
grn_ja_info(grn_ctx *ctx, grn_ja *ja, unsigned int *max_element_size)
{
  if (!ja) { return GRN_INVALID_ARGUMENT; }
  if (max_element_size) { *max_element_size = ja->header->max_element_size; }
  return GRN_SUCCESS;
}

grn_column_flags
grn_ja_get_flags(grn_ctx *ctx, grn_ja *ja)
{
  if (!ja) {
    return 0;
  }

  return ja->header->flags;
}

void
grn_ja_set_visibility(grn_ctx *ctx, grn_ja *ja, bool is_visible)
{
  if (is_visible) {
    ja->header->flags &= ~GRN_OBJ_INVISIBLE;
  } else {
    ja->header->flags |= GRN_OBJ_INVISIBLE;
  }
}

grn_rc
grn_ja_close(grn_ctx *ctx, grn_ja *ja)
{
  grn_rc rc;
  if (!ja) { return GRN_INVALID_ARGUMENT; }
  rc = grn_io_close(ctx, ja->io);
  GRN_FREE(ja->header);
  GRN_FREE(ja);
  return rc;
}

grn_rc
grn_ja_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return GRN_INVALID_ARGUMENT; }
  return grn_io_remove(ctx, path);
}

grn_rc
grn_ja_truncate(grn_ctx *ctx, grn_ja *ja)
{
  grn_rc rc;
  const char *io_path;
  char *path;
  unsigned int max_element_size;
  uint32_t flags;
  if ((io_path = grn_io_path(ja->io)) && *io_path != '\0') {
    if (!(path = GRN_STRDUP(io_path))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_path);
      return GRN_NO_MEMORY_AVAILABLE;
    }
  } else {
    path = NULL;
  }
  max_element_size = ja->header->max_element_size;
  flags = ja->header->flags;
  if ((rc = grn_io_close(ctx, ja->io))) { goto exit; }
  ja->io = NULL;
  if (path && (rc = grn_io_remove(ctx, path))) { goto exit; }
  GRN_FREE(ja->header);
  if (!_grn_ja_create(ctx, ja, path, max_element_size, flags)) {
    rc = GRN_UNKNOWN_ERROR;
  }
exit:
  if (path) { GRN_FREE(path); }
  return rc;
}

static void *
grn_ja_ref_raw(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
  uint32_t pseg = ja->header->element_segs[id >> JA_W_EINFO_IN_A_SEGMENT];
  iw->size = 0;
  iw->addr = NULL;
  iw->pseg = pseg;
  iw->uncompressed_value = NULL;
  if (pseg != JA_ELEMENT_SEG_VOID) {
    grn_ja_einfo *einfo = NULL;
    einfo = grn_io_seg_ref(ctx, ja->io, pseg);
    if (einfo) {
      grn_ja_einfo *ei = &einfo[id & JA_M_EINFO_IN_A_SEGMENT];
      if (ETINY_P(ei)) {
        iw->tiny_p = 1;
        ETINY_DEC(ei, iw->size);
        iw->io = ja->io;
        iw->ctx = ctx;
        iw->addr = (void *)ei;
      } else {
        uint32_t jag, vpos, vsize;
        iw->tiny_p = 0;
        if (EHUGE_P(ei)) {
          EHUGE_DEC(ei, jag, vsize);
          vpos = 0;
        } else {
          EINFO_DEC(ei, jag, vpos, vsize);
        }
        grn_io_win_map(ctx, ja->io, iw, jag, vpos, vsize, GRN_IO_RDONLY);
      }
      if (!iw->addr) { grn_io_seg_unref(ctx, ja->io, pseg); }
    }
  }
  *value_len = iw->size;
  return iw->addr;
}

grn_rc
grn_ja_unref(grn_ctx *ctx, grn_io_win *iw)
{
  if (iw->uncompressed_value) {
    GRN_FREE(iw->uncompressed_value);
    iw->uncompressed_value = NULL;
  }
  if (!iw->addr) { return GRN_INVALID_ARGUMENT; }
  grn_io_seg_unref(ctx, iw->io, iw->pseg);
  if (!iw->tiny_p) { grn_io_win_unmap(ctx, iw); }
  return GRN_SUCCESS;
}

#define DELETED 0x80000000

static grn_rc
grn_ja_free(grn_ctx *ctx, grn_ja *ja, grn_ja_einfo *einfo)
{
  const char *tag = "[ja][free]";
  grn_ja_ginfo *ginfo = NULL;
  uint32_t seg, pos, element_size, aligned_size, m, *gseg;
  if (ETINY_P(einfo)) { return GRN_SUCCESS; }
  if (EHUGE_P(einfo)) {
    uint32_t n;
    EHUGE_DEC(einfo, seg, element_size);
    n = ((element_size + JA_SEGMENT_SIZE - 1) >> GRN_JA_W_SEGMENT);
    for (; n--; seg++) { SEGMENT_OFF(ja, seg); }
    return GRN_SUCCESS;
  }
  EINFO_DEC(einfo, seg, pos, element_size);
  if (!element_size) { return GRN_SUCCESS; }
  {
    int es = element_size - 1;
    GRN_BIT_SCAN_REV(es, m);
    m++;
  }
  if (m > ja->header->chunk_threshold) {
    byte *addr = NULL;
    addr = grn_io_seg_ref(ctx, ja->io, seg);
    if (!addr) { return GRN_NO_MEMORY_AVAILABLE; }
    aligned_size = (element_size + sizeof(grn_id) - 1) & ~(sizeof(grn_id) - 1);
    *(uint32_t *)(addr + pos - sizeof(grn_id)) = DELETED|aligned_size;
    if (SEGMENT_INFO_AT(ja, seg) < (aligned_size + sizeof(grn_id)) + SEG_SEQ) {
      GRN_DEFINE_NAME(ja);
      GRN_LOG(ctx,
              GRN_WARN,
              "%s[%.*s] inconsistent ja entry detected (%d > %d): path:<%s>",
              tag,
              name_size, name,
              element_size,
              SEGMENT_INFO_AT(ja, seg) - SEG_SEQ,
              ja->io->path);
    }
    SEGMENT_INFO_AT(ja, seg) -= (aligned_size + sizeof(grn_id));
    if (SEGMENT_INFO_AT(ja, seg) == SEG_SEQ) {
      /* reuse the segment */
      SEGMENT_OFF(ja, seg);
      if (seg == *(ja->header->curr_seg)) {
        *(ja->header->curr_pos) = JA_SEGMENT_SIZE;
      }
    }
    grn_io_seg_unref(ctx, ja->io, seg);
  } else {
    /* The segment that is opened as ginfo. */
    uint32_t target_seg = 0;
    gseg = &ja->header->garbages[m - JA_W_EINFO];
    const uint32_t initial_gseg = *gseg;
    while (*gseg != 0) {
      if (target_seg != 0) { grn_io_seg_unref(ctx, ja->io, target_seg); }
      ginfo = grn_io_seg_ref(ctx, ja->io, *gseg);
      if (!ginfo) {
        GRN_DEFINE_NAME(ja);
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "%s[%.*s] failed to refer garbage segment: "
            "segment:%u, "
            "element_size:%u, "
            "variation:%u, "
            "initial_garbage:%u, "
            "n_garbages:%u, "
            "path:<%s>",
            tag,
            name_size, name,
            *gseg,
            element_size,
            m - JA_W_EINFO,
            initial_gseg,
            ja->header->n_garbages[m - JA_W_EINFO],
            ja->io->path);
        return ctx->rc;
      }
      target_seg = *gseg;
      if (ginfo->nrecs < JA_N_GARBAGES_IN_A_SEGMENT) { break; }
      gseg = &ginfo->next;
    }
    if (*gseg == 0) {
      uint32_t i = 0;
      while (SEGMENT_INFO_AT(ja, i)) {
        if (++i >= JA_N_DATA_SEGMENTS) {
          if (target_seg != 0) { grn_io_seg_unref(ctx, ja->io, target_seg); }
          {
            GRN_DEFINE_NAME(ja);
            ERR(GRN_NOT_ENOUGH_SPACE,
                "%s[%.*s] can't find free segment for garbage segment: "
                "element_size:%u, "
                "variation:%u, "
                "initial_garbage:%u, "
                "n_garbages:%u, "
                "path:<%s>",
                tag,
                name_size, name,
                element_size,
                m - JA_W_EINFO,
                initial_gseg,
                ja->header->n_garbages[m - JA_W_EINFO],
                ja->io->path);
            return ctx->rc;
          }
        }
      }
      SEGMENT_GINFO_ON(ja, i, m - JA_W_EINFO);
      *gseg = i;
      if (target_seg != 0) { grn_io_seg_unref(ctx, ja->io, target_seg); }
      ginfo = grn_io_seg_ref(ctx, ja->io, i);
      if (!ginfo) {
        GRN_DEFINE_NAME(ja);
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "%s[%.*s] failed to refer newly allocated garbage segment: "
            "segment:%u, "
            "element_size:%u, "
            "variation:%u, "
            "initial_garbage:%u, "
            "n_garbages:%u, "
            "path:<%s>",
            tag,
            name_size, name,
            i,
            element_size,
            m - JA_W_EINFO,
            initial_gseg,
            ja->header->n_garbages[m - JA_W_EINFO],
            ja->io->path);
        return ctx->rc;
      }
      target_seg = i;
      ginfo->head = 0;
      ginfo->tail = 0;
      ginfo->nrecs = 0;
      ginfo->next = 0;
    }
    ginfo->recs[ginfo->head].seg = seg;
    ginfo->recs[ginfo->head].pos = pos;
    if (++ginfo->head == JA_N_GARBAGES_IN_A_SEGMENT) { ginfo->head = 0; }
    ginfo->nrecs++;
    ja->header->n_garbages[m - JA_W_EINFO]++;
    grn_io_seg_unref(ctx, ja->io, target_seg);
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ja_replace(grn_ctx *ctx, grn_ja *ja, grn_id id,
               grn_ja_einfo *ei, uint64_t *cas)
{
  const char *tag = "[ja][replace]";
  uint32_t lseg, *pseg, pos;
  grn_ja_einfo *einfo = NULL, eback;
  lseg = id >> JA_W_EINFO_IN_A_SEGMENT;
  pos = id & JA_M_EINFO_IN_A_SEGMENT;
  pseg = &ja->header->element_segs[lseg];
  if (grn_io_lock(ctx, ja->io, grn_lock_timeout)) {
    return ctx->rc;
  }
  uint32_t target_seg;
  if (*pseg == JA_ELEMENT_SEG_VOID) {
    int i = 0;
    while (SEGMENT_INFO_AT(ja, i)) {
      if (++i >= JA_N_DATA_SEGMENTS) {
        GRN_DEFINE_NAME(ja);
        ERR(GRN_NOT_ENOUGH_SPACE,
            "%s[%.*s][%u] can't find free segment: <%s>",
            tag,
            name_size, name,
            id,
            ja->io->path);
        goto exit;
      }
    }
    target_seg = i;
    einfo = grn_io_seg_ref(ctx, ja->io, target_seg);
    if (einfo) {
      *pseg = target_seg;
      SEGMENT_EINFO_ON(ja, target_seg, lseg);
      memset(einfo, 0, JA_SEGMENT_SIZE);
    }
  } else {
    target_seg = *pseg;
    einfo = grn_io_seg_ref(ctx, ja->io, target_seg);
  }
  if (!einfo) {
    GRN_DEFINE_NAME(ja);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "%s[%.*s][%u] failed to refer element info segment: "
        "segment:%u, "
        "path:<%s>",
        tag,
        name_size, name,
        id,
        target_seg,
        ja->io->path);
    goto exit;
  }
  eback = einfo[pos];
  if (cas && *cas != *((uint64_t *)&eback)) {
    GRN_DEFINE_NAME(ja);
    ERR(GRN_CAS_ERROR,
        "%s[%.*s][%u] failed to CAS: "
        "%" GRN_FMT_INT64U " != %" GRN_FMT_INT64U
        ": <%s>",
        tag,
        name_size, name,
        id,
        *((uint64_t *)&eback),
        *cas,
        ja->io->path);
    grn_io_seg_unref(ctx, ja->io, *pseg);
    goto exit;
  }
  // smb_wmb();
  {
    uint64_t *location = (uint64_t *)(einfo + pos);
    uint64_t value = *((uint64_t *)ei);
    GRN_SET_64BIT(location, value);
  }
  grn_io_seg_unref(ctx, ja->io, *pseg);
  grn_ja_free(ctx, ja, &eback);
exit :
  grn_io_unlock(ja->io);
  return ctx->rc;
}

#define JA_N_GARBAGES_TH 10

// todo : grn_io_win_map cause verbose copy when nseg > 1, it should be copied directly.
static grn_rc
grn_ja_alloc(grn_ctx *ctx, grn_ja *ja, grn_id id,
             uint32_t element_size, grn_ja_einfo *einfo, grn_io_win *iw)
{
  const char *tag = "[ja][alloc]";
  byte *addr = NULL;
  iw->io = ja->io;
  iw->ctx = ctx;
  iw->cached = 1;
  if (element_size < 8) {
    ETINY_ENC(einfo, element_size);
    iw->tiny_p = 1;
    iw->addr = (void *)einfo;
    return GRN_SUCCESS;
  }
  iw->tiny_p = 0;
  if (grn_io_lock(ctx, ja->io, grn_lock_timeout)) { return ctx->rc; }
  if (element_size + sizeof(grn_id) > JA_SEGMENT_SIZE) {
    int i, j, n = (element_size + JA_SEGMENT_SIZE - 1) >> GRN_JA_W_SEGMENT;
    for (i = 0, j = -1; i < JA_N_DATA_SEGMENTS; i++) {
      if (SEGMENT_INFO_AT(ja, i)) {
        j = i;
      } else {
        if (i == j + n) {
          j++;
          addr = grn_io_win_map(ctx, ja->io, iw, j, 0, element_size, GRN_IO_WRONLY);
          if (!addr) {
            GRN_DEFINE_NAME(ja);
            ERR(GRN_NO_MEMORY_AVAILABLE,
                "%s[%.*s][%u] failed to map new window for huge element: "
                "n_segments:%u, "
                "start_segment:%u, "
                "element_size:%u, "
                "path:<%s>",
                tag,
                name_size, name,
                id,
                n,
                j,
                element_size,
                ja->io->path);
            grn_io_unlock(ja->io);
            return ctx->rc;
          }
          EHUGE_ENC(einfo, j, element_size);
          for (; j <= i; j++) { SEGMENT_HUGE_ON(ja, j); }
          grn_io_unlock(ja->io);
          return GRN_SUCCESS;
        }
      }
    }
    {
      GRN_DEFINE_NAME(ja);
      ERR(GRN_NOT_ENOUGH_SPACE,
          "%s[%.*s][%u] "
          "failed to allocate huge segment because of full: "
          "element_size:%u, "
          "path:<%s>",
          tag,
          name_size, name,
          id,
          element_size,
          ja->io->path);
    }
    grn_io_unlock(ja->io);
    return ctx->rc;
  } else {
    ja_pos *vp;
    int m, aligned_size, es = element_size - 1;
    GRN_BIT_SCAN_REV(es, m);
    m++;
    if (m > ja->header->chunk_threshold) {
      uint32_t seg = *(ja->header->curr_seg);
      uint32_t pos = *(ja->header->curr_pos);
      if (pos + element_size + sizeof(grn_id) > JA_SEGMENT_SIZE) {
        seg = 0;
        while (SEGMENT_INFO_AT(ja, seg)) {
          if (++seg >= JA_N_DATA_SEGMENTS) {
            GRN_DEFINE_NAME(ja);
            ERR(GRN_NOT_ENOUGH_SPACE,
                "%s[%*.s][%u] "
                "failed to allocate sequential segment because of full: "
                "element_size:%u, "
                "path:<%s>",
                tag,
                name_size, name,
                id,
                element_size,
                ja->io->path);
            grn_io_unlock(ja->io);
            return ctx->rc;
          }
        }
        SEGMENT_SEQ_ON(ja, seg);
        *(ja->header->curr_seg) = seg;
        pos = 0;
      }
      addr = grn_io_seg_ref(ctx, ja->io, seg);
      if (!addr) {
        GRN_DEFINE_NAME(ja);
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "%s[%*.s][%u] failed to refer sequential segment: "
            "segment:%u, "
            "element_size:%u, "
            "path:<%s>",
            tag,
            name_size, name,
            id,
            seg,
            element_size,
            ja->io->path);
        grn_io_unlock(ja->io);
        return ctx->rc;
      }
      *(grn_id *)(addr + pos) = id;
      aligned_size = (element_size + sizeof(grn_id) - 1) & ~(sizeof(grn_id) - 1);
      if (pos + aligned_size < JA_SEGMENT_SIZE) {
        *(grn_id *)(addr + pos + aligned_size) = GRN_ID_NIL;
      }
      SEGMENT_INFO_AT(ja, seg) += aligned_size + sizeof(grn_id);
      pos += sizeof(grn_id);
      EINFO_ENC(einfo, seg, pos, element_size);
      iw->segment = seg;
      iw->addr = addr + pos;
      *(ja->header->curr_pos) = pos + aligned_size;
      grn_io_unlock(ja->io);
      return GRN_SUCCESS;
    } else {
      uint32_t lseg = 0, lseg_;
      aligned_size = 1 << m;
      if (ja->header->n_garbages[m - JA_W_EINFO] > JA_N_GARBAGES_TH) {
        grn_ja_ginfo *ginfo = NULL;
        uint32_t seg, pos, *gseg;
        gseg = &ja->header->garbages[m - JA_W_EINFO];
        while ((lseg_ = *gseg)) {
          ginfo = grn_io_seg_ref(ctx, ja->io, lseg_);
          if (!ginfo) {
            if (lseg) { grn_io_seg_unref(ctx, ja->io, lseg); }
            {
              GRN_DEFINE_NAME(ja);
              ERR(GRN_NO_MEMORY_AVAILABLE,
                  "%s[%.*s][%u] failed to refer garbage segment: "
                  "segment:%u, "
                  "element_size:%u, "
                  "path:<%s>",
                  tag,
                  name_size, name,
                  id,
                  lseg_,
                  element_size,
                  ja->io->path);
            }
            grn_io_unlock(ja->io);
            return ctx->rc;
          }
          if (ginfo->next || ginfo->nrecs > JA_N_GARBAGES_TH) {
            seg = ginfo->recs[ginfo->tail].seg;
            pos = ginfo->recs[ginfo->tail].pos;
            addr = grn_io_seg_ref(ctx, ja->io, seg);
            if (!addr) {
              {
                GRN_DEFINE_NAME(ja);
                ERR(GRN_NO_MEMORY_AVAILABLE,
                    "%s[%.*s][%u] "
                    "failed to refer content segment from garbage segment: "
                    "segment:%u, "
                    "position:%u, "
                    "element_size:%u, "
                    "path:<%s>",
                    tag,
                    name_size, name,
                    id,
                    seg,
                    pos,
                    element_size,
                    ja->io->path);
              }
              if (lseg) { grn_io_seg_unref(ctx, ja->io, lseg); }
              grn_io_seg_unref(ctx, ja->io, lseg_);
              grn_io_unlock(ja->io);
              return ctx->rc;
            }
            EINFO_ENC(einfo, seg, pos, element_size);
            iw->segment = seg;
            iw->addr = addr + pos;
            if (++ginfo->tail == JA_N_GARBAGES_IN_A_SEGMENT) { ginfo->tail = 0; }
            ginfo->nrecs--;
            ja->header->n_garbages[m - JA_W_EINFO]--;
            if (!ginfo->nrecs) {
              if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
                GRN_DEFINE_NAME(ja);
                GRN_LOG(ctx,
                        GRN_LOG_DEBUG,
                        "%s[%.*s][%u] "
                        "free a garbage info segment that "
                        "has no more garbage info: "
                        "segment:%u, "
                        "next_segment:%u, "
                        "element_size:%u, "
                        "variation:%u,  "
                        "path:<%s>",
                        tag,
                        name_size, name,
                        id,
                        *gseg,
                        ginfo->next,
                        element_size,
                        m - JA_W_EINFO,
                        ja->io->path);
              }
              SEGMENT_OFF(ja, *gseg);
              *gseg = ginfo->next;
            }
            if (lseg) { grn_io_seg_unref(ctx, ja->io, lseg); }
            grn_io_seg_unref(ctx, ja->io, lseg_);
            grn_io_unlock(ja->io);
            return GRN_SUCCESS;
          }
          if (lseg) { grn_io_seg_unref(ctx, ja->io, lseg); }
          if (!ginfo->next) {
            grn_io_seg_unref(ctx, ja->io, lseg_);
            break;
          }
          lseg = lseg_;
          gseg = &ginfo->next;
        }
      }
      vp = &ja->header->free_elements[m - JA_W_EINFO];
      if (!vp->seg) {
        int i = 0;
        while (SEGMENT_INFO_AT(ja, i)) {
          if (++i >= JA_N_DATA_SEGMENTS) {
            GRN_DEFINE_NAME(ja);
            ERR(GRN_NO_MEMORY_AVAILABLE,
                "%s[%.*s][%u] "
                "failed to allocate reference segment because of full: "
                "element_size:%u, "
                "variation:%u, "
                "path:<%s>",
                tag,
                name_size, name,
                id,
                element_size,
                m - JA_W_EINFO,
                ja->io->path);
            grn_io_unlock(ja->io);
            return ctx->rc;
          }
        }
        SEGMENT_CHUNK_ON(ja, i, m);
        vp->seg = i;
        vp->pos = 0;
      }
      EINFO_ENC(einfo, vp->seg, vp->pos, element_size);
      addr = grn_io_seg_ref(ctx, ja->io, vp->seg);
      if (!addr) {
        GRN_DEFINE_NAME(ja);
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "%s[%*.s][%u] failed to refer content segment in free elements: "
            "segment:%u, "
            "position:%u, "
            "element_size:%u, "
            "path:<%s>",
            tag,
            name_size, name,
            id,
            vp->seg,
            vp->pos,
            element_size,
            ja->io->path);
        grn_io_unlock(ja->io);
        return ctx->rc;
      }
      iw->segment = vp->seg;
      iw->addr = addr + vp->pos;
      if ((vp->pos += aligned_size) == JA_SEGMENT_SIZE) {
        vp->seg = 0;
        vp->pos = 0;
      }
      iw->uncompressed_value = NULL;
      grn_io_unlock(ja->io);
      return GRN_SUCCESS;
    }
  }
}

static grn_rc
set_value(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, uint32_t value_len,
          grn_ja_einfo *einfo)
{
  grn_rc rc = GRN_SUCCESS;
  grn_io_win iw;
  if ((ja->header->flags & GRN_OBJ_RING_BUFFER) &&
      value_len >= ja->header->max_element_size) {
    if ((rc = grn_ja_alloc(ctx, ja, id, value_len + sizeof(uint32_t), einfo, &iw))) {
      return rc;
    }
    grn_memcpy(iw.addr, value, value_len);
    memset((byte *)iw.addr + value_len, 0, sizeof(uint32_t));
    grn_io_win_unmap(ctx, &iw);
  } else {
    if ((rc = grn_ja_alloc(ctx, ja, id, value_len, einfo, &iw))) { return rc; }
    grn_memcpy(iw.addr, value, value_len);
    grn_io_win_unmap(ctx, &iw);
  }
  return rc;
}

static grn_rc
grn_ja_put_raw(grn_ctx *ctx, grn_ja *ja, grn_id id,
               void *value, uint32_t value_len, int flags, uint64_t *cas)
{
  int rc;
  int64_t buf;
  grn_io_win iw;
  grn_ja_einfo einfo;

  if ((flags & GRN_OBJ_SET_MASK) == GRN_OBJ_SET &&
      value_len > 0) {
    grn_io_win jw;
    uint32_t old_len;
    void *old_value;
    grn_bool same_value = GRN_FALSE;

    old_value = grn_ja_ref(ctx, ja, id, &jw, &old_len);
    if (value_len == old_len && memcmp(value, old_value, value_len) == 0) {
      same_value = GRN_TRUE;
    }
    grn_ja_unref(ctx, &jw);
    if (same_value) {
      return GRN_SUCCESS;
    }
  }

  switch (flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_APPEND :
    if (value_len) {
      grn_io_win jw;
      uint32_t old_len;
      void *oldvalue = grn_ja_ref(ctx, ja, id, &jw, &old_len);
      if (oldvalue) {
        if ((ja->header->flags & GRN_OBJ_RING_BUFFER) &&
            old_len + value_len >= ja->header->max_element_size) {
          if (old_len >= ja->header->max_element_size) {
            byte *b = oldvalue;
            uint32_t el = old_len - sizeof(uint32_t);
            uint32_t pos = *((uint32_t *)(b + el));
            GRN_ASSERT(pos < el);
            if (el <= pos + value_len) {
              uint32_t rest = el - pos;
              grn_memcpy(b + pos, value, rest);
              grn_memcpy(b, (byte *)value + rest, value_len - rest);
              *((uint32_t *)(b + el)) = value_len - rest;
            } else {
              grn_memcpy(b + pos, value, value_len);
              *((uint32_t *)(b + el)) = pos + value_len;
            }
            return GRN_SUCCESS;
          } else {
            if ((rc = grn_ja_alloc(ctx, ja, id,
                                   value_len + old_len + sizeof(uint32_t),
                                   &einfo, &iw))) {
              grn_ja_unref(ctx, &jw);
              return rc;
            }
            grn_memcpy(iw.addr, oldvalue, old_len);
            grn_memcpy((byte *)iw.addr + old_len, value, value_len);
            memset((byte *)iw.addr + old_len + value_len, 0, sizeof(uint32_t));
            grn_io_win_unmap(ctx, &iw);
          }
        } else {
          if ((rc = grn_ja_alloc(ctx, ja, id, value_len + old_len, &einfo, &iw))) {
            grn_ja_unref(ctx, &jw);
            return rc;
          }
          grn_memcpy(iw.addr, oldvalue, old_len);
          grn_memcpy((byte *)iw.addr + old_len, value, value_len);
          grn_io_win_unmap(ctx, &iw);
        }
        grn_ja_unref(ctx, &jw);
      } else {
        set_value(ctx, ja, id, value, value_len, &einfo);
      }
    }
    break;
  case GRN_OBJ_PREPEND :
    if (value_len) {
      grn_io_win jw;
      uint32_t old_len;
      void *oldvalue = grn_ja_ref(ctx, ja, id, &jw, &old_len);
      if (oldvalue) {
        if ((ja->header->flags & GRN_OBJ_RING_BUFFER) &&
            old_len + value_len >= ja->header->max_element_size) {
          if (old_len >= ja->header->max_element_size) {
            byte *b = oldvalue;
            uint32_t el = old_len - sizeof(uint32_t);
            uint32_t pos = *((uint32_t *)(b + el));
            GRN_ASSERT(pos < el);
            if (pos < value_len) {
              uint32_t rest = value_len - pos;
              grn_memcpy(b, (byte *)value + rest, pos);
              grn_memcpy(b + el - rest, value, rest);
              *((uint32_t *)(b + el)) = el - rest;
            } else {
              grn_memcpy(b + pos - value_len, value, value_len);
              *((uint32_t *)(b + el)) = pos - value_len;
            }
            return GRN_SUCCESS;
          } else {
            if ((rc = grn_ja_alloc(ctx, ja, id,
                                   value_len + old_len + sizeof(uint32_t),
                                   &einfo, &iw))) {
              grn_ja_unref(ctx, &jw);
              return rc;
            }
            grn_memcpy(iw.addr, value, value_len);
            grn_memcpy((byte *)iw.addr + value_len, oldvalue, old_len);
            memset((byte *)iw.addr + value_len + old_len, 0, sizeof(uint32_t));
            grn_io_win_unmap(ctx, &iw);
          }
        } else {
          if ((rc = grn_ja_alloc(ctx, ja, id, value_len + old_len, &einfo, &iw))) {
            grn_ja_unref(ctx, &jw);
            return rc;
          }
          grn_memcpy(iw.addr, value, value_len);
          grn_memcpy((byte *)iw.addr + value_len, oldvalue, old_len);
          grn_io_win_unmap(ctx, &iw);
        }
        grn_ja_unref(ctx, &jw);
      } else {
        set_value(ctx, ja, id, value, value_len, &einfo);
      }
    }
    break;
  case GRN_OBJ_DECR :
    if (value_len == sizeof(int64_t)) {
      int64_t *v = (int64_t *)&buf;
      *v = -(*(int64_t *)value);
      value = v;
    } else if (value_len == sizeof(int32_t)) {
      int32_t *v = (int32_t *)&buf;
      *v = -(*(int32_t *)value);
      value = v;
    } else {
      return GRN_INVALID_ARGUMENT;
    }
    /* fallthru */
  case GRN_OBJ_INCR :
    {
      grn_io_win jw;
      uint32_t old_len;
      void *oldvalue = grn_ja_ref(ctx, ja, id, &jw, &old_len);
      if (oldvalue && old_len) {
        grn_rc rc = GRN_INVALID_ARGUMENT;
        if (old_len == sizeof(int64_t) && value_len == sizeof(int64_t)) {
          (*(int64_t *)oldvalue) += (*(int64_t *)value);
          rc = GRN_SUCCESS;
        } else if (old_len == sizeof(int32_t) && value_len == sizeof(int32_t)) {
          (*(int32_t *)oldvalue) += (*(int32_t *)value);
          rc = GRN_SUCCESS;
        }
        grn_ja_unref(ctx, &jw);
        return rc;
      }
    }
    /* fallthru */
  case GRN_OBJ_SET :
    if (value_len) {
      set_value(ctx, ja, id, value, value_len, &einfo);
    } else {
      memset(&einfo, 0, sizeof(grn_ja_einfo));
    }
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT, "grn_ja_put_raw called with illegal flags value");
    return GRN_INVALID_ARGUMENT;
  }
  if ((rc = grn_ja_replace(ctx, ja, id, &einfo, cas))) {
    if (!grn_io_lock(ctx, ja->io, grn_lock_timeout)) {
      grn_ja_free(ctx, ja, &einfo);
      grn_io_unlock(ja->io);
    }
  }
  return rc;
}

static grn_rc
grn_ja_putv_raw(grn_ctx *ctx,
                grn_ja *ja,
                grn_id id,
                grn_obj *header,
                grn_obj *body,
                grn_obj *footer)
{
  grn_rc rc;
  grn_io_win iw;
  grn_ja_einfo einfo;
  const size_t header_size = GRN_BULK_VSIZE(header);
  const size_t body_size = body ? GRN_BULK_VSIZE(body) : 0;
  const size_t footer_size = GRN_BULK_VSIZE(footer);
  const size_t size = header_size + body_size + footer_size;

  rc = grn_ja_alloc(ctx, ja, id, size, &einfo, &iw);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  grn_memcpy(iw.addr, GRN_BULK_HEAD(header), header_size);
  if (body_size > 0) {
    grn_memcpy((char *)iw.addr + header_size,
               GRN_BULK_HEAD(body), body_size);
  }
  if (footer_size > 0) {
    grn_memcpy((char *)iw.addr + header_size + body_size,
               GRN_BULK_HEAD(footer), footer_size);
  }
  grn_io_win_unmap(ctx, &iw);
  rc = grn_ja_replace(ctx, ja, id, &einfo, NULL);

  return rc;
}

uint32_t
grn_ja_size(grn_ctx *ctx, grn_ja *ja, grn_id id)
{
  grn_ja_einfo *einfo = NULL, *ei;
  uint32_t lseg, *pseg, pos, size;
  lseg = id >> JA_W_EINFO_IN_A_SEGMENT;
  pos = id & JA_M_EINFO_IN_A_SEGMENT;
  pseg = &ja->header->element_segs[lseg];
  if (*pseg == JA_ELEMENT_SEG_VOID) {
    ctx->rc = GRN_INVALID_ARGUMENT;
    return 0;
  }
  einfo = grn_io_seg_ref(ctx, ja->io, *pseg);
  if (!einfo) {
    ctx->rc = GRN_NO_MEMORY_AVAILABLE;
    return 0;
  }
  ei = &einfo[pos];
  if (ETINY_P(ei)) {
    ETINY_DEC(ei, size);
  } else {
    if (EHUGE_P(ei)) {
      size = ei->u.h.size;
    } else {
      size = (ei->u.n.c2 << 16) + ei->u.n.size;
    }
  }
  grn_io_seg_unref(ctx, ja->io, *pseg);
  return size;
}

grn_rc
grn_ja_element_info(grn_ctx *ctx, grn_ja *ja, grn_id id,
                    uint64_t *cas, uint32_t *pos, uint32_t *size)
{
  uint32_t pseg = ja->header->element_segs[id >> JA_W_EINFO_IN_A_SEGMENT];
  if (pseg == JA_ELEMENT_SEG_VOID) {
    return GRN_INVALID_ARGUMENT;
  } else {
    grn_ja_einfo *einfo = NULL;
    einfo = grn_io_seg_ref(ctx, ja->io, pseg);
    if (einfo) {
      grn_ja_einfo *ei;
      *cas = *((uint64_t *)&einfo[id & JA_M_EINFO_IN_A_SEGMENT]);
      ei = (grn_ja_einfo *)cas;
      if (ETINY_P(ei)) {
        ETINY_DEC(ei, *size);
        *pos = 0;
      } else {
        uint32_t jag;
        if (EHUGE_P(ei)) {
          EHUGE_DEC(ei, jag, *size);
          *pos = 0;
        } else {
          EINFO_DEC(ei, jag, *pos, *size);
        }
      }
      grn_io_seg_unref(ctx, ja->io, pseg);
    } else {
      return GRN_INVALID_ARGUMENT;
    }
  }
  return GRN_SUCCESS;
}

#define COMPRESSED_VALUE_META_FLAG(meta) ((meta) & 0xf000000000000000)
#define COMPRESSED_VALUE_META_FLAG_RAW             0x1000000000000000
#define COMPRESSED_VALUE_META_UNCOMPRESSED_LEN(meta) \
                                         ((meta) & 0x0fffffffffffffff)

#define COMPRESS_THRESHOLD_BYTE 256
#define COMPRESS_PACKED_VALUE_SIZE_MAX 257
        /* COMPRESS_THRESHOLD_BYTE - 1 + sizeof(uint64_t) = 257 */

#if defined(GRN_WITH_ZLIB) || defined(GRN_WITH_LZ4) || defined(GRN_WITH_ZSTD)
#  define GRN_WITH_COMPRESSED
#endif

#ifdef GRN_WITH_COMPRESSED
static void *
grn_ja_ref_packed(grn_ctx *ctx,
                  grn_io_win *iw,
                  uint32_t *value_len,
                  void *raw_value,
                  uint32_t raw_value_len,
                  void **compressed_value,
                  uint32_t *compressed_value_len,
                  uint32_t *uncompressed_value_len)
{
  uint64_t compressed_value_meta;

  compressed_value_meta = *((uint64_t *)raw_value);
  *compressed_value = (void *)(((uint64_t *)raw_value) + 1);
  *compressed_value_len = raw_value_len - sizeof(uint64_t);

  *uncompressed_value_len =
    COMPRESSED_VALUE_META_UNCOMPRESSED_LEN(compressed_value_meta);
  switch (COMPRESSED_VALUE_META_FLAG(compressed_value_meta)) {
  case COMPRESSED_VALUE_META_FLAG_RAW :
    iw->uncompressed_value = NULL;
    *value_len = *uncompressed_value_len;
    return *compressed_value;
  default :
    return NULL;
  }
}

static grn_rc
grn_ja_put_packed(grn_ctx *ctx,
                  grn_ja *ja,
                  grn_id id,
                  void *value,
                  uint32_t value_len,
                  int flags,
                  uint64_t *cas)
{
  char *packed_value[COMPRESS_PACKED_VALUE_SIZE_MAX];
  uint32_t packed_value_len;
  uint64_t packed_value_meta;

  packed_value_len = value_len + sizeof(uint64_t);
  packed_value_meta = value_len | COMPRESSED_VALUE_META_FLAG_RAW;
  *((uint64_t *)packed_value) = packed_value_meta;
  grn_memcpy(((uint64_t *)packed_value) + 1,
             value,
             value_len);
  return grn_ja_put_raw(ctx,
                        ja,
                        id,
                        packed_value,
                        packed_value_len,
                        flags,
                        cas);
}

static grn_rc
grn_ja_putv_packed(grn_ctx *ctx,
                   grn_ja *ja,
                   grn_id id,
                   grn_obj *header,
                   grn_obj *body,
                   grn_obj *footer)
{
  grn_rc rc;
  grn_io_win iw;
  grn_ja_einfo einfo;
  uint64_t packed_value_meta;
  const size_t meta_size = sizeof(uint64_t);
  const size_t header_size = GRN_BULK_VSIZE(header);
  const size_t body_size = body ? GRN_BULK_VSIZE(body) : 0;
  const size_t footer_size = GRN_BULK_VSIZE(footer);
  const size_t size = header_size + body_size + footer_size;

  packed_value_meta = size | COMPRESSED_VALUE_META_FLAG_RAW;

  rc = grn_ja_alloc(ctx, ja, id, meta_size + size, &einfo, &iw);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  *((uint64_t *)iw.addr) = packed_value_meta;
  grn_memcpy(((char *)iw.addr) + meta_size,
             GRN_BULK_HEAD(header),
             header_size);
  if (body_size > 0) {
    grn_memcpy((char *)iw.addr + meta_size + header_size,
               GRN_BULK_HEAD(body),
               body_size);
  }
  if (footer_size > 0) {
    grn_memcpy((char *)iw.addr + meta_size + header_size + body_size,
               GRN_BULK_HEAD(footer),
               footer_size);
  }
  grn_io_win_unmap(ctx, &iw);
  rc = grn_ja_replace(ctx, ja, id, &einfo, NULL);

  return rc;
}

static grn_rc
grn_ja_putv_compressed(grn_ctx *ctx,
                       grn_ja *ja,
                       grn_id id,
                       void *compressed,
                       size_t compressed_size,
                       size_t uncompressed_size)
{
  grn_rc rc;
  grn_io_win iw;
  grn_ja_einfo einfo;
  uint64_t compressed_meta;
  const size_t meta_size = sizeof(uint64_t);
  const size_t size = meta_size + compressed_size;

  compressed_meta = uncompressed_size;

  rc = grn_ja_alloc(ctx, ja, id, size, &einfo, &iw);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  *((uint64_t *)iw.addr) = compressed_meta;
  grn_memcpy(((char *)iw.addr) + meta_size,
             compressed,
             compressed_size);
  grn_io_win_unmap(ctx, &iw);
  rc = grn_ja_replace(ctx, ja, id, &einfo, NULL);

  return rc;
}

static void
grn_ja_compress_error(grn_ctx *ctx,
                      grn_ja *ja,
                      grn_id id,
                      grn_rc rc,
                      const char *message,
                      const char *detail)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_len;

  if (ja->obj.id == GRN_ID_NIL) {
    name[0] = '\0';
    name_len = 0;
  } else {
    name_len = grn_obj_name(ctx, (grn_obj *)ja, name, GRN_TABLE_MAX_KEY_SIZE);
  }
  ERR(rc,
      "[ja]%s: %s%.*s%s<%u>%s%s%s",
      message,
      name_len == 0 ? "" : "<",
      name_len,
      name,
      name_len == 0 ? "" : ">: ",
      id,
      detail ? " :<" : "",
      detail ? detail : "",
      detail ? ">" : "");
}
#endif /* GRN_WITH_COMPRESSED */

#ifdef GRN_WITH_ZLIB
#include <zlib.h>

static const char *
grn_zrc_to_string(int zrc)
{
  switch (zrc) {
  case Z_OK :
    return "OK";
  case Z_STREAM_END :
    return "Stream is end";
  case Z_NEED_DICT :
    return "Need dictionary";
  case Z_ERRNO :
    return "See errno";
  case Z_STREAM_ERROR :
    return "Stream error";
  case Z_DATA_ERROR :
    return "Data error";
  case Z_MEM_ERROR :
    return "Memory error";
  case Z_BUF_ERROR :
    return "Buffer error";
  case Z_VERSION_ERROR :
    return "Version error";
  default :
    return "Unknown";
  }
}

static void *
grn_ja_ref_zlib(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
  z_stream zstream;
  void *raw_value;
  uint32_t raw_value_len;
  void *zvalue;
  uint32_t zvalue_len;
  void *unpacked_value;
  uint32_t uncompressed_value_len;
  int zrc;

  if (!(raw_value = grn_ja_ref_raw(ctx, ja, id, iw, &raw_value_len))) {
    iw->uncompressed_value = NULL;
    *value_len = 0;
    return NULL;
  }

  unpacked_value = grn_ja_ref_packed(ctx,
                                     iw, value_len,
                                     raw_value, raw_value_len,
                                     &zvalue, &zvalue_len,
                                     &uncompressed_value_len);
  if (unpacked_value) {
    return unpacked_value;
  }

  zstream.next_in = (Bytef *)zvalue;
  zstream.avail_in = zvalue_len;
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  zrc = inflateInit2(&zstream, 15 /* windowBits */);
  if (zrc != Z_OK) {
    iw->uncompressed_value = NULL;
    *value_len = 0;
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to decompress: initialize",
                          grn_zrc_to_string(zrc));
    return NULL;
  }
  if (!(iw->uncompressed_value = GRN_MALLOC(uncompressed_value_len))) {
    inflateEnd(&zstream);
    iw->uncompressed_value = NULL;
    *value_len = 0;
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to decompress: allocate buffer",
                          NULL);
    return NULL;
  }
  zstream.next_out = (Bytef *)iw->uncompressed_value;
  zstream.avail_out = uncompressed_value_len;
  zrc = inflate(&zstream, Z_FINISH);
  if (zrc != Z_STREAM_END) {
    inflateEnd(&zstream);
    GRN_FREE(iw->uncompressed_value);
    iw->uncompressed_value = NULL;
    *value_len = 0;
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to decompress: finish",
                          grn_zrc_to_string(zrc));
    return NULL;
  }
  *value_len = zstream.total_out;
  zrc = inflateEnd(&zstream);
  if (zrc != Z_OK) {
    GRN_FREE(iw->uncompressed_value);
    iw->uncompressed_value = NULL;
    *value_len = 0;
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to decompress: end",
                          grn_zrc_to_string(zrc));
    return NULL;
  }
  return iw->uncompressed_value;
}
#endif /* GRN_WITH_ZLIB */

#ifdef GRN_WITH_LZ4
#include <lz4.h>

# if (LZ4_VERSION_MAJOR == 1 && LZ4_VERSION_MINOR < 6)
#  define LZ4_compress_default(source, dest, source_size, max_dest_size) \
  LZ4_compress((source), (dest), (source_size))
# endif

static void *
grn_ja_ref_lz4(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
  void *raw_value;
  uint32_t raw_value_len;
  void *lz4_value;
  uint32_t lz4_value_len;
  void *unpacked_value;
  uint32_t uncompressed_value_len;

  if (!(raw_value = grn_ja_ref_raw(ctx, ja, id, iw, &raw_value_len))) {
    iw->uncompressed_value = NULL;
    *value_len = 0;
    return NULL;
  }

  unpacked_value = grn_ja_ref_packed(ctx,
                                     iw, value_len,
                                     raw_value, raw_value_len,
                                     &lz4_value, &lz4_value_len,
                                     &uncompressed_value_len);
  if (unpacked_value) {
    return unpacked_value;
  }

  if (!(iw->uncompressed_value = GRN_MALLOC(uncompressed_value_len))) {
    iw->uncompressed_value = NULL;
    *value_len = 0;
    return NULL;
  }
  if (LZ4_decompress_safe((const char *)(lz4_value),
                          (char *)(iw->uncompressed_value),
                          lz4_value_len,
                          uncompressed_value_len) < 0) {
    GRN_FREE(iw->uncompressed_value);
    iw->uncompressed_value = NULL;
    *value_len = 0;
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_LZ4_ERROR,
                          "[lz4] failed to decompress",
                          NULL);
    return NULL;
  }
  *value_len = uncompressed_value_len;
  return iw->uncompressed_value;
}
#endif /* GRN_WITH_LZ4 */

#ifdef GRN_WITH_ZSTD
#include <zstd.h>

static void *
grn_ja_ref_zstd(grn_ctx *ctx,
                grn_ja *ja,
                grn_id id,
                grn_io_win *iw,
                uint32_t *value_len)
{
  void *raw_value;
  uint32_t raw_value_len;
  void *zstd_value;
  uint32_t zstd_value_len;
  void *unpacked_value;
  uint32_t uncompressed_value_len;
  size_t written_len;

  if (!(raw_value = grn_ja_ref_raw(ctx, ja, id, iw, &raw_value_len))) {
    iw->uncompressed_value = NULL;
    *value_len = 0;
    return NULL;
  }

  unpacked_value = grn_ja_ref_packed(ctx,
                                     iw, value_len,
                                     raw_value, raw_value_len,
                                     &zstd_value, &zstd_value_len,
                                     &uncompressed_value_len);
  if (unpacked_value) {
    return unpacked_value;
  }

  if (!(iw->uncompressed_value = GRN_MALLOC(uncompressed_value_len))) {
    iw->uncompressed_value = NULL;
    *value_len = 0;
    return NULL;
  }

  written_len = ZSTD_decompress((char *)(iw->uncompressed_value),
                                uncompressed_value_len,
                                zstd_value,
                                zstd_value_len);
  if (ZSTD_isError(written_len)) {
    GRN_FREE(iw->uncompressed_value);
    iw->uncompressed_value = NULL;
    *value_len = 0;
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZSTD_ERROR,
                          "[zstd] failed to decompress",
                          ZSTD_getErrorName(written_len));
    return NULL;
  }
  *value_len = uncompressed_value_len;
  return iw->uncompressed_value;
}
#endif /* GRN_WITH_ZSTD */

void *
grn_ja_ref(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_io_win *iw, uint32_t *value_len)
{
  switch (ja->header->flags & GRN_OBJ_COMPRESS_MASK) {
#ifdef GRN_WITH_ZLIB
  case GRN_OBJ_COMPRESS_ZLIB :
    return grn_ja_ref_zlib(ctx, ja, id, iw, value_len);
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_OBJ_COMPRESS_LZ4 :
    return grn_ja_ref_lz4(ctx, ja, id, iw, value_len);
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_OBJ_COMPRESS_ZSTD :
    return grn_ja_ref_zstd(ctx, ja, id, iw, value_len);
#endif /* GRN_WITH_ZSTD */
  default :
    return grn_ja_ref_raw(ctx, ja, id, iw, value_len);
  }
}

static grn_rc
grn_ja_unpack_value(grn_ctx *ctx,
                    grn_ja *ja,
                    grn_obj *value,
                    size_t offset);

grn_obj *
grn_ja_get_value(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_obj *value)
{
  grn_column_flags flags = ja->header->flags;
  if (!value) {
    uint8_t type;
    if ((flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR) {
      if (grn_type_id_is_text_family(ctx, ja->obj.range)) {
        type = GRN_VECTOR;
      } else {
        type = GRN_UVECTOR;
      }
    } else {
      type = GRN_BULK;
    }
    if (!(value = grn_obj_open(ctx, type, 0, ja->obj.range))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "[ja][get-value] failed to allocate value");
      goto exit;
    }
  }

  bool with_weight = ((flags & GRN_OBJ_WITH_WEIGHT) != 0);
  if (with_weight) {
    value->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }
  grn_vector_pack_flags pack_flags = 0;
  if (flags & GRN_OBJ_WEIGHT_FLOAT32) {
    pack_flags |= GRN_VECTOR_PACK_WEIGHT_FLOAT32;
  }

  void *v;
  uint32_t len;
  grn_io_win iw;
  if ((v = grn_ja_ref(ctx, ja, id, &iw, &len))) {
    if ((flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR) {
      bool is_var_size_element = grn_type_id_is_text_family(ctx, ja->obj.range);
      if (is_var_size_element) {
        grn_vector_unpack(ctx, value, v, len, pack_flags);
      } else {
        size_t offset = GRN_BULK_VSIZE(value);
        grn_bulk_write(ctx, value, v, len);
        grn_ja_unpack_value(ctx, ja, value, offset);
      }
    } else if ((flags & GRN_OBJ_RING_BUFFER) &&
               len > ja->header->max_element_size) {
      byte *b = v;
      uint32_t el = len - sizeof(uint32_t);
      uint32_t pos = *((uint32_t *)(b + el));
      GRN_ASSERT(pos < el);
      grn_bulk_write(ctx, value, (char *)(b + pos), el - pos);
      grn_bulk_write(ctx, value, (char *)(b), pos);
    } else {
      grn_bulk_write(ctx, value, v, len);
    }
    grn_ja_unref(ctx, &iw);
  }
exit :
  return value;
}

#ifdef GRN_WITH_ZLIB
grn_inline static grn_rc
grn_ja_put_zlib(grn_ctx *ctx, grn_ja *ja, grn_id id,
                void *value, uint32_t value_len, int flags, uint64_t *cas)
{
  grn_rc rc;
  z_stream zstream;
  void *zvalue;
  int zvalue_len;
  int zrc;

  if (value_len == 0) {
    return grn_ja_put_raw(ctx, ja, id, value, value_len, flags, cas);
  }

  if (value_len < COMPRESS_THRESHOLD_BYTE) {
    return grn_ja_put_packed(ctx, ja, id, value, value_len, flags, cas);
  }

  zstream.next_in = value;
  zstream.avail_in = value_len;
  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  zrc = deflateInit2(&zstream, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                     15 /* windowBits */,
                     8 /* memLevel */,
                     Z_DEFAULT_STRATEGY);
  if (zrc != Z_OK) {
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to compress: initialize",
                          grn_zrc_to_string(zrc));
    return ctx->rc;
  }
  zvalue_len = deflateBound(&zstream, value_len);
  if (!(zvalue = GRN_MALLOC(zvalue_len + sizeof(uint64_t)))) {
    deflateEnd(&zstream);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to allocate compress buffer",
                          NULL);
    return ctx->rc;
  }
  zstream.next_out = (Bytef *)(((uint64_t *)zvalue) + 1);
  zstream.avail_out = zvalue_len;
  zrc = deflate(&zstream, Z_FINISH);
  if (zrc != Z_STREAM_END) {
    deflateEnd(&zstream);
    GRN_FREE(zvalue);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to compress: finish",
                          grn_zrc_to_string(zrc));
    return ctx->rc;
  }
  zvalue_len = zstream.total_out;
  zrc = deflateEnd(&zstream);
  if (zrc != Z_OK) {
    GRN_FREE(zvalue);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to compress: end",
                          grn_zrc_to_string(zrc));
    return ctx->rc;
  }
  *(uint64_t *)zvalue = value_len;
  rc = grn_ja_put_raw(ctx, ja, id, zvalue, zvalue_len + sizeof(uint64_t), flags, cas);
  GRN_FREE(zvalue);
  return rc;
}

grn_inline static grn_rc
grn_ja_putv_zlib(grn_ctx *ctx,
                 grn_ja *ja,
                 grn_id id,
                 grn_obj *header,
                 grn_obj *body,
                 grn_obj *footer)
{
  grn_rc rc;
  const size_t header_size = GRN_BULK_VSIZE(header);
  const size_t body_size = body ? GRN_BULK_VSIZE(body) : 0;
  const size_t footer_size = GRN_BULK_VSIZE(footer);
  const size_t size = header_size + body_size + footer_size;
  z_stream zstream;
  Bytef *zvalue = NULL;
  int zwindow_bits = 15;
  int zmem_level = 8;
  int zrc;

  if (size < COMPRESS_THRESHOLD_BYTE) {
    return grn_ja_putv_packed(ctx, ja, id, header, body, footer);
  }

  zstream.zalloc = Z_NULL;
  zstream.zfree = Z_NULL;
  zrc = deflateInit2(&zstream,
                     Z_DEFAULT_COMPRESSION,
                     Z_DEFLATED,
                     zwindow_bits,
                     zmem_level,
                     Z_DEFAULT_STRATEGY);
  if (zrc != Z_OK) {
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to initialize compressor",
                          grn_zrc_to_string(zrc));
    return ctx->rc;
  }

  zstream.avail_out = deflateBound(&zstream, size);
  zvalue = GRN_MALLOC(zstream.avail_out);
  zstream.next_out = zvalue;
  if (!zstream.next_out) {
    deflateEnd(&zstream);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to allocate compress buffer",
                          NULL);
    return ctx->rc;
  }

  zstream.next_in = GRN_BULK_HEAD(header);
  zstream.avail_in = header_size;
  zrc = deflate(&zstream, Z_NO_FLUSH);
  if (zrc != Z_OK) {
    GRN_FREE(zvalue);
    deflateEnd(&zstream);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to compress header",
                          grn_zrc_to_string(zrc));
    return ctx->rc;
  }

  if (body_size > 0) {
    zstream.next_in = GRN_BULK_HEAD(body);
    zstream.avail_in = body_size;
    zrc = deflate(&zstream, Z_NO_FLUSH);
    if (zrc != Z_OK) {
      GRN_FREE(zvalue);
      deflateEnd(&zstream);
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZLIB_ERROR,
                            "[zlib] failed to compress body",
                            grn_zrc_to_string(zrc));
      return ctx->rc;
    }
  }

  if (footer_size > 0) {
    zstream.next_in = GRN_BULK_HEAD(footer);
    zstream.avail_in = footer_size;
    zrc = deflate(&zstream, Z_NO_FLUSH);
    if (zrc != Z_OK) {
      GRN_FREE(zvalue);
      deflateEnd(&zstream);
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZLIB_ERROR,
                            "[zlib] failed to compress footer",
                            grn_zrc_to_string(zrc));
      return ctx->rc;
    }
  }

  zrc = deflate(&zstream, Z_FINISH);
  if (zrc != Z_STREAM_END) {
    GRN_FREE(zvalue);
    deflateEnd(&zstream);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to finish compression",
                          grn_zrc_to_string(zrc));
    return ctx->rc;
  }

  rc = grn_ja_putv_compressed(ctx,
                              ja,
                              id,
                              zvalue,
                              zstream.total_out,
                              size);

  GRN_FREE(zvalue);
  zrc = deflateEnd(&zstream);
  if (zrc != Z_OK) {
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZLIB_ERROR,
                          "[zlib] failed to free compressor",
                          grn_zrc_to_string(zrc));
    return ctx->rc;
  }

  return rc;
}
#endif /* GRN_WITH_ZLIB */

#ifdef GRN_WITH_LZ4
grn_inline static grn_rc
grn_ja_put_lz4(grn_ctx *ctx, grn_ja *ja, grn_id id,
               void *value, uint32_t value_len, int flags, uint64_t *cas)
{
  grn_rc rc;
  void *packed_value;
  int packed_value_len_max;
  int packed_value_len_real;
  char *lz4_value;
  int lz4_value_len_max;
  int lz4_value_len_real;

  if (value_len == 0) {
    return grn_ja_put_raw(ctx, ja, id, value, value_len, flags, cas);
  }

  if (value_len < COMPRESS_THRESHOLD_BYTE) {
    return grn_ja_put_packed(ctx, ja, id, value, value_len, flags, cas);
  }

  if (value_len > (uint32_t)LZ4_MAX_INPUT_SIZE) {
    uint64_t packed_value_meta;

    packed_value_len_real = value_len + sizeof(uint64_t);
    packed_value = GRN_MALLOC(packed_value_len_real);
    if (!packed_value) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_LZ4_ERROR,
                            "[lz4] failed to allocate packed buffer",
                            NULL);
      return ctx->rc;
    }
    packed_value_meta = value_len | COMPRESSED_VALUE_META_FLAG_RAW;
    *((uint64_t *)packed_value) = packed_value_meta;
    grn_memcpy(((uint64_t *)packed_value) + 1,
               value,
               value_len);
    rc = grn_ja_put_raw(ctx,
                        ja,
                        id,
                        packed_value,
                        packed_value_len_real,
                        flags,
                        cas);
    GRN_FREE(packed_value);
    return rc;
  }

  lz4_value_len_max = LZ4_compressBound(value_len);
  packed_value_len_max = lz4_value_len_max + sizeof(uint64_t);
  if (!(packed_value = GRN_MALLOC(packed_value_len_max))) {
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_LZ4_ERROR,
                          "[lz4] failed to allocate compress buffer",
                          NULL);
    return ctx->rc;
  }
  lz4_value = (char *)((uint64_t *)packed_value + 1);
  lz4_value_len_real = LZ4_compress_default((const char *)value,
                                            lz4_value,
                                            value_len,
                                            lz4_value_len_max);
  if (lz4_value_len_real <= 0) {
    GRN_FREE(packed_value);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_LZ4_ERROR,
                          "[lz4] failed to compress",
                          NULL);
    return ctx->rc;
  }
  *(uint64_t *)packed_value = value_len;
  packed_value_len_real = lz4_value_len_real + sizeof(uint64_t);
  rc = grn_ja_put_raw(ctx,
                      ja,
                      id,
                      packed_value,
                      packed_value_len_real,
                      flags,
                      cas);
  GRN_FREE(packed_value);
  return rc;
}

grn_inline static grn_rc
grn_ja_putv_lz4(grn_ctx *ctx,
                grn_ja *ja,
                grn_id id,
                grn_obj *header,
                grn_obj *body,
                grn_obj *footer)
{
  grn_rc rc;
  const size_t header_size = GRN_BULK_VSIZE(header);
  const size_t body_size = body ? GRN_BULK_VSIZE(body) : 0;
  const size_t footer_size = GRN_BULK_VSIZE(footer);
  const size_t size = header_size + body_size + footer_size;
  grn_obj buffer;
  char *lz4_value;
  int lz4_value_len_max;
  int lz4_value_len_real;

  if (size < COMPRESS_THRESHOLD_BYTE) {
    return grn_ja_putv_packed(ctx, ja, id, header, body, footer);
  }

  if (size > (uint32_t)LZ4_MAX_INPUT_SIZE) {
    return grn_ja_putv_packed(ctx, ja, id, header, body, footer);
  }

  GRN_TEXT_INIT(&buffer, 0);
  GRN_TEXT_PUT(ctx, &buffer, GRN_BULK_HEAD(header), header_size);
  if (body_size > 0)
    GRN_TEXT_PUT(ctx, &buffer, GRN_BULK_HEAD(body), body_size);
  if (footer_size > 0)
    GRN_TEXT_PUT(ctx, &buffer, GRN_BULK_HEAD(footer), footer_size);

  lz4_value_len_max = LZ4_compressBound(size);
  if (!(lz4_value = GRN_MALLOC(lz4_value_len_max))) {
    GRN_OBJ_FIN(ctx, &buffer);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_LZ4_ERROR,
                          "[lz4] failed to allocate compress buffer",
                          NULL);
    return ctx->rc;
  }

  lz4_value_len_real = LZ4_compress_default(GRN_TEXT_VALUE(&buffer),
                                            lz4_value,
                                            GRN_TEXT_LEN(&buffer),
                                            lz4_value_len_max);
  if (lz4_value_len_real <= 0) {
    GRN_OBJ_FIN(ctx, &buffer);
    GRN_FREE(lz4_value);
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_LZ4_ERROR,
                          "[lz4] failed to compress",
                          NULL);
    return ctx->rc;
  }

  rc = grn_ja_putv_compressed(ctx,
                              ja,
                              id,
                              lz4_value,
                              lz4_value_len_real,
                              size);

  GRN_OBJ_FIN(ctx, &buffer);
  GRN_FREE(lz4_value);

  return rc;
}
#endif /* GRN_WITH_LZ4 */

#ifdef GRN_WITH_ZSTD
grn_inline static grn_rc
grn_ja_put_zstd(grn_ctx *ctx,
                grn_ja *ja,
                grn_id id,
                void *value,
                uint32_t value_len,
                int flags,
                uint64_t *cas)
{
  grn_rc rc;
  void *packed_value;
  int packed_value_len_max;
  int packed_value_len_real;
  void *zstd_value;
  int zstd_value_len_max;
  int zstd_value_len_real;
  int zstd_compression_level = 3;

  if (value_len == 0) {
    return grn_ja_put_raw(ctx, ja, id, value, value_len, flags, cas);
  }

  if (value_len < COMPRESS_THRESHOLD_BYTE) {
    return grn_ja_put_packed(ctx, ja, id, value, value_len, flags, cas);
  }

  zstd_value_len_max = ZSTD_compressBound(value_len);
  packed_value_len_max = zstd_value_len_max + sizeof(uint64_t);
  if (!(packed_value = GRN_MALLOC(packed_value_len_max))) {
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZSTD_ERROR,
                          "[zstd] failed to allocate compress buffer",
                          NULL);
    return ctx->rc;
  }
  zstd_value = ((uint64_t *)packed_value) + 1;
  zstd_value_len_real = ZSTD_compress(zstd_value, zstd_value_len_max,
                                      value, value_len,
                                      zstd_compression_level);
  if (ZSTD_isError(zstd_value_len_real)) {
    grn_ja_compress_error(ctx,
                          ja,
                          id,
                          GRN_ZSTD_ERROR,
                          "[zstd] failed to compress",
                          ZSTD_getErrorName(zstd_value_len_real));
    return ctx->rc;
  }
  *(uint64_t *)packed_value = value_len;
  packed_value_len_real = zstd_value_len_real + sizeof(uint64_t);
  rc = grn_ja_put_raw(ctx,
                      ja,
                      id,
                      packed_value,
                      packed_value_len_real,
                      flags,
                      cas);
  GRN_FREE(packed_value);
  return rc;
}

grn_inline static grn_rc
grn_ja_putv_zstd(grn_ctx *ctx,
                 grn_ja *ja,
                 grn_id id,
                 grn_obj *header,
                 grn_obj *body,
                 grn_obj *footer)
{
  grn_rc rc;
  const size_t header_size = GRN_BULK_VSIZE(header);
  const size_t body_size = body ? GRN_BULK_VSIZE(body) : 0;
  const size_t footer_size = GRN_BULK_VSIZE(footer);
  const size_t size = header_size + body_size + footer_size;
  int zstd_compression_level = 3;
#if ZSTD_VERSION_MAJOR < 1
  grn_obj all_value;
  void *zstd_value = NULL;

  GRN_TEXT_INIT(&all_value, 0);
#else /* ZSTD_VERSION_MAJOR < 1 */
  ZSTD_CStream *zstd_stream = NULL;
  ZSTD_outBuffer zstd_output;

  zstd_output.dst = NULL;
  zstd_output.pos = 0;
#endif /* ZSTD_VERSION_MAJOR < 1 */

  if (size < COMPRESS_THRESHOLD_BYTE) {
    return grn_ja_putv_packed(ctx, ja, id, header, body, footer);
  }

#if ZSTD_VERSION_MAJOR < 1
  {
    int zstd_value_len_max;
    int zstd_value_len_real;

    zstd_value_len_max = ZSTD_compressBound(size);
    zstd_value = GRN_MALLOC(zstd_value_len_max);
    if (!zstd_value) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to allocate compress buffer",
                            NULL);
      rc = ctx->rc;
      goto exit;
    }

    GRN_TEXT_PUT(ctx, &all_value, GRN_TEXT_VALUE(header), header_size);
    if (body_size > 0) {
      GRN_TEXT_PUT(ctx, &all_value, GRN_TEXT_VALUE(body), body_size);
    }
    GRN_TEXT_PUT(ctx, &all_value, GRN_TEXT_VALUE(footer), footer_size);
    zstd_value_len_real = ZSTD_compress(zstd_value, zstd_value_len_max,
                                        GRN_TEXT_VALUE(&all_value), size,
                                        zstd_compression_level);
    if (ZSTD_isError(zstd_value_len_real)) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to compress",
                            ZSTD_getErrorName(zstd_value_len_real));
      rc = ctx->rc;
      goto exit;
    }
    rc = grn_ja_putv_compressed(ctx,
                                ja,
                                id,
                                zstd_value,
                                zstd_value_len_real,
                                size);
  }
#else /* ZSTD_VERSION_MAJOR < 1 */
  {
    ZSTD_inBuffer zstd_input;
    size_t zstd_result;

    zstd_stream = ZSTD_createCStream();
    if (!zstd_stream) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to allocate stream compressor",
                            NULL);
      rc = ctx->rc;
      goto exit;
    }

    zstd_result = ZSTD_initCStream(zstd_stream, zstd_compression_level);
    if (ZSTD_isError(zstd_result)) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to initialize stream compressor",
                            ZSTD_getErrorName(zstd_result));
      rc = ctx->rc;
      goto exit;
    }

    zstd_output.size = ZSTD_compressBound(size);
    zstd_output.dst = GRN_MALLOC(zstd_output.size);
    if (!zstd_output.dst) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to allocate compress buffer",
                            NULL);
      rc = ctx->rc;
      goto exit;
    }

    zstd_input.src = GRN_BULK_HEAD(header);
    zstd_input.size = header_size;
    zstd_input.pos = 0;
    zstd_result = ZSTD_compressStream(zstd_stream, &zstd_output, &zstd_input);
    if (ZSTD_isError(zstd_result)) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to compress header",
                            ZSTD_getErrorName(zstd_result));
      rc = ctx->rc;
      goto exit;
    }
    if (body_size > 0) {
      zstd_input.src = GRN_BULK_HEAD(body);
      zstd_input.size = body_size;
      zstd_input.pos = 0;
      zstd_result = ZSTD_compressStream(zstd_stream, &zstd_output, &zstd_input);
      if (ZSTD_isError(zstd_result)) {
        grn_ja_compress_error(ctx,
                              ja,
                              id,
                              GRN_ZSTD_ERROR,
                              "[zstd] failed to compress body",
                              ZSTD_getErrorName(zstd_result));
        rc = ctx->rc;
        goto exit;
      }
    }
    if (footer_size > 0) {
      zstd_input.src = GRN_BULK_HEAD(footer);
      zstd_input.size = footer_size;
      zstd_input.pos = 0;
      zstd_result = ZSTD_compressStream(zstd_stream, &zstd_output, &zstd_input);
      if (ZSTD_isError(zstd_result)) {
        grn_ja_compress_error(ctx,
                              ja,
                              id,
                              GRN_ZSTD_ERROR,
                              "[zstd] failed to compress footer",
                              ZSTD_getErrorName(zstd_result));
        rc = ctx->rc;
        goto exit;
      }
    }

    zstd_result = ZSTD_endStream(zstd_stream, &zstd_output);
    if (ZSTD_isError(zstd_result)) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to finish stream compression",
                            ZSTD_getErrorName(zstd_result));
      rc = ctx->rc;
      goto exit;
    }

    if (zstd_result > 0) {
      grn_ja_compress_error(ctx,
                            ja,
                            id,
                            GRN_ZSTD_ERROR,
                            "[zstd] failed to finish stream compression "
                            "because some data remain buffer",
                            ZSTD_getErrorName(zstd_result));
      rc = ctx->rc;
      goto exit;
    }

    rc = grn_ja_putv_compressed(ctx,
                                ja,
                                id,
                                zstd_output.dst,
                                zstd_output.pos,
                                size);
  }
#endif /* ZSTD_VERSION_MAJOR < 1 */

exit :
#if ZSTD_VERSION_MAJOR < 1
  GRN_OBJ_FIN(ctx, &all_value);
  if (zstd_value) {
    GRN_FREE(zstd_value);
  }
#else /* ZSTD_VERSION_MAJOR < 1 */
  if (zstd_stream) {
    ZSTD_freeCStream(zstd_stream);
  }
  if (zstd_output.dst) {
    GRN_FREE(zstd_output.dst);
  }
#endif /* ZSTD_VERSION_MAJOR < 1 */

  return rc;
}
#endif /* GRN_WITH_ZSTD */

grn_rc
grn_ja_put(grn_ctx *ctx, grn_ja *ja, grn_id id, void *value, uint32_t value_len,
           int flags, uint64_t *cas)
{
  switch (ja->header->flags & GRN_OBJ_COMPRESS_MASK) {
#ifdef GRN_WITH_ZLIB
  case GRN_OBJ_COMPRESS_ZLIB :
    return grn_ja_put_zlib(ctx, ja, id, value, value_len, flags, cas);
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_OBJ_COMPRESS_LZ4 :
    return grn_ja_put_lz4(ctx, ja, id, value, value_len, flags, cas);
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_OBJ_COMPRESS_ZSTD :
    return grn_ja_put_zstd(ctx, ja, id, value, value_len, flags, cas);
#endif /* GRN_WITH_ZSTD */
  default :
    return grn_ja_put_raw(ctx, ja, id, value, value_len, flags, cas);
  }
}

grn_rc
grn_ja_putv(grn_ctx *ctx, grn_ja *ja, grn_id id, grn_obj *vector, int flags)
{
  const grn_column_flags column_flags = ja->header->flags;
  grn_vector_pack_flags pack_flags = 0;
  if (column_flags & GRN_OBJ_WEIGHT_FLOAT32) {
    pack_flags |= GRN_VECTOR_PACK_WEIGHT_FLOAT32;
  }
  grn_obj header, footer;
  grn_rc rc = GRN_SUCCESS;
  GRN_TEXT_INIT(&header, 0);
  GRN_TEXT_INIT(&footer, 0);
  grn_obj *target_vector = vector;
  grn_obj target_vector_buffer;
  uint32_t set_mode = (flags & GRN_OBJ_SET_MASK);
  switch (set_mode) {
  case GRN_OBJ_SET :
    break;
  case GRN_OBJ_APPEND :
  case GRN_OBJ_PREPEND :
    {
      grn_id range_id = DB_OBJ(ja)->range;
      if (grn_type_id_is_text_family(ctx, range_id)) {
        GRN_VALUE_VAR_SIZE_INIT(&target_vector_buffer,
                                GRN_OBJ_VECTOR,
                                range_id);
      } else {
        GRN_VALUE_FIX_SIZE_INIT(&target_vector_buffer,
                                GRN_OBJ_VECTOR,
                                range_id);
        grn_column_flags flags = grn_ja_get_flags(ctx, ja);
        if (flags & GRN_OBJ_WITH_WEIGHT) {
          target_vector_buffer.header.flags |= GRN_OBJ_WITH_WEIGHT;
        }
      }
      if ((flags & GRN_OBJ_SET_MASK) == GRN_OBJ_APPEND) {
        grn_ja_get_value(ctx, ja, id, &target_vector_buffer);
      }
      if (grn_obj_is_vector(ctx, vector)) {
        grn_vector_copy(ctx, vector, &target_vector_buffer);
      } else {
        grn_uvector_copy(ctx, vector, &target_vector_buffer);
      }
      if ((flags & GRN_OBJ_SET_MASK) == GRN_OBJ_PREPEND) {
        grn_ja_get_value(ctx, ja, id, &target_vector_buffer);
      }
    }
    target_vector = &target_vector_buffer;
    break;
  default :
    {
      GRN_DEFINE_NAME(ja);
      ERR(GRN_INVALID_ARGUMENT,
          "[ja][putv][%.*s][%u] unsupported set mode: %d: %s",
          name_size, name,
          id,
          flags,
          grn_obj_set_flag_to_string(flags));
      return GRN_INVALID_ARGUMENT;
    }
  }
  grn_obj *body = grn_vector_pack(ctx,
                                  target_vector,
                                  0,
                                  grn_vector_size(ctx, target_vector),
                                  pack_flags,
                                  &header,
                                  &footer);
  switch (column_flags & GRN_OBJ_COMPRESS_MASK) {
#ifdef GRN_WITH_ZLIB
  case GRN_OBJ_COMPRESS_ZLIB :
    rc = grn_ja_putv_zlib(ctx, ja, id, &header, body, &footer);
    break;
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_OBJ_COMPRESS_LZ4 :
    rc = grn_ja_putv_lz4(ctx, ja, id, &header, body, &footer);
    break;
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_OBJ_COMPRESS_ZSTD :
    rc = grn_ja_putv_zstd(ctx, ja, id, &header, body, &footer);
    break;
#endif /* GRN_WITH_ZSTD */
  default :
    rc = grn_ja_putv_raw(ctx, ja, id, &header, body, &footer);
    break;
  }
  GRN_OBJ_FIN(ctx, &footer);
  GRN_OBJ_FIN(ctx, &header);
  if (&target_vector_buffer == target_vector) {
    GRN_OBJ_FIN(ctx, &target_vector_buffer);
  }
  return rc;
}

static void
grn_ja_cast_value_set_error(grn_ctx *ctx,
                            grn_ja *ja,
                            grn_rc rc,
                            grn_obj *invalid_value,
                            const char *tag,
                            const char *message)
{
  char column_name[GRN_TABLE_MAX_KEY_SIZE];
  int column_name_size;
  grn_obj inspected;
  column_name_size = grn_obj_name(ctx,
                                  (grn_obj *)ja,
                                  column_name,
                                  GRN_TABLE_MAX_KEY_SIZE);
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, invalid_value);
  ERR(rc,
      "[ja][cast-value]%s[%.*s] %s: <%.*s>",
      tag,
      column_name_size,
      column_name,
      message,
      (int)GRN_TEXT_LEN(&inspected),
      GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
}

static grn_obj *
grn_ja_cast_value_scalar(grn_ctx *ctx,
                         grn_ja *ja,
                         grn_obj *value,
                         grn_obj *buffer,
                         int set_flags)
{
  grn_id range_id = ja->obj.range;
  grn_id buffer_domain_id = GRN_DB_VOID;

  switch (set_flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_INCR :
  case GRN_OBJ_DECR :
    if (value->header.domain == GRN_DB_INT32 ||
        value->header.domain == GRN_DB_INT64) {
      /* do nothing */
    } else if (GRN_DB_INT8 <= value->header.domain &&
               value->header.domain < GRN_DB_INT32) {
      buffer_domain_id = GRN_DB_INT32;
    } else {
      buffer_domain_id = GRN_DB_INT64;
    }
    break;
  default :
    if (grn_type_id_is_text_family(ctx, range_id) &&
        grn_type_id_is_text_family(ctx, value->header.domain)) {
      /* do nothing */
    } else {
      if (range_id != value->header.domain) {
        buffer_domain_id = range_id;
      }
    }
    break;
  }

  if (buffer_domain_id == GRN_DB_VOID) {
    return value;
  }


  grn_obj_reinit(ctx, buffer, buffer_domain_id, 0);
  grn_rc rc = grn_obj_cast(ctx, value, buffer, true);
  if (rc != GRN_SUCCESS) {
    grn_ja_cast_value_set_error(ctx,
                                ja,
                                rc,
                                value,
                                "[scalar]",
                                "failed to cast");
    return NULL;
  }

  return buffer;
}

static grn_obj *
grn_ja_cast_value_vector_var_bulk(grn_ctx *ctx,
                                  grn_ja *ja,
                                  grn_obj *value,
                                  grn_obj *buffer,
                                  int set_flags,
                                  grn_obj *range)
{
  grn_column_flags flags = ja->header->flags;
  bool with_weight = ((flags & GRN_OBJ_WITH_WEIGHT) != 0);

  grn_obj_reinit(ctx, buffer, ja->obj.range, GRN_OBJ_VECTOR);
  if (with_weight) {
    buffer->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }

  if (GRN_BULK_VSIZE(value) == 0) {
    return buffer;
  }

  buffer->header.impl_flags = GRN_OBJ_DO_SHALLOW_COPY;
  buffer->u.v.body = value;
  grn_vector_delimit(ctx, buffer, 0, GRN_ID_NIL);

  return buffer;
}

static grn_obj *
grn_ja_cast_value_vector_var_uvector(grn_ctx *ctx,
                                     grn_ja *ja,
                                     grn_obj *value,
                                     grn_obj *buffer,
                                     int set_flags,
                                     grn_obj *range)
{
  grn_column_flags flags = ja->header->flags;
  bool with_weight = ((flags & GRN_OBJ_WITH_WEIGHT) != 0);

  grn_obj_reinit(ctx, buffer, ja->obj.range, GRN_OBJ_VECTOR);
  if (with_weight) {
    buffer->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }

  uint32_t n = grn_uvector_size(ctx, value);
  if (n == 0) {
    return buffer;
  }

  size_t element_value_size = grn_type_id_size(ctx, value->header.domain);
  size_t element_size = element_value_size;
  bool value_is_weight_uvector = grn_obj_is_weight_uvector(ctx, value);
  if (value_is_weight_uvector) {
    element_size += sizeof(float);
  }

  grn_obj element;
  grn_obj casted_element;
  GRN_VALUE_FIX_SIZE_INIT(&element,
                          GRN_OBJ_DO_SHALLOW_COPY,
                          value->header.domain);
  GRN_VALUE_VAR_SIZE_INIT(&casted_element, 0, ja->obj.range);
  const char *value_raw = GRN_BULK_HEAD(value);
  uint32_t i;
  for (i = 0; i < n; i++) {
    size_t offset = (element_size * i);
    GRN_TEXT_SET(ctx, &element, value_raw + offset, element_value_size);
    GRN_BULK_REWIND(&casted_element);
    grn_rc rc = grn_obj_cast(ctx, &element, &casted_element, true);
    if (rc != GRN_SUCCESS) {
      ERR_CAST((grn_obj *)ja, range, &element);
      GRN_OBJ_FIN(ctx, &element);
      GRN_OBJ_FIN(ctx, &casted_element);
      return NULL;
    }
    float weight = *((float *)(value_raw + offset + element_value_size));
    grn_vector_add_element_float(ctx,
                                 buffer,
                                 GRN_BULK_HEAD(&casted_element),
                                 GRN_BULK_VSIZE(&casted_element),
                                 weight,
                                 casted_element.header.domain);
  }
  GRN_OBJ_FIN(ctx, &casted_element);
  GRN_OBJ_FIN(ctx, &element);

  return buffer;
}

static grn_obj *
grn_ja_cast_value_vector_var_vector(grn_ctx *ctx,
                                    grn_ja *ja,
                                    grn_obj *value,
                                    grn_obj *buffer,
                                    int set_flags,
                                    grn_obj *range)
{
  grn_column_flags flags = ja->header->flags;
  bool with_weight = ((flags & GRN_OBJ_WITH_WEIGHT) != 0);

  grn_obj_reinit(ctx, buffer, ja->obj.range, GRN_OBJ_VECTOR);
  if (with_weight) {
    buffer->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }

  uint32_t n = grn_vector_size(ctx, value);
  if (n == 0) {
    return buffer;
  }

  bool need_convert = false;
  uint32_t i;
  for (i = 0; i < n; i++) {
    const char *element_raw;
    grn_id domain;
    grn_vector_get_element_float(ctx,
                                 value,
                                 i,
                                 &element_raw,
                                 NULL,
                                 &domain);
    if (domain != ja->obj.range) {
      need_convert = true;
      break;
    }
  }
  if (!need_convert) {
    return value;
  }

  grn_obj element;
  grn_obj casted_element;
  GRN_OBJ_INIT(&element, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY, GRN_ID_NIL);
  GRN_OBJ_INIT(&casted_element, GRN_BULK, 0, ja->obj.range);
  for (i = 0; i < n; i++) {
    const char *element_raw;
    float weight;
    grn_id domain;
    uint32_t element_size = grn_vector_get_element_float(ctx,
                                                         value,
                                                         i,
                                                         &element_raw,
                                                         &weight,
                                                         &domain);
    if (domain == ja->obj.range) {
      grn_vector_add_element_float(ctx,
                                   buffer,
                                   element_raw,
                                   element_size,
                                   weight,
                                   domain);
    } else {
      GRN_TEXT_SET(ctx, &element, element_raw, element_size);
      element.header.domain = domain;
      GRN_BULK_REWIND(&casted_element);
      grn_rc rc = grn_obj_cast(ctx, &element, &casted_element, true);
      if (rc != GRN_SUCCESS) {
        ERR_CAST((grn_obj *)ja, range, &element);
        GRN_OBJ_FIN(ctx, &element);
        GRN_OBJ_FIN(ctx, &casted_element);
        return NULL;
      }
      grn_vector_add_element_float(ctx,
                                   buffer,
                                   GRN_BULK_HEAD(&casted_element),
                                   GRN_BULK_VSIZE(&casted_element),
                                   weight,
                                   casted_element.header.domain);
    }
  }
  GRN_OBJ_FIN(ctx, &element);
  GRN_OBJ_FIN(ctx, &casted_element);

  return buffer;
}

static grn_obj *
grn_ja_cast_value_vector_fixed_bulk(grn_ctx *ctx,
                                    grn_ja *ja,
                                    grn_obj *value,
                                    grn_obj *buffer,
                                    int set_flags,
                                    grn_obj *range)
{
  grn_column_flags flags = ja->header->flags;
  bool with_weight = ((flags & GRN_OBJ_WITH_WEIGHT) != 0);

  grn_obj_reinit(ctx, buffer, ja->obj.range, GRN_OBJ_VECTOR);
  if (with_weight) {
    buffer->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }

  if (GRN_BULK_VSIZE(value) == 0) {
    return buffer;
  }

  if (value->header.domain == ja->obj.range) {
    grn_uvector_add_element_record(ctx, buffer, GRN_RECORD_VALUE(value), 0);
    return buffer;
  }

  bool need_tokenize = false;
  if (grn_obj_is_table_with_key(ctx, range)) {
    grn_obj *tokenizer;
    grn_table_get_info(ctx, range, NULL, NULL, &tokenizer, NULL, NULL);
    if (tokenizer) {
      grn_obj_unref(ctx, tokenizer);
      need_tokenize = true;
    }
  }

  if (need_tokenize) {
    /* TODO: Should we move this logic to grn_obj_cast()? */
    unsigned int token_flags = 0;
    grn_token_cursor *token_cursor;
    token_cursor = grn_token_cursor_open(ctx,
                                         range,
                                         GRN_BULK_HEAD(value),
                                         GRN_BULK_VSIZE(value),
                                         GRN_TOKEN_ADD,
                                         token_flags);
    if (token_cursor) {
      while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
        grn_id tid = grn_token_cursor_next(ctx, token_cursor);
        grn_uvector_add_element_record(ctx, buffer, tid, 0);
      }
      grn_token_cursor_close(ctx, token_cursor);
    }
  } else {
    grn_rc rc = grn_obj_cast(ctx, value, buffer, true);
    if (rc != GRN_SUCCESS) {
      ERR_CAST((grn_obj *)ja, range, value);
      return NULL;
    }
  }

  return buffer;
}

static grn_obj *
grn_ja_cast_value_vector_fixed_uvector(grn_ctx *ctx,
                                       grn_ja *ja,
                                       grn_obj *value,
                                       grn_obj *buffer,
                                       int set_flags,
                                       grn_obj *range)
{
  grn_column_flags flags = ja->header->flags;
  bool with_weight = ((flags & GRN_OBJ_WITH_WEIGHT) != 0);
  bool is_weight_float32 = ((flags & GRN_OBJ_WEIGHT_FLOAT32) != 0);
  bool value_is_weight_uvector = grn_obj_is_weight_uvector(ctx, value);

  bool need_convert = false;
  bool need_cast = false;

  if (with_weight) {
    if (!value_is_weight_uvector) {
      need_convert = true;
    }
    if (!is_weight_float32) {
      need_convert = true;
    }
  } else {
    if (value_is_weight_uvector) {
      need_convert = true;
    }
  }

  grn_id range_id = ja->obj.range;
  if (range_id != value->header.domain) {
    need_convert = true;
    need_cast = true;
  }

  if (!need_convert) {
    return value;
  }

  grn_obj_reinit(ctx, buffer, range_id, GRN_OBJ_VECTOR);
  if (with_weight) {
    buffer->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }
  uint32_t n = grn_uvector_size(ctx, value);
  if (n == 0) {
    return buffer;
  }

  size_t element_value_size = grn_type_id_size(ctx, value->header.domain);
  size_t element_size = element_value_size;
  if (value_is_weight_uvector) {
    element_size += sizeof(float);
  }

  grn_obj element;
  grn_obj casted_element;
  GRN_VALUE_FIX_SIZE_INIT(&element,
                          GRN_OBJ_DO_SHALLOW_COPY,
                          value->header.domain);
  GRN_VALUE_FIX_SIZE_INIT(&casted_element, 0, ja->obj.range);
  const char *value_raw = GRN_BULK_HEAD(value);
  uint32_t i;
  for (i = 0; i < n; i++) {
    size_t offset = (element_size * i);
    if (need_cast) {
      GRN_TEXT_SET(ctx, &element, value_raw + offset, element_value_size);
      GRN_BULK_REWIND(&casted_element);
      grn_rc rc = grn_obj_cast(ctx, &element, &casted_element, true);
      if (rc != GRN_SUCCESS) {
        ERR_CAST((grn_obj *)ja, range, &element);
        GRN_OBJ_FIN(ctx, &element);
        GRN_OBJ_FIN(ctx, &casted_element);
        return NULL;
      }
      grn_bulk_write(ctx,
                     buffer,
                     GRN_BULK_HEAD(&casted_element),
                     GRN_BULK_VSIZE(&casted_element));
    } else {
      grn_bulk_write(ctx, buffer, value_raw + offset, element_value_size);
    }
    if (with_weight) {
      float weight = *((float *)(value_raw + offset + element_value_size));
      GRN_FLOAT32_PUT(ctx, buffer, weight);
    }
  }
  GRN_OBJ_FIN(ctx, &casted_element);
  GRN_OBJ_FIN(ctx, &element);

  return buffer;
}

static grn_obj *
grn_ja_cast_value_vector_fixed_vector(grn_ctx *ctx,
                                      grn_ja *ja,
                                      grn_obj *value,
                                      grn_obj *buffer,
                                      int set_flags,
                                      grn_obj *range)
{
  grn_column_flags flags = ja->header->flags;
  bool with_weight = ((flags & GRN_OBJ_WITH_WEIGHT) != 0);

  grn_obj_reinit(ctx, buffer, ja->obj.range, GRN_OBJ_VECTOR);
  if (with_weight) {
    buffer->header.flags |= GRN_OBJ_WITH_WEIGHT;
  }
  uint32_t n = grn_vector_size(ctx, value);
  if (n == 0) {
    return buffer;
  }

  uint32_t i;
  grn_obj element;
  grn_obj casted_element;
  GRN_TEXT_INIT(&element, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_OBJ_INIT(&casted_element, GRN_BULK, 0, ja->obj.range);
  for (i = 0; i < n; i++) {
    const char *element_raw;
    float weight;
    grn_id element_domain;
    uint32_t element_size = grn_vector_get_element_float(ctx,
                                                         value,
                                                         i,
                                                         &element_raw,
                                                         &weight,
                                                         &element_domain);
    if (element_domain == buffer->header.domain) {
      grn_bulk_write(ctx, buffer, element_raw, element_size);
    } else {
      GRN_TEXT_SET(ctx, &element, element_raw, element_size);
      element.header.domain = element_domain;
      GRN_BULK_REWIND(&casted_element);
      grn_rc rc = grn_obj_cast(ctx, &element, &casted_element, true);
      if (rc != GRN_SUCCESS) {
        ERR_CAST((grn_obj *)ja, range, &element);
        return NULL;
      }
      grn_bulk_write(ctx,
                     buffer,
                     GRN_BULK_HEAD(&casted_element),
                     GRN_BULK_VSIZE(&casted_element));
    }
    if (with_weight) {
      GRN_FLOAT32_PUT(ctx, buffer, weight);
    }
  }
  GRN_OBJ_FIN(ctx, &casted_element);
  GRN_OBJ_FIN(ctx, &element);

  return buffer;
}

static grn_obj *
grn_ja_cast_value_vector(grn_ctx *ctx,
                         grn_ja *ja,
                         grn_obj *value,
                         grn_obj *buffer,
                         int set_flags)
{
  if (value->header.type == GRN_VOID) {
    return value;
  }

  grn_id range_id = ja->obj.range;
  grn_obj *range = grn_ctx_at(ctx, range_id);
  if (!range) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_INVALID_ARGUMENT;
    }
    grn_ja_cast_value_set_error(ctx,
                                ja,
                                rc,
                                (grn_obj *)ja,
                                "[vector]",
                                "range is dangling");
    return NULL;
  }

  grn_obj *casted_value = NULL;
  bool invalid_type = false;
  if (grn_obj_is_text_family_type(ctx, range)) {
    switch (value->header.type) {
    case GRN_BULK :
      casted_value = grn_ja_cast_value_vector_var_bulk(ctx,
                                                       ja,
                                                       value,
                                                       buffer,
                                                       set_flags,
                                                       range);
      break;
    case GRN_UVECTOR :
      casted_value = grn_ja_cast_value_vector_var_uvector(ctx,
                                                          ja,
                                                          value,
                                                          buffer,
                                                          set_flags,
                                                          range);
      break;
    case GRN_VECTOR :
      casted_value = grn_ja_cast_value_vector_var_vector(ctx,
                                                         ja,
                                                         value,
                                                         buffer,
                                                         set_flags,
                                                         range);
      break;
    default :
      invalid_type = true;
      break;
    }
  } else {
    switch (value->header.type) {
    case GRN_BULK :
      casted_value = grn_ja_cast_value_vector_fixed_bulk(ctx,
                                                         ja,
                                                         value,
                                                         buffer,
                                                         set_flags,
                                                         range);
      break;
    case GRN_UVECTOR :
      casted_value = grn_ja_cast_value_vector_fixed_uvector(ctx,
                                                            ja,
                                                            value,
                                                            buffer,
                                                            set_flags,
                                                            range);
      break;
    case GRN_VECTOR :
      casted_value = grn_ja_cast_value_vector_fixed_vector(ctx,
                                                           ja,
                                                           value,
                                                           buffer,
                                                           set_flags,
                                                           range);
      break;
    default :
      invalid_type = true;
      break;
    }
  }
  grn_obj_unref(ctx, range);

  if (invalid_type) {
    grn_ja_cast_value_set_error(ctx,
                                ja,
                                GRN_INVALID_ARGUMENT,
                                value,
                                "[vector]",
                                "value must be GRN_VOID, GRN_BULK, "
                                "GRN_UVECTOR or GRN_VECTOR");
  }

  return casted_value;
}

grn_obj *
grn_ja_cast_value(grn_ctx *ctx,
                  grn_ja *ja,
                  grn_obj *value,
                  grn_obj *buffer,
                  int set_flags)
{
  switch (ja->header->flags & GRN_OBJ_COLUMN_TYPE_MASK) {
  case GRN_OBJ_COLUMN_SCALAR :
    return grn_ja_cast_value_scalar(ctx, ja, value, buffer, set_flags);
  case GRN_OBJ_COLUMN_VECTOR :
    return grn_ja_cast_value_vector(ctx, ja, value, buffer, set_flags);
  default :
    break;
  }

  grn_ja_cast_value_set_error(ctx,
                              ja,
                              GRN_INVALID_ARGUMENT,
                              (grn_obj *)ja,
                              "[uvector]",
                              "invalid column");
  return NULL;
}

grn_rc
grn_ja_pack_value(grn_ctx *ctx,
                  grn_ja *ja,
                  grn_obj *value,
                  int set_flags)
{
  if (value->header.type != GRN_UVECTOR) {
    return GRN_SUCCESS;
  }

  if (!(ja->header->flags & GRN_OBJ_WITH_WEIGHT)) {
    return GRN_SUCCESS;
  }

  if (ja->header->flags & GRN_OBJ_WEIGHT_FLOAT32) {
    return GRN_SUCCESS;
  }

  const char *value_raw = GRN_BULK_HEAD(value);
  uint32_t n = grn_uvector_size(ctx, value);
  size_t element_size = grn_uvector_element_size(ctx, value);
  size_t element_value_size = element_size - sizeof(float);
  uint32_t i;
  for (i = 0; i < n; i++) {
    *((uint32_t *)(value_raw + (element_size * i) + element_value_size)) =
      *((float *)(value_raw + (element_size * i) + element_value_size));
  }

  return GRN_SUCCESS;
}

static grn_rc
grn_ja_unpack_value(grn_ctx *ctx,
                    grn_ja *ja,
                    grn_obj *value,
                    size_t offset)
{
  if (value->header.type != GRN_UVECTOR) {
    return GRN_SUCCESS;
  }

  if (!(ja->header->flags & GRN_OBJ_WITH_WEIGHT)) {
    return GRN_SUCCESS;
  }

  if (ja->header->flags & GRN_OBJ_WEIGHT_FLOAT32) {
    return GRN_SUCCESS;
  }

  const char *value_raw = GRN_BULK_HEAD(value) + offset;
  uint32_t n = grn_uvector_size(ctx, value);
  size_t element_size = grn_uvector_element_size(ctx, value);
  size_t element_value_size = element_size - sizeof(float);
  uint32_t i;
  for (i = 0; i < n; i++) {
    *((float *)(value_raw + (element_size * i) + element_value_size)) =
      *((uint32_t *)(value_raw + (element_size * i) + element_value_size));
  }

  return GRN_SUCCESS;
}

static grn_rc
grn_ja_defrag_seg(grn_ctx *ctx, grn_ja *ja, uint32_t seg)
{
  byte *v = NULL, *ve;
  uint32_t element_size, cum = 0, *seginfo = &SEGMENT_INFO_AT(ja,seg), sum;
  sum = (*seginfo & ~SEG_MASK);
  v = grn_io_seg_ref(ctx, ja->io, seg);
  if (!v) { return GRN_NO_MEMORY_AVAILABLE; }
  ve = v + JA_SEGMENT_SIZE;
  while (v < ve && cum < sum) {
    grn_id id = *((grn_id *)v);
    if (!id) { break; }
    if (id & DELETED) {
      element_size = (id & ~DELETED);
    } else {
      uint64_t cas;
      uint32_t pos;
      if (grn_ja_element_info(ctx, ja, id, &cas, &pos, &element_size)) { break; }
      if (v + sizeof(uint32_t) != ve - JA_SEGMENT_SIZE + pos) {
        GRN_LOG(ctx, GRN_LOG_WARNING,
                "segment_infos[%d] = pos unmatch (%d != %" GRN_FMT_LLD ")",
                seg, pos, (long long int)(v + sizeof(uint32_t) + JA_SEGMENT_SIZE - ve));
        break;
      }
      if (grn_ja_put(ctx, ja, id, v + sizeof(uint32_t), element_size, GRN_OBJ_SET, &cas)) {
        GRN_LOG(ctx, GRN_LOG_WARNING,
                "segment_infos[%d] = put failed (%d)", seg, id);
        break;
      }
      element_size = (element_size + sizeof(grn_id) - 1) & ~(sizeof(grn_id) - 1);
      cum += sizeof(uint32_t) + element_size;
    }
    v += sizeof(uint32_t) + element_size;
  }
  if (*seginfo) {
    GRN_LOG(ctx, GRN_LOG_WARNING, "segment_infos[%d] = %d after defrag", seg, (*seginfo & ~SEG_MASK));
  }
  grn_io_seg_unref(ctx, ja->io, seg);
  return GRN_SUCCESS;
}

int
grn_ja_defrag(grn_ctx *ctx, grn_ja *ja, int threshold)
{
  int nsegs = 0;
  uint32_t seg, ts = 1U << (GRN_JA_W_SEGMENT - threshold);
  for (seg = 0; seg < JA_N_DATA_SEGMENTS; seg++) {
    if (seg == *(ja->header->curr_seg)) { continue; }
    if (((SEGMENT_INFO_AT(ja, seg) & SEG_MASK) == SEG_SEQ) &&
        ((SEGMENT_INFO_AT(ja, seg) & ~SEG_MASK) < ts)) {
      if (!grn_ja_defrag_seg(ctx, ja, seg)) { nsegs++; }
    }
  }
  return nsegs;
}

static bool
grn_ja_check_segment_chunk(grn_ctx *ctx,
                           grn_ja *ja,
                           uint32_t segment,
                           uint32_t info)
{
  GRN_OUTPUT_MAP_OPEN("segment", 4);
  GRN_OUTPUT_CSTR("id");
  GRN_OUTPUT_UINT32(segment);
  GRN_OUTPUT_CSTR("type");
  GRN_OUTPUT_UINT32(SEG_CHUNK >> SEG_TYPE_SHIFT);
  GRN_OUTPUT_CSTR("type_name");
  GRN_OUTPUT_CSTR(grn_ja_segment_info_type_name(ctx, info));
  GRN_OUTPUT_CSTR("variation");
  uint32_t variation = grn_ja_segment_info_value(ctx, info);
  GRN_OUTPUT_UINT32(variation);
  GRN_OUTPUT_MAP_CLOSE();

  return true;
}

static bool
grn_ja_check_segment_seq(grn_ctx *ctx,
                         grn_ja *ja,
                         uint32_t seg,
                         uint32_t data_seg)
{
  GRN_OUTPUT_MAP_OPEN("segment", -1);
  GRN_OUTPUT_CSTR("id");
  GRN_OUTPUT_UINT32(seg);
  GRN_OUTPUT_CSTR("type");
  GRN_OUTPUT_UINT32(SEG_SEQ >> SEG_TYPE_SHIFT);
  GRN_OUTPUT_CSTR("type_name");
  GRN_OUTPUT_CSTR(grn_ja_segment_info_type_name(ctx, data_seg));
  GRN_OUTPUT_CSTR("value");
  GRN_OUTPUT_UINT32(grn_ja_segment_info_value(ctx, data_seg));
  byte *v = NULL, *ve;
  uint32_t element_size, cum = 0, sum = data_seg & ~SEG_MASK;
  uint32_t n_del_elements = 0;
  uint32_t n_elements = 0;
  uint32_t s_del_elements = 0;
  uint32_t s_elements = 0;
  v = grn_io_seg_ref(ctx, ja->io, seg);
  if (v) {
    /*
      GRN_OUTPUT_CSTR("seg seq");
      GRN_OUTPUT_ARRAY_OPEN("SEQ", -1);
    */
    ve = v + JA_SEGMENT_SIZE;
    while (v < ve && cum < sum) {
      grn_id id = *((grn_id *)v);
      /*
        GRN_OUTPUT_MAP_OPEN("ENTRY", -1);
        GRN_OUTPUT_CSTR("id");
        GRN_OUTPUT_INT64(id);
      */
      if (!id) { break; }
      if (id & DELETED) {
        element_size = (id & ~DELETED);
        n_del_elements++;
        s_del_elements += element_size;
      } else {
        element_size = grn_ja_size(ctx, ja, id);
        element_size = (element_size + sizeof(grn_id) - 1) & ~(sizeof(grn_id) - 1);
        cum += sizeof(uint32_t) + element_size;
        n_elements++;
        s_elements += sizeof(uint32_t) + element_size;
      }
      v += sizeof(uint32_t) + element_size;
      /*
        GRN_OUTPUT_CSTR("size");
        GRN_OUTPUT_INT64(element_size);
        GRN_OUTPUT_CSTR("cum");
        GRN_OUTPUT_INT64(cum);
        GRN_OUTPUT_MAP_CLOSE();
      */
    }
    grn_io_seg_unref(ctx, ja->io, seg);
    /*
      GRN_OUTPUT_ARRAY_CLOSE();
    */
    GRN_OUTPUT_CSTR("n_elements");
    GRN_OUTPUT_INT64(n_elements);
    GRN_OUTPUT_CSTR("s_elements");
    GRN_OUTPUT_INT64(s_elements);
    GRN_OUTPUT_CSTR("n_del_elements");
    GRN_OUTPUT_INT64(n_del_elements);
    GRN_OUTPUT_CSTR("s_del_elements");
    GRN_OUTPUT_INT64(s_del_elements);
    if (cum != sum) {
      GRN_OUTPUT_CSTR("cum gap");
      GRN_OUTPUT_INT64(cum - sum);
    }
  }
  GRN_OUTPUT_MAP_CLOSE();

  return true;
}

static bool
grn_ja_check_segment_einfo_validate(grn_ctx *ctx,
                                    grn_ja *ja,
                                    uint32_t segment,
                                    uint32_t element_info_index,
                                    uint32_t referred_segment)
{
  const char *tag = "[ja][check][segment][einfo]";
  if (referred_segment != segment) {
    GRN_DEFINE_NAME(ja);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s[%.*s][%u] header->element_segs[%u] refers wrong segment: "
            "referred_segment:%u",
            tag,
            name_size, name,
            segment,
            element_info_index,
            referred_segment);
    return false;
  }

  grn_ja_einfo *einfo = grn_io_seg_ref(ctx, ja->io, segment);
  if (!einfo) {
    GRN_DEFINE_NAME(ja);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s[%.*s][%u] failed to refer element info segment",
            tag,
            name_size, name,
            segment);
    return false;
  }

  bool is_valid = true;
  uint32_t i;
  for (i = 0; i < JA_N_EINFO_IN_A_SEGMENT; i++) {
    grn_id record_id = (element_info_index << JA_W_EINFO_IN_A_SEGMENT) + i;
    grn_ja_einfo *ei = &(einfo[i]);
    if (ETINY_P(ei)) {
      continue;
    }
    if (EHUGE_P(ei)) {
      uint32_t data_start_segment;
      uint32_t size;
      EHUGE_DEC(ei, data_start_segment, size);
      uint32_t n_segments = (size + JA_SEGMENT_SIZE - 1) >> GRN_JA_W_SEGMENT;
      uint32_t j;
      for (j = 0; j < n_segments; j++) {
        uint32_t info = SEGMENT_INFO_AT(ja, data_start_segment + j);
        uint32_t segment_type = grn_ja_segment_info_type(ctx, info);
        if (segment_type != SEG_HUGE) {
          GRN_DEFINE_NAME(ja);
          GRN_LOG(ctx,
                  GRN_LOG_ERROR,
                  "%s[%.*s][%u] segment must be huge type: "
                  "record_id:%u, "
                  "type:%u, "
                  "type_name:<%s>, "
                  "data_segment:%u, "
                  "element_size:%u, "
                  "n_segments:%u, ",
                  tag,
                  name_size, name,
                  segment,
                  record_id,
                  segment_type >> SEG_TYPE_SHIFT,
                  grn_ja_segment_info_type_name(ctx, info),
                  data_start_segment + j,
                  size,
                  n_segments);
          is_valid = false;
          break;
        }
      }
    } else {
      uint32_t data_segment;
      uint32_t position;
      uint32_t size;
      EINFO_DEC(ei, data_segment, position, size);
      if (size == 0) {
        continue;
      }
      if (size < 8) {
        GRN_DEFINE_NAME(ja);
        GRN_LOG(ctx,
                GRN_LOG_ERROR,
                "%s[%.*s][%u] the element must be tiny type: "
                "record_id:%u, "
                "data_segment:%u, "
                "position:%u, "
                "element_size:%u",
                tag,
                name_size, name,
                segment,
                record_id,
                data_segment,
                position,
                size);
        is_valid = false;
        continue;
      }
      uint32_t metadata = SEGMENT_INFO_AT(ja, data_segment);
      uint32_t segment_type = grn_ja_segment_info_type(ctx, metadata);
      if (segment_type != SEG_CHUNK) {
        GRN_DEFINE_NAME(ja);
        GRN_LOG(ctx,
                GRN_LOG_ERROR,
                "%s[%.*s][%u] segment must be chunk type: "
                "record_id:%u, "
                "type:%u, "
                "type_name:<%s>, "
                "data_segment:%u, "
                "position:%u, "
                "element_size:%u",
                tag,
                name_size, name,
                segment,
                record_id,
                segment_type >> SEG_TYPE_SHIFT,
                grn_ja_segment_info_type_name(ctx, metadata),
                data_segment,
                position,
                size);
        is_valid = false;
        continue;
      }
      uint32_t variation;
      {
        uint32_t s = size - 1;
        GRN_BIT_SCAN_REV(s, variation);
        variation++;
      }
      if (variation > ja->header->chunk_threshold) {
        /* TODO */
      } else {
        uint32_t aligned_size = 1U << variation;
        uint32_t max_position = JA_SEGMENT_SIZE - aligned_size;
        if (position > max_position) {
          GRN_DEFINE_NAME(ja);
          GRN_LOG(ctx,
                  GRN_LOG_ERROR,
                  "%s[%.*s][%u] out of range position: "
                  "record_id:%u, "
                  "variation:%u, "
                  "position:%u, "
                  "max_position:%u, "
                  "element_size:%u",
                  tag,
                  name_size, name,
                  segment,
                  record_id,
                  variation,
                  position,
                  max_position,
                  size);
          is_valid = false;
          continue;
        }
      }
    }
  }
  grn_io_seg_unref(ctx, ja->io, segment);
  return is_valid;
}

static bool
grn_ja_check_segment_einfo(grn_ctx *ctx,
                           grn_ja *ja,
                           uint32_t seg,
                           uint32_t info)
{
  GRN_OUTPUT_MAP_OPEN("segment", 6);
  GRN_OUTPUT_CSTR("id");
  GRN_OUTPUT_UINT32(seg);
  GRN_OUTPUT_CSTR("type");
  GRN_OUTPUT_UINT32(SEG_EINFO >> SEG_TYPE_SHIFT);
  GRN_OUTPUT_CSTR("type_name");
  GRN_OUTPUT_CSTR(grn_ja_segment_info_type_name(ctx, info));
  GRN_OUTPUT_CSTR("element_info_index");
  uint32_t element_info_index = grn_ja_segment_info_value(ctx, info);
  GRN_OUTPUT_UINT32(element_info_index);
  uint32_t referred_segment = ja->header->element_segs[element_info_index];
  GRN_OUTPUT_CSTR("referred_segment");
  GRN_OUTPUT_UINT32(referred_segment);
  GRN_OUTPUT_CSTR("valid");
  bool is_valid =
    grn_ja_check_segment_einfo_validate(ctx,
                                        ja,
                                        seg,
                                        element_info_index,
                                        referred_segment);
  GRN_OUTPUT_BOOL(is_valid);
  GRN_OUTPUT_MAP_CLOSE();

  return is_valid;
}

static bool
grn_ja_check_segment_ginfo_validate(grn_ctx *ctx,
                                    grn_ja *ja,
                                    uint32_t segment,
                                    uint32_t variation,
                                    grn_ja_ginfo *ginfo,
                                    const char *tag)
{
  bool is_valid = true;
  uint32_t n_existing_records = 0;
  uint32_t i;
  for (i = ginfo->tail;
       i != ginfo->head;
       i = ((i + 1) % JA_N_GARBAGES_IN_A_SEGMENT)) {
    n_existing_records++;
    uint32_t garbage_segment = ginfo->recs[i].seg;
    uint32_t garbage_position = ginfo->recs[i].pos;
    if (garbage_segment >= JA_N_DATA_SEGMENTS) {
      GRN_DEFINE_NAME(ja);
      GRN_LOG(ctx,
              GRN_LOG_ERROR,
              "%s[%.*s][%u] out of range garbage segment: "
              "variation:%u, "
              "index:%u, "
              "garbage_segment:%u",
              tag,
              name_size, name,
              segment,
              variation,
              i,
              garbage_segment);
      is_valid = false;
      continue;
    }
    uint32_t garbage_segment_info = SEGMENT_INFO_AT(ja, garbage_segment);
    uint32_t garbage_segment_type =
      grn_ja_segment_info_type(ctx, garbage_segment_info);
    if (garbage_segment_type != SEG_CHUNK) {
      GRN_DEFINE_NAME(ja);
      GRN_LOG(ctx,
              GRN_LOG_ERROR,
              "%s[%.*s][%u] segment that has garbage must be chunk type: "
              "variation:%u, "
              "index:%u, "
              "garbage_segment:%u, "
              "garbage_segment_type:%u, "
              "garbage_segment_type_name:<%s>",
              tag,
              name_size, name,
              segment,
              variation,
              i,
              garbage_segment,
              garbage_segment_type >> SEG_TYPE_SHIFT,
              grn_ja_segment_info_type_name(ctx, garbage_segment_info));
      is_valid = false;
      continue;
    }
    uint32_t aligned_size = 1U << variation;
    uint32_t max_position = JA_SEGMENT_SIZE - aligned_size;
    if (garbage_position > max_position) {
      GRN_DEFINE_NAME(ja);
      GRN_LOG(ctx,
              GRN_LOG_ERROR,
              "%s[%.*s][%u] out of range garbage position: "
              "variation:%u, "
              "index:%u, "
              "garbage_segment:%u, "
              "garbage_position:%u, "
              "max_position:%u",
              tag,
              name_size, name,
              segment,
              variation,
              i,
              garbage_segment,
              garbage_position,
              max_position);
      is_valid = false;
      continue;
    }
  }
  if (ginfo->nrecs != n_existing_records) {
    GRN_DEFINE_NAME(ja);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s[%.*s][%u] inconsistent the number of records: "
            "variation:%u, "
            "n_records:%u, "
            "real_n_records:%u",
            tag,
            name_size, name,
            segment,
            variation,
            ginfo->nrecs,
            n_existing_records);
    is_valid = false;
  }
  if (ginfo->next != 0) {
    uint32_t next_segment_info = SEGMENT_INFO_AT(ja, ginfo->next);
    uint32_t next_segment_type =
      grn_ja_segment_info_type(ctx, next_segment_info);
    if (next_segment_type != SEG_GINFO) {
      GRN_DEFINE_NAME(ja);
      GRN_LOG(ctx,
              GRN_LOG_ERROR,
              "%s[%.*s][%u] next segment must be ginfo type: "
              "variation:%u, "
              "next_segment:%u, "
              "next_segment_type:%u, "
              "next_segment_type_name:<%s>",
              tag,
              name_size, name,
              segment,
              variation,
              ginfo->next,
              next_segment_type >> SEG_TYPE_SHIFT,
              grn_ja_segment_info_type_name(ctx, next_segment_info));
      is_valid = false;
    }
  }
  return is_valid;
}

static bool
grn_ja_check_segment_ginfo(grn_ctx *ctx,
                           grn_ja *ja,
                           uint32_t segment,
                           uint32_t info)
{
  const char *tag = "[ja][check][segment][ginfo]";
  GRN_OUTPUT_MAP_OPEN("segment", 9);
  GRN_OUTPUT_CSTR("id");
  GRN_OUTPUT_UINT32(segment);
  GRN_OUTPUT_CSTR("type");
  GRN_OUTPUT_UINT32(SEG_GINFO >> SEG_TYPE_SHIFT);
  GRN_OUTPUT_CSTR("type_name");
  GRN_OUTPUT_CSTR(grn_ja_segment_info_type_name(ctx, info));
  GRN_OUTPUT_CSTR("variation");
  uint32_t variation = grn_ja_segment_info_value(ctx, info) + JA_W_EINFO;
  GRN_OUTPUT_UINT32(variation);

  uint32_t head = 0;
  uint32_t tail = 0;
  uint32_t n_garbages = 0;
  uint32_t next = 0;
  bool is_valid = false;
  grn_ja_ginfo *ginfo = grn_io_seg_ref(ctx, ja->io, segment);
  if (ginfo) {
    head = ginfo->head;
    tail = ginfo->tail;
    n_garbages = ginfo->nrecs;
    next = ginfo->next;
    is_valid = grn_ja_check_segment_ginfo_validate(ctx,
                                                   ja,
                                                   segment,
                                                   variation,
                                                   ginfo,
                                                   tag);
  } else {
    GRN_DEFINE_NAME(ja);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s[%.*s][%u] failed to refer garbage info segment",
            tag,
            name_size, name,
            segment);
  }

  GRN_OUTPUT_CSTR("head");
  GRN_OUTPUT_UINT32(head);
  GRN_OUTPUT_CSTR("tail");
  GRN_OUTPUT_UINT32(tail);
  GRN_OUTPUT_CSTR("n_garbages");
  GRN_OUTPUT_UINT32(n_garbages);
  GRN_OUTPUT_CSTR("next");
  GRN_OUTPUT_UINT32(next);
  GRN_OUTPUT_CSTR("valid");
  GRN_OUTPUT_BOOL(is_valid);

  if (ginfo) {
    grn_io_seg_unref(ctx, ja->io, segment);
  }

  GRN_OUTPUT_MAP_CLOSE();

  return is_valid;
}

static bool
grn_ja_check_segment(grn_ctx *ctx,
                     grn_ja *ja,
                     uint32_t seg,
                     uint32_t info)
{
  uint32_t seg_type = grn_ja_segment_info_type(ctx, info);
  switch (seg_type) {
  case SEG_CHUNK :
    return grn_ja_check_segment_chunk(ctx, ja, seg, info);
  case SEG_SEQ :
    return grn_ja_check_segment_seq(ctx, ja, seg, info);
  case SEG_EINFO :
    return grn_ja_check_segment_einfo(ctx, ja, seg, info);
  case SEG_GINFO :
    return grn_ja_check_segment_ginfo(ctx, ja, seg, info);
  default :
    break;
  }
  int32_t n_elements = 4;
  GRN_OUTPUT_MAP_OPEN("segment", n_elements);
  GRN_OUTPUT_CSTR("id");
  GRN_OUTPUT_UINT32(seg);
  GRN_OUTPUT_CSTR("type");
  GRN_OUTPUT_UINT32(seg_type >> SEG_TYPE_SHIFT);
  GRN_OUTPUT_CSTR("type_name");
  GRN_OUTPUT_CSTR(grn_ja_segment_info_type_name(ctx, info));
  GRN_OUTPUT_CSTR("value");
  GRN_OUTPUT_UINT32(grn_ja_segment_info_value(ctx, info));
  GRN_OUTPUT_MAP_CLOSE();
  return true;
}

static bool
grn_ja_check_free_element_validate(grn_ctx *ctx,
                                   grn_ja *ja,
                                   uint32_t variation,
                                   ja_pos *start)
{
  const char *tag = "[ja][check][free-element]";
  uint32_t info = SEGMENT_INFO_AT(ja, start->seg);
  uint32_t segment_type = grn_ja_segment_info_type(ctx, info);
  if (segment_type != SEG_CHUNK) {
    GRN_DEFINE_NAME(ja);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s[%.*s] free element segment must be chunk segment: "
            "variation:%u, "
            "free_segment:%u, "
            "free_segment_type:%u, "
            "free_segment_type_name:<%s>",
            tag,
            name_size, name,
            variation,
            start->seg,
            segment_type >> SEG_TYPE_SHIFT,
            grn_ja_segment_info_type_name(ctx, info));
    return false;
  }

  uint32_t aligned_size = 1U << variation;
  uint32_t max_position = JA_SEGMENT_SIZE - aligned_size;
  if (start->pos > max_position) {
    GRN_DEFINE_NAME(ja);
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s[%.*s] out of range free element position: "
            "variation:%u, "
            "free_segment:%u, "
            "free_segment_position:%u, "
            "max_position:%u",
            tag,
            name_size, name,
            variation,
            start->seg,
            start->pos,
            max_position);
    return false;
  }

  return true;
}

void
grn_ja_check(grn_ctx *ctx, grn_ja *ja)
{
  GRN_OUTPUT_MAP_OPEN("result", 2);
  uint32_t n_using_segments = 0;
  {
    uint32_t seg;
    for (seg = 0; seg < JA_N_DATA_SEGMENTS; seg++) {
      int info = SEGMENT_INFO_AT(ja, seg);
      if (info != 0) {
        n_using_segments++;
      }
    }
  }
  bool is_valid = true;
  GRN_OUTPUT_CSTR("details");
  {
    GRN_OUTPUT_MAP_OPEN("details", 3);
    GRN_OUTPUT_CSTR("segments");
    {
      GRN_OUTPUT_ARRAY_OPEN("segments", n_using_segments);
      uint32_t seg;
      for (seg = 0; seg < JA_N_DATA_SEGMENTS; seg++) {
        uint32_t info = SEGMENT_INFO_AT(ja, seg);
        if (info != 0) {
          if (!grn_ja_check_segment(ctx, ja, seg, info)) {
            is_valid = false;
          }
        }
      }
      GRN_OUTPUT_ARRAY_CLOSE();
    }

    GRN_OUTPUT_CSTR("garbage_counts");
    {
      uint32_t n_variantions = 0;
      uint32_t i;
      for (i = 0; i < ja->header->n_element_variations; i++) {
        if (ja->header->n_garbages[i] == 0) {
          continue;
        }
        n_variantions++;
      }
      {
        GRN_OUTPUT_MAP_OPEN("garbage_counts", n_variantions);
        for (i = 0; i < ja->header->n_element_variations; i++) {
          if (ja->header->n_garbages[i] == 0) {
            continue;
          }
          uint32_t variation = i + JA_W_EINFO;
          grn_obj variation_string;
          GRN_TEXT_INIT(&variation_string, 0);
          grn_text_printf(ctx, &variation_string, "%u", variation);
          GRN_OUTPUT_STR(GRN_TEXT_VALUE(&variation_string),
                         GRN_TEXT_LEN(&variation_string));
          GRN_OBJ_FIN(ctx, &variation_string);
          uint32_t real_total = 0;
          {
            GRN_OUTPUT_MAP_OPEN("variation", 3);
            GRN_OUTPUT_CSTR("total");
            GRN_OUTPUT_UINT32(ja->header->n_garbages[i]);
            uint32_t n_segments = 0;
            uint32_t segment;
            for (segment = 0; segment < JA_N_DATA_SEGMENTS; segment++) {
              uint32_t info = SEGMENT_INFO_AT(ja, segment);
              if (grn_ja_segment_info_type(ctx, info) != SEG_GINFO) {
                continue;
              }
              n_segments++;
            }
            GRN_OUTPUT_CSTR("details");
            {
              GRN_OUTPUT_MAP_OPEN("segment", n_segments);
              for (segment = 0; segment < JA_N_DATA_SEGMENTS; segment++) {
                uint32_t info = SEGMENT_INFO_AT(ja, segment);
                if (grn_ja_segment_info_type(ctx, info) != SEG_GINFO) {
                  continue;
                }
                uint32_t v = grn_ja_segment_info_value(ctx, info) + JA_W_EINFO;
                if (v != variation) {
                  continue;
                }
                grn_obj segment_string;
                GRN_TEXT_INIT(&segment_string, 0);
                grn_text_printf(ctx, &segment_string, "%u", segment);
                GRN_OUTPUT_STR(GRN_TEXT_VALUE(&segment_string),
                               GRN_TEXT_LEN(&segment_string));
                GRN_OBJ_FIN(ctx, &segment_string);
                grn_ja_ginfo *ginfo = grn_io_seg_ref(ctx, ja->io, segment);
                if (ginfo) {
                  real_total += ginfo->nrecs;
                  GRN_OUTPUT_UINT32(ginfo->nrecs);
                  grn_io_seg_unref(ctx, ja->io, segment);
                } else {
                  GRN_OUTPUT_INT32(-1);
                }
              }
              GRN_OUTPUT_MAP_CLOSE();
            }
            GRN_OUTPUT_CSTR("valid");
            bool is_valid_n_garbages = (ja->header->n_garbages[i] == real_total);
            if (!is_valid_n_garbages) {
              is_valid = false;
            }
            GRN_OUTPUT_BOOL(is_valid_n_garbages);
            GRN_OUTPUT_MAP_CLOSE();
          }
        }
        GRN_OUTPUT_MAP_CLOSE();
      }
    }

    GRN_OUTPUT_CSTR("free_elements");
    {
      uint32_t n_variantions = 0;
      uint32_t i;
      for (i = 0; i < ja->header->n_element_variations; i++) {
        if (ja->header->free_elements[i].seg == 0) {
          continue;
        }
        n_variantions++;
      }
      {
        GRN_OUTPUT_MAP_OPEN("free_elements", n_variantions);
        for (i = 0; i < ja->header->n_element_variations; i++) {
          ja_pos start = ja->header->free_elements[i];
          if (start.seg == 0) {
            continue;
          }
          uint32_t variation = i + JA_W_EINFO;
          grn_obj variation_string;
          GRN_TEXT_INIT(&variation_string, 0);
          grn_text_printf(ctx, &variation_string, "%u", variation);
          GRN_OUTPUT_STR(GRN_TEXT_VALUE(&variation_string),
                         GRN_TEXT_LEN(&variation_string));
          GRN_OBJ_FIN(ctx, &variation_string);
          {
            GRN_OUTPUT_MAP_OPEN("start", 3);
            GRN_OUTPUT_CSTR("segment");
            GRN_OUTPUT_UINT32(start.seg);
            GRN_OUTPUT_CSTR("position");
            GRN_OUTPUT_UINT32(start.pos);
            GRN_OUTPUT_CSTR("valid");
            bool is_valid_free_element =
              grn_ja_check_free_element_validate(ctx,
                                                 ja,
                                                 variation,
                                                 &start);
            if (!is_valid_free_element) {
              is_valid = false;
            }
            GRN_OUTPUT_BOOL(is_valid_free_element);
            GRN_OUTPUT_MAP_CLOSE();
          }
        }
        GRN_OUTPUT_MAP_CLOSE();
      }
    }
    GRN_OUTPUT_MAP_CLOSE();
  }

  GRN_OUTPUT_CSTR("summary");
  {
    struct grn_ja_header *h = ja->header;
    char buf[8];
    GRN_OUTPUT_MAP_OPEN("summary", 8);
    GRN_OUTPUT_CSTR("flags");
    grn_itoh(h->flags, buf, 8);
    GRN_OUTPUT_STR(buf, 8);
    GRN_OUTPUT_CSTR("curr seg");
    GRN_OUTPUT_INT64(*(h->curr_seg));
    GRN_OUTPUT_CSTR("curr pos");
    GRN_OUTPUT_INT64(*(h->curr_pos));
    GRN_OUTPUT_CSTR("max_element_size");
    GRN_OUTPUT_INT64(h->max_element_size);
    GRN_OUTPUT_CSTR("chunk_threshold");
    GRN_OUTPUT_INT64(h->chunk_threshold);
    GRN_OUTPUT_CSTR("n_element_variations");
    GRN_OUTPUT_INT64(h->n_element_variations);
    GRN_OUTPUT_CSTR("n_using_segments");
    GRN_OUTPUT_UINT32(n_using_segments);
    GRN_OUTPUT_CSTR("valid");
    GRN_OUTPUT_BOOL(is_valid);
    GRN_OUTPUT_MAP_CLOSE();
  }
  GRN_OUTPUT_MAP_CLOSE();
}

grn_rc
grn_ja_warm(grn_ctx *ctx, grn_ja *ja)
{
  return grn_io_warm(ctx, ja->io);
}

/* grn_ja_reader */

grn_rc
grn_ja_reader_init(grn_ctx *ctx, grn_ja_reader *reader, grn_ja *ja)
{
  reader->ja = ja;
  reader->einfo_seg_id = JA_ELEMENT_SEG_VOID;
  reader->ref_avail = GRN_FALSE;
  reader->ref_seg_id = JA_ELEMENT_SEG_VOID;
  reader->ref_seg_ids = NULL;
  reader->nref_seg_ids = 0;
  reader->ref_seg_ids_size = 0;
  reader->body_seg_id = JA_ELEMENT_SEG_VOID;
  reader->body_seg_addr = NULL;
  reader->packed_buf = NULL;
  reader->packed_buf_size = 0;
#ifdef GRN_WITH_ZLIB
  if (reader->ja->header->flags & GRN_OBJ_COMPRESS_ZLIB) {
    z_stream *new_stream = GRN_MALLOCN(z_stream, 1);
    if (!new_stream) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    new_stream->zalloc = NULL;
    new_stream->zfree = NULL;
    new_stream->opaque = NULL;
    if (inflateInit2(new_stream, 15) != Z_OK) {
      GRN_FREE(new_stream);
      return GRN_ZLIB_ERROR;
    }
    reader->stream = new_stream;
  }
#endif /* GRN_WITH_ZLIB */
  return GRN_SUCCESS;
}

grn_rc
grn_ja_reader_fin(grn_ctx *ctx, grn_ja_reader *reader)
{
  grn_rc rc = GRN_SUCCESS;
  if (reader->einfo_seg_id != JA_ELEMENT_SEG_VOID) {
    grn_io_seg_unref(ctx, reader->ja->io, reader->einfo_seg_id);
  }
  if (reader->ref_seg_ids) {
    grn_ja_reader_unref(ctx, reader);
    GRN_FREE(reader->ref_seg_ids);
  }
  if (reader->body_seg_addr) {
    grn_io_seg_unref(ctx, reader->ja->io, reader->body_seg_id);
  }
  if (reader->packed_buf) {
    GRN_FREE(reader->packed_buf);
  }
#ifdef GRN_WITH_ZLIB
  if (reader->ja->header->flags & GRN_OBJ_COMPRESS_ZLIB) {
    if (reader->stream) {
      if (inflateEnd((z_stream *)reader->stream) != Z_OK) {
        rc = GRN_UNKNOWN_ERROR;
      }
      GRN_FREE(reader->stream);
    }
  }
#endif /* GRN_WITH_ZLIB */
  return rc;
}

grn_rc
grn_ja_reader_open(grn_ctx *ctx, grn_ja *ja, grn_ja_reader **reader)
{
  grn_rc rc;
  grn_ja_reader *new_reader = GRN_MALLOCN(grn_ja_reader, 1);
  if (!new_reader) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  rc = grn_ja_reader_init(ctx, new_reader, ja);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_reader);
    return rc;
  }
  *reader = new_reader;
  return GRN_SUCCESS;
}

grn_rc
grn_ja_reader_close(grn_ctx *ctx, grn_ja_reader *reader)
{
  grn_rc rc = grn_ja_reader_fin(ctx, reader);
  GRN_FREE(reader);
  return rc;
}

#ifdef GRN_WITH_COMPRESSED
/* grn_ja_reader_seek_compressed() prepares to access a compressed value. */
static grn_rc
grn_ja_reader_seek_compressed(grn_ctx *ctx, grn_ja_reader *reader, grn_id id)
{
  grn_ja_einfo *einfo;
  void *seg_addr;
  uint32_t seg_id = reader->ja->header->element_segs[id >> JA_W_EINFO_IN_A_SEGMENT];
  if (seg_id == JA_ELEMENT_SEG_VOID) {
    return GRN_INVALID_ARGUMENT;
  }
  if (seg_id != reader->einfo_seg_id) {
    seg_addr = grn_io_seg_ref(ctx, reader->ja->io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    if (reader->einfo_seg_id != JA_ELEMENT_SEG_VOID) {
      grn_io_seg_unref(ctx, reader->ja->io, reader->einfo_seg_id);
    }
    reader->einfo_seg_id = seg_id;
    reader->einfo_seg_addr = seg_addr;
  }
  einfo = (grn_ja_einfo *)reader->einfo_seg_addr;
  einfo += id & JA_M_EINFO_IN_A_SEGMENT;
  reader->einfo = einfo;
  /* ETINY_P(einfo) is always false because the original size needs 8 bytes. */
  if (EHUGE_P(einfo)) {
    EHUGE_DEC(einfo, seg_id, reader->packed_size);
    reader->body_seg_offset = 0;
  } else {
    EINFO_DEC(einfo, seg_id, reader->body_seg_offset, reader->packed_size);
  }
  if (seg_id != reader->body_seg_id) {
    seg_addr = grn_io_seg_ref(ctx, reader->ja->io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    if (reader->body_seg_addr) {
      grn_io_seg_unref(ctx, reader->ja->io, reader->body_seg_id);
    }
    reader->body_seg_id = seg_id;
    reader->body_seg_addr = seg_addr;
  }
  seg_addr = (char *)reader->body_seg_addr + reader->body_seg_offset;
  reader->value_size = (uint32_t)*(uint64_t *)seg_addr;
  return GRN_SUCCESS;
}
#endif /* GRN_WITH_COMPRESSED */

/* grn_ja_reader_seek_raw() prepares to access a value. */
static grn_rc
grn_ja_reader_seek_raw(grn_ctx *ctx, grn_ja_reader *reader, grn_id id)
{
  grn_ja_einfo *einfo;
  void *seg_addr;
  uint32_t seg_id = reader->ja->header->element_segs[id >> JA_W_EINFO_IN_A_SEGMENT];
  if (seg_id == JA_ELEMENT_SEG_VOID) {
    return GRN_INVALID_ARGUMENT;
  }
  if (seg_id != reader->einfo_seg_id) {
    seg_addr = grn_io_seg_ref(ctx, reader->ja->io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    if (reader->einfo_seg_id != JA_ELEMENT_SEG_VOID) {
      grn_io_seg_unref(ctx, reader->ja->io, reader->einfo_seg_id);
    }
    reader->einfo_seg_id = seg_id;
    reader->einfo_seg_addr = seg_addr;
  }
  einfo = (grn_ja_einfo *)reader->einfo_seg_addr;
  einfo += id & JA_M_EINFO_IN_A_SEGMENT;
  reader->einfo = einfo;
  if (ETINY_P(einfo)) {
    ETINY_DEC(einfo, reader->value_size);
    reader->ref_avail = GRN_FALSE;
  } else {
    if (EHUGE_P(einfo)) {
      EHUGE_DEC(einfo, seg_id, reader->value_size);
      reader->ref_avail = GRN_FALSE;
    } else {
      EINFO_DEC(einfo, seg_id, reader->body_seg_offset, reader->value_size);
      reader->ref_avail = GRN_TRUE;
    }
    if (reader->body_seg_addr) {
      if (seg_id != reader->body_seg_id) {
        grn_io_seg_unref(ctx, reader->ja->io, reader->body_seg_id);
        reader->body_seg_addr = NULL;
      }
    }
    reader->body_seg_id = seg_id;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ja_reader_seek(grn_ctx *ctx, grn_ja_reader *reader, grn_id id)
{
  switch (reader->ja->header->flags & GRN_OBJ_COMPRESS_MASK) {
#ifdef GRN_WITH_ZLIB
  case GRN_OBJ_COMPRESS_ZLIB :
    return grn_ja_reader_seek_compressed(ctx, reader, id);
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_OBJ_COMPRESS_LZ4 :
    return grn_ja_reader_seek_compressed(ctx, reader, id);
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_OBJ_COMPRESS_ZSTD :
    return grn_ja_reader_seek_compressed(ctx, reader, id);
#endif /* GRN_WITH_ZSTD */
  default :
    return grn_ja_reader_seek_raw(ctx, reader, id);
  }
}

grn_rc
grn_ja_reader_ref(grn_ctx *ctx, grn_ja_reader *reader, void **addr)
{
  if (!reader->ref_avail) {
    return GRN_INVALID_ARGUMENT;
  }
  if (reader->body_seg_id != reader->ref_seg_id) {
    void *seg_addr;
    if (reader->nref_seg_ids == reader->ref_seg_ids_size) {
      size_t n_bytes;
      uint32_t new_size, *new_seg_ids;
      if (reader->ref_seg_ids_size == 0) {
        new_size = GRN_JA_READER_INITIAL_REF_SEG_IDS_SIZE;
      } else {
        new_size = reader->ref_seg_ids_size * 2;
      }
      n_bytes = sizeof(uint32_t) * new_size;
      new_seg_ids = (uint32_t *)GRN_REALLOC(reader->ref_seg_ids, n_bytes);
      if (!new_seg_ids) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      reader->ref_seg_ids = new_seg_ids;
      reader->ref_seg_ids_size = new_size;
    }
    seg_addr = grn_io_seg_ref(ctx, reader->ja->io, reader->body_seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    reader->ref_seg_id = reader->body_seg_id;
    reader->ref_seg_addr = seg_addr;
    reader->ref_seg_ids[reader->nref_seg_ids++] = reader->body_seg_id;
  }
  *addr = (char *)reader->ref_seg_addr + reader->body_seg_offset;
  return GRN_SUCCESS;
}

grn_rc
grn_ja_reader_unref(grn_ctx *ctx, grn_ja_reader *reader)
{
  uint32_t i;
  for (i = 0; i < reader->nref_seg_ids; i++) {
    grn_io_seg_unref(ctx, reader->ja->io, reader->ref_seg_ids[i]);
  }
  reader->ref_seg_id = JA_ELEMENT_SEG_VOID;
  reader->nref_seg_ids = 0;
  return GRN_FUNCTION_NOT_IMPLEMENTED;
}

#ifdef GRN_WITH_ZLIB
/* grn_ja_reader_read_zlib() reads a value compressed with zlib. */
static grn_rc
grn_ja_reader_read_zlib(grn_ctx *ctx, grn_ja_reader *reader, void *buf)
{
  uLong dest_size = reader->value_size;
  z_stream *stream = (z_stream *)reader->stream;
  grn_ja_einfo *einfo = (grn_ja_einfo *)reader->einfo;
  if (EHUGE_P(einfo)) {
    /* TODO: Use z_stream to avoid copy. */
    grn_io *io = reader->ja->io;
    void *seg_addr;
    char *packed_ptr;
    uint32_t size, seg_id;
    if (reader->packed_size > reader->packed_buf_size) {
      void *new_buf = GRN_REALLOC(reader->packed_buf, reader->packed_size);
      if (!new_buf) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      reader->packed_buf = new_buf;
      reader->packed_buf_size = reader->packed_size;
    }
    packed_ptr = (char *)reader->packed_buf;
    grn_memcpy(packed_ptr, (char *)reader->body_seg_addr + sizeof(uint64_t),
               io->header->segment_size - sizeof(uint64_t));
    packed_ptr += io->header->segment_size - sizeof(uint64_t);
    size = reader->packed_size - (io->header->segment_size - sizeof(uint64_t));
    seg_id = reader->body_seg_id + 1;
    while (size > io->header->segment_size) {
      seg_addr = grn_io_seg_ref(ctx, io, seg_id);
      if (!seg_addr) {
        return GRN_UNKNOWN_ERROR;
      }
      grn_memcpy(packed_ptr, seg_addr, io->header->segment_size);
      grn_io_seg_unref(ctx, io, seg_id);
      seg_id++;
      size -= io->header->segment_size;
      packed_ptr += io->header->segment_size;
    }
    seg_addr = grn_io_seg_ref(ctx, io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    grn_memcpy(packed_ptr, seg_addr, size);
    grn_io_seg_unref(ctx, io, seg_id);
    seg_id++;
    if (uncompress((Bytef *)buf, &dest_size, (Bytef *)reader->packed_buf,
                   reader->packed_size - sizeof(uint64_t)) != Z_OK) {
      return GRN_ZLIB_ERROR;
    }
    if (dest_size != reader->value_size) {
      return GRN_ZLIB_ERROR;
    }
  } else {
    char *packed_addr = (char *)reader->body_seg_addr;
    packed_addr += reader->body_seg_offset + sizeof(uint64_t);
    if (inflateReset(stream) != Z_OK) {
      return GRN_ZLIB_ERROR;
    }
    stream->next_in = (Bytef *)packed_addr;
    stream->avail_in = reader->packed_size - sizeof(uint64_t);
    stream->next_out = (Bytef *)buf;
    stream->avail_out = dest_size;
    if ((inflate(stream, Z_FINISH) != Z_STREAM_END) || stream->avail_out) {
      return GRN_ZLIB_ERROR;
    }
  }
  return GRN_SUCCESS;
}
#endif /* GRN_WITH_ZLIB */

#ifdef GRN_WITH_LZ4
/* grn_ja_reader_read_lz4() reads a value compressed with LZ4. */
static grn_rc
grn_ja_reader_read_lz4(grn_ctx *ctx, grn_ja_reader *reader, void *buf)
{
  int src_size, dest_size;
  grn_ja_einfo *einfo = (grn_ja_einfo *)reader->einfo;
  if (EHUGE_P(einfo)) {
    grn_io *io = reader->ja->io;
    void *seg_addr;
    char *packed_ptr;
    uint32_t size, seg_id;
    if (reader->packed_size > reader->packed_buf_size) {
      void *new_buf = GRN_REALLOC(reader->packed_buf, reader->packed_size);
      if (!new_buf) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      reader->packed_buf = new_buf;
      reader->packed_buf_size = reader->packed_size;
    }
    packed_ptr = (char *)reader->packed_buf;
    grn_memcpy(packed_ptr, (char *)reader->body_seg_addr + sizeof(uint64_t),
               io->header->segment_size - sizeof(uint64_t));
    packed_ptr += io->header->segment_size - sizeof(uint64_t);
    size = reader->packed_size - (io->header->segment_size - sizeof(uint64_t));
    seg_id = reader->body_seg_id + 1;
    while (size > io->header->segment_size) {
      seg_addr = grn_io_seg_ref(ctx, io, seg_id);
      if (!seg_addr) {
        return GRN_UNKNOWN_ERROR;
      }
      grn_memcpy(packed_ptr, seg_addr, io->header->segment_size);
      grn_io_seg_unref(ctx, io, seg_id);
      seg_id++;
      size -= io->header->segment_size;
      packed_ptr += io->header->segment_size;
    }
    seg_addr = grn_io_seg_ref(ctx, io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    grn_memcpy(packed_ptr, seg_addr, size);
    grn_io_seg_unref(ctx, io, seg_id);
    seg_id++;
    src_size = (int)(reader->packed_size - sizeof(uint64_t));
    dest_size = LZ4_decompress_safe(reader->packed_buf, buf, src_size,
                                    (int)reader->value_size);
  } else {
    char *packed_addr = (char *)reader->body_seg_addr;
    packed_addr += reader->body_seg_offset + sizeof(uint64_t);
    src_size = (int)(reader->packed_size - sizeof(uint64_t));
    dest_size = LZ4_decompress_safe(packed_addr, buf, src_size,
                                    (int)reader->value_size);
  }
  if ((uint32_t)dest_size != reader->value_size) {
    return GRN_LZ4_ERROR;
  }
  return GRN_SUCCESS;
}
#endif /* GRN_WITH_LZ4 */

#ifdef GRN_WITH_ZSTD
/* grn_ja_reader_read_zstd() reads a value compressed with Zstandard. */
static grn_rc
grn_ja_reader_read_zstd(grn_ctx *ctx, grn_ja_reader *reader, void *buf)
{
  int src_size, dest_size;
  grn_ja_einfo *einfo = (grn_ja_einfo *)reader->einfo;
  if (EHUGE_P(einfo)) {
    grn_io *io = reader->ja->io;
    void *seg_addr;
    char *packed_ptr;
    uint32_t size, seg_id;
    if (reader->packed_size > reader->packed_buf_size) {
      void *new_buf = GRN_REALLOC(reader->packed_buf, reader->packed_size);
      if (!new_buf) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      reader->packed_buf = new_buf;
      reader->packed_buf_size = reader->packed_size;
    }
    packed_ptr = (char *)reader->packed_buf;
    grn_memcpy(packed_ptr, (char *)reader->body_seg_addr + sizeof(uint64_t),
               io->header->segment_size - sizeof(uint64_t));
    packed_ptr += io->header->segment_size - sizeof(uint64_t);
    size = reader->packed_size - (io->header->segment_size - sizeof(uint64_t));
    seg_id = reader->body_seg_id + 1;
    while (size > io->header->segment_size) {
      seg_addr = grn_io_seg_ref(ctx, io, seg_id);
      if (!seg_addr) {
        return GRN_UNKNOWN_ERROR;
      }
      grn_memcpy(packed_ptr, seg_addr, io->header->segment_size);
      grn_io_seg_unref(ctx, io, seg_id);
      seg_id++;
      size -= io->header->segment_size;
      packed_ptr += io->header->segment_size;
    }
    seg_addr = grn_io_seg_ref(ctx, io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    grn_memcpy(packed_ptr, seg_addr, size);
    grn_io_seg_unref(ctx, io, seg_id);
    seg_id++;
    src_size = (int)(reader->packed_size - sizeof(uint64_t));
    dest_size = ZSTD_decompress(reader->packed_buf, reader->value_size,
                                buf, src_size);
  } else {
    char *packed_addr = (char *)reader->body_seg_addr;
    packed_addr += reader->body_seg_offset + sizeof(uint64_t);
    src_size = (int)(reader->packed_size - sizeof(uint64_t));
    dest_size = ZSTD_decompress(packed_addr, reader->value_size,
                                buf, src_size);
  }
  if ((uint32_t)dest_size != reader->value_size) {
    return GRN_ZSTD_ERROR;
  }
  return GRN_SUCCESS;
}
#endif /* GRN_WITH_ZSTD */

/* grn_ja_reader_read_raw() reads a value. */
static grn_rc
grn_ja_reader_read_raw(grn_ctx *ctx, grn_ja_reader *reader, void *buf)
{
  grn_io *io = reader->ja->io;
  grn_ja_einfo *einfo = (grn_ja_einfo *)reader->einfo;
  if (ETINY_P(einfo)) {
    grn_memcpy(buf, einfo, reader->value_size);
  } else if (EHUGE_P(einfo)) {
    char *buf_ptr = (char *)buf;
    void *seg_addr;
    uint32_t seg_id = reader->body_seg_id;
    uint32_t size = reader->value_size;
    while (size > io->header->segment_size) {
      seg_addr = grn_io_seg_ref(ctx, io, seg_id);
      if (!seg_addr) {
        return GRN_UNKNOWN_ERROR;
      }
      grn_memcpy(buf_ptr, seg_addr, io->header->segment_size);
      grn_io_seg_unref(ctx, io, seg_id);
      seg_id++;
      size -= io->header->segment_size;
      buf_ptr += io->header->segment_size;
    }
    seg_addr = grn_io_seg_ref(ctx, io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    grn_memcpy(buf_ptr, seg_addr, size);
    grn_io_seg_unref(ctx, io, seg_id);
    seg_id++;
  } else {
    if (!reader->body_seg_addr) {
      reader->body_seg_addr = grn_io_seg_ref(ctx, io, reader->body_seg_id);
      if (!reader->body_seg_addr) {
        return GRN_UNKNOWN_ERROR;
      }
    }
    grn_memcpy(buf, (char *)reader->body_seg_addr + reader->body_seg_offset,
               reader->value_size);
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ja_reader_read(grn_ctx *ctx, grn_ja_reader *reader, void *buf)
{
  switch (reader->ja->header->flags & GRN_OBJ_COMPRESS_MASK) {
#ifdef GRN_WITH_ZLIB
  case GRN_OBJ_COMPRESS_ZLIB :
    return grn_ja_reader_read_zlib(ctx, reader, buf);
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_OBJ_COMPRESS_LZ4 :
    return grn_ja_reader_read_lz4(ctx, reader, buf);
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_OBJ_COMPRESS_ZSTD :
    return grn_ja_reader_read_zstd(ctx, reader, buf);
#endif /* GRN_WITH_ZSTD */
  default :
    return grn_ja_reader_read_raw(ctx, reader, buf);
  }
}

#ifdef GRN_WITH_ZLIB
/* grn_ja_reader_pread_zlib() reads a part of a value compressed with zlib. */
static grn_rc
grn_ja_reader_pread_zlib(grn_ctx *ctx, grn_ja_reader *reader,
                         size_t offset, size_t size, void *buf)
{
  /* TODO: To be supported? */
  return GRN_FUNCTION_NOT_IMPLEMENTED;
}
#endif /* GRN_WITH_ZLIB */

#ifdef GRN_WITH_LZ4
/* grn_ja_reader_pread_lz4() reads a part of a value compressed with LZ4. */
static grn_rc
grn_ja_reader_pread_lz4(grn_ctx *ctx, grn_ja_reader *reader,
                        size_t offset, size_t size, void *buf)
{
  /* TODO: To be supported? */
  return GRN_FUNCTION_NOT_IMPLEMENTED;
}
#endif /* GRN_WITH_LZ4 */

#ifdef GRN_WITH_ZSTD
/* grn_ja_reader_pread_zstd() reads a part of a value compressed with ZSTD. */
static grn_rc
grn_ja_reader_pread_zstd(grn_ctx *ctx, grn_ja_reader *reader,
                         size_t offset, size_t size, void *buf)
{
  /* TODO: To be supported? */
  return GRN_FUNCTION_NOT_IMPLEMENTED;
}
#endif /* GRN_WITH_ZSTD */

/* grn_ja_reader_pread_raw() reads a part of a value. */
static grn_rc
grn_ja_reader_pread_raw(grn_ctx *ctx, grn_ja_reader *reader,
                        size_t offset, size_t size, void *buf)
{
  grn_io *io = reader->ja->io;
  grn_ja_einfo *einfo = (grn_ja_einfo *)reader->einfo;
  if ((offset >= reader->value_size) || !size) {
    return GRN_SUCCESS;
  }
  if (size > (reader->value_size - offset)) {
    size = reader->value_size - offset;
  }
  if (ETINY_P(einfo)) {
    grn_memcpy(buf, (char *)einfo + offset, size);
  } else if (EHUGE_P(einfo)) {
    char *buf_ptr = (char *)buf;
    void *seg_addr;
    uint32_t seg_id = reader->body_seg_id;
    if (offset >= io->header->segment_size) {
      seg_id += offset / io->header->segment_size;
      offset %= io->header->segment_size;
    }
    seg_addr = grn_io_seg_ref(ctx, io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    grn_memcpy(buf_ptr, (char *)seg_addr + offset,
               io->header->segment_size - offset);
    grn_io_seg_unref(ctx, io, seg_id);
    seg_id++;
    size -= io->header->segment_size - offset;
    buf_ptr += io->header->segment_size - offset;
    while (size > io->header->segment_size) {
      seg_addr = grn_io_seg_ref(ctx, io, seg_id);
      if (!seg_addr) {
        return GRN_UNKNOWN_ERROR;
      }
      grn_memcpy(buf_ptr, (char *)seg_addr, io->header->segment_size);
      grn_io_seg_unref(ctx, io, seg_id);
      seg_id++;
      size -= io->header->segment_size;
      buf_ptr += io->header->segment_size;
    }
    seg_addr = grn_io_seg_ref(ctx, io, seg_id);
    if (!seg_addr) {
      return GRN_UNKNOWN_ERROR;
    }
    grn_memcpy(buf_ptr, seg_addr, size);
    grn_io_seg_unref(ctx, io, seg_id);
  } else {
    if (!reader->body_seg_addr) {
      reader->body_seg_addr = grn_io_seg_ref(ctx, io, reader->body_seg_id);
      if (!reader->body_seg_addr) {
        return GRN_UNKNOWN_ERROR;
      }
    }
    offset += reader->body_seg_offset;
    grn_memcpy(buf, (char *)reader->body_seg_addr + offset, size);
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ja_reader_pread(grn_ctx *ctx, grn_ja_reader *reader,
                    size_t offset, size_t size, void *buf)
{
  switch (reader->ja->header->flags & GRN_OBJ_COMPRESS_MASK) {
#ifdef GRN_WITH_ZLIB
  case GRN_OBJ_COMPRESS_ZLIB :
    return grn_ja_reader_pread_zlib(ctx, reader, offset, size, buf);
#endif /* GRN_WITH_ZLIB */
#ifdef GRN_WITH_LZ4
  case GRN_OBJ_COMPRESS_LZ4 :
    return grn_ja_reader_pread_lz4(ctx, reader, offset, size, buf);
#endif /* GRN_WITH_LZ4 */
#ifdef GRN_WITH_ZSTD
  case GRN_OBJ_COMPRESS_ZSTD :
    return grn_ja_reader_pread_zstd(ctx, reader, offset, size, buf);
#endif /* GRN_WITH_ZSTD */
  default :
    return grn_ja_reader_pread_raw(ctx, reader, offset, size, buf);
  }
}

/**** vgram ****/

/*

static int len_sum = 0;
static int img_sum = 0;
static int simple_sum = 0;
static int skip_sum = 0;

grn_vgram *
grn_vgram_create(const char *path)
{
  grn_vgram *s;
  if (!(s = GRN_MALLOCN(grn_vgram, 1))) { return NULL; }
  s->vgram = grn_sym_create(path, sizeof(grn_id) * 2, 0, GRN_ENC_NONE);
  if (!s->vgram) {
    GRN_FREE(s);
    return NULL;
  }
  return s;
}

grn_vgram *
grn_vgram_open(const char *path)
{
  grn_vgram *s;
  if (!(s = GRN_MALLOCN(grn_vgram, 1))) { return NULL; }
  s->vgram = grn_sym_open(path);
  if (!s->vgram) {
    GRN_FREE(s);
    return NULL;
  }
  return s;
}

grn_vgram_buf *
grn_vgram_buf_open(size_t len)
{
  grn_vgram_buf *b;
  if (!(b = GRN_MALLOCN(grn_vgram_buf, 1))) { return NULL; }
  b->len = len;
  b->tvs = b->tvp = GRN_MALLOCN(grn_id, len);
  if (!b->tvp) { GRN_FREE(b); return NULL; }
  b->tve = b->tvs + len;
  b->vps = b->vpp = GRN_MALLOCN(grn_vgram_vnode, len * 2);
  if (!b->vpp) { GRN_FREE(b->tvp); GRN_FREE(b); return NULL; }
  b->vpe = b->vps + len;
  return b;
}

grn_rc
grn_vgram_buf_add(grn_vgram_buf *b, grn_id tid)
{
  uint8_t dummybuf[8], *dummyp;
  if (b->tvp < b->tve) { *b->tvp++ = tid; }
  dummyp = dummybuf;
  GRN_B_ENC(tid, dummyp);
  simple_sum += dummyp - dummybuf;
  return GRN_SUCCESS;
}

typedef struct {
  grn_id vid;
  grn_id tid;
} vgram_key;

grn_rc
grn_vgram_update(grn_vgram *vgram, grn_id rid, grn_vgram_buf *b, grn_hash *terms)
{
  grn_inv_updspec **u;
  if (b && b->tvs < b->tvp) {
    grn_id *t0, *tn;
    for (t0 = b->tvs; t0 < b->tvp - 1; t0++) {
      grn_vgram_vnode *v, **vp;
      if (grn_set_at(terms, t0, (void **) &u)) {
        vp = &(*u)->vnodes;
        for (tn = t0 + 1; tn < b->tvp; tn++) {
          for (v = *vp; v && v->tid != *tn; v = v->cdr) ;
          if (!v) {
            if (b->vpp < b->vpe) {
              v = b->vpp++;
            } else {
              // todo;
              break;
            }
            v->car = NULL;
            v->cdr = *vp;
            *vp = v;
            v->tid = *tn;
            v->vid = 0;
            v->freq = 0;
            v->len = tn - t0;
          }
          v->freq++;
          if (v->vid) {
            vp = &v->car;
          } else {
            break;
          }
        }
      }
    }
    {
      grn_set *th = grn_set_open(sizeof(grn_id), sizeof(int), 0);
      if (!th) { return GRN_NO_MEMORY_AVAILABLE; }
      if (t0 == b->tvp) { GRN_LOG(ctx, GRN_LOG_DEBUG, "t0 == tvp"); }
      for (t0 = b->tvs; t0 < b->tvp; t0++) {
        grn_id vid, vid0 = *t0, vid1 = 0;
        grn_vgram_vnode *v, *v2 = NULL, **vp;
        if (grn_set_at(terms, t0, (void **) &u)) {
          vp = &(*u)->vnodes;
          for (tn = t0 + 1; tn < b->tvp; tn++) {
            for (v = *vp; v; v = v->cdr) {
              if (!v->vid && (v->freq < 2 || v->freq * v->len < 4)) {
                *vp = v->cdr;
                v->freq = 0;
              }
              if (v->tid == *tn) { break; }
              vp = &v->cdr;
            }
            if (v) {
              if (v->freq) {
                v2 = v;
                vid1 = vid0;
                vid0 = v->vid;
              }
              if (v->vid) {
                vp = &v->car;
                continue;
              }
            }
            break;
          }
        }
        if (v2) {
          if (!v2->vid) {
            vgram_key key;
            key.vid = vid1;
            key.tid = v2->tid;
            if (!(v2->vid = grn_sym_get(vgram->vgram, (char *)&key))) {
              grn_set_close(th);
              return GRN_NO_MEMORY_AVAILABLE;
            }
          }
          vid = *t0 = v2->vid * 2 + 1;
          memset(t0 + 1, 0, sizeof(grn_id) * v2->len);
          t0 += v2->len;
        } else {
          vid = *t0 *= 2;
        }
        {
          int *tf;
          if (!grn_set_get(th, &vid, (void **) &tf)) {
            grn_set_close(th);
            return GRN_NO_MEMORY_AVAILABLE;
          }
          (*tf)++;
        }
      }
      if (!th->n_entries) { GRN_LOG(ctx, GRN_LOG_DEBUG, "th->n_entries == 0"); }
      {
        int j = 0;
        int skip = 0;
        grn_set_eh *ehs, *ehp, *ehe;
        grn_set_sort_optarg arg;
        uint8_t *ps = GRN_MALLOC(b->len * 2), *pp, *pe;
        if (!ps) {
          grn_set_close(th);
          return GRN_NO_MEMORY_AVAILABLE;
        }
        pp = ps;
        pe = ps + b->len * 2;
        arg.mode = grn_sort_descending;
        arg.compar = NULL;
        arg.compar_arg = (void *)(intptr_t)sizeof(grn_id);
        ehs = grn_set_sort(th, 0, &arg);
        if (!ehs) {
          GRN_FREE(ps);
          grn_set_close(th);
          return GRN_NO_MEMORY_AVAILABLE;
        }
        GRN_B_ENC(th->n_entries, pp);
        for (ehp = ehs, ehe = ehs + th->n_entries; ehp < ehe; ehp++, j++) {
          int *id = (int *)GRN_SET_INTVAL(*ehp);
          GRN_B_ENC(*GRN_SET_INTKEY(*ehp), pp);
          *id = j;
        }
        for (t0 = b->tvs; t0 < b->tvp; t0++) {
          if (*t0) {
            int *id;
            if (!grn_set_at(th, t0, (void **) &id)) {
              GRN_LOG(ctx, GRN_LOG_ERROR, "lookup error (%d)", *t0);
            }
            GRN_B_ENC(*id, pp);
          } else {
            skip++;
          }
        }
        len_sum += b->len;
        img_sum += pp - ps;
        skip_sum += skip;
        GRN_FREE(ehs);
        GRN_FREE(ps);
      }
      grn_set_close(th);
    }
  }
  return GRN_SUCCESS;
}

grn_rc
grn_vgram_buf_close(grn_vgram_buf *b)
{
  if (!b) { return GRN_INVALID_ARGUMENT; }
  if (b->tvs) { GRN_FREE(b->tvs); }
  if (b->vps) { GRN_FREE(b->vps); }
  GRN_FREE(b);
  return GRN_SUCCESS;
}

grn_rc
grn_vgram_close(grn_vgram *vgram)
{
  if (!vgram) { return GRN_INVALID_ARGUMENT; }
  GRN_LOG(ctx, GRN_LOG_DEBUG, "len=%d img=%d skip=%d simple=%d", len_sum, img_sum, skip_sum, simple_sum);
  grn_sym_close(vgram->vgram);
  GRN_FREE(vgram);
  return GRN_SUCCESS;
}
*/
