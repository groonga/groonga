// Copyright (C) 2009-2018  Brazil
// Copyright (C) 2018-2025  Sutou Kouhei <kou@clear-code.com>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

#include "grn.h"

#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#ifdef WIN32
#  include <io.h>
#  include <share.h>
#endif /* WIN32 */

#include "grn_ctx.hpp"
#include "grn_ii.h"
#include "grn_ii_select_cursor.h"
#include "grn_ctx_impl.h"
#include "grn_table.h"
#include "grn_token_cursor.h"
#include "grn_pat.h"
#include "grn_db.h"
#include "grn_obj.h"
#include "grn_output.h"
#include "grn_progress.h"
#include "grn_report.h"
#include "grn_scorer.h"
#include "grn_util.h"
#include "grn_posting.h"
#include "grn_vector.h"
#include "grn_selector.h"
#include "grn_wal.h"

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#ifdef GRN_SUPPORT_REGEXP
#  define GRN_II_SELECT_ENABLE_SEQUENTIAL_SEARCH_TEXT
#endif

#ifdef GRN_II_SELECT_ENABLE_SEQUENTIAL_SEARCH_TEXT
#  include "grn_string.h"
#  include "grn_onigmo.h"
#endif

/* #define GRN_II_TOKEN_INFO_DEBUG */
#ifdef GRN_II_TOKEN_INFO_DEBUG
#  define P_NOTE(...)               printf(__VA_ARGS__)
#  define P_TI(ctx, token_info)     grn_p_token_info((ctx), (token_info))
#  define P_TIS(ctx, token_info, n) grn_p_token_infos((ctx), (token_info), (n))
#else
#  define P_NOTE(...)
#  define P_TI(ctx, token_info)
#  define P_TIS(ctx, token_info, n)
#endif

/* P is for physical? */
#define MAX_PSEG       0x20000
#define MAX_PSEG_SMALL 0x00200
/* MAX_PSEG_MEDIUM has enough space for the following source:
 *   * Single source.
 *   * Source is a fixed size column or _key of a table.
 *   * Source column is a scalar column.
 *   * Lexicon doesn't have tokenizer.
 */
#define MAX_PSEG_MEDIUM         0x10000
#define MAX_PSEG_LARGE          0x40000
#define S_CHUNK                 (1 << GRN_II_W_CHUNK)
#define W_SEGMENT               18
#define S_SEGMENT               (1 << W_SEGMENT)
#define W_ARRAY_ELEMENT         3
#define S_ARRAY_ELEMENT         (1 << W_ARRAY_ELEMENT)
#define W_ARRAY                 (W_SEGMENT - W_ARRAY_ELEMENT)
#define ARRAY_MASK_IN_A_SEGMENT ((1 << W_ARRAY) - 1)

#define S_GARBAGE               (1 << 12)

#define CHUNK_SPLIT             0x80000000
#define CHUNK_SPLIT_THRESHOLD   0x60000
#define CHUNK_SPLIT_OFF(tid)    ((tid) & ~CHUNK_SPLIT)

#define MAX_N_ELEMENTS          5

#ifndef S_IRUSR
#  define S_IRUSR 0400
#endif /* S_IRUSR */
#ifndef S_IWUSR
#  define S_IWUSR 0200
#endif /* S_IWUSR */

static bool grn_ii_cursor_set_min_enable = true;
static double grn_ii_select_too_many_index_match_ratio_text = -1;
static double grn_ii_select_too_many_index_match_ratio_reference = -1;
static double grn_ii_estimate_size_for_query_reduce_ratio = 0.9;
static bool grn_ii_overlap_token_skip_enable = false;
static uint32_t grn_ii_builder_block_threshold_force = 0;
static uint32_t grn_ii_max_n_segments_small = MAX_PSEG_SMALL;
static uint32_t grn_ii_max_n_chunks_small = GRN_II_MAX_CHUNK_SMALL;
static int64_t grn_ii_reduce_expire_threshold = 32;
static bool grn_ii_dump_index_source_on_merge = false;

extern "C" void
grn_ii_init_from_env(void)
{
  {
    char grn_ii_cursor_set_min_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_CURSOR_SET_MIN_ENABLE",
               grn_ii_cursor_set_min_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_ii_cursor_set_min_enable_env, "no") == 0) {
      grn_ii_cursor_set_min_enable = false;
    } else {
      grn_ii_cursor_set_min_enable = true;
    }
  }

  {
    char grn_ii_select_too_many_index_match_ratio_text_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_SELECT_TOO_MANY_INDEX_MATCH_RATIO_TEXT",
               grn_ii_select_too_many_index_match_ratio_text_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ii_select_too_many_index_match_ratio_text_env[0]) {
      grn_ii_select_too_many_index_match_ratio_text =
        atof(grn_ii_select_too_many_index_match_ratio_text_env);
    } else {
      /* For backward compatibility. */
      char grn_ii_select_too_many_index_match_ratio_env[GRN_ENV_BUFFER_SIZE];
      grn_getenv("GRN_II_SELECT_TOO_MANY_INDEX_MATCH_RATIO",
                 grn_ii_select_too_many_index_match_ratio_env,
                 GRN_ENV_BUFFER_SIZE);
      if (grn_ii_select_too_many_index_match_ratio_env[0]) {
        grn_ii_select_too_many_index_match_ratio_text =
          atof(grn_ii_select_too_many_index_match_ratio_env);
      }
    }
  }

  {
    char grn_ii_select_too_many_index_match_ratio_reference_env
      [GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_SELECT_TOO_MANY_INDEX_MATCH_RATIO_REFERENCE",
               grn_ii_select_too_many_index_match_ratio_reference_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ii_select_too_many_index_match_ratio_reference_env[0]) {
      grn_ii_select_too_many_index_match_ratio_reference =
        atof(grn_ii_select_too_many_index_match_ratio_reference_env);
    }
  }

  {
    char grn_ii_estimate_size_for_query_reduce_ratio_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_ESTIMATE_SIZE_FOR_QUERY_REDUCE_RATIO",
               grn_ii_estimate_size_for_query_reduce_ratio_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ii_estimate_size_for_query_reduce_ratio_env[0]) {
      grn_ii_estimate_size_for_query_reduce_ratio =
        atof(grn_ii_estimate_size_for_query_reduce_ratio_env);
    }
  }

  {
    char grn_ii_overlap_token_skip_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_OVERLAP_TOKEN_SKIP_ENABLE",
               grn_ii_overlap_token_skip_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_ii_overlap_token_skip_enable_env, "yes") == 0) {
      grn_ii_overlap_token_skip_enable = true;
    } else {
      grn_ii_overlap_token_skip_enable = false;
    }
  }

  {
    char grn_ii_builder_block_threshold_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_BUILDER_BLOCK_THRESHOLD",
               grn_ii_builder_block_threshold_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ii_builder_block_threshold_env[0]) {
      grn_ii_builder_block_threshold_force =
        grn_atoui(grn_ii_builder_block_threshold_env,
                  grn_ii_builder_block_threshold_env +
                    strlen(grn_ii_builder_block_threshold_env),
                  NULL);
    } else {
      grn_ii_builder_block_threshold_force = 0;
    }
  }

  {
    char grn_ii_max_n_segments_small_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_MAX_N_SEGMENTS_SMALL",
               grn_ii_max_n_segments_small_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ii_max_n_segments_small_env[0]) {
      grn_ii_max_n_segments_small =
        grn_atoui(grn_ii_max_n_segments_small_env,
                  grn_ii_max_n_segments_small_env +
                    strlen(grn_ii_max_n_segments_small_env),
                  NULL);
      if (grn_ii_max_n_segments_small > MAX_PSEG) {
        grn_ii_max_n_segments_small = MAX_PSEG;
      }
    }
  }

  {
    char grn_ii_max_n_chunks_small_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_MAX_N_CHUNKS_SMALL",
               grn_ii_max_n_chunks_small_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ii_max_n_chunks_small_env[0]) {
      grn_ii_max_n_chunks_small = grn_atoui(
        grn_ii_max_n_chunks_small_env,
        grn_ii_max_n_chunks_small_env + strlen(grn_ii_max_n_chunks_small_env),
        NULL);
      if (grn_ii_max_n_chunks_small > GRN_II_MAX_CHUNK) {
        grn_ii_max_n_chunks_small = GRN_II_MAX_CHUNK;
      }
    }
  }

  {
    char grn_ii_reduce_expire_threshold_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_REDUCE_EXPIRE_THRESHOLD",
               grn_ii_reduce_expire_threshold_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_ii_reduce_expire_threshold_env[0]) {
      grn_ii_reduce_expire_threshold =
        grn_atoll(grn_ii_reduce_expire_threshold_env,
                  grn_ii_reduce_expire_threshold_env +
                    strlen(grn_ii_reduce_expire_threshold_env),
                  NULL);
    }
  }

  {
    char grn_ii_dump_index_source_on_merge_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_II_DUMP_INDEX_SOURCE_ON_MERGE",
               grn_ii_dump_index_source_on_merge_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_ii_dump_index_source_on_merge_env, "yes") == 0) {
      grn_ii_dump_index_source_on_merge = true;
    } else {
      grn_ii_dump_index_source_on_merge = false;
    }
  }
}

extern "C" void
grn_ii_cursor_set_min_enable_set(bool enable)
{
  grn_ii_cursor_set_min_enable = enable;
}

extern "C" bool
grn_ii_cursor_set_min_enable_get(void)
{
  return grn_ii_cursor_set_min_enable;
}

static void
grn_ii_get_term(grn_ctx *ctx, grn_ii *ii, grn_id tid, grn_obj *term)
{
  if (tid == GRN_ID_NIL) {
    return;
  }

  char key[GRN_TABLE_MAX_KEY_SIZE];
  int key_size;
  key_size =
    grn_table_get_key(ctx, ii->lexicon, tid, key, GRN_TABLE_MAX_KEY_SIZE);
  if (key_size != 0) {
    grn_obj key_buf;
    GRN_OBJ_INIT(&key_buf,
                 GRN_BULK,
                 GRN_OBJ_DO_SHALLOW_COPY,
                 ii->lexicon->header.domain);
    GRN_TEXT_SET(ctx, &key_buf, key, key_size);
    grn_inspect(ctx, term, &key_buf);
    GRN_OBJ_FIN(ctx, &key_buf);
  }
}

/* segment */

static inline uint32_t
grn_ii_get_array_pseg_inline(grn_ii *ii, uint32_t lseg)
{
  if (lseg < GRN_II_MAX_LSEG) {
    return ii->header.common->ainfo[lseg];
  } else {
    return ii->header.large->ainfo_extend[lseg - GRN_II_MAX_LSEG];
  }
}

static inline void
grn_ii_set_array_pseg_inline(grn_ii *ii, uint32_t lseg, uint32_t pseg)
{
  if (lseg < GRN_II_MAX_LSEG) {
    ii->header.common->ainfo[lseg] = pseg;
  } else {
    ii->header.large->ainfo_extend[lseg - GRN_II_MAX_LSEG] = pseg;
  }
}

static inline uint32_t
grn_ii_get_buffer_pseg_inline(grn_ii *ii, uint32_t lseg)
{
  if (lseg < GRN_II_MAX_LSEG) {
    return ii->header.common->binfo[lseg];
  } else {
    return ii->header.large->binfo_extend[lseg - GRN_II_MAX_LSEG];
  }
}

static inline uint32_t *
grn_ii_get_buffer_pseg_address_inline(grn_ii *ii, uint32_t lseg)
{
  if (lseg < GRN_II_MAX_LSEG) {
    return &(ii->header.common->binfo[lseg]);
  } else {
    return &(ii->header.large->binfo_extend[lseg - GRN_II_MAX_LSEG]);
  }
}

static inline void
grn_ii_set_buffer_pseg_inline(grn_ii *ii, uint32_t lseg, uint32_t pseg)
{
  if (lseg < GRN_II_MAX_LSEG) {
    ii->header.common->binfo[lseg] = pseg;
  } else {
    ii->header.large->binfo_extend[lseg - GRN_II_MAX_LSEG] = pseg;
  }
}

static inline uint32_t
grn_ii_n_logical_segments_inline(grn_ii *ii)
{
  if (ii->header.common->flags & GRN_OBJ_INDEX_LARGE) {
    return GRN_II_MAX_LSEG + GRN_II_MAX_LSEG_EXTEND;
  } else {
    return GRN_II_MAX_LSEG;
  }
}

static inline uint32_t
segment_get(grn_ctx *ctx, grn_ii *ii)
{
  grn_ii_header_common *header = ii->header.common;
  uint32_t pseg;
  if (header->bgqtail == ((header->bgqhead + 1) & (GRN_II_BGQSIZE - 1))) {
    pseg = header->bgqbody[header->bgqtail];
    header->bgqtail = (header->bgqtail + 1) & (GRN_II_BGQSIZE - 1);
  } else {
    pseg = header->pnext;
    if (!pseg) {
      uint32_t i;
      uint32_t pmax = 0;
      const uint32_t max_segment = ii->seg->header->max_segment;
      auto used = static_cast<char *>(GRN_CALLOC(max_segment));
      if (!used) {
        return max_segment;
      }
      const uint32_t n_logical_segments = grn_ii_n_logical_segments_inline(ii);
      for (i = 0; i < n_logical_segments; i++) {
        pseg = grn_ii_get_array_pseg_inline(ii, i);
        if (pseg != GRN_II_PSEG_NOT_ASSIGNED) {
          if (pseg > pmax) {
            pmax = pseg;
          }
          used[pseg] = 1;
        }
        pseg = grn_ii_get_buffer_pseg_inline(ii, i);
        if (pseg != GRN_II_PSEG_NOT_ASSIGNED) {
          if (pseg > pmax) {
            pmax = pseg;
          }
          used[pseg] = 1;
        }
      }
      for (pseg = 0; pseg < max_segment && used[pseg]; pseg++)
        ;
      GRN_FREE(used);
      header->pnext = pmax + 1;
    } else if (ii->header.common->pnext < ii->seg->header->max_segment) {
      header->pnext++;
    }
  }
  return pseg;
}

static inline grn_rc
segment_get_clear(grn_ctx *ctx, grn_ii *ii, uint32_t *pseg)
{
  uint32_t seg = segment_get(ctx, ii);
  if (seg < ii->seg->header->max_segment) {
    void *p = grn_io_seg_ref(ctx, ii->seg, seg);
    if (!p) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    memset(p, 0, S_SEGMENT);
    grn_io_seg_unref(ctx, ii->seg, seg);
    *pseg = seg;
    return GRN_SUCCESS;
  } else {
    return GRN_NO_MEMORY_AVAILABLE;
  }
}

static inline grn_rc
buffer_segment_new(grn_ctx *ctx, grn_ii *ii, uint32_t *segno)
{
  uint32_t lseg, pseg;
  if (*segno == GRN_II_PSEG_NOT_ASSIGNED) {
    const uint32_t n_logical_segments = grn_ii_n_logical_segments_inline(ii);
    for (lseg = 0; lseg < n_logical_segments; lseg++) {
      if (grn_ii_get_buffer_pseg_inline(ii, lseg) == GRN_II_PSEG_NOT_ASSIGNED) {
        break;
      }
    }
    if (lseg == n_logical_segments) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    *segno = lseg;
  } else {
    if (grn_ii_get_buffer_pseg_inline(ii, *segno) != GRN_II_PSEG_NOT_ASSIGNED) {
      return GRN_INVALID_ARGUMENT;
    }
    lseg = *segno;
  }
  pseg = segment_get(ctx, ii);
  if (pseg < ii->seg->header->max_segment) {
    grn_ii_set_buffer_pseg_inline(ii, lseg, pseg);
    if (lseg >= ii->header.common->bmax) {
      ii->header.common->bmax = lseg + 1;
    }
    return GRN_SUCCESS;
  } else {
    return GRN_NO_MEMORY_AVAILABLE;
  }
}

static grn_rc
buffer_segment_reserve(grn_ctx *ctx,
                       grn_ii *ii,
                       uint32_t *lseg0,
                       uint32_t *pseg0,
                       uint32_t *lseg1,
                       uint32_t *pseg1)
{
  uint32_t i = 0;
  const uint32_t n_logical_segments = grn_ii_n_logical_segments_inline(ii);
  for (;; i++) {
    if (i == n_logical_segments) {
      GRN_DEFINE_NAME(ii);
      MERR("[ii][buffer][segment][reserve] "
           "couldn't find a free buffer: <%.*s>: max:<%u>",
           name_size,
           name,
           n_logical_segments);
      return ctx->rc;
    }
    if (grn_ii_get_buffer_pseg_inline(ii, i) == GRN_II_PSEG_NOT_ASSIGNED) {
      break;
    }
  }
  *lseg0 = i++;
  for (;; i++) {
    if (i == n_logical_segments) {
      GRN_DEFINE_NAME(ii);
      MERR("[ii][buffer][segment][reserve] "
           "couldn't find two free buffers: "
           "<%.*s>: "
           "found:<%u>, max:<%u>",
           name_size,
           name,
           *lseg0,
           n_logical_segments);
      return ctx->rc;
    }
    if (grn_ii_get_buffer_pseg_inline(ii, i) == GRN_II_PSEG_NOT_ASSIGNED) {
      break;
    }
  }
  *lseg1 = i;
  if ((*pseg0 = segment_get(ctx, ii)) == ii->seg->header->max_segment) {
    GRN_DEFINE_NAME(ii);
    MERR("[ii][buffer][segment][reserve] "
         "couldn't allocate a free segment: <%.*s>: "
         "buffer:<%u>, max:<%u>",
         name_size,
         name,
         *lseg0,
         ii->seg->header->max_segment);
    return ctx->rc;
  }
  if ((*pseg1 = segment_get(ctx, ii)) == ii->seg->header->max_segment) {
    GRN_DEFINE_NAME(ii);
    MERR("[ii][buffer][segment][reserve] "
         "couldn't allocate two free segments: "
         "<%.*s>: "
         "found:<%u>, not-found:<%u>, max:<%u>",
         name_size,
         name,
         *lseg0,
         *lseg1,
         ii->seg->header->max_segment);
    return ctx->rc;
  }
  return ctx->rc;
}

static inline void
buffer_garbage_enqueue(grn_ii *ii, uint32_t lseg)
{
  const uint32_t pseg = grn_ii_get_buffer_pseg_inline(ii, lseg);
  if (pseg != GRN_II_PSEG_NOT_ASSIGNED) {
    grn_ii_header_common *header = ii->header.common;
    header->bgqbody[header->bgqhead] = header->binfo[lseg];
    header->bgqhead = (header->bgqhead + 1) & (GRN_II_BGQSIZE - 1);
    GRN_ASSERT(header->bgqhead != header->bgqtail);
  }
}

static inline void
buffer_segment_update(grn_ii *ii, uint32_t lseg, uint32_t pseg)
{
  buffer_garbage_enqueue(ii, lseg);
  // smb_wmb();
  grn_ii_set_buffer_pseg_inline(ii, lseg, pseg);
  if (lseg >= ii->header.common->bmax) {
    ii->header.common->bmax = lseg + 1;
  }
}

static inline void
buffer_segment_clear(grn_ii *ii, uint32_t lseg)
{
  buffer_garbage_enqueue(ii, lseg);
  // smb_wmb();
  grn_ii_set_buffer_pseg_inline(ii, lseg, GRN_II_PSEG_NOT_ASSIGNED);
}

/* chunk */

#define HEADER_CHUNK_AT(ii, offset)                                            \
  ((((ii)->header.common->chunks[((offset) >> 3)]) >> ((offset) & 7)) & 1)

#define HEADER_CHUNK_ON(ii, offset)                                            \
  (((ii)->header.common->chunks[((offset) >> 3)]) |= (1 << ((offset) & 7)))

#define HEADER_CHUNK_OFF(ii, offset)                                           \
  (((ii)->header.common->chunks[((offset) >> 3)]) &= ~(1 << ((offset) & 7)))

#define N_GARBAGES_TH 1

#define N_GARBAGES    ((S_GARBAGE - (sizeof(uint32_t) * 4)) / (sizeof(uint32_t)))

typedef struct {
  uint32_t head;
  uint32_t tail;
  uint32_t nrecs;
  uint32_t next;
  uint32_t recs[N_GARBAGES];
} grn_ii_ginfo;

#define WIN_MAP(ctx, chunk, iw, seg, pos, size, mode)                          \
  grn_io_win_map(ctx,                                                          \
                 chunk,                                                        \
                 iw,                                                           \
                 ((seg) >> GRN_II_N_CHUNK_VARIATION),                          \
                 (((seg) & ((1 << GRN_II_N_CHUNK_VARIATION) - 1))              \
                  << GRN_II_W_LEAST_CHUNK) +                                   \
                   (pos),                                                      \
                 size,                                                         \
                 mode)

/*
static int new_histogram[32];
static int free_histogram[32];
*/

/*
  N = grn_ii_max_n_chunks_small  for GRN_OBJ_INDEX_SMALL
  N = GRN_II_MAX_N_CHUNKS_MEDIUM for GRN_OBJ_INDEX_MEDIUM
  N = GRN_II_MAX_N_CHUNKS        for GRN_OBJ_INDEX_LARGE
  N = GRN_II_MAX_N_CHUNKS        without index size flag

  S_CHUNK = (1 << GRN_II_W_CHUNK(22)) = 4MiB

  +------------+     +------------+
  |   chunk0   | ... |   chunkN   |
  +------------+     +------------+
  |<--S_CHUNK->|

  ii->header.common->chunks has flags whether the target chunk is
  assigned or not. We can manipulate the flags by HEADER_CHUNK_AT(),
  HEADER_CHUNK_ON() and HEADER_CHUNK_OFF(). Here is a snapshot when we
  only assign chunk0 and chunk2:

  +------------+------------+------------+     +------------+
  |   chunk0   |   chunk1   |   chunk2   | ... |   chunkN   |
  +------------+------------+------------+     +------------+
        ON          OFF           ON                 OFF

  If requested size is greater than S_CHUNK, continuous non-assigned
  chunks are assigned. For example, if S_CHUNK + 100 is requested and
  chunk1 is assigned, chunk0 isn't assigned. chunk2 and chunk3 are
  assigned.

  +------------+------------+------------+------------+     +------------+
  |   chunk0   |   chunk1   |   chunk2   |   chunk3   | ... |   chunkN   |
  +------------+------------+------------+------------+     +------------+
       OFF          ON        OFF -> ON    OFF -> ON             OFF

  If requested size is smaller than or equal to S_CHUNK, a subchunk is
  assigned. Subchunks in the same chunk has the same size. The
  smallest subchunk size is (1 << GRN_II_W_LEAST_CHUNK(8)) = 256
  bytes. Subchunk size is power of 2. For example, chunk0 uses 512
  bytes for subchunk size, chunk0 can have 812 (S_CHUNK / 512)
  subchunks.

  +-------------+
  |   chunk0    |
  +-------------+
  <->|<->|...|<-> subchunks
  512|512|   |512

  If one of subchunk is assigned, HEADER_CHUNK_AT() of the chunk is true.

  +-------------+
  |   chunk0    |
  +-------------+
  <------>|<->|... subchunks
  ASSIGNED|NOT|
        ON

  Chunks that are assigning subchunks are keeping in
  ii->header.common.free_chunks. free_chunks keeps a chunk per
  subchunk size. For example, chunk0 is kept for 256 subchunk size and
  chunk1 is kept for 512 subchunk size.

  chunk_new() returns the (offset >> GRN_II_W_LEAST_CHUNK(8)) of
  subchunk not the index of chunk. For example, if the third subchunk
  of chunk0 that uses 512 bytes for subchunk size is assigned, 4 (1024
  >> 8) is returned.

  +-------------+
  |   chunk0    |
  +-------------+
  <0>|<1>|<2>|...
  0   2   4

  chunk_new() wants to reuse freed subchunks as much as possible to
  reduce disk usage. ii->header.common.garbages keeps freed subchunks
  to reuse later.

  TODO: More description for garbages.
*/

static grn_rc
chunk_free(grn_ctx *ctx, grn_ii *ii, uint32_t offset, uint32_t size);

static grn_rc
chunk_new(grn_ctx *ctx, grn_ii *ii, uint32_t *res, uint32_t size)
{
  uint32_t n_chunks;

  n_chunks = ii->chunk->header->max_segment;

  /*
  if (size) {
    int es = size - 1;
    grn_bit_scan_rev_result m;
    GRN_BIT_SCAN_REV(es, m);
    m++;
    new_histogram[m]++;
  }
  */
  if (size > S_CHUNK) {
    uint32_t i, j;
    uint32_t n = (size + S_CHUNK - 1) >> GRN_II_W_CHUNK;
    for (i = 0, j = -1; i < n_chunks; i++) {
      if (HEADER_CHUNK_AT(ii, i)) {
        j = i;
      } else {
        if (i == j + n) {
          j++;
          *res = j << GRN_II_N_CHUNK_VARIATION;
          for (; j <= i; j++) {
            HEADER_CHUNK_ON(ii, j);
          }
          return GRN_SUCCESS;
        }
      }
    }
    {
      GRN_DEFINE_NAME(ii);
      MERR("[ii][chunk][new] index is full: "
           "<%.*s>: "
           "size:<%u>, n-chunks:<%u>",
           name_size,
           name,
           size,
           n_chunks);
    }
    return ctx->rc;
  } else {
    uint32_t *vp;
    grn_bit_scan_rev_result m;
    if (size > (1 << GRN_II_W_LEAST_CHUNK)) {
      int es = size - 1;
      GRN_BIT_SCAN_REV(es, m);
      m++;
    } else {
      m = GRN_II_W_LEAST_CHUNK;
    }
    grn_ii_header_common *header = ii->header.common;
    if (header->ngarbages[m - GRN_II_W_LEAST_CHUNK] > N_GARBAGES_TH) {
      uint32_t *gseg;
      grn_io_win iw, iw_;
      iw_.addr = NULL;
      gseg = &header->garbages[m - GRN_II_W_LEAST_CHUNK];
      while (*gseg != GRN_II_PSEG_NOT_ASSIGNED) {
        auto ginfo = static_cast<grn_ii_ginfo *>(
          WIN_MAP(ctx, ii->chunk, &iw, *gseg, 0, S_GARBAGE, GRN_IO_RDWR));
        // GRN_IO_SEG_MAP2(ii->chunk, *gseg, ginfo);
        if (!ginfo) {
          if (iw_.addr) {
            grn_io_win_unmap(ctx, &iw_);
          }
          {
            GRN_DEFINE_NAME(ii);
            MERR("[ii][chunk][new] failed to allocate garbage segment: "
                 "<%.*s>: "
                 "n-garbages:<%u>, size:<%u>, n-chunks:<%u>",
                 name_size,
                 name,
                 header->ngarbages[m - GRN_II_W_LEAST_CHUNK],
                 size,
                 n_chunks);
          }
          return ctx->rc;
        }
        if (ginfo->next != GRN_II_PSEG_NOT_ASSIGNED ||
            ginfo->nrecs > N_GARBAGES_TH) {
          *res = ginfo->recs[ginfo->tail];
          if (++ginfo->tail == N_GARBAGES) {
            ginfo->tail = 0;
          }
          ginfo->nrecs--;
          header->ngarbages[m - GRN_II_W_LEAST_CHUNK]--;
          uint32_t current_garbage_segment = *gseg;
          bool no_garbage_info = (ginfo->nrecs == 0);
          if (no_garbage_info) {
            *gseg = ginfo->next;
          }
          if (iw_.addr) {
            grn_io_win_unmap(ctx, &iw_);
          }
          grn_io_win_unmap(ctx, &iw);
          if (no_garbage_info) {
            chunk_free(ctx, ii, current_garbage_segment, S_GARBAGE);
          }
          return GRN_SUCCESS;
        }
        if (iw_.addr) {
          grn_io_win_unmap(ctx, &iw_);
        }
        iw_ = iw;
        gseg = &ginfo->next;
      }
      if (iw_.addr) {
        grn_io_win_unmap(ctx, &iw_);
      }
    }
    vp = &header->free_chunks[m - GRN_II_W_LEAST_CHUNK];
    if (*vp == GRN_II_PSEG_NOT_ASSIGNED) {
      uint32_t i = 0;
      while (HEADER_CHUNK_AT(ii, i)) {
        if (++i >= n_chunks) {
          GRN_DEFINE_NAME(ii);
          MERR("[ii][chunk][new] failed to find a free chunk: "
               "<%.*s>: "
               "index:<%u>, size:<%u>, n-chunks:<%u>",
               name_size,
               name,
               m - GRN_II_W_LEAST_CHUNK,
               size,
               n_chunks);
          return ctx->rc;
        }
      }
      HEADER_CHUNK_ON(ii, i);
      *vp = i << GRN_II_N_CHUNK_VARIATION;
    }
    *res = *vp;
    /* Next subchunk. */
    *vp += 1 << (m - GRN_II_W_LEAST_CHUNK);
    /* (*vp % GRN_II_N_CHUNK_VARIATION) == 0 */
    if (!(*vp & ((1 << GRN_II_N_CHUNK_VARIATION) - 1))) {
      /* All subchunks are consumed. */
      *vp = GRN_II_PSEG_NOT_ASSIGNED;
    }
    return GRN_SUCCESS;
  }
}

static grn_rc
chunk_free(grn_ctx *ctx, grn_ii *ii, uint32_t offset, uint32_t size)
{
  /*
  if (size) {
    int es = size - 1;
    grn_bit_scan_rev_result m;
    GRN_BIT_SCAN_REV(es, m);
    m++;
    free_histogram[m]++;
  }
  */
  grn_io_win iw, iw_;
  grn_ii_ginfo *ginfo;
  uint32_t seg, *gseg;
  seg = offset >> GRN_II_N_CHUNK_VARIATION;
  if (size > S_CHUNK) {
    int n = (size + S_CHUNK - 1) >> GRN_II_W_CHUNK;
    for (; n--; seg++) {
      HEADER_CHUNK_OFF(ii, seg);
    }
    return GRN_SUCCESS;
  }
  grn_bit_scan_rev_result m;
  if (size > (1 << GRN_II_W_LEAST_CHUNK)) {
    int es = size - 1;
    GRN_BIT_SCAN_REV(es, m);
    m++;
  } else {
    m = GRN_II_W_LEAST_CHUNK;
  }
  gseg = &ii->header.common->garbages[m - GRN_II_W_LEAST_CHUNK];
  iw_.addr = NULL;
  while (*gseg != GRN_II_PSEG_NOT_ASSIGNED) {
    ginfo = static_cast<grn_ii_ginfo *>(
      WIN_MAP(ctx, ii->chunk, &iw, *gseg, 0, S_GARBAGE, GRN_IO_RDWR));
    // GRN_IO_SEG_MAP2(ii->chunk, *gseg, ginfo);
    if (!ginfo) {
      if (iw_.addr) {
        grn_io_win_unmap(ctx, &iw_);
      }
      return GRN_NO_MEMORY_AVAILABLE;
    }
    if (ginfo->nrecs < N_GARBAGES) {
      break;
    }
    if (iw_.addr) {
      grn_io_win_unmap(ctx, &iw_);
    }
    iw_ = iw;
    gseg = &ginfo->next;
  }
  if (*gseg == GRN_II_PSEG_NOT_ASSIGNED) {
    grn_rc rc;
    if ((rc = chunk_new(ctx, ii, gseg, S_GARBAGE))) {
      if (iw_.addr) {
        grn_io_win_unmap(ctx, &iw_);
      }
      return rc;
    }
    ginfo = static_cast<grn_ii_ginfo *>(
      WIN_MAP(ctx, ii->chunk, &iw, *gseg, 0, S_GARBAGE, GRN_IO_RDWR));
    /*
    uint32_t i = 0;
    while (HEADER_CHUNK_AT(ii, i)) {
      if (++i >= ii->chunk->header->max_segment) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
    }
    HEADER_CHUNK_ON(ii, i);
    *gseg = i;
    GRN_IO_SEG_MAP2(ii->chunk, *gseg, ginfo);
    */
    if (!ginfo) {
      if (iw_.addr) {
        grn_io_win_unmap(ctx, &iw_);
      }
      return GRN_NO_MEMORY_AVAILABLE;
    }
    ginfo->head = 0;
    ginfo->tail = 0;
    ginfo->nrecs = 0;
    ginfo->next = GRN_II_PSEG_NOT_ASSIGNED;
  }
  if (iw_.addr) {
    grn_io_win_unmap(ctx, &iw_);
  }
  ginfo->recs[ginfo->head] = offset;
  if (++ginfo->head == N_GARBAGES) {
    ginfo->head = 0;
  }
  ginfo->nrecs++;
  grn_io_win_unmap(ctx, &iw);
  ii->header.common->ngarbages[m - GRN_II_W_LEAST_CHUNK]++;
  return GRN_SUCCESS;
}

#define DATA_USE_P_FOR_ENC (1 << 0) /* Use PFor */
#define DATA_ODD           (1 << 2) /* Variable size data */

typedef struct {
  uint32_t *data;
  uint32_t data_size;
  uint32_t flags;
} datavec;

static grn_rc
datavec_reset(
  grn_ctx *ctx, grn_ii *ii, datavec *dv, size_t unitsize, size_t totalsize)
{
  uint32_t i;
  if (!dv[0].data || dv[ii->n_elements].data < dv[0].data + totalsize) {
    if (dv[0].data) {
      GRN_FREE(dv[0].data);
    }
    if (!(dv[0].data = static_cast<uint32_t *>(
            GRN_MALLOC(totalsize * sizeof(uint32_t))))) {
      MERR("[ii][data-vector][reset] failed to allocate data: "
           "length:<%u>, "
           "unit-size:<%" GRN_FMT_SIZE ">, "
           "total-size:<%" GRN_FMT_SIZE ">",
           ii->n_elements,
           unitsize,
           totalsize);
      return ctx->rc;
    }
    dv[ii->n_elements].data = dv[0].data + totalsize;
  }
  for (i = 1; i < ii->n_elements; i++) {
    dv[i].data = dv[i - 1].data + unitsize;
  }
  return GRN_SUCCESS;
}

static grn_rc
datavec_init(
  grn_ctx *ctx, grn_ii *ii, datavec *dv, size_t unitsize, size_t totalsize)
{
  uint32_t i;
  if (totalsize == 0) {
    memset(dv, 0, sizeof(datavec) * (ii->n_elements + 1));
  } else {
    if (!(dv[0].data = static_cast<uint32_t *>(
            GRN_MALLOC(totalsize * sizeof(uint32_t))))) {
      MERR("[ii][data-vector][init] failed to allocate data: "
           "length:<%u>, "
           "unit-size:<%" GRN_FMT_SIZE ">, "
           "total-size:<%" GRN_FMT_SIZE ">",
           ii->n_elements,
           unitsize,
           totalsize);
      return ctx->rc;
    }
    dv[ii->n_elements].data = dv[0].data + totalsize;
    dv[0].flags = 0;
    for (i = 1; i < ii->n_elements; i++) {
      dv[i].data = dv[i - 1].data + unitsize;
      dv[i].flags = 0;
    }
  }
  if (ii->header.common->flags & GRN_OBJ_WITH_POSITION) {
    dv[ii->n_elements - 1].flags = DATA_ODD;
  }
  return GRN_SUCCESS;
}

static void
datavec_fin(grn_ctx *ctx, datavec *dv)
{
  if (dv[0].data) {
    GRN_FREE(dv[0].data);
  }
}

static inline bool
data_records_use_p_for_enc(uint32_t n_records, uint32_t max_record_id)
{
  return n_records >= 16 && (n_records > (max_record_id >> 8));
}

static inline bool
data_sections_use_p_for_enc(uint32_t n_records)
{
  return n_records >= 3;
}

static inline bool
data_positions_use_p_for_enc(uint32_t n_positions, uint32_t max_position)
{
  return n_positions >= 32 && n_positions > (max_position >> 13);
}

static inline void
datavec_set_data_size(grn_ctx *ctx,
                      grn_ii *ii,
                      datavec *dv,
                      uint32_t n_records,
                      grn_id max_record_id,
                      uint32_t n_positions,
                      uint32_t max_position)
{
  int i = 0;
  bool records_use_p_for_enc =
    data_records_use_p_for_enc(n_records, max_record_id);
  bool sections_use_p_for_enc = data_sections_use_p_for_enc(n_records);
  /* record IDs */
  dv[i].data_size = n_records;
  if (records_use_p_for_enc) {
    dv[i++].flags |= DATA_USE_P_FOR_ENC;
  } else {
    dv[i++].flags &= ~DATA_USE_P_FOR_ENC;
  }
  if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
    /* section IDs */
    dv[i].data_size = n_records;
    if (sections_use_p_for_enc) {
      dv[i++].flags |= DATA_USE_P_FOR_ENC;
    } else {
      dv[i++].flags &= ~DATA_USE_P_FOR_ENC;
    }
  }
  /* term frequencies */
  dv[i].data_size = n_records;
  if (sections_use_p_for_enc) {
    dv[i++].flags |= DATA_USE_P_FOR_ENC;
  } else {
    dv[i++].flags &= ~DATA_USE_P_FOR_ENC;
  }
  if ((ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
    /* weights */
    dv[i].data_size = n_records;
    if (sections_use_p_for_enc) {
      dv[i++].flags |= DATA_USE_P_FOR_ENC;
    } else {
      dv[i++].flags &= ~DATA_USE_P_FOR_ENC;
    }
  }
  if (ii->header.common->flags & GRN_OBJ_WITH_POSITION) {
    /* positions */
    dv[i].data_size = n_positions;
    if (data_positions_use_p_for_enc(n_positions, max_position)) {
      dv[i++].flags |= DATA_USE_P_FOR_ENC;
    } else {
      dv[i++].flags &= ~DATA_USE_P_FOR_ENC;
    }
  }
}

typedef struct {
  grn_ii *ii;
  grn_id term_id;
  uint32_t nth_element;
  uint32_t n_odd_values;
  uint32_t *values;
  uint32_t n_values;
  uint32_t flags;
  size_t offset;
  size_t size;
  /* Only for encoder */
  uint8_t *output;
  /* Only for decoder */
  const uint8_t *input;
} grn_codec_data;

static inline void
grn_codec_error(grn_ctx *ctx,
                grn_rc rc,
                grn_codec_data *data,
                const char *message,
                const char *file,
                uint32_t line,
                const char *function)
{
  grn_obj term;
  GRN_DEFINE_NAME(data->ii);
  GRN_TEXT_INIT(&term, 0);
  if (data->term_id != GRN_ID_NIL) {
    grn_ii_get_term(ctx, data->ii, data->term_id, &term);
  }
  grn_error_set(ctx,
                GRN_ERROR,
                rc,
                file,
                line,
                function,
                "%s: "
                "<%.*s>: "
                "<%.*s>(%u): "
                "n-values(%u): "
                "n-odd-values(%u): "
                "dv[%u/%u]: "
                "data[%" GRN_FMT_SIZE "/%" GRN_FMT_SIZE "]",
                message,
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                data->term_id,
                data->n_values,
                data->n_odd_values,
                data->nth_element,
                data->ii->n_elements,
                data->offset,
                data->size);
  GRN_OBJ_FIN(ctx, &term);
}

static inline size_t
grn_enc_estimate_byte(grn_ctx *ctx, grn_codec_data *data)
{
  return GRN_B_ENC_MAX_SIZE * data->n_values;
}

static inline uint8_t *
grn_enc_byte(grn_ctx *ctx, grn_codec_data *data)
{
  uint8_t *output = data->output;
  uint32_t i;
  for (i = 0; i < data->n_values; i++) {
    GRN_B_ENC(data->values[i], output);
  }
  return output;
}

static inline const uint8_t *
grn_dec_byte_check(grn_ctx *ctx,
                   grn_codec_data *data,
                   uint32_t *value,
                   const uint8_t *input,
                   const uint8_t *input_end)
{
  if (input >= input_end) {
    grn_codec_error(ctx,
                    GRN_FILE_CORRUPT,
                    data,
                    "[ii][dec][byte] "
                    "input is short: at least 1 byte is needed",
                    __FILE__,
                    __LINE__,
                    __FUNCTION__);
    return NULL;
  }
  uint32_t v = *input++;
  data->offset++;
  switch (v >> 4) {
  case 0x08:
    if (v == 0x8f) {
      if (input + sizeof(uint32_t) > input_end) {
        grn_codec_error(ctx,
                        GRN_FILE_CORRUPT,
                        data,
                        "[ii][dec][byte] "
                        "input is short: at least more 4 bytes are needed",
                        __FILE__,
                        __LINE__,
                        __FUNCTION__);
        return NULL;
      }
      grn_memcpy(&v, input, sizeof(uint32_t));
      input += sizeof(uint32_t);
      data->offset += sizeof(uint32_t);
    }
    break;
  case 0x09:
    if (input + 3 > input_end) {
      grn_codec_error(ctx,
                      GRN_FILE_CORRUPT,
                      data,
                      "[ii][dec][byte] "
                      "input is short: at least more 3 bytes are needed",
                      __FILE__,
                      __LINE__,
                      __FUNCTION__);
      return NULL;
    }
    v = (v - 0x90) * 0x100 + *input++;
    v = v * 0x100 + *input++;
    v = v * 0x100 + *input++ + 0x20408f;
    data->offset += 3;
    break;
  case 0x0a:
  case 0x0b:
    if (input + 2 > input_end) {
      grn_codec_error(ctx,
                      GRN_FILE_CORRUPT,
                      data,
                      "[ii][dec][byte] "
                      "input is short: at least more 2 bytes are needed",
                      __FILE__,
                      __LINE__,
                      __FUNCTION__);
      return NULL;
    }
    v = (v - 0xa0) * 0x100 + *input++;
    v = v * 0x100 + *input++ + 0x408f;
    data->offset += 2;
    break;
  case 0x0c:
  case 0x0d:
  case 0x0e:
  case 0x0f:
    if (input + 1 > input_end) {
      grn_codec_error(ctx,
                      GRN_FILE_CORRUPT,
                      data,
                      "[ii][dec][byte] "
                      "input is short: at least more 1 byte is needed",
                      __FILE__,
                      __LINE__,
                      __FUNCTION__);
      return NULL;
    }
    v = (v - 0xc0) * 0x100 + *input++ + 0x8f;
    break;
  }
  *value = v;
  return input;
}

static inline const uint8_t *
grn_dec_byte(grn_ctx *ctx, grn_codec_data *data)
{
  const uint8_t *input = data->input;
  const uint8_t *input_end = input + data->size;
  uint32_t i;
  for (i = 0; i < data->n_values && input; i++) {
    input = grn_dec_byte_check(ctx, data, &(data->values[i]), input, input_end);
  }
  return input;
}

/* PFor implementation: start */

/*
 * PFor (Patched Frame of Reference) encode: M. Zukowski , S. Heman,
 * N. Nes, P. Boncz, Super-Scalar RAM-CPU Cache Compression,
 * Proceedings of the 22nd International Conference on Data
 * Engineering, 2006
 *
 * Delta decoding must be done by caller. So pack()/unpack() only do
 * PFor (not PForDelta).
 */

/*
 * We use PForDelta not NewPForDelta nor OptPForDelta because "An
 * Experimental Study of Bitmap Compression vs. Inverted List
 * Compression"
 * https://www.cs.purdue.edu/homes/csjgwang/pubs/SIGMOD17-Bitmap.pdf
 * shows that PForDelta is faster than others. (They are smaller than
 * PForDelta.)
 *
 * We use PForDelta not Roaring bitmaps because PForDelta is faster
 * and smaller for posting list. And we need a cursor interface for
 * searching. Roaring bitmaps' intersection and union may be faster
 * but it's not a cursor interface. See also the above paper.
 */

#define UNIT_SIZE 0x80
#define UNIT_MASK (UNIT_SIZE - 1)

/* clang-format off */
/* <generated> */
static uint8_t *
pack_1(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  v = *p++ << 7;
  v += *p++ << 6;
  v += *p++ << 5;
  v += *p++ << 4;
  v += *p++ << 3;
  v += *p++ << 2;
  v += *p++ << 1;
  *rp++ = v + *p++;
  return rp;
}
static const uint8_t *
unpack_1(uint32_t *p, const uint8_t *dp)
{
  *p++ = (*dp >> 7);
  *p++ = ((*dp >> 6) & 0x1);
  *p++ = ((*dp >> 5) & 0x1);
  *p++ = ((*dp >> 4) & 0x1);
  *p++ = ((*dp >> 3) & 0x1);
  *p++ = ((*dp >> 2) & 0x1);
  *p++ = ((*dp >> 1) & 0x1);
  *p++ = (*dp++ & 0x1);
  return dp;
}
static uint8_t *
pack_2(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  v = *p++ << 6;
  v += *p++ << 4;
  v += *p++ << 2;
  *rp++ = v + *p++;
  v = *p++ << 6;
  v += *p++ << 4;
  v += *p++ << 2;
  *rp++ = v + *p++;
  return rp;
}
static const uint8_t *
unpack_2(uint32_t *p, const uint8_t *dp)
{
  *p++ = (*dp >> 6);
  *p++ = ((*dp >> 4) & 0x3);
  *p++ = ((*dp >> 2) & 0x3);
  *p++ = (*dp++ & 0x3);
  *p++ = (*dp >> 6);
  *p++ = ((*dp >> 4) & 0x3);
  *p++ = ((*dp >> 2) & 0x3);
  *p++ = (*dp++ & 0x3);
  return dp;
}
static uint8_t *
pack_3(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  v = *p++ << 5;
  v += *p++ << 2;
  *rp++ = v + (*p >> 1); v = *p++ << 7;
  v += *p++ << 4;
  v += *p++ << 1;
  *rp++ = v + (*p >> 2); v = *p++ << 6;
  v += *p++ << 3;
  *rp++ = v + *p++;
  return rp;
}
static const uint8_t *
unpack_3(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  *p++ = (*dp >> 5);
  *p++ = ((*dp >> 2) & 0x7);
  v = ((*dp++ << 1) & 0x7); *p++ = v + (*dp >> 7);
  *p++ = ((*dp >> 4) & 0x7);
  *p++ = ((*dp >> 1) & 0x7);
  v = ((*dp++ << 2) & 0x7); *p++ = v + (*dp >> 6);
  *p++ = ((*dp >> 3) & 0x7);
  *p++ = (*dp++ & 0x7);
  return dp;
}
static uint8_t *
pack_4(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  v = *p++ << 4;
  *rp++ = v + *p++;
  v = *p++ << 4;
  *rp++ = v + *p++;
  v = *p++ << 4;
  *rp++ = v + *p++;
  v = *p++ << 4;
  *rp++ = v + *p++;
  return rp;
}
static const uint8_t *
unpack_4(uint32_t *p, const uint8_t *dp)
{
  *p++ = (*dp >> 4);
  *p++ = (*dp++ & 0xf);
  *p++ = (*dp >> 4);
  *p++ = (*dp++ & 0xf);
  *p++ = (*dp >> 4);
  *p++ = (*dp++ & 0xf);
  *p++ = (*dp >> 4);
  *p++ = (*dp++ & 0xf);
  return dp;
}
static uint8_t *
pack_5(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  v = *p++ << 3;
  *rp++ = v + (*p >> 2); v = *p++ << 6;
  v += *p++ << 1;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 1); v = *p++ << 7;
  v += *p++ << 2;
  *rp++ = v + (*p >> 3); v = *p++ << 5;
  *rp++ = v + *p++;
  return rp;
}
static const uint8_t *
unpack_5(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  *p++ = (*dp >> 3);
  v = ((*dp++ << 2) & 0x1f); *p++ = v + (*dp >> 6);
  *p++ = ((*dp >> 1) & 0x1f);
  v = ((*dp++ << 4) & 0x1f); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 1) & 0x1f); *p++ = v + (*dp >> 7);
  *p++ = ((*dp >> 2) & 0x1f);
  v = ((*dp++ << 3) & 0x1f); *p++ = v + (*dp >> 5);
  *p++ = (*dp++ & 0x1f);
  return dp;
}
static uint8_t *
pack_6(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  v = *p++ << 2;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 2); v = *p++ << 6;
  *rp++ = v + *p++;
  v = *p++ << 2;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 2); v = *p++ << 6;
  *rp++ = v + *p++;
  return rp;
}
static const uint8_t *
unpack_6(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  *p++ = (*dp >> 2);
  v = ((*dp++ << 4) & 0x3f); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 2) & 0x3f); *p++ = v + (*dp >> 6);
  *p++ = (*dp++ & 0x3f);
  *p++ = (*dp >> 2);
  v = ((*dp++ << 4) & 0x3f); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 2) & 0x3f); *p++ = v + (*dp >> 6);
  *p++ = (*dp++ & 0x3f);
  return dp;
}
static uint8_t *
pack_7(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  v = *p++ << 1;
  *rp++ = v + (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 1); v = *p++ << 7;
  *rp++ = v + *p++;
  return rp;
}
static const uint8_t *
unpack_7(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  *p++ = (*dp >> 1);
  v = ((*dp++ << 6) & 0x7f); *p++ = v + (*dp >> 2);
  v = ((*dp++ << 5) & 0x7f); *p++ = v + (*dp >> 3);
  v = ((*dp++ << 4) & 0x7f); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 3) & 0x7f); *p++ = v + (*dp >> 5);
  v = ((*dp++ << 2) & 0x7f); *p++ = v + (*dp >> 6);
  v = ((*dp++ << 1) & 0x7f); *p++ = v + (*dp >> 7);
  *p++ = (*dp++ & 0x7f);
  return dp;
}
static uint8_t *
pack_8(uint32_t *p, uint8_t *rp)
{
  *rp++ = *p++;
  *rp++ = *p++;
  *rp++ = *p++;
  *rp++ = *p++;
  *rp++ = *p++;
  *rp++ = *p++;
  *rp++ = *p++;
  *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_8(uint32_t *p, const uint8_t *dp)
{
  *p++ = *dp++;
  *p++ = *dp++;
  *p++ = *dp++;
  *p++ = *dp++;
  *p++ = *dp++;
  *p++ = *dp++;
  *p++ = *dp++;
  *p++ = *dp++;
  return dp;
}
static uint8_t *
pack_9(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_9(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 2) & 0x1ff); *p++ = v + (*dp >> 6);
  v = ((*dp++ << 3) & 0x1ff); *p++ = v + (*dp >> 5);
  v = ((*dp++ << 4) & 0x1ff); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 5) & 0x1ff); *p++ = v + (*dp >> 3);
  v = ((*dp++ << 6) & 0x1ff); *p++ = v + (*dp >> 2);
  v = ((*dp++ << 7) & 0x1ff); *p++ = v + (*dp >> 1);
  v = ((*dp++ << 8) & 0x1ff); *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_10(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_10(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 4) & 0x3ff); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 6) & 0x3ff); *p++ = v + (*dp >> 2);
  v = ((*dp++ << 8) & 0x3ff); *p++ = v + *dp++;
  v = *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 4) & 0x3ff); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 6) & 0x3ff); *p++ = v + (*dp >> 2);
  v = ((*dp++ << 8) & 0x3ff); *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_11(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_11(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 6) & 0x7ff); *p++ = v + (*dp >> 2);
  v = ((*dp++ << 9) & 0x7ff); v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 4) & 0x7ff); *p++ = v + (*dp >> 4);
  v = ((*dp++ << 7) & 0x7ff); *p++ = v + (*dp >> 1);
  v = ((*dp++ << 10) & 0x7ff); v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 5) & 0x7ff); *p++ = v + (*dp >> 3);
  v = ((*dp++ << 8) & 0x7ff); *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_12(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_12(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 8) & 0xfff); *p++ = v + *dp++;
  v = *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 8) & 0xfff); *p++ = v + *dp++;
  v = *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 8) & 0xfff); *p++ = v + *dp++;
  v = *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 8) & 0xfff); *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_13(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_13(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 5; *p++ = v + (*dp >> 3);
  v = ((*dp++ << 10) & 0x1fff); v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 7) & 0x1fff); *p++ = v + (*dp >> 1);
  v = ((*dp++ << 12) & 0x1fff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 9) & 0x1fff); v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 6) & 0x1fff); *p++ = v + (*dp >> 2);
  v = ((*dp++ << 11) & 0x1fff); v += *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 8) & 0x1fff); *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_14(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_14(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 12) & 0x3fff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 10) & 0x3fff); v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 8) & 0x3fff); *p++ = v + *dp++;
  v = *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 12) & 0x3fff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 10) & 0x3fff); v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 8) & 0x3fff); *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_15(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_15(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 7; *p++ = v + (*dp >> 1);
  v = ((*dp++ << 14) & 0x7fff); v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 13) & 0x7fff); v += *dp++ << 5; *p++ = v + (*dp >> 3);
  v = ((*dp++ << 12) & 0x7fff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 11) & 0x7fff); v += *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 10) & 0x7fff); v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 9) & 0x7fff); v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 8) & 0x7fff); *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_16(uint32_t *p, uint8_t *rp)
{
  *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_16(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_17(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_17(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 9; v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 10) & 0x1ffff); v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 11) & 0x1ffff); v += *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 12) & 0x1ffff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 13) & 0x1ffff); v += *dp++ << 5; *p++ = v + (*dp >> 3);
  v = ((*dp++ << 14) & 0x1ffff); v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 15) & 0x1ffff); v += *dp++ << 7; *p++ = v + (*dp >> 1);
  v = ((*dp++ << 16) & 0x1ffff); v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_18(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_18(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 10; v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 12) & 0x3ffff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 14) & 0x3ffff); v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 16) & 0x3ffff); v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 10; v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 12) & 0x3ffff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 14) & 0x3ffff); v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 16) & 0x3ffff); v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_19(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 17); *rp++ = (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_19(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 11; v += *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 14) & 0x7ffff); v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 17) & 0x7ffff); v += *dp++ << 9; v += *dp++ << 1;
  *p++ = v + (*dp >> 7);
  v = ((*dp++ << 12) & 0x7ffff); v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 15) & 0x7ffff); v += *dp++ << 7; *p++ = v + (*dp >> 1);
  v = ((*dp++ << 18) & 0x7ffff); v += *dp++ << 10; v += *dp++ << 2;
  *p++ = v + (*dp >> 6);
  v = ((*dp++ << 13) & 0x7ffff); v += *dp++ << 5; *p++ = v + (*dp >> 3);
  v = ((*dp++ << 16) & 0x7ffff); v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_20(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_20(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 16) & 0xfffff); v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 16) & 0xfffff); v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 16) & 0xfffff); v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 16) & 0xfffff); v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_21(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 17); *rp++ = (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 19); *rp++ = (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_21(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 13; v += *dp++ << 5; *p++ = v + (*dp >> 3);
  v = ((*dp++ << 18) & 0x1fffff); v += *dp++ << 10; v += *dp++ << 2;
  *p++ = v + (*dp >> 6);
  v = ((*dp++ << 15) & 0x1fffff); v += *dp++ << 7; *p++ = v + (*dp >> 1);
  v = ((*dp++ << 20) & 0x1fffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 17) & 0x1fffff); v += *dp++ << 9; v += *dp++ << 1;
  *p++ = v + (*dp >> 7);
  v = ((*dp++ << 14) & 0x1fffff); v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 19) & 0x1fffff); v += *dp++ << 11; v += *dp++ << 3;
  *p++ = v + (*dp >> 5);
  v = ((*dp++ << 16) & 0x1fffff); v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_22(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_22(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 14; v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 20) & 0x3fffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 18) & 0x3fffff); v += *dp++ << 10; v += *dp++ << 2;
  *p++ = v + (*dp >> 6);
  v = ((*dp++ << 16) & 0x3fffff); v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 14; v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 20) & 0x3fffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 18) & 0x3fffff); v += *dp++ << 10; v += *dp++ << 2;
  *p++ = v + (*dp >> 6);
  v = ((*dp++ << 16) & 0x3fffff); v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_23(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 21); *rp++ = (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 19); *rp++ = (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 17); *rp++ = (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_23(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 15; v += *dp++ << 7; *p++ = v + (*dp >> 1);
  v = ((*dp++ << 22) & 0x7fffff); v += *dp++ << 14; v += *dp++ << 6;
  *p++ = v + (*dp >> 2);
  v = ((*dp++ << 21) & 0x7fffff); v += *dp++ << 13; v += *dp++ << 5;
  *p++ = v + (*dp >> 3);
  v = ((*dp++ << 20) & 0x7fffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 19) & 0x7fffff); v += *dp++ << 11; v += *dp++ << 3;
  *p++ = v + (*dp >> 5);
  v = ((*dp++ << 18) & 0x7fffff); v += *dp++ << 10; v += *dp++ << 2;
  *p++ = v + (*dp >> 6);
  v = ((*dp++ << 17) & 0x7fffff); v += *dp++ << 9; v += *dp++ << 1;
  *p++ = v + (*dp >> 7);
  v = ((*dp++ << 16) & 0x7fffff); v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_24(uint32_t *p, uint8_t *rp)
{
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_24(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_25(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 17); *rp++ = (*p >> 9); *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 19); *rp++ = (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 21); *rp++ = (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 23); *rp++ = (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_25(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 17; v += *dp++ << 9; v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 18) & 0x1ffffff); v += *dp++ << 10; v += *dp++ << 2;
  *p++ = v + (*dp >> 6);
  v = ((*dp++ << 19) & 0x1ffffff); v += *dp++ << 11; v += *dp++ << 3;
  *p++ = v + (*dp >> 5);
  v = ((*dp++ << 20) & 0x1ffffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 21) & 0x1ffffff); v += *dp++ << 13; v += *dp++ << 5;
  *p++ = v + (*dp >> 3);
  v = ((*dp++ << 22) & 0x1ffffff); v += *dp++ << 14; v += *dp++ << 6;
  *p++ = v + (*dp >> 2);
  v = ((*dp++ << 23) & 0x1ffffff); v += *dp++ << 15; v += *dp++ << 7;
  *p++ = v + (*dp >> 1);
  v = ((*dp++ << 24) & 0x1ffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_26(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 18); *rp++ = (*p >> 10); *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_26(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 18; v += *dp++ << 10; v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 20) & 0x3ffffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 22) & 0x3ffffff); v += *dp++ << 14; v += *dp++ << 6;
  *p++ = v + (*dp >> 2);
  v = ((*dp++ << 24) & 0x3ffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  v = *dp++ << 18; v += *dp++ << 10; v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 20) & 0x3ffffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 22) & 0x3ffffff); v += *dp++ << 14; v += *dp++ << 6;
  *p++ = v + (*dp >> 2);
  v = ((*dp++ << 24) & 0x3ffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_27(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 19); *rp++ = (*p >> 11); *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 25); *rp++ = (*p >> 17); *rp++ = (*p >> 9);
  *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 23); *rp++ = (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 26); *rp++ = (*p >> 18); *rp++ = (*p >> 10);
  *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 21); *rp++ = (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_27(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 19; v += *dp++ << 11; v += *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 22) & 0x7ffffff); v += *dp++ << 14; v += *dp++ << 6;
  *p++ = v + (*dp >> 2);
  v = ((*dp++ << 25) & 0x7ffffff); v += *dp++ << 17; v += *dp++ << 9;
  v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 20) & 0x7ffffff); v += *dp++ << 12; v += *dp++ << 4;
  *p++ = v + (*dp >> 4);
  v = ((*dp++ << 23) & 0x7ffffff); v += *dp++ << 15; v += *dp++ << 7;
  *p++ = v + (*dp >> 1);
  v = ((*dp++ << 26) & 0x7ffffff); v += *dp++ << 18; v += *dp++ << 10;
  v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 21) & 0x7ffffff); v += *dp++ << 13; v += *dp++ << 5;
  *p++ = v + (*dp >> 3);
  v = ((*dp++ << 24) & 0x7ffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_28(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 20); *rp++ = (*p >> 12); *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_28(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 20; v += *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 24) & 0xfffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  v = *dp++ << 20; v += *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 24) & 0xfffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  v = *dp++ << 20; v += *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 24) & 0xfffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  v = *dp++ << 20; v += *dp++ << 12; v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 24) & 0xfffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_29(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 21); *rp++ = (*p >> 13); *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 26); *rp++ = (*p >> 18); *rp++ = (*p >> 10);
  *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 23); *rp++ = (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 28); *rp++ = (*p >> 20); *rp++ = (*p >> 12);
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 25); *rp++ = (*p >> 17); *rp++ = (*p >> 9);
  *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 27); *rp++ = (*p >> 19); *rp++ = (*p >> 11);
  *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_29(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 21; v += *dp++ << 13; v += *dp++ << 5; *p++ = v + (*dp >> 3);
  v = ((*dp++ << 26) & 0x1fffffff); v += *dp++ << 18; v += *dp++ << 10;
  v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 23) & 0x1fffffff); v += *dp++ << 15; v += *dp++ << 7;
  *p++ = v + (*dp >> 1);
  v = ((*dp++ << 28) & 0x1fffffff); v += *dp++ << 20; v += *dp++ << 12;
  v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 25) & 0x1fffffff); v += *dp++ << 17; v += *dp++ << 9;
  v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 22) & 0x1fffffff); v += *dp++ << 14; v += *dp++ << 6;
  *p++ = v + (*dp >> 2);
  v = ((*dp++ << 27) & 0x1fffffff); v += *dp++ << 19; v += *dp++ << 11;
  v += *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 24) & 0x1fffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_30(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 28); *rp++ = (*p >> 20); *rp++ = (*p >> 12);
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 26); *rp++ = (*p >> 18); *rp++ = (*p >> 10);
  *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 22); *rp++ = (*p >> 14); *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 28); *rp++ = (*p >> 20); *rp++ = (*p >> 12);
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 26); *rp++ = (*p >> 18); *rp++ = (*p >> 10);
  *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8);
  *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_30(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 22; v += *dp++ << 14; v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 28) & 0x3fffffff); v += *dp++ << 20; v += *dp++ << 12;
  v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 26) & 0x3fffffff); v += *dp++ << 18; v += *dp++ << 10;
  v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 24) & 0x3fffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  v = *dp++ << 22; v += *dp++ << 14; v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 28) & 0x3fffffff); v += *dp++ << 20; v += *dp++ << 12;
  v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 26) & 0x3fffffff); v += *dp++ << 18; v += *dp++ << 10;
  v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 24) & 0x3fffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_31(uint32_t *p, uint8_t *rp)
{
  uint8_t v;
  *rp++ = (*p >> 23); *rp++ = (*p >> 15); *rp++ = (*p >> 7); v = *p++ << 1;
  *rp++ = v + (*p >> 30); *rp++ = (*p >> 22); *rp++ = (*p >> 14);
  *rp++ = (*p >> 6); v = *p++ << 2;
  *rp++ = v + (*p >> 29); *rp++ = (*p >> 21); *rp++ = (*p >> 13);
  *rp++ = (*p >> 5); v = *p++ << 3;
  *rp++ = v + (*p >> 28); *rp++ = (*p >> 20); *rp++ = (*p >> 12);
  *rp++ = (*p >> 4); v = *p++ << 4;
  *rp++ = v + (*p >> 27); *rp++ = (*p >> 19); *rp++ = (*p >> 11);
  *rp++ = (*p >> 3); v = *p++ << 5;
  *rp++ = v + (*p >> 26); *rp++ = (*p >> 18); *rp++ = (*p >> 10);
  *rp++ = (*p >> 2); v = *p++ << 6;
  *rp++ = v + (*p >> 25); *rp++ = (*p >> 17); *rp++ = (*p >> 9);
  *rp++ = (*p >> 1); v = *p++ << 7;
  *rp++ = v + (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8);
  *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_31(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 23; v += *dp++ << 15; v += *dp++ << 7; *p++ = v + (*dp >> 1);
  v = ((*dp++ << 30) & 0x7fffffff); v += *dp++ << 22; v += *dp++ << 14;
  v += *dp++ << 6; *p++ = v + (*dp >> 2);
  v = ((*dp++ << 29) & 0x7fffffff); v += *dp++ << 21; v += *dp++ << 13;
  v += *dp++ << 5; *p++ = v + (*dp >> 3);
  v = ((*dp++ << 28) & 0x7fffffff); v += *dp++ << 20; v += *dp++ << 12;
  v += *dp++ << 4; *p++ = v + (*dp >> 4);
  v = ((*dp++ << 27) & 0x7fffffff); v += *dp++ << 19; v += *dp++ << 11;
  v += *dp++ << 3; *p++ = v + (*dp >> 5);
  v = ((*dp++ << 26) & 0x7fffffff); v += *dp++ << 18; v += *dp++ << 10;
  v += *dp++ << 2; *p++ = v + (*dp >> 6);
  v = ((*dp++ << 25) & 0x7fffffff); v += *dp++ << 17; v += *dp++ << 9;
  v += *dp++ << 1; *p++ = v + (*dp >> 7);
  v = ((*dp++ << 24) & 0x7fffffff); v += *dp++ << 16; v += *dp++ << 8;
  *p++ = v + *dp++;
  return dp;
}
static uint8_t *
pack_32(uint32_t *p, uint8_t *rp)
{
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  *rp++ = (*p >> 24); *rp++ = (*p >> 16); *rp++ = (*p >> 8); *rp++ = *p++;
  return rp;
}
static const uint8_t *
unpack_32(uint32_t *p, const uint8_t *dp)
{
  uint32_t v;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  v = *dp++ << 24; v += *dp++ << 16; v += *dp++ << 8; *p++ = v + *dp++;
  return dp;
}
/* </generated> */
/* clang-format on */

static uint8_t *
pack_(uint32_t *p, uint32_t i, int w, uint8_t *rp)
{
  while (i >= 8) {
    /* clang-format off */
    switch (w) {
    case  0 : break;
    case  1 : rp = pack_1(p, rp); break;
    case  2 : rp = pack_2(p, rp); break;
    case  3 : rp = pack_3(p, rp); break;
    case  4 : rp = pack_4(p, rp); break;
    case  5 : rp = pack_5(p, rp); break;
    case  6 : rp = pack_6(p, rp); break;
    case  7 : rp = pack_7(p, rp); break;
    case  8 : rp = pack_8(p, rp); break;
    case  9 : rp = pack_9(p, rp); break;
    case 10 : rp = pack_10(p, rp); break;
    case 11 : rp = pack_11(p, rp); break;
    case 12 : rp = pack_12(p, rp); break;
    case 13 : rp = pack_13(p, rp); break;
    case 14 : rp = pack_14(p, rp); break;
    case 15 : rp = pack_15(p, rp); break;
    case 16 : rp = pack_16(p, rp); break;
    case 17 : rp = pack_17(p, rp); break;
    case 18 : rp = pack_18(p, rp); break;
    case 19 : rp = pack_19(p, rp); break;
    case 20 : rp = pack_20(p, rp); break;
    case 21 : rp = pack_21(p, rp); break;
    case 22 : rp = pack_22(p, rp); break;
    case 23 : rp = pack_23(p, rp); break;
    case 24 : rp = pack_24(p, rp); break;
    case 25 : rp = pack_25(p, rp); break;
    case 26 : rp = pack_26(p, rp); break;
    case 27 : rp = pack_27(p, rp); break;
    case 28 : rp = pack_28(p, rp); break;
    case 29 : rp = pack_29(p, rp); break;
    case 30 : rp = pack_30(p, rp); break;
    case 31 : rp = pack_31(p, rp); break;
    case 32 : rp = pack_32(p, rp); break;
    }
    /* clang-format on */
    p += 8;
    i -= 8;
  }
  {
    int b;
    uint8_t v;
    uint32_t *pe = p + i;
    for (b = 8 - w, v = 0; p < pe;) {
      if (b > 0) {
        v += *p++ << b;
        b -= w;
      } else if (b < 0) {
        *rp++ = v + (*p >> -b);
        b += 8;
        v = 0;
      } else {
        *rp++ = v + *p++;
        b = 8 - w;
        v = 0;
      }
    }
    if (b + w != 8) {
      *rp++ = v;
    }
    return rp;
  }
}

/* TODO: This is too rough. I hope that this is large enough. */
#define PACK_MAX_SIZE ((UNIT_SIZE / 8) * 32 + (UNIT_SIZE * GRN_B_ENC_MAX_SIZE))

/*
 * w: b on the paper.
 * ebuf, ep: exception values on the paper.
 */
static uint8_t *
pack(uint32_t *p, uint32_t i, uint8_t *freq, uint8_t *rp)
{
  int32_t k, w;
  uint8_t ebuf[UNIT_SIZE], *ep = ebuf;
  uint32_t s, *pe = p + i, r, th = i - (i >> 3);
  for (w = 0, s = 0; w <= 32; w++) {
    if ((s += freq[w]) >= th) {
      break;
    }
  }
  if (i == s) {
    *rp++ = w;
    return pack_(p, i, w, rp);
  }
  r = 1 << w;
  *rp++ = w + 0x80;
  *rp++ = i - s;
  if (r >= UNIT_SIZE) {
    uint32_t first, *last = &first;
    for (k = 0; p < pe; p++, k++) {
      if (*p >= r) {
        GRN_B_ENC(*p - r, ep);
        *last = k;
        last = p;
      }
    }
    *last = 0;
    *rp++ = (uint8_t)first;
  } else {
    for (k = 0; p < pe; p++, k++) {
      if (*p >= r) {
        *ep++ = k;
        GRN_B_ENC(*p - r, ep);
        *p = 0;
      }
    }
  }
  rp = pack_(p - i, i, w, rp);
  grn_memcpy(rp, ebuf, ep - ebuf);
  return rp + (ep - ebuf);
}

/*
 * w: b on the paper.
 * ne: the number of exception values on the paper.
 */
static const uint8_t *
unpack(grn_ctx *ctx,
       grn_codec_data *data,
       const uint8_t *dp,
       const uint8_t *dpe,
       int i,
       uint32_t *rp)
{
  uint8_t ne = 0, k = 0, w = *dp++;
  uint32_t m, *p = rp;
  if (w & 0x80) {
    ne = *dp++;
    w -= 0x80;
    m = (1 << w) - 1;
    if (m >= UNIT_MASK) {
      k = *dp++;
    }
  } else {
    m = (1 << w) - 1;
  }
  if (w) {
    while (i >= 8) {
      if (dp + w > dpe) {
        return NULL;
      }
      /* clang-format off */
      switch (w) {
      case 1 : dp = unpack_1(p, dp); break;
      case 2 : dp = unpack_2(p, dp); break;
      case 3 : dp = unpack_3(p, dp); break;
      case 4 : dp = unpack_4(p, dp); break;
      case 5 : dp = unpack_5(p, dp); break;
      case 6 : dp = unpack_6(p, dp); break;
      case 7 : dp = unpack_7(p, dp); break;
      case 8 : dp = unpack_8(p, dp); break;
      case 9 : dp = unpack_9(p, dp); break;
      case 10 : dp = unpack_10(p, dp); break;
      case 11 : dp = unpack_11(p, dp); break;
      case 12 : dp = unpack_12(p, dp); break;
      case 13 : dp = unpack_13(p, dp); break;
      case 14 : dp = unpack_14(p, dp); break;
      case 15 : dp = unpack_15(p, dp); break;
      case 16 : dp = unpack_16(p, dp); break;
      case 17 : dp = unpack_17(p, dp); break;
      case 18 : dp = unpack_18(p, dp); break;
      case 19 : dp = unpack_19(p, dp); break;
      case 20 : dp = unpack_20(p, dp); break;
      case 21 : dp = unpack_21(p, dp); break;
      case 22 : dp = unpack_22(p, dp); break;
      case 23 : dp = unpack_23(p, dp); break;
      case 24 : dp = unpack_24(p, dp); break;
      case 25 : dp = unpack_25(p, dp); break;
      case 26 : dp = unpack_26(p, dp); break;
      case 27 : dp = unpack_27(p, dp); break;
      case 28 : dp = unpack_28(p, dp); break;
      case 29 : dp = unpack_29(p, dp); break;
      case 30 : dp = unpack_30(p, dp); break;
      case 31 : dp = unpack_31(p, dp); break;
      case 32 : dp = unpack_32(p, dp); break;
      }
      /* clang-format on */
      i -= 8;
      p += 8;
    }
    {
      int b;
      uint32_t v, *pe;
      for (b = 8 - w, v = 0, pe = p + i; p < pe && dp < dpe;) {
        if (b > 0) {
          *p++ = v + ((*dp >> b) & m);
          b -= w;
          v = 0;
        } else if (b < 0) {
          v += (*dp++ << -b) & m;
          b += 8;
        } else {
          *p++ = v + (*dp++ & m);
          b = 8 - w;
          v = 0;
        }
      }
      if (b + w != 8) {
        dp++;
      }
    }
  } else {
    memset(p, 0, sizeof(uint32_t) * i);
  }
  if (ne) {
    if (m >= UNIT_MASK) {
      uint32_t *pp;
      while (ne-- && dp) {
        pp = &rp[k];
        k = *pp;
        dp = grn_dec_byte_check(ctx, data, pp, dp, dpe);
        *pp += (m + 1);
      }
    } else {
      while (ne-- && dp) {
        k = *dp++;
        dp = grn_dec_byte_check(ctx, data, &(rp[k]), dp, dpe);
        rp[k] += (m + 1);
      }
    }
  }
  return dp;
}

static inline size_t
grn_enc_estimate_p_for(grn_ctx *ctx, grn_codec_data *data)
{
  return PACK_MAX_SIZE * ((data->n_values / UNIT_SIZE) + 1);
}

static inline uint8_t *
grn_enc_p_for(grn_ctx *ctx, grn_codec_data *data)
{
  uint8_t *output = data->output;
  uint32_t *values = data->values;
  uint32_t n_values = data->n_values;
  uint32_t unit[UNIT_SIZE];
  uint8_t freq[33];
  while (n_values >= UNIT_SIZE) {
    memset(freq, 0, 33);
    uint32_t i;
    for (i = 0; i < UNIT_SIZE; i++) {
      unit[i] = values[i];
      if (unit[i]) {
        grn_bit_scan_rev_result w;
        GRN_BIT_SCAN_REV(unit[i], w);
        freq[w + 1]++;
      } else {
        freq[0]++;
      }
    }
    data->offset = output - data->output;
    output = pack(unit, UNIT_SIZE, freq, output);
    values += UNIT_SIZE;
    n_values -= UNIT_SIZE;
  }
  if (n_values > 0) {
    memset(freq, 0, 33);
    uint32_t i;
    for (i = 0; i < n_values; i++) {
      unit[i] = values[i];
      if (unit[i]) {
        grn_bit_scan_rev_result w;
        GRN_BIT_SCAN_REV(unit[i], w);
        freq[w + 1]++;
      } else {
        freq[0]++;
      }
    }
    data->offset = output - data->output;
    output = pack(unit, n_values, freq, output);
  }
  return output;
}

static inline const uint8_t *
grn_dec_p_for(grn_ctx *ctx, grn_codec_data *data)
{
  const uint8_t *input = data->input;
  const uint8_t *input_end = input + data->size;
  uint32_t *values = data->values;
  uint32_t n_values = data->n_values;
  for (; n_values >= UNIT_SIZE; n_values -= UNIT_SIZE) {
    data->offset = input - data->input;
    const uint8_t *next_input =
      unpack(ctx, data, input, input_end, UNIT_SIZE, values);
    if (!next_input) {
      grn_codec_error(ctx,
                      GRN_FILE_CORRUPT,
                      data,
                      "[ii][dec][p-for] "
                      "failed to unpack PFor encoded fixed size data",
                      __FILE__,
                      __LINE__,
                      __FUNCTION__);
      return NULL;
    }
    input = next_input;
    values += UNIT_SIZE;
  }
  if (n_values > 0) {
    data->offset = input - data->input;
    const uint8_t *next_input =
      unpack(ctx, data, input, input_end, n_values, values);
    if (!next_input) {
      grn_codec_error(
        ctx,
        GRN_FILE_CORRUPT,
        data,
        "[ii][dec][p-for] failed to unpack PFor encoded rest data",
        __FILE__,
        __LINE__,
        __FUNCTION__);
      return NULL;
    }
    input = next_input;
    values += n_values;
  }
  return input;
}
/* PFor implementation: end */

/* Binary format used in grn_encv()/grn_decv().
 *
 * df: Data Frequency: The number of postings
 *
 * The uint32_t value encoded by GRN_B_ENC:
 *   +========+========+========+========+
 *   |12345678|12345678|12345678|12345678|
 *   +========+========+========+========+
 *
 * Case 1: If all data don't use PFor:
 *
 *   +========+========+========+========+
 *   |AAAAAAAA|BBBBBBBB|CCCCCCCC|DDDDDDD1| The last bit is important!
 *   +========+========+========+========+
 *   A = ((df << 1) >> 24) & 0xff
 *   B = ((df << 1) >> 16) & 0xff
 *   C = ((df << 1) >> 8)  & 0xff
 *   D = ((df << 1) >> 0)  & 0xff
 *   for (i = 0; i < ii->n_elements; i++) {
 *     for (j = 0; j < dv[i].data_size; j++) {
 *       +========+========+========+========+
 *       |AAAAAAAA|BBBBBBBB|CCCCCCCC|DDDDDDDD| dv[i].data[j]
 *       +========+========+========+========+
 *       A = (dv[i].data[j] >> 24) & 0xff
 *       B = (dv[i].data[j] >> 16) & 0xff
 *       C = (dv[i].data[j] >> 8)  & 0xff
 *       D = (dv[i].data[j] >> 0)  & 0xff
 *     }
 *   }
 *
 * Case 2: If any data use PFor:
 *
 *   use_p_for_flags: It's 32bit flags that show that the i-th data
 *                    uses PFor. For example, 0x02 means that the
 *                    second data only uses PFor.
 *   positions_gap: the_number_positions - df.
 *
 *   +========+========+========+========+
 *   |AAAAAAAA|BBBBBBBB|CCCCCCCC|DDDDDDD0| The last bit is important!
 *   +========+========+========+========+
 *   A = ((use_p_for_flags << 1) >> 24) & 0xff
 *   B = ((use_p_for_flags << 1) >> 16) & 0xff
 *   C = ((use_p_for_flags << 1) >> 8)  & 0xff
 *   D = ((use_p_for_flags << 1) >> 0)  & 0xff
 *   +========+========+========+========+
 *   |AAAAAAAA|BBBBBBBB|CCCCCCCC|DDDDDDDD| df
 *   +========+========+========+========+
 *   A = (df >> 24) & 0xff
 *   B = (df >> 16) & 0xff
 *   C = (df >> 8)  & 0xff
 *   D = (df >> 0)  & 0xff
 *   if (ii has positions) {
 *     +========+========+========+========+
 *     |AAAAAAAA|BBBBBBBB|CCCCCCCC|DDDDDDDD| positions_gap
 *     +========+========+========+========+
 *     A = (pgap >> 24) & 0xff
 *     B = (pgap >> 16) & 0xff
 *     C = (pgap >> 8)  & 0xff
 *     D = (pgap >> 0)  & 0xff
 *   }
 *   for (i = 0; i < ii->n_elements; i++) {
 *     if (dv[i].flags & DATA_USE_P_FOR_ENC) {
 *       Encode dv[i].data by PFor (unit size is 128)
 *     } else {
 *       Same as case 1
 *       for (j = 0; j < dv[i].data_size; j++) {
 *         +========+========+========+========+
 *         |AAAAAAAA|BBBBBBBB|CCCCCCCC|DDDDDDDD| dv[i].data[j]
 *         +========+========+========+========+
 *         A = (dv[i].data[j] >> 24) & 0xff
 *         B = (dv[i].data[j] >> 16) & 0xff
 *         C = (dv[i].data[j] >> 8)  & 0xff
 *         D = (dv[i].data[j] >> 0)  & 0xff
 *       }
 *     }
 *   }
 * */

static ssize_t
grn_encv(grn_ctx *ctx,
         grn_ii *ii,
         grn_id term_id,
         datavec *dv,
         uint8_t *res,
         size_t res_size)
{
  uint8_t *rp = res;
  ssize_t estimated = 0;
  /* f in df is for frequency */
  uint32_t positions_gap, l, df, data_size;
  if (!(df = dv[0].data_size)) {
    return 0;
  }
  grn_codec_data codec_data = {};
  codec_data.ii = ii;
  codec_data.term_id = term_id;
  uint32_t use_p_for_enc_flags = 0;
  for (data_size = 0, l = 0; l < ii->n_elements; l++) {
    uint32_t dl = dv[l].data_size;
    if (dl < df || ((dl > df) && (l != ii->n_elements - 1))) {
      /* invalid argument */
      return -1;
    }
    if (dv[l].flags & DATA_USE_P_FOR_ENC) {
      use_p_for_enc_flags += 1 << l;
    }
    data_size += dl;
  }
  positions_gap = data_size - df * ii->n_elements;
  if (use_p_for_enc_flags == 0) {
    if (rp) {
      GRN_B_ENC((df << 1) + 1, rp);
      for (l = 0; l < ii->n_elements; l++) {
        codec_data.nth_element = l;
        codec_data.values = dv[l].data;
        codec_data.n_values = dv[l].data_size;
        codec_data.flags = dv[l].flags;
        codec_data.offset = 0;
        codec_data.output = rp;
        codec_data.size = res_size - (rp - res);
        rp = grn_enc_byte(ctx, &codec_data);
      }
    } else {
      codec_data.n_values = 1;
      estimated += grn_enc_estimate_byte(ctx, &codec_data);
      for (l = 0; l < ii->n_elements; l++) {
        codec_data.nth_element = l;
        codec_data.values = dv[l].data;
        codec_data.n_values = dv[l].data_size;
        codec_data.flags = dv[l].flags;
        estimated += grn_enc_estimate_byte(ctx, &codec_data);
      }
    }
  } else {
    codec_data.n_values = 2;
    codec_data.flags = 0;
    if (rp) {
      uint32_t values[2];
      values[0] = (use_p_for_enc_flags << 1);
      values[1] = df;
      codec_data.values = values;
      codec_data.offset = 0;
      codec_data.output = rp;
      codec_data.size = res_size - (rp - res);
      rp = grn_enc_byte(ctx, &codec_data);
    } else {
      estimated += grn_enc_estimate_byte(ctx, &codec_data);
    }
    if (dv[ii->n_elements - 1].flags & DATA_ODD) {
      codec_data.n_values = 1;
      if (rp) {
        codec_data.values = &positions_gap;
        codec_data.offset = 0;
        codec_data.output = rp;
        codec_data.size = res_size - (rp - res);
        rp = grn_enc_byte(ctx, &codec_data);
      } else {
        estimated += grn_enc_estimate_byte(ctx, &codec_data);
      }
    } else {
      GRN_ASSERT(positions_gap == 0);
    }
    for (l = 0; l < ii->n_elements; l++) {
      codec_data.nth_element = l;
      codec_data.values = dv[l].data;
      codec_data.n_values = dv[l].data_size;
      codec_data.flags = dv[l].flags;
      codec_data.offset = 0;
      codec_data.output = rp;
      codec_data.size = res_size - (rp - res);
      if (dv[l].flags & DATA_USE_P_FOR_ENC) {
        if (rp) {
          rp = grn_enc_p_for(ctx, &codec_data);
        } else {
          estimated += grn_enc_estimate_p_for(ctx, &codec_data);
        }
      } else {
        if (rp) {
          rp = grn_enc_byte(ctx, &codec_data);
        } else {
          estimated += grn_enc_estimate_byte(ctx, &codec_data);
        }
      }
    }
  }
  if (rp) {
    return rp - res;
  } else {
    return estimated;
  }
}

static int
grn_decv(grn_ctx *ctx,
         grn_ii *ii,
         grn_id term_id,
         const uint8_t *data,
         uint32_t data_size,
         datavec *dv)
{
  size_t size;
  uint32_t df = 0, l, i, *rp;
  const uint8_t *dp = data;
  const uint8_t *dpe = data + data_size;
  if (!data_size) {
    dv[0].data_size = 0;
    return 0;
  }
  grn_codec_data codec_data = {};
  codec_data.ii = ii;
  codec_data.term_id = term_id;
  codec_data.offset = 0;
  codec_data.input = dp;
  codec_data.size = dpe - dp;
  dp = grn_dec_byte_check(ctx, &codec_data, &df, dp, dpe);
  if (!dp) {
    return 0;
  }
  if ((df & 1)) {
    df >>= 1;
    size = data_size;
    if (dv[ii->n_elements].data < dv[0].data + size) {
      if (dv[0].data) {
        GRN_FREE(dv[0].data);
        dv[0].data = NULL;
      }
      if (!(rp =
              static_cast<uint32_t *>(GRN_MALLOC(size * sizeof(uint32_t))))) {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        if (term_id != GRN_ID_NIL) {
          grn_ii_get_term(ctx, ii, term_id, &term);
        }
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "[ii][decv] failed to extend buffer for data vector: "
            "<%.*s>: "
            "<%.*s>(%u): "
            "%" GRN_FMT_SIZE " -> %" GRN_FMT_SIZE ": "
            "PFor(nothing)",
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            term_id,
            dv[ii->n_elements].data - dv[0].data,
            size);
        GRN_OBJ_FIN(ctx, &term);
        return 0;
      }
      dv[ii->n_elements].data = rp + size;
    } else {
      rp = dv[0].data;
    }
    for (l = 0; l < ii->n_elements; l++) {
      dv[l].data = rp;
      codec_data.nth_element = l;
      codec_data.offset = 0;
      codec_data.input = dp;
      codec_data.size = dpe - dp;
      if (l < ii->n_elements - 1) {
        for (i = 0; i < df && dp; i++, rp++) {
          dp = grn_dec_byte_check(ctx, &codec_data, rp, dp, dpe);
        }
      } else {
        for (i = 0; dp < dpe && dp; i++, rp++) {
          dp = grn_dec_byte_check(ctx, &codec_data, rp, dp, dpe);
        }
      }
      if (!dp) {
        return 0;
      }
      dv[l].data_size = i;
    }
  } else {
    uint32_t n, rest;
    uint32_t use_p_for_enc_flags = df >> 1;
    codec_data.offset = 0;
    codec_data.input = dp;
    codec_data.size = dpe - dp;
    dp = grn_dec_byte_check(ctx, &codec_data, &df, dp, dpe);
    if (!dp) {
      return 0;
    }
    if (dv[ii->n_elements - 1].flags & DATA_ODD) {
      codec_data.offset = 0;
      codec_data.input = dp;
      codec_data.size = dpe - dp;
      dp = grn_dec_byte_check(ctx, &codec_data, &rest, dp, dpe);
      if (!dp) {
        return 0;
      }
    } else {
      rest = 0;
    }
    codec_data.n_odd_values = rest;
    size = df * ii->n_elements + rest;
    if (dv[ii->n_elements].data < dv[0].data + size) {
      if (dv[0].data) {
        GRN_FREE(dv[0].data);
        dv[0].data = NULL;
      }
      if (!(rp =
              static_cast<uint32_t *>(GRN_MALLOC(size * sizeof(uint32_t))))) {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        if (term_id != GRN_ID_NIL) {
          grn_ii_get_term(ctx, ii, term_id, &term);
        }
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "[ii][decv] failed to extend buffer for data vector: "
            "<%.*s>: "
            "<%.*s>(%u): "
            "%" GRN_FMT_SIZE " -> %" GRN_FMT_SIZE ": "
            "PFor(%#x)", /* Binary number is better but %b doesn't exist. */
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            term_id,
            dv[ii->n_elements].data - dv[0].data,
            size,
            use_p_for_enc_flags);
        GRN_OBJ_FIN(ctx, &term);
        return 0;
      }
      dv[ii->n_elements].data = rp + size;
    } else {
      rp = dv[0].data;
    }
    for (l = 0; l < ii->n_elements; l++) {
      dv[l].data = rp;
      dv[l].data_size = n = (l < ii->n_elements - 1) ? df : df + rest;
      if (use_p_for_enc_flags & (1 << l)) {
        dv[l].flags |= DATA_USE_P_FOR_ENC;
      } else {
        dv[l].flags &= ~DATA_USE_P_FOR_ENC;
      }
      codec_data.nth_element = l;
      codec_data.values = dv[l].data;
      codec_data.n_values = dv[l].data_size;
      codec_data.flags = dv[l].flags;
      codec_data.offset = 0;
      codec_data.input = dp;
      codec_data.size = dpe - dp;
      if (codec_data.flags & DATA_USE_P_FOR_ENC) {
        dp = grn_dec_p_for(ctx, &codec_data);
      } else {
        dp = grn_dec_byte(ctx, &codec_data);
      }
      if (!dp) {
        return 0;
      }
      rp += dv[l].data_size;
    }
    if (dp != dpe) {
      codec_data.n_values = df;
      codec_data.offset = dp - data;
      codec_data.input = data;
      codec_data.size = data_size;
      grn_codec_error(ctx,
                      GRN_FILE_CORRUPT,
                      &codec_data,
                      "[ii][decv] input size and used size aren't matched",
                      __FILE__,
                      __LINE__,
                      __FUNCTION__);
      return 0;
    }
  }
  return rp - dv[0].data;
}

extern "C" int
grn_b_enc(grn_ctx *ctx, uint32_t *data, uint32_t data_size, uint8_t **res)
{
  uint8_t *rp;
  uint32_t *dp, i;
  *res = rp =
    static_cast<uint8_t *>(GRN_MALLOC(data_size * sizeof(uint32_t) * 2));
  GRN_B_ENC(data_size, rp);
  for (i = data_size, dp = data; i; i--, dp++) {
    GRN_B_ENC(*dp, rp);
  }
  return rp - *res;
}

extern "C" int
grn_b_dec(grn_ctx *ctx, uint8_t *data, uint32_t data_size, uint32_t **res)
{
  uint32_t i, *rp, orig_size;
  uint8_t *dp = data;
  GRN_B_DEC(orig_size, dp);
  *res = rp = static_cast<uint32_t *>(GRN_MALLOC(orig_size * sizeof(uint32_t)));
  for (i = orig_size; i; i--, rp++) {
    GRN_B_DEC(*rp, dp);
  }
  return orig_size;
}

/* buffer */

typedef struct {
  uint32_t tid;
  uint32_t size_in_chunk;
  uint32_t pos_in_chunk;
  uint16_t size_in_buffer;
  uint16_t pos_in_buffer;
} buffer_term;

typedef struct {
  uint16_t step;
  uint16_t jump;
} buffer_rec;

typedef struct {
  uint32_t chunk;
  uint32_t chunk_size;
  uint32_t buffer_free;
  uint16_t nterms;
  uint16_t nterms_void;
} buffer_header;

typedef struct {
  buffer_header header;
  buffer_term terms[(S_SEGMENT - sizeof(buffer_header)) / sizeof(buffer_term)];
} buffer;

#define GRN_II_POS_LSEG_SHIFT_SIZE          (32 - GRN_II_W_LSEG)
#define GRN_II_POS_LSEG_SHIFT_SIZE_LARGE    (32 - GRN_II_W_LSEG_LARGE)
#define GRN_II_POS_LOFFSET_SHIFT_SIZE       ((16 - GRN_II_W_LOFFSET) + 2)
#define GRN_II_POS_LOFFSET_SHIFT_SIZE_LARGE ((16 - GRN_II_W_LOFFSET_LARGE) + 2)

static inline uint32_t
grn_ii_pos_lseg(grn_ii *ii, uint32_t pos)
{
  if (ii->header.common->flags & GRN_OBJ_INDEX_LARGE) {
    return pos >> GRN_II_POS_LSEG_SHIFT_SIZE_LARGE;
  } else {
    return pos >> GRN_II_POS_LSEG_SHIFT_SIZE;
  }
}

static inline uint32_t
grn_ii_pos_loffset(grn_ii *ii, uint32_t pos)
{
  if (ii->header.common->flags & GRN_OBJ_INDEX_LARGE) {
    return (pos & ((1 << GRN_II_W_LOFFSET_LARGE) - 1))
           << GRN_II_POS_LOFFSET_SHIFT_SIZE_LARGE;
  } else {
    return (pos & ((1 << GRN_II_W_LOFFSET) - 1))
           << GRN_II_POS_LOFFSET_SHIFT_SIZE;
  }
}

static inline uint32_t
grn_ii_pos_pack(grn_ii *ii, uint32_t lseg, uint32_t loffset)
{
  if (ii->header.common->flags & GRN_OBJ_INDEX_LARGE) {
    return (lseg << GRN_II_POS_LSEG_SHIFT_SIZE_LARGE) +
           (loffset >> GRN_II_POS_LOFFSET_SHIFT_SIZE_LARGE);
  } else {
    return (lseg << GRN_II_POS_LSEG_SHIFT_SIZE) +
           (loffset >> GRN_II_POS_LOFFSET_SHIFT_SIZE);
  }
}

#define POS_LOFFSET_HEADER sizeof(buffer_header)
#define POS_LOFFSET_TERM   sizeof(buffer_term)

static inline bool
grn_ii_pos_is_available(grn_ctx *ctx, uint32_t flags, const char *context)
{
  if (flags & GRN_OBJ_INDEX_LARGE) {
    if ((POS_LOFFSET_HEADER % 4) != 0) {
      ERR(GRN_OPERATION_NOT_SUPPORTED,
          "%s INDEX_LARGE requires 32bit aligned offset header: "
          "<%" GRN_FMT_SIZE ">",
          context,
          POS_LOFFSET_HEADER);
      return false;
    }
    if ((POS_LOFFSET_TERM % 4) != 0) {
      ERR(GRN_OPERATION_NOT_SUPPORTED,
          "%s INDEX_LARGE requires 32bit aligned offset term: "
          "<%" GRN_FMT_SIZE ">",
          context,
          POS_LOFFSET_TERM);
      return false;
    }
  }
  return true;
}

#define POS_IS_EMBED(pos)            ((pos) & 1)
#define POS_EMBED_RID(rid)           (((rid) << 1) | 1)
#define POS_EMBED_RID_SID(rid, sid)  ((((rid) << 12) + ((sid) << 1)) | 1)
#define POS_RID_EXTRACT(pos)         ((pos) >> 1)
#define POS_RID_SID_EXTRACT_RID(pos) BIT31_12((pos))
#define POS_RID_SID_EXTRACT_SID(pos) BIT11_01((pos))

static inline uint32_t
buffer_open(
  grn_ctx *ctx, grn_ii *ii, uint32_t pos, buffer_term **bt, buffer **b)
{
  const uint32_t lseg = grn_ii_pos_lseg(ii, pos);
  const uint32_t pseg = grn_ii_get_buffer_pseg_inline(ii, lseg);
  if (pseg != GRN_II_PSEG_NOT_ASSIGNED) {
    auto p = static_cast<byte *>(grn_io_seg_ref(ctx, ii->seg, pseg));
    if (!p) {
      return GRN_II_PSEG_NOT_ASSIGNED;
    }
    if (b) {
      *b = (buffer *)p;
    }
    if (bt) {
      *bt = (buffer_term *)(p + grn_ii_pos_loffset(ii, pos));
    }
  }
  return pseg;
}

static inline grn_rc
buffer_close(grn_ctx *ctx, grn_ii *ii, uint32_t pseg)
{
  if (pseg >= ii->seg->header->max_segment) {
    GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid pseg buffer_close(%d)", pseg);
    return GRN_INVALID_ARGUMENT;
  }
  grn_io_seg_unref(ctx, ii->seg, pseg);
  return GRN_SUCCESS;
}

typedef struct {
  uint32_t rid;
  uint32_t sid;
} docid;

#define BUFFER_REC_DEL(r)      ((r)->jump = 1)
#define BUFFER_REC_DELETED(r)  ((r)->jump == 1)

#define BUFFER_REC_AT(b, pos)  ((buffer_rec *)(b) + (pos))
#define BUFFER_REC_POS(b, rec) ((uint16_t)((rec) - (buffer_rec *)(b)))

static inline void
buffer_term_dump(grn_ctx *ctx, grn_ii *ii, buffer *b, buffer_term *bt)
{
  int pos, rid, sid;
  uint8_t *p;
  buffer_rec *r;

  if (!grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
    return;
  }

  GRN_LOG(ctx,
          GRN_LOG_DEBUG,
          "b=(%x %u %u %u)",
          b->header.chunk,
          b->header.chunk_size,
          b->header.buffer_free,
          b->header.nterms);
  GRN_LOG(ctx,
          GRN_LOG_DEBUG,
          "bt=(%u %u %u %u %u)",
          bt->tid,
          bt->size_in_chunk,
          bt->pos_in_chunk,
          bt->size_in_buffer,
          bt->pos_in_buffer);
  for (pos = bt->pos_in_buffer; pos; pos = r->step) {
    r = BUFFER_REC_AT(b, pos);
    p = GRN_NEXT_ADDR(r);
    GRN_B_DEC(rid, p);
    if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
      GRN_B_DEC(sid, p);
    } else {
      sid = 1;
    }
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "%d=(%d:%d),(%d:%d)",
            pos,
            r->jump,
            r->step,
            rid,
            sid);
  }
}

static inline grn_rc
check_jump(grn_ctx *ctx, grn_ii *ii, buffer *b, buffer_rec *r, int j)
{
  uint16_t i = BUFFER_REC_POS(b, r);
  uint8_t *p;
  buffer_rec *r2;
  docid id, id2;
  if (!j) {
    return GRN_SUCCESS;
  }
  p = GRN_NEXT_ADDR(r);
  GRN_B_DEC(id.rid, p);
  if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
    GRN_B_DEC(id.sid, p);
  } else {
    id.sid = 1;
  }
  if (j == 1) {
    GRN_LOG(ctx, GRN_LOG_DEBUG, "deleting! %d(%d:%d)", i, id.rid, id.sid);
    return GRN_SUCCESS;
  }
  r2 = BUFFER_REC_AT(b, j);
  p = GRN_NEXT_ADDR(r2);
  GRN_B_DEC(id2.rid, p);
  if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
    GRN_B_DEC(id2.sid, p);
  } else {
    id2.sid = 1;
  }
  if (r2->step == i) {
    GRN_LOG(ctx,
            GRN_LOG_EMERG,
            "cycle! %d(%d:%d)<->%d(%d:%d)",
            i,
            id.rid,
            id.sid,
            j,
            id2.rid,
            id2.sid);
    return GRN_FILE_CORRUPT;
  }
  if (id2.rid < id.rid || (id2.rid == id.rid && id2.sid <= id.sid)) {
    GRN_LOG(ctx,
            GRN_LOG_CRIT,
            "invalid jump! %d(%d:%d)(%d:%d)->%d(%d:%d)(%d:%d)",
            i,
            r->jump,
            r->step,
            id.rid,
            id.sid,
            j,
            r2->jump,
            r2->step,
            id2.rid,
            id2.sid);
    return GRN_FILE_CORRUPT;
  }
  return GRN_SUCCESS;
}

static inline grn_rc
set_jump_r(grn_ctx *ctx, grn_ii *ii, buffer *b, buffer_rec *from, int to)
{
  int i, j, max_jump = 100;
  buffer_rec *r, *r2;
  for (r = from, j = to; j > 1 && max_jump--; r = BUFFER_REC_AT(b, r->step)) {
    r2 = BUFFER_REC_AT(b, j);
    if (r == r2) {
      break;
    }
    if (BUFFER_REC_DELETED(r2)) {
      break;
    }
    if (j == (i = r->jump)) {
      break;
    }
    if (j == r->step) {
      break;
    }
    if (check_jump(ctx, ii, b, r, j)) {
      ERR(GRN_FILE_CORRUPT, "check_jump failed");
      return ctx->rc;
    }
    r->jump = j;
    j = i;
    if (!r->step) {
      return GRN_FILE_CORRUPT;
    }
  }
  return GRN_SUCCESS;
}

#define GET_NUM_BITS(x, n)                                                     \
  do {                                                                         \
    n = x;                                                                     \
    n = (n & 0x55555555) + ((n >> 1) & 0x55555555);                            \
    n = (n & 0x33333333) + ((n >> 2) & 0x33333333);                            \
    n = (n & 0x0F0F0F0F) + ((n >> 4) & 0x0F0F0F0F);                            \
    n = (n & 0x00FF00FF) + ((n >> 8) & 0x00FF00FF);                            \
    n = (n & 0x0000FFFF) + ((n >> 16) & 0x0000FFFF);                           \
  } while (0)

static inline grn_rc
buffer_put(grn_ctx *ctx,
           grn_ii *ii,
           buffer *b,
           buffer_term *bt,
           buffer_rec *rnew,
           uint8_t *bs,
           grn_ii_updspec *u,
           int size)
{
  uint8_t *p;
  docid id_curr = {0, 0}, id_start = {0, 0}, id_post = {0, 0};
  buffer_rec *r_curr, *r_start = NULL;
  uint16_t last = 0, *lastp = &bt->pos_in_buffer, pos = BUFFER_REC_POS(b, rnew);
  int vdelta = 0, delta, delta0 = 0, vhops = 0, nhops = 0, reset = 1;
  grn_memcpy(GRN_NEXT_ADDR(rnew), bs, size - sizeof(buffer_rec));
  for (;;) {
    if (!*lastp) {
      rnew->step = 0;
      rnew->jump = 0;
      // smb_wmb();
      *lastp = pos;
      if (bt->size_in_buffer++ > 1) {
        buffer_rec *rhead = BUFFER_REC_AT(b, bt->pos_in_buffer);
        rhead->jump = pos;
        if (!(bt->size_in_buffer & 1)) {
          int n;
          buffer_rec *r = BUFFER_REC_AT(b, rhead->step), *r2;
          GET_NUM_BITS(bt->size_in_buffer, n);
          while (n-- && (r->jump > 1)) {
            r2 = BUFFER_REC_AT(b, r->jump);
            if (BUFFER_REC_DELETED(r2)) {
              break;
            }
            r = r2;
          }
          if (r != rnew) {
            set_jump_r(ctx, ii, b, r, last);
            if (ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
        }
      }
      break;
    }
    r_curr = BUFFER_REC_AT(b, *lastp);
    p = GRN_NEXT_ADDR(r_curr);
    GRN_B_DEC(id_curr.rid, p);
    if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
      GRN_B_DEC(id_curr.sid, p);
    } else {
      id_curr.sid = 1;
    }
    if (id_curr.rid < id_post.rid ||
        (id_curr.rid == id_post.rid && id_curr.sid < id_post.sid)) {
      {
        GRN_DEFINE_NAME(ii);
        CRIT(GRN_FILE_CORRUPT,
             "[ii][buffer][put] loop is found: "
             "<%.*s>: "
             "(%d:%d)->(%d:%d)",
             name_size,
             name,
             id_post.rid,
             id_post.sid,
             id_curr.rid,
             id_curr.sid);
      }
      buffer_term_dump(ctx, ii, b, bt);
      bt->pos_in_buffer = 0;
      bt->size_in_buffer = 0;
      lastp = &bt->pos_in_buffer;
      continue;
    }
    id_post.rid = id_curr.rid;
    id_post.sid = id_curr.sid;
    if (u->rid < id_curr.rid ||
        (u->rid == id_curr.rid && u->sid <= id_curr.sid)) {
      uint16_t step = *lastp, jump = r_curr->jump;
      if (u->rid == id_curr.rid) {
        if (u->sid == 0) {
          while (id_curr.rid == u->rid) {
            BUFFER_REC_DEL(r_curr);
            if (!(step = r_curr->step)) {
              break;
            }
            r_curr = BUFFER_REC_AT(b, step);
            p = GRN_NEXT_ADDR(r_curr);
            GRN_B_DEC(id_curr.rid, p);
            if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
              GRN_B_DEC(id_curr.sid, p);
            } else {
              id_curr.sid = 1;
            }
          }
        } else if (u->sid == id_curr.sid) {
          BUFFER_REC_DEL(r_curr);
          step = r_curr->step;
        }
      }
      rnew->step = step;
      rnew->jump = check_jump(ctx, ii, b, rnew, jump) ? 0 : jump;
      // smb_wmb();
      *lastp = pos;
      break;
    }

    if (reset) {
      r_start = r_curr;
      id_start.rid = id_curr.rid;
      id_start.sid = id_curr.sid;
      if (!(delta0 = u->rid - id_start.rid)) {
        delta0 = u->sid - id_start.sid;
      }
      nhops = 0;
      vhops = 1;
      vdelta = delta0 >> 1;
    } else {
      if (!(delta = id_curr.rid - id_start.rid)) {
        delta = id_curr.sid - id_start.sid;
      }
      if (vdelta < delta) {
        vdelta += (delta0 >> ++vhops);
        r_start = r_curr;
      }
      if (nhops > vhops) {
        set_jump_r(ctx, ii, b, r_start, *lastp);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
      } else {
        nhops++;
      }
    }

    last = *lastp;
    lastp = &r_curr->step;
    reset = 0;
    {
      uint16_t posj = r_curr->jump;
      if (posj > 1) {
        buffer_rec *rj = BUFFER_REC_AT(b, posj);
        if (!BUFFER_REC_DELETED(rj)) {
          docid idj;
          p = GRN_NEXT_ADDR(rj);
          GRN_B_DEC(idj.rid, p);
          if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
            GRN_B_DEC(idj.sid, p);
          } else {
            idj.sid = 1;
          }
          if (idj.rid < u->rid || (idj.rid == u->rid && idj.sid < u->sid)) {
            last = posj;
            lastp = &rj->step;
          } else {
            reset = 1;
          }
        }
      }
    }
  }
  return ctx->rc;
}

/* array */

static inline uint32_t *
array_at(grn_ctx *ctx, grn_ii *ii, uint32_t id)
{
  if (id > GRN_ID_MAX) {
    return NULL;
  }
  const uint16_t seg = id >> W_ARRAY;
  const uint32_t pseg = grn_ii_get_array_pseg_inline(ii, seg);
  if (pseg == GRN_II_PSEG_NOT_ASSIGNED) {
    return NULL;
  }
  auto p = static_cast<byte *>(grn_io_seg_ref(ctx, ii->seg, pseg));
  if (!p) {
    return NULL;
  }
  return (uint32_t *)(p + (id & ARRAY_MASK_IN_A_SEGMENT) * S_ARRAY_ELEMENT);
}

static inline uint32_t *
array_get(grn_ctx *ctx, grn_ii *ii, uint32_t id)
{
  byte *p = NULL;
  if (id > GRN_ID_MAX) {
    return NULL;
  }
  const uint16_t seg = id >> W_ARRAY;
  uint32_t pseg = grn_ii_get_array_pseg_inline(ii, seg);
  if (pseg == GRN_II_PSEG_NOT_ASSIGNED) {
    if (segment_get_clear(ctx, ii, &pseg)) {
      return NULL;
    }
    grn_ii_set_array_pseg_inline(ii, seg, pseg);
    if (seg >= ii->header.common->amax) {
      ii->header.common->amax = seg + 1;
    }
  }
  p = static_cast<byte *>(grn_io_seg_ref(ctx, ii->seg, pseg));
  if (!p) {
    return NULL;
  }
  return (uint32_t *)(p + (id & ARRAY_MASK_IN_A_SEGMENT) * S_ARRAY_ELEMENT);
}

static inline void
array_unref(grn_ctx *ctx, grn_ii *ii, uint32_t id)
{
  const uint32_t pseg = grn_ii_get_array_pseg_inline(ii, id >> W_ARRAY);
  grn_io_seg_unref(ctx, ii->seg, pseg);
}

/* updspec */

extern "C" grn_ii_updspec *
grn_ii_updspec_open(grn_ctx *ctx, uint32_t rid, uint32_t sid)
{
  auto u = static_cast<grn_ii_updspec *>(GRN_CALLOC(sizeof(grn_ii_updspec)));
  if (!u) {
    return NULL;
  }
  u->rid = rid;
  u->sid = sid;
  u->weight = 0;
  u->tf = 0;
  u->atf = 0;
  u->pos = NULL;
  u->tail = NULL;
  //  u->vnodes = NULL;
  return u;
}

extern "C" grn_rc
grn_ii_updspec_add(grn_ctx *ctx, grn_ii_updspec *u, int pos, int32_t weight)
{
  u->atf++;
  if (u->tf >= GRN_II_MAX_TF) {
    return GRN_SUCCESS;
  }
  auto p = static_cast<_grn_ii_pos *>(GRN_CALLOC(sizeof(struct _grn_ii_pos)));
  if (!p) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  u->weight += weight;
  p->pos = pos;
  p->next = NULL;
  if (u->tail) {
    u->tail->next = p;
  } else {
    u->pos = p;
  }
  u->tail = p;
  u->tf++;
  return GRN_SUCCESS;
}

extern "C" int
grn_ii_updspec_cmp(grn_ii_updspec *a, grn_ii_updspec *b)
{
  struct _grn_ii_pos *pa, *pb;
  if (a->rid != b->rid) {
    return a->rid - b->rid;
  }
  if (a->sid != b->sid) {
    return a->sid - b->sid;
  }
  if (a->weight != b->weight) {
    return a->weight - b->weight;
  }
  if (a->tf != b->tf) {
    return a->tf - b->tf;
  }
  for (pa = a->pos, pb = b->pos; pa && pb; pa = pa->next, pb = pb->next) {
    if (pa->pos != pb->pos) {
      return pa->pos - pb->pos;
    }
  }
  if (pa) {
    return 1;
  }
  if (pb) {
    return -1;
  }
  return 0;
}

extern "C" grn_rc
grn_ii_updspec_close(grn_ctx *ctx, grn_ii_updspec *u)
{
  struct _grn_ii_pos *p = u->pos, *q;
  while (p) {
    q = p->next;
    GRN_FREE(p);
    p = q;
  }
  GRN_FREE(u);
  return GRN_SUCCESS;
}

static inline uint8_t *
encode_rec(
  grn_ctx *ctx, grn_ii *ii, grn_ii_updspec *u, unsigned int *size, int deletep)
{
  struct _grn_ii_pos *pp;
  uint32_t lpos, tf, weight;
  if (deletep) {
    tf = 0;
    weight = 0;
  } else {
    tf = u->tf;
    weight = u->weight;
  }
  auto br = static_cast<uint8_t *>(GRN_MALLOC((tf + 4) * 5));
  if (!br) {
    return NULL;
  }
  auto p = br;
  GRN_B_ENC(u->rid, p);
  if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
    GRN_B_ENC(u->sid, p);
  } else {
    u->sid = 1;
  }
  GRN_B_ENC(tf, p);
  if ((ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
    GRN_B_ENC(weight, p);
  }
  if ((ii->header.common->flags & GRN_OBJ_WITH_POSITION)) {
    for (lpos = 0, pp = u->pos; pp && tf--; lpos = pp->pos, pp = pp->next) {
      GRN_B_ENC(pp->pos - lpos, p);
    }
  }
  while (((intptr_t)p & 0x03)) {
    *p++ = 0;
  }
  *size = (unsigned int)((p - br) + sizeof(buffer_rec));
  return br;
}

typedef struct {
  grn_ii *ii;
  grn_hash *h;
} lexicon_deletable_arg;

#ifdef CASCADE_DELETE_LEXICON
static int
lexicon_deletable(grn_ctx *ctx, grn_obj *lexicon, grn_id tid, void *arg)
{
  uint32_t *a;
  grn_hash *h = ((lexicon_deletable_arg *)arg)->h;
  grn_ii *ii = ((lexicon_deletable_arg *)arg)->ii;
  if (!h) {
    return 0;
  }
  if ((a = array_at(ctx, ii, tid))) {
    if (a[0]) {
      array_unref(ctx, ii, tid);
      return 0;
    }
    array_unref(ctx, ii, tid);
  }
  {
    grn_ii_updspec **u;
    if (!grn_hash_get(ctx, h, &tid, sizeof(grn_id), (void **)&u)) {
      return (ERRP(ctx, GRN_ERROR)) ? 0 : 1;
    }
    if (!(*u)->tf || !(*u)->sid) {
      return 1;
    }
    return 0;
  }
}
#endif /* CASCADE_DELETE_LEXICON */

static inline void
lexicon_delete(grn_ctx *ctx, grn_ii *ii, uint32_t tid, grn_hash *h)
{
#ifdef CASCADE_DELETE_LEXICON
  lexicon_deletable_arg arg = {ii, h};
  grn_table_delete_optarg optarg = {0, lexicon_deletable, &arg};
  grn_table_delete_data data;
  grn_table_delete_data_init(ctx, &data, ii->lexicon);
  data.id = tid;
  data.optarg = &optarg;
  grn_table_delete_by_id_without_lock(ctx, &data);
  grn_table_delete_data_fin(ctx, &data);
#endif /* CASCADE_DELETE_LEXICON */
}

typedef struct {
  grn_id rid;
  uint32_t sid;
  uint32_t tf;
  uint32_t weight;
  uint32_t flags;
} docinfo;

static docinfo null_docinfo = {};

typedef struct {
  uint32_t segno;
  uint32_t size;
  uint32_t dgap;
} chunk_info;

typedef struct {
  grn_log_level log_level;
  const char *tag;
  grn_ii *ii;
  const uint8_t *chunk;
  buffer_term *term;
  uint16_t nth_term;
  uint16_t n_terms;
  uint32_t nth_chunk;
  uint32_t n_chunks;
  datavec data_vector[MAX_N_ELEMENTS + 1];
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_size;
  grn_obj inspected_term;
  grn_obj inspected_entries;
  uint32_t n_inspected_entries;
} merge_dump_source_data;

#define MERGE_DUMP_SOURCE_BATCH_SIZE 10

typedef enum {
  MERGE_DUMP_SOURCE_ENTRY_BUFFER,
  MERGE_DUMP_SOURCE_ENTRY_CHUNK,
} merge_dump_source_entry_type;

static void
merge_dump_source_flush_entries(grn_ctx *ctx, merge_dump_source_data *data)
{
  if (GRN_TEXT_LEN(&(data->inspected_entries)) == 0) {
    return;
  }

  GRN_LOG(ctx,
          data->log_level,
          "%.*s",
          (int)GRN_TEXT_LEN(&(data->inspected_entries)),
          GRN_TEXT_VALUE(&(data->inspected_entries)));
  GRN_BULK_REWIND(&(data->inspected_entries));
  data->n_inspected_entries = 0;
}

static void
merge_dump_source_add_entry(grn_ctx *ctx,
                            merge_dump_source_data *data,
                            merge_dump_source_entry_type type,
                            docinfo *info,
                            uint8_t *position_gaps)
{
  if (GRN_TEXT_LEN(&(data->inspected_entries)) > 0) {
    GRN_TEXT_PUTC(ctx, &(data->inspected_entries), ' ');
  }
  grn_text_printf(ctx,
                  &(data->inspected_entries),
                  "(%u:%u:%u:%u)",
                  info->rid,
                  info->sid,
                  info->tf,
                  info->weight);
  if (data->ii->header.common->flags & GRN_OBJ_WITH_POSITION) {
    uint32_t i;
    uint32_t position = 0;

    GRN_TEXT_PUTC(ctx, &(data->inspected_entries), '[');
    for (i = 0; i < info->tf; i++) {
      uint32_t position_gap;

      if (i > 0) {
        GRN_TEXT_PUTC(ctx, &(data->inspected_entries), ',');
      }
      if (type == MERGE_DUMP_SOURCE_ENTRY_BUFFER) {
        GRN_B_DEC(position_gap, position_gaps);
      } else {
        position_gap = ((uint32_t *)position_gaps)[i];
      }
      position += position_gap;
      grn_text_printf(ctx, &(data->inspected_entries), "%u", position);
    }
    GRN_TEXT_PUTC(ctx, &(data->inspected_entries), ']');
  }
  data->n_inspected_entries++;
  if (data->n_inspected_entries == MERGE_DUMP_SOURCE_BATCH_SIZE) {
    merge_dump_source_flush_entries(ctx, data);
  }
}

static void
merge_dump_source_chunk_raw(grn_ctx *ctx,
                            merge_dump_source_data *data,
                            const uint8_t *chunk_start,
                            const uint8_t *chunk_end)
{
  const uint8_t *chunk_current = chunk_start;
  int decoded_size;

  if ((chunk_end - chunk_current) == 0) {
    GRN_LOG(ctx,
            data->log_level,
            "%s[%.*s][%d/%d] <%.*s>(%u): %u/%u: no data",
            data->tag,
            data->name_size,
            data->name,
            data->nth_term,
            data->n_terms,
            (int)GRN_TEXT_LEN(&(data->inspected_term)),
            GRN_TEXT_VALUE(&(data->inspected_term)),
            CHUNK_SPLIT_OFF(data->term->tid),
            data->nth_chunk,
            data->n_chunks);
    return;
  }

  decoded_size = grn_decv(ctx,
                          data->ii,
                          CHUNK_SPLIT_OFF(data->term->tid),
                          chunk_current,
                          chunk_end - chunk_current,
                          data->data_vector);
  if (decoded_size == 0) {
    GRN_LOG(ctx,
            data->log_level,
            "%s[%.*s][%d/%d] <%.*s>(%u): %u/%u: failed to decode",
            data->tag,
            data->name_size,
            data->name,
            data->nth_term,
            data->n_terms,
            (int)GRN_TEXT_LEN(&(data->inspected_term)),
            GRN_TEXT_VALUE(&(data->inspected_term)),
            CHUNK_SPLIT_OFF(data->term->tid),
            data->nth_chunk,
            data->n_chunks);
    return;
  }

  {
    uint32_t n_documents;
    const uint32_t *record_id_gaps;
    const uint32_t *section_id_gaps = NULL;
    const uint32_t *n_terms_list;
    const uint32_t *weights = NULL;
    const uint32_t *position_gaps = NULL;

    {
      int i = 0;
      n_documents = data->data_vector[i].data_size;
      record_id_gaps = data->data_vector[i++].data;
      if (data->ii->header.common->flags & GRN_OBJ_WITH_SECTION) {
        section_id_gaps = data->data_vector[i++].data;
      }
      n_terms_list = data->data_vector[i++].data;
      if (data->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT) {
        weights = data->data_vector[i++].data;
      }
      if (data->ii->header.common->flags & GRN_OBJ_WITH_POSITION) {
        position_gaps = data->data_vector[i++].data;
      }
    }

    {
      uint32_t i;
      grn_id record_id = GRN_ID_NIL;
      uint32_t section_id = 0;

      for (i = 0; i < n_documents; i++) {
        uint32_t record_id_gap = record_id_gaps[i];
        docinfo info;

        record_id += record_id_gap;
        info.rid = record_id;
        if (data->ii->header.common->flags & GRN_OBJ_WITH_SECTION) {
          if (record_id_gap > 0) {
            section_id = 0;
          }
          section_id += 1 + section_id_gaps[i];
        } else {
          section_id = 1;
        }
        info.sid = section_id;
        info.tf = 1 + n_terms_list[i];
        if (data->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT) {
          info.weight = weights[i];
        } else {
          info.weight = 0;
        }

        merge_dump_source_add_entry(ctx,
                                    data,
                                    MERGE_DUMP_SOURCE_ENTRY_CHUNK,
                                    &info,
                                    (uint8_t *)position_gaps);
      }
      merge_dump_source_flush_entries(ctx, data);
    }
  }
}

static void
merge_dump_source_chunk(grn_ctx *ctx, merge_dump_source_data *data)
{
  const uint8_t *chunk_start = data->chunk + data->term->pos_in_chunk;
  const uint8_t *chunk_end = chunk_start + data->term->size_in_chunk;

  if (data->term->tid & CHUNK_SPLIT) {
    const uint8_t *chunk_current = chunk_start;

    GRN_B_DEC(data->n_chunks, chunk_current);
    GRN_LOG(ctx,
            data->log_level,
            "%s[%.*s][%d/%d] <%.*s>(%u): chunk: %u",
            data->tag,
            data->name_size,
            data->name,
            data->nth_term,
            data->n_terms,
            (int)GRN_TEXT_LEN(&(data->inspected_term)),
            GRN_TEXT_VALUE(&(data->inspected_term)),
            CHUNK_SPLIT_OFF(data->term->tid),
            data->n_chunks);

    for (data->nth_chunk = 0; data->nth_chunk < data->n_chunks;
         data->nth_chunk++) {
      chunk_info info;

      if (!(chunk_current < chunk_end)) {
        GRN_LOG(ctx,
                data->log_level,
                "%s[%.*s][%d/%d] <%.*s>(%u): chunk: %u/%u: no data",
                data->tag,
                data->name_size,
                data->name,
                data->nth_term,
                data->n_terms,
                (int)GRN_TEXT_LEN(&(data->inspected_term)),
                GRN_TEXT_VALUE(&(data->inspected_term)),
                CHUNK_SPLIT_OFF(data->term->tid),
                data->nth_chunk,
                data->n_chunks);
        return;
      }

      GRN_B_DEC(info.segno, chunk_current);
      GRN_B_DEC(info.size, chunk_current);
      GRN_B_DEC(info.dgap, chunk_current);

      GRN_LOG(ctx,
              data->log_level,
              "%s[%.*s][%d/%d] <%.*s>(%u): chunk: %u/%u: "
              "segment:<%d>, size:<%d>, gap:<%d>",
              data->tag,
              data->name_size,
              data->name,
              data->nth_term,
              data->n_terms,
              (int)GRN_TEXT_LEN(&(data->inspected_term)),
              GRN_TEXT_VALUE(&(data->inspected_term)),
              CHUNK_SPLIT_OFF(data->term->tid),
              data->nth_chunk,
              data->n_chunks,
              info.segno,
              info.size,
              info.dgap);
      if (info.size == 0) {
        continue;
      }
      {
        grn_io_win iw;
        auto sub_chunk = static_cast<uint8_t *>(WIN_MAP(ctx,
                                                        data->ii->chunk,
                                                        &iw,
                                                        info.segno,
                                                        0,
                                                        info.size,
                                                        GRN_IO_RDONLY));
        if (!sub_chunk) {
          GRN_LOG(ctx,
                  data->log_level,
                  "%s[%.*s][%d/%d] <%.*s>(%u): chunk: "
                  "%u/%u: failed to open sub chunk",
                  data->tag,
                  data->name_size,
                  data->name,
                  data->nth_term,
                  data->n_terms,
                  (int)GRN_TEXT_LEN(&(data->inspected_term)),
                  GRN_TEXT_VALUE(&(data->inspected_term)),
                  CHUNK_SPLIT_OFF(data->term->tid),
                  data->nth_chunk,
                  data->n_chunks);
          continue;
        }
        merge_dump_source_chunk_raw(ctx,
                                    data,
                                    sub_chunk,
                                    sub_chunk + info.size);
        grn_io_win_unmap(ctx, &iw);
      }
    }
    if (chunk_current < chunk_end) {
      GRN_LOG(ctx,
              data->log_level,
              "%s[%.*s][%d/%d] <%.*s>(%u): chunk: %u",
              data->tag,
              data->name_size,
              data->name,
              data->nth_term,
              data->n_terms,
              (int)GRN_TEXT_LEN(&(data->inspected_term)),
              GRN_TEXT_VALUE(&(data->inspected_term)),
              CHUNK_SPLIT_OFF(data->term->tid),
              data->n_chunks);
      merge_dump_source_chunk_raw(ctx, data, chunk_current, chunk_end);
    }
  } else {
    data->nth_chunk = 0;
    data->n_chunks = 0;
    GRN_LOG(ctx,
            data->log_level,
            "%s[%.*s][%d/%d] <%.*s>(%u): chunk: %u",
            data->tag,
            data->name_size,
            data->name,
            data->nth_term,
            data->n_terms,
            (int)GRN_TEXT_LEN(&(data->inspected_term)),
            GRN_TEXT_VALUE(&(data->inspected_term)),
            data->term->tid,
            data->n_chunks);
    merge_dump_source_chunk_raw(ctx, data, chunk_start, chunk_end);
  }
}

static void
merge_dump_source(grn_ctx *ctx,
                  grn_ii *ii,
                  buffer *buf,
                  const uint8_t *chunk,
                  grn_log_level log_level)
{
  merge_dump_source_data data;

  if (!grn_logger_pass(ctx, log_level)) {
    return;
  }

  data.log_level = log_level;
  data.tag = "[ii][merge][source]";
  data.ii = ii;
  data.chunk = chunk;
  data.term = NULL;
  data.nth_term = 0;
  data.n_terms = buf->header.nterms;
  data.nth_chunk = 0;
  data.n_chunks = 0;
  datavec_init(ctx, ii, data.data_vector, 0, 0);
  {
    GRN_DEFINE_NAME(ii);
    grn_memcpy(data.name, name, name_size);
    data.name_size = name_size;
  }
  GRN_TEXT_INIT(&(data.inspected_term), 0);
  GRN_TEXT_INIT(&(data.inspected_entries), 0);
  data.n_inspected_entries = 0;

  GRN_LOG(ctx,
          data.log_level,
          "%s[%.*s] %u",
          data.tag,
          data.name_size,
          data.name,
          data.n_terms);
  for (data.nth_term = 0; data.nth_term < data.n_terms; data.nth_term++) {
    uint16_t position;

    data.term = buf->terms + data.nth_term;
    if (data.term->tid == 0) {
      GRN_LOG(ctx,
              data.log_level,
              "%s[%.*s][%d] void",
              data.tag,
              data.name_size,
              data.name,
              data.nth_term);
      continue;
    }

    GRN_BULK_REWIND(&(data.inspected_term));
    grn_ii_get_term(ctx,
                    ii,
                    CHUNK_SPLIT_OFF(data.term->tid),
                    &(data.inspected_term));
    GRN_LOG(ctx,
            data.log_level,
            "%s[%.*s][%d/%d] <%.*s>(%u): chunk:<%u:%u>, buffer:<%u:%u>",
            data.tag,
            data.name_size,
            data.name,
            data.nth_term,
            data.n_terms,
            (int)GRN_TEXT_LEN(&(data.inspected_term)),
            GRN_TEXT_VALUE(&(data.inspected_term)),
            CHUNK_SPLIT_OFF(data.term->tid),
            data.term->pos_in_chunk,
            data.term->size_in_chunk,
            data.term->pos_in_buffer,
            data.term->size_in_buffer);

    position = data.term->pos_in_buffer;
    while (position > 0) {
      buffer_rec *record = BUFFER_REC_AT(buf, position);
      uint8_t *record_data = GRN_NEXT_ADDR(record);
      docinfo info;

      grn_text_printf(ctx,
                      &(data.inspected_entries),
                      "record: %u: <%u:%u%s>",
                      position,
                      record->step,
                      record->jump,
                      BUFFER_REC_DELETED(record) ? ":deleted" : "");

      position = record->step;

      if (BUFFER_REC_DELETED(record)) {
        merge_dump_source_flush_entries(ctx, &data);
        continue;
      }

      GRN_TEXT_PUTC(ctx, &(data.inspected_entries), ':');

      GRN_B_DEC(info.rid, record_data);
      if (ii->header.common->flags & GRN_OBJ_WITH_SECTION) {
        GRN_B_DEC(info.sid, record_data);
      } else {
        info.sid = 1;
      }
      GRN_B_DEC(info.tf, record_data);
      if (ii->header.common->flags & GRN_OBJ_WITH_WEIGHT) {
        GRN_B_DEC(info.weight, record_data);
      } else {
        info.weight = 0;
      }
      merge_dump_source_add_entry(ctx,
                                  &data,
                                  MERGE_DUMP_SOURCE_ENTRY_BUFFER,
                                  &info,
                                  record_data);
      merge_dump_source_flush_entries(ctx, &data);
    }

    if (chunk && data.term->size_in_chunk > 0) {
      merge_dump_source_chunk(ctx, &data);
    }
  }

  datavec_fin(ctx, data.data_vector);
  GRN_OBJ_FIN(ctx, &(data.inspected_term));
  GRN_OBJ_FIN(ctx, &(data.inspected_entries));
}

typedef struct {
  grn_ii *ii;
  bool succeeded;
  grn_obj source_chunks;
  grn_obj dest_chunks;
} grn_merging_data;

static void
grn_merging_data_init(grn_ctx *ctx, grn_merging_data *data, grn_ii *ii)
{
  data->ii = ii;
  data->succeeded = false;
  GRN_UINT32_INIT(&(data->source_chunks), GRN_OBJ_VECTOR);
  GRN_UINT32_INIT(&(data->dest_chunks), GRN_OBJ_VECTOR);
}

static void
grn_merging_data_fin(grn_ctx *ctx, grn_merging_data *data)
{
  grn_obj *target_chunks;
  if (data->succeeded) {
    target_chunks = &(data->source_chunks);
  } else {
    target_chunks = &(data->dest_chunks);
  }
  const size_t n_target_chunks = GRN_UINT32_VECTOR_SIZE(target_chunks) / 2;
  size_t i;
  for (i = 0; i < n_target_chunks; i++) {
    const uint32_t segno = GRN_UINT32_VALUE_AT(target_chunks, i * 2);
    const uint32_t size = GRN_UINT32_VALUE_AT(target_chunks, i * 2 + 1);
    chunk_free(ctx, data->ii, segno, size);
  }
  GRN_OBJ_FIN(ctx, &(data->source_chunks));
  GRN_OBJ_FIN(ctx, &(data->dest_chunks));
}

static void
grn_merging_data_add_source_chunk(grn_ctx *ctx,
                                  grn_merging_data *data,
                                  uint32_t segno,
                                  uint32_t size)
{
  GRN_UINT32_PUT(ctx, &(data->source_chunks), segno);
  GRN_UINT32_PUT(ctx, &(data->source_chunks), size);
}

static void
grn_merging_data_add_dest_chunk(grn_ctx *ctx,
                                grn_merging_data *data,
                                uint32_t segno,
                                uint32_t size)
{
  GRN_UINT32_PUT(ctx, &(data->dest_chunks), segno);
  GRN_UINT32_PUT(ctx, &(data->dest_chunks), size);
}

typedef struct {
  buffer *buf;
  const uint8_t *data;
  docinfo id;
  uint16_t next_position;
} merger_buffer_data;

typedef struct {
  const uint8_t *data_start;
  const uint8_t *data_end;
  const uint8_t *data;
  docinfo id;
  uint32_t n_documents;
  const uint32_t *record_id_gaps;
  const uint32_t *section_id_gaps;
  const uint32_t *tfs;
  const uint32_t *weights;
  const uint32_t *position_gaps;
  const uint32_t *position_gaps_end;
} merger_chunk_data;

typedef struct {
  grn_ii *ii;
  grn_id term_id;
  docinfo last_id;
  uint64_t position;
  uint64_t max_position;
  grn_merging_data *merging_data;
  struct {
    merger_buffer_data buffer;
    merger_chunk_data chunk;
  } source;
  struct {
    uint32_t *record_id_gaps;
    uint32_t *section_id_gaps;
    uint32_t *tfs;
    uint32_t *weights;
    uint32_t *position_gaps;
    uint32_t *position_gaps_end;
  } dest;
} merger_data;

static inline void
merger_init_chunk_data(grn_ctx *ctx, merger_data *data, datavec *rdv)
{
  merger_chunk_data *chunk_data = &(data->source.chunk);
  grn_ii *ii = data->ii;
  int j = 0;
  memset(&(chunk_data->id), 0, sizeof(docinfo));
  chunk_data->n_documents = rdv[j].data_size;
  chunk_data->record_id_gaps = rdv[j++].data;
  if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
    chunk_data->section_id_gaps = rdv[j++].data;
  }
  chunk_data->tfs = rdv[j++].data;
  if ((ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
    chunk_data->weights = rdv[j++].data;
  }
  chunk_data->position_gaps = rdv[j].data;
  chunk_data->position_gaps_end = chunk_data->position_gaps + rdv[j].data_size;
}

static inline void
merger_report_error(grn_ctx *ctx,
                    merger_data *data,
                    const char *message,
                    docinfo *posting1,
                    docinfo *posting2)
{
  merger_buffer_data *buffer_data = &(data->source.buffer);
  merger_chunk_data *chunk_data = &(data->source.chunk);
  grn_obj term;
  GRN_DEFINE_NAME(data->ii);
  GRN_TEXT_INIT(&term, 0);
  grn_ii_get_term(ctx, data->ii, CHUNK_SPLIT_OFF(data->term_id), &term);
  if (posting1 && posting2) {
    CRIT(GRN_FILE_CORRUPT,
         "[ii][broken] %s: <%.*s>: <%.*s>(%u): (%u:%u) -> (%u:%u)",
         message,
         name_size,
         name,
         (int)GRN_TEXT_LEN(&term),
         GRN_TEXT_VALUE(&term),
         CHUNK_SPLIT_OFF(data->term_id),
         posting1->rid,
         posting1->sid,
         posting2->rid,
         posting2->sid);
  } else {
    CRIT(GRN_FILE_CORRUPT,
         "[ii][broken] %s: <%.*s>: <%.*s>(%u): (%u:%u)",
         message,
         name_size,
         name,
         (int)GRN_TEXT_LEN(&term),
         GRN_TEXT_VALUE(&term),
         CHUNK_SPLIT_OFF(data->term_id),
         posting1->rid,
         posting1->sid);
  }
  GRN_OBJ_FIN(ctx, &term);
  merge_dump_source(ctx,
                    data->ii,
                    buffer_data->buf,
                    chunk_data->data_start,
                    GRN_LOG_CRIT);
}

static inline void
merger_get_next_chunk(grn_ctx *ctx, merger_data *data)
{
  merger_chunk_data *chunk_data = &(data->source.chunk);
  if (chunk_data->n_documents == 0) {
    chunk_data->id.rid = 0;
  } else {
    uint32_t record_id_gap = *chunk_data->record_id_gaps;
    chunk_data->record_id_gaps++;
    chunk_data->id.rid += record_id_gap;
    if (record_id_gap > 0) {
      chunk_data->id.sid = 0;
    }
    chunk_data->position_gaps += chunk_data->id.tf;
    chunk_data->id.tf = 1 + *chunk_data->tfs;
    chunk_data->tfs++;
    if ((data->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
      chunk_data->id.weight = *chunk_data->weights;
      chunk_data->weights++;
    }
    if ((data->ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
      chunk_data->id.sid += 1 + *chunk_data->section_id_gaps;
      chunk_data->section_id_gaps++;
    } else {
      chunk_data->id.sid = 1;
    }
    chunk_data->n_documents--;
  }
}

static inline void
merger_put_next_id(grn_ctx *ctx, merger_data *data, docinfo *id)
{
  uint32_t record_id_gap = id->rid - data->last_id.rid;
  *(data->dest.record_id_gaps) = record_id_gap;
  data->dest.record_id_gaps++;
  if ((data->ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
    uint32_t section_id_gap;
    if (record_id_gap > 0) {
      section_id_gap = id->sid - 1;
    } else {
      section_id_gap = id->sid - data->last_id.sid - 1;
    }
    *(data->dest.section_id_gaps) = section_id_gap;
    data->dest.section_id_gaps++;
  }
  *(data->dest.tfs) = id->tf - 1;
  data->dest.tfs++;
  if ((data->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
    *(data->dest.weights) = id->weight;
    data->dest.weights++;
  }
  data->last_id.rid = id->rid;
  data->last_id.sid = id->sid;
}

static inline void
merger_put_next_chunk(grn_ctx *ctx, merger_data *data)
{
  merger_chunk_data *chunk_data = &(data->source.chunk);
  if (chunk_data->id.rid > 0) {
    if (chunk_data->id.tf > 0) {
      if (data->last_id.rid > chunk_data->id.rid ||
          (data->last_id.rid == chunk_data->id.rid &&
           data->last_id.sid >= chunk_data->id.sid)) {
        merger_report_error(ctx,
                            data,
                            "the last posting is larger than "
                            "the next posting in chunk",
                            &(data->last_id),
                            &(chunk_data->id));
        return;
      }
      merger_put_next_id(ctx, data, &(chunk_data->id));
      if ((data->ii->header.common->flags & GRN_OBJ_WITH_POSITION)) {
        uint32_t i;
        uint32_t rest_n_positions =
          (uint32_t)(chunk_data->position_gaps_end - chunk_data->position_gaps);
        uint32_t rest_n_dest_positions =
          (uint32_t)(data->dest.position_gaps_end - data->dest.position_gaps);
        if (chunk_data->id.tf > rest_n_positions) {
          grn_obj term;
          GRN_DEFINE_NAME(data->ii);
          GRN_TEXT_INIT(&term, 0);
          grn_ii_get_term(ctx, data->ii, CHUNK_SPLIT_OFF(data->term_id), &term);
          GRN_LOG(
            ctx,
            GRN_LOG_WARNING,
            "[ii][merge][put][chunk] "
            "the number of terms are larger than the number of positions: "
            "<%.*s>: <%.*s>(%u): (%u:%u): (%u:%u)",
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            CHUNK_SPLIT_OFF(data->term_id),
            chunk_data->id.rid,
            chunk_data->id.sid,
            chunk_data->id.tf,
            rest_n_positions);
          GRN_OBJ_FIN(ctx, &term);
        }
        if (chunk_data->id.tf > rest_n_dest_positions) {
          /* TODO: Make this case error. */
          grn_obj term;
          GRN_DEFINE_NAME(data->ii);
          GRN_TEXT_INIT(&term, 0);
          grn_ii_get_term(ctx, data->ii, CHUNK_SPLIT_OFF(data->term_id), &term);
          GRN_LOG(ctx,
                  GRN_LOG_WARNING,
                  "[ii][merge][put][chunk] "
                  "the number of terms are larger than "
                  "the rest destination number of positions: "
                  "<%.*s>: <%.*s>(%u): (%u:%u): (%u:%u)",
                  name_size,
                  name,
                  (int)GRN_TEXT_LEN(&term),
                  GRN_TEXT_VALUE(&term),
                  CHUNK_SPLIT_OFF(data->term_id),
                  chunk_data->id.rid,
                  chunk_data->id.sid,
                  chunk_data->id.tf,
                  rest_n_dest_positions);
          GRN_OBJ_FIN(ctx, &term);
        }
        for (i = 0; i < chunk_data->id.tf; i++) {
          *(data->dest.position_gaps) = chunk_data->position_gaps[i];
          data->dest.position_gaps++;
          data->position += chunk_data->position_gaps[i];
        }
        if (data->position > data->max_position) {
          data->max_position = data->position;
        }
      }
    } else {
      merger_report_error(ctx,
                          data,
                          "invalid posting in chunk",
                          &(chunk_data->id),
                          NULL);
      return;
    }
  }
  merger_get_next_chunk(ctx, data);
}

static inline void
merger_get_next_buffer(grn_ctx *ctx, merger_data *data)
{
  merger_buffer_data *buffer_data = &(data->source.buffer);
  if (buffer_data->next_position == 0) {
    buffer_data->id.rid = 0;
  } else {
    docinfo last_id = {buffer_data->id.rid, buffer_data->id.sid, 0, 0, 0};
    buffer_rec *record =
      BUFFER_REC_AT(buffer_data->buf, buffer_data->next_position);
    buffer_data->data = GRN_NEXT_ADDR(record);
    GRN_B_DEC(buffer_data->id.rid, buffer_data->data);
    if ((data->ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
      GRN_B_DEC(buffer_data->id.sid, buffer_data->data);
    } else {
      buffer_data->id.sid = 1;
    }
    if (last_id.rid > buffer_data->id.rid ||
        (last_id.rid == buffer_data->id.rid &&
         last_id.sid >= buffer_data->id.sid)) {
      merger_report_error(ctx,
                          data,
                          "postings in block aren't sorted",
                          &last_id,
                          &(buffer_data->id));
      return;
    }
    buffer_data->next_position = record->step;
  }
}

static inline void
merger_put_next_buffer(grn_ctx *ctx, merger_data *data)
{
  merger_buffer_data *buffer_data = &(data->source.buffer);
  if (buffer_data->id.rid > 0 && buffer_data->id.sid > 0) {
    GRN_B_DEC(buffer_data->id.tf, buffer_data->data);
    if (buffer_data->id.tf > 0) {
      if (data->last_id.rid > buffer_data->id.rid ||
          (data->last_id.rid == buffer_data->id.rid &&
           data->last_id.sid >= buffer_data->id.sid)) {
        merger_report_error(ctx,
                            data,
                            "the last posting is larger than "
                            "the next posting in buffer",
                            &(data->last_id),
                            &(buffer_data->id));
        return;
      }
      if ((data->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
        GRN_B_DEC(buffer_data->id.weight, buffer_data->data);
      }
      merger_put_next_id(ctx, data, &(buffer_data->id));
      if ((data->ii->header.common->flags & GRN_OBJ_WITH_POSITION)) {
        uint32_t rest_n_dest_positiongs =
          (uint32_t)(data->dest.position_gaps_end - data->dest.position_gaps);
        if (buffer_data->id.tf > rest_n_dest_positiongs) {
          /* TODO: Make this case error. */
          merger_buffer_data *buffer_data = &(data->source.buffer);
          grn_obj term;
          GRN_DEFINE_NAME(data->ii);
          GRN_TEXT_INIT(&term, 0);
          grn_ii_get_term(ctx, data->ii, CHUNK_SPLIT_OFF(data->term_id), &term);
          GRN_LOG(ctx,
                  GRN_LOG_WARNING,
                  "[ii][merge][put][buffer] "
                  "the number of terms are larger than "
                  "the rest destination number of positions: "
                  "<%.*s>: <%.*s>(%u): (%u:%u): (%u:%u)",
                  name_size,
                  name,
                  (int)GRN_TEXT_LEN(&term),
                  GRN_TEXT_VALUE(&term),
                  CHUNK_SPLIT_OFF(data->term_id),
                  buffer_data->id.rid,
                  buffer_data->id.sid,
                  buffer_data->id.tf,
                  rest_n_dest_positiongs);
          GRN_OBJ_FIN(ctx, &term);
        }
        while (buffer_data->id.tf--) {
          GRN_B_DEC(*(data->dest.position_gaps), buffer_data->data);
          data->position += *(data->dest.position_gaps);
          data->dest.position_gaps++;
        }
        if (data->position > data->max_position) {
          data->max_position = data->position;
        }
      }
    }
  }
  merger_get_next_buffer(ctx, data);
}

static inline bool
merger_merge(grn_ctx *ctx, merger_data *data)
{
  merger_buffer_data *buffer_data = &(data->source.buffer);
  merger_chunk_data *chunk_data = &(data->source.chunk);
  if (buffer_data->id.rid > 0) {
    if (chunk_data->id.rid > 0) {
      if (chunk_data->id.rid < buffer_data->id.rid) {
        merger_put_next_chunk(ctx, data);
        if (ctx->rc != GRN_SUCCESS) {
          return false;
        }
      } else {
        if (buffer_data->id.rid < chunk_data->id.rid) {
          merger_put_next_buffer(ctx, data);
          if (ctx->rc != GRN_SUCCESS) {
            return false;
          }
        } else {
          if (buffer_data->id.sid > 0) {
            if (chunk_data->id.sid < buffer_data->id.sid) {
              merger_put_next_chunk(ctx, data);
              if (ctx->rc != GRN_SUCCESS) {
                return false;
              }
            } else {
              if (buffer_data->id.sid == chunk_data->id.sid) {
                merger_get_next_chunk(ctx, data);
                if (ctx->rc != GRN_SUCCESS) {
                  return false;
                }
              }
              merger_put_next_buffer(ctx, data);
              if (ctx->rc != GRN_SUCCESS) {
                return false;
              }
            }
          } else {
            merger_get_next_chunk(ctx, data);
            if (ctx->rc != GRN_SUCCESS) {
              return false;
            }
          }
        }
      }
    } else {
      merger_put_next_buffer(ctx, data);
      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    }
  } else {
    if (chunk_data->id.rid > 0) {
      merger_put_next_chunk(ctx, data);
      if (ctx->rc != GRN_SUCCESS) {
        return false;
      }
    } else {
      return false;
    }
  }
  return true;
}

static grn_rc
chunk_flush(
  grn_ctx *ctx, grn_ii *ii, chunk_info *cinfo, uint8_t *enc, uint32_t encsize)
{
  uint32_t dcn;
  grn_io_win dw;
  if (encsize) {
    chunk_new(ctx, ii, &dcn, encsize);
    if (ctx->rc == GRN_SUCCESS) {
      auto dc = static_cast<uint8_t *>(
        WIN_MAP(ctx, ii->chunk, &dw, dcn, 0, encsize, GRN_IO_WRONLY));
      if (dc) {
        grn_memcpy(dc, enc, encsize);
        grn_io_win_unmap(ctx, &dw);
        cinfo->segno = dcn;
        cinfo->size = encsize;
      } else {
        chunk_free(ctx, ii, dcn, encsize);
        {
          GRN_DEFINE_NAME(ii);
          MERR("[ii][chunk][flush] failed to allocate a destination chunk: "
               "<%.*s>:"
               "segment:<%u>, size:<%u>",
               name_size,
               name,
               dcn,
               encsize);
        }
      }
    }
  } else {
    cinfo->segno = 0;
    cinfo->size = 0;
  }
  return ctx->rc;
}

static grn_rc
chunk_merge(grn_ctx *ctx,
            merger_data *data,
            chunk_info *cinfo,
            grn_id rid,
            datavec *dv,
            int32_t *balance)
{
  merger_buffer_data *buffer_data = &(data->source.buffer);
  merger_chunk_data *chunk_data = &(data->source.chunk);
  grn_ii *ii = data->ii;
  grn_id term_id = CHUNK_SPLIT_OFF(data->term_id);
  grn_io_win sw;
  uint32_t segno = cinfo->segno;
  uint32_t size = cinfo->size;
  uint32_t ndf = 0;
  auto scp = static_cast<const uint8_t *>(
    WIN_MAP(ctx, ii->chunk, &sw, segno, 0, size, GRN_IO_RDONLY));
  datavec rdv[MAX_N_ELEMENTS + 1];
  size_t bufsize = S_SEGMENT * ii->n_elements;

  if (!scp) {
    grn_obj term;
    GRN_DEFINE_NAME(ii);
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, term_id, &term);
    MERR("[ii][chunk][merge] failed to allocate a source chunk: "
         "<%.*s>: "
         "<%.*s>(%u): "
         "record:<%u>, segment:<%u>, size:<%u>",
         name_size,
         name,
         (int)GRN_TEXT_LEN(&term),
         GRN_TEXT_VALUE(&term),
         term_id,
         rid,
         segno,
         size);
    GRN_OBJ_FIN(ctx, &term);
    return ctx->rc;
  }

  data->last_id = null_docinfo;
  chunk_data->section_id_gaps = NULL;
  chunk_data->weights = NULL;

  datavec_init(ctx, ii, rdv, 0, 0);
  if (ctx->rc != GRN_SUCCESS) {
    grn_io_win_unmap(ctx, &sw);
    {
      GRN_DEFINE_NAME(ii);
      ERR(ctx->rc,
          "[ii][chunk][merge] failed to initialize data vector: <%.*s>",
          name_size,
          name);
    }
    return ctx->rc;
  }

  {
    int decoded_size;
    decoded_size = grn_decv(ctx, ii, term_id, scp, cinfo->size, rdv);
    if (decoded_size == 0) {
      grn_obj term;
      grn_rc rc = ctx->rc;
      GRN_DEFINE_NAME(ii);
      GRN_TEXT_INIT(&term, 0);
      grn_ii_get_term(ctx, ii, term_id, &term);
      if (rc == GRN_SUCCESS) {
        rc = GRN_UNKNOWN_ERROR;
      }
      ERR(rc,
          "[ii][chunk][merge] failed to decode: "
          "<%.*s>: "
          "<%.*s>(%u): "
          "<%u>: "
          "<%u>",
          name_size,
          name,
          (int)GRN_TEXT_LEN(&term),
          GRN_TEXT_VALUE(&term),
          term_id,
          rid,
          cinfo->size);
      GRN_OBJ_FIN(ctx, &term);
      goto exit;
    }
    bufsize += decoded_size;
  }
  merger_init_chunk_data(ctx, data, rdv);
  /* (df in chunk list) = a[1] - chunk_data->n_documents; */
  datavec_reset(ctx, ii, dv, chunk_data->n_documents + S_SEGMENT, bufsize);
  if (ctx->rc != GRN_SUCCESS) {
    grn_obj term;
    GRN_DEFINE_NAME(ii);
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, term_id, &term);
    ERR(ctx->rc,
        "[ii][chunk][merge] failed to reset data vector: "
        "<%.*s>: "
        "<%.*s>(%u)",
        name_size,
        name,
        (int)GRN_TEXT_LEN(&term),
        GRN_TEXT_VALUE(&term),
        term_id);
    GRN_OBJ_FIN(ctx, &term);
    goto exit;
  }
  {
    int j = 0;
    data->dest.record_id_gaps = dv[j++].data;
    if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
      data->dest.section_id_gaps = dv[j++].data;
    }
    data->dest.tfs = dv[j++].data;
    if ((ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
      data->dest.weights = dv[j++].data;
    }
    data->dest.position_gaps = dv[j++].data;
    data->dest.position_gaps_end = dv[j].data;
  }
  merger_get_next_chunk(ctx, data);
  if (ctx->rc != GRN_SUCCESS) {
    grn_obj term;
    GRN_DEFINE_NAME(ii);
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, term_id, &term);
    ERR(ctx->rc,
        "[ii][chunk][merge] failed to get the next chunk posting: "
        "<%.*s>: "
        "<%.*s>(%u)",
        name_size,
        name,
        (int)GRN_TEXT_LEN(&term),
        GRN_TEXT_VALUE(&term),
        term_id);
    GRN_OBJ_FIN(ctx, &term);
    goto exit;
  }
  data->position = 0;
  do {
    if (!merger_merge(ctx, data)) {
      break;
    }
  } while (buffer_data->id.rid <= rid || chunk_data->id.rid);
  if (ctx->rc != GRN_SUCCESS) {
    grn_obj term;
    GRN_DEFINE_NAME(ii);
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, term_id, &term);
    ERR(ctx->rc,
        "[ii][chunk][merge] failed to merge: "
        "<%.*s>: "
        "<%.*s>(%u)",
        name_size,
        name,
        (int)GRN_TEXT_LEN(&term),
        GRN_TEXT_VALUE(&term),
        term_id);
    GRN_OBJ_FIN(ctx, &term);
    goto exit;
  }
  ndf = data->dest.record_id_gaps - dv[0].data;
  {
    const uint32_t n_positions =
      data->dest.position_gaps - dv[ii->n_elements - 1].data;
    datavec_set_data_size(ctx,
                          ii,
                          dv,
                          ndf,
                          data->last_id.rid,
                          n_positions,
                          data->max_position);
    const ssize_t encsize_estimated = grn_encv(ctx, ii, term_id, dv, NULL, 0);
    if (encsize_estimated == -1) {
      grn_obj term;
      GRN_DEFINE_NAME(ii);
      GRN_TEXT_INIT(&term, 0);
      grn_ii_get_term(ctx, ii, term_id, &term);
      MERR("[ii][chunk][merge] failed to estimate encode buffer size: "
           "<%.*s>: "
           "<%.*s>(%u): "
           "record:<%u>, "
           "segment:<%u>, "
           "size:<%u>, "
           "estimated-buffer-size: <%" GRN_FMT_SSIZE ">",
           name_size,
           name,
           (int)GRN_TEXT_LEN(&term),
           GRN_TEXT_VALUE(&term),
           term_id,
           rid,
           segno,
           size,
           encsize_estimated);
      GRN_OBJ_FIN(ctx, &term);
      goto exit;
    }
    if (encsize_estimated == 0) {
      chunk_flush(ctx, ii, cinfo, NULL, 0);
    } else {
      auto enc = static_cast<uint8_t *>(GRN_MALLOC(encsize_estimated));
      if (!enc) {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, ii, term_id, &term);
        MERR("[ii][chunk][merge] failed to allocate a encode buffer: "
             "<%.*s>: "
             "<%.*s>(%u): "
             "record:<%u>, "
             "segment:<%u>, "
             "size:<%u>, "
             "estimated-buffer-size: <%" GRN_FMT_SSIZE ">",
             name_size,
             name,
             (int)GRN_TEXT_LEN(&term),
             GRN_TEXT_VALUE(&term),
             term_id,
             rid,
             segno,
             size,
             encsize_estimated);
        GRN_OBJ_FIN(ctx, &term);
        goto exit;
      }
      const ssize_t encsize =
        grn_encv(ctx, ii, term_id, dv, enc, encsize_estimated);
      if (encsize == -1) {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, ii, term_id, &term);
        MERR("[ii][chunk][merge] failed to encode: "
             "<%.*s>: "
             "<%.*s>(%u): "
             "record:<%u>, "
             "segment:<%u>, "
             "size:<%u>, "
             "estimated-buffer-size: <%" GRN_FMT_SSIZE ">",
             name_size,
             name,
             (int)GRN_TEXT_LEN(&term),
             GRN_TEXT_VALUE(&term),
             term_id,
             rid,
             segno,
             size,
             encsize_estimated);
        GRN_OBJ_FIN(ctx, &term);
        GRN_FREE(enc);
        goto exit;
      }
      chunk_flush(ctx, ii, cinfo, enc, encsize);
      if (ctx->rc == GRN_SUCCESS) {
        grn_merging_data_add_dest_chunk(ctx,
                                        data->merging_data,
                                        cinfo->segno,
                                        cinfo->size);
      }
      GRN_FREE(enc);
    }
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    grn_merging_data_add_source_chunk(ctx, data->merging_data, segno, size);
  }
  *balance += (ndf - chunk_data->n_documents);

exit:
  datavec_fin(ctx, rdv);
  grn_io_win_unmap(ctx, &sw);

  return ctx->rc;
}

static void
buffer_merge_dump_datavec(grn_ctx *ctx, grn_ii *ii, datavec *dv, datavec *rdv)
{
  uint32_t i, j;
  grn_obj buffer;

  GRN_TEXT_INIT(&buffer, 0);
  for (i = 0; i < ii->n_elements; i++) {
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "rdv[%d] data_size=%d, flags=%d",
            i,
            rdv[i].data_size,
            rdv[i].flags);
    GRN_BULK_REWIND(&buffer);
    for (j = 0; j < rdv[i].data_size;) {
      grn_text_printf(ctx, &buffer, " %d", rdv[i].data[j]);
      j++;
      if (!(j % 32) || j == rdv[i].data_size) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "rdv[%d].data[%d]%.*s",
                i,
                j,
                (int)GRN_TEXT_LEN(&buffer),
                GRN_TEXT_VALUE(&buffer));
        GRN_BULK_REWIND(&buffer);
      }
    }
  }

  for (i = 0; i < ii->n_elements; i++) {
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "dv[%d] data_size=%d, flags=%d",
            i,
            dv[i].data_size,
            dv[i].flags);
    GRN_BULK_REWIND(&buffer);
    for (j = 0; j < dv[i].data_size;) {
      grn_text_printf(ctx, &buffer, " %d", dv[i].data[j]);
      j++;
      if (!(j % 32) || j == dv[i].data_size) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "dv[%d].data[%d]%.*s",
                i,
                j,
                (int)GRN_TEXT_LEN(&buffer),
                GRN_TEXT_VALUE(&buffer));
        GRN_BULK_REWIND(&buffer);
      }
    }
  }

  GRN_OBJ_FIN(ctx, &buffer);
}

static void
buffer_merge_ensure_dc(grn_ctx *ctx,
                       grn_ii *ii,
                       uint8_t **dc,
                       uint8_t **dcp,
                       size_t *dc_size,
                       size_t required_size,
                       const char *tag)
{
  size_t dc_used_size = *dcp - *dc;
  if (required_size <= (*dc_size - dc_used_size)) {
    return;
  }

  size_t new_dc_size = *dc_size * 2;
  while (required_size > (new_dc_size - dc_used_size)) {
    new_dc_size *= 2;
  }
  auto new_dc = static_cast<uint8_t *>(GRN_REALLOC(*dc, new_dc_size));
  if (new_dc) {
    GRN_DEFINE_NAME(ii);
    GRN_LOG(ctx,
            GRN_LOG_INFO,
            "[ii][buffer][merge]%s destination chunk is expanded: "
            "<%.*s>: "
            "<%" GRN_FMT_SIZE "> -> <%" GRN_FMT_SIZE ">",
            tag,
            name_size,
            name,
            *dc_size,
            new_dc_size);
    *dcp = new_dc + dc_used_size;
    *dc = new_dc;
    *dc_size = new_dc_size;
  } else {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    GRN_DEFINE_NAME(ii);
    ERR(rc,
        "[ii][buffer][merge]%s failed to expand destination chunk: "
        "<%.*s>: "
        "<%" GRN_FMT_SIZE "> -> <%" GRN_FMT_SIZE ">",
        tag,
        name_size,
        name,
        *dc_size,
        new_dc_size);
  }
}

static inline grn_rc
buffer_merge_internal(grn_ctx *ctx,
                      grn_ii *ii,
                      uint32_t seg,
                      grn_hash *h,
                      buffer *sb,
                      const uint8_t *sc,
                      buffer *db,
                      uint8_t **dc_output,
                      grn_merging_data *merging_data)
{
  buffer_term *bt;
  uint8_t *dcp = NULL;
  datavec dv[MAX_N_ELEMENTS + 1];
  datavec rdv[MAX_N_ELEMENTS + 1];
  uint16_t n = db->header.nterms, nterms_void = 0;
  size_t unitsize = (S_SEGMENT + sb->header.chunk_size / sb->header.nterms) * 2;
  // size_t unitsize = (S_SEGMENT + sb->header.chunk_size) * 2 + (1<<24);
  size_t totalsize = unitsize * ii->n_elements;

  // todo : realloc
  datavec_init(ctx, ii, dv, unitsize, totalsize);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_DEFINE_NAME(ii);
    ERR(ctx->rc,
        "[ii][buffer][merge] failed to initialize data vector: "
        "<%.*s>: "
        "unit-size:<%" GRN_FMT_SIZE ">, "
        "total-size:<%" GRN_FMT_SIZE ">",
        name_size,
        name,
        unitsize,
        totalsize);
    return ctx->rc;
  }
  datavec_init(ctx, ii, rdv, 0, 0);
  if (ctx->rc != GRN_SUCCESS) {
    datavec_fin(ctx, dv);
    GRN_DEFINE_NAME(ii);
    ERR(ctx->rc,
        "[ii][buffer][merge] failed to initialize data vector for read: "
        "<%.*s>",
        name_size,
        name);
    return ctx->rc;
  }

  size_t dc_size = (sb->header.chunk_size + S_SEGMENT) * 2;
  auto dc = static_cast<uint8_t *>(GRN_MALLOC(dc_size));
  if (!dc) {
    grn_rc rc = ctx->rc;
    if (rc == GRN_SUCCESS) {
      rc = GRN_NO_MEMORY_AVAILABLE;
    }
    GRN_DEFINE_NAME(ii);
    ERR(rc,
        "[ii][buffer][merge] failed to allocate destination chunk: "
        "<%.*s>: <%" GRN_FMT_SIZE ">",
        name_size,
        name,
        dc_size);
    goto exit;
  }
  dcp = dc;

  for (bt = db->terms; n; n--, bt++) {
    merger_data data = {};
    merger_buffer_data *buffer_data = &(data.source.buffer);
    merger_chunk_data *chunk_data = &(data.source.chunk);
    int32_t balance = 0;
    uint32_t nchunks = 0;
    uint32_t nvchunks = 0;
    chunk_info *cinfo = NULL;
    grn_id crid = GRN_ID_NIL;

    if (!bt->tid) {
      nterms_void++;
      continue;
    }

    data.ii = ii;
    data.term_id = bt->tid;
    data.merging_data = merging_data;
    buffer_data->buf = sb;
    chunk_data->data_start = sc;

    if (bt->pos_in_buffer == 0) {
      if (bt->size_in_buffer > 0) {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, ii, CHUNK_SPLIT_OFF(bt->tid), &term);
        GRN_LOG(ctx,
                GRN_WARN,
                "[ii][buffer][merge] invalid size for buffer term: "
                "<%.*s>: "
                "<%.*s>(%u): "
                "size:<%u>",
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                CHUNK_SPLIT_OFF(bt->tid),
                bt->size_in_buffer);
        GRN_OBJ_FIN(ctx, &term);
        goto exit;
      }
      if (bt->size_in_chunk > 0) {
        buffer_merge_ensure_dc(ctx,
                               ii,
                               &dc,
                               &dcp,
                               &dc_size,
                               bt->size_in_chunk,
                               "[copy-chunk]");
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
        grn_memcpy(dcp,
                   chunk_data->data_start + bt->pos_in_chunk,
                   bt->size_in_chunk);
        bt->pos_in_chunk = (uint32_t)(dcp - dc);
        dcp += bt->size_in_chunk;
      }
      continue;
    }

    buffer_data->next_position = bt->pos_in_buffer;
    merger_get_next_buffer(ctx, &data);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
    if (chunk_data->data_start && bt->size_in_chunk > 0) {
      size_t size = S_SEGMENT * ii->n_elements;
      chunk_data->data = chunk_data->data_start + bt->pos_in_chunk;
      chunk_data->data_end = chunk_data->data + bt->size_in_chunk;
      if ((bt->tid & CHUNK_SPLIT)) {
        uint32_t i;
        GRN_B_DEC(nchunks, chunk_data->data);
        if (!(cinfo = GRN_MALLOCN(chunk_info, nchunks + 1))) {
          grn_obj term;
          GRN_DEFINE_NAME(ii);
          GRN_TEXT_INIT(&term, 0);
          grn_ii_get_term(ctx, ii, CHUNK_SPLIT_OFF(bt->tid), &term);
          MERR("[ii][buffer][merge] failed to allocate chunk info: "
               "<%.*s>: "
               "<%.*s>(%u): "
               "segment:<%u>, "
               "n-chunks:<%u>, "
               "unit-size:<%" GRN_FMT_SIZE ">, "
               "total-size:<%" GRN_FMT_SIZE ">",
               name_size,
               name,
               (int)GRN_TEXT_LEN(&term),
               GRN_TEXT_VALUE(&term),
               CHUNK_SPLIT_OFF(bt->tid),
               seg,
               nchunks,
               unitsize,
               totalsize);
          GRN_OBJ_FIN(ctx, &term);
          goto exit;
        }
        grn_id dgap_keep = GRN_ID_NIL;
        for (i = 0; i < nchunks; i++) {
          GRN_B_DEC(cinfo[i].segno, chunk_data->data);
          GRN_B_DEC(cinfo[i].size, chunk_data->data);
          GRN_B_DEC(cinfo[i].dgap, chunk_data->data);
          crid += cinfo[i].dgap;
          if (buffer_data->id.rid <= crid) {
            chunk_merge(ctx, &data, &cinfo[i], crid, dv, &balance);
            if (ctx->rc != GRN_SUCCESS) {
              if (cinfo) {
                GRN_FREE(cinfo);
              }
              {
                grn_obj term;
                GRN_DEFINE_NAME(ii);
                GRN_TEXT_INIT(&term, 0);
                grn_ii_get_term(ctx, ii, CHUNK_SPLIT_OFF(bt->tid), &term);
                ERR(ctx->rc,
                    "[ii][buffer][merge] failed to merge chunk: "
                    "<%.*s>: "
                    "<%.*s>(%u): "
                    "chunk:<%u>, "
                    "n-chunks:<%u>",
                    name_size,
                    name,
                    (int)GRN_TEXT_LEN(&term),
                    GRN_TEXT_VALUE(&term),
                    CHUNK_SPLIT_OFF(bt->tid),
                    i,
                    nchunks);
                GRN_OBJ_FIN(ctx, &term);
              }
              goto exit;
            }
          }
          if (cinfo[i].size == 0) {
            dgap_keep += cinfo[i].dgap;
          } else {
            cinfo[i].dgap += dgap_keep;
            dgap_keep = GRN_ID_NIL;
            nvchunks++;
          }
        }
      }
      if (chunk_data->data_end > chunk_data->data) {
        int decoded_size;
        decoded_size = grn_decv(ctx,
                                ii,
                                CHUNK_SPLIT_OFF(bt->tid),
                                chunk_data->data,
                                chunk_data->data_end - chunk_data->data,
                                rdv);
        if (decoded_size == 0) {
          if (cinfo) {
            GRN_FREE(cinfo);
          }
          {
            grn_obj term;
            grn_rc rc = ctx->rc;
            GRN_DEFINE_NAME(ii);
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, ii, CHUNK_SPLIT_OFF(bt->tid), &term);
            if (rc == GRN_SUCCESS) {
              rc = GRN_UNKNOWN_ERROR;
            }
            ERR(rc,
                "[ii][buffer][merge] failed to decode: "
                "<%.*s>: "
                "<%.*s>(%u): "
                "n-chunks:<%u>",
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                CHUNK_SPLIT_OFF(bt->tid),
                nvchunks);
            GRN_OBJ_FIN(ctx, &term);
          }
          goto exit;
        }
        size += decoded_size;
        merger_init_chunk_data(ctx, &data, rdv);
        datavec_reset(ctx, ii, dv, chunk_data->n_documents + S_SEGMENT, size);
        if (ctx->rc != GRN_SUCCESS) {
          if (cinfo) {
            GRN_FREE(cinfo);
          }
          {
            grn_obj term;
            GRN_DEFINE_NAME(ii);
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, ii, CHUNK_SPLIT_OFF(bt->tid), &term);
            ERR(ctx->rc,
                "[ii][buffer][merge] failed to reset data vector: "
                "<%.*s>: "
                "<%.*s>(%u): "
                "unit-size:<%" GRN_FMT_SIZE ">, "
                "total-size:<%" GRN_FMT_SIZE ">",
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                CHUNK_SPLIT_OFF(bt->tid),
                (size_t)(chunk_data->n_documents + S_SEGMENT),
                size);
            GRN_OBJ_FIN(ctx, &term);
          }
          goto exit;
        }
      }
    }
    {
      int j = 0;
      data.dest.record_id_gaps = dv[j++].data;
      if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
        data.dest.section_id_gaps = dv[j++].data;
      }
      data.dest.tfs = dv[j++].data;
      if ((ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
        data.dest.weights = dv[j++].data;
      }
      data.dest.position_gaps = dv[j++].data;
      data.dest.position_gaps_end = dv[j].data;
    }
    data.last_id = null_docinfo;
    merger_get_next_chunk(ctx, &data);
    if (ctx->rc != GRN_SUCCESS) {
      if (cinfo) {
        GRN_FREE(cinfo);
      }
      goto exit;
    }
    data.position = 0;
    while (merger_merge(ctx, &data)) {
    }
    if (ctx->rc != GRN_SUCCESS) {
      if (cinfo) {
        GRN_FREE(cinfo);
      }
      {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, ii, CHUNK_SPLIT_OFF(bt->tid), &term);
        ERR(ctx->rc,
            "[ii][buffer][merge] failed to merge chunk: "
            "<%.*s>: "
            "<%.*s>(%u)",
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            CHUNK_SPLIT_OFF(bt->tid));
        GRN_OBJ_FIN(ctx, &term);
      }
      goto exit;
    }
    {
      /* TODO: Is n_documents better? */
      uint32_t ndf = data.dest.record_id_gaps - dv[0].data;
      grn_id tid = CHUNK_SPLIT_OFF(bt->tid);
      uint32_t *a = array_at(ctx, ii, tid);
      if (!a) {
        /* TODO: warning */
        GRN_LOG(ctx, GRN_LOG_DEBUG, "array_entry not found tid=%d", tid);
        memset(bt, 0, sizeof(buffer_term));
        nterms_void++;
      } else {
        if (!ndf && !nvchunks) {
          a[0] = 0;
          a[1] = 0;
          lexicon_delete(ctx, ii, tid, h);
          memset(bt, 0, sizeof(buffer_term));
          nterms_void++;
        } else if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION) &&
                   !nvchunks && ndf == 1 && data.last_id.rid < 0x100000 &&
                   data.last_id.sid < 0x800 && data.last_id.tf == 1 &&
                   data.last_id.weight == 0) {
          a[0] = POS_EMBED_RID_SID(data.last_id.rid, data.last_id.sid);
          if (ii->header.common->flags & GRN_OBJ_WITH_POSITION) {
            a[1] = data.dest.position_gaps[-1];
          } else {
            a[1] = 0;
          }
          memset(bt, 0, sizeof(buffer_term));
          nterms_void++;
        } else if (!(ii->header.common->flags & GRN_OBJ_WITH_SECTION) &&
                   !nvchunks && ndf == 1 && data.last_id.tf == 1 &&
                   data.last_id.weight == 0) {
          a[0] = POS_EMBED_RID(data.last_id.rid);
          if (ii->header.common->flags & GRN_OBJ_WITH_POSITION) {
            a[1] = data.dest.position_gaps[-1];
          } else {
            a[1] = 0;
          }
          memset(bt, 0, sizeof(buffer_term));
          nterms_void++;
        } else {
          const uint32_t n_positions =
            data.dest.position_gaps - dv[ii->n_elements - 1].data;
          datavec_set_data_size(ctx,
                                ii,
                                dv,
                                ndf,
                                data.last_id.rid,
                                n_positions,
                                data.max_position);
          const size_t dc_offset = dcp - dc;
          a[1] = (bt->size_in_chunk ? a[1] : 0) +
                 (ndf - chunk_data->n_documents) + balance;
          if (nvchunks) {
            buffer_merge_ensure_dc(ctx,
                                   ii,
                                   &dc,
                                   &dcp,
                                   &dc_size,
                                   GRN_B_ENC_MAX_SIZE * (1 + 3 * nchunks),
                                   "[encode-chunk-metadata]");
            if (ctx->rc != GRN_SUCCESS) {
              if (cinfo) {
                GRN_FREE(cinfo);
              }
              array_unref(ctx, ii, tid);
              goto exit;
            }
            GRN_B_ENC(nvchunks, dcp);
            for (uint32_t i = 0; i < nchunks; i++) {
              if (cinfo[i].size) {
                GRN_B_ENC(cinfo[i].segno, dcp);
                GRN_B_ENC(cinfo[i].size, dcp);
                GRN_B_ENC(cinfo[i].dgap, dcp);
              }
            }
          }
          const ssize_t estimated_encsize = grn_encv(ctx, ii, tid, dv, NULL, 0);
          if (estimated_encsize == -1) {
            grn_obj term;
            GRN_DEFINE_NAME(ii);
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, ii, tid, &term);
            ERR(ctx->rc,
                "[ii][buffer][merge] failed to estimate encode buffer size: "
                "<%.*s>: "
                "<%.*s>(%u)",
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                tid);
            GRN_OBJ_FIN(ctx, &term);
            buffer_merge_dump_datavec(ctx, ii, dv, rdv);
            if (cinfo) {
              GRN_FREE(cinfo);
            }
            array_unref(ctx, ii, tid);
            goto exit;
          }
          buffer_merge_ensure_dc(ctx,
                                 ii,
                                 &dc,
                                 &dcp,
                                 &dc_size,
                                 estimated_encsize,
                                 "[encode-data-vector]");
          if (ctx->rc != GRN_SUCCESS) {
            buffer_merge_dump_datavec(ctx, ii, dv, rdv);
            if (cinfo) {
              GRN_FREE(cinfo);
            }
            array_unref(ctx, ii, tid);
            goto exit;
          }

          const ssize_t encsize =
            grn_encv(ctx, ii, tid, dv, dcp, dc_size - (dcp - dc));
          if (encsize == -1) {
            grn_obj term;
            GRN_DEFINE_NAME(ii);
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, ii, tid, &term);
            ERR(ctx->rc,
                "[ii][buffer][merge] failed to encode: "
                "<%.*s>: "
                "<%.*s>(%u): "
                "<%" GRN_FMT_SSIZE ">",
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                tid,
                estimated_encsize);
            GRN_OBJ_FIN(ctx, &term);
            buffer_merge_dump_datavec(ctx, ii, dv, rdv);
            if (cinfo) {
              GRN_FREE(cinfo);
            }
            array_unref(ctx, ii, tid);
            goto exit;
          }

          if (encsize > CHUNK_SPLIT_THRESHOLD &&
              (cinfo || (cinfo = GRN_MALLOCN(chunk_info, nchunks + 1))) &&
              !chunk_flush(ctx, ii, &cinfo[nchunks], dcp, encsize)) {
            uint32_t i;
            cinfo[nchunks].dgap = data.last_id.rid - crid;
            nvchunks++;
            dcp = dc + dc_offset;
            GRN_B_ENC(nvchunks, dcp);
            for (i = 0; i <= nchunks; i++) {
              if (cinfo[i].size) {
                GRN_B_ENC(cinfo[i].segno, dcp);
                GRN_B_ENC(cinfo[i].size, dcp);
                GRN_B_ENC(cinfo[i].dgap, dcp);
              }
            }
            /* TODO: format */
            GRN_LOG(ctx,
                    GRN_LOG_DEBUG,
                    "split (%d) encsize=%" GRN_FMT_SSIZE,
                    tid,
                    encsize);
            bt->tid |= CHUNK_SPLIT;
          } else {
            dcp += encsize;
            if (!nvchunks) {
              bt->tid &= ~CHUNK_SPLIT;
            }
          }
          bt->pos_in_chunk = (uint32_t)dc_offset;
          bt->size_in_chunk = (uint32_t)(dcp - (dc + dc_offset));
          bt->size_in_buffer = 0;
          bt->pos_in_buffer = 0;
        }
        array_unref(ctx, ii, tid);
      }
    }
    if (cinfo) {
      GRN_FREE(cinfo);
    }
  }
  db->header.chunk_size = (uint32_t)(dcp - dc);
  db->header.buffer_free =
    S_SEGMENT - sizeof(buffer_header) - db->header.nterms * sizeof(buffer_term);
  db->header.nterms_void = nterms_void;

  if (grn_ii_dump_index_source_on_merge) {
    merge_dump_source(ctx, ii, sb, sc, GRN_LOG_DEBUG);
  }

exit:
  if (ctx->rc == GRN_SUCCESS) {
    *dc_output = dc;
  } else {
    if (dc) {
      GRN_FREE(dc);
    }
  }
  datavec_fin(ctx, dv);
  datavec_fin(ctx, rdv);
  return ctx->rc;
}

static grn_rc
buffer_merge(grn_ctx *ctx,
             grn_ii *ii,
             uint32_t seg,
             grn_hash *h,
             buffer *sb,
             const uint8_t *sc,
             buffer *db,
             uint8_t **dc_output,
             grn_merging_data *merging_data)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_rc rc =
    buffer_merge_internal(ctx, ii, seg, h, sb, sc, db, dc_output, merging_data);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(ii);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[ii][buffer][merge][slow][%f] "
            "<%.*s>: "
            "physical-segment:<%u>, "
            "logical-segment:<%u>",
            elapsed_time,
            name_size,
            name,
            grn_ii_get_buffer_pseg_inline(ii, seg),
            seg);
  }
  GRN_SLOW_LOG_POP_END(ctx);
  return rc;
}

static void
fake_map(grn_ctx *ctx,
         grn_io *io,
         grn_io_win *iw,
         void *addr,
         uint32_t seg,
         uint32_t size)
{
  iw->ctx = ctx;
  iw->diff = 0;
  iw->io = io;
  iw->mode = GRN_IO_WRONLY;
  iw->segment = ((seg) >> GRN_II_N_CHUNK_VARIATION);
  iw->offset =
    (((seg) & ((1 << GRN_II_N_CHUNK_VARIATION) - 1)) << GRN_II_W_LEAST_CHUNK);
  iw->size = size;
  iw->cached = 0;
  iw->addr = addr;
}

static inline grn_rc
buffer_flush_internal(grn_ctx *ctx, grn_ii *ii, uint32_t lseg, grn_hash *h)
{
  grn_io_win sw, dw;
  buffer *sb;
  uint32_t ds, pseg, scn, dcn = 0;
  if (grn_ii_get_buffer_pseg_inline(ii, lseg) == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_DEFINE_NAME(ii);
    CRIT(GRN_FILE_CORRUPT,
         "[ii][buffer][flush] invalid segment: "
         "<%.*s>: "
         "request:<%u>, max:<%u>",
         name_size,
         name,
         lseg,
         ii->seg->header->max_segment);
    return ctx->rc;
  }
  ds = segment_get(ctx, ii);
  if (ds == ii->seg->header->max_segment) {
    GRN_DEFINE_NAME(ii);
    MERR("[ii][buffer][flush] segment is full: "
         "<%.*s>: "
         "request:<%u>, max:<%u>",
         name_size,
         name,
         lseg,
         ii->seg->header->max_segment);
    return ctx->rc;
  }
  if (ds == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_DEFINE_NAME(ii);
    MERR("[ii][buffer][flush] couldn't assign a segment: "
         "<%.*s>: "
         "request:<%u>, max:<%u>",
         name_size,
         name,
         lseg,
         ii->seg->header->max_segment);
    return ctx->rc;
  }
  pseg = buffer_open(ctx, ii, grn_ii_pos_pack(ii, lseg, 0), NULL, &sb);
  if (pseg == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_DEFINE_NAME(ii);
    MERR("[ii][buffer][flush] failed to open buffer: "
         "<%.*s>: "
         "segment:<%u>, position:<%u>, max:<%u>",
         name_size,
         name,
         lseg,
         grn_ii_pos_pack(ii, lseg, 0),
         ii->seg->header->max_segment);
    return ctx->rc;
  }
  {
    auto db = static_cast<buffer *>(grn_io_seg_ref(ctx, ii->seg, ds));
    if (db) {
      const uint8_t *sc = NULL;
      if ((scn = sb->header.chunk) == GRN_II_PSEG_NOT_ASSIGNED ||
          (sc = static_cast<const uint8_t *>(WIN_MAP(ctx,
                                                     ii->chunk,
                                                     &sw,
                                                     scn,
                                                     0,
                                                     sb->header.chunk_size,
                                                     GRN_IO_RDONLY)))) {
        uint16_t n = sb->header.nterms;
        memset(db, 0, S_SEGMENT);
        grn_memcpy(db->terms, sb->terms, n * sizeof(buffer_term));
        db->header.nterms = n;
        uint8_t *dc = NULL;
        grn_merging_data merging_data;
        grn_merging_data_init(ctx, &merging_data, ii);
        buffer_merge(ctx, ii, lseg, h, sb, sc, db, &dc, &merging_data);
        if (ctx->rc == GRN_SUCCESS) {
          const uint32_t actual_chunk_size = db->header.chunk_size;
          bool need_chunk_free = true;
          if (actual_chunk_size > 0) {
            chunk_new(ctx, ii, &dcn, actual_chunk_size);
            if (ctx->rc == GRN_SUCCESS) {
              need_chunk_free = true;
            }
          }
          if (ctx->rc == GRN_SUCCESS) {
            grn_rc rc;
            db->header.chunk =
              (actual_chunk_size > 0) ? dcn : GRN_II_PSEG_NOT_ASSIGNED;
            fake_map(ctx, ii->chunk, &dw, dc, dcn, actual_chunk_size);
            rc = grn_io_win_unmap(ctx, &dw);
            if (rc == GRN_SUCCESS) {
              dc = NULL;
              buffer_segment_update(ii, lseg, ds);
              ii->header.common->total_chunk_size += actual_chunk_size;
              need_chunk_free = false;
              merging_data.succeeded = true;
              if (scn != GRN_II_PSEG_NOT_ASSIGNED) {
                chunk_free(ctx, ii, scn, sb->header.chunk_size);
                ii->header.common->total_chunk_size -= sb->header.chunk_size;
              }
            } else {
              GRN_DEFINE_NAME(ii);
              ERR(rc,
                  "[ii][buffer][flush] failed to unmap a destination chunk: "
                  "<%.*s> : "
                  "segment:<%u>, destination-segment:<%u>, actual-size:<%u>",
                  name_size,
                  name,
                  lseg,
                  dcn,
                  actual_chunk_size);
            }
          }
          if (need_chunk_free) {
            chunk_free(ctx, ii, dcn, actual_chunk_size);
          }
          if (dc) {
            GRN_FREE(dc);
          }
        }
        grn_merging_data_fin(ctx, &merging_data);
        if (scn != GRN_II_PSEG_NOT_ASSIGNED) {
          grn_io_win_unmap(ctx, &sw);
        }
      } else {
        GRN_DEFINE_NAME(ii);
        MERR("[ii][buffer][flush] failed to map a source chunk: "
             "<%.*s>: "
             "segment:<%u>, source-segment:<%u>, chunk-size:<%u>",
             name_size,
             name,
             lseg,
             scn,
             sb->header.chunk_size);
      }
      grn_io_seg_unref(ctx, ii->seg, ds);
    } else {
      GRN_DEFINE_NAME(ii);
      MERR("[ii][buffer][flush] failed to allocate a destination segment: "
           "<%.*s>: "
           "segment:<%u>, destination-segment:<%u>",
           name_size,
           name,
           lseg,
           ds);
    }
    buffer_close(ctx, ii, pseg);
  }
  return ctx->rc;
}

static grn_rc
buffer_flush(grn_ctx *ctx, grn_ii *ii, uint32_t lseg, grn_hash *h)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_rc rc = buffer_flush_internal(ctx, ii, lseg, h);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(ii);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[ii][buffer][flush][slow][%f] "
            "<%.*s>: "
            "physical-segment:<%u>, "
            "logical-segment:<%u>",
            elapsed_time,
            name_size,
            name,
            grn_ii_get_buffer_pseg_inline(ii, lseg),
            lseg);
  }
  GRN_SLOW_LOG_POP_END(ctx);
  return rc;
}

extern "C" void
grn_ii_buffer_check(grn_ctx *ctx, grn_ii *ii, uint32_t lseg)
{
  grn_io_win sw;
  buffer *sb;
  uint8_t *sc = NULL;
  uint32_t pseg, scn, nterms_with_corrupt_chunk = 0, nterm_with_chunk = 0;
  uint32_t ndeleted_terms_with_value = 0;
  buffer_term *bt;
  datavec rdv[MAX_N_ELEMENTS + 1];
  uint16_t n;
  int nterms_void = 0;
  int size_in_buffer = 0;
  grn_obj buf;
  size_t lower_bound;
  int64_t nloops = 0, nviolations = 0;
  if (grn_ii_get_buffer_pseg_inline(ii, lseg) == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_OUTPUT_BOOL(false);
    return;
  }
  pseg = buffer_open(ctx, ii, grn_ii_pos_pack(ii, lseg, 0), NULL, &sb);
  if (pseg == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_OUTPUT_BOOL(false);
    return;
  }
  lower_bound =
    (sb->header.buffer_free + sizeof(buffer_term) * sb->header.nterms) /
    sizeof(buffer_rec);
  datavec_init(ctx, ii, rdv, 0, 0);
  GRN_OUTPUT_MAP_OPEN("BUFFER", -1);
  GRN_OUTPUT_CSTR("buffer id");
  GRN_OUTPUT_INT64(lseg);
  if ((scn = sb->header.chunk) == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_OUTPUT_CSTR("void chunk size");
    GRN_OUTPUT_INT64(sb->header.chunk_size);
  } else {
    if ((sc = static_cast<uint8_t *>(WIN_MAP(ctx,
                                             ii->chunk,
                                             &sw,
                                             scn,
                                             0,
                                             sb->header.chunk_size,
                                             GRN_IO_RDONLY)))) {
      GRN_OUTPUT_CSTR("chunk size");
      GRN_OUTPUT_INT64(sb->header.chunk_size);
    } else {
      GRN_OUTPUT_CSTR("unmappable chunk size");
      GRN_OUTPUT_INT64(sb->header.chunk_size);
    }
  }
  GRN_OUTPUT_CSTR("buffer term");
  GRN_OUTPUT_ARRAY_OPEN("TERMS", sb->header.nterms);

  GRN_OBJ_INIT(&buf, GRN_BULK, 0, ii->lexicon->header.domain);
  for (bt = sb->terms, n = sb->header.nterms; n; n--, bt++) {
    grn_id tid, tid_;
    char key[GRN_TABLE_MAX_KEY_SIZE];
    int key_size;
    uint32_t nchunks = 0;
    chunk_info *cinfo = NULL;
    grn_id crid = GRN_ID_NIL;
    merger_data data = {};
    merger_chunk_data *chunk_data = &(data.source.chunk);

    if (!bt->tid && !bt->pos_in_buffer && !bt->size_in_buffer) {
      nterms_void++;
      continue;
    }
    GRN_OUTPUT_ARRAY_OPEN("TERM", -1);
    tid = CHUNK_SPLIT_OFF(bt->tid);
    key_size =
      grn_table_get_key(ctx, ii->lexicon, tid, key, GRN_TABLE_MAX_KEY_SIZE);
    tid_ = grn_table_get(ctx, ii->lexicon, key, key_size);
    GRN_TEXT_SET(ctx, &buf, key, key_size);
    GRN_OUTPUT_OBJ(&buf, NULL);
    GRN_OUTPUT_INT64(bt->tid);
    GRN_OUTPUT_INT64(tid_);

    size_in_buffer += bt->size_in_buffer;
    if (tid != tid_ && (bt->size_in_buffer || bt->size_in_chunk)) {
      ndeleted_terms_with_value++;
    }
    GRN_OUTPUT_INT64(bt->size_in_buffer);
    GRN_OUTPUT_INT64(bt->size_in_chunk);

    data.ii = ii;
    data.term_id = bt->tid;
    data.source.buffer.buf = sb;
    data.source.buffer.next_position = bt->pos_in_buffer;
    chunk_data->data_start = sc;
    merger_get_next_buffer(ctx, &data);
    if (sc && bt->size_in_chunk) {
      size_t size = S_SEGMENT * ii->n_elements;
      chunk_data->data = chunk_data->data_start + bt->pos_in_chunk;
      chunk_data->data_end = chunk_data->data + bt->size_in_chunk;
      if ((bt->tid & CHUNK_SPLIT)) {
        uint32_t i;
        GRN_B_DEC(nchunks, chunk_data->data);
        if (!(cinfo = GRN_MALLOCN(chunk_info, nchunks + 1))) {
          datavec_fin(ctx, rdv);
          GRN_OBJ_FIN(ctx, &buf);
          return;
        }
        for (i = 0; i < nchunks; i++) {
          GRN_B_DEC(cinfo[i].segno, chunk_data->data);
          GRN_B_DEC(cinfo[i].size, chunk_data->data);
          GRN_B_DEC(cinfo[i].dgap, chunk_data->data);
          crid += cinfo[i].dgap;
        }
      }
      if (chunk_data->data_end > chunk_data->data) {
        size += grn_decv(ctx,
                         ii,
                         crid,
                         chunk_data->data,
                         chunk_data->data_end - chunk_data->data,
                         rdv);
        merger_init_chunk_data(ctx, &data, rdv);
        GRN_OUTPUT_INT64(chunk_data->n_documents);
        GRN_OUTPUT_INT64(chunk_data->position_gaps_end -
                         chunk_data->position_gaps);
        {
          int j = 0;
          j++;
          if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
            j++;
          }
          if (chunk_data->n_documents != rdv[j].data_size) {
            nterms_with_corrupt_chunk++;
          }
          j++;
          if ((ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
            j++;
          }
        }
        nterm_with_chunk++;
      }
    }
    {
      uint16_t pos;
      grn_id rid, sid, rid_ = 0, sid_ = 0;
      uint8_t *p;
      buffer_rec *r;
      for (pos = bt->pos_in_buffer; pos; pos = r->step) {
        if (pos < lower_bound) {
          nviolations++;
        }
        r = BUFFER_REC_AT(sb, pos);
        p = GRN_NEXT_ADDR(r);
        GRN_B_DEC(rid, p);
        if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
          GRN_B_DEC(sid, p);
        } else {
          sid = 1;
        }
        if (rid < rid_ || (rid == rid_ && sid < sid_)) {
          nloops++;
        }
        rid_ = rid;
        sid_ = sid;
      }
    }
    GRN_OUTPUT_ARRAY_CLOSE();
    if (cinfo) {
      GRN_FREE(cinfo);
    }
  }
  GRN_OBJ_FIN(ctx, &buf);

  GRN_OUTPUT_ARRAY_CLOSE();
  GRN_OUTPUT_CSTR("buffer free");
  GRN_OUTPUT_INT64(sb->header.buffer_free);
  GRN_OUTPUT_CSTR("size in buffer");
  GRN_OUTPUT_INT64(size_in_buffer);
  GRN_OUTPUT_CSTR("nterms");
  GRN_OUTPUT_INT64(sb->header.nterms);
  if (nterms_void != sb->header.nterms_void) {
    GRN_OUTPUT_CSTR("nterms void gap");
    GRN_OUTPUT_INT64(nterms_void - sb->header.nterms_void);
  }
  GRN_OUTPUT_CSTR("nterms with chunk");
  GRN_OUTPUT_INT64(nterm_with_chunk);
  if (nterms_with_corrupt_chunk) {
    GRN_OUTPUT_CSTR("nterms with corrupt chunk");
    GRN_OUTPUT_INT64(nterms_with_corrupt_chunk);
  }
  if (ndeleted_terms_with_value) {
    GRN_OUTPUT_CSTR("number of deleted terms with value");
    GRN_OUTPUT_INT64(ndeleted_terms_with_value);
  }
  if (nloops) {
    GRN_OUTPUT_CSTR("number of loops");
    GRN_OUTPUT_INT64(nloops);
  }
  if (nviolations) {
    GRN_OUTPUT_CSTR("number of violations");
    GRN_OUTPUT_INT64(nviolations);
  }
  GRN_OUTPUT_MAP_CLOSE();
  datavec_fin(ctx, rdv);
  if (sc) {
    grn_io_win_unmap(ctx, &sw);
  }
  buffer_close(ctx, ii, pseg);
}

typedef struct {
  buffer_term *bt;
  const char *key;
  uint32_t key_size;
} term_sort;

static int
term_compar(const void *t1, const void *t2)
{
  int r;
  const term_sort *x = (term_sort *)t1, *y = (term_sort *)t2;
  if (x->key_size > y->key_size) {
    r = memcmp(x->key, y->key, y->key_size);
    return r ? r : (int)(x->key_size - y->key_size);
  } else {
    r = memcmp(x->key, y->key, x->key_size);
    return r ? r : (int)(x->key_size - y->key_size);
  }
}

static grn_rc
term_split(grn_ctx *ctx, grn_obj *lexicon, buffer *sb, buffer *db0, buffer *db1)
{
  uint16_t i, n, *nt;
  buffer_term *bt;
  uint32_t s, th = (sb->header.chunk_size + sb->header.nterms) >> 1;
  auto ts =
    static_cast<term_sort *>(GRN_MALLOC(sb->header.nterms * sizeof(term_sort)));
  if (!ts) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  for (i = 0, n = sb->header.nterms, bt = sb->terms; n; bt++, n--) {
    if (bt->tid) {
      grn_id tid = CHUNK_SPLIT_OFF(bt->tid);
      ts[i].key = _grn_table_key(ctx, lexicon, tid, &ts[i].key_size);
      ts[i].bt = bt;
      i++;
    }
  }
  qsort(ts, i, sizeof(term_sort), term_compar);
  memset(db0, 0, S_SEGMENT);
  bt = db0->terms;
  nt = &db0->header.nterms;
  for (s = 0; n + 1 < i && s <= th; n++, bt++) {
    grn_memcpy(bt, ts[n].bt, sizeof(buffer_term));
    (*nt)++;
    s += ts[n].bt->size_in_chunk + 1;
  }
  memset(db1, 0, S_SEGMENT);
  bt = db1->terms;
  nt = &db1->header.nterms;
  for (; n < i; n++, bt++) {
    grn_memcpy(bt, ts[n].bt, sizeof(buffer_term));
    (*nt)++;
  }
  GRN_FREE(ts);
  GRN_LOG(ctx,
          GRN_LOG_DEBUG,
          "d0=%d d1=%d",
          db0->header.nterms,
          db1->header.nterms);
  return GRN_SUCCESS;
}

static void
array_update(grn_ctx *ctx, grn_ii *ii, uint32_t dls, buffer *db)
{
  uint16_t n;
  buffer_term *bt;
  uint32_t loffset = POS_LOFFSET_HEADER;
  uint32_t *a;
  for (n = db->header.nterms, bt = db->terms; n; n--, bt++) {
    if (bt->tid) {
      grn_id tid = CHUNK_SPLIT_OFF(bt->tid);
      if ((a = array_at(ctx, ii, tid))) {
        a[0] = grn_ii_pos_pack(ii, dls, loffset);
        array_unref(ctx, ii, tid);
      } else {
        GRN_LOG(ctx, GRN_LOG_WARNING, "array_at failed (%d)", tid);
      }
    }
    loffset += POS_LOFFSET_HEADER;
  }
}

static inline grn_rc
buffer_split_internal(grn_ctx *ctx, grn_ii *ii, uint32_t lseg, grn_hash *h)
{
  grn_io_win sw, dw0, dw1;
  buffer *sb, *db0 = NULL, *db1 = NULL;
  uint32_t dps0 = 0, dps1 = 0, dls0 = 0, dls1 = 0, sps, scn, dcn0 = 0, dcn1 = 0;
  if (grn_ii_get_buffer_pseg_inline(ii, lseg) == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_DEFINE_NAME(ii);
    CRIT(GRN_FILE_CORRUPT,
         "[ii][buffer][split] invalid segment: "
         "<%.*s> :"
         "request:<%u>, max:<%u>",
         name_size,
         name,
         lseg,
         ii->seg->header->max_segment);
    return ctx->rc;
  }
  buffer_segment_reserve(ctx, ii, &dls0, &dps0, &dls1, &dps1);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_DEFINE_NAME(ii);
    ERR(ctx->rc,
        "[ii][buffer][split] failed to reserve buffer segments: "
        "<%.*s> :"
        "request:<%u>, max:<%u>",
        name_size,
        name,
        lseg,
        ii->seg->header->max_segment);
    return ctx->rc;
  }
  sps = buffer_open(ctx, ii, grn_ii_pos_pack(ii, lseg, 0), NULL, &sb);
  if (sps == GRN_II_PSEG_NOT_ASSIGNED) {
    GRN_DEFINE_NAME(ii);
    MERR("[ii][buffer][split] failed to open buffer: "
         "<%.*s> :"
         "segment:<%u>, position:<%u>, max-segment:<%u>",
         name_size,
         name,
         lseg,
         grn_ii_pos_pack(ii, lseg, 0),
         ii->seg->header->max_segment);
  } else {
    db0 = static_cast<buffer *>(grn_io_seg_ref(ctx, ii->seg, dps0));
    if (db0) {
      db1 = static_cast<buffer *>(grn_io_seg_ref(ctx, ii->seg, dps1));
      if (db1) {
        const uint8_t *sc = NULL;
        if ((scn = sb->header.chunk) == GRN_II_PSEG_NOT_ASSIGNED ||
            (sc = static_cast<const uint8_t *>(WIN_MAP(ctx,
                                                       ii->chunk,
                                                       &sw,
                                                       scn,
                                                       0,
                                                       sb->header.chunk_size,
                                                       GRN_IO_RDONLY)))) {
          term_split(ctx, ii->lexicon, sb, db0, db1);
          uint8_t *dc0 = NULL;
          grn_merging_data merging_data;
          grn_merging_data_init(ctx, &merging_data, ii);
          buffer_merge(ctx, ii, lseg, h, sb, sc, db0, &dc0, &merging_data);
          if (ctx->rc == GRN_SUCCESS) {
            const uint32_t actual_db0_chunk_size = db0->header.chunk_size;
            bool need_db0_chunk_free = false;
            if (actual_db0_chunk_size > 0) {
              chunk_new(ctx, ii, &dcn0, actual_db0_chunk_size);
              if (ctx->rc == GRN_SUCCESS) {
                need_db0_chunk_free = true;
              }
            }
            if (ctx->rc == GRN_SUCCESS) {
              grn_rc rc;
              db0->header.chunk =
                actual_db0_chunk_size ? dcn0 : GRN_II_PSEG_NOT_ASSIGNED;
              fake_map(ctx, ii->chunk, &dw0, dc0, dcn0, actual_db0_chunk_size);
              rc = grn_io_win_unmap(ctx, &dw0);
              if (rc == GRN_SUCCESS) {
                dc0 = NULL;
                uint8_t *dc1 = NULL;
                buffer_merge(ctx,
                             ii,
                             lseg,
                             h,
                             sb,
                             sc,
                             db1,
                             &dc1,
                             &merging_data);
                if (ctx->rc == GRN_SUCCESS) {
                  const uint32_t actual_db1_chunk_size = db1->header.chunk_size;
                  bool need_db1_chunk_free = false;
                  if (actual_db1_chunk_size > 0) {
                    chunk_new(ctx, ii, &dcn1, actual_db1_chunk_size);
                    if (ctx->rc == GRN_SUCCESS) {
                      need_db1_chunk_free = true;
                    }
                  }
                  if (ctx->rc == GRN_SUCCESS) {
                    fake_map(ctx,
                             ii->chunk,
                             &dw1,
                             dc1,
                             dcn1,
                             actual_db1_chunk_size);
                    rc = grn_io_win_unmap(ctx, &dw1);
                    if (rc == GRN_SUCCESS) {
                      dc1 = NULL;
                      db1->header.chunk = (actual_db1_chunk_size > 0)
                                            ? dcn1
                                            : GRN_II_PSEG_NOT_ASSIGNED;
                      buffer_segment_update(ii, dls0, dps0);
                      buffer_segment_update(ii, dls1, dps1);
                      array_update(ctx, ii, dls0, db0);
                      array_update(ctx, ii, dls1, db1);
                      buffer_segment_clear(ii, lseg);
                      ii->header.common->total_chunk_size +=
                        actual_db0_chunk_size + actual_db1_chunk_size;
                      need_db0_chunk_free = false;
                      need_db1_chunk_free = false;
                      merging_data.succeeded = true;
                      if (scn != GRN_II_PSEG_NOT_ASSIGNED) {
                        chunk_free(ctx, ii, scn, sb->header.chunk_size);
                        ii->header.common->total_chunk_size -=
                          sb->header.chunk_size;
                      }
                    } else {
                      GRN_DEFINE_NAME(ii);
                      ERR(rc,
                          "[ii][buffer][merge] "
                          "failed to unmap a destination chunk2: "
                          "<%.*s>: "
                          "segment:<%u>, "
                          "destination-chunk1:<%u>, "
                          "destination-chunk2:<%u>, "
                          "actual-size1:<%u>, "
                          "actual-size2:<%u>",
                          name_size,
                          name,
                          lseg,
                          dcn0,
                          dcn1,
                          actual_db0_chunk_size,
                          actual_db1_chunk_size);
                    }
                  }
                  if (need_db1_chunk_free) {
                    chunk_free(ctx, ii, dcn1, actual_db1_chunk_size);
                  }
                  if (dc1) {
                    GRN_FREE(dc1);
                  }
                }
              } else {
                GRN_DEFINE_NAME(ii);
                ERR(rc,
                    "[ii][buffer][merge] "
                    "failed to unmap a destination chunk1: "
                    "<%.*s> :"
                    "segment:<%u>, "
                    "destination-chunk1:<%u>, "
                    "actual-size1:<%u>",
                    name_size,
                    name,
                    lseg,
                    dcn0,
                    actual_db0_chunk_size);
              }
            }
            if (need_db0_chunk_free) {
              chunk_free(ctx, ii, dcn0, actual_db0_chunk_size);
            }
            if (dc0) {
              GRN_FREE(dc0);
            }
          }
          grn_merging_data_fin(ctx, &merging_data);
          if (scn != GRN_II_PSEG_NOT_ASSIGNED) {
            grn_io_win_unmap(ctx, &sw);
          }
        } else {
          GRN_DEFINE_NAME(ii);
          MERR("[ii][buffer][split] failed to map a source chunk: "
               "<%.*s> :"
               "segment:<%u>, "
               "source-segment:<%u>, "
               "chunk-size:<%u>",
               name_size,
               name,
               lseg,
               scn,
               sb->header.chunk_size);
        }
        grn_io_seg_unref(ctx, ii->seg, dps1);
      } else {
        GRN_DEFINE_NAME(ii);
        MERR("[ii][buffer][split] failed to allocate a destination segment2: "
             "<%.*s>: "
             "segment:<%u>, "
             "destination-segment1:<%u>, "
             "destination-segment2:<%u>",
             name_size,
             name,
             lseg,
             dps0,
             dps1);
      }
      grn_io_seg_unref(ctx, ii->seg, dps0);
    } else {
      GRN_DEFINE_NAME(ii);
      MERR("[ii][buffer][split] failed to allocate a destination segment1: "
           "<%.*s>: "
           "segment:<%u>, "
           "destination-segment1:<%u>, "
           "destination-segment2:<%u>",
           name_size,
           name,
           lseg,
           dps0,
           dps1);
    }
    buffer_close(ctx, ii, sps);
  }
  return ctx->rc;
}

static grn_rc
buffer_split(grn_ctx *ctx, grn_ii *ii, uint32_t lseg, grn_hash *h)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_rc rc = buffer_split_internal(ctx, ii, lseg, h);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(ii);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[ii][buffer][split][slow][%f] "
            "<%.*s>: "
            "physical-segment:<%u>, "
            "logical-segment:<%u>",
            elapsed_time,
            name_size,
            name,
            grn_ii_get_buffer_pseg_inline(ii, lseg),
            lseg);
  }
  GRN_SLOW_LOG_POP_END(ctx);
  return rc;
}

#define SCALE_FACTOR 2048
#define MAX_NTERMS   8192
#define SPLIT_COND(ii, buffer)                                                 \
  ((buffer)->header.nterms > 1024 ||                                           \
   ((buffer)->header.nterms > 1 && (buffer)->header.chunk_size * 100 >         \
                                     (ii)->header.common->total_chunk_size))

static inline void
buffer_new_find_segment(grn_ctx *ctx,
                        grn_ii *ii,
                        int size,
                        grn_id tid,
                        grn_hash *h,
                        buffer **b,
                        uint32_t *lseg,
                        uint32_t *pseg)
{
  uint32_t *a;

  a = array_at(ctx, ii, tid);
  if (!a) {
    return;
  }

  for (;;) {
    uint32_t pos = a[0];
    if (!pos || POS_IS_EMBED(pos)) {
      break;
    }
    *pseg = buffer_open(ctx, ii, pos, NULL, b);
    if (*pseg == GRN_II_PSEG_NOT_ASSIGNED) {
      break;
    }
    if ((*b)->header.buffer_free >= size + sizeof(buffer_term)) {
      *lseg = grn_ii_pos_lseg(ii, pos);
      break;
    }
    buffer_close(ctx, ii, *pseg);
    if (SPLIT_COND(ii, (*b))) {
      /* ((S_SEGMENT - sizeof(buffer_header) + ii->header.common->bmax -
         (*b)->header.nterms * sizeof(buffer_term)) * 4 <
         (*b)->header.chunk_size) */
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "nterms=%d chunk=%d total=%" GRN_FMT_INT64U,
              (*b)->header.nterms,
              (*b)->header.chunk_size,
              ii->header.common->total_chunk_size >> 10);
      if (buffer_split(ctx, ii, grn_ii_pos_lseg(ii, pos), h)) {
        break;
      }
    } else {
      if (S_SEGMENT - sizeof(buffer_header) -
            (*b)->header.nterms * sizeof(buffer_term) <
          size + sizeof(buffer_term)) {
        break;
      }
      if (buffer_flush(ctx, ii, grn_ii_pos_lseg(ii, pos), h)) {
        break;
      }
    }
  }

  array_unref(ctx, ii, tid);
}

static inline void
buffer_new_lexicon_pat(grn_ctx *ctx,
                       grn_ii *ii,
                       int size,
                       grn_id id,
                       grn_hash *h,
                       buffer **b,
                       uint32_t *lseg,
                       uint32_t *pseg)
{
  grn_pat_cursor *cursor;
  char key[GRN_TABLE_MAX_KEY_SIZE];
  int key_size;

  key_size =
    grn_table_get_key(ctx, ii->lexicon, id, key, GRN_TABLE_MAX_KEY_SIZE);
  if (ii->lexicon->header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    grn_obj *tokenizer = NULL;

    grn_table_get_info(ctx, ii->lexicon, NULL, NULL, &tokenizer, NULL, NULL);
    if (tokenizer) {
      /* For natural language */
      cursor = grn_pat_cursor_open(ctx,
                                   (grn_pat *)(ii->lexicon),
                                   key,
                                   key_size,
                                   NULL,
                                   0,
                                   0,
                                   -1,
                                   GRN_CURSOR_ASCENDING | GRN_CURSOR_GT);
      if (cursor) {
        grn_id tid;
        while (ctx->rc == GRN_SUCCESS && *lseg == GRN_II_PSEG_NOT_ASSIGNED &&
               (tid = grn_pat_cursor_next(ctx, cursor))) {
          buffer_new_find_segment(ctx, ii, size, tid, h, b, lseg, pseg);
        }
        grn_pat_cursor_close(ctx, cursor);
      }
    } else {
      /* For binary data or text data */
      int target_key_size = key_size;
      int reduced_key_size = 0;

      bool is_binary_data = (ctx->encoding == GRN_ENC_NONE);

      while (*lseg == GRN_II_PSEG_NOT_ASSIGNED && target_key_size > 0) {
        grn_id tid;

        cursor = grn_pat_cursor_open(ctx,
                                     (grn_pat *)(ii->lexicon),
                                     key,
                                     target_key_size,
                                     NULL,
                                     0,
                                     0,
                                     -1,
                                     GRN_CURSOR_PREFIX);
        if (!cursor) {
          break;
        }

        if (reduced_key_size == 0) {
          while (ctx->rc == GRN_SUCCESS && *lseg == GRN_II_PSEG_NOT_ASSIGNED &&
                 (tid = grn_pat_cursor_next(ctx, cursor))) {
            buffer_new_find_segment(ctx, ii, size, tid, h, b, lseg, pseg);
          }
        } else {
          while (ctx->rc == GRN_SUCCESS && *lseg == GRN_II_PSEG_NOT_ASSIGNED &&
                 (tid = grn_pat_cursor_next(ctx, cursor))) {
            void *current_key;
            int current_key_size;

            current_key_size =
              grn_pat_cursor_get_key(ctx, cursor, &current_key);
            if (memcmp(((char *)current_key) + target_key_size,
                       key + target_key_size,
                       reduced_key_size) == 0) {
              continue;
            }
            buffer_new_find_segment(ctx, ii, size, tid, h, b, lseg, pseg);
          }
        }
        grn_pat_cursor_close(ctx, cursor);

        if (reduced_key_size == 0) {
          reduced_key_size = 1;
        } else {
          if (is_binary_data) {
            reduced_key_size *= 2;
          }
        }
        target_key_size -= reduced_key_size;
      }
    }
  } else {
    /* For other data */
    cursor = grn_pat_cursor_open(ctx,
                                 (grn_pat *)(ii->lexicon),
                                 NULL,
                                 0,
                                 key,
                                 key_size,
                                 0,
                                 -1,
                                 GRN_CURSOR_PREFIX);
    if (cursor) {
      grn_id tid;
      while (ctx->rc == GRN_SUCCESS && *lseg == GRN_II_PSEG_NOT_ASSIGNED &&
             (tid = grn_pat_cursor_next(ctx, cursor))) {
        buffer_new_find_segment(ctx, ii, size, tid, h, b, lseg, pseg);
      }
      grn_pat_cursor_close(ctx, cursor);
    }
  }
}

static inline void
buffer_new_lexicon_other(grn_ctx *ctx,
                         grn_ii *ii,
                         int size,
                         grn_id id,
                         grn_hash *h,
                         buffer **b,
                         uint32_t *lseg,
                         uint32_t *pseg)
{
  GRN_TABLE_EACH_BEGIN(ctx, ii->lexicon, cursor, tid)
  {
    if (ctx->rc != GRN_SUCCESS || *lseg != GRN_II_PSEG_NOT_ASSIGNED) {
      break;
    }
    buffer_new_find_segment(ctx, ii, size, tid, h, b, lseg, pseg);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
}

static inline uint32_t
buffer_new_internal(grn_ctx *ctx,
                    grn_ii *ii,
                    int size,
                    uint32_t *pos,
                    buffer_term **bt,
                    buffer_rec **br,
                    buffer **bp,
                    grn_id id,
                    grn_hash *h)
{
  buffer *b = NULL;
  uint16_t offset;
  uint32_t lseg = GRN_II_PSEG_NOT_ASSIGNED, pseg = GRN_II_PSEG_NOT_ASSIGNED;
  if (S_SEGMENT - sizeof(buffer_header) < size + sizeof(buffer_term)) {
    GRN_DEFINE_NAME(ii);
    MERR("[ii][buffer][new] requested size is too large: "
         "<%.*s> :"
         "requested:<%" GRN_FMT_SIZE ">, max:<%" GRN_FMT_SIZE ">",
         name_size,
         name,
         (size_t)(size + sizeof(buffer_term)),
         (size_t)(S_SEGMENT - sizeof(buffer_header)));
    return GRN_II_PSEG_NOT_ASSIGNED;
  }
  if (ii->lexicon->header.type == GRN_TABLE_PAT_KEY) {
    buffer_new_lexicon_pat(ctx, ii, size, id, h, &b, &lseg, &pseg);
  } else {
    buffer_new_lexicon_other(ctx, ii, size, id, h, &b, &lseg, &pseg);
  }
  if (lseg == GRN_II_PSEG_NOT_ASSIGNED) {
    if (buffer_segment_new(ctx, ii, &lseg) ||
        (pseg = buffer_open(ctx, ii, grn_ii_pos_pack(ii, lseg, 0), NULL, &b)) ==
          GRN_II_PSEG_NOT_ASSIGNED) {
      return GRN_II_PSEG_NOT_ASSIGNED;
    }
    memset(b, 0, S_SEGMENT);
    b->header.buffer_free = S_SEGMENT - sizeof(buffer_header);
    b->header.chunk = GRN_II_PSEG_NOT_ASSIGNED;
  }
  if (b->header.nterms_void) {
    for (offset = 0; offset < b->header.nterms; offset++) {
      if (!b->terms[offset].tid) {
        break;
      }
    }
    if (offset == b->header.nterms) {
      GRN_LOG(ctx, GRN_LOG_DEBUG, "inconsistent buffer(%d)", lseg);
      b->header.nterms_void = 0;
      b->header.nterms++;
      b->header.buffer_free -= size + sizeof(buffer_term);
    } else {
      b->header.nterms_void--;
      b->header.buffer_free -= size;
    }
  } else {
    offset = b->header.nterms++;
    b->header.buffer_free -= size + sizeof(buffer_term);
  }
  *pos =
    grn_ii_pos_pack(ii, lseg, POS_LOFFSET_HEADER + POS_LOFFSET_TERM * offset);
  *bt = &b->terms[offset];
  *br = (buffer_rec *)(((byte *)&b->terms[b->header.nterms]) +
                       b->header.buffer_free);
  *bp = b;
  return pseg;
}

static inline uint32_t
buffer_new(grn_ctx *ctx,
           grn_ii *ii,
           int size,
           uint32_t *pos,
           buffer_term **bt,
           buffer_rec **br,
           buffer **bp,
           grn_id id,
           grn_hash *h)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  uint32_t pseg = buffer_new_internal(ctx, ii, size, pos, bt, br, bp, id, h);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(ii);
    grn_obj term;
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, id, &term);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[ii][buffer][new][slow][%f] "
            "<%.*s>: "
            "<%.*s>(%u): "
            "size:<%u>, "
            "physical-segment:<%u>",
            elapsed_time,
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            id,
            size,
            pseg);
    GRN_OBJ_FIN(ctx, &term);
  }
  GRN_SLOW_LOG_POP_END(ctx);
  return pseg;
}

/* ii */

static grn_ii *
_grn_ii_create(
  grn_ctx *ctx, grn_ii *ii, const char *path, grn_obj *lexicon, uint32_t flags)
{
  int i;
  uint32_t header_size;
  uint32_t max_n_segments;
  uint32_t max_n_chunks;
  grn_io *seg, *chunk;
  char path2[PATH_MAX];
  grn_ii_header_common *header;
  grn_table_flags lflags;
  grn_encoding encoding;
  grn_obj *tokenizer;
  /*
  for (i = 0; i < 32; i++) {
    new_histogram[i] = 0;
    free_histogram[i] = 0;
  }
  */
  if (grn_table_get_info(ctx,
                         lexicon,
                         &lflags,
                         &encoding,
                         &tokenizer,
                         NULL,
                         NULL)) {
    return NULL;
  }
  if (path && strlen(path) + 6 >= PATH_MAX) {
    return NULL;
  }

  if (!grn_ii_pos_is_available(ctx, flags, "[ii][create]")) {
    return NULL;
  }

  if (flags & GRN_OBJ_INDEX_SMALL) {
    header_size = sizeof(grn_ii_header_normal);
    max_n_segments = grn_ii_max_n_segments_small;
    max_n_chunks = grn_ii_max_n_chunks_small;
  } else if (flags & GRN_OBJ_INDEX_MEDIUM) {
    header_size = sizeof(grn_ii_header_normal);
    max_n_segments = MAX_PSEG_MEDIUM;
    max_n_chunks = GRN_II_MAX_CHUNK_MEDIUM;
  } else if (flags & GRN_OBJ_INDEX_LARGE) {
    header_size = sizeof(grn_ii_header_large);
    max_n_segments = MAX_PSEG_LARGE;
    max_n_chunks = GRN_II_MAX_CHUNK;
  } else {
    header_size = sizeof(grn_ii_header_normal);
    max_n_segments = MAX_PSEG;
    max_n_chunks = GRN_II_MAX_CHUNK;
  }

  seg = grn_io_create(ctx,
                      path,
                      header_size,
                      S_SEGMENT,
                      max_n_segments,
                      GRN_IO_AUTO,
                      GRN_IO_EXPIRE_SEGMENT);
  if (!seg) {
    return NULL;
  }
  if (path) {
    grn_strcpy(path2, PATH_MAX, path);
    grn_strcat(path2, PATH_MAX, ".c");
    chunk = grn_io_create(ctx,
                          path2,
                          0,
                          S_CHUNK,
                          max_n_chunks,
                          GRN_IO_AUTO,
                          GRN_IO_EXPIRE_SEGMENT);
  } else {
    chunk = grn_io_create(ctx, NULL, 0, S_CHUNK, max_n_chunks, GRN_IO_AUTO, 0);
  }
  if (!chunk) {
    grn_io_close(ctx, seg);
    grn_io_remove(ctx, path);
    return NULL;
  }
  header = static_cast<grn_ii_header_common *>(grn_io_header(seg));
  grn_io_set_type(seg, GRN_COLUMN_INDEX);
  for (i = 0; i < GRN_II_MAX_LSEG; i++) {
    header->ainfo[i] = GRN_II_PSEG_NOT_ASSIGNED;
    header->binfo[i] = GRN_II_PSEG_NOT_ASSIGNED;
  }
  if (flags & GRN_OBJ_INDEX_LARGE) {
    grn_ii_header_large *header_large = (grn_ii_header_large *)header;
    for (i = 0; i < GRN_II_MAX_LSEG_EXTEND; i++) {
      header_large->ainfo_extend[i] = GRN_II_PSEG_NOT_ASSIGNED;
      header_large->binfo_extend[i] = GRN_II_PSEG_NOT_ASSIGNED;
    }
  }
  for (i = 0; i <= GRN_II_N_CHUNK_VARIATION; i++) {
    header->free_chunks[i] = GRN_II_PSEG_NOT_ASSIGNED;
    header->garbages[i] = GRN_II_PSEG_NOT_ASSIGNED;
  }
  header->flags = flags;
  ii->seg = seg;
  ii->chunk = chunk;
  ii->lexicon = grn_ctx_at(ctx, DB_OBJ(lexicon)->id);
  ii->lflags = lflags;
  ii->encoding = encoding;
  ii->header.common = header;
  ii->n_elements = 2;
  if ((flags & GRN_OBJ_WITH_SECTION)) {
    ii->n_elements++;
  }
  if ((flags & GRN_OBJ_WITH_WEIGHT)) {
    ii->n_elements++;
  }
  if ((flags & GRN_OBJ_WITH_POSITION)) {
    ii->n_elements++;
  }
  ii->wal_touched = false;
  return ii;
}

extern "C" grn_ii *
grn_ii_create(grn_ctx *ctx, const char *path, grn_obj *lexicon, uint32_t flags)
{
  auto ii = static_cast<grn_ii *>(GRN_CALLOC(sizeof(grn_ii)));
  if (!ii) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ii, GRN_COLUMN_INDEX);
  if (!_grn_ii_create(ctx, ii, path, lexicon, flags)) {
    GRN_FREE(ii);
    return NULL;
  }
  return ii;
}

extern "C" grn_rc
grn_ii_remove(grn_ctx *ctx, const char *path)
{
  grn_rc rc;
  char buffer[PATH_MAX];
  if (!path || strlen(path) > PATH_MAX - 4) {
    return GRN_INVALID_ARGUMENT;
  }
  grn_rc wal_rc = grn_wal_remove(ctx, path, "[ii]");
  if ((rc = grn_io_remove(ctx, path))) {
    goto exit;
  }
  grn_snprintf(buffer, PATH_MAX, PATH_MAX, "%s.c", path);
  rc = grn_io_remove(ctx, buffer);
  if (rc == GRN_SUCCESS) {
    rc = wal_rc;
  }
exit:
  return rc;
}

extern "C" grn_rc
grn_ii_truncate(grn_ctx *ctx, grn_ii *ii)
{
  grn_rc rc;
  const char *io_segpath;
  char *segpath = NULL;
  grn_obj *lexicon;
  uint32_t flags;
  if ((io_segpath = grn_io_path(ii->seg)) && *io_segpath != '\0') {
    if (!(segpath = GRN_STRDUP(io_segpath))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_segpath);
      return GRN_NO_MEMORY_AVAILABLE;
    }
  } else {
    segpath = NULL;
  }
  lexicon = ii->lexicon;
  flags = ii->header.common->flags;
  if ((rc = grn_io_close(ctx, ii->seg))) {
    goto exit;
  }
  if ((rc = grn_io_close(ctx, ii->chunk))) {
    goto exit;
  }
  ii->seg = NULL;
  ii->chunk = NULL;
  if (segpath && (rc = grn_ii_remove(ctx, segpath))) {
    goto exit;
  }
  if (!_grn_ii_create(ctx, ii, segpath, lexicon, flags)) {
    rc = GRN_UNKNOWN_ERROR;
    goto exit;
  }
  grn_obj_unref(ctx, lexicon);
exit:
  if (segpath) {
    GRN_FREE(segpath);
  }
  return rc;
}

extern "C" grn_ii *
grn_ii_open(grn_ctx *ctx, const char *path, grn_obj *lexicon)
{
  grn_io *seg, *chunk;
  char path2[PATH_MAX];
  grn_ii_header_common *header;
  uint32_t io_type;
  grn_table_flags lflags;
  grn_encoding encoding;
  grn_obj *tokenizer;
  if (grn_table_get_info(ctx,
                         lexicon,
                         &lflags,
                         &encoding,
                         &tokenizer,
                         NULL,
                         NULL)) {
    return NULL;
  }
  if (strlen(path) + 6 >= PATH_MAX) {
    return NULL;
  }
  grn_strcpy(path2, PATH_MAX, path);
  grn_strcat(path2, PATH_MAX, ".c");
  seg = grn_io_open(ctx, path, GRN_IO_AUTO);
  if (!seg) {
    return NULL;
  }
  chunk = grn_io_open(ctx, path2, GRN_IO_AUTO);
  if (!chunk) {
    grn_io_close(ctx, seg);
    return NULL;
  }
  header = static_cast<grn_ii_header_common *>(grn_io_header(seg));
  io_type = grn_io_get_type(seg);
  if (io_type != GRN_COLUMN_INDEX) {
    ERR(GRN_INVALID_FORMAT,
        "[column][index] file type must be %#04x: <%#04x>",
        GRN_COLUMN_INDEX,
        io_type);
    grn_io_close(ctx, seg);
    grn_io_close(ctx, chunk);
    return NULL;
  }
  auto ii = static_cast<grn_ii *>(GRN_CALLOC(sizeof(grn_ii)));
  if (!ii) {
    grn_io_close(ctx, seg);
    grn_io_close(ctx, chunk);
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(ii, GRN_COLUMN_INDEX);
  ii->seg = seg;
  ii->chunk = chunk;
  ii->lexicon = grn_ctx_at(ctx, DB_OBJ(lexicon)->id);
  ii->lflags = lflags;
  ii->encoding = encoding;
  ii->header.common = header;
  ii->n_elements = 2;
  if ((header->flags & GRN_OBJ_WITH_SECTION)) {
    ii->n_elements++;
  }
  if ((header->flags & GRN_OBJ_WITH_WEIGHT)) {
    ii->n_elements++;
  }
  if ((header->flags & GRN_OBJ_WITH_POSITION)) {
    ii->n_elements++;
  }
  ii->wal_touched = false;
  return ii;
}

extern "C" grn_rc
grn_ii_close(grn_ctx *ctx, grn_ii *ii)
{
  grn_rc rc = GRN_SUCCESS;
  if (!ii) {
    return GRN_INVALID_ARGUMENT;
  }
  if (ii->seg->path[0] != '\0' &&
      GRN_CTX_GET_WAL_ROLE(ctx) == GRN_WAL_ROLE_PRIMARY) {
    grn_obj_flush(ctx, (grn_obj *)ii);
  }
  {
    grn_rc sub_rc = grn_io_close(ctx, ii->seg);
    if (rc == GRN_SUCCESS) {
      rc = sub_rc;
    }
  }
  {
    grn_rc sub_rc = grn_io_close(ctx, ii->chunk);
    if (rc == GRN_SUCCESS) {
      rc = sub_rc;
    }
  }
  grn_obj_unref(ctx, ii->lexicon);
  GRN_FREE(ii);
  /*
  {
    int i;
    for (i = 0; i < 32; i++) {
      GRN_LOG(ctx, GRN_LOG_DEBUG, "new[%d]=%d free[%d]=%d",
              i, new_histogram[i],
              i, free_histogram[i]);
    }
  }
  */
  return rc;
}

extern "C" grn_rc
grn_ii_info(grn_ctx *ctx, grn_ii *ii, uint64_t *seg_size, uint64_t *chunk_size)
{
  grn_rc rc;

  if (seg_size) {
    if ((rc = grn_io_size(ctx, ii->seg, seg_size))) {
      return rc;
    }
  }

  if (chunk_size) {
    if ((rc = grn_io_size(ctx, ii->chunk, chunk_size))) {
      return rc;
    }
  }

  return GRN_SUCCESS;
}

extern "C" grn_column_flags
grn_ii_get_flags(grn_ctx *ctx, grn_ii *ii)
{
  if (!ii) {
    return 0;
  }

  return ii->header.common->flags;
}

extern "C" uint32_t
grn_ii_get_n_elements(grn_ctx *ctx, grn_ii *ii)
{
  if (!ii) {
    return 0;
  }

  return ii->n_elements;
}

extern "C" grn_obj *
grn_ii_get_lexicon(grn_ctx *ctx, grn_ii *ii)
{
  if (!ii) {
    return NULL;
  }

  return ii->lexicon;
}

extern "C" void
grn_ii_expire(grn_ctx *ctx, grn_ii *ii)
{
  int64_t threshold = ii->chunk->max_map_seg / 2;
  /*
  grn_io_expire(ctx, ii->seg, 128, 1000000);
  */
  if (0 <= grn_ii_reduce_expire_threshold &&
      grn_ii_reduce_expire_threshold < threshold) {
    threshold = grn_ii_reduce_expire_threshold;
  }
  if (ii->chunk->nmaps > threshold) {
    grn_io_expire(ctx, ii->chunk, 0, 1000000);
  }
}

extern "C" grn_rc
grn_ii_flush(grn_ctx *ctx, grn_ii *ii)
{
  grn_rc rc;

  rc = grn_io_flush(ctx, ii->seg);
  if (rc == GRN_SUCCESS) {
    rc = grn_io_flush(ctx, ii->chunk);
  }

  if (rc == GRN_SUCCESS) {
    ii->wal_touched = false;
  }

  return rc;
}

extern "C" size_t
grn_ii_get_disk_usage(grn_ctx *ctx, grn_ii *ii)
{
  size_t usage;

  usage = grn_io_get_disk_usage(ctx, ii->seg);
  usage += grn_io_get_disk_usage(ctx, ii->chunk);

  return usage;
}

extern "C" void
grn_ii_set_visibility(grn_ctx *ctx, grn_ii *ii, bool is_visible)
{
  if (is_visible) {
    ii->header.common->flags &= ~GRN_OBJ_INVISIBLE;
  } else {
    ii->header.common->flags |= GRN_OBJ_INVISIBLE;
  }
}

#define BIT11_01(x) ((x >> 1) & 0x7ff)
#define BIT31_12(x) (x >> 12)

static inline grn_rc
grn_ii_wal_touch(grn_ctx *ctx, grn_ii *ii, const char *tag)
{
  if (ii->wal_touched) {
    return GRN_SUCCESS;
  }

  if (GRN_CTX_GET_WAL_ROLE(ctx) == GRN_WAL_ROLE_NONE) {
    return GRN_SUCCESS;
  }

  grn_rc rc = grn_wal_touch(ctx, (grn_obj *)ii, false, tag);
  if (rc == GRN_SUCCESS) {
    ii->wal_touched = true;
  }
  return rc;
}

static inline grn_rc
grn_ii_update_one_internal(
  grn_ctx *ctx, grn_ii *ii, grn_id tid, grn_ii_updspec *u, grn_hash *h)
{
  const char *tag = "[ii][update][one]";
  buffer *b;
  uint8_t *bs;
  buffer_rec *br = NULL;
  buffer_term *bt;
  uint32_t pseg = 0, pos = 0, size, *a;
  if (!tid) {
    return ctx->rc;
  }
  if (!u->tf || !u->sid) {
    return grn_ii_delete_one(ctx, ii, tid, u, h);
  }
  if (u->sid > ii->header.common->smax) {
    ii->header.common->smax = u->sid;
  }
  if (!(a = array_get(ctx, ii, tid))) {
    grn_obj term;
    GRN_DEFINE_NAME(ii);
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, tid, &term);
    MERR("%s failed to allocate an array: "
         "<%.*s>: "
         "<%.*s>(%u): "
         "(%u:%u)",
         tag,
         name_size,
         name,
         (int)GRN_TEXT_LEN(&term),
         GRN_TEXT_VALUE(&term),
         tid,
         u->rid,
         u->sid);
    GRN_OBJ_FIN(ctx, &term);
    return ctx->rc;
  }
  if (!(bs = encode_rec(ctx, ii, u, &size, 0))) {
    grn_obj term;
    GRN_DEFINE_NAME(ii);
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, tid, &term);
    MERR("%s failed to encode a record: "
         "<%.*s>: "
         "<%.*s>(%u): "
         "(%u:%u)",
         tag,
         name_size,
         name,
         (int)GRN_TEXT_LEN(&term),
         GRN_TEXT_VALUE(&term),
         tid,
         u->rid,
         u->sid);
    GRN_OBJ_FIN(ctx, &term);
    goto exit;
  }
  if (grn_ii_wal_touch(ctx, ii, tag) != GRN_SUCCESS) {
    goto exit;
  }
  for (;;) {
    if (a[0]) {
      if (!POS_IS_EMBED(a[0])) {
        pos = a[0];
        if ((pseg = buffer_open(ctx, ii, pos, &bt, &b)) ==
            GRN_II_PSEG_NOT_ASSIGNED) {
          grn_obj term;
          GRN_DEFINE_NAME(ii);
          GRN_TEXT_INIT(&term, 0);
          grn_ii_get_term(ctx, ii, tid, &term);
          MERR("%s failed to allocate a buffer: "
               "<%.*s>: "
               "<%.*s>(%u): "
               "(%u:%u): "
               "segment:<%u>, "
               "free:<%u>, "
               "required:<%u>",
               tag,
               name_size,
               name,
               (int)GRN_TEXT_LEN(&term),
               GRN_TEXT_VALUE(&term),
               tid,
               u->rid,
               u->sid,
               pos,
               b->header.buffer_free,
               size);
          GRN_OBJ_FIN(ctx, &term);
          goto exit;
        }
        if (b->header.buffer_free < size) {
          int bfb = b->header.buffer_free;
          GRN_LOG(ctx,
                  GRN_LOG_DEBUG,
                  "flushing a[0]=%d seg=%d(%p) free=%d",
                  a[0],
                  grn_ii_pos_lseg(ii, a[0]),
                  b,
                  b->header.buffer_free);
          buffer_close(ctx, ii, pseg);
          if (SPLIT_COND(ii, b)) {
            /*((S_SEGMENT - sizeof(buffer_header) + ii->header.common->bmax -
               b->header.nterms * sizeof(buffer_term)) * 4 <
               b->header.chunk_size)*/
            GRN_LOG(ctx,
                    GRN_LOG_DEBUG,
                    "nterms=%d chunk=%d total=%" GRN_FMT_INT64U,
                    b->header.nterms,
                    b->header.chunk_size,
                    ii->header.common->total_chunk_size >> 10);
            buffer_split(ctx, ii, grn_ii_pos_lseg(ii, pos), h);
            if (ctx->rc != GRN_SUCCESS) {
              grn_obj term;
              char errbuf[GRN_CTX_MSGSIZE];
              GRN_DEFINE_NAME(ii);
              GRN_TEXT_INIT(&term, 0);
              grn_ii_get_term(ctx, ii, tid, &term);
              grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
              ERR(ctx->rc,
                  "%s failed to split a buffer: "
                  "<%.*s>: "
                  "<%.*s>(%u): "
                  "(%u:%u): "
                  "segment:<%u>, "
                  "free:<%u>, "
                  "required:<%u>, "
                  "%s",
                  tag,
                  name_size,
                  name,
                  (int)GRN_TEXT_LEN(&term),
                  GRN_TEXT_VALUE(&term),
                  tid,
                  u->rid,
                  u->sid,
                  pos,
                  b->header.buffer_free,
                  size,
                  errbuf);
              GRN_OBJ_FIN(ctx, &term);
              goto exit;
            }
            continue;
          }
          buffer_flush(ctx, ii, grn_ii_pos_lseg(ii, pos), h);
          if (ctx->rc != GRN_SUCCESS) {
            grn_obj term;
            char errbuf[GRN_CTX_MSGSIZE];
            GRN_DEFINE_NAME(ii);
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, ii, tid, &term);
            grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
            ERR(ctx->rc,
                "%s failed to flush a buffer: "
                "<%.*s>: "
                "<%u>:<%u>:<%u>: "
                "term:<%.*s>, "
                "segment:<%u>, "
                "free:<%u>, "
                "required:<%u>: "
                "%s",
                tag,
                name_size,
                name,
                u->rid,
                u->sid,
                tid,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                pos,
                b->header.buffer_free,
                size,
                errbuf);
            GRN_OBJ_FIN(ctx, &term);
            goto exit;
          }
          if (a[0] != pos) {
            GRN_LOG(ctx,
                    GRN_LOG_DEBUG,
                    "grn_ii_update_one: a[0] changed %d->%d",
                    a[0],
                    pos);
            continue;
          }
          if ((pseg = buffer_open(ctx, ii, pos, &bt, &b)) ==
              GRN_II_PSEG_NOT_ASSIGNED) {
            GRN_LOG(ctx, GRN_LOG_CRIT, "buffer not found a[0]=%d", a[0]);
            {
              grn_obj term;
              GRN_DEFINE_NAME(ii);
              GRN_TEXT_INIT(&term, 0);
              grn_ii_get_term(ctx, ii, tid, &term);
              MERR("%s failed to reallocate a buffer: "
                   "<%.*s>: "
                   "<%.*s>(%u): "
                   "(%u:%u): "
                   "segment:<%u>, "
                   "new-segment:<%u>, "
                   "free:<%u>, "
                   "required:<%u>",
                   tag,
                   name_size,
                   name,
                   (int)GRN_TEXT_LEN(&term),
                   GRN_TEXT_VALUE(&term),
                   tid,
                   u->rid,
                   u->sid,
                   pos,
                   a[0],
                   b->header.buffer_free,
                   size);
              GRN_OBJ_FIN(ctx, &term);
            }
            goto exit;
          }
          GRN_LOG(ctx,
                  GRN_LOG_DEBUG,
                  "flushed  a[0]=%d seg=%d(%p) free=%d->%d nterms=%d v=%d",
                  a[0],
                  grn_ii_pos_lseg(ii, a[0]),
                  b,
                  bfb,
                  b->header.buffer_free,
                  b->header.nterms,
                  b->header.nterms_void);
          if (b->header.buffer_free < size) {
            grn_obj term;
            GRN_DEFINE_NAME(ii);
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, ii, tid, &term);
            MERR("%s buffer is full: "
                 "<%.*s>: "
                 "<%.*s>(%u): "
                 "(%u:%u): "
                 "segment:<%u>, "
                 "new-segment:<%u>, "
                 "free:<%u>, "
                 "required:<%u>",
                 tag,
                 name_size,
                 name,
                 (int)GRN_TEXT_LEN(&term),
                 GRN_TEXT_VALUE(&term),
                 tid,
                 u->rid,
                 u->sid,
                 pos,
                 a[0],
                 b->header.buffer_free,
                 size);
            GRN_OBJ_FIN(ctx, &term);
            buffer_close(ctx, ii, pseg);
            /* todo: direct merge */
            goto exit;
          }
        }
        b->header.buffer_free -= size;
        br = (buffer_rec *)(((byte *)&b->terms[b->header.nterms]) +
                            b->header.buffer_free);
      } else {
        grn_ii_updspec u2;
        uint32_t size2 = 0, v = a[0];
        struct _grn_ii_pos pos2;
        pos2.pos = a[1];
        pos2.next = NULL;
        u2.pos = &pos2;
        if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
          u2.rid = POS_RID_SID_EXTRACT_RID(v);
          u2.sid = POS_RID_SID_EXTRACT_SID(v);
        } else {
          u2.rid = POS_RID_EXTRACT(v);
          u2.sid = 1;
        }
        u2.tf = 1;
        u2.weight = 0;
        if (u2.rid != u->rid || u2.sid != u->sid) {
          uint8_t *bs2 = encode_rec(ctx, ii, &u2, &size2, 0);
          if (!bs2) {
            grn_obj term;
            GRN_DEFINE_NAME(ii);
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, ii, tid, &term);
            MERR("%s failed to encode a record2: "
                 "<%.*s>: "
                 "<%.*s>(%u): "
                 "(%u:%u)",
                 tag,
                 name_size,
                 name,
                 (int)GRN_TEXT_LEN(&term),
                 GRN_TEXT_VALUE(&term),
                 tid,
                 u2.rid,
                 u2.sid);
            GRN_OBJ_FIN(ctx, &term);
            goto exit;
          }
          pseg = buffer_new(ctx, ii, size + size2, &pos, &bt, &br, &b, tid, h);
          if (pseg == GRN_II_PSEG_NOT_ASSIGNED) {
            GRN_FREE(bs2);
            {
              grn_obj term;
              GRN_DEFINE_NAME(ii);
              GRN_TEXT_INIT(&term, 0);
              grn_ii_get_term(ctx, ii, tid, &term);
              MERR("%s failed to create a buffer2: "
                   "<%.*s>: "
                   "<%.*s>(%u): "
                   "(%u:%u): "
                   "size:<%u>",
                   tag,
                   name_size,
                   name,
                   (int)GRN_TEXT_LEN(&term),
                   GRN_TEXT_VALUE(&term),
                   tid,
                   u2.rid,
                   u2.sid,
                   size + size2);
              GRN_OBJ_FIN(ctx, &term);
            }
            goto exit;
          }
          bt->tid = tid;
          bt->size_in_chunk = 0;
          bt->pos_in_chunk = 0;
          bt->size_in_buffer = 0;
          bt->pos_in_buffer = 0;
          buffer_put(ctx, ii, b, bt, br, bs2, &u2, size2);
          if (ctx->rc != GRN_SUCCESS) {
            GRN_FREE(bs2);
            buffer_close(ctx, ii, pseg);
            {
              grn_obj term;
              char errbuf[GRN_CTX_MSGSIZE];
              GRN_DEFINE_NAME(ii);
              GRN_TEXT_INIT(&term, 0);
              grn_ii_get_term(ctx, ii, tid, &term);
              grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
              MERR("%s failed to put to buffer: "
                   "<%.*s>: "
                   "<%.*s>(%u): "
                   "(%u:%u): "
                   "%s",
                   tag,
                   name_size,
                   name,
                   (int)GRN_TEXT_LEN(&term),
                   GRN_TEXT_VALUE(&term),
                   tid,
                   u2.rid,
                   u2.sid,
                   errbuf);
              GRN_OBJ_FIN(ctx, &term);
            }
            goto exit;
          }
          br = (buffer_rec *)(((byte *)br) + size2);
          GRN_FREE(bs2);
        }
      }
    }
    break;
  }
  if (!br) {
    if (u->tf == 1 && u->weight == 0) {
      if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
        if (u->rid < 0x100000 && u->sid < 0x800) {
          a[0] = (u->rid << 12) + (u->sid << 1) + 1;
          a[1] = u->pos->pos;
          goto exit;
        }
      } else {
        a[0] = (u->rid << 1) + 1;
        a[1] = u->pos->pos;
        goto exit;
      }
    }
    pseg = buffer_new(ctx, ii, size, &pos, &bt, &br, &b, tid, h);
    if (pseg == GRN_II_PSEG_NOT_ASSIGNED) {
      grn_obj term;
      GRN_DEFINE_NAME(ii);
      GRN_TEXT_INIT(&term, 0);
      grn_ii_get_term(ctx, ii, tid, &term);
      MERR("%s failed to create a buffer: "
           "<%.*s>: "
           "<%.*s>(%u): "
           "(%u:%u): "
           "size:<%u>",
           tag,
           name_size,
           name,
           (int)GRN_TEXT_LEN(&term),
           GRN_TEXT_VALUE(&term),
           tid,
           u->rid,
           u->sid,
           size);
      GRN_OBJ_FIN(ctx, &term);
      goto exit;
    }
    bt->tid = tid;
    bt->size_in_chunk = 0;
    bt->pos_in_chunk = 0;
    bt->size_in_buffer = 0;
    bt->pos_in_buffer = 0;
  }
  buffer_put(ctx, ii, b, bt, br, bs, u, size);
  buffer_close(ctx, ii, pseg);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  if (!a[0] || POS_IS_EMBED(a[0])) {
    a[0] = pos;
  }
exit:
  array_unref(ctx, ii, tid);
  if (bs) {
    GRN_FREE(bs);
  }
  if (u->tf != u->atf) {
    grn_obj *source_table = grn_ctx_at(ctx, DB_OBJ(ii)->range);
    GRN_DEFINE_NAME_CUSTOM(source_table, source_table_name);
    grn_obj_unref(ctx, source_table);
    GRN_DEFINE_NAME(ii);
    grn_obj term;
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, tid, &term);
    GRN_LOG(ctx,
            GRN_LOG_WARNING,
            "%s too many postings: "
            "<%.*s>: "
            "<%.*s>(%u): "
            "record:<%.*s>(%u:%u), "
            "n-postings:<%d>, "
            "n-discarded-postings:<%d>",
            tag,
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            tid,
            source_table_name_size,
            source_table_name,
            u->rid,
            u->sid,
            u->atf,
            u->atf - u->tf);
    GRN_OBJ_FIN(ctx, &term);
  }
  grn_ii_expire(ctx, ii);
  return ctx->rc;
}

extern "C" grn_rc
grn_ii_update_one(
  grn_ctx *ctx, grn_ii *ii, grn_id tid, grn_ii_updspec *u, grn_hash *h)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_rc rc = grn_ii_update_one_internal(ctx, ii, tid, u, h);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(ii);
    grn_obj *source_table = grn_ctx_at(ctx, DB_OBJ(ii)->range);
    GRN_DEFINE_NAME_CUSTOM(source_table, source_table_name);
    grn_obj_unref(ctx, source_table);
    grn_obj term;
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, tid, &term);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[ii][update][one][slow][%f] "
            "<%.*s>: "
            "<%.*s>(%u): "
            "record:<%.*s>(%u:%u), "
            "n-postings:<%d>, "
            "n-discarded-postings:<%d>",
            elapsed_time,
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            tid,
            source_table_name_size,
            source_table_name,
            u->rid,
            u->sid,
            u->atf,
            u->atf - u->tf);
    GRN_OBJ_FIN(ctx, &term);
  }
  GRN_SLOW_LOG_POP_END(ctx);
  return rc;
}

static inline grn_rc
grn_ii_delete_one_internal(
  grn_ctx *ctx, grn_ii *ii, grn_id tid, grn_ii_updspec *u, grn_hash *h)
{
  const char *tag = "[ii][delete][one]";
  buffer *b;
  uint8_t *bs = NULL;
  buffer_rec *br;
  buffer_term *bt;
  uint32_t pseg, size, *a;
  if (!tid) {
    return ctx->rc;
  }
  if (!(a = array_at(ctx, ii, tid))) {
    return ctx->rc;
  }
  if (grn_ii_wal_touch(ctx, ii, tag) != GRN_SUCCESS) {
    return ctx->rc;
  }
  for (;;) {
    const uint32_t pos = a[0];
    if (pos == 0) {
      goto exit;
    }
    if (POS_IS_EMBED(pos)) {
      if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
        const uint32_t rid = POS_RID_SID_EXTRACT_RID(pos);
        const uint32_t sid = POS_RID_SID_EXTRACT_SID(pos);
        if (u->rid == rid && (!u->sid || u->sid == sid)) {
          a[0] = 0;
          lexicon_delete(ctx, ii, tid, h);
        }
      } else {
        const uint32_t rid = POS_RID_EXTRACT(pos);
        if (u->rid == rid) {
          a[0] = 0;
          lexicon_delete(ctx, ii, tid, h);
        }
      }
      goto exit;
    }
    if (!(bs = encode_rec(ctx, ii, u, &size, 1))) {
      grn_obj term;
      GRN_DEFINE_NAME(ii);
      GRN_TEXT_INIT(&term, 0);
      grn_ii_get_term(ctx, ii, tid, &term);
      MERR("%s failed to encode a record: "
           "<%.*s>: "
           "<%.*s>(%u): "
           "(%u:%u)",
           tag,
           name_size,
           name,
           (int)GRN_TEXT_LEN(&term),
           GRN_TEXT_VALUE(&term),
           tid,
           u->rid,
           u->sid);
      GRN_OBJ_FIN(ctx, &term);
      goto exit;
    }
    if ((pseg = buffer_open(ctx, ii, pos, &bt, &b)) ==
        GRN_II_PSEG_NOT_ASSIGNED) {
      grn_obj term;
      GRN_DEFINE_NAME(ii);
      GRN_TEXT_INIT(&term, 0);
      grn_ii_get_term(ctx, ii, tid, &term);
      MERR("%s failed to allocate a buffer: "
           "<%.*s>: "
           "<%.*s>(%u): "
           "(%u:%u): "
           "position:<%u>",
           tag,
           name_size,
           name,
           (int)GRN_TEXT_LEN(&term),
           GRN_TEXT_VALUE(&term),
           tid,
           u->rid,
           u->sid,
           pos);
      GRN_OBJ_FIN(ctx, &term);
      goto exit;
    }
    if (b->header.buffer_free < size) {
      const uint32_t pos_keep = pos;
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "flushing! b=%p free=%d, seg(%d)",
              b,
              b->header.buffer_free,
              grn_ii_pos_lseg(ii, pos));
      buffer_close(ctx, ii, pseg);
      buffer_flush(ctx, ii, grn_ii_pos_lseg(ii, pos), h);
      if (ctx->rc != GRN_SUCCESS) {
        grn_obj term;
        char errbuf[GRN_CTX_MSGSIZE];
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, ii, tid, &term);
        grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
        ERR(ctx->rc,
            "%s failed to flush a buffer: "
            "<%.*s>: "
            "<%.*s>(%u): "
            "(%u:%u): "
            "position:<%u>: "
            "%s",
            tag,
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            tid,
            u->rid,
            u->sid,
            pos,
            errbuf);
        GRN_OBJ_FIN(ctx, &term);
        goto exit;
      }
      if (a[0] != pos_keep) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "grn_ii_delete_one: a[0] changed %d->%d)",
                a[0],
                pos_keep);
        continue;
      }
      if ((pseg = buffer_open(ctx, ii, a[0], &bt, &b)) ==
          GRN_II_PSEG_NOT_ASSIGNED) {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, ii, tid, &term);
        MERR("%s failed to reallocate a buffer: "
             "<%.*s>: "
             "<%.*s>(%u): "
             "(%u:%u): "
             "position:<%u>",
             tag,
             name_size,
             name,
             (int)GRN_TEXT_LEN(&term),
             GRN_TEXT_VALUE(&term),
             tid,
             u->rid,
             u->sid,
             a[0]);
        GRN_OBJ_FIN(ctx, &term);
        goto exit;
      }
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "flushed!  b=%p free=%d, seg(%d)",
              b,
              b->header.buffer_free,
              grn_ii_pos_lseg(ii, a[0]));
      if (b->header.buffer_free < size) {
        grn_obj term;
        GRN_DEFINE_NAME(ii);
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, ii, tid, &term);
        MERR("%s buffer is full: "
             "<%.*s>: "
             "<%.*s>(%u): "
             "(%u:%u): "
             "segment:<%u>, free:<%u>, required:<%u>",
             tag,
             name_size,
             name,
             (int)GRN_TEXT_LEN(&term),
             GRN_TEXT_VALUE(&term),
             tid,
             u->rid,
             u->sid,
             a[0],
             b->header.buffer_free,
             size);
        GRN_OBJ_FIN(ctx, &term);
        buffer_close(ctx, ii, pseg);
        goto exit;
      }
    }

    b->header.buffer_free -= size;
    br = (buffer_rec *)(((byte *)&b->terms[b->header.nterms]) +
                        b->header.buffer_free);
    buffer_put(ctx, ii, b, bt, br, bs, u, size);
    buffer_close(ctx, ii, pseg);
    break;
  }
exit:
  array_unref(ctx, ii, tid);
  if (bs) {
    GRN_FREE(bs);
  }
  return ctx->rc;
}

extern "C" grn_rc
grn_ii_delete_one(
  grn_ctx *ctx, grn_ii *ii, grn_id tid, grn_ii_updspec *u, grn_hash *h)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_rc rc = grn_ii_delete_one_internal(ctx, ii, tid, u, h);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(ii);
    grn_obj *source_table = grn_ctx_at(ctx, DB_OBJ(ii)->range);
    GRN_DEFINE_NAME_CUSTOM(source_table, source_table_name);
    grn_obj_unref(ctx, source_table);
    grn_obj term;
    GRN_TEXT_INIT(&term, 0);
    grn_ii_get_term(ctx, ii, tid, &term);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[ii][delete][one][slow][%f] "
            "<%.*s>: "
            "<%.*s>(%u): "
            "record:<%.*s>(%u:%u)",
            elapsed_time,
            name_size,
            name,
            (int)GRN_TEXT_LEN(&term),
            GRN_TEXT_VALUE(&term),
            tid,
            source_table_name_size,
            source_table_name,
            u->rid,
            u->sid);
    GRN_OBJ_FIN(ctx, &term);
  }
  GRN_SLOW_LOG_POP_END(ctx);
  return rc;
}

#define CHUNK_USED    1
#define BUFFER_USED   2
#define SOLE_DOC_USED 4
#define SOLE_POS_USED 8

struct _grn_ii_cursor {
  grn_db_obj obj;
  grn_ctx *ctx;
  grn_ii *ii;
  grn_id id;
  grn_posting *post;

  grn_id min; /* Minimum record ID */
  grn_id max;
  grn_posting_internal pc;
  grn_posting_internal pb;

  uint32_t cdf; /* Document frequency */
  uint32_t *cdp;
  uint32_t *crp; /* Record ID */
  uint32_t *csp; /* Section ID */
  uint32_t *ctp; /* Term frequency */
  uint32_t *cwp; /* Weight */
  uint32_t *cpp; /* Position */

  uint8_t *bp;

  uint32_t nelements;
  uint32_t nchunks;
  uint32_t curr_chunk;
  chunk_info *cinfo;
  grn_io_win iw;
  uint8_t *cp;
  uint8_t *cpe;
  datavec rdv[MAX_N_ELEMENTS + 1];

  buffer *buf;
  uint16_t stat;
  uint16_t nextb;
  uint32_t buffer_pseg;
  int flags;
  uint32_t *ppseg;

  int weight;

  uint32_t prev_chunk_rid;

  float scale;
  float *scales;
  size_t n_scales;
};

static bool
buffer_is_reused(grn_ctx *ctx, grn_ii *ii, grn_ii_cursor *c)
{
  if (*c->ppseg != c->buffer_pseg) {
    uint32_t i;
    for (i = ii->header.common->bgqtail; i != ii->header.common->bgqhead;
         i = (i + 1) & (GRN_II_BGQSIZE - 1)) {
      if (ii->header.common->bgqbody[i] == c->buffer_pseg) {
        return false;
      }
    }
    return true;
  }
  return false;
}

static int
chunk_is_reused(
  grn_ctx *ctx, grn_ii *ii, grn_ii_cursor *c, uint32_t offset, uint32_t size)
{
  if (*c->ppseg != c->buffer_pseg) {
    uint32_t i, gseg;
    if (size > S_CHUNK) {
      return 1;
    }
    grn_bit_scan_rev_result m;
    if (size > (1 << GRN_II_W_LEAST_CHUNK)) {
      int es = size - 1;
      GRN_BIT_SCAN_REV(es, m);
      m++;
    } else {
      m = GRN_II_W_LEAST_CHUNK;
    }
    gseg = ii->header.common->garbages[m - GRN_II_W_LEAST_CHUNK];
    while (gseg != GRN_II_PSEG_NOT_ASSIGNED) {
      grn_io_win iw;
      auto ginfo = static_cast<grn_ii_ginfo *>(
        WIN_MAP(ctx, ii->chunk, &iw, gseg, 0, S_GARBAGE, GRN_IO_RDWR));
      if (!ginfo) {
        break;
      }
      for (i = 0; i < ginfo->nrecs; i++) {
        if (ginfo->recs[i] == offset) {
          grn_io_win_unmap(ctx, &iw);
          return 0;
        }
      }
      gseg = ginfo->next;
      grn_io_win_unmap(ctx, &iw);
    }
    return 1;
  }
  return 0;
}

#define GRN_II_CURSOR_CMP(c1, c2)                                              \
  (((c1)->post->rid > (c2)->post->rid) ||                                      \
   (((c1)->post->rid == (c2)->post->rid) &&                                    \
    (((c1)->post->sid > (c2)->post->sid) ||                                    \
     (((c1)->post->sid == (c2)->post->sid) &&                                  \
      ((c1)->post->pos > (c2)->post->pos)))))

extern "C" grn_ii_cursor *
grn_ii_cursor_open(grn_ctx *ctx,
                   grn_ii *ii,
                   grn_id tid,
                   grn_id min,
                   grn_id max,
                   int nelements,
                   int flags)
{
  grn_ii_cursor *c = NULL;
  uint32_t pos, *a;
  if (!(a = array_at(ctx, ii, tid))) {
    return NULL;
  }
  for (;;) {
    c = NULL;
    if (!(pos = a[0])) {
      goto exit;
    }
    c = static_cast<grn_ii_cursor *>(GRN_CALLOC(sizeof(grn_ii_cursor)));
    if (!c) {
      goto exit;
    }
    c->ctx = ctx;
    c->ii = ii;
    c->id = tid;
    c->min = min;
    c->max = max;
    c->nelements = nelements;
    c->flags = flags;
    c->weight = 0;
    c->scale = 1.0;
    c->scales = NULL;
    c->n_scales = 0;
    datavec_init(ctx, ii, c->rdv, 0, 0);
    if (POS_IS_EMBED(pos)) {
      c->stat = 0;
      if ((ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
        c->pb.rid = POS_RID_SID_EXTRACT_RID(pos);
        c->pb.sid = POS_RID_SID_EXTRACT_SID(pos);
      } else {
        c->pb.rid = POS_RID_EXTRACT(pos);
        c->pb.sid = 1;
      }
      c->pb.tf = 1;
      c->pb.weight = 0;
      c->pb.weight_float = (float)(c->pb.weight);
      c->pb.pos = a[1];
      c->pb.scale = c->scale;
    } else {
      uint32_t chunk;
      buffer_term *bt;
      c->buffer_pseg = buffer_open(ctx, ii, pos, &bt, &c->buf);
      if (c->buffer_pseg == GRN_II_PSEG_NOT_ASSIGNED) {
        GRN_FREE(c);
        c = NULL;
        goto exit;
      }
      const uint32_t lseg = grn_ii_pos_lseg(ii, pos);
      c->ppseg = grn_ii_get_buffer_pseg_address_inline(ii, lseg);
      if (bt->size_in_chunk &&
          (chunk = c->buf->header.chunk) != GRN_II_PSEG_NOT_ASSIGNED) {
        if (!(c->cp = static_cast<uint8_t *>(WIN_MAP(ctx,
                                                     ii->chunk,
                                                     &c->iw,
                                                     chunk,
                                                     bt->pos_in_chunk,
                                                     bt->size_in_chunk,
                                                     GRN_IO_RDONLY)))) {
          buffer_close(ctx, ii, c->buffer_pseg);
          GRN_FREE(c);
          c = NULL;
          goto exit;
        }
        if (buffer_is_reused(ctx, ii, c)) {
          grn_ii_cursor_close(ctx, c);
          continue;
        }
        c->cpe = c->cp + bt->size_in_chunk;
        if ((bt->tid & CHUNK_SPLIT)) {
          uint32_t i;
          grn_id crid;
          GRN_B_DEC(c->nchunks, c->cp);
          if (chunk_is_reused(ctx, ii, c, chunk, c->buf->header.chunk_size)) {
            grn_ii_cursor_close(ctx, c);
            continue;
          }
          if (!(c->cinfo = GRN_MALLOCN(chunk_info, c->nchunks))) {
            buffer_close(ctx, ii, c->buffer_pseg);
            grn_io_win_unmap(ctx, &c->iw);
            GRN_FREE(c);
            c = NULL;
            goto exit;
          }
          for (i = 0, crid = GRN_ID_NIL; i < c->nchunks; i++) {
            GRN_B_DEC(c->cinfo[i].segno, c->cp);
            GRN_B_DEC(c->cinfo[i].size, c->cp);
            GRN_B_DEC(c->cinfo[i].dgap, c->cp);
            crid += c->cinfo[i].dgap;
            if (crid < min) {
              c->pc.rid = crid;
              c->curr_chunk = i + 1;
            }
          }
          if (chunk_is_reused(ctx, ii, c, chunk, c->buf->header.chunk_size)) {
            grn_ii_cursor_close(ctx, c);
            continue;
          }
        }
      }
      c->nextb = bt->pos_in_buffer;
      c->stat = CHUNK_USED | BUFFER_USED;
    }
    if (pos == a[0]) {
      break;
    }
    grn_ii_cursor_close(ctx, c);
  }
exit:
  array_unref(ctx, ii, tid);
  return c;
}

extern "C" grn_ii *
grn_ii_cursor_get_ii(grn_ctx *ctx, grn_ii_cursor *cursor)
{
  return cursor->ii;
}

static inline float
grn_ii_cursor_resolve_scale(grn_ctx *ctx,
                            grn_ii_cursor *cursor,
                            uint32_t section_id)
{
  if (cursor->n_scales > 0) {
    if (section_id <= cursor->n_scales) {
      return cursor->scales[section_id - 1];
    } else {
      return 0.0;
    }
  } else {
    return cursor->scale;
  }
}

extern "C" grn_rc
grn_ii_cursor_set_scale(grn_ctx *ctx, grn_ii_cursor *cursor, float scale)
{
  cursor->scale = scale;
  cursor->scales = NULL;
  cursor->n_scales = 0;
  if (!cursor->buf) {
    cursor->pb.scale = grn_ii_cursor_resolve_scale(ctx, cursor, cursor->pb.sid);
  }
  return ctx->rc;
}

extern "C" grn_rc
grn_ii_cursor_set_scales(grn_ctx *ctx,
                         grn_ii_cursor *cursor,
                         float *scales,
                         size_t n_scales)
{
  cursor->scale = 0.0;
  cursor->scales = scales;
  cursor->n_scales = n_scales;
  if (!cursor->buf) {
    cursor->pb.scale = grn_ii_cursor_resolve_scale(ctx, cursor, cursor->pb.sid);
  }
  return ctx->rc;
}

static inline void
grn_ii_cursor_set_min(grn_ctx *ctx, grn_ii_cursor *c, grn_id min)
{
  if (c->min >= min) {
    return;
  }

  if (grn_ii_cursor_set_min_enable) {
    grn_id old_min = c->min;
    c->min = min;
    if (c->buf && c->pc.rid != GRN_ID_NIL && c->pc.rid < c->min &&
        c->prev_chunk_rid < c->min && c->curr_chunk < c->nchunks) {
      uint32_t i;
      uint32_t skip_chunk = 0;
      grn_id rid = c->prev_chunk_rid;

      if (c->curr_chunk > 0) {
        i = c->curr_chunk - 1;
      } else {
        i = 0;
      }
      for (; i < c->nchunks; i++) {
        rid += c->cinfo[i].dgap;
        if (rid < c->min) {
          skip_chunk = i + 1;
        } else {
          rid -= c->cinfo[i].dgap;
          break;
        }
      }
      if (skip_chunk > c->curr_chunk) {
        grn_id old_rid = c->pc.rid;
        grn_id old_prev_chunk_rid = c->prev_chunk_rid;
        uint32_t old_chunk = c->curr_chunk;
        bool old_chunk_used = (c->stat & CHUNK_USED);
        c->pc.rid = rid;
        c->pc.rest = 0;
        c->prev_chunk_rid = rid - c->cinfo[skip_chunk - 1].dgap;
        c->curr_chunk = skip_chunk;
        c->crp = c->cdp + c->cdf;
        c->stat |= CHUNK_USED;
        if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
          GRN_DEFINE_NAME(c->ii);
          grn_obj term;
          GRN_TEXT_INIT(&term, 0);
          grn_ii_get_term(ctx, c->ii, c->id, &term);
          GRN_LOG(ctx,
                  GRN_LOG_DEBUG,
                  "[ii][cursor][min][%.*s][%.*s][%u] skip: "
                  "<%p>: "
                  "rid(%u->%u): "
                  "prev-chunk-rid(%u->%u): "
                  "min(%u->%u): "
                  "chunk(%u->%u): "
                  "chunk-used(%s->%s)",
                  name_size,
                  name,
                  (int)GRN_TEXT_LEN(&term),
                  GRN_TEXT_VALUE(&term),
                  c->id,
                  c,
                  old_rid,
                  rid,
                  old_prev_chunk_rid,
                  c->prev_chunk_rid,
                  old_min,
                  min,
                  old_chunk,
                  c->curr_chunk,
                  old_chunk_used ? "true" : "false",
                  (c->stat & CHUNK_USED) ? "true" : "false");
          GRN_OBJ_FIN(ctx, &term);
        }
      }
    }
  }
}

typedef struct {
  bool include_garbage;
} grn_ii_cursor_next_options;

static inline grn_posting *
grn_ii_cursor_next_internal(grn_ctx *ctx,
                            grn_ii_cursor *c,
                            grn_ii_cursor_next_options *options)
{
  const bool include_garbage = options->include_garbage;
  if (c->buf) {
    for (;;) {
      if (c->stat & CHUNK_USED) {
        for (;;) {
          if (c->crp < c->cdp + c->cdf) {
            uint32_t dgap = *c->crp++;
            c->pc.rid += dgap;
            if (dgap) {
              c->pc.sid = 0;
            }
            if ((c->ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
              c->pc.sid += 1 + *c->csp++;
            } else {
              c->pc.sid = 1;
            }
            c->cpp += c->pc.rest;
            c->pc.rest = c->pc.tf = 1 + *c->ctp++;
            if ((c->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
              c->pc.weight = *c->cwp++;
            } else {
              c->pc.weight = 0;
            }
            c->pc.weight_float = (float)(c->pc.weight);
            c->pc.pos = 0;
            c->pc.scale = grn_ii_cursor_resolve_scale(ctx, c, c->pc.sid);
            /*
            {
              static int count = 0;
              int tf = c->pc.tf, pos = 0, *pp = (int *)c->cpp;
              grn_obj buf;
              GRN_TEXT_INIT(&buf, 0);
              grn_text_itoa(ctx, &buf, c->pc.rid);
              GRN_TEXT_PUTC(ctx, &buf, ':');
              grn_text_itoa(ctx, &buf, c->pc.sid);
              GRN_TEXT_PUTC(ctx, &buf, ':');
              grn_text_itoa(ctx, &buf, c->pc.tf);
              GRN_TEXT_PUTC(ctx, &buf, '(');
              while (tf--) {
                pos += *pp++;
                count++;
                grn_text_itoa(ctx, &buf, pos);
                if (tf) { GRN_TEXT_PUTC(ctx, &buf, ':'); }
              }
              GRN_TEXT_PUTC(ctx, &buf, ')');
              GRN_TEXT_PUTC(ctx, &buf, '\0');
              GRN_LOG(ctx, GRN_LOG_DEBUG, "posting(%d):%s", count,
            GRN_TEXT_VALUE(&buf)); GRN_OBJ_FIN(ctx, &buf);
            }
            */
            if (c->pc.rid < c->min || c->pc.scale <= 0.0) {
              continue;
            }
          } else {
            if (c->curr_chunk <= c->nchunks) {
              if (c->curr_chunk == c->nchunks) {
                if (c->cp < c->cpe) {
                  int decoded_size;
                  decoded_size =
                    grn_decv(ctx, c->ii, c->id, c->cp, c->cpe - c->cp, c->rdv);
                  if (decoded_size == 0) {
                    GRN_DEFINE_NAME(c->ii);
                    grn_obj term;
                    GRN_TEXT_INIT(&term, 0);
                    grn_ii_get_term(ctx, c->ii, c->id, &term);
                    GRN_LOG(ctx,
                            GRN_LOG_WARNING,
                            "[ii][cursor][next][chunk][last][%.*s][%.*s][%u] "
                            "failed to decode the last chunk. "
                            "Another thread might change "
                            "the chunk while decoding: "
                            "<%p>: <%d>: <%" GRN_FMT_INT64D ">",
                            name_size,
                            name,
                            (int)GRN_TEXT_LEN(&term),
                            GRN_TEXT_VALUE(&term),
                            c->id,
                            c,
                            c->curr_chunk,
                            c->cinfo ? (int64_t)(c->cinfo[c->curr_chunk].segno)
                                     : -1);
                    GRN_OBJ_FIN(ctx, &term);
                    c->pc.rid = GRN_ID_NIL;
                    break;
                  }
                  if (buffer_is_reused(ctx, c->ii, c)) {
                    GRN_DEFINE_NAME(c->ii);
                    grn_obj term;
                    GRN_TEXT_INIT(&term, 0);
                    grn_ii_get_term(ctx, c->ii, c->id, &term);
                    GRN_LOG(ctx,
                            GRN_LOG_WARNING,
                            "[ii][cursor][next][chunk][last][%.*s][%.*s][%u] "
                            "buffer is reused by another thread: <%p>",
                            name_size,
                            name,
                            (int)GRN_TEXT_LEN(&term),
                            GRN_TEXT_VALUE(&term),
                            c->id,
                            c);
                    GRN_OBJ_FIN(ctx, &term);
                    c->pc.rid = GRN_ID_NIL;
                    break;
                  }
                  if (chunk_is_reused(ctx,
                                      c->ii,
                                      c,
                                      c->buf->header.chunk,
                                      c->buf->header.chunk_size)) {
                    GRN_DEFINE_NAME(c->ii);
                    grn_obj term;
                    GRN_TEXT_INIT(&term, 0);
                    grn_ii_get_term(ctx, c->ii, c->id, &term);
                    GRN_LOG(ctx,
                            GRN_LOG_WARNING,
                            "[ii][cursor][next][chunk][last][%.*s][%.*s][%u] "
                            "chunk is reused by another thread: "
                            "<%p>: <%d>",
                            name_size,
                            name,
                            (int)GRN_TEXT_LEN(&term),
                            GRN_TEXT_VALUE(&term),
                            c->id,
                            c,
                            c->buf->header.chunk);
                    GRN_OBJ_FIN(ctx, &term);
                    c->pc.rid = GRN_ID_NIL;
                    break;
                  }
                } else {
                  c->pc.rid = GRN_ID_NIL;
                  break;
                }
              } else {
                uint8_t *cp;
                grn_io_win iw;
                uint32_t size = c->cinfo[c->curr_chunk].size;
                if (size && (cp = static_cast<uint8_t *>(
                               WIN_MAP(ctx,
                                       c->ii->chunk,
                                       &iw,
                                       c->cinfo[c->curr_chunk].segno,
                                       0,
                                       size,
                                       GRN_IO_RDONLY)))) {
                  int decoded_size;
                  decoded_size = grn_decv(ctx, c->ii, c->id, cp, size, c->rdv);
                  grn_io_win_unmap(ctx, &iw);
                  if (decoded_size == 0) {
                    GRN_DEFINE_NAME(c->ii);
                    grn_obj term;
                    GRN_TEXT_INIT(&term, 0);
                    grn_ii_get_term(ctx, c->ii, c->id, &term);
                    GRN_LOG(ctx,
                            GRN_LOG_WARNING,
                            "[ii][cursor][next][chunk][%.*s][%.*s][%u]  "
                            "failed to decode the next chunk. "
                            "Another thread might change "
                            "the chunk while decoding: "
                            "<%p>: <%d>: <%" GRN_FMT_INT64D ">",
                            name_size,
                            name,
                            (int)GRN_TEXT_LEN(&term),
                            GRN_TEXT_VALUE(&term),
                            c->id,
                            c,
                            c->curr_chunk,
                            c->cinfo ? (int64_t)(c->cinfo[c->curr_chunk].segno)
                                     : -1);
                    GRN_OBJ_FIN(ctx, &term);
                    c->pc.rid = GRN_ID_NIL;
                    break;
                  }
                  if (buffer_is_reused(ctx, c->ii, c)) {
                    GRN_DEFINE_NAME(c->ii);
                    grn_obj term;
                    GRN_TEXT_INIT(&term, 0);
                    grn_ii_get_term(ctx, c->ii, c->id, &term);
                    GRN_LOG(ctx,
                            GRN_LOG_WARNING,
                            "[ii][cursor][next][chunk][%.*s][%.*s][%u] "
                            "buffer is reused by another thread: <%p>",
                            name_size,
                            name,
                            (int)GRN_TEXT_LEN(&term),
                            GRN_TEXT_VALUE(&term),
                            c->id,
                            c);
                    GRN_OBJ_FIN(ctx, &term);
                    c->pc.rid = GRN_ID_NIL;
                    break;
                  }
                  if (chunk_is_reused(ctx,
                                      c->ii,
                                      c,
                                      c->cinfo[c->curr_chunk].segno,
                                      size)) {
                    GRN_DEFINE_NAME(c->ii);
                    grn_obj term;
                    GRN_TEXT_INIT(&term, 0);
                    grn_ii_get_term(ctx, c->ii, c->id, &term);
                    GRN_LOG(ctx,
                            GRN_LOG_WARNING,
                            "[ii][cursor][next][chunk][%.*s][%.*s][%u] "
                            "chunk is reused by another thread: "
                            "<%p>: <%d>",
                            name_size,
                            name,
                            (int)GRN_TEXT_LEN(&term),
                            GRN_TEXT_VALUE(&term),
                            c->id,
                            c,
                            c->cinfo[c->curr_chunk].segno);
                    GRN_OBJ_FIN(ctx, &term);
                    c->pc.rid = GRN_ID_NIL;
                    break;
                  }
                } else {
                  c->pc.rid = GRN_ID_NIL;
                  break;
                }
              }
              {
                int j = 0;
                c->cdf = c->rdv[j].data_size;
                c->crp = c->cdp = c->rdv[j++].data;
                if ((c->ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
                  c->csp = c->rdv[j++].data;
                }
                c->ctp = c->rdv[j++].data;
                if ((c->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
                  c->cwp = c->rdv[j++].data;
                }
                if ((c->ii->header.common->flags & GRN_OBJ_WITH_POSITION)) {
                  c->cpp = c->rdv[j].data;
                }
              }
              c->prev_chunk_rid = c->pc.rid;
              c->pc.rid = GRN_ID_NIL;
              c->pc.sid = 0;
              c->pc.rest = 0;
              c->curr_chunk++;
              continue;
            } else {
              c->pc.rid = GRN_ID_NIL;
            }
          }
          break;
        }
      }
      if (c->stat & BUFFER_USED) {
        for (;;) {
          if (c->nextb) {
            uint32_t lrid = c->pb.rid, lsid = c->pb.sid; /* for check */
            buffer_rec *br = BUFFER_REC_AT(c->buf, c->nextb);
            if (buffer_is_reused(ctx, c->ii, c)) {
              GRN_DEFINE_NAME(c->ii);
              grn_obj term;
              GRN_TEXT_INIT(&term, 0);
              grn_ii_get_term(ctx, c->ii, c->id, &term);
              GRN_LOG(ctx,
                      GRN_LOG_WARNING,
                      "[ii][cursor][next][buffer][%.*s][%.*s][%u] "
                      "buffer is reused by another thread: "
                      "<%p>: <%d>: <%d>",
                      name_size,
                      name,
                      (int)GRN_TEXT_LEN(&term),
                      GRN_TEXT_VALUE(&term),
                      c->id,
                      c,
                      c->buffer_pseg,
                      *c->ppseg);
              GRN_OBJ_FIN(ctx, &term);
              c->pb.rid = GRN_ID_NIL;
              break;
            }
            c->bp = GRN_NEXT_ADDR(br);
            GRN_B_DEC(c->pb.rid, c->bp);
            if ((c->ii->header.common->flags & GRN_OBJ_WITH_SECTION)) {
              GRN_B_DEC(c->pb.sid, c->bp);
            } else {
              c->pb.sid = 1;
            }
            if (lrid > c->pb.rid || (lrid == c->pb.rid && lsid >= c->pb.sid)) {
              GRN_DEFINE_NAME(c->ii);
              grn_obj term;
              GRN_TEXT_INIT(&term, 0);
              grn_ii_get_term(ctx, c->ii, c->id, &term);
              ERR(GRN_FILE_CORRUPT,
                  "[ii][broken][cursor][next][buffer][%.*s][%.*s][%u] "
                  "posting in list in buffer isn't sorted: "
                  "(%d:%d) -> (%d:%d) (%d->%d)",
                  name_size,
                  name,
                  (int)GRN_TEXT_LEN(&term),
                  GRN_TEXT_VALUE(&term),
                  c->id,
                  lrid,
                  lsid,
                  c->pb.rid,
                  c->pb.sid,
                  c->buffer_pseg,
                  *c->ppseg);
              GRN_OBJ_FIN(ctx, &term);
              c->pb.rid = GRN_ID_NIL;
              break;
            }
            c->pb.scale = grn_ii_cursor_resolve_scale(ctx, c, c->pb.sid);
            if (c->pb.rid < c->min || c->pb.scale <= 0.0) {
              c->pb.rid = 0;
              if (br->jump > 0 && !BUFFER_REC_DELETED(br)) {
                buffer_rec *jump_br = BUFFER_REC_AT(c->buf, br->jump);
                if (BUFFER_REC_DELETED(jump_br)) {
                  c->nextb = br->step;
                } else {
                  uint8_t *jump_bp;
                  uint32_t jump_rid;
                  jump_bp = GRN_NEXT_ADDR(jump_br);
                  GRN_B_DEC(jump_rid, jump_bp);
                  if (jump_rid < c->min) {
                    c->nextb = br->jump;
                  } else {
                    c->nextb = br->step;
                  }
                }
              } else {
                c->nextb = br->step;
              }
              continue;
            }
            c->nextb = br->step;
            GRN_B_DEC(c->pb.tf, c->bp);
            if ((c->ii->header.common->flags & GRN_OBJ_WITH_WEIGHT)) {
              GRN_B_DEC(c->pb.weight, c->bp);
            } else {
              c->pb.weight = 0;
            }
            c->pb.weight_float = (float)(c->pb.weight);
            c->pb.rest = c->pb.tf;
            c->pb.pos = 0;
          } else {
            c->pb.rid = 0;
          }
          break;
        }
      }
      if (c->pb.rid) {
        if (c->pc.rid) {
          if (c->pc.rid < c->pb.rid) {
            c->stat = CHUNK_USED;
            if (include_garbage || (c->pc.tf && c->pc.sid)) {
              c->post = (grn_posting *)(&c->pc);
              break;
            }
          } else {
            if (c->pb.rid < c->pc.rid) {
              c->stat = BUFFER_USED;
              if (include_garbage || (c->pb.tf && c->pb.sid)) {
                c->post = (grn_posting *)(&c->pb);
                break;
              }
            } else {
              if (c->pb.sid) {
                if (c->pc.sid < c->pb.sid) {
                  c->stat = CHUNK_USED;
                  if (include_garbage || (c->pc.tf && c->pc.sid)) {
                    c->post = (grn_posting *)(&c->pc);
                    break;
                  }
                } else {
                  c->stat = BUFFER_USED;
                  if (c->pb.sid == c->pc.sid) {
                    c->stat |= CHUNK_USED;
                  }
                  if (include_garbage || (c->pb.tf)) {
                    c->post = (grn_posting *)(&c->pb);
                    break;
                  }
                }
              } else {
                c->stat = CHUNK_USED;
              }
            }
          }
        } else {
          c->stat = BUFFER_USED;
          if (include_garbage || (c->pb.tf && c->pb.sid)) {
            c->post = (grn_posting *)(&c->pb);
            break;
          }
        }
      } else {
        if (c->pc.rid) {
          c->stat = CHUNK_USED;
          if (include_garbage || (c->pc.tf && c->pc.sid)) {
            c->post = (grn_posting *)(&c->pc);
            break;
          }
        } else {
          c->post = NULL;
          return NULL;
        }
      }
    }
  } else {
    if (c->stat & SOLE_DOC_USED) {
      c->post = NULL;
      return NULL;
    } else {
      c->post = (grn_posting *)(&c->pb);
      c->stat |= SOLE_DOC_USED;
      if (c->post->rid < c->min) {
        c->post = NULL;
        return NULL;
      }
      if (c->pb.scale <= 0.0) {
        c->post = NULL;
        return NULL;
      }
    }
  }
  return c->post;
}

extern "C" grn_posting *
grn_ii_cursor_next(grn_ctx *ctx, grn_ii_cursor *c)
{
  grn_ii_cursor_next_options options = {/* .include_garbage = */ false};
  return grn_ii_cursor_next_internal(ctx, c, &options);
}

extern "C" grn_posting *
grn_ii_cursor_next_pos(grn_ctx *ctx, grn_ii_cursor *c)
{
  uint32_t gap;
  if ((c->ii->header.common->flags & GRN_OBJ_WITH_POSITION)) {
    if (c->nelements == c->ii->n_elements) {
      if (c->buf) {
        if (c->post == (grn_posting *)(&c->pc)) {
          if (c->pc.rest) {
            c->pc.rest--;
            c->pc.pos += *c->cpp++;
          } else {
            return NULL;
          }
        } else if (c->post == (grn_posting *)(&c->pb)) {
          if (buffer_is_reused(ctx, c->ii, c)) {
            GRN_DEFINE_NAME(c->ii);
            grn_obj term;
            GRN_TEXT_INIT(&term, 0);
            grn_ii_get_term(ctx, c->ii, c->id, &term);
            GRN_LOG(ctx,
                    GRN_LOG_WARNING,
                    "[ii][cursor][next][pos][buffer][%.*s][%.*s][%u] "
                    "buffer(%d,%d) is reused by another thread: %p",
                    name_size,
                    name,
                    (int)GRN_TEXT_LEN(&term),
                    GRN_TEXT_VALUE(&term),
                    c->id,
                    c->buffer_pseg,
                    *c->ppseg,
                    c);
            GRN_OBJ_FIN(ctx, &term);
            return NULL;
          }
          if (c->pb.rest) {
            c->pb.rest--;
            GRN_B_DEC(gap, c->bp);
            c->pb.pos += gap;
          } else {
            return NULL;
          }
        } else {
          return NULL;
        }
      } else {
        if (c->stat & SOLE_POS_USED) {
          return NULL;
        } else {
          c->stat |= SOLE_POS_USED;
        }
      }
    }
  } else {
    if (c->stat & SOLE_POS_USED) {
      return NULL;
    } else {
      c->stat |= SOLE_POS_USED;
    }
  }
  return c->post;
}

extern "C" grn_rc
grn_ii_cursor_close(grn_ctx *ctx, grn_ii_cursor *c)
{
  if (!c) {
    return GRN_INVALID_ARGUMENT;
  }
  datavec_fin(ctx, c->rdv);
  if (c->cinfo) {
    GRN_FREE(c->cinfo);
  }
  if (c->buf) {
    buffer_close(ctx, c->ii, c->buffer_pseg);
  }
  if (c->cp) {
    grn_io_win_unmap(ctx, &c->iw);
  }
  GRN_FREE(c);
  return GRN_SUCCESS;
}

extern "C" uint32_t
grn_ii_get_chunksize(grn_ctx *ctx, grn_ii *ii, grn_id tid)
{
  uint32_t res, pos, *a;
  a = array_at(ctx, ii, tid);
  if (!a) {
    return 0;
  }
  if ((pos = a[0])) {
    if (POS_IS_EMBED(pos)) {
      res = 0;
    } else {
      buffer *buf;
      uint32_t pseg;
      buffer_term *bt;
      if ((pseg = buffer_open(ctx, ii, pos, &bt, &buf)) ==
          GRN_II_PSEG_NOT_ASSIGNED) {
        res = 0;
      } else {
        res = bt->size_in_chunk;
        buffer_close(ctx, ii, pseg);
      }
    }
  } else {
    res = 0;
  }
  array_unref(ctx, ii, tid);
  return res;
}

/* This doesn't return the number of records that have the token. But
 * this returns a similar number based of size in buffer. Buffer may
 * have deleted tokens. One record may use multiple buffers. (To be
 * confirmed.) So the estimated size may not match the number of
 * records. */
extern "C" uint32_t
grn_ii_estimate_size(grn_ctx *ctx, grn_ii *ii, grn_id tid)
{
  uint32_t res, pos, *a;
  a = array_at(ctx, ii, tid);
  if (!a) {
    return 0;
  }
  if ((pos = a[0])) {
    if (POS_IS_EMBED(pos)) {
      res = 1;
    } else {
      buffer *buf;
      uint32_t pseg;
      buffer_term *bt;
      if ((pseg = buffer_open(ctx, ii, pos, &bt, &buf)) ==
          GRN_II_PSEG_NOT_ASSIGNED) {
        res = 0;
      } else {
        res = a[1] + bt->size_in_buffer + 2;
        buffer_close(ctx, ii, pseg);
      }
    }
  } else {
    res = 0;
  }
  array_unref(ctx, ii, tid);
  return res;
}

extern "C" int
grn_ii_entry_info(grn_ctx *ctx,
                  grn_ii *ii,
                  grn_id tid,
                  unsigned int *a,
                  unsigned int *chunk,
                  unsigned int *chunk_size,
                  unsigned int *buffer_free,
                  unsigned int *nterms,
                  unsigned int *nterms_void,
                  unsigned int *bt_tid,
                  unsigned int *size_in_chunk,
                  unsigned int *pos_in_chunk,
                  unsigned int *size_in_buffer,
                  unsigned int *pos_in_buffer)
{
  buffer *b;
  buffer_term *bt;
  uint32_t pseg, *ap;
  ERRCLR(NULL);
  ap = array_at(ctx, ii, tid);
  if (!ap) {
    return 0;
  }
  a[0] = *ap;
  array_unref(ctx, ii, tid);
  if (!a[0]) {
    return 1;
  }
  if (POS_IS_EMBED(a[0])) {
    return 2;
  }
  if ((pseg = buffer_open(ctx, ii, a[0], &bt, &b)) ==
      GRN_II_PSEG_NOT_ASSIGNED) {
    return 3;
  }
  *chunk = b->header.chunk;
  *chunk_size = b->header.chunk_size;
  *buffer_free = b->header.buffer_free;
  *nterms = b->header.nterms;
  *bt_tid = bt->tid;
  *size_in_chunk = bt->size_in_chunk;
  *pos_in_chunk = bt->pos_in_chunk;
  *size_in_buffer = bt->size_in_buffer;
  *pos_in_buffer = bt->pos_in_buffer;
  buffer_close(ctx, ii, pseg);
  return 4;
}

extern "C" uint32_t
grn_ii_get_array_pseg(grn_ii *ii, uint32_t lseg)
{
  return grn_ii_get_array_pseg_inline(ii, lseg);
}

extern "C" uint32_t
grn_ii_get_buffer_pseg(grn_ii *ii, uint32_t lseg)
{
  return grn_ii_get_buffer_pseg_inline(ii, lseg);
}

extern "C" uint32_t
grn_ii_n_logical_segments(grn_ii *ii)
{
  return grn_ii_n_logical_segments_inline(ii);
}

extern "C" const char *
grn_ii_path(grn_ii *ii)
{
  return grn_io_path(ii->seg);
}

extern "C" uint32_t
grn_ii_max_section(grn_ii *ii)
{
  return ii->header.common->smax;
}

/* private classes */

/* b-heap */

typedef struct {
  int n_entries;
  int n_bins;
  grn_ii_cursor **bins;
} cursor_heap;

extern "C" void
grn_inspect_cursor_heap(grn_ctx *ctx, grn_obj *buffer, cursor_heap *heap)
{
  grn_text_printf(ctx,
                  buffer,
                  "#<cursor_heap: n_entries=%d n_bins=%d\n",
                  heap->n_entries,
                  heap->n_bins);
  grn_text_printf(ctx, buffer, "  bins:\n");
  int i;
  for (i = 0; i < heap->n_bins; i++) {
    grn_text_printf(ctx, buffer, "    [%d] ", i);
    grn_ii_get_term(ctx, heap->bins[i]->ii, heap->bins[i]->id, buffer);
    grn_text_printf(ctx, buffer, "\n");
  }
  grn_text_printf(ctx, buffer, ">");
}

static inline cursor_heap *
cursor_heap_open(grn_ctx *ctx, int max)
{
  auto h = static_cast<cursor_heap *>(GRN_CALLOC(sizeof(cursor_heap)));
  if (!h) {
    return NULL;
  }
  h->bins =
    static_cast<grn_ii_cursor **>(GRN_MALLOC(sizeof(grn_ii_cursor *) * max));
  if (!h->bins) {
    GRN_FREE(h);
    return NULL;
  }
  h->n_entries = 0;
  h->n_bins = max;
  return h;
}

static inline grn_rc
cursor_heap_push(grn_ctx *ctx,
                 cursor_heap *h,
                 grn_ii *ii,
                 grn_id tid,
                 uint32_t offset2,
                 int weight,
                 grn_id min)
{
  int n, n2;
  grn_ii_cursor *c, *c2;
  if (h->n_entries >= h->n_bins) {
    int max = h->n_bins * 2;
    auto bins = static_cast<grn_ii_cursor **>(
      GRN_REALLOC(h->bins, sizeof(grn_ii_cursor *) * max));
    GRN_LOG(ctx, GRN_LOG_DEBUG, "expanded cursor_heap to %d,%p", max, bins);
    if (!bins) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    h->n_bins = max;
    h->bins = bins;
  }
  {
    if (!(c = grn_ii_cursor_open(ctx,
                                 ii,
                                 tid,
                                 min,
                                 GRN_ID_MAX,
                                 ii->n_elements,
                                 0))) {
      GRN_LOG(ctx, GRN_LOG_ERROR, "cursor open failed");
      return ctx->rc;
    }
    if (!grn_ii_cursor_next(ctx, c)) {
      grn_ii_cursor_close(ctx, c);
      return GRN_END_OF_DATA;
    }
    if (!grn_ii_cursor_next_pos(ctx, c)) {
      if (grn_logger_pass(ctx, GRN_LOG_ERROR)) {
        GRN_DEFINE_NAME(c->ii);
        grn_obj term;
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, c->ii, c->id, &term);
        GRN_LOG(ctx,
                GRN_LOG_ERROR,
                "[ii][cursor][heap][push][%.*s][%.*s][%u] "
                "invalid cursor: <%p>",
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                c->id,
                c);
        GRN_OBJ_FIN(ctx, &term);
      }
      grn_ii_cursor_close(ctx, c);
      return GRN_END_OF_DATA;
    }
    if (weight) {
      c->weight = weight;
    }
    n = h->n_entries++;
    while (n) {
      n2 = (n - 1) >> 1;
      c2 = h->bins[n2];
      if (GRN_II_CURSOR_CMP(c, c2)) {
        break;
      }
      h->bins[n] = c2;
      n = n2;
    }
    h->bins[n] = c;
  }
  return GRN_SUCCESS;
}

static inline grn_rc
cursor_heap_push2(cursor_heap *h)
{
  grn_rc rc = GRN_SUCCESS;
  return rc;
}

static inline grn_ii_cursor *
cursor_heap_min(cursor_heap *h)
{
  return h->n_entries ? h->bins[0] : NULL;
}

static inline void
cursor_heap_recalc_min(cursor_heap *h)
{
  int n = 0, n1, n2, m;
  if ((m = h->n_entries) > 1) {
    grn_ii_cursor *c = h->bins[0], *c1, *c2;
    for (;;) {
      n1 = n * 2 + 1;
      n2 = n1 + 1;
      c1 = n1 < m ? h->bins[n1] : NULL;
      c2 = n2 < m ? h->bins[n2] : NULL;
      if (c1 && GRN_II_CURSOR_CMP(c, c1)) {
        if (c2 && GRN_II_CURSOR_CMP(c, c2) && GRN_II_CURSOR_CMP(c1, c2)) {
          h->bins[n] = c2;
          n = n2;
        } else {
          h->bins[n] = c1;
          n = n1;
        }
      } else {
        if (c2 && GRN_II_CURSOR_CMP(c, c2)) {
          h->bins[n] = c2;
          n = n2;
        } else {
          h->bins[n] = c;
          break;
        }
      }
    }
  }
}

static inline void
cursor_heap_pop(grn_ctx *ctx, cursor_heap *h, grn_id min)
{
  if (h->n_entries) {
    grn_ii_cursor *c = h->bins[0];
    grn_ii_cursor_set_min(ctx, c, min);
    if (!grn_ii_cursor_next(ctx, c)) {
      grn_ii_cursor_close(ctx, c);
      h->bins[0] = h->bins[--h->n_entries];
    } else if (!grn_ii_cursor_next_pos(ctx, c)) {
      if (grn_logger_pass(ctx, GRN_LOG_ERROR)) {
        GRN_DEFINE_NAME(c->ii);
        grn_obj term;
        GRN_TEXT_INIT(&term, 0);
        grn_ii_get_term(ctx, c->ii, c->id, &term);
        GRN_LOG(ctx,
                GRN_LOG_ERROR,
                "[ii][cursor][heap][pop][%.*s][%.*s][%u] invalid cursor: <%p>",
                name_size,
                name,
                (int)GRN_TEXT_LEN(&term),
                GRN_TEXT_VALUE(&term),
                c->id,
                c);
        GRN_OBJ_FIN(ctx, &term);
      }
      grn_ii_cursor_close(ctx, c);
      h->bins[0] = h->bins[--h->n_entries];
    }
    if (h->n_entries > 1) {
      cursor_heap_recalc_min(h);
    }
  }
}

static inline void
cursor_heap_pop_pos(grn_ctx *ctx, cursor_heap *h)
{
  if (h->n_entries) {
    grn_ii_cursor *c = h->bins[0];
    if (!grn_ii_cursor_next_pos(ctx, c)) {
      if (!grn_ii_cursor_next(ctx, c)) {
        grn_ii_cursor_close(ctx, c);
        h->bins[0] = h->bins[--h->n_entries];
      } else if (!grn_ii_cursor_next_pos(ctx, c)) {
        if (grn_logger_pass(ctx, GRN_LOG_ERROR)) {
          GRN_DEFINE_NAME(c->ii);
          grn_obj term;
          GRN_TEXT_INIT(&term, 0);
          grn_ii_get_term(ctx, c->ii, c->id, &term);
          GRN_LOG(ctx,
                  GRN_LOG_ERROR,
                  "[ii][cursor][heap][pop][position][%.*s][%.*s][%u] "
                  "invalid cursor: <%p>",
                  name_size,
                  name,
                  (int)GRN_TEXT_LEN(&term),
                  GRN_TEXT_VALUE(&term),
                  c->id,
                  c);
          GRN_OBJ_FIN(ctx, &term);
        }
        grn_ii_cursor_close(ctx, c);
        h->bins[0] = h->bins[--h->n_entries];
      }
    }
    if (h->n_entries > 1) {
      cursor_heap_recalc_min(h);
    }
  }
}

static inline void
cursor_heap_close(grn_ctx *ctx, cursor_heap *h)
{
  int i;
  if (!h) {
    return;
  }
  for (i = h->n_entries; i--;) {
    grn_ii_cursor_close(ctx, h->bins[i]);
  }
  GRN_FREE(h->bins);
  GRN_FREE(h);
}

static grn_rc
grn_vector2updspecs(grn_ctx *ctx,
                    grn_ii *ii,
                    grn_id rid,
                    unsigned int section,
                    grn_obj *in,
                    grn_obj *out,
                    grn_tokenize_mode mode,
                    grn_obj *posting)
{
  grn_id tid;
  grn_section *v;
  grn_token_cursor *token_cursor;
  grn_ii_updspec **u;
  grn_hash *h = (grn_hash *)out;
  grn_obj *lexicon = ii->lexicon;
  if (in->u.v.body) {
    bool have_tokenizer = grn_table_have_tokenizer(ctx, lexicon);
    const char *head = GRN_BULK_HEAD(in->u.v.body);
    int32_t i;
    int32_t n = (int32_t)(in->u.v.n_sections);
    for (i = 0, v = in->u.v.sections; i < n; i++, v++) {
      unsigned int token_flags = 0;
      if (v->length && (token_cursor = grn_token_cursor_open(ctx,
                                                             lexicon,
                                                             head + v->offset,
                                                             v->length,
                                                             mode,
                                                             token_flags))) {
        if (v->domain != GRN_ID_NIL) {
          grn_token_cursor_set_query_domain(ctx, token_cursor, v->domain);
        }
        while (!token_cursor->status) {
          if ((tid = grn_token_cursor_next(ctx, token_cursor))) {
            if (posting) {
              GRN_RECORD_PUT(ctx, posting, tid);
            }
            if (!grn_hash_add(ctx,
                              h,
                              &tid,
                              sizeof(grn_id),
                              (void **)&u,
                              NULL)) {
              break;
            }
            if (!*u) {
              if (!(*u = grn_ii_updspec_open(ctx, rid, section))) {
                GRN_DEFINE_NAME(ii);
                MERR("[ii][update][spec] failed to create an update spec: "
                     "<%.*s>: "
                     "record:<%u>:<%u>, token:<%u>:<%d>:<%f>",
                     name_size,
                     name,
                     rid,
                     section,
                     tid,
                     token_cursor->pos,
                     v->weight);
                grn_token_cursor_close(ctx, token_cursor);
                return ctx->rc;
              }
            }
            if (grn_ii_updspec_add(ctx,
                                   *u,
                                   have_tokenizer ? token_cursor->pos : i,
                                   (int32_t)(v->weight))) {
              GRN_DEFINE_NAME(ii);
              MERR("[ii][update][spec] failed to add to update spec: "
                   "<%.*s>: "
                   "record:<%u>:<%u>, token:<%u>:<%d>:<%f>",
                   name_size,
                   name,
                   rid,
                   section,
                   tid,
                   token_cursor->pos,
                   v->weight);
              grn_token_cursor_close(ctx, token_cursor);
              return ctx->rc;
            }
          }
        }
        grn_token_cursor_close(ctx, token_cursor);
      }
    }
  }
  return ctx->rc;
}

static grn_rc
grn_uvector2updspecs_data(grn_ctx *ctx,
                          grn_ii *ii,
                          grn_id rid,
                          unsigned int section,
                          grn_obj *in,
                          grn_obj *out,
                          grn_tokenize_mode mode,
                          grn_obj *posting)
{
  grn_rc rc = GRN_SUCCESS;
  grn_hash *h = (grn_hash *)out;
  grn_obj *lexicon = ii->lexicon;
  grn_obj *tokenizer;
  bool need_cast;
  grn_obj element_buffer;
  grn_obj casted_element_buffer;
  int i, n;
  unsigned int element_size;

  tokenizer = grn_obj_get_info(ctx, lexicon, GRN_INFO_DEFAULT_TOKENIZER, NULL);
  need_cast = (!tokenizer && lexicon->header.domain != in->header.domain);
  if (need_cast) {
    GRN_VALUE_FIX_SIZE_INIT(&element_buffer, 0, in->header.domain);
    GRN_VALUE_FIX_SIZE_INIT(&casted_element_buffer,
                            GRN_OBJ_VECTOR,
                            lexicon->header.domain);
  } else {
    GRN_VOID_INIT(&element_buffer);
    GRN_VOID_INIT(&casted_element_buffer);
  }

  n = grn_uvector_size(ctx, in);
  element_size = grn_uvector_element_size(ctx, in);
  for (i = 0; i < n; i++) {
    grn_token_cursor *token_cursor;
    unsigned int token_flags = 0;
    const char *element;

    element = GRN_BULK_HEAD(in) + (element_size * i);
    if (need_cast) {
      GRN_BULK_REWIND(&element_buffer);
      GRN_BULK_REWIND(&casted_element_buffer);
      grn_bulk_write(ctx, &element_buffer, element, element_size);
      grn_rc cast_rc =
        grn_obj_cast(ctx, &element_buffer, &casted_element_buffer, true);
      if (cast_rc != GRN_SUCCESS) {
        GRN_DEFINE_NAME(ii);
        {
          grn_obj *domain =
            grn_ctx_at(ctx, casted_element_buffer.header.domain);
          char domain_name[GRN_TABLE_MAX_KEY_SIZE];
          int domain_name_size;
          domain_name_size =
            grn_obj_name(ctx, domain, domain_name, GRN_TABLE_MAX_KEY_SIZE);
          grn_obj inspected_element;
          GRN_TEXT_INIT(&inspected_element, 0);
          grn_inspect(ctx, &inspected_element, &element_buffer);
          GRN_LOG(ctx,
                  GRN_LOG_WARNING,
                  "[ii][updspec][uvector][data][%.*s] "
                  "failed to cast to <%.*s>: <%.*s>",
                  name_size,
                  name,
                  domain_name_size,
                  domain_name,
                  (int)GRN_TEXT_LEN(&inspected_element),
                  GRN_TEXT_VALUE(&inspected_element));
          GRN_OBJ_FIN(ctx, &inspected_element);
        }
      }
      element = GRN_BULK_HEAD(&casted_element_buffer);
      element_size = GRN_BULK_VSIZE(&casted_element_buffer);
    }
    token_cursor = grn_token_cursor_open(ctx,
                                         lexicon,
                                         element,
                                         element_size,
                                         mode,
                                         token_flags);
    if (!token_cursor) {
      continue;
    }

    grn_token_cursor_set_query_domain(ctx, token_cursor, in->header.domain);
    while (!token_cursor->status) {
      grn_id tid;
      if ((tid = grn_token_cursor_next(ctx, token_cursor))) {
        grn_ii_updspec **u;
        int pos;

        if (posting) {
          GRN_RECORD_PUT(ctx, posting, tid);
        }
        if (!grn_hash_add(ctx, h, &tid, sizeof(grn_id), (void **)&u, NULL)) {
          break;
        }
        if (!*u) {
          if (!(*u = grn_ii_updspec_open(ctx, rid, section))) {
            GRN_LOG(ctx,
                    GRN_LOG_ALERT,
                    "grn_ii_updspec_open on grn_uvector2updspecs_data failed!");
            grn_token_cursor_close(ctx, token_cursor);
            rc = GRN_NO_MEMORY_AVAILABLE;
            goto exit;
          }
        }
        if (tokenizer) {
          pos = token_cursor->pos;
        } else {
          pos = i;
        }
        if (grn_ii_updspec_add(ctx, *u, pos, 0)) {
          GRN_LOG(ctx,
                  GRN_LOG_ALERT,
                  "grn_ii_updspec_add on grn_uvector2updspecs failed!");
          grn_token_cursor_close(ctx, token_cursor);
          rc = GRN_NO_MEMORY_AVAILABLE;
          goto exit;
        }
      }
    }

    grn_token_cursor_close(ctx, token_cursor);
  }

exit:
  GRN_OBJ_FIN(ctx, &element_buffer);
  GRN_OBJ_FIN(ctx, &casted_element_buffer);

  return rc;
}

static grn_rc
grn_uvector2updspecs_id(grn_ctx *ctx,
                        grn_ii *ii,
                        grn_id rid,
                        unsigned int section,
                        grn_obj *in,
                        grn_obj *out)
{
  int i, n;
  grn_ii_updspec **u;
  grn_hash *h = (grn_hash *)out;

  n = grn_vector_size(ctx, in);
  for (i = 0; i < n; i++) {
    grn_id id;
    uint32_t weight;

    id = grn_uvector_get_element(ctx, in, i, &weight);
    if (!grn_hash_add(ctx, h, &id, sizeof(grn_id), (void **)&u, NULL)) {
      break;
    }
    if (!*u) {
      if (!(*u = grn_ii_updspec_open(ctx, rid, section))) {
        GRN_LOG(ctx,
                GRN_LOG_ALERT,
                "grn_ii_updspec_open on grn_ii_update failed!");
        return GRN_NO_MEMORY_AVAILABLE;
      }
    }
    if (grn_ii_updspec_add(ctx, *u, i, weight)) {
      GRN_LOG(ctx,
              GRN_LOG_ALERT,
              "grn_ii_updspec_add on grn_ii_update failed!");
      return GRN_NO_MEMORY_AVAILABLE;
    }
  }
  return GRN_SUCCESS;
}

static grn_rc
grn_uvector2updspecs(grn_ctx *ctx,
                     grn_ii *ii,
                     grn_id rid,
                     unsigned int section,
                     grn_obj *in,
                     grn_obj *out,
                     grn_tokenize_mode mode,
                     grn_obj *posting)
{
  if (grn_type_id_is_builtin(ctx, in->header.domain)) {
    return grn_uvector2updspecs_data(ctx,
                                     ii,
                                     rid,
                                     section,
                                     in,
                                     out,
                                     mode,
                                     posting);
  } else {
    return grn_uvector2updspecs_id(ctx, ii, rid, section, in, out);
  }
}

static inline grn_rc
grn_ii_column_update_internal(grn_ctx *ctx,
                              grn_ii *ii,
                              grn_id rid,
                              unsigned int section,
                              grn_obj *oldvalue,
                              grn_obj *newvalue,
                              grn_obj *posting)
{
  grn_id *tp;
  bool do_grn_ii_updspec_cmp = true;
  grn_ii_updspec **u, **un;
  grn_obj *old_value = oldvalue;
  grn_obj *new_value = newvalue;
  grn_obj old_buffer;
  grn_obj new_buffer;
  grn_obj buf, *post = NULL;

  if (!ii) {
    ERR(GRN_INVALID_ARGUMENT, "[ii][column][update] ii is NULL");
    return ctx->rc;
  }
  if (!ii->lexicon) {
    ERR(GRN_INVALID_ARGUMENT, "[ii][column][update] lexicon is NULL");
    return ctx->rc;
  }
  if (rid == GRN_ID_NIL) {
    GRN_DEFINE_NAME(ii);
    ERR(GRN_INVALID_ARGUMENT,
        "[ii][column][update] record ID is nil: <%.*s>",
        name_size,
        name);
    return ctx->rc;
  }
  if (old_value && old_value->header.type == GRN_VOID) {
    old_value = NULL;
  }
  if (new_value && new_value->header.type == GRN_VOID) {
    new_value = NULL;
  }
  if (old_value || new_value) {
    bool is_text_vector_index = true;
    if (old_value) {
      if (!(old_value->header.type == GRN_VECTOR &&
            grn_type_id_is_text_family(ctx, old_value->header.domain))) {
        is_text_vector_index = false;
      }
    }
    if (new_value) {
      if (!(new_value->header.type == GRN_VECTOR &&
            grn_type_id_is_text_family(ctx, new_value->header.domain))) {
        is_text_vector_index = false;
      }
    }
    if (is_text_vector_index) {
      grn_obj *tokenizer;
      grn_table_get_info(ctx, ii->lexicon, NULL, NULL, &tokenizer, NULL, NULL);
      if (tokenizer) {
        grn_obj old_elem, new_elem;
        unsigned int i, max_n;
        unsigned int old_n = 0, new_n = 0;
        if (old_value) {
          old_n = grn_vector_size(ctx, old_value);
          GRN_OBJ_INIT(&old_elem,
                       GRN_BULK,
                       GRN_OBJ_DO_SHALLOW_COPY,
                       old_value->header.domain);
        }
        if (new_value) {
          new_n = grn_vector_size(ctx, new_value);
          GRN_OBJ_INIT(&new_elem,
                       GRN_BULK,
                       GRN_OBJ_DO_SHALLOW_COPY,
                       new_value->header.domain);
        }
        max_n = (old_n > new_n) ? old_n : new_n;
        for (i = 0; i < max_n; i++) {
          grn_rc rc;
          grn_obj *old_p = NULL, *new_p = NULL;
          if (i < old_n) {
            const char *value;
            unsigned int size;
            size =
              grn_vector_get_element(ctx, old_value, i, &value, NULL, NULL);
            GRN_TEXT_SET_REF(&old_elem, value, size);
            old_p = &old_elem;
          }
          if (i < new_n) {
            const char *value;
            unsigned int size;
            size =
              grn_vector_get_element(ctx, new_value, i, &value, NULL, NULL);
            GRN_TEXT_SET_REF(&new_elem, value, size);
            new_p = &new_elem;
          }
          rc = grn_ii_column_update(ctx,
                                    ii,
                                    rid,
                                    section + i,
                                    old_p,
                                    new_p,
                                    posting);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
        if (old_value) {
          GRN_OBJ_FIN(ctx, &old_elem);
        }
        if (new_value) {
          GRN_OBJ_FIN(ctx, &new_elem);
        }
        return ctx->rc;
      }
    }
  }
  if (posting) {
    GRN_RECORD_INIT(&buf, GRN_OBJ_VECTOR, grn_obj_id(ctx, ii->lexicon));
    post = &buf;
  }
  if (grn_io_lock(ctx, ii->seg, grn_lock_timeout)) {
    return ctx->rc;
  }
  if (new_value) {
    unsigned char type = (ii->obj.header.domain == new_value->header.domain)
                           ? GRN_UVECTOR
                           : new_value->header.type;
    switch (type) {
    case GRN_BULK:
      {
        if (grn_bulk_is_zero(ctx, new_value)) {
          do_grn_ii_updspec_cmp = false;
        }
        grn_obj *new_value_before = new_value;
        GRN_OBJ_INIT(&new_buffer,
                     GRN_VECTOR,
                     GRN_OBJ_DO_SHALLOW_COPY,
                     new_value->header.domain);
        new_buffer.u.v.body = new_value;
        new_value = &new_buffer;
        grn_vector_delimit(ctx, new_value, 0, new_value_before->header.domain);
        if (new_value_before != newvalue) {
          grn_obj_close(ctx, new_value_before);
        }
      }
      /* fallthru */
    case GRN_VECTOR:
      {
        grn_obj *new_value_before = new_value;
        new_value = (grn_obj *)grn_hash_create(ctx,
                                               NULL,
                                               sizeof(grn_id),
                                               sizeof(grn_ii_updspec *),
                                               GRN_HASH_TINY);
        if (!new_value) {
          GRN_DEFINE_NAME(ii);
          MERR(
            "[ii][column][update][new][vector] failed to create a hash table: "
            "<%.*s>: ",
            name_size,
            name);
        } else {
          grn_vector2updspecs(ctx,
                              ii,
                              rid,
                              section,
                              new_value_before,
                              new_value,
                              GRN_TOKEN_ADD,
                              post);
        }
        if (new_value_before != newvalue) {
          grn_obj_close(ctx, new_value_before);
        }
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
      }
      break;
    case GRN_UVECTOR:
      {
        grn_obj *new_value_before = new_value;
        new_value = (grn_obj *)grn_hash_create(ctx,
                                               NULL,
                                               sizeof(grn_id),
                                               sizeof(grn_ii_updspec *),
                                               GRN_HASH_TINY);
        if (!new_value) {
          GRN_DEFINE_NAME(ii);
          MERR(
            "[ii][column][update][new][uvector] failed to create a hash table: "
            "<%.*s>: ",
            name_size,
            name);
        } else {
          if (new_value_before->header.type == GRN_UVECTOR) {
            grn_uvector2updspecs(ctx,
                                 ii,
                                 rid,
                                 section,
                                 new_value_before,
                                 new_value,
                                 GRN_TOKEN_ADD,
                                 post);
          } else {
            grn_obj uvector;
            uint32_t weight = 0;
            GRN_VALUE_FIX_SIZE_INIT(&uvector,
                                    GRN_OBJ_VECTOR,
                                    new_value_before->header.domain);
            if (new_value_before->header.impl_flags & GRN_OBJ_WITH_WEIGHT) {
              uvector.header.impl_flags |= GRN_OBJ_WITH_WEIGHT;
            }
            grn_uvector_add_element(ctx,
                                    &uvector,
                                    GRN_RECORD_VALUE(new_value_before),
                                    weight);
            grn_uvector2updspecs(ctx,
                                 ii,
                                 rid,
                                 section,
                                 &uvector,
                                 new_value,
                                 GRN_TOKEN_ADD,
                                 post);
            GRN_OBJ_FIN(ctx, &uvector);
          }
        }
        if (new_value_before != newvalue) {
          grn_obj_close(ctx, new_value_before);
        }
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
      }
      break;
    case GRN_TABLE_HASH_KEY:
      break;
    default:
      {
        GRN_DEFINE_NAME(ii);
        ERR(GRN_INVALID_ARGUMENT,
            "[ii][column][update][new] invalid object: "
            "<%.*s>: "
            "<%s>(%#x)",
            name_size,
            name,
            grn_obj_type_to_string(type),
            type);
      }
      goto exit;
    }
  }
  if (posting) {
    grn_ii_updspec *u_;
    uint32_t offset = 0;
    grn_id tid_ = 0, gap, tid, *tpe;
    grn_table_sort_optarg arg = {
      GRN_TABLE_SORT_ASC | GRN_TABLE_SORT_AS_NUMBER |
        GRN_TABLE_SORT_AS_UNSIGNED,
      NULL,
      NULL,
      NULL,
      0,
    };
    grn_array *sorted = grn_array_create(ctx, NULL, sizeof(grn_id), 0);
    grn_hash_sort(ctx, (grn_hash *)new_value, -1, sorted, &arg);
    GRN_TEXT_PUT(ctx,
                 posting,
                 ((grn_hash *)new_value)->n_entries,
                 sizeof(uint32_t));
    GRN_ARRAY_EACH(ctx, sorted, 0, 0, id, &tp, {
      grn_hash_get_key(ctx, (grn_hash *)new_value, *tp, &tid, sizeof(grn_id));
      gap = tid - tid_;
      GRN_TEXT_PUT(ctx, posting, &gap, sizeof(grn_id));
      tid_ = tid;
    });
    GRN_ARRAY_EACH(ctx, sorted, 0, 0, id, &tp, {
      grn_hash_get_value(ctx, (grn_hash *)new_value, *tp, &u_);
      u_->offset = offset++;
      GRN_TEXT_PUT(ctx, posting, &u_->tf, sizeof(int32_t));
    });
    tpe = (grn_id *)GRN_BULK_CURR(post);
    for (tp = (grn_id *)GRN_BULK_HEAD(post); tp < tpe; tp++) {
      grn_hash_get(ctx,
                   (grn_hash *)new_value,
                   (void *)tp,
                   sizeof(grn_id),
                   (void **)&u);
      GRN_TEXT_PUT(ctx, posting, &(*u)->offset, sizeof(int32_t));
    }
    GRN_OBJ_FIN(ctx, post);
    grn_array_close(ctx, sorted);
  }

  if (old_value) {
    unsigned char type = (ii->obj.header.domain == old_value->header.domain)
                           ? GRN_UVECTOR
                           : old_value->header.type;
    switch (type) {
    case GRN_BULK:
      {
        //        const char *str = GRN_BULK_HEAD(old);
        //        unsigned int str_len = GRN_BULK_VSIZE(old);
        grn_obj *old_value_before = old_value;
        GRN_OBJ_INIT(&old_buffer,
                     GRN_VECTOR,
                     GRN_OBJ_DO_SHALLOW_COPY,
                     old_value->header.domain);
        old_buffer.u.v.body = old_value;
        old_value = &old_buffer;
        grn_vector_delimit(ctx, old_value, 0, old_value_before->header.domain);
        if (old_value_before != oldvalue) {
          grn_obj_close(ctx, old_value_before);
        }
      }
      /* fallthru */
    case GRN_VECTOR:
      {
        grn_obj *old_value_keep = old_value;
        old_value = (grn_obj *)grn_hash_create(ctx,
                                               NULL,
                                               sizeof(grn_id),
                                               sizeof(grn_ii_updspec *),
                                               GRN_HASH_TINY);
        if (!old_value) {
          GRN_DEFINE_NAME(ii);
          MERR(
            "[ii][column][update][old][vector] failed to create a hash table: "
            "<%.*s>: ",
            name_size,
            name);
        } else {
          grn_vector2updspecs(ctx,
                              ii,
                              rid,
                              section,
                              old_value_keep,
                              old_value,
                              GRN_TOKEN_DEL,
                              NULL);
        }
        if (old_value_keep != oldvalue) {
          grn_obj_close(ctx, old_value_keep);
        }
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
      }
      break;
    case GRN_UVECTOR:
      {
        grn_obj *old_value_keep = old_value;
        old_value = (grn_obj *)grn_hash_create(ctx,
                                               NULL,
                                               sizeof(grn_id),
                                               sizeof(grn_ii_updspec *),
                                               GRN_HASH_TINY);
        if (!old_value) {
          GRN_DEFINE_NAME(ii);
          MERR(
            "[ii][column][update][old][uvector] failed to create a hash table: "
            "<%.*s>: ",
            name_size,
            name);
        } else {
          if (old_value_keep->header.type == GRN_UVECTOR) {
            grn_uvector2updspecs(ctx,
                                 ii,
                                 rid,
                                 section,
                                 old_value_keep,
                                 old_value,
                                 GRN_TOKEN_DEL,
                                 NULL);
          } else {
            grn_obj uvector;
            uint32_t weight = 0;
            GRN_VALUE_FIX_SIZE_INIT(&uvector,
                                    GRN_OBJ_VECTOR,
                                    old_value_keep->header.domain);
            if (old_value_keep->header.impl_flags & GRN_OBJ_WITH_WEIGHT) {
              uvector.header.impl_flags |= GRN_OBJ_WITH_WEIGHT;
            }
            grn_uvector_add_element(ctx,
                                    &uvector,
                                    GRN_RECORD_VALUE(old_value_keep),
                                    weight);
            grn_uvector2updspecs(ctx,
                                 ii,
                                 rid,
                                 section,
                                 &uvector,
                                 old_value,
                                 GRN_TOKEN_DEL,
                                 NULL);
            GRN_OBJ_FIN(ctx, &uvector);
          }
        }
        if (old_value_keep != oldvalue) {
          grn_obj_close(ctx, old_value_keep);
        }
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
      }
      break;
    case GRN_TABLE_HASH_KEY:
      break;
    default:
      {
        GRN_DEFINE_NAME(ii);
        ERR(GRN_INVALID_ARGUMENT,
            "[ii][column][update][old] invalid object: "
            "<%.*s>: "
            "<%s>(%#x)",
            name_size,
            name,
            grn_obj_type_to_string(type),
            type);
      }
      goto exit;
    }
  }

  if (old_value) {
    grn_id eid;
    grn_hash *o = (grn_hash *)old_value;
    grn_hash *n = (grn_hash *)new_value;
    GRN_HASH_EACH(ctx, o, id, &tp, NULL, &u, {
      if (n && (eid = grn_hash_get(ctx, n, tp, sizeof(grn_id), (void **)&un))) {
        if (do_grn_ii_updspec_cmp && !grn_ii_updspec_cmp(*u, *un)) {
          grn_ii_updspec_close(ctx, *un);
          grn_hash_delete_by_id(ctx, n, eid, NULL);
        }
      } else {
        grn_ii_delete_one(ctx, ii, *tp, *u, n);
      }
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    });
  }
  if (new_value) {
    grn_hash *n = (grn_hash *)new_value;
    GRN_HASH_EACH(ctx, n, id, &tp, NULL, &u, {
      grn_ii_update_one(ctx, ii, *tp, *u, n);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    });
  } else {
    if (!section) {
      /* todo: delete key when all sections deleted */
    }
  }
exit:
  grn_io_unlock(ctx, ii->seg);
  if (old_value && old_value->header.type == GRN_TABLE_HASH_KEY) {
    grn_hash *o = (grn_hash *)old_value;
    GRN_HASH_EACH(ctx, o, id, &tp, NULL, &u, {
      grn_ii_updspec_close(ctx, *u);
    });
    if (old_value != oldvalue) {
      grn_obj_close(ctx, old_value);
    }
  }
  if (new_value && new_value->header.type == GRN_TABLE_HASH_KEY) {
    grn_hash *n = (grn_hash *)new_value;
    GRN_HASH_EACH(ctx, n, id, &tp, NULL, &u, {
      grn_ii_updspec_close(ctx, *u);
    });
    if (new_value != newvalue) {
      grn_obj_close(ctx, new_value);
    }
  }
  return ctx->rc;
}

extern "C" grn_rc
grn_ii_column_update(grn_ctx *ctx,
                     grn_ii *ii,
                     grn_id rid,
                     unsigned int section,
                     grn_obj *oldvalue,
                     grn_obj *newvalue,
                     grn_obj *posting)
{
  GRN_SLOW_LOG_PUSH(ctx, GRN_LOG_DEBUG);
  grn_rc rc = grn_ii_column_update_internal(ctx,
                                            ii,
                                            rid,
                                            section,
                                            oldvalue,
                                            newvalue,
                                            posting);
  GRN_SLOW_LOG_POP_BEGIN(ctx, GRN_LOG_DEBUG, elapsed_time)
  {
    GRN_DEFINE_NAME(ii);
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "[ii][column][update][slow][%f] "
            "<%.*s>: "
            "record:<%u>:<%u>",
            elapsed_time,
            name_size,
            name,
            rid,
            section);
  }
  GRN_SLOW_LOG_POP_END(ctx);
  return rc;
}

/* token_info */

typedef struct {
  /* All token cursors of the token info. If the target token is
   * expanded (EX_* except EX_NONE), token cursors of multiple tokens
   * may be included. */
  cursor_heap *cursors;
  /* The number of tokens from the first token in the given query or a
   * phrase in the given query. */
  uint32_t offset;
  /* The number of tokens from the first token in the input. */
  int32_t pos;
  /* The estimated size (see grn_ii_estimate_size() for estimated
   * size) of all tokens including this token_info. */
  uint32_t size;
  /* The number of the token info. If the target token is expanded
   * (EX_* except EX_NONE), there may be multiple tokens in a token
   * info. */
  int ntoken;
  /* The current posting of the token info. */
  grn_posting *p;
  /* The ID of phrase group that the token info belongs.
   *
   * This is used only for GRN_OP_*NEAR_PHRASE_PRODUCT. */
  uint32_t phrase_group_id;
  /* The ID of phrase that the token info belongs.
   *
   * This is used only for GRN_OP_*NEAR_PHRASE*. */
  uint32_t phrase_id;
  /* The number of tokens in the phrase that the token info
   * belongs. This counts not used tokens. For example, "abc" with
   * TokenNgram is tokenized to "ab", "bc" and "c" but "c" isn't
   * used. Because we can find "abc" only with "ab" and
   * "bc". n_tokens_in_phrase is 3 because it counts
   * "c". n_used_tokens_in_phrase is 2 because it doesn't count "c".
   *
   * This is used only for GRN_OP_*NEAR_PHRASE*. */
  uint32_t n_tokens_in_phrase;
  /* The number of used tokens in the phrase that the token info
   * belongs. See also the above n_tokens_in_phrase document.
   *
   * This is used only for GRN_OP_*NEAR_PHRASE*. */
  uint32_t n_used_tokens_in_phrase;
  /* Whether the token info must be the last token or not. If the
   * token info is appeared not at the last, the query isn't matched.
   *
   * This is used only for GRN_OP_*NEAR_PHRASE*. */
  bool must_last;
} token_info;

extern "C" void
grn_inspect_token_info(grn_ctx *ctx, grn_obj *buffer, token_info *info)
{
  grn_text_printf(ctx, buffer, "#<token_info:\n");
  grn_text_printf(ctx, buffer, "  cursors: ");
  grn_obj cursor_heap_inspected;
  GRN_TEXT_INIT(&cursor_heap_inspected, 0);
  grn_inspect_cursor_heap(ctx, &cursor_heap_inspected, info->cursors);
  grn_inspect_indent(ctx, buffer, &cursor_heap_inspected, "  ");
  GRN_OBJ_FIN(ctx, &cursor_heap_inspected);
  grn_text_printf(ctx, buffer, "\n");
  grn_text_printf(ctx, buffer, "  offset:%u\n", info->offset);
  grn_text_printf(ctx, buffer, "  pos:%d\n", info->pos);
  grn_text_printf(ctx, buffer, "  size:%u\n", info->size);
  grn_text_printf(ctx, buffer, "  n_tokens:%d\n", info->ntoken);
  grn_text_printf(
    ctx,
    buffer,
    "  posting:#<posting: rid:%u sid:%u pos:%u tf:%u weight:%u>\n",
    info->p->rid,
    info->p->sid,
    info->p->pos,
    info->p->tf,
    info->p->weight);
  grn_text_printf(ctx, buffer, "  phrase_group_id:%u\n", info->phrase_group_id);
  grn_text_printf(ctx, buffer, "  phrase_id:%u\n", info->phrase_id);
  grn_text_printf(ctx,
                  buffer,
                  "  n_tokens_in_phrase:%u\n",
                  info->n_tokens_in_phrase);
  grn_text_printf(ctx,
                  buffer,
                  "  n_used_tokens_in_phrase:%u\n",
                  info->n_used_tokens_in_phrase);
  grn_text_printf(ctx,
                  buffer,
                  "  must_last:%s\n",
                  info->must_last ? "true" : "false");
  grn_text_printf(ctx, buffer, ">");
}

extern "C" void
grn_p_token_info(grn_ctx *ctx, token_info *info)
{
  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect_token_info(ctx, &inspected, info);
  printf("%.*s\n", (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
}

extern "C" void
grn_inspect_token_infos(grn_ctx *ctx,
                        grn_obj *buffer,
                        token_info **infos,
                        uint32_t n)
{
  uint32_t i;
  for (i = 0; i < n; i++) {
    if (i > 0) {
      GRN_TEXT_PUTC(ctx, buffer, '\n');
    }
    grn_text_printf(ctx, buffer, "[%u]\n", i);
    grn_inspect_token_info(ctx, buffer, infos[i]);
  }
}

extern "C" void
grn_p_token_infos(grn_ctx *ctx, token_info **infos, uint32_t n)
{
  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect_token_infos(ctx, &inspected, infos, n);
  printf("%.*s\n", (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
}

#define EX_NONE   0
#define EX_PREFIX 1
#define EX_SUFFIX 2
#define EX_BOTH   3
#define EX_FUZZY  4

struct _btr_node {
  struct _btr_node *car;
  struct _btr_node *cdr;
  token_info *ti;
};

typedef struct _btr_node btr_node;

typedef struct {
  int n;
  token_info *min;
  token_info *max;
  btr_node *root;
  btr_node *nodes;
  uint32_t n_must_lasts;
} btr;

typedef enum {
  GRN_WV_NONE = 0,
  GRN_WV_STATIC,
  GRN_WV_DYNAMIC,
  GRN_WV_CONSTANT
} grn_wv_mode;

typedef struct {
  uint32_t n_phrases;
  btr *btree;
} phrase_group;

typedef struct {
  grn_ii *ii;
  grn_obj *lexicon;
  const char *query;
  unsigned int query_len;
  grn_hash *result_set;
  grn_operator op;
  grn_select_optarg *optarg;
  grn_operator mode;
  grn_wv_mode wv_mode;
  grn_scorer_score_func *score_func;
  grn_scorer_matched_record record;
  grn_id previous_min;
  grn_id current_min;
  bool set_min_enable_for_and_query;
  grn_fuzzy_search_optarg *fuzzy_search_optarg;
  grn_obj *query_options;
  bool only_skip_token;
  token_info **token_infos;
  uint32_t n_token_infos;
  uint32_t n_phrase_groups;
  phrase_group *phrase_groups;
  uint32_t n_phrases;
  token_info *current_token_info;
  btr *bt;
  int max_interval;
  int additional_last_interval;
  grn_obj *max_element_intervals;
  int min_interval;
  btr *check_element_intervals_btree;
  int32_t pos;
  grn_id rid;
  uint32_t sid;
  grn_id next_rid;
  uint32_t next_sid;
  bool may_match;
  int32_t start_pos;
  int32_t end_pos;
  int32_t total_score;
  uint32_t n_occurrences;
} grn_ii_select_data;

static inline grn_token_cursor *
grn_ii_select_data_open_token_cursor(grn_ctx *ctx,
                                     grn_ii_select_data *data,
                                     const char *text,
                                     size_t text_length,
                                     grn_tokenize_mode mode)
{
  uint32_t token_flags = GRN_TOKEN_CURSOR_ENABLE_TOKENIZED_DELIMITER;
  grn_token_cursor *token_cursor = grn_token_cursor_open(ctx,
                                                         data->lexicon,
                                                         text,
                                                         text_length,
                                                         mode,
                                                         token_flags);
  if (!token_cursor) {
    return NULL;
  }
  if (data->query_options) {
    grn_token_cursor_set_query_options(ctx, token_cursor, data->query_options);
    if (ctx->rc != GRN_SUCCESS) {
      grn_token_cursor_close(ctx, token_cursor);
      return NULL;
    }
  }
  if (data->optarg->query_domain != GRN_ID_NIL) {
    grn_token_cursor_set_query_domain(ctx,
                                      token_cursor,
                                      data->optarg->query_domain);
  }
  return token_cursor;
}

static inline void
token_info_expand_both(grn_ctx *ctx,
                       grn_ii_select_data *data,
                       const char *key,
                       unsigned int key_size,
                       token_info *ti)
{
  int s = 0;
  grn_hash *h, *g;
  uint32_t *offset2;
  grn_hash_cursor *c;
  grn_id *tp, *tq;
  if ((h = grn_hash_create(ctx, NULL, sizeof(grn_id), 0, 0))) {
    grn_operator mode = GRN_OP_PREFIX;
    if (data->lexicon->header.type == GRN_TABLE_HASH_KEY) {
      mode = GRN_OP_EXACT;
    }
    /* TODO: use cursor like EX_PREFIX */
    grn_table_search(ctx,
                     data->lexicon,
                     key,
                     key_size,
                     mode,
                     (grn_obj *)h,
                     GRN_OP_OR);
    if (GRN_HASH_SIZE(h)) {
      if ((ti->cursors = cursor_heap_open(ctx, GRN_HASH_SIZE(h) + 256))) {
        if ((c = grn_hash_cursor_open(ctx, h, NULL, 0, NULL, 0, 0, -1, 0))) {
          uint32_t key2_size;
          const char *key2;
          while (grn_hash_cursor_next(ctx, c)) {
            grn_hash_cursor_get_key(ctx, c, (void **)&tp);
            key2 = _grn_table_key(ctx, data->lexicon, *tp, &key2_size);
            if (!key2) {
              break;
            }
            if ((data->lexicon->header.type != GRN_TABLE_PAT_KEY) ||
                !(data->lexicon->header.flags & GRN_OBJ_KEY_WITH_SIS) ||
                key2_size <= 2) { // todo: refine
              if ((s = grn_ii_estimate_size(ctx, data->ii, *tp))) {
                cursor_heap_push(ctx,
                                 ti->cursors,
                                 data->ii,
                                 *tp,
                                 0,
                                 0,
                                 data->previous_min);
                ti->ntoken++;
                ti->size += s;
              }
            } else {
              if ((g = grn_hash_create(ctx,
                                       NULL,
                                       sizeof(grn_id),
                                       0,
                                       GRN_HASH_TINY))) {
                /* TODO: use cursor like EX_PREFIX */
                grn_pat_suffix_search(ctx,
                                      (grn_pat *)(data->lexicon),
                                      key2,
                                      key2_size,
                                      g);
                GRN_HASH_EACH(ctx, g, id, &tq, NULL, &offset2, {
                  if ((s = grn_ii_estimate_size(ctx, data->ii, *tq))) {
                    cursor_heap_push(ctx,
                                     ti->cursors,
                                     data->ii,
                                     *tq,
                                     /* *offset2 */ 0,
                                     0,
                                     data->previous_min);
                    ti->ntoken++;
                    ti->size += s;
                  }
                });
                grn_hash_close(ctx, g);
              }
            }
          }
          grn_hash_cursor_close(ctx, c);
        }
      }
    }
    grn_hash_close(ctx, h);
  }
}

static inline grn_rc
token_info_close(grn_ctx *ctx, token_info *ti)
{
  cursor_heap_close(ctx, ti->cursors);
  GRN_FREE(ti);
  return GRN_SUCCESS;
}

/*
 * If tid is GRN_ID_NIL, caller must specify valid key and key_size
 * (not NULL and 0). tid may be retrieved by grn_table_get(ctx,
 * data->lexicon, key, key_size) if tid is needed and tid is GRN_ID_NIL.
 *
 * If key is NULL, caller must specify valid tid (not GRN_ID_NIL). key
 * and key_size may be retrieved by _grn_table_key(ctx, data->lexicon,
 * tid, &key_size) if key is needed and key is NULL.
 *
 * Note that we should not normalize the input multiple times. Some
 * normalizers such as NormalizerTable aren't idempotent. If we
 * normalize the input multipe times with a non-idempotent normalizer,
 * the tokenized result is broken.
 */
static inline token_info *
token_info_open(grn_ctx *ctx,
                grn_ii_select_data *data,
                grn_id tid,
                const char *key,
                unsigned int key_size,
                uint32_t offset,
                int mode)
{
  uint32_t s = 0;
  grn_hash *h;
  grn_id *tp;
  if (tid == GRN_ID_NIL && !key) {
    return NULL;
  }
  auto ti = static_cast<token_info *>(GRN_CALLOC(sizeof(token_info)));
  if (!ti) {
    return NULL;
  }
  ti->cursors = NULL;
  ti->size = 0;
  ti->ntoken = 0;
  ti->offset = offset;
  ti->phrase_id = 0;
  ti->must_last = false;
  switch (mode) {
  case EX_BOTH:
    if (!key) {
      key = _grn_table_key(ctx, data->lexicon, tid, &key_size);
    }
    token_info_expand_both(ctx, data, key, key_size, ti);
    break;
  case EX_NONE:
    if (tid == GRN_ID_NIL) {
      tid = grn_table_get(ctx, data->lexicon, key, key_size);
    }
    if (tid != GRN_ID_NIL && (s = grn_ii_estimate_size(ctx, data->ii, tid)) &&
        (ti->cursors = cursor_heap_open(ctx, 1))) {
      cursor_heap_push(ctx,
                       ti->cursors,
                       data->ii,
                       tid,
                       0,
                       0,
                       data->previous_min);
      ti->ntoken++;
      ti->size = s;
    }
    break;
  case EX_PREFIX:
    if (data->lexicon->header.type == GRN_TABLE_HASH_KEY) {
      if (tid == GRN_ID_NIL) {
        tid = grn_table_get(ctx, data->lexicon, key, key_size);
      }
      if (tid != GRN_ID_NIL) {
        ti->cursors = cursor_heap_open(ctx, 1);
        /* TODO: report error on !ti->cursors */
        if (ti->cursors) {
          s = grn_ii_estimate_size(ctx, data->ii, tid);
          if (s > 0) {
            cursor_heap_push(ctx,
                             ti->cursors,
                             data->ii,
                             tid,
                             0,
                             0,
                             data->previous_min);
            ti->ntoken++;
            ti->size += s;
          }
        }
      }
    } else {
      if (!key) {
        key = _grn_table_key(ctx, data->lexicon, tid, &key_size);
      }
      GRN_TABLE_EACH_BEGIN_MIN(ctx,
                               data->lexicon,
                               cursor,
                               id,
                               key,
                               key_size,
                               GRN_CURSOR_PREFIX)
      {
        if (!ti->cursors) {
          /* This is heuristic. 0.1% tokens may be matched. Must be
           * greater than 0. */
          unsigned int initial_capacity =
            (grn_table_size(ctx, data->lexicon) / 1000) + 1;
          ti->cursors = cursor_heap_open(ctx, initial_capacity);
          if (!ti->cursors) {
            /* TODO: report error */
            break;
          }
        }
        s = grn_ii_estimate_size(ctx, data->ii, id);
        if (s > 0) {
          cursor_heap_push(ctx,
                           ti->cursors,
                           data->ii,
                           id,
                           0,
                           0,
                           data->previous_min);
          ti->ntoken++;
          ti->size += s;
        }
      }
      GRN_TABLE_EACH_END(ctx, cursor);
    }
    break;
  case EX_SUFFIX:
    if ((h = grn_hash_create(ctx, NULL, sizeof(grn_id), 0, 0))) {
      grn_operator mode = GRN_OP_SUFFIX;
      if (data->lexicon->header.type == GRN_TABLE_HASH_KEY) {
        mode = GRN_OP_EXACT;
      }
      if (!key) {
        key = _grn_table_key(ctx, data->lexicon, tid, &key_size);
      }
      /* TODO: use cursor like EX_PREFIX */
      grn_table_search(ctx,
                       data->lexicon,
                       key,
                       key_size,
                       mode,
                       (grn_obj *)h,
                       GRN_OP_OR);
      if (GRN_HASH_SIZE(h)) {
        if ((ti->cursors = cursor_heap_open(ctx, GRN_HASH_SIZE(h)))) {
          uint32_t *offset2;
          GRN_HASH_EACH(ctx, h, id, &tp, NULL, &offset2, {
            if ((s = grn_ii_estimate_size(ctx, data->ii, *tp))) {
              cursor_heap_push(ctx,
                               ti->cursors,
                               data->ii,
                               *tp,
                               /* *offset2 */ 0,
                               0,
                               data->previous_min);
              ti->ntoken++;
              ti->size += s;
            }
          });
        }
      }
      grn_hash_close(ctx, h);
    }
    break;
  case EX_FUZZY:
    if ((h = (grn_hash *)grn_table_create(ctx,
                                          NULL,
                                          0,
                                          NULL,
                                          GRN_OBJ_TABLE_HASH_KEY |
                                            GRN_OBJ_WITH_SUBREC,
                                          grn_ctx_at(ctx, GRN_DB_UINT32),
                                          NULL))) {
      if (!key) {
        key = _grn_table_key(ctx, data->lexicon, tid, &key_size);
      }
      grn_ctx_trace_log_emit_string(ctx,
                                    "ii.select.fuzzy.input",
                                    key,
                                    key_size);
      /* TODO: use cursor like EX_PREFIX */
      grn_table_fuzzy_search(ctx,
                             data->lexicon,
                             key,
                             key_size,
                             data->fuzzy_search_optarg,
                             (grn_obj *)h,
                             GRN_OP_OR);
      if (GRN_HASH_SIZE(h)) {
        if ((ti->cursors = cursor_heap_open(ctx, GRN_HASH_SIZE(h)))) {
          grn_rset_recinfo *ri;
          GRN_HASH_EACH(ctx, h, id, &tp, NULL, (void **)&ri, {
            if ((s = grn_ii_estimate_size(ctx, data->ii, *tp))) {
              cursor_heap_push(ctx,
                               ti->cursors,
                               data->ii,
                               *tp,
                               0,
                               (int)(ri->score - 1),
                               data->previous_min);
              ti->ntoken++;
              ti->size += s;
              grn_ctx_trace_log_emit_record_key(ctx,
                                                "ii.select.fuzzy.input.actual",
                                                data->ii->lexicon,
                                                *tp);
            }
          });
        }
      }
      grn_obj_close(ctx, (grn_obj *)h);
    }
    break;
  }
  if (ctx->rc != GRN_SUCCESS) {
    token_info_close(ctx, ti);
    return NULL;
  }
  if (cursor_heap_push2(ti->cursors)) {
    token_info_close(ctx, ti);
    return NULL;
  }
  {
    grn_ii_cursor *ic;
    if (ti->cursors && (ic = cursor_heap_min(ti->cursors))) {
      grn_posting *p = ic->post;
      ti->pos = p->pos - ti->offset;
      ti->p = p;
    } else {
      token_info_close(ctx, ti);
      ti = NULL;
    }
  }
  return ti;
}

static inline grn_rc
token_info_skip(grn_ctx *ctx, token_info *ti, uint32_t rid, uint32_t sid)
{
  grn_ii_cursor *c;
  grn_posting *p;
  for (;;) {
    if (!(c = cursor_heap_min(ti->cursors))) {
      return GRN_END_OF_DATA;
    }
    p = c->post;
    if (p->rid > rid || (p->rid == rid && p->sid >= sid)) {
      break;
    }
    cursor_heap_pop(ctx, ti->cursors, rid);
  }
  ti->pos = p->pos - ti->offset;
  ti->p = p;
  return GRN_SUCCESS;
}

static inline grn_rc
token_info_skip_pos(
  grn_ctx *ctx, token_info *ti, uint32_t rid, uint32_t sid, int32_t pos)
{
  grn_ii_cursor *c;
  grn_posting *p;
  pos += ti->offset;
  for (;;) {
    if (!(c = cursor_heap_min(ti->cursors))) {
      return GRN_END_OF_DATA;
    }
    p = c->post;
    if (p->rid != rid || p->sid != sid || (int32_t)(p->pos) >= pos) {
      break;
    }
    cursor_heap_pop_pos(ctx, ti->cursors);
  }
  ti->pos = p->pos - ti->offset;
  ti->p = p;
  return GRN_SUCCESS;
}

static inline int
token_compare(const void *a, const void *b)
{
  const token_info *t1 = *((token_info **)a), *t2 = *((token_info **)b);
  if (t1->phrase_group_id == t2->phrase_group_id) {
    if (t1->phrase_id == t2->phrase_id) {
      return t1->size - t2->size;
    } else {
      return t1->phrase_id - t2->phrase_id;
    }
  } else {
    return t1->phrase_group_id - t2->phrase_group_id;
  }
}

#define TOKEN_CANDIDATE_NODE_SIZE         32
#define TOKEN_CANDIDATE_ADJACENT_MAX_SIZE 16
#define TOKEN_CANDIDATE_QUEUE_SIZE        64
#define TOKEN_CANDIDATE_SIZE              16

typedef struct {
  grn_id tid;
  const unsigned char *token;
  uint32_t token_size;
  const unsigned char *source_token;
  uint32_t source_token_size;
  int32_t pos;
  grn_token_cursor_status status;
  int ef;
  uint32_t estimated_size;
  uint8_t adjacent[TOKEN_CANDIDATE_ADJACENT_MAX_SIZE]; /* Index of adjacent node
                                                          from top */
  uint8_t n_adjacent;
} token_candidate_node;

typedef struct {
  uint32_t
    *candidates; /* Standing bits indicate index of token_candidate_node */
  int top;
  int rear;
  int size;
} token_candidate_queue;

typedef struct {
  grn_ii_select_data *select_data;
  token_candidate_node *nodes;
  uint32_t n_nodes;
  uint32_t max_estimated_size;
  uint32_t selected_candidate;
} token_candidate_data;

static inline void
token_candidate_adjacent_set(grn_ctx *ctx,
                             grn_token_cursor *token_cursor,
                             token_candidate_node *top,
                             token_candidate_node *curr)
{
  bool exists_adjacent = false;
  token_candidate_node *adj;
  for (adj = top; adj < curr; adj++) {
    if (token_cursor->curr <= adj->token + adj->token_size) {
      if (adj->n_adjacent < TOKEN_CANDIDATE_ADJACENT_MAX_SIZE) {
        adj->adjacent[adj->n_adjacent] = curr - top;
        adj->n_adjacent++;
        exists_adjacent = true;
      }
    }
  }
  if (!exists_adjacent) {
    adj = curr - 1;
    if (adj->n_adjacent < TOKEN_CANDIDATE_ADJACENT_MAX_SIZE) {
      adj->adjacent[adj->n_adjacent] = curr - top;
      adj->n_adjacent++;
    }
  }
}

static inline grn_rc
token_candidate_init(grn_ctx *ctx,
                     token_candidate_data *data,
                     grn_ii_select_data *select_data,
                     grn_token_cursor *token_cursor,
                     grn_id tid,
                     int ef)
{
  data->select_data = select_data;
  size_t size = TOKEN_CANDIDATE_NODE_SIZE;
  data->nodes = static_cast<token_candidate_node *>(
    GRN_MALLOC(TOKEN_CANDIDATE_NODE_SIZE * sizeof(token_candidate_node)));
  if (!data->nodes) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  token_candidate_node *top = data->nodes;
  token_candidate_node *curr = top;

#define TOKEN_CANDIDATE_NODE_SET()                                             \
  {                                                                            \
    grn_token *token = grn_token_cursor_get_token(ctx, token_cursor);          \
    curr->tid = tid;                                                           \
    curr->token = token_cursor->curr;                                          \
    curr->token_size = token_cursor->curr_size;                                \
    curr->source_token_size = grn_token_get_source_length(ctx, token);         \
    if (curr->source_token_size > 0) {                                         \
      curr->source_token = NULL;                                               \
    } else {                                                                   \
      curr->source_token =                                                     \
        token_cursor->orig + grn_token_get_source_offset(ctx, token);          \
    }                                                                          \
    curr->pos = token_cursor->pos;                                             \
    curr->status = token_cursor->status;                                       \
    curr->ef = ef;                                                             \
    curr->estimated_size = grn_ii_estimate_size(ctx, select_data->ii, tid);    \
    curr->n_adjacent = 0;                                                      \
  }
  TOKEN_CANDIDATE_NODE_SET();
  GRN_LOG(ctx,
          GRN_LOG_DEBUG,
          "[ii][overlap_token_skip] tid=%u pos=%d estimated_size=%u",
          curr->tid,
          curr->pos,
          curr->estimated_size);
  data->max_estimated_size = curr->estimated_size;
  curr++;

  while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
    if ((size_t)(curr - top) >= size) {
      auto nodes = static_cast<token_candidate_node *>(
        GRN_REALLOC(data->nodes,
                    (curr - top + TOKEN_CANDIDATE_NODE_SIZE) *
                      sizeof(token_candidate_node)));
      if (!nodes) {
        GRN_FREE(data->nodes);
        data->nodes = NULL;
        return GRN_NO_MEMORY_AVAILABLE;
      }
      top = data->nodes;
      curr = top + size;
      size += TOKEN_CANDIDATE_NODE_SIZE;
    }
    tid = grn_token_cursor_next(ctx, token_cursor);
    if (token_cursor->status != GRN_TOKEN_CURSOR_DONE_SKIP) {
      grn_token *token;
      token = grn_token_cursor_get_token(ctx, token_cursor);
      if (grn_token_get_force_prefix_search(ctx, token)) {
        ef |= EX_PREFIX;
      }
      TOKEN_CANDIDATE_NODE_SET();
      token_candidate_adjacent_set(ctx, token_cursor, top, curr);
      if (curr->estimated_size > data->max_estimated_size) {
        data->max_estimated_size = curr->estimated_size;
      }
      curr++;
    }
  }
  data->n_nodes = curr - top;
  return GRN_SUCCESS;
#undef TOKEN_CANDIDATE_NODE_SET
}

static inline grn_rc
token_candidate_queue_init(grn_ctx *ctx, token_candidate_queue *q)
{
  q->top = 0;
  q->rear = 0;
  q->size = TOKEN_CANDIDATE_QUEUE_SIZE;

  q->candidates = static_cast<uint32_t *>(
    GRN_MALLOC(TOKEN_CANDIDATE_QUEUE_SIZE * sizeof(uint32_t)));
  if (!q->candidates) {
    q->size = 0;
    return GRN_NO_MEMORY_AVAILABLE;
  }
  return GRN_SUCCESS;
}

static inline grn_rc
token_candidate_enqueue(grn_ctx *ctx,
                        token_candidate_queue *q,
                        uint32_t candidate)
{
  if (q->rear >= q->size) {
    if (!(q->candidates = static_cast<uint32_t *>(GRN_REALLOC(
            q->candidates,
            (q->rear + TOKEN_CANDIDATE_QUEUE_SIZE) * sizeof(uint32_t))))) {
      q->size = 0;
      return GRN_NO_MEMORY_AVAILABLE;
    }
    q->size += TOKEN_CANDIDATE_QUEUE_SIZE;
  }
  *(q->candidates + q->rear) = candidate;
  q->rear++;
  return GRN_SUCCESS;
}

static inline grn_rc
token_candidate_dequeue(grn_ctx *ctx,
                        token_candidate_queue *q,
                        uint32_t *candidate)
{
  if (q->top == q->rear) {
    return GRN_END_OF_DATA;
  }
  *candidate = *(q->candidates + q->top);
  q->top++;
  return GRN_SUCCESS;
}

static inline void
token_candidate_queue_fin(grn_ctx *ctx, token_candidate_queue *q)
{
  GRN_FREE(q->candidates);
}

static inline token_candidate_node *
token_candidate_last_node(grn_ctx *ctx,
                          token_candidate_data *data,
                          uint32_t candidate,
                          int offset)
{
  grn_bit_scan_rev_result i;
  GRN_BIT_SCAN_REV(candidate, i);
  return data->nodes + i + offset;
}

static inline uint64_t
token_candidate_score(grn_ctx *ctx,
                      token_candidate_data *data,
                      uint32_t candidate,
                      int offset)
{
  grn_bit_scan_rev_result i, last;
  uint64_t score = 0;
  GRN_BIT_SCAN_REV(candidate, last);
  for (i = 0; i <= last; i++) {
    if (candidate & (1 << i)) {
      token_candidate_node *node = data->nodes + i + offset;
      if (node->estimated_size > 0) {
        score += data->max_estimated_size / node->estimated_size;
      }
    }
  }
  return score;
}

static inline grn_rc
token_candidate_select(grn_ctx *ctx,
                       token_candidate_data *data,
                       uint32_t offset,
                       uint32_t limit)
{
  if (offset + limit > (data->n_nodes - 1)) {
    limit = (data->n_nodes - 1) - offset;
  }
  token_candidate_queue q;
  grn_rc rc = token_candidate_queue_init(ctx, &q);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  uint32_t candidate;
  uint64_t max_score = 0;
  uint32_t min_n_nodes = 0;
  rc = token_candidate_enqueue(ctx, &q, 1);
  if (rc != GRN_SUCCESS) {
    goto exit;
  }
  while (token_candidate_dequeue(ctx, &q, &candidate) != GRN_END_OF_DATA) {
    token_candidate_node *candidate_last_node =
      token_candidate_last_node(ctx, data, candidate, offset);
    int i;
    for (i = 0; i < candidate_last_node->n_adjacent; i++) {
      uint32_t adjacent;
      uint32_t new_candidate;
      uint32_t n_nodes = 0;
      adjacent = candidate_last_node->adjacent[i] - offset;
      if (adjacent > limit) {
        break;
      }
      new_candidate = candidate | (1 << adjacent);
      GET_NUM_BITS(new_candidate, n_nodes);
      if (min_n_nodes > 0 && n_nodes > min_n_nodes + 1) {
        goto exit;
      }
      rc = token_candidate_enqueue(ctx, &q, new_candidate);
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
      if (adjacent == limit) {
        if (min_n_nodes == 0) {
          min_n_nodes = n_nodes;
        }
        if (min_n_nodes <= n_nodes && n_nodes <= min_n_nodes + 1) {
          uint64_t score =
            token_candidate_score(ctx, data, new_candidate, offset);
          if (score > max_score) {
            max_score = score;
            data->selected_candidate = new_candidate;
          }
        }
      }
    }
  }
  rc = GRN_SUCCESS;
exit:
  token_candidate_queue_fin(ctx, &q);
  return rc;
}

static inline grn_rc
token_candidate_build(grn_ctx *ctx, token_candidate_data *data, int offset)
{
  grn_rc rc = GRN_END_OF_DATA;
  grn_bit_scan_rev_result last = 0;
  GRN_BIT_SCAN_REV(data->selected_candidate, last);
  grn_bit_scan_rev_result i;
  for (i = 1; i <= last; i++) {
    if (data->selected_candidate & (1 << i)) {
      token_info *ti;
      token_candidate_node *node = data->nodes + i + offset;
      switch (node->status) {
      case GRN_TOKEN_CURSOR_DOING:
        ti = token_info_open(ctx,
                             data->select_data,
                             node->tid,
                             reinterpret_cast<const char *>(node->source_token),
                             node->source_token_size,
                             node->pos,
                             EX_NONE);
        break;
      case GRN_TOKEN_CURSOR_DONE:
        if (node->tid != GRN_ID_NIL) {
          ti =
            token_info_open(ctx,
                            data->select_data,
                            node->tid,
                            reinterpret_cast<const char *>(node->source_token),
                            node->source_token_size,
                            node->pos,
                            node->ef & EX_PREFIX);
          break;
        } /* else fallthru */
      default:
        ti = token_info_open(ctx,
                             data->select_data,
                             node->tid,
                             (const char *)(node->token),
                             node->token_size,
                             node->pos,
                             node->ef & EX_PREFIX);
        break;
      }
      if (!ti) {
        goto exit;
      }
      data->select_data->token_infos[data->select_data->n_token_infos++] = ti;
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "[ii][overlap_token_skip] tid=%u pos=%d estimated_size=%u",
              node->tid,
              node->pos,
              node->estimated_size);
    }
  }
  rc = GRN_SUCCESS;
exit:
  return rc;
}

static inline grn_rc
token_info_build_skipping_overlap(grn_ctx *ctx,
                                  grn_ii_select_data *select_data,
                                  grn_token_cursor *token_cursor,
                                  grn_id tid,
                                  int ef)
{
  token_candidate_data data;
  grn_rc rc =
    token_candidate_init(ctx, &data, select_data, token_cursor, tid, ef);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  uint32_t offset = 0;
  uint32_t limit = TOKEN_CANDIDATE_SIZE - 1;
  while (offset < data.n_nodes - 1) {
    data.selected_candidate = 0;
    rc = token_candidate_select(ctx, &data, offset, limit);
    if (rc != GRN_SUCCESS) {
      goto exit;
    }
    rc = token_candidate_build(ctx, &data, offset);
    if (rc != GRN_SUCCESS) {
      goto exit;
    }
    offset += limit;
  }
  rc = GRN_SUCCESS;
exit:
  if (data.nodes) {
    GRN_FREE(data.nodes);
  }
  return rc;
}

static inline grn_rc
token_info_build_phrase(grn_ctx *ctx,
                        grn_ii_select_data *data,
                        const char *phrase,
                        unsigned int phrase_len)
{
  token_info *ti;
  grn_rc rc = GRN_END_OF_DATA;
  grn_token_cursor *token_cursor =
    grn_ii_select_data_open_token_cursor(ctx,
                                         data,
                                         phrase,
                                         phrase_len,
                                         GRN_TOKENIZE_GET);
  if (!token_cursor) {
    return ctx->rc;
  }
  if (data->mode == GRN_OP_UNSPLIT) {
    if ((ti = token_info_open(ctx,
                              data,
                              GRN_ID_NIL,
                              (char *)token_cursor->orig,
                              token_cursor->orig_blen,
                              0,
                              EX_BOTH))) {
      data->token_infos[data->n_token_infos++] = ti;
      rc = GRN_SUCCESS;
    }
  } else {
    grn_id tid;
    int default_ef = EX_NONE;
    grn_token *token;
    switch (data->mode) {
    case GRN_OP_PREFIX:
      default_ef = EX_PREFIX;
      break;
    case GRN_OP_SUFFIX:
      default_ef = EX_SUFFIX;
      break;
    case GRN_OP_PARTIAL:
      default_ef = EX_BOTH;
      break;
    default:
      default_ef = EX_NONE;
      break;
    }
    tid = grn_token_cursor_next(ctx, token_cursor);
    if (ctx->rc != GRN_SUCCESS) {
      rc = ctx->rc;
      goto exit;
    }
    token = grn_token_cursor_get_token(ctx, token_cursor);
    const unsigned char *key = NULL;
    uint32_t size = 0;
    if (tid == GRN_ID_NIL) {
      key = token_cursor->orig;
      size = token_cursor->orig_blen;
    } else {
      size = grn_token_get_source_length(ctx, token);
      if (size > 0) {
        key = token_cursor->orig + grn_token_get_source_offset(ctx, token);
      }
    }
    int ef = default_ef;
    if (grn_token_get_force_prefix_search(ctx, token)) {
      ef |= EX_PREFIX;
    }
    switch (token_cursor->status) {
    case GRN_TOKEN_CURSOR_DOING:
      ti = token_info_open(ctx,
                           data,
                           tid,
                           reinterpret_cast<const char *>(key),
                           size,
                           token_cursor->pos,
                           ef & EX_SUFFIX);
      break;
    case GRN_TOKEN_CURSOR_DONE:
    case GRN_TOKEN_CURSOR_NOT_FOUND:
      ti = token_info_open(ctx,
                           data,
                           tid,
                           reinterpret_cast<const char *>(key),
                           size,
                           0,
                           ef);
      break;
    case GRN_TOKEN_CURSOR_DONE_SKIP:
      data->only_skip_token = true;
      goto exit;
    default:
      goto exit;
    }
    if (!ti) {
      goto exit;
    }
    data->token_infos[data->n_token_infos++] = ti;

    if (grn_ii_overlap_token_skip_enable) {
      rc = token_info_build_skipping_overlap(ctx, data, token_cursor, tid, ef);
      goto exit;
    }

    while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
      grn_token *token;
      tid = grn_token_cursor_next(ctx, token_cursor);
      token = grn_token_cursor_get_token(ctx, token_cursor);
      const unsigned char *key = NULL;
      uint32_t size = 0;
      if (tid == GRN_ID_NIL) {
        key = token_cursor->curr;
        size = token_cursor->curr_size;
      } else {
        size = grn_token_get_source_length(ctx, token);
        if (size > 0) {
          key = token_cursor->orig + grn_token_get_source_offset(ctx, token);
        }
      }
      ef = default_ef;
      if (grn_token_get_force_prefix_search(ctx, token)) {
        ef |= EX_PREFIX;
      }
      switch (token_cursor->status) {
      case GRN_TOKEN_CURSOR_DONE_SKIP:
        continue;
      case GRN_TOKEN_CURSOR_DOING:
        ti = token_info_open(ctx,
                             data,
                             tid,
                             reinterpret_cast<const char *>(key),
                             size,
                             token_cursor->pos,
                             EX_NONE);
        break;
      default:
        ti = token_info_open(ctx,
                             data,
                             tid,
                             reinterpret_cast<const char *>(key),
                             size,
                             token_cursor->pos,
                             ef & EX_PREFIX);
        break;
      }
      if (!ti) {
        goto exit;
      }
      data->token_infos[data->n_token_infos++] = ti;
    }
    rc = GRN_SUCCESS;
  }
exit:
  grn_token_cursor_close(ctx, token_cursor);
  return rc;
}

static inline grn_rc
token_info_build(grn_ctx *ctx, grn_ii_select_data *data)
{
  data->only_skip_token = false;
  return token_info_build_phrase(ctx, data, data->query, data->query_len);
}

static inline grn_rc
token_info_build_fuzzy(grn_ctx *ctx, grn_ii_select_data *data)
{
  token_info *ti;
  grn_rc rc = GRN_END_OF_DATA;

  if (data->fuzzy_search_optarg->flags & GRN_TABLE_FUZZY_SEARCH_SKIP_TOKENIZE) {
    ti = token_info_open(ctx,
                         data,
                         GRN_ID_NIL,
                         data->query,
                         data->query_len,
                         1,
                         EX_FUZZY);
    if (!ti) {
      return rc;
    }
    data->token_infos[data->n_token_infos++] = ti;
    return GRN_SUCCESS;
  }

  grn_token_cursor *token_cursor =
    grn_ii_select_data_open_token_cursor(ctx,
                                         data,
                                         data->query,
                                         data->query_len,
                                         GRN_TOKENIZE_ONLY);
  data->only_skip_token = false;
  if (!token_cursor) {
    return ctx->rc;
  }
  grn_id tid = grn_token_cursor_next(ctx, token_cursor);
  switch (token_cursor->status) {
  case GRN_TOKEN_CURSOR_DONE_SKIP:
    data->only_skip_token = true;
    goto exit;
  case GRN_TOKEN_CURSOR_DOING:
  case GRN_TOKEN_CURSOR_DONE:
    ti = token_info_open(ctx,
                         data,
                         tid,
                         (const char *)token_cursor->curr,
                         token_cursor->curr_size,
                         token_cursor->pos,
                         EX_FUZZY);
    break;
  default:
    ti = NULL;
    break;
  }
  if (!ti) {
    goto exit;
  }
  data->token_infos[data->n_token_infos++] = ti;
  while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
    tid = grn_token_cursor_next(ctx, token_cursor);
    const unsigned char *key = NULL;
    uint32_t key_size = 0;
    if (tid == GRN_ID_NIL) {
      key = token_cursor->curr;
      key_size = token_cursor->curr_size;
    } else {
      grn_token *token = grn_token_cursor_get_token(ctx, token_cursor);
      key_size = grn_token_get_source_length(ctx, token);
      if (key_size > 0) {
        key = token_cursor->orig + grn_token_get_source_offset(ctx, token);
      }
    }
    switch (token_cursor->status) {
    case GRN_TOKEN_CURSOR_DONE_SKIP:
      continue;
    case GRN_TOKEN_CURSOR_DOING:
    case GRN_TOKEN_CURSOR_DONE:
      ti = token_info_open(ctx,
                           data,
                           tid,
                           reinterpret_cast<const char *>(key),
                           key_size,
                           token_cursor->pos,
                           EX_FUZZY);
      break;
    default:
      break;
    }
    if (!ti) {
      goto exit;
    }
    data->token_infos[data->n_token_infos++] = ti;
  }
  rc = GRN_SUCCESS;
exit:
  grn_token_cursor_close(ctx, token_cursor);
  return rc;
}

static inline bool
token_info_build_near_phrase_is_same_phrase(grn_ctx *ctx,
                                            grn_ii_select_data *data,
                                            uint32_t start1,
                                            uint32_t n1,
                                            uint32_t start2,
                                            uint32_t n2)
{
  uint32_t i1 = 0;
  uint32_t i2 = 0;
  while (i1 < n1 && i2 < n2) {
    token_info *token_info1 = data->token_infos[start1 + i1];
    token_info *token_info2 = data->token_infos[start2 + i2];
    if (token_info1->offset != token_info2->offset) {
      if (token_info1->offset > token_info2->offset) {
        i2++;
      } else {
        i1++;
      }
      continue;
    }
    if (token_info1->must_last != token_info2->must_last) {
      return false;
    }
    cursor_heap *cursors1 = token_info1->cursors;
    cursor_heap *cursors2 = token_info2->cursors;
    if (cursors1->n_entries != cursors2->n_entries) {
      return false;
    }
    int j;
    for (j = 0; j < cursors1->n_entries; j++) {
      if (cursors1->bins[j]->id != cursors2->bins[j]->id) {
        return false;
      }
    }
    i1++;
    i2++;
  }
  return i1 == n1 && i2 == n2;
}

static inline bool
token_info_build_near_phrase_have_same_phrase(grn_ctx *ctx,
                                              grn_ii_select_data *data,
                                              uint32_t n_token_infos_before,
                                              uint32_t phrase_group_id)
{
  uint32_t n_target_tokens = data->n_token_infos - n_token_infos_before;
  uint32_t i = 0;
  token_info *token_info;
  for (i = 0; i < n_token_infos_before;
       i += token_info->n_used_tokens_in_phrase) {
    token_info = data->token_infos[i];
    if (token_info->phrase_group_id != phrase_group_id) {
      continue;
    }

    if (token_info_build_near_phrase_is_same_phrase(
          ctx,
          data,
          i,
          token_info->n_used_tokens_in_phrase,
          n_token_infos_before,
          n_target_tokens)) {
      return true;
    }
  }
  return false;
}

static inline grn_rc
token_info_build_near_phrase(grn_ctx *ctx, grn_ii_select_data *data)
{
  const char *tag = "[ii][token-info][build]";
  grn_rc rc = GRN_SUCCESS;
  data->only_skip_token = false;
  grn_obj buffer;
  GRN_TEXT_INIT(&buffer, 0);
  const char *current = data->query;
  const char *query_end = current + data->query_len;
  const char *last_char = NULL;
  int last_char_len = 0;
  bool in_paren = false;
  uint32_t phrase_group_id = 0;
  uint32_t n_phrases_in_group = 0;
  bool have_no_phrase_group = false;
  uint32_t phrase_id = 0;
  const bool is_product_mode =
    (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
     data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT);

  if (is_product_mode) {
    phrase_group_id--;
  }

  while (current < query_end) {
    int space_char_len = grn_isspace(current, ctx->encoding);
    if (space_char_len > 0) {
      current += space_char_len;
      continue;
    }

    const char *phrase = NULL;
    size_t phrase_len = 0;
    bool use_buffer = false;
    GRN_BULK_REWIND(&buffer);
    bool escaping = false;
    int char_len = grn_charlen(ctx, current, query_end);
    if (char_len == 0) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s invalid character: <%0x>: <%.*s>",
          tag,
          current[0],
          (int)(data->query_len),
          data->query);
      goto exit;
    }

    if (is_product_mode) {
      if (char_len == 1 && current[0] == GRN_QUERY_PARENL) {
        if (in_paren) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s parenthesis must not be nested: <%.*s|%.*s|%.*s>",
              tag,
              (int)(current - data->query),
              data->query,
              char_len,
              current,
              (int)((data->query + data->query_len) - current - char_len),
              current + char_len);
          goto exit;
        }
        in_paren = true;
        phrase_group_id++;
        n_phrases_in_group = 0;
        current += char_len;
        continue;
      } else if (char_len == 1 && current[0] == GRN_QUERY_PARENR) {
        if (!in_paren) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s unbalanced close parenthesis is detected: <%.*s|%.*s|%.*s>",
              tag,
              (int)(current - data->query),
              data->query,
              char_len,
              current,
              (int)((data->query + data->query_len) - current - char_len),
              current + char_len);
          goto exit;
        }
        in_paren = false;
        current += char_len;
        if (n_phrases_in_group == 0) {
          have_no_phrase_group = true;
        }
        continue;
      } else {
        if (!in_paren) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s all phrases must be in parenthesis: <%.*s|%.*s|%.*s>",
              tag,
              (int)(current - data->query),
              data->query,
              char_len,
              current,
              (int)((data->query + data->query_len) - current - char_len),
              current + char_len);
          goto exit;
        }
      }
    }

    if (char_len == 1 && current[0] == GRN_QUERY_QUOTEL) {
      use_buffer = true;
      current += char_len;
      while (current < query_end) {
        char_len = grn_charlen(ctx, current, query_end);
        if (char_len == 0) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s invalid character: <%0x>: <%.*s>",
              tag,
              current[0],
              (int)(data->query_len),
              data->query);
          goto exit;
        }
        if (escaping) {
          escaping = false;
          last_char = NULL;
          last_char_len = 0;
        } else {
          if (char_len == 1) {
            if (current[0] == GRN_QUERY_ESCAPE) {
              escaping = true;
              current += char_len;
              continue;
            } else if (current[0] == GRN_QUERY_QUOTER) {
              current += char_len;
              break;
            }
          }
          last_char = current;
          last_char_len = char_len;
        }
        GRN_TEXT_PUT(ctx, &buffer, current, char_len);
        current += char_len;
      }
      phrase = GRN_TEXT_VALUE(&buffer);
      phrase_len = GRN_TEXT_LEN(&buffer);
    } else {
      phrase = current;
      while (current < query_end) {
        if (!escaping) {
          space_char_len = grn_isspace(current, ctx->encoding);
          if (space_char_len > 0) {
            break;
          }
        }
        char_len = grn_charlen(ctx, current, query_end);
        if (char_len == 0) {
          ERR(GRN_INVALID_ARGUMENT,
              "%s invalid character: <%0x>: <%.*s>",
              tag,
              current[0],
              (int)(data->query_len),
              data->query);
          goto exit;
        }
        if (escaping) {
          escaping = false;
          last_char = NULL;
          last_char_len = 0;
        } else {
          if (char_len == 1) {
            if (current[0] == GRN_QUERY_ESCAPE) {
              if (!use_buffer) {
                use_buffer = true;
                GRN_TEXT_PUT(ctx, &buffer, phrase, current - phrase);
              }
              current += char_len;
              escaping = true;
              continue;
            } else if (current[0] == GRN_QUERY_QUOTEL) {
              break;
            } else if (is_product_mode && (current[0] == GRN_QUERY_PARENL ||
                                           current[0] == GRN_QUERY_PARENR)) {
              break;
            }
          }
          last_char = current;
          last_char_len = char_len;
        }
        if (use_buffer) {
          GRN_TEXT_PUT(ctx, &buffer, current, char_len);
        }
        current += char_len;
      }
      if (use_buffer) {
        phrase = GRN_TEXT_VALUE(&buffer);
        phrase_len = GRN_TEXT_LEN(&buffer);
      } else {
        phrase_len = current - phrase;
      }
    }

    if (phrase_len == 0) {
      continue;
    }

    bool must_last = false;
    if (!(data->mode == GRN_OP_ORDERED_NEAR_PHRASE ||
          data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT)) {
      if (last_char_len == 1 && *last_char == '$') {
        phrase_len -= last_char_len;
        must_last = true;
      }
    }

    uint32_t n_before = data->n_token_infos;
    rc = token_info_build_phrase(ctx, data, phrase, phrase_len);
    if (is_product_mode) {
      if (!(rc == GRN_SUCCESS || rc == GRN_END_OF_DATA)) {
        goto exit;
      }
      if (rc == GRN_END_OF_DATA ||
          token_info_build_near_phrase_have_same_phrase(ctx,
                                                        data,
                                                        n_before,
                                                        phrase_group_id)) {
        rc = GRN_SUCCESS;
        uint32_t i;
        for (i = n_before; i < data->n_token_infos; i++) {
          token_info_close(ctx, data->token_infos[i]);
        }
        data->n_token_infos = n_before;
        continue;
      }
      n_phrases_in_group++;
    } else {
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
    }
    if (data->n_token_infos > n_before) {
      data->token_infos[data->n_token_infos - 1]->must_last = must_last;
    }
    must_last = false;
    uint32_t n_tokens_in_phrase = 0;
    {
      grn_token_cursor *token_cursor =
        grn_ii_select_data_open_token_cursor(ctx,
                                             data,
                                             phrase,
                                             phrase_len,
                                             GRN_TOKENIZE_ONLY);
      if (token_cursor) {
        while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
          grn_token_cursor_next(ctx, token_cursor);
          n_tokens_in_phrase++;
        }
        grn_token_cursor_close(ctx, token_cursor);
      }
    }
    uint32_t i;
    uint32_t n_used_tokens_in_phrase = data->n_token_infos - n_before;
    for (i = n_before; i < data->n_token_infos; i++) {
      data->token_infos[i]->n_tokens_in_phrase = n_tokens_in_phrase;
      data->token_infos[i]->n_used_tokens_in_phrase = n_used_tokens_in_phrase;
      data->token_infos[i]->phrase_group_id = phrase_group_id;
      data->token_infos[i]->phrase_id = phrase_id;
    }
    phrase_id++;
  }

  if (is_product_mode) {
    if (in_paren) {
      ERR(GRN_INVALID_ARGUMENT, "%s close parenthesis is missing", tag);
      goto exit;
    }
    if (have_no_phrase_group) {
      rc = GRN_END_OF_DATA;
      goto exit;
    }
  }

  data->n_phrase_groups = phrase_group_id + 1;
  data->n_phrases = phrase_id;

  if (data->mode == GRN_OP_ORDERED_NEAR_PHRASE) {
    if (data->n_token_infos > 0) {
      data->token_infos[data->n_token_infos - 1]->must_last = true;
    }
  }

exit:
  if (rc == GRN_SUCCESS && ctx->rc != GRN_SUCCESS) {
    rc = ctx->rc;
  }
  GRN_OBJ_FIN(ctx, &buffer);
  return rc;
}

static void
token_info_clear_offset(token_info **tis, uint32_t n)
{
  token_info **tie;
  for (tie = tis + n; tis < tie; tis++) {
    (*tis)->offset = 0;
  }
}

static inline void
bt_zap(btr *bt)
{
  bt->n = 0;
  bt->min = NULL;
  bt->max = NULL;
  bt->root = NULL;
  bt->n_must_lasts = 0;
}

static inline btr *
bt_open(grn_ctx *ctx, int size)
{
  auto bt = static_cast<btr *>(GRN_CALLOC(sizeof(btr)));
  if (bt) {
    bt_zap(bt);
    bt->nodes = static_cast<btr_node *>(GRN_MALLOC(sizeof(btr_node) * size));
    if (!bt->nodes) {
      GRN_FREE(bt);
      bt = NULL;
    }
  }
  return bt;
}

static inline void
bt_close(grn_ctx *ctx, btr *bt)
{
  if (!bt) {
    return;
  }
  GRN_FREE(bt->nodes);
  GRN_FREE(bt);
}

/* Adds a token info to the given tree. btr::min and btr::max are updated
 * are updated when the given token info is minimum or maximum token info
 * in the given tree. */
static inline void
bt_push(btr *bt, token_info *ti)
{
  bool min_p = true;
  bool max_p = true;
  int32_t pos = ti->pos;
  btr_node *node, *new_node, **last;
  new_node = bt->nodes + bt->n++;
  new_node->ti = ti;
  new_node->car = NULL;
  new_node->cdr = NULL;
  for (last = &bt->root; (node = *last);) {
    if (pos < node->ti->pos) {
      last = &node->car;
      max_p = false;
    } else {
      last = &node->cdr;
      min_p = false;
    }
  }
  *last = new_node;
  if (min_p) {
    bt->min = ti;
  }
  if (max_p) {
    bt->max = ti;
  }
  if (ti->must_last) {
    bt->n_must_lasts++;
  }
}

/* Drops the minimum token info. Note that you can't push a token info to
 * this btr after this function is called. */
static inline void
bt_pop(btr *bt)
{
  btr_node *min;
  btr_node *parent = NULL;
  btr_node **last;
  for (last = &bt->root; (min = *last) && min->car;
       parent = min, last = &min->car) {
  }
  if (min) {
    *last = min->cdr;
    if (min->cdr) {
      btr_node *new_min;
      for (new_min = min->cdr; new_min->car; new_min = new_min->car)
        ;
      bt->min = new_min->ti;
    } else if (parent) {
      bt->min = parent->ti;
    } else {
      bt->min = NULL;
    }
    if (last == &(bt->root)) {
      bt->max = bt->min;
    }
    min->cdr = NULL;
    bt->n--;
    if (min->ti->must_last) {
      bt->n_must_lasts--;
    }
  }
}

/* Replace the token info of the minimum node. Note that bt->min may
 * not be correct until you call bt_reorder_min(). */
static inline void
bt_replace_min(btr *bt, token_info *ti)
{
  btr_node *min, **last;
  if (ti->must_last) {
    bt->n_must_lasts++;
  }
  for (last = &bt->root; (min = *last) && min->car; last = &min->car)
    ;
  if (min) {
    if (min->ti->must_last) {
      bt->n_must_lasts--;
    }
    min->ti = ti;
    bt->min = ti;
  }
}

/* Re-order the minimum token info. The minimum token info must be
 * updated before you call this function. */
static inline void
bt_reorder_min(btr *bt)
{
  btr_node *node, *min, **last;
  for (last = &bt->root; (min = *last) && min->car; last = &min->car)
    ;
  if (min) {
    bool min_p = true;
    bool max_p = true;
    int32_t pos = min->ti->pos;
    *last = min->cdr;
    min->cdr = NULL;
    for (last = &bt->root; (node = *last);) {
      if (pos < node->ti->pos) {
        last = &node->car;
        max_p = false;
      } else {
        last = &node->cdr;
        min_p = false;
      }
    }
    *last = min;
    if (max_p) {
      bt->max = min->ti;
    }
    if (!min_p) {
      btr_node *new_min;
      for (new_min = bt->root; new_min->car; new_min = new_min->car)
        ;
      bt->min = new_min->ti;
    }
  }
}

/* select */

static inline void
grn_rset_add_record(grn_ctx *ctx,
                    grn_hash *rset,
                    grn_rset_posinfo *posinfo,
                    double score,
                    grn_operator op)
{
  grn_id id;
  grn_rset_recinfo *recinfo;
  switch (op) {
  case GRN_OP_OR:
    id =
      grn_hash_add(ctx, rset, posinfo, rset->key_size, (void **)&recinfo, NULL);
    if (id != GRN_ID_NIL) {
      if (rset->obj.header.flags & GRN_OBJ_WITH_SUBREC) {
        grn_table_add_subrec(ctx, (grn_obj *)rset, recinfo, score, posinfo, 1);
        grn_selector_data_current_add_score(ctx,
                                            (grn_obj *)rset,
                                            id,
                                            posinfo->rid,
                                            score);
      }
    }
    break;
  case GRN_OP_AND:
    id = grn_hash_get(ctx, rset, posinfo, rset->key_size, (void **)&recinfo);
    if (id != GRN_ID_NIL) {
      if (rset->obj.header.flags & GRN_OBJ_WITH_SUBREC) {
        recinfo->n_subrecs |= GRN_RSET_UTIL_BIT;
        grn_table_add_subrec(ctx, (grn_obj *)rset, recinfo, score, posinfo, 1);
        grn_selector_data_current_add_score(ctx,
                                            (grn_obj *)rset,
                                            id,
                                            posinfo->rid,
                                            score);
      }
    }
    break;
  case GRN_OP_AND_NOT:
    id = grn_hash_get(ctx, rset, posinfo, rset->key_size, (void **)&recinfo);
    if (id != GRN_ID_NIL) {
      grn_hash_delete_by_id(ctx, rset, id, NULL);
    }
    break;
  case GRN_OP_ADJUST:
    id = grn_hash_get(ctx, rset, posinfo, rset->key_size, (void **)&recinfo);
    if (id != GRN_ID_NIL) {
      if (rset->obj.header.flags & GRN_OBJ_WITH_SUBREC) {
        recinfo->score += score;
        grn_selector_data_current_add_score(ctx,
                                            (grn_obj *)rset,
                                            id,
                                            posinfo->rid,
                                            score);
      }
    }
    break;
  default:
    break;
  }
}

/* TODO: It may be better that we record the min ID when add/delete a
 * record. */
/* TODO: It may be useful that we just return approximate value. For
 * example, we can stop min ID search when we find an ID that is less
 * than grn_table_size(ctx, grn_ctx_at(ctx,
 * result_set->header.domain))/100. */
extern "C" grn_id
grn_result_set_get_min_id(grn_ctx *ctx, grn_hash *result_set)
{
  GRN_API_ENTER;
  if (!result_set) {
    GRN_API_RETURN(GRN_ID_NIL);
  }
  if (GRN_HASH_SIZE(result_set) == 0) {
    GRN_API_RETURN(GRN_ID_NIL);
  }
  grn_id min_id = GRN_ID_MAX;
  GRN_HASH_EACH_BEGIN(ctx, result_set, cursor, id)
  {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    grn_id id = *((grn_id *)key);
    if (id < min_id) {
      min_id = id;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  GRN_API_RETURN(min_id);
}

extern "C" grn_rc
grn_result_set_add_record(grn_ctx *ctx,
                          grn_hash *result_set,
                          grn_posting *posting,
                          grn_operator op)
{
  GRN_API_ENTER;
  grn_rset_add_record(ctx,
                      result_set,
                      (grn_rset_posinfo *)posting,
                      ((grn_posting_internal *)posting)->weight_float,
                      op);
  GRN_API_RETURN(ctx->rc);
}

extern "C" grn_rc
grn_result_set_add_table(grn_ctx *ctx,
                         grn_hash *result_set,
                         grn_obj *table,
                         double score,
                         grn_operator op)
{
  GRN_API_ENTER;
  const int flags = GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING;
  grn_table_cursor *cursor =
    grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, flags);
  if (cursor) {
    grn_result_set_add_table_cursor(ctx, result_set, cursor, score, op);
    grn_table_cursor_close(ctx, cursor);
  }
  GRN_API_RETURN(ctx->rc);
}

extern "C" grn_rc
grn_result_set_add_table_cursor(grn_ctx *ctx,
                                grn_hash *result_set,
                                grn_table_cursor *cursor,
                                double score,
                                grn_operator op)
{
  GRN_API_ENTER;
  grn_obj *table = grn_table_cursor_table(ctx, cursor);
  if (result_set->obj.header.domain != DB_OBJ(table)->id) {
    grn_obj result_set_inspected;
    grn_obj table_inspected;
    GRN_TEXT_INIT(&result_set_inspected, 0);
    GRN_TEXT_INIT(&table_inspected, 0);
    grn_inspect_limited(ctx, &result_set_inspected, (grn_obj *)result_set);
    grn_inspect_limited(ctx, &table_inspected, table);
    ERR(GRN_INVALID_ARGUMENT,
        "[result-set][add-table-cursor] "
        "table must be the domain of the result set: %.*s: %.*s",
        (int)GRN_TEXT_LEN(&result_set_inspected),
        GRN_TEXT_VALUE(&result_set_inspected),
        (int)GRN_TEXT_LEN(&table_inspected),
        GRN_TEXT_VALUE(&table_inspected));
    GRN_OBJ_FIN(ctx, &result_set_inspected);
    GRN_OBJ_FIN(ctx, &table_inspected);
    GRN_API_RETURN(ctx->rc);
  }

  grn_rc rc;
  if (op == GRN_OP_OR || op == GRN_OP_AND) {
    rc = grn_hash_add_table_cursor(ctx, result_set, cursor, score, op);
  } else {
    grn_rset_posinfo posinfo = {};
    while ((posinfo.rid = grn_table_cursor_next(ctx, cursor))) {
      grn_rset_add_record(ctx, result_set, &posinfo, score, op);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
    rc = ctx->rc;
  }
  GRN_API_RETURN(rc);
}

extern "C" grn_rc
grn_result_set_add_index_cursor(grn_ctx *ctx,
                                grn_hash *result_set,
                                grn_obj *cursor,
                                double additional_score,
                                double weight,
                                grn_operator op)
{
  GRN_API_ENTER;

  grn_obj *index_column = grn_index_cursor_get_index_column(ctx, cursor);
  if (result_set->obj.header.domain != DB_OBJ(index_column)->range) {
    grn_obj result_set_inspected;
    grn_obj index_column_inspected;
    GRN_TEXT_INIT(&result_set_inspected, 0);
    GRN_TEXT_INIT(&index_column_inspected, 0);
    grn_inspect_limited(ctx, &result_set_inspected, (grn_obj *)result_set);
    grn_inspect_limited(ctx, &index_column_inspected, index_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[result-set][add-index-cursor] "
        "not an index column for the result set: %.*s: %.*s",
        (int)GRN_TEXT_LEN(&result_set_inspected),
        GRN_TEXT_VALUE(&result_set_inspected),
        (int)GRN_TEXT_LEN(&index_column_inspected),
        GRN_TEXT_VALUE(&index_column_inspected));
    GRN_OBJ_FIN(ctx, &result_set_inspected);
    GRN_OBJ_FIN(ctx, &index_column_inspected);
    GRN_API_RETURN(ctx->rc);
  }

  grn_rc rc;
  if (op == GRN_OP_OR || op == GRN_OP_AND) {
    rc = grn_hash_add_index_cursor(ctx,
                                   result_set,
                                   cursor,
                                   additional_score,
                                   weight,
                                   op);
  } else {
    grn_id term_id;
    grn_posting *posting;
    while ((posting = grn_index_cursor_next(ctx, cursor, &term_id))) {
      grn_rset_posinfo posinfo = {
        posting->rid,
        posting->sid,
        posting->pos,
      };
      grn_posting_internal *posting_internal = (grn_posting_internal *)posting;
      double score =
        (posting_internal->weight_float + additional_score) * weight;
      grn_rset_add_record(ctx, result_set, &posinfo, score, op);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
    rc = ctx->rc;
  }
  GRN_API_RETURN(rc);
}

extern "C" grn_rc
grn_result_set_add_ii_cursor(grn_ctx *ctx,
                             grn_hash *result_set,
                             grn_ii_cursor *cursor,
                             double additional_score,
                             double weight,
                             grn_operator op)
{
  GRN_API_ENTER;

  grn_obj *index_column = (grn_obj *)(cursor->ii);
  if (result_set->obj.header.domain != DB_OBJ(index_column)->range) {
    grn_obj result_set_inspected;
    grn_obj index_column_inspected;
    GRN_TEXT_INIT(&result_set_inspected, 0);
    GRN_TEXT_INIT(&index_column_inspected, 0);
    grn_inspect_limited(ctx, &result_set_inspected, (grn_obj *)result_set);
    grn_inspect_limited(ctx, &index_column_inspected, index_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[result-set][add-ii-cursor] "
        "not an index column for the result set: %.*s: %.*s",
        (int)GRN_TEXT_LEN(&result_set_inspected),
        GRN_TEXT_VALUE(&result_set_inspected),
        (int)GRN_TEXT_LEN(&index_column_inspected),
        GRN_TEXT_VALUE(&index_column_inspected));
    GRN_OBJ_FIN(ctx, &result_set_inspected);
    GRN_OBJ_FIN(ctx, &index_column_inspected);
    GRN_API_RETURN(ctx->rc);
  }

  grn_rc rc;
  if (op == GRN_OP_OR || op == GRN_OP_AND) {
    rc = grn_hash_add_ii_cursor(ctx,
                                result_set,
                                cursor,
                                additional_score,
                                weight,
                                op);
  } else {
    grn_posting *posting;
    while ((posting = grn_ii_cursor_next(ctx, cursor))) {
      grn_rset_posinfo posinfo = {
        posting->rid,
        posting->sid,
        posting->pos,
      };
      grn_posting_internal *posting_internal = (grn_posting_internal *)posting;
      double score =
        (posting_internal->weight_float + additional_score) * weight;
      grn_rset_add_record(ctx, result_set, &posinfo, score, op);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
    rc = ctx->rc;
  }
  GRN_API_RETURN(rc);
}

extern "C" grn_rc
grn_result_set_add_ii_select_cursor(grn_ctx *ctx,
                                    grn_hash *result_set,
                                    grn_ii_select_cursor *cursor,
                                    grn_operator op)
{
  GRN_API_ENTER;

  grn_obj *index_column = (grn_obj *)grn_ii_select_cursor_get_ii(ctx, cursor);
  if (result_set->obj.header.domain != DB_OBJ(index_column)->range) {
    grn_obj result_set_inspected;
    grn_obj index_column_inspected;
    GRN_TEXT_INIT(&result_set_inspected, 0);
    GRN_TEXT_INIT(&index_column_inspected, 0);
    grn_inspect_limited(ctx, &result_set_inspected, (grn_obj *)result_set);
    grn_inspect_limited(ctx, &index_column_inspected, index_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[result-set][add-ii-select-cursor] "
        "not an index column for the result set: %.*s: %.*s",
        (int)GRN_TEXT_LEN(&result_set_inspected),
        GRN_TEXT_VALUE(&result_set_inspected),
        (int)GRN_TEXT_LEN(&index_column_inspected),
        GRN_TEXT_VALUE(&index_column_inspected));
    GRN_OBJ_FIN(ctx, &result_set_inspected);
    GRN_OBJ_FIN(ctx, &index_column_inspected);
    GRN_API_RETURN(ctx->rc);
  }

  grn_rc rc;
  if (op == GRN_OP_OR || op == GRN_OP_AND) {
    rc = grn_hash_add_ii_select_cursor(ctx, result_set, cursor, op);
  } else {
    grn_ii_select_cursor_posting *posting;
    while ((posting = grn_ii_select_cursor_next(ctx, cursor))) {
      grn_rset_posinfo posinfo = {
        posting->rid,
        posting->sid,
        posting->pos,
      };
      grn_rset_add_record(ctx, result_set, &posinfo, posting->score, op);
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
    }
    rc = ctx->rc;
  }
  GRN_API_RETURN(rc);
}

extern "C" grn_rc
grn_result_set_copy(grn_ctx *ctx,
                    grn_hash *result_set,
                    grn_hash *output_result_set)
{
  GRN_API_ENTER;

  GRN_HASH_EACH_BEGIN(ctx, result_set, cursor, id)
  {
    void *key = NULL;
    uint32_t key_size = 0;
    void *value = NULL;
    grn_hash_cursor_get_key_value(ctx, cursor, &key, &key_size, &value);
    void *output_value = NULL;
    grn_id id = grn_hash_add(ctx,
                             output_result_set,
                             key,
                             (int)key_size,
                             &output_value,
                             NULL);
    if (id == GRN_ID_NIL) {
      if (ctx->rc == GRN_SUCCESS) {
        continue;
      } else {
        break;
      }
    }
    grn_rset_recinfo *ri = static_cast<grn_rset_recinfo *>(value);
    grn_rset_recinfo *output_ri = static_cast<grn_rset_recinfo *>(output_value);
    grn_memcpy(output_ri, ri, (output_result_set)->value_size);
    output_ri->score = 0;
  }
  GRN_HASH_EACH_END(ctx, cursor);

  GRN_API_RETURN(ctx->rc);
}

static inline void
res_add(grn_ctx *ctx,
        grn_hash *s,
        grn_rset_posinfo *pi,
        double score,
        grn_operator op)
{
  grn_rset_add_record(ctx, s, pi, score, op);
}

extern "C" grn_rc
grn_ii_posting_add(grn_ctx *ctx, grn_posting *pos, grn_hash *s, grn_operator op)
{
  grn_rset_add_record(ctx, s, (grn_rset_posinfo *)(pos), pos->weight, op);
  return ctx->rc;
}

extern "C" grn_rc
grn_ii_posting_add_float(grn_ctx *ctx,
                         grn_posting *pos,
                         grn_hash *s,
                         grn_operator op)
{
  return grn_result_set_add_record(ctx, s, pos, op);
}

static float
get_weight(grn_ctx *ctx, grn_ii_select_data *data)
{
  switch (data->wv_mode) {
  case GRN_WV_NONE:
    return 1;
  case GRN_WV_STATIC:
    if (data->sid == 0) {
      return 0;
    } else if (data->sid <= (uint32_t)(data->optarg->vector_size)) {
      if (data->optarg->weight_vector) {
        return (float)(data->optarg->weight_vector[data->sid - 1]);
      } else {
        return data->optarg->weight_vector_float[data->sid - 1];
      }
    } else {
      return 0;
    }
  case GRN_WV_DYNAMIC:
    /* todo : support hash with keys
    if (s->keys) {
      uint32_t key_size;
      const char *key = _grn_table_key(ctx, s->keys, rid, &key_size);
      // todo : change grn_select_optarg
      return key ? optarg->func(s, key, key_size, sid, optarg->func_arg) : 0;
    }
    */
    /* todo : cast */
    {
      int weight = data->optarg->func(ctx,
                                      data->result_set,
                                      (void *)(intptr_t)(data->rid),
                                      data->sid,
                                      data->optarg->func_arg);
      return (float)weight;
    }
  case GRN_WV_CONSTANT:
    if (data->optarg->vector_size < 0) {
      return data->optarg->weight_float;
    } else {
      return (float)(data->optarg->vector_size);
    }
  default:
    return 1;
  }
}

static void
grn_ii_select_data_init(grn_ctx *ctx,
                        grn_ii_select_data *data,
                        grn_select_optarg *optarg)
{
  data->optarg = optarg;
  data->mode = GRN_OP_EXACT;
  data->wv_mode = GRN_WV_NONE;
  data->score_func = NULL;
  data->previous_min = GRN_ID_NIL;
  data->current_min = GRN_ID_NIL;
  data->set_min_enable_for_and_query = false;
  data->only_skip_token = false;
  data->query_options = NULL;

  data->token_infos = NULL;
  data->n_token_infos = 0;
  data->n_phrase_groups = 0;
  data->phrase_groups = NULL;
  data->n_phrases = 0;

  data->bt = NULL;
  data->check_element_intervals_btree = NULL;

  data->rid = 0;
  data->sid = 0;
  data->next_rid = 0;
  data->next_sid = 0;

  if (!optarg) {
    return;
  }

  data->mode = optarg->mode;

  if (optarg->func) {
    data->wv_mode = GRN_WV_DYNAMIC;
  } else if (optarg->vector_size != 0) {
    if (optarg->weight_vector || optarg->weight_vector_float) {
      data->wv_mode = GRN_WV_STATIC;
    } else {
      data->wv_mode = GRN_WV_CONSTANT;
    }
  }

  if (optarg->match_info) {
    if (optarg->match_info->flags & GRN_MATCH_INFO_GET_MIN_RECORD_ID) {
      data->previous_min = optarg->match_info->min;
      data->set_min_enable_for_and_query = true;
    }
  }

  if (optarg->scorer) {
    grn_proc *scorer = (grn_proc *)(optarg->scorer);
    data->score_func = scorer->callbacks.scorer.score;
    data->record.table = grn_ctx_at(ctx, data->result_set->obj.header.domain);
    data->record.lexicon = data->lexicon;
    data->record.id = GRN_ID_NIL;
    GRN_RECORD_INIT(&(data->record.terms),
                    GRN_OBJ_VECTOR,
                    data->lexicon->header.domain);
    GRN_UINT32_INIT(&(data->record.term_weights), GRN_OBJ_VECTOR);
    data->record.total_term_weights = 0;
    data->record.n_documents = grn_table_size(ctx, data->record.table);
    data->record.n_occurrences = 0;
    data->record.n_candidates = 0;
    data->record.n_tokens = 0;
    data->record.weight = 0;
    data->record.args_expr = optarg->scorer_args_expr;
    data->record.args_expr_offset = optarg->scorer_args_expr_offset;
  }

  data->fuzzy_search_optarg = &(optarg->fuzzy);
  data->query_options = optarg->query_options;
}

static bool
grn_ii_select_data_init_token_infos(grn_ctx *ctx,
                                    grn_ii_select_data *data,
                                    const char *tag)
{
  if (data->query_len == 0) {
    return false;
  }

  data->token_infos = static_cast<token_info **>(
    GRN_MALLOC(sizeof(token_info *) * data->query_len * 2));
  if (!data->token_infos) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc, "%s failed to allocate token info container: %s", tag, errbuf);
    return false;
  }

  if (data->mode == GRN_OP_FUZZY) {
    grn_rc rc = token_info_build_fuzzy(ctx, data);
    if (rc != GRN_SUCCESS) {
      return false;
    }
    if (data->n_token_infos == 0) {
      return false;
    }
  } else if (data->mode == GRN_OP_NEAR_PHRASE ||
             data->mode == GRN_OP_ORDERED_NEAR_PHRASE ||
             data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
             data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
    grn_rc rc = token_info_build_near_phrase(ctx, data);
    if (rc != GRN_SUCCESS) {
      return false;
    }
  } else {
    grn_rc rc = token_info_build(ctx, data);
    if (rc != GRN_SUCCESS) {
      return false;
    }
  }
  if (data->n_token_infos == 0) {
    return false;
  }
  P_NOTE("original token infos\n");
  P_TIS(ctx, data->token_infos, data->n_token_infos);

  if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
      data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
    data->phrase_groups = static_cast<phrase_group *>(
      GRN_MALLOC(sizeof(phrase_group) * data->n_phrase_groups));
    if (!data->phrase_groups) {
      return false;
    }
    uint32_t i;
    for (i = 0; i < data->n_phrase_groups; i++) {
      uint32_t i_token_info;
      uint32_t current_phrase_id = (uint32_t)-1;
      uint32_t n_phrases = 0;
      for (i_token_info = 0; i_token_info < data->n_token_infos;
           i_token_info++) {
        const token_info *token_info = data->token_infos[i_token_info];
        if (token_info->phrase_id != current_phrase_id &&
            data->token_infos[i_token_info]->phrase_group_id == i) {
          n_phrases++;
        }
        current_phrase_id = token_info->phrase_id;
      }
      data->phrase_groups[i].n_phrases = n_phrases;
      data->phrase_groups[i].btree = bt_open(ctx, n_phrases);
    }
  } else {
    data->phrase_groups = NULL;
  }

  if (data->mode == GRN_OP_NEAR_NO_OFFSET) {
    token_info_clear_offset(data->token_infos, data->n_token_infos);
    data->mode = GRN_OP_NEAR;
  }
  switch (data->mode) {
  case GRN_OP_NEAR:
  case GRN_OP_NEAR_PHRASE:
  case GRN_OP_ORDERED_NEAR_PHRASE:
  case GRN_OP_NEAR_PHRASE_PRODUCT:
  case GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT:
    {
      size_t btree_size;
      if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
          data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
        btree_size = data->n_phrase_groups;
      } else {
        btree_size = data->n_token_infos;
      }
      data->bt = bt_open(ctx, btree_size);
      if (!data->bt) {
        char errbuf[GRN_CTX_MSGSIZE];
        grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
        ERR(ctx->rc, "%s failed to allocate btree: %s", tag, errbuf);
        return false;
      }
      data->max_interval = data->optarg->max_interval;
      data->additional_last_interval = data->optarg->additional_last_interval;
      data->max_element_intervals = data->optarg->max_element_intervals;
      data->min_interval = data->optarg->min_interval;
      if ((data->mode == GRN_OP_NEAR || data->mode == GRN_OP_NEAR_PHRASE ||
           data->mode == GRN_OP_NEAR_PHRASE_PRODUCT) &&
          data->max_element_intervals &&
          GRN_INT32_VECTOR_SIZE(data->max_element_intervals) > 0) {
        data->check_element_intervals_btree = bt_open(ctx, btree_size);
        if (!data->check_element_intervals_btree) {
          char errbuf[GRN_CTX_MSGSIZE];
          grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
          ERR(ctx->rc,
              "%s failed to allocate btree for checking element intervals: %s",
              tag,
              errbuf);
          return false;
        }
      }
    }
    break;
  default:
    break;
  }
  qsort(data->token_infos,
        data->n_token_infos,
        sizeof(token_info *),
        token_compare);
  P_NOTE("sorted token infos\n");
  P_TIS(ctx, data->token_infos, data->n_token_infos);
  if (data->mode == GRN_OP_NEAR_PHRASE) {
    bool have_must_last = false;
    uint32_t i;
    for (i = 0; i < data->n_token_infos; i++) {
      if (data->token_infos[i]->must_last) {
        if (have_must_last) {
          /* Multiple last tokens query are never matched. */
          return false;
        }
        have_must_last = true;
      }
    }
  }
  return true;
}

static void
grn_ii_select_data_fin(grn_ctx *ctx, grn_ii_select_data *data)
{
  if (data->score_func) {
    GRN_OBJ_FIN(ctx, &(data->record.terms));
    GRN_OBJ_FIN(ctx, &(data->record.term_weights));
  }

  if (data->set_min_enable_for_and_query && !data->only_skip_token) {
    if (data->current_min > data->previous_min) {
      data->optarg->match_info->min = data->current_min;
    }
  }

  if (data->only_skip_token && data->optarg && data->optarg->match_info) {
    data->optarg->match_info->flags |= GRN_MATCH_INFO_ONLY_SKIP_TOKEN;
  }

  if (data->phrase_groups) {
    uint32_t i;
    for (i = 0; i < data->n_phrase_groups; i++) {
      bt_close(ctx, data->phrase_groups[i].btree);
    }
    GRN_FREE(data->phrase_groups);
  }

  if (data->token_infos) {
    token_info **token_info;
    for (token_info = data->token_infos;
         token_info < data->token_infos + data->n_token_infos;
         token_info++) {
      if (*token_info) {
        token_info_close(ctx, *token_info);
      }
    }
    GRN_FREE(data->token_infos);
  }

  if (data->bt) {
    bt_close(ctx, data->bt);
  }

  if (data->check_element_intervals_btree) {
    bt_close(ctx, data->check_element_intervals_btree);
  }
}

static grn_rc
grn_ii_similar_search_internal(grn_ctx *ctx, grn_ii_select_data *data)
{
  int *w1;
  uint32_t limit;
  grn_id tid, *tp, max_size;
  grn_rc rc = GRN_SUCCESS;
  grn_hash *h;
  grn_token_cursor *token_cursor;
  if (data->query_len == 0) {
    return GRN_SUCCESS;
  }
  if (!data->lexicon || !data->query || !data->query_len || !data->result_set ||
      !data->optarg) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!(h = grn_hash_create(ctx, NULL, sizeof(grn_id), sizeof(int), 0))) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  token_cursor = grn_ii_select_data_open_token_cursor(ctx,
                                                      data,
                                                      data->query,
                                                      data->query_len,
                                                      GRN_TOKENIZE_GET);
  if (!token_cursor) {
    grn_hash_close(ctx, h);
    return ctx->rc;
  }
  if (!(max_size = data->optarg->max_size)) {
    max_size = 1048576;
  }
  while (token_cursor->status != GRN_TOKEN_CURSOR_DONE &&
         token_cursor->status != GRN_TOKEN_CURSOR_DONE_SKIP) {
    if ((tid = grn_token_cursor_next(ctx, token_cursor))) {
      if (grn_hash_add(ctx, h, &tid, sizeof(grn_id), (void **)&w1, NULL)) {
        (*w1)++;
      }
    }
    if (tid && token_cursor->curr_size) {
      if (data->mode == GRN_OP_UNSPLIT) {
        grn_table_search(ctx,
                         data->lexicon,
                         token_cursor->curr,
                         token_cursor->curr_size,
                         GRN_OP_PREFIX,
                         (grn_obj *)h,
                         GRN_OP_OR);
      }
      if (data->mode == GRN_OP_PARTIAL) {
        grn_table_search(ctx,
                         data->lexicon,
                         token_cursor->curr,
                         token_cursor->curr_size,
                         GRN_OP_SUFFIX,
                         (grn_obj *)h,
                         GRN_OP_OR);
      }
    }
  }
  grn_token_cursor_close(ctx, token_cursor);
  {
    grn_hash_cursor *c =
      grn_hash_cursor_open(ctx, h, NULL, 0, NULL, 0, 0, -1, 0);
    if (!c) {
      GRN_LOG(ctx,
              GRN_LOG_ALERT,
              "grn_hash_cursor_open on grn_ii_similar_search failed !");
      grn_hash_close(ctx, h);
      return GRN_NO_MEMORY_AVAILABLE;
    }
    while (grn_hash_cursor_next(ctx, c)) {
      uint32_t es;
      grn_hash_cursor_get_key_value(ctx, c, (void **)&tp, NULL, (void **)&w1);
      if ((es = grn_ii_estimate_size(ctx, data->ii, *tp))) {
        *w1 += max_size / es;
      } else {
        grn_hash_cursor_delete(ctx, c, NULL);
      }
    }
    grn_hash_cursor_close(ctx, c);
  }
  limit = data->optarg->similarity_threshold
            ? ((uint32_t)(data->optarg->similarity_threshold) > GRN_HASH_SIZE(h)
                 ? GRN_HASH_SIZE(h)
                 : (uint32_t)(data->optarg->similarity_threshold))
            : (GRN_HASH_SIZE(h) >> 3) + 1;
  if (GRN_HASH_SIZE(h)) {
    grn_id j, id;
    float w2;
    int rep;
    grn_ii_cursor *c;
    grn_posting *pos;
    grn_table_sort_optarg arg = {GRN_TABLE_SORT_DESC | GRN_TABLE_SORT_BY_VALUE |
                                   GRN_TABLE_SORT_AS_NUMBER,
                                 NULL,
                                 NULL,
                                 NULL,
                                 0};
    grn_array *sorted = grn_array_create(ctx, NULL, sizeof(grn_id), 0);
    if (!sorted) {
      GRN_LOG(ctx,
              GRN_LOG_ALERT,
              "grn_hash_sort on grn_ii_similar_search failed !");
      grn_hash_close(ctx, h);
      return GRN_NO_MEMORY_AVAILABLE;
    }
    grn_hash_sort(ctx, h, limit, sorted, &arg);
    /* todo support subrec
    rep = (s->record_unit == GRN_REC_POSITION || s->subrec_unit ==
    GRN_REC_POSITION);
    */
    rep = 0;
    for (j = 1; j <= limit; j++) {
      grn_array_get_value(ctx, sorted, j, &id);
      _grn_hash_get_key_value(ctx, h, id, (void **)&tp, (void **)&w1);
      if (!*tp || !(c = grn_ii_cursor_open(ctx,
                                           data->ii,
                                           *tp,
                                           GRN_ID_NIL,
                                           GRN_ID_MAX,
                                           rep ? data->ii->n_elements
                                               : data->ii->n_elements - 1,
                                           0))) {
        GRN_LOG(ctx, GRN_LOG_ERROR, "cursor open failed (%d)", *tp);
        continue;
      }
      if (rep) {
        while (grn_ii_cursor_next(ctx, c)) {
          pos = c->post;
          data->rid = pos->rid;
          data->sid = pos->sid;
          if ((w2 = get_weight(ctx, data)) > 0) {
            while (grn_ii_cursor_next_pos(ctx, c)) {
              float pos_weight = grn_posting_get_weight_float(ctx, pos);
              res_add(ctx,
                      data->result_set,
                      (grn_rset_posinfo *)pos,
                      *w1 * w2 * (1 + pos_weight),
                      data->op);
            }
          }
        }
      } else {
        while (grn_ii_cursor_next(ctx, c)) {
          pos = c->post;
          data->rid = pos->rid;
          data->sid = pos->sid;
          if ((w2 = get_weight(ctx, data)) > 0) {
            float pos_weight = grn_posting_get_weight_float(ctx, pos);
            res_add(ctx,
                    data->result_set,
                    (grn_rset_posinfo *)pos,
                    *w1 * w2 * (pos->tf + pos_weight),
                    data->op);
          }
        }
      }
      grn_ii_cursor_close(ctx, c);
    }
    grn_array_close(ctx, sorted);
  }
  grn_hash_close(ctx, h);
  grn_ii_resolve_sel_and(ctx, data->result_set, data->op);
  //  grn_hash_cursor_clear(r);
  return rc;
}

extern "C" grn_rc
grn_ii_similar_search(grn_ctx *ctx,
                      grn_ii *ii,
                      const char *string,
                      unsigned int string_len,
                      grn_hash *s,
                      grn_operator op,
                      grn_select_optarg *optarg)
{
  grn_ii_select_data data;
  data.ii = ii;
  data.lexicon = data.ii->lexicon;
  data.query = string;
  data.query_len = string_len;
  data.result_set = s;
  data.op = op;
  grn_ii_select_data_init(ctx, &data, optarg);
  return grn_ii_similar_search_internal(ctx, &data);
}

#define TERM_EXTRACT_EACH_POST 0
#define TERM_EXTRACT_EACH_TERM 1

static grn_rc
grn_ii_term_extract_internal(grn_ctx *ctx, grn_ii_select_data *data)
{
  grn_rset_posinfo pi;
  grn_id tid;
  const char *p, *pe;
  grn_obj *nstr;
  const char *normalized;
  unsigned int normalized_length_in_bytes;
  grn_ii_cursor *c;
  grn_posting *pos;
  int skip, rep, policy;
  grn_rc rc = GRN_SUCCESS;
  if (data->query_len == 0) {
    return GRN_SUCCESS;
  }
  if (!data->ii || !data->query || !data->query_len || !data->result_set ||
      !data->optarg) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!(nstr = grn_string_open(ctx, data->query, data->query_len, NULL, 0))) {
    return GRN_INVALID_ARGUMENT;
  }
  policy = data->optarg->max_interval;
  /* todo support subrec
  if (policy == TERM_EXTRACT_EACH_POST) {
    if ((rc = grn_records_reopen(s, GRN_REC_SECTION, GRN_REC_NONE, 0))) { goto
  exit; }
  }
  rep = (s->record_unit == GRN_REC_POSITION || s->subrec_unit ==
  GRN_REC_POSITION);
  */
  rep = 0;
  grn_string_get_normalized(ctx,
                            nstr,
                            &normalized,
                            &normalized_length_in_bytes,
                            NULL);
  for (p = normalized, pe = p + normalized_length_in_bytes; p < pe; p += skip) {
    if ((tid = grn_table_lcp_search(ctx, data->ii->lexicon, p, pe - p))) {
      if (policy == TERM_EXTRACT_EACH_POST) {
        if (!(skip = grn_table_get_key(ctx, data->ii->lexicon, tid, NULL, 0))) {
          break;
        }
      } else {
        if (!(skip = (int)grn_charlen(ctx, p, pe))) {
          break;
        }
      }
      if (!(c = grn_ii_cursor_open(ctx,
                                   data->ii,
                                   tid,
                                   GRN_ID_NIL,
                                   GRN_ID_MAX,
                                   rep ? data->ii->n_elements
                                       : data->ii->n_elements - 1,
                                   0))) {
        GRN_LOG(ctx, GRN_LOG_ERROR, "cursor open failed (%d)", tid);
        continue;
      }
      if (rep) {
        while (grn_ii_cursor_next(ctx, c)) {
          pos = c->post;
          data->rid = pos->rid;
          data->sid = pos->sid;
          while (grn_ii_cursor_next_pos(ctx, c)) {
            res_add(ctx,
                    data->result_set,
                    (grn_rset_posinfo *)pos,
                    get_weight(ctx, data),
                    data->op);
          }
        }
      } else {
        while (grn_ii_cursor_next(ctx, c)) {
          if (policy == TERM_EXTRACT_EACH_POST) {
            pi.rid = c->post->rid;
            pi.sid = p - normalized;
            res_add(ctx, data->result_set, &pi, pi.sid + 1, data->op);
          } else {
            pos = c->post;
            data->rid = pos->rid;
            data->sid = pos->sid;
            res_add(ctx,
                    data->result_set,
                    (grn_rset_posinfo *)pos,
                    get_weight(ctx, data),
                    data->op);
          }
        }
      }
      grn_ii_cursor_close(ctx, c);
    } else {
      if (!(skip = (int)grn_charlen(ctx, p, pe))) {
        break;
      }
    }
  }
  grn_obj_close(ctx, nstr);
  return rc;
}

extern "C" grn_rc
grn_ii_term_extract(grn_ctx *ctx,
                    grn_ii *ii,
                    const char *string,
                    unsigned int string_len,
                    grn_hash *s,
                    grn_operator op,
                    grn_select_optarg *optarg)
{
  grn_ii_select_data data;
  data.ii = ii;
  data.lexicon = data.ii->lexicon;
  data.query = string;
  data.query_len = string_len;
  data.result_set = s;
  data.op = op;
  grn_ii_select_data_init(ctx, &data, optarg);
  return grn_ii_term_extract_internal(ctx, &data);
}

struct grn_ii_select_cursor_ {
  grn_ii_select_data data;
  bool per_occurrence_mode;
  grn_ii_select_cursor_posting posting;
  bool done;
  grn_ii_select_cursor_posting unshifted_posting;
  bool have_unshifted_posting;
};

static void
grn_ii_select_cursor_init_internal(grn_ctx *ctx, grn_ii_select_cursor *cursor)
{
  cursor->per_occurrence_mode = false;
  cursor->done = false;
  cursor->have_unshifted_posting = false;
}

static grn_rc
grn_ii_select_cursor_close(grn_ctx *ctx, grn_ii_select_cursor *cursor)
{
  if (!cursor) {
    return GRN_SUCCESS;
  }

  grn_ii_select_data_fin(ctx, &(cursor->data));
  GRN_FREE(cursor);

  return GRN_SUCCESS;
}

static grn_ii_select_cursor *
grn_ii_select_cursor_open(grn_ctx *ctx,
                          grn_ii *ii,
                          const char *string,
                          unsigned int string_len,
                          grn_select_optarg *optarg)
{
  const char *tag = "[ii][select][cursor][open]";

  if (string_len == 0) {
    ERR(GRN_INVALID_ARGUMENT, "%s empty string", tag);
    return NULL;
  }

  auto cursor = static_cast<grn_ii_select_cursor *>(
    GRN_CALLOC(sizeof(grn_ii_select_cursor)));
  if (!cursor) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc, "%s failed to allocate cursor: %s", tag, errbuf);
    return NULL;
  }

  cursor->data.ii = ii;
  cursor->data.lexicon = cursor->data.ii->lexicon;
  cursor->data.query = string;
  cursor->data.query_len = string_len;
  cursor->data.result_set = NULL;
  cursor->data.op = GRN_OP_OR;
  grn_ii_select_data_init(ctx, &(cursor->data), optarg);
  switch (cursor->data.mode) {
  case GRN_OP_EXACT:
  case GRN_OP_FUZZY:
  case GRN_OP_NEAR:
  case GRN_OP_NEAR_NO_OFFSET:
    break;
  default:
    ERR(GRN_INVALID_ARGUMENT,
        "%s EXACT, FUZZY, NEAR and NEAR_NO_OFFSET are only supported mode: %s",
        tag,
        grn_operator_to_string(cursor->data.mode));
    grn_ii_select_cursor_close(ctx, cursor);
    return NULL;
  }

  if (!grn_ii_select_data_init_token_infos(ctx, &(cursor->data), tag)) {
    grn_ii_select_cursor_close(ctx, cursor);
    return NULL;
  }
  if (cursor->data.n_token_infos == 0) {
    grn_ii_select_cursor_close(ctx, cursor);
    return NULL;
  }

  GRN_LOG(ctx,
          GRN_LOG_INFO,
          "%s n=%d <%.*s>",
          tag,
          cursor->data.n_token_infos,
          string_len,
          string);

  grn_ii_select_cursor_init_internal(ctx, cursor);

  return cursor;
}

static void
grn_ii_select_cursor_set_per_occurrence_mode(grn_ctx *ctx,
                                             grn_ii_select_cursor *cursor,
                                             bool mode)
{
  cursor->per_occurrence_mode = mode;
}

static inline bool
grn_ii_select_skip_pos(grn_ctx *ctx, grn_ii_select_data *data, int32_t pos)
{
  if (token_info_skip_pos(ctx,
                          data->current_token_info,
                          data->rid,
                          data->sid,
                          pos) != GRN_SUCCESS) {
    return false;
  }
  if (data->current_token_info->p->rid != data->rid ||
      data->current_token_info->p->sid != data->sid) {
    if (!(data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
          data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT)) {
      data->next_rid = data->current_token_info->p->rid;
      data->next_sid = data->current_token_info->p->sid;
    }
    return false;
  }
  return true;
}

/* This function must return a "must_last" token info if it exists.
 * btr::n_must_lasts is the number of "must_last" token infos and
 * we use btr::n_must_lasts as the number of phrases that must be
 * last. So if this function doesn't return a "must_last" token info,
 * we misjudge the number of phrases that must be last.
 */
static inline bool
grn_ii_select_data_find_phrase(grn_ctx *ctx,
                               grn_ii_select_data *data,
                               uint32_t phrase_id,
                               int32_t start_pos,
                               uint32_t start_n_tokens_in_phrase)
{
  token_info **tip;
  token_info **tis = data->token_infos;
  token_info **tie = tis + data->n_token_infos;
  token_info **ti_last = NULL;
  token_info *must_last_ti = NULL;
  uint32_t n_tokens_in_same_pos = 0;
  for (tip = tis;; tip++) {
    if (tip == tie) {
      tip = tis;
    }
    if (!ti_last) {
      ti_last = tip;
    } else if (ti_last == tip) {
      n_tokens_in_same_pos = 0;
      start_pos++;
    }
    data->current_token_info = *tip;
    if (data->current_token_info->phrase_id != phrase_id) {
      continue;
    }
    if (data->current_token_info->must_last) {
      must_last_ti = data->current_token_info;
    }
    if (!grn_ii_select_skip_pos(ctx, data, start_pos)) {
      return false;
    }
    if (data->current_token_info->pos == start_pos &&
        data->current_token_info->n_tokens_in_phrase >=
          start_n_tokens_in_phrase) {
      n_tokens_in_same_pos++;
    } else {
      n_tokens_in_same_pos = 1;
      if (data->current_token_info->pos > start_pos) {
        start_n_tokens_in_phrase = 0;
        ti_last = tip;
      }
      start_pos = data->current_token_info->pos;
    }
    if (n_tokens_in_same_pos ==
          data->current_token_info->n_used_tokens_in_phrase &&
        data->current_token_info->n_tokens_in_phrase >=
          start_n_tokens_in_phrase) {
      if (must_last_ti) {
        data->current_token_info = must_last_ti;
      }
      P_NOTE("%s: found: record:%u phrase:%u/%u\n",
             __func__,
             data->rid,
             data->current_token_info->phrase_id,
             data->current_token_info->phrase_group_id);
      P_TI(ctx, data->current_token_info);
      return true;
    }
  }
  return false;
}

static inline bool
grn_ii_select_data_find_phrase_product(grn_ctx *ctx,
                                       grn_ii_select_data *data,
                                       int32_t start_pos,
                                       uint32_t start_n_tokens_in_phrase)
{
  uint32_t phrase_group_id = data->current_token_info->phrase_group_id;
  phrase_group *group = &(data->phrase_groups[phrase_group_id]);
  bool skipped = false;
  token_info *min_before = group->btree->min;
  while (true) {
    data->current_token_info = group->btree->min;
    if (grn_ii_select_data_find_phrase(ctx,
                                       data,
                                       data->current_token_info->phrase_id,
                                       start_pos,
                                       start_n_tokens_in_phrase)) {
      skipped = true;
      bt_reorder_min(group->btree);
      token_info *min = group->btree->min;
      if (min == min_before) {
        break;
      }
      if (min->pos == min_before->pos &&
          min->n_tokens_in_phrase > min_before->n_tokens_in_phrase) {
        break;
      }
      min_before = min;
    } else {
      bt_pop(group->btree);
      if (!group->btree->min) {
        break;
      }
    }
  }
  if (!skipped) {
    return false;
  }
  P_NOTE("%s: found: record:%u\n", __func__, data->rid);
  P_TI(ctx, group->btree->min);
  bt_replace_min(data->bt, group->btree->min);
  return true;
}

static inline bool
grn_ii_select_data_is_matched_near_phrase_real(grn_ctx *ctx,
                                               grn_ii_select_data *data,
                                               int interval)
{
  if (interval < data->min_interval) {
    return false;
  }
  if (data->max_interval < 0) {
    return true;
  }
  if (data->additional_last_interval == 0) {
    return (interval <= data->max_interval);
  }

  token_info *min_token_info = NULL;
  int min_without_last_token = -1;
  int max_without_last_token = -1;
  if (data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
    uint32_t i;
    uint32_t n = data->n_phrase_groups;
    for (i = 0; i < n - 1; i++) {
      token_info *ti = data->phrase_groups[i].btree->min;
      if (min_without_last_token == -1 || ti->pos < min_without_last_token) {
        min_without_last_token = ti->pos;
        min_token_info = ti;
      }
      if (max_without_last_token == -1 || ti->pos > max_without_last_token) {
        max_without_last_token = ti->pos;
      }
    }
  } else if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT) {
    uint32_t i;
    uint32_t n = data->n_phrase_groups;
    for (i = 0; i < n; i++) {
      token_info *ti = data->phrase_groups[i].btree->min;
      if (ti->must_last) {
        continue;
      }
      if (min_without_last_token == -1 || ti->pos < min_without_last_token) {
        min_without_last_token = ti->pos;
        min_token_info = ti;
      }
      if (max_without_last_token == -1 || ti->pos > max_without_last_token) {
        max_without_last_token = ti->pos;
      }
    }
  } else {
    uint32_t i;
    uint32_t n = data->n_token_infos;
    for (i = 0; i < n; i++) {
      token_info *ti = data->token_infos[i];
      if (ti->must_last) {
        continue;
      }
      if (min_without_last_token == -1 || ti->pos < min_without_last_token) {
        min_without_last_token = ti->pos;
        min_token_info = ti;
      }
      if (max_without_last_token == -1 || ti->pos > max_without_last_token) {
        max_without_last_token = ti->pos;
      }
    }
  }
  if (!min_token_info) {
    return true;
  }

  int interval_without_last_token = max_without_last_token -
                                    min_without_last_token -
                                    min_token_info->n_tokens_in_phrase;
  if (data->additional_last_interval < 0) {
    return (interval_without_last_token <= data->max_interval);
  } else {
    return (
      (interval <= (data->max_interval + data->additional_last_interval)) &&
      (interval_without_last_token <= data->max_interval));
  }
}

static inline bool
grn_ii_select_data_is_matched_near_phrase(grn_ctx *ctx,
                                          grn_ii_select_data *data,
                                          int interval)
{
  bool matched =
    grn_ii_select_data_is_matched_near_phrase_real(ctx, data, interval);
  P_NOTE("%s: %s: record:%u interval:%d min:%d max:%d/%d\n",
         __func__,
         matched ? "matched" : "not matched",
         data->rid,
         interval,
         data->min_interval,
         data->max_interval,
         data->additional_last_interval);
  return matched;
}

static inline bool
grn_ii_select_data_check_near_element_intervals(grn_ctx *ctx,
                                                grn_ii_select_data *data)
{
  if (!data->max_element_intervals) {
    return true;
  }

  uint32_t n_max_element_intervals =
    GRN_INT32_VECTOR_SIZE(data->max_element_intervals);
  if (n_max_element_intervals == 0) {
    return true;
  }

  if (data->mode == GRN_OP_NEAR || data->mode == GRN_OP_NEAR_PHRASE ||
      data->mode == GRN_OP_NEAR_PHRASE_PRODUCT) {
    uint32_t i;
    uint32_t n = data->bt->n;
    bt_zap(data->check_element_intervals_btree);
    for (i = 0; i < n; i++) {
      bt_push(data->check_element_intervals_btree, data->bt->nodes[i].ti);
    }
    if (n > n_max_element_intervals + 1) {
      n = n_max_element_intervals + 1;
    }
    int32_t previous_pos = data->check_element_intervals_btree->min->pos;
    int32_t previous_n_tokens_in_phrase =
      data->check_element_intervals_btree->min->n_tokens_in_phrase;
    for (i = 1; i < n; i++) {
      bt_pop(data->check_element_intervals_btree);
      token_info *min = data->check_element_intervals_btree->min;
      int32_t pos = min->pos;
      int32_t max_element_interval =
        GRN_INT32_VALUE_AT(data->max_element_intervals, i - 1);
      int32_t interval = pos - previous_pos - previous_n_tokens_in_phrase;
      if (max_element_interval >= 0 && interval > max_element_interval) {
        return false;
      }
      if (interval < data->min_interval) {
        return false;
      }
      previous_pos = pos;
      previous_n_tokens_in_phrase = min->n_tokens_in_phrase;
    }
    return true;
  } else if (data->mode == GRN_OP_ORDERED_NEAR_PHRASE) {
    uint32_t i_token_info;
    uint32_t n_token_infos = data->n_token_infos;
    uint32_t i_interval;
    uint32_t n_intervals = data->n_phrases - 1;
    if (n_intervals > n_max_element_intervals) {
      n_intervals = n_max_element_intervals;
    }
    uint32_t previous_phrase_id = data->token_infos[0]->phrase_id;
    int32_t previous_pos = data->token_infos[0]->pos;
    int32_t previous_n_tokens_in_phrase =
      data->token_infos[0]->n_tokens_in_phrase;
    for (i_token_info = 1, i_interval = 0;
         i_token_info < n_token_infos && i_interval < n_intervals;
         i_token_info++) {
      token_info *token_info = data->token_infos[i_token_info];
      if (token_info->phrase_id == previous_phrase_id) {
        continue;
      }
      previous_phrase_id = token_info->phrase_id;
      int32_t pos = token_info->pos;
      int32_t max_element_interval =
        GRN_INT32_VALUE_AT(data->max_element_intervals, i_interval);
      int32_t interval = pos - previous_pos - previous_n_tokens_in_phrase;
      if (max_element_interval >= 0 && interval > max_element_interval) {
        return false;
      }
      if (interval < data->min_interval) {
        return false;
      }
      previous_pos = pos;
      previous_n_tokens_in_phrase = token_info->n_tokens_in_phrase;
      i_interval++;
    }
    return true;
  } else if (data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
    uint32_t i;
    uint32_t n = data->n_phrase_groups;
    if (n > n_max_element_intervals + 1) {
      n = n_max_element_intervals + 1;
    }
    int32_t previous_pos = data->phrase_groups[0].btree->min->pos;
    int32_t previous_n_tokens_in_phrase =
      data->phrase_groups[0].btree->min->n_tokens_in_phrase;
    for (i = 1; i < n; i++) {
      token_info *min = data->phrase_groups[i].btree->min;
      int32_t pos = min->pos;
      int32_t max_element_interval =
        GRN_INT32_VALUE_AT(data->max_element_intervals, i - 1);
      int32_t interval = pos - previous_pos - previous_n_tokens_in_phrase;
      if (max_element_interval >= 0 && interval > max_element_interval) {
        return false;
      }
      if (interval < data->min_interval) {
        return false;
      }
      previous_pos = pos;
      previous_n_tokens_in_phrase = min->n_tokens_in_phrase;
    }
    return true;
  } else {
    return false;
  }
}

static inline bool
grn_ii_select_cursor_next_prepare(grn_ctx *ctx, grn_ii_select_cursor *cursor)
{
  grn_ii_select_data *data = &(cursor->data);
  uint32_t i;
  uint32_t n = data->n_token_infos;
  if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
      data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
    uint32_t min_rid = 0;
    uint32_t min_sid = 0;
    for (i = 0; i < n; i++) {
      token_info *ti = data->token_infos[i];
      if (ti->phrase_group_id == 0) {
        if ((ti->p->rid < data->next_rid) ||
            (ti->p->rid == data->next_rid && ti->p->sid < data->next_sid)) {
          continue;
        }
        if (min_rid == 0 || ti->p->rid < min_rid ||
            (ti->p->rid == min_rid && ti->p->sid < min_sid)) {
          min_rid = ti->p->rid;
          min_sid = ti->p->sid;
        }
      } else {
        token_info_skip(ctx, ti, min_rid, min_sid);
      }
    }
    data->rid = min_rid;
    data->sid = min_sid;
    data->next_rid = data->rid;
    data->next_sid = data->sid + 1;
  } else {
    data->rid = data->token_infos[0]->p->rid;
    data->sid = data->token_infos[0]->p->sid;
    data->next_rid = data->rid;
    data->next_sid = data->sid + 1;
    for (i = 1; i < n; i++) {
      data->current_token_info = data->token_infos[i];
      if (token_info_skip(ctx,
                          data->current_token_info,
                          data->rid,
                          data->sid) != GRN_SUCCESS) {
        return false;
      }
      if (data->current_token_info->p->rid != data->rid ||
          data->current_token_info->p->sid != data->sid) {
        data->next_rid = data->current_token_info->p->rid;
        data->next_sid = data->current_token_info->p->sid;
        break;
      }
    }
    data->may_match = (i == data->n_token_infos);
  }
  return true;
}

static inline bool
grn_ii_select_cursor_next_skip(grn_ctx *ctx, grn_ii_select_cursor *cursor)
{
  grn_ii_select_data *data = &(cursor->data);
  if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
      data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
    bool skipped = false;
    uint32_t i;
    uint32_t n = data->n_token_infos;
    for (i = 0; i < n; i++) {
      token_info *ti = data->token_infos[i];
      if (ti->phrase_group_id != 0) {
        break;
      }
      if (token_info_skip(ctx, ti, data->next_rid, data->next_sid) ==
          GRN_SUCCESS) {
        skipped = true;
      }
    }
    return skipped;
  } else {
    return token_info_skip(ctx,
                           data->token_infos[0],
                           data->next_rid,
                           data->next_sid) == GRN_SUCCESS;
  }
}

static inline bool
grn_ii_select_cursor_next_find_one_token(grn_ctx *ctx,
                                         grn_ii_select_cursor *cursor,
                                         const char *tag)
{
  grn_ii_select_data *data = &(cursor->data);
  token_info *ti = data->token_infos[0];
  data->start_pos = data->end_pos = data->pos = ti->p->pos;
  if (cursor->per_occurrence_mode) {
    data->n_occurrences = 1;
  } else {
    data->n_occurrences = ti->p->tf;
  }
  data->total_score = ti->p->weight + ti->cursors->bins[0]->weight;
  if (data->score_func) {
    GRN_RECORD_PUT(ctx, &(data->record.terms), ti->cursors->bins[0]->id);
    GRN_UINT32_PUT(ctx, &(data->record.term_weights), data->total_score);
    data->record.n_occurrences = data->n_occurrences;
    data->record.n_candidates = ti->size;
    data->record.n_tokens = ti->ntoken;
  }
  return true;
}

static inline bool
grn_ii_select_cursor_next_find_near(grn_ctx *ctx,
                                    grn_ii_select_cursor *cursor,
                                    const char *tag)
{
  grn_ii_select_data *data = &(cursor->data);
  bt_zap(data->bt);
  bool need_check = true;
  if (data->mode == GRN_OP_NEAR_PHRASE ||
      data->mode == GRN_OP_ORDERED_NEAR_PHRASE) {
    uint32_t phrase_id;
    for (phrase_id = 0; phrase_id < data->n_phrases; phrase_id++) {
      if (!grn_ii_select_data_find_phrase(ctx, data, phrase_id, data->pos, 0)) {
        need_check = false;
        break;
      }
      /* TODO: Can we update data->pos to reduce needless search? */
      bt_push(data->bt, data->current_token_info);
    }
  } else if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
             data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
    uint32_t i;
    uint32_t phrase_id = 0;
    for (i = 0; i < data->n_phrase_groups; i++) {
      phrase_group *group = &(data->phrase_groups[i]);
      bt_zap(group->btree);
      uint32_t end_phrase_id = phrase_id + group->n_phrases;
      bool have_phrase = false;
      for (; phrase_id < end_phrase_id; phrase_id++) {
        if (grn_ii_select_data_find_phrase(ctx,
                                           data,
                                           phrase_id,
                                           data->pos,
                                           0)) {
          have_phrase = true;
          bt_push(group->btree, data->current_token_info);
          /* TODO: Can we update data->pos to reduce needless search? */
        }
      }
      if (!have_phrase) {
        need_check = false;
        break;
      }
      bt_push(data->bt, group->btree->min);
    }
  } else {
    uint32_t i;
    uint32_t n = data->n_token_infos;
    for (i = 0; i < n; i++) {
      token_info *ti = data->token_infos[i];
      bt_push(data->bt, ti);
    }
  }
  if (!need_check) {
    return true;
  }

  for (;;) {
    data->current_token_info = data->bt->min;
    token_info *min_token_info = data->bt->min;
    token_info *max_token_info = data->bt->max;
    int32_t min = min_token_info->pos;
    uint32_t min_n_tokens_in_phrase = min_token_info->n_tokens_in_phrase;
    int32_t max = max_token_info->pos;
    if (min > max) {
      char ii_name[GRN_TABLE_MAX_KEY_SIZE];
      int ii_name_size;
      ii_name_size = grn_obj_name(ctx,
                                  (grn_obj *)(data->ii),
                                  ii_name,
                                  GRN_TABLE_MAX_KEY_SIZE);
      ERR(GRN_FILE_CORRUPT,
          "%s[%s] "
          "max position must be larger than min position: "
          "min:<%d> max:<%d> ii:<%.*s> string:<%.*s>",
          tag,
          grn_operator_to_string(data->mode),
          min,
          max,
          ii_name_size,
          ii_name,
          data->query_len,
          data->query);
      return false;
    }
    int32_t interval = max - min;
    if (data->mode == GRN_OP_NEAR_PHRASE ||
        data->mode == GRN_OP_ORDERED_NEAR_PHRASE ||
        data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
        data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
      interval -= data->current_token_info->n_tokens_in_phrase;
    }
    P_NOTE("%s record:%u interval:%d min:%d max:%d\n",
           __func__,
           data->rid,
           interval,
           min,
           max);
    P_TI(ctx, min_token_info);
    P_TI(ctx, max_token_info);
    bool matched = false;
    if (data->mode == GRN_OP_ORDERED_NEAR_PHRASE) {
      matched = true;
      uint32_t i;
      uint32_t n = data->n_token_infos;
      int32_t pos = data->token_infos[0]->pos;
      for (i = 1; i < n; i++) {
        token_info *ti = data->token_infos[i];
        if (ti->pos < pos) {
          matched = false;
          break;
        }
        pos = ti->pos;
      }
      if (matched) {
        matched =
          grn_ii_select_data_is_matched_near_phrase(ctx, data, interval);
      }
    } else if (data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
      matched = true;
      uint32_t i;
      uint32_t n = data->n_phrase_groups;
      int32_t pos = data->phrase_groups[0].btree->min->pos;
      for (i = 1; i < n; i++) {
        token_info *ti = data->phrase_groups[i].btree->min;
        if (ti->pos < pos) {
          matched = false;
          break;
        }
        pos = ti->pos;
      }
      if (matched) {
        matched =
          grn_ii_select_data_is_matched_near_phrase(ctx, data, interval);
      }
    } else if (data->bt->n_must_lasts > 0) {
      matched =
        (data->bt->n_must_lasts == 1 && data->bt->max->must_last &&
         grn_ii_select_data_is_matched_near_phrase(ctx, data, interval));
    } else {
      matched = ((data->max_interval < 0 || interval <= data->max_interval) &&
                 interval >= data->min_interval);
    }
    if (matched) {
      matched = grn_ii_select_data_check_near_element_intervals(ctx, data);
    }
    if (matched) {
      data->n_occurrences++;
      int32_t next_pos = max + 1;
      if (data->mode == GRN_OP_NEAR_PHRASE ||
          data->mode == GRN_OP_ORDERED_NEAR_PHRASE) {
        if (!grn_ii_select_data_find_phrase(ctx,
                                            data,
                                            data->current_token_info->phrase_id,
                                            next_pos,
                                            0)) {
          break;
        }
      } else if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
                 data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
        if (!grn_ii_select_data_find_phrase_product(ctx, data, next_pos, 0)) {
          break;
        }
      } else {
        if (!grn_ii_select_skip_pos(ctx, data, next_pos)) {
          break;
        }
      }
    } else {
      if (data->mode == GRN_OP_NEAR_PHRASE ||
          data->mode == GRN_OP_ORDERED_NEAR_PHRASE) {
        int32_t next_pos = min + 1;
        if (!grn_ii_select_data_find_phrase(ctx,
                                            data,
                                            data->current_token_info->phrase_id,
                                            next_pos,
                                            0)) {
          break;
        }
      } else if (data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
                 data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
        uint32_t next_tokens_in_phrase = min_token_info->n_tokens_in_phrase + 1;
        if (!grn_ii_select_data_find_phrase_product(ctx,
                                                    data,
                                                    min,
                                                    next_tokens_in_phrase)) {
          break;
        }
      } else {
        int32_t next_pos = max - data->max_interval;
        if (next_pos <= min) {
          next_pos = min + 1;
        }
        if (!grn_ii_select_skip_pos(ctx, data, next_pos)) {
          break;
        }
      }
    }
    bt_reorder_min(data->bt);
    if ((data->mode == GRN_OP_NEAR_PHRASE ||
         data->mode == GRN_OP_ORDERED_NEAR_PHRASE ||
         data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
         data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) &&
        data->bt->min->pos == min &&
        data->bt->min->n_tokens_in_phrase == min_n_tokens_in_phrase) {
      break;
    }
  }
  return true;
}

static inline bool
grn_ii_select_cursor_next_find(grn_ctx *ctx,
                               grn_ii_select_cursor *cursor,
                               const char *tag)
{
  grn_ii_select_data *data = &(cursor->data);
  uint32_t count = 0;
  int32_t score = 0;
  token_info **tis = data->token_infos;
  token_info **tie = data->token_infos + data->n_token_infos;
  token_info **tip;
  for (tip = tis;; tip++) {
    if (tip == tie) {
      tip = tis;
    }
    data->current_token_info = *tip;
    if (!grn_ii_select_skip_pos(ctx, data, data->pos)) {
      break;
    }
    if (data->current_token_info->pos == (int32_t)(data->pos)) {
      if (count == 0) {
        data->start_pos = data->pos;
      }
      score += data->current_token_info->p->weight +
               data->current_token_info->cursors->bins[0]->weight;
      count++;
      if ((int32_t)(data->current_token_info->p->pos) > data->end_pos) {
        data->end_pos = data->current_token_info->p->pos;
      }
    } else {
      score = data->current_token_info->p->weight +
              data->current_token_info->cursors->bins[0]->weight;
      count = 1;
      data->start_pos = data->pos = data->current_token_info->pos;
      data->end_pos = data->current_token_info->p->pos;
      if (data->n_occurrences == 0 && data->score_func) {
        GRN_BULK_REWIND(&(data->record.terms));
        GRN_BULK_REWIND(&(data->record.term_weights));
        data->record.n_candidates = 0;
        data->record.n_tokens = 0;
      }
    }
    if (data->n_occurrences == 0 && data->score_func) {
      GRN_RECORD_PUT(ctx,
                     &(data->record.terms),
                     data->current_token_info->cursors->bins[0]->id);
      GRN_UINT32_PUT(ctx,
                     &(data->record.term_weights),
                     data->current_token_info->p->weight +
                       data->current_token_info->cursors->bins[0]->weight);
      data->record.n_candidates += data->current_token_info->size;
      data->record.n_tokens += data->current_token_info->ntoken;
    }
    if (count == data->n_token_infos) {
      if ((int32_t)(data->current_token_info->p->pos) > data->end_pos) {
        data->end_pos = data->current_token_info->p->pos;
      }
      data->total_score += score;
      score = 0;
      count = 0;
      data->pos++;
      data->n_occurrences++;
      if (cursor->per_occurrence_mode) {
        break;
      }
    }
  }
  return true;
}

extern "C" grn_ii_select_cursor_posting *
grn_ii_select_cursor_next(grn_ctx *ctx, grn_ii_select_cursor *cursor)
{
  const char *tag = "[ii][select][cursor][next]";

  if (cursor->have_unshifted_posting) {
    cursor->have_unshifted_posting = false;
    return &(cursor->unshifted_posting);
  }

  if (cursor->done) {
    return NULL;
  }

  grn_ii_select_data *data = &(cursor->data);
  for (;;) {
    data->may_match = true;
    if (!grn_ii_select_cursor_next_prepare(ctx, cursor)) {
      goto exit;
    }
    float weight = get_weight(ctx, data);
    if (data->may_match && fpclassify(weight) == FP_ZERO) {
      data->may_match = false;
    }
    if (data->may_match) {
      if (data->op != GRN_OP_OR) {
        if (data->result_set) {
          grn_rset_posinfo pi = {data->rid, data->sid, 0};
          grn_id id = grn_hash_get(ctx,
                                   data->result_set,
                                   &pi,
                                   data->result_set->key_size,
                                   NULL);
          data->may_match = (id != GRN_ID_NIL);
        }
      }
    }
    if (!data->may_match) {
      if (!grn_ii_select_cursor_next_skip(ctx, cursor)) {
        goto exit;
      }
      continue;
    }

    data->start_pos = 0;
    data->end_pos = 0;
    data->total_score = 0;
    data->n_occurrences = 0;

    data->pos = 0;
    if (data->score_func) {
      GRN_BULK_REWIND(&(data->record.terms));
      GRN_BULK_REWIND(&(data->record.term_weights));
      data->record.n_candidates = 0;
      data->record.n_tokens = 0;
    }

    if (data->n_token_infos == 1) {
      if (!grn_ii_select_cursor_next_find_one_token(ctx, cursor, tag)) {
        goto exit;
      }
    } else if (data->mode == GRN_OP_NEAR || data->mode == GRN_OP_NEAR_PHRASE ||
               data->mode == GRN_OP_ORDERED_NEAR_PHRASE ||
               data->mode == GRN_OP_NEAR_PHRASE_PRODUCT ||
               data->mode == GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT) {
      if (!grn_ii_select_cursor_next_find_near(ctx, cursor, tag)) {
        goto exit;
      }
    } else {
      if (!grn_ii_select_cursor_next_find(ctx, cursor, tag)) {
        goto exit;
      }
    }

    if (data->n_occurrences == 0) {
      if (!grn_ii_select_cursor_next_skip(ctx, cursor)) {
        goto exit;
      }
      continue;
    }

    double record_score;
    if (data->score_func) {
      data->record.id = data->rid;
      data->record.weight = (int)weight;
      data->record.n_occurrences = data->n_occurrences;
      data->record.total_term_weights = data->total_score;
      record_score = data->score_func(ctx, &(data->record)) * (double)weight;
    } else {
      record_score = (data->n_occurrences + data->total_score) * (double)weight;
    }
    if (data->set_min_enable_for_and_query) {
      if (data->current_min == GRN_ID_NIL) {
        data->current_min = data->rid;
      }
    }

    cursor->posting.rid = data->rid;
    cursor->posting.sid = data->sid;
    cursor->posting.pos = data->pos;
    cursor->posting.start_pos = data->start_pos;
    cursor->posting.end_pos = data->end_pos;
    cursor->posting.tf = data->n_occurrences;
    cursor->posting.score = record_score;
    if (cursor->per_occurrence_mode) {
      if (token_info_skip_pos(ctx,
                              data->token_infos[0],
                              data->rid,
                              data->sid,
                              data->end_pos + 1) != GRN_SUCCESS) {
        cursor->done = true;
      }
    } else {
      if (!grn_ii_select_cursor_next_skip(ctx, cursor)) {
        cursor->done = true;
      }
    }
    return &(cursor->posting);
  }

exit:
  cursor->done = true;
  return NULL;
}

static void
grn_ii_select_cursor_unshift(grn_ctx *ctx,
                             grn_ii_select_cursor *cursor,
                             grn_ii_select_cursor_posting *posting)
{
  cursor->unshifted_posting = *posting;
  cursor->have_unshifted_posting = true;
}

extern "C" grn_ii *
grn_ii_select_cursor_get_ii(grn_ctx *ctx, grn_ii_select_cursor *cursor)
{
  return cursor->data.ii;
}

static void
grn_ii_parse_regexp_query_flush(grn_ctx *ctx,
                                grn_obj *parsed_strings,
                                grn_obj *parsed_n_following_characters,
                                grn_obj *string,
                                uint32_t *n_following_characters_start,
                                uint32_t *n_following_characters_end)
{
  if (GRN_TEXT_LEN(string) > 0) {
    grn_vector_add_element(ctx,
                           parsed_strings,
                           GRN_TEXT_VALUE(string),
                           GRN_TEXT_LEN(string),
                           0,
                           GRN_DB_TEXT);
    GRN_BULK_REWIND(string);
    GRN_UINT32_PUT(ctx,
                   parsed_n_following_characters,
                   *n_following_characters_start);
    GRN_UINT32_PUT(ctx,
                   parsed_n_following_characters,
                   *n_following_characters_end);
  }
  *n_following_characters_start = 0;
  *n_following_characters_end = 0;
}

static grn_rc
grn_ii_parse_regexp_query(grn_ctx *ctx,
                          const char *log_tag,
                          const char *string,
                          unsigned int string_len,
                          grn_obj *parsed_strings,
                          grn_obj *parsed_n_following_characters)
{
  bool escaping = false;
  bool in_paren = false;
  const char *all_off_options = "?-mix:";
  size_t all_off_options_len = strlen(all_off_options);
  int nth_char = 0;
  const char *current = string;
  const char *string_end = string + string_len;
  grn_obj buffer;
  uint32_t n_following_characters_start = 0;
  uint32_t n_following_characters_end = 0;

  GRN_TEXT_INIT(&buffer, 0);
  while (current < string_end) {
    const char *target;
    int char_len;

    char_len = grn_charlen(ctx, current, string_end);
    if (char_len == 0) {
      GRN_OBJ_FIN(ctx, &buffer);
      ERR(GRN_INVALID_ARGUMENT,
          "%s invalid encoding character: <%.*s|%#x|>",
          log_tag,
          (int)(current - string),
          string,
          *current);
      return ctx->rc;
    }
    target = current;
    current += char_len;

    if (escaping) {
      escaping = false;
      if (char_len == 1) {
        switch (*target) {
        case 'A':
          if (nth_char == 0) {
            target = GRN_TOKENIZER_BEGIN_MARK_UTF8;
            char_len = GRN_TOKENIZER_BEGIN_MARK_UTF8_LEN;
          }
          break;
        case 'z':
          if (current == string_end ||
              (in_paren && grn_charlen(ctx, current, string_end) == 1 &&
               *current == ')' && (current + 1) == string_end)) {
            target = GRN_TOKENIZER_END_MARK_UTF8;
            char_len = GRN_TOKENIZER_END_MARK_UTF8_LEN;
          }
          break;
        default:
          break;
        }
      }
    } else {
      if (n_following_characters_start > 0 &&
          (!(char_len == 1 && (*target == '.' || *target == '*')))) {
        grn_ii_parse_regexp_query_flush(ctx,
                                        parsed_strings,
                                        parsed_n_following_characters,
                                        &buffer,
                                        &n_following_characters_start,
                                        &n_following_characters_end);
      }

      if (char_len == 1) {
        if (*target == '\\') {
          escaping = true;
          continue;
        } else if (*target == '(' &&
                   (size_t)(string_end - current) >= all_off_options_len &&
                   memcmp(current, all_off_options, all_off_options_len) == 0) {
          current += all_off_options_len;
          in_paren = true;
          continue;
        } else if (*target == ')') {
          in_paren = false;
          continue;
        } else if (*target == '.') {
          n_following_characters_start++;
          n_following_characters_end++;
          continue;
        } else if (*target == '*') {
          /* ".*" is only supported for now. */
          if (n_following_characters_start > 0) {
            n_following_characters_start--;
            n_following_characters_end--;
          }
          grn_ii_parse_regexp_query_flush(ctx,
                                          parsed_strings,
                                          parsed_n_following_characters,
                                          &buffer,
                                          &n_following_characters_start,
                                          &n_following_characters_end);
          continue;
        }
      }
    }

    GRN_TEXT_PUT(ctx, &buffer, target, char_len);
    nth_char++;
  }
  grn_ii_parse_regexp_query_flush(ctx,
                                  parsed_strings,
                                  parsed_n_following_characters,
                                  &buffer,
                                  &n_following_characters_start,
                                  &n_following_characters_end);
  size_t parsed_n_following_characters_size =
    GRN_UINT32_VECTOR_SIZE(parsed_n_following_characters);
  if (parsed_n_following_characters_size > 0 &&
      (GRN_UINT32_VALUE_AT(parsed_n_following_characters,
                           parsed_n_following_characters_size - 1) > 0)) {
    /* 0 means no limit for now. It may be changed when we implement full
     * the number of following characters range support. */
    GRN_UINT32_VALUE_AT(parsed_n_following_characters,
                        parsed_n_following_characters_size - 1) = 0;
    GRN_TEXT_SET(ctx,
                 &buffer,
                 GRN_TOKENIZER_END_MARK_UTF8,
                 GRN_TOKENIZER_END_MARK_UTF8_LEN);
    grn_ii_parse_regexp_query_flush(ctx,
                                    parsed_strings,
                                    parsed_n_following_characters,
                                    &buffer,
                                    &n_following_characters_start,
                                    &n_following_characters_end);
  }
  GRN_OBJ_FIN(ctx, &buffer);

  return GRN_SUCCESS;
}

typedef struct {
  grn_ii_select_cursor *cursor;
  uint32_t n_following_characters_start;
  uint32_t n_following_characters_end;
  uint32_t next_start_pos_offset;
} grn_ii_select_regexp_chunk;

static grn_rc
grn_ii_select_regexp(grn_ctx *ctx,
                     grn_ii *ii,
                     const char *string,
                     unsigned int string_len,
                     grn_hash *s,
                     grn_operator op,
                     grn_select_optarg *optarg)
{
  grn_rc rc;
  grn_obj parsed_strings;
  grn_obj parsed_n_following_characters;
  unsigned int n_parsed_strings;

  if (string_len == 0) {
    return GRN_SUCCESS;
  }

  GRN_TEXT_INIT(&parsed_strings, GRN_OBJ_VECTOR);
  GRN_UINT32_INIT(&parsed_n_following_characters, GRN_OBJ_VECTOR);
  rc = grn_ii_parse_regexp_query(ctx,
                                 "[ii][select][regexp]",
                                 string,
                                 string_len,
                                 &parsed_strings,
                                 &parsed_n_following_characters);
  if (rc != GRN_SUCCESS) {
    GRN_OBJ_FIN(ctx, &parsed_strings);
    return rc;
  }

  if (optarg) {
    optarg->mode = GRN_OP_EXACT;
  }

  n_parsed_strings = grn_vector_size(ctx, &parsed_strings);
  if (n_parsed_strings == 1 &&
      GRN_UINT32_VALUE_AT(&parsed_n_following_characters, 0) == 0) {
    const char *parsed_string;
    unsigned int parsed_string_len;
    parsed_string_len = grn_vector_get_element(ctx,
                                               &parsed_strings,
                                               0,
                                               &parsed_string,
                                               NULL,
                                               NULL);
    rc =
      grn_ii_select(ctx, ii, parsed_string, parsed_string_len, s, op, optarg);
  } else {
    unsigned int i;
    bool have_error = false;

    auto chunks = static_cast<grn_ii_select_regexp_chunk *>(
      GRN_CALLOC(sizeof(grn_ii_select_regexp_chunk) * n_parsed_strings));
    for (i = 0; i < n_parsed_strings; i++) {
      const char *parsed_string;
      unsigned int parsed_string_len;
      parsed_string_len = grn_vector_get_element(ctx,
                                                 &parsed_strings,
                                                 i,
                                                 &parsed_string,
                                                 NULL,
                                                 NULL);
      chunks[i].cursor = grn_ii_select_cursor_open(ctx,
                                                   ii,
                                                   parsed_string,
                                                   parsed_string_len,
                                                   optarg);
      if (!chunks[i].cursor) {
        have_error = true;
        break;
      }
      grn_ii_select_cursor_set_per_occurrence_mode(ctx, chunks[i].cursor, true);
      chunks[i].n_following_characters_start =
        GRN_UINT32_VALUE_AT(&parsed_n_following_characters, i * 2);
      chunks[i].n_following_characters_end =
        GRN_UINT32_VALUE_AT(&parsed_n_following_characters, i * 2 + 1);
      uint32_t next_start_pos_offset = chunks[i].n_following_characters_start;
      if (chunks[i].n_following_characters_start > 0) {
        size_t n_characters = 0;
        const char *current = parsed_string;
        const char *parsed_string_end = parsed_string + parsed_string_len;
        while (current < parsed_string_end) {
          int char_len = grn_charlen(ctx, current, parsed_string_end);
          if (char_len == 0) { /* must not happen */
            break;
          }
          current += char_len;
          n_characters++;
        }
        if (n_characters > 1) {
          /* text: "abcd", query: "a." case:
           *   tokens: "ab", "bc", "cd", "d"
           *   n_characters: 1
           *   n_following_characters: 1
           *   There are no token that is ignored after the "ab" token.
           *
           * text: "abcd", query: "ab." case:
           *   tokens: "ab", "bc", "cd", "d"
           *   n_characters: 2
           *   n_following_characters: 1
           *   The "bc" token must be ignored because the cursor returns
           *   the position for the "ab" token.
           */
          next_start_pos_offset++;
        }
        next_start_pos_offset++;
      }
      chunks[i].next_start_pos_offset = next_start_pos_offset;
    }

    while (!have_error) {
      grn_ii_select_cursor_posting *posting;

      posting = grn_ii_select_cursor_next(ctx, chunks[0].cursor);
      if (!posting) {
        break;
      }

      uint32_t pos = posting->end_pos;
      uint32_t n_following_characters_start =
        chunks[0].n_following_characters_start;
      uint32_t n_following_characters_end =
        chunks[0].n_following_characters_end;
      uint32_t next_start_pos = pos + chunks[0].next_start_pos_offset;
      for (i = 1; i < n_parsed_strings; i++) {
        grn_ii_select_cursor_posting *posting_i;

        for (;;) {
          posting_i = grn_ii_select_cursor_next(ctx, chunks[i].cursor);
          if (!posting_i) {
            break;
          }

          if (posting_i->rid > posting->rid) {
            grn_ii_select_cursor_unshift(ctx, chunks[i].cursor, posting_i);
            break;
          }
          if (posting_i->sid > posting->sid) {
            grn_ii_select_cursor_unshift(ctx, chunks[i].cursor, posting_i);
            break;
          }
          if (posting_i->rid == posting->rid &&
              posting_i->sid == posting->sid) {
            if (n_following_characters_start == 0) {
              if (posting_i->start_pos > pos) {
                grn_ii_select_cursor_unshift(ctx, chunks[i].cursor, posting_i);
                break;
              }
            } else {
              if (posting_i->start_pos >= next_start_pos) {
                grn_ii_select_cursor_unshift(ctx, chunks[i].cursor, posting_i);
                break;
              }
            }
          }
        }

        if (!posting_i) {
          break;
        }
        if (posting_i->rid != posting->rid) {
          break;
        }
        if (posting_i->sid != posting->sid) {
          break;
        }
        if (n_following_characters_start > 0) {
          if (n_following_characters_end == 0) { /* 0 means no limit for now. */
            if (posting_i->start_pos < next_start_pos) {
              break;
            }
          } else {
            if (posting_i->start_pos != next_start_pos) {
              break;
            }
          }
        }

        pos = posting_i->end_pos;
        n_following_characters_start = chunks[i].n_following_characters_start;
        n_following_characters_end = chunks[i].n_following_characters_end;
        next_start_pos = pos + chunks[i].next_start_pos_offset;
      }

      if (i == n_parsed_strings && n_following_characters_start == 0) {
        grn_rset_posinfo pi = {posting->rid, posting->sid, pos};
        double record_score = 1.0;
        res_add(ctx, s, &pi, record_score, op);
        if (ctx->rc != GRN_SUCCESS) {
          rc = ctx->rc;
          break;
        }
      }
    }

    for (i = 0; i < n_parsed_strings; i++) {
      if (chunks[i].cursor) {
        grn_ii_select_cursor_close(ctx, chunks[i].cursor);
      }
    }
    GRN_FREE(chunks);
  }
  GRN_OBJ_FIN(ctx, &parsed_strings);
  GRN_OBJ_FIN(ctx, &parsed_n_following_characters);

  if (optarg) {
    optarg->mode = GRN_OP_REGEXP;
  }

  return rc;
}

typedef struct {
  uint32_t n_occurs;
  double score;
} grn_ii_quorum_match_record_data;

extern "C" grn_rc
grn_ii_quorum_match(grn_ctx *ctx, grn_ii *ii, grn_ii_select_data *data)
{
  size_t record_key_size;
  grn_hash *n_occurs;
  grn_token_cursor *token_cursor;
  int ii_cursor_flags = 0;

  record_key_size = sizeof(grn_id);
  if (ii->header.common->flags & GRN_OBJ_WITH_SECTION) {
    record_key_size += sizeof(uint32_t);
    ii_cursor_flags |= GRN_OBJ_WITH_SECTION;
  }

  n_occurs = grn_hash_create(ctx,
                             NULL,
                             record_key_size,
                             sizeof(grn_ii_quorum_match_record_data),
                             0);
  if (!n_occurs) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  token_cursor = grn_ii_select_data_open_token_cursor(ctx,
                                                      data,
                                                      data->query,
                                                      data->query_len,
                                                      GRN_TOKENIZE_GET);
  if (!token_cursor) {
    grn_rc rc = ctx->rc;
    char ii_name[GRN_TABLE_MAX_KEY_SIZE];
    int ii_name_len;
    char errbuf[GRN_CTX_MSGSIZE];

    if (rc == GRN_SUCCESS) {
      rc = GRN_UNKNOWN_ERROR;
    }
    ii_name_len =
      grn_obj_name(ctx, (grn_obj *)ii, ii_name, GRN_TABLE_MAX_KEY_SIZE);
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(rc,
        "[ii][quorum-match] failed to create token cursor: <%.*s>: <%.*s>%s%s",
        ii_name_len,
        ii_name,
        data->query_len,
        data->query,
        errbuf[0] ? ": " : "",
        errbuf[0] ? errbuf : "");
    grn_hash_close(ctx, n_occurs);
    return rc;
  }
  while (token_cursor->status != GRN_TOKEN_CURSOR_DONE &&
         token_cursor->status != GRN_TOKEN_CURSOR_DONE_SKIP) {
    grn_id tid;
    if ((tid = grn_token_cursor_next(ctx, token_cursor)) != GRN_ID_NIL) {
      grn_ii_cursor *cursor;
      cursor = grn_ii_cursor_open(ctx,
                                  ii,
                                  tid,
                                  GRN_ID_NIL,
                                  GRN_ID_MAX,
                                  ii->n_elements,
                                  ii_cursor_flags);
      while (grn_ii_cursor_next(ctx, cursor)) {
        grn_ii_quorum_match_record_data *record_data;

        data->rid = cursor->post->rid;
        data->sid = cursor->post->sid;
        float weight = get_weight(ctx, data);
        if (fpclassify(weight) == FP_ZERO) {
          continue;
        }
        if (grn_hash_add(ctx,
                         n_occurs,
                         cursor->post,
                         record_key_size,
                         (void **)&record_data,
                         NULL)) {
          record_data->n_occurs++;
          record_data->score += weight;
        }
      }
      grn_ii_cursor_close(ctx, cursor);
    }
  }
  grn_token_cursor_close(ctx, token_cursor);

  {
    grn_rset_posinfo posinfo;
    grn_hash_cursor *cursor;

    memset(&posinfo, 0, sizeof(grn_rset_posinfo));
    cursor = grn_hash_cursor_open(ctx, n_occurs, NULL, 0, NULL, 0, 0, -1, 0);
    if (!cursor) {
      grn_rc rc = ctx->rc;
      char ii_name[GRN_TABLE_MAX_KEY_SIZE];
      int ii_name_len;
      char errbuf[GRN_CTX_MSGSIZE];

      if (rc == GRN_SUCCESS) {
        rc = GRN_UNKNOWN_ERROR;
      }
      ii_name_len =
        grn_obj_name(ctx, (grn_obj *)ii, ii_name, GRN_TABLE_MAX_KEY_SIZE);
      grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
      ERR(
        rc,
        "[ii][quorum-match] failed to create count cursor: <%.*s>: <%.*s>%s%s",
        ii_name_len,
        ii_name,
        data->query_len,
        data->query,
        errbuf[0] ? ": " : "",
        errbuf[0] ? errbuf : "");
      grn_hash_close(ctx, n_occurs);
      return rc;
    }
    {
      const uint32_t quorum_threshold = data->optarg->quorum_threshold;
      while (grn_hash_cursor_next(ctx, cursor) != GRN_ID_NIL) {
        grn_posting *posting;
        grn_ii_quorum_match_record_data *record_data;

        grn_hash_cursor_get_key_value(ctx,
                                      cursor,
                                      (void **)&posting,
                                      NULL,
                                      (void **)&record_data);
        if (record_data->n_occurs >= quorum_threshold) {
          double score = record_data->score;
          grn_memcpy(&posinfo, posting, record_key_size);
          if (data->score_func) {
            data->record.id = posting->rid;
            data->record.weight = (int)(score / record_data->n_occurs);
            data->record.n_occurrences = record_data->n_occurs;
            data->record.total_term_weights = data->record.weight;
            score =
              data->score_func(ctx, &(data->record)) * data->record.weight;
          }
          res_add(ctx, data->result_set, &posinfo, score, data->op);
        }
      }
    }
    grn_hash_cursor_close(ctx, cursor);
  }
  grn_hash_close(ctx, n_occurs);
  grn_ii_resolve_sel_and(ctx, data->result_set, data->op);

  return GRN_SUCCESS;
}

static bool
grn_ii_select_sequential_search_reference_should_use(grn_ctx *ctx,
                                                     grn_ii_select_data *data)
{
  int n_sources;

  if (grn_ii_select_too_many_index_match_ratio_reference < 0.0) {
    return false;
  }

  if (data->op != GRN_OP_AND) {
    return false;
  }

  if (data->mode != GRN_OP_EXACT) {
    return false;
  }

  n_sources = DB_OBJ(data->ii)->source_size / sizeof(grn_id);
  if (n_sources == 0) {
    return false;
  }

  {
    grn_obj *tokenizer;
    grn_table_get_info(ctx, data->lexicon, NULL, NULL, &tokenizer, NULL, NULL);
    if (tokenizer) {
      return false;
    }
  }

  {
    int i;
    auto source_ids = static_cast<grn_id *>(DB_OBJ(data->ii)->source);

    for (i = 0; i < n_sources; i++) {
      grn_id source_id = source_ids[i];
      grn_obj *source;

      source = grn_ctx_at(ctx, source_id);
      if (!source) {
        return false;
      }
      const bool is_reference_column = grn_obj_is_reference_column(ctx, source);
      grn_obj_unref(ctx, source);
      if (!is_reference_column) {
        return false;
      }
    }
  }

  {
    uint32_t n_existing_records = GRN_HASH_SIZE(data->result_set);
    uint32_t i;
    for (i = 0; i < data->n_token_infos; i++) {
      token_info *info = data->token_infos[i];
      if (n_existing_records <=
          (info->size * grn_ii_select_too_many_index_match_ratio_reference)) {
        grn_obj reason;
        GRN_TEXT_INIT(&reason, 0);
        grn_text_printf(
          ctx,
          &reason,
          "enough filtered and "
          "index search will match many records: "
          "%u <= %.2f: "
          "n-existing-records:%u "
          "estimated-size:%u "
          "ratio:%.2f%%",
          n_existing_records,
          info->size * grn_ii_select_too_many_index_match_ratio_reference,
          n_existing_records,
          info->size,
          grn_ii_select_too_many_index_match_ratio_reference * 100);
        GRN_TEXT_PUTC(ctx, &reason, '\0');
        grn_report_index_not_used(ctx,
                                  "[ii][select]",
                                  "[reference]",
                                  (grn_obj *)(data->ii),
                                  GRN_TEXT_VALUE(&reason));
        GRN_OBJ_FIN(ctx, &reason);
        return true;
      }
    }
    return false;
  }
}

static bool
grn_ii_select_sequential_search_reference(grn_ctx *ctx,
                                          grn_ii_select_data *data)
{
  const grn_id query_id = data->token_infos[0]->cursors[0].bins[0]->id;
  const grn_id lexicon_id = DB_OBJ(data->ii->lexicon)->id;
  auto source_ids = static_cast<grn_id *>(DB_OBJ(data->ii)->source);
  const size_t n_sources = DB_OBJ(data->ii)->source_size / sizeof(grn_id);
  size_t i;
  for (i = 0; i < n_sources; i++) {
    const grn_id source_id = source_ids[i];
    grn_obj *source = grn_ctx_at(ctx, source_id);

    grn_obj value;
    switch (source->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) {
    case GRN_OBJ_COLUMN_SCALAR:
      GRN_RECORD_INIT(&value, 0, lexicon_id);
      break;
    case GRN_OBJ_COLUMN_VECTOR:
      GRN_RECORD_INIT(&value, GRN_OBJ_VECTOR, lexicon_id);
      break;
    default:
      grn_obj_unref(ctx, source);
      return false;
    }

    GRN_HASH_EACH_BEGIN(ctx, data->result_set, cursor, id)
    {
      void *key;
      grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_id source_table_id = *((grn_id *)key);
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, source, source_table_id, &value);
      switch (value.header.type) {
      case GRN_BULK:
        if (GRN_RECORD_VALUE(&value) == query_id) {
          grn_rset_posinfo info;
          info.rid = source_table_id;
          info.sid = i + 1;
          info.pos = 0;
          data->rid = info.rid;
          data->sid = info.sid;
          float score = get_weight(ctx, data);
          res_add(ctx, data->result_set, &info, score, data->op);
        }
        break;
      case GRN_UVECTOR:
        {
          uint32_t n_elements = grn_vector_size(ctx, &value);
          uint32_t j;
          uint32_t n_founds = 0;
          for (j = 0; j < n_elements; j++) {
            if (GRN_RECORD_VALUE_AT(&value, j) == query_id) {
              n_founds++;
            }
          }
          if (n_founds > 0) {
            grn_rset_posinfo info;
            info.rid = source_table_id;
            info.sid = i + 1;
            info.pos = 0;
            data->rid = info.rid;
            data->sid = info.sid;
            float score = get_weight(ctx, data);
            res_add(ctx, data->result_set, &info, score * n_founds, data->op);
          }
        }
        break;
      default:
        break;
      }
    }
    GRN_HASH_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &value);
    grn_obj_unref(ctx, source);
  }

  return true;
}

#ifdef GRN_II_SELECT_ENABLE_SEQUENTIAL_SEARCH_TEXT
static bool
grn_ii_select_sequential_search_text_should_use(grn_ctx *ctx,
                                                grn_ii_select_data *data)
{
  if (grn_ii_select_too_many_index_match_ratio_text < 0.0) {
    return false;
  }

  if (data->op != GRN_OP_AND) {
    return false;
  }

  if (data->mode != GRN_OP_EXACT) {
    return false;
  }

  size_t n_sources = DB_OBJ(data->ii)->source_size / sizeof(grn_id);
  if (n_sources == 0) {
    return false;
  }

  {
    size_t i;
    auto source_ids = static_cast<grn_id *>(DB_OBJ(data->ii)->source);

    for (i = 0; i < n_sources; i++) {
      grn_id source_id = source_ids[i];
      grn_obj *source;

      source = grn_ctx_at(ctx, source_id);
      if (!source) {
        return false;
      }
      grn_id value_type_id;
      if (grn_obj_is_table(ctx, source)) {
        value_type_id = source->header.domain;
      } else {
        value_type_id = grn_obj_get_range(ctx, source);
      }
      const bool is_text_family =
        grn_type_id_is_text_family(ctx, value_type_id);
      ;
      grn_obj_unref(ctx, source);
      if (!is_text_family) {
        return false;
      }
    }
  }

  {
    uint32_t n_existing_records = GRN_HASH_SIZE(data->result_set);
    uint32_t i;
    for (i = 0; i < data->n_token_infos; i++) {
      token_info *info = data->token_infos[i];
      if (n_existing_records <=
          (info->size * grn_ii_select_too_many_index_match_ratio_text)) {
        grn_obj reason;
        GRN_TEXT_INIT(&reason, 0);
        grn_text_printf(ctx,
                        &reason,
                        "enough filtered and "
                        "index search will match many records: "
                        "%u <= %.2f: "
                        "n-existing-records:%u "
                        "estimated-size:%u "
                        "ratio:%.2f%%",
                        n_existing_records,
                        info->size *
                          grn_ii_select_too_many_index_match_ratio_text,
                        n_existing_records,
                        info->size,
                        grn_ii_select_too_many_index_match_ratio_text * 100);
        GRN_TEXT_PUTC(ctx, &reason, '\0');
        grn_report_index_not_used(ctx,
                                  "[ii][select]",
                                  "[text]",
                                  (grn_obj *)(data->ii),
                                  GRN_TEXT_VALUE(&reason));
        GRN_OBJ_FIN(ctx, &reason);
        return true;
      }
    }
    return false;
  }
}

static void
grn_ii_select_sequential_search_text_body(grn_ctx *ctx,
                                          grn_ii_select_data *data,
                                          grn_encoding encoding,
                                          OnigRegex regex)
{
  grn_obj buffer;
  GRN_TEXT_INIT(&buffer, 0);
  auto source_ids = static_cast<grn_id *>(DB_OBJ(data->ii)->source);
  const size_t n_sources = DB_OBJ(data->ii)->source_size / sizeof(grn_id);
  size_t i;
  for (i = 0; i < n_sources; i++) {
    const grn_id source_id = source_ids[i];
    grn_obj *source = grn_ctx_at(ctx, source_id);
    const bool source_is_table = grn_obj_is_table(ctx, source);
    GRN_HASH_EACH_BEGIN(ctx, data->result_set, cursor, id)
    {
      void *key;
      grn_hash_cursor_get_key(ctx, cursor, &key);
      grn_id source_table_id = *((grn_id *)key);
      GRN_BULK_REWIND(&buffer);
      if (source_is_table) {
        grn_table_get_key2(ctx, source, source_table_id, &buffer);
      } else {
        grn_obj_get_value(ctx, source, source_table_id, &buffer);
      }

      OnigPosition position;
      grn_obj *value;
      const char *normalized_value;
      unsigned int normalized_value_length;
      value = grn_string_open_(ctx,
                               GRN_TEXT_VALUE(&buffer),
                               GRN_TEXT_LEN(&buffer),
                               data->ii->lexicon,
                               0,
                               encoding);
      grn_string_get_normalized(ctx,
                                value,
                                &normalized_value,
                                &normalized_value_length,
                                NULL);
      const OnigUChar *onig_value =
        reinterpret_cast<const OnigUChar *>(normalized_value);
      position = onig_search(regex,
                             onig_value,
                             onig_value + normalized_value_length,
                             onig_value,
                             onig_value + normalized_value_length,
                             NULL,
                             0);
      if (position != ONIG_MISMATCH) {
        grn_rset_posinfo info;
        info.rid = source_table_id;
        info.sid = i + 1;
        info.pos = 0;
        data->rid = info.rid;
        data->sid = info.rid;
        float score = get_weight(ctx, data);
        res_add(ctx, data->result_set, &info, score, data->op);
      }
      grn_obj_unlink(ctx, value);
    }
    GRN_HASH_EACH_END(ctx, cursor);
    grn_obj_unref(ctx, source);
  }
  GRN_OBJ_FIN(ctx, &buffer);
}

static bool
grn_ii_select_sequential_search_text(grn_ctx *ctx, grn_ii_select_data *data)
{
  bool processed = false;

  {
    grn_encoding encoding;
    grn_table_get_info(ctx,
                       data->ii->lexicon,
                       NULL,
                       &encoding,
                       NULL,
                       NULL,
                       NULL);
    int nflags = 0;
    grn_obj *query = grn_string_open_(ctx,
                                      data->query,
                                      data->query_len,
                                      data->ii->lexicon,
                                      nflags,
                                      encoding);
    const char *normalized_query;
    unsigned int normalized_query_length;
    grn_string_get_normalized(ctx,
                              query,
                              &normalized_query,
                              &normalized_query_length,
                              NULL);
    {
      OnigRegex regex;
      regex = grn_onigmo_new(ctx,
                             normalized_query,
                             normalized_query_length,
                             ONIG_OPTION_NONE,
                             ONIG_SYNTAX_ASIS,
                             "[ii][select][sequential]");
      if (regex) {
        grn_ii_select_sequential_search_text_body(ctx, data, encoding, regex);
        onig_free(regex);
        processed = true;
      } else {
        processed = false;
      }
    }
    grn_obj_unlink(ctx, query);
  }

  return processed;
}
#endif

static bool
grn_ii_select_sequential_search(grn_ctx *ctx, grn_ii_select_data *data)
{
  if (grn_ii_select_sequential_search_reference_should_use(ctx, data)) {
    return grn_ii_select_sequential_search_reference(ctx, data);
  }
#ifdef GRN_II_SELECT_ENABLE_SEQUENTIAL_SEARCH_TEXT
  else if (grn_ii_select_sequential_search_text_should_use(ctx, data)) {
    return grn_ii_select_sequential_search_text(ctx, data);
  }
#endif

  return false;
}

extern "C" grn_rc
grn_ii_select(grn_ctx *ctx,
              grn_ii *ii,
              const char *string,
              unsigned int string_len,
              grn_hash *s,
              grn_operator op,
              grn_select_optarg *optarg)
{
  const char *tag = "[ii][select]";
  int rep, orp;

  if (!ii) {
    return GRN_INVALID_ARGUMENT;
  }

  grn_ii_select_cursor cursor;
  grn_ii_select_data *data;
  data = &(cursor.data);
  data->ii = ii;
  data->lexicon = data->ii->lexicon;
  if (!data->lexicon || !s) {
    return GRN_INVALID_ARGUMENT;
  }

  grn_selector_data_current_set_default_tag_raw(ctx, string, string_len);

  data->query = string;
  data->query_len = string_len;
  data->result_set = s;
  data->op = op;
  grn_ii_select_data_init(ctx, data, optarg);
  if (data->mode == GRN_OP_SIMILAR) {
    grn_rc rc = grn_ii_similar_search_internal(ctx, data);
    grn_ii_select_data_fin(ctx, data);
    return rc;
  }
  if (data->mode == GRN_OP_TERM_EXTRACT) {
    grn_rc rc = grn_ii_term_extract_internal(ctx, data);
    grn_ii_select_data_fin(ctx, data);
    return rc;
  }
  if (data->mode == GRN_OP_REGEXP) {
    grn_rc rc =
      grn_ii_select_regexp(ctx, ii, string, string_len, s, op, optarg);
    grn_ii_select_data_fin(ctx, data);
    return rc;
  }
  if (data->mode == GRN_OP_QUORUM) {
    grn_rc rc = grn_ii_quorum_match(ctx, ii, data);
    grn_ii_select_data_fin(ctx, data);
    return rc;
  }
  /* todo : support subrec
  rep = (s->record_unit == GRN_REC_POSITION || s->subrec_unit ==
  GRN_REC_POSITION); orp = (s->record_unit == GRN_REC_POSITION || op ==
  GRN_OP_OR);
  */
  rep = 0;
  orp = op == GRN_OP_OR;
  if (!grn_ii_select_data_init_token_infos(ctx, data, tag)) {
    goto exit;
  }
  if (grn_logger_pass(ctx, GRN_LOG_INFO)) {
    if (grn_type_id_is_text_family(ctx, ii->lexicon->header.domain)) {
      GRN_LOG(ctx,
              GRN_LOG_INFO,
              "%s n=%d (%.*s)",
              tag,
              data->n_token_infos,
              string_len,
              string);
    } else {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect_key(ctx, &inspected, ii->lexicon, string, string_len);
      GRN_LOG(ctx,
              GRN_LOG_INFO,
              "%s n=%d (%.*s)",
              tag,
              data->n_token_infos,
              (int)GRN_TEXT_LEN(&inspected),
              GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
  }
  /* todo : array as result
  if (n == 1 && (*tis)->cursors->n_entries == 1 && op == GRN_OP_OR
      && !GRN_HASH_SIZE(s) && !s->garbages
      && s->record_unit == GRN_REC_DOCUMENT && !s->max_n_subrecs
      && grn_ii_max_section(ii) == 1) {
    grn_ii_cursor *c = (*tis)->cursors->bins[0];
    if ((rc = grn_hash_array_init(s, (*tis)->size + 32768))) { goto exit; }
    do {
      grn_rset_recinfo *ri;
      grn_posting *p = c->post;
      if ((weight = get_weight(ctx, s, p->rid, p->sid, wvm, optarg))) {
        GRN_HASH_INT_ADD(s, p, ri);
        ri->score = (p->tf + p->score) * weight;
        ri->n_subrecs = 1;
      }
    } while (grn_ii_cursor_next(ctx, c));
    goto exit;
  }
  */
  if (grn_ii_select_sequential_search(ctx, data)) {
    goto exit;
  }

  grn_ii_select_cursor_init_internal(ctx, &cursor);
  grn_result_set_add_ii_select_cursor(ctx, s, &cursor, op);

exit:
  if (!data->only_skip_token) {
    grn_ii_resolve_sel_and(ctx, s, op);
  }

  grn_ii_select_data_fin(ctx, data);

  //  grn_hash_cursor_clear(r);
#ifdef DEBUG
  {
    uint32_t i;
    uint32_t n_segments = ii->seg->header->max_segment;
    uint32_t n_nrefs = 0;
    grn_io_mapinfo *info = ii->seg->maps;
    for (i = 0; i < n_segments; i++, info++) {
      if (info->nref > 0) {
        n_nrefs++;
      }
    }
    GRN_LOG(ctx, GRN_LOG_INFO, "n_nrefs=%u", n_nrefs);
  }
#endif /* DEBUG */
  return ctx->rc;
}

static void
grn_select_optarg_init_by_search_optarg(grn_ctx *ctx,
                                        grn_select_optarg *select_optarg,
                                        grn_search_optarg *search_optarg)
{
  memset(select_optarg, 0, sizeof(grn_select_optarg));
  select_optarg->mode = GRN_OP_EXACT;
  if (!search_optarg) {
    return;
  }

  switch (search_optarg->mode) {
  case GRN_OP_NEAR:
  case GRN_OP_NEAR_NO_OFFSET:
    select_optarg->mode = search_optarg->mode;
    select_optarg->max_interval = search_optarg->max_interval;
    select_optarg->max_element_intervals = search_optarg->max_element_intervals;
    select_optarg->min_interval = search_optarg->min_interval
                                    ? *search_optarg->min_interval
                                    : GRN_II_DEFAULT_NEAR_MIN_INTERVAL;
    break;
  case GRN_OP_NEAR_PHRASE:
  case GRN_OP_ORDERED_NEAR_PHRASE:
  case GRN_OP_NEAR_PHRASE_PRODUCT:
  case GRN_OP_ORDERED_NEAR_PHRASE_PRODUCT:
    select_optarg->mode = search_optarg->mode;
    select_optarg->max_interval = search_optarg->max_interval;
    select_optarg->additional_last_interval =
      search_optarg->additional_last_interval;
    select_optarg->max_element_intervals = search_optarg->max_element_intervals;
    select_optarg->min_interval = search_optarg->min_interval
                                    ? *search_optarg->min_interval
                                    : GRN_II_DEFAULT_NEAR_MIN_INTERVAL;
    break;
  case GRN_OP_SIMILAR:
    select_optarg->mode = search_optarg->mode;
    select_optarg->similarity_threshold = search_optarg->similarity_threshold;
    break;
  case GRN_OP_REGEXP:
    select_optarg->mode = search_optarg->mode;
    break;
  case GRN_OP_FUZZY:
    select_optarg->mode = search_optarg->mode;
    select_optarg->fuzzy = search_optarg->fuzzy;
    break;
  case GRN_OP_QUORUM:
    select_optarg->mode = search_optarg->mode;
    select_optarg->quorum_threshold = search_optarg->quorum_threshold;
    break;
  default:
    break;
  }
  if (search_optarg->vector_size != 0) {
    select_optarg->weight_vector = search_optarg->weight_vector;
    select_optarg->weight_vector_float = search_optarg->weight_vector_float;
    select_optarg->vector_size = search_optarg->vector_size;
    select_optarg->weight_float = search_optarg->weight_float;
  }
  select_optarg->scorer = search_optarg->scorer;
  select_optarg->scorer_args_expr = search_optarg->scorer_args_expr;
  select_optarg->scorer_args_expr_offset =
    search_optarg->scorer_args_expr_offset;
  select_optarg->fuzzy = search_optarg->fuzzy;
  select_optarg->match_info = &(search_optarg->match_info);
  select_optarg->query_options = search_optarg->query_options;
  if (search_optarg->query_domain) {
    select_optarg->query_domain = *(search_optarg->query_domain);
  } else {
    select_optarg->query_domain = GRN_ID_NIL;
  }
}

static uint32_t
grn_ii_estimate_size_for_query_regexp(grn_ctx *ctx,
                                      grn_ii *ii,
                                      const char *query,
                                      unsigned int query_len,
                                      grn_search_optarg *optarg)
{
  grn_rc rc;
  grn_obj parsed_strings;
  grn_obj parsed_n_following_characters;
  uint32_t size;

  GRN_TEXT_INIT(&parsed_strings, GRN_OBJ_VECTOR);
  GRN_UINT32_INIT(&parsed_n_following_characters, GRN_OBJ_VECTOR);
  rc = grn_ii_parse_regexp_query(ctx,
                                 "[ii][estimate-size][query][regexp]",
                                 query,
                                 query_len,
                                 &parsed_strings,
                                 &parsed_n_following_characters);
  if (rc != GRN_SUCCESS) {
    GRN_OBJ_FIN(ctx, &parsed_strings);
    return 0;
  }

  if (optarg) {
    optarg->mode = GRN_OP_EXACT;
  }

  unsigned int i;
  unsigned int n_parsed_strings = grn_vector_size(ctx, &parsed_strings);
  for (i = 0; i < n_parsed_strings; i++) {
    const char *parsed_string;
    unsigned int parsed_string_len;
    parsed_string_len = grn_vector_get_element(ctx,
                                               &parsed_strings,
                                               0,
                                               &parsed_string,
                                               NULL,
                                               NULL);
    size_t estimated_size = grn_ii_estimate_size_for_query(ctx,
                                                           ii,
                                                           parsed_string,
                                                           parsed_string_len,
                                                           optarg);
    if (i == 0) {
      size = estimated_size;
    } else {
      if (estimated_size < size) {
        size = estimated_size;
      }
    }
  }
  GRN_OBJ_FIN(ctx, &parsed_strings);

  if (optarg) {
    optarg->mode = GRN_OP_REGEXP;
  }

  return size;
}

extern "C" uint32_t
grn_ii_estimate_size_for_query(grn_ctx *ctx,
                               grn_ii *ii,
                               const char *query,
                               unsigned int query_len,
                               grn_search_optarg *optarg)
{
  const char *tag = "[ii][estimate][size][query]";

  if (query_len == 0) {
    return 0;
  }

  grn_select_optarg select_optarg;
  grn_select_optarg_init_by_search_optarg(ctx, &select_optarg, optarg);

  grn_ii_select_data data;
  data.ii = ii;
  data.lexicon = data.ii->lexicon;
  data.query = query;
  data.query_len = query_len;
  grn_ii_select_data_init(ctx, &data, &select_optarg);
  if (data.mode == GRN_OP_REGEXP) {
    grn_ii_select_data_fin(ctx, &data);
    return grn_ii_estimate_size_for_query_regexp(ctx,
                                                 ii,
                                                 query,
                                                 query_len,
                                                 optarg);
  }

  if (!grn_ii_select_data_init_token_infos(ctx, &data, tag)) {
    grn_ii_select_data_fin(ctx, &data);
    return 0;
  }

  uint32_t i;
  double estimated_size = 0;
  double normalized_ratio = 1.0;
  for (i = 0; i < data.n_token_infos; i++) {
    token_info *ti = data.token_infos[i];
    double term_estimated_size;
    term_estimated_size = ((double)ti->size / ti->ntoken);
    if (i == 0) {
      estimated_size = term_estimated_size;
    } else {
      if (term_estimated_size < estimated_size) {
        estimated_size = term_estimated_size;
      }
      normalized_ratio *= grn_ii_estimate_size_for_query_reduce_ratio;
    }
  }

  estimated_size *= normalized_ratio;
  if (estimated_size > 0.0 && estimated_size < 1.0) {
    estimated_size = 1.0;
  }

  grn_ii_select_data_fin(ctx, &data);

  return (uint32_t)estimated_size;
}

extern "C" uint32_t
grn_ii_estimate_size_for_lexicon_cursor(grn_ctx *ctx,
                                        grn_ii *ii,
                                        grn_table_cursor *lexicon_cursor)
{
  grn_id term_id;
  uint32_t estimated_size = 0;

  while ((term_id = grn_table_cursor_next(ctx, lexicon_cursor)) != GRN_ID_NIL) {
    uint32_t term_estimated_size;
    term_estimated_size = grn_ii_estimate_size(ctx, ii, term_id);
    estimated_size += term_estimated_size;
  }

  return estimated_size;
}

extern "C" grn_rc
grn_ii_sel(grn_ctx *ctx,
           grn_ii *ii,
           const char *string,
           unsigned int string_len,
           grn_hash *s,
           grn_operator op,
           grn_search_optarg *optarg)
{
  ERRCLR(ctx);
  if (grn_logger_pass(ctx, GRN_LOG_INFO)) {
    grn_id query_domain = GRN_ID_NIL;
    if (optarg && optarg->query_domain) {
      query_domain = *(optarg->query_domain);
    }
    if (query_domain == GRN_ID_NIL) {
      query_domain = ii->lexicon->header.domain;
    }
    if (grn_type_id_is_text_family(ctx, query_domain)) {
      GRN_LOG(ctx, GRN_LOG_INFO, "grn_ii_sel > (%.*s)", string_len, string);
    } else {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      if (query_domain == ii->lexicon->header.domain) {
        grn_inspect_key(ctx, &inspected, ii->lexicon, string, string_len);
      } else {
        grn_obj query;
        GRN_OBJ_INIT(&query, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY, query_domain);
        GRN_TEXT_SET(ctx, &query, string, string_len);
        grn_inspect(ctx, &inspected, &query);
        GRN_OBJ_FIN(ctx, &query);
      }
      GRN_LOG(ctx,
              GRN_LOG_INFO,
              "grn_ii_sel > (%.*s)",
              (int)GRN_TEXT_LEN(&inspected),
              GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
  }
  if (!s) {
    return GRN_INVALID_ARGUMENT;
  }
  uint16_t start_depth = grn_ctx_trace_log_get_current_depth(ctx);
  grn_ctx_trace_log_push(ctx);
  grn_ctx_trace_log_emit_string(ctx, "ii.select.input", string, string_len);
  grn_ctx_trace_log_emit_cstring(ctx,
                                 "ii.select.operator",
                                 grn_operator_to_string(op));
  {
    grn_select_optarg arg;
    grn_select_optarg_init_by_search_optarg(ctx, &arg, optarg);
    /* todo : support subrec
    grn_rset_init(ctx, s, GRN_REC_DOCUMENT, 0, GRN_REC_NONE, 0, 0);
    */
    grn_ctx_trace_log_push(ctx);
    if (grn_ii_select(ctx, ii, string, string_len, s, op, &arg)) {
      GRN_LOG(ctx, GRN_LOG_ERROR, "grn_ii_select on grn_ii_sel() failed!");
      goto exit;
    }
    GRN_LOG(ctx, GRN_LOG_INFO, "exact: %d", GRN_HASH_SIZE(s));
    grn_ctx_trace_log_emit_uint32(ctx,
                                  "ii.select.exact.n_hits",
                                  GRN_HASH_SIZE(s));
    grn_ctx_trace_log_pop(ctx);
    if (op == GRN_OP_OR || ctx->impl->force_match_escalation) {
      grn_id min = GRN_ID_NIL;
      if (((int64_t)GRN_HASH_SIZE(s) <= ctx->impl->match_escalation_threshold ||
           ctx->impl->force_match_escalation) &&
          ((arg.fuzzy.flags & GRN_TABLE_FUZZY_SEARCH_USE_MAX_DISTANCE_RATIO)
             ? (arg.fuzzy.max_distance_ratio > FLT_EPSILON)
             : (arg.fuzzy.max_distance > 0))) {
        arg.mode = GRN_OP_FUZZY;
        if (arg.match_info) {
          if (arg.match_info->flags & GRN_MATCH_INFO_GET_MIN_RECORD_ID) {
            min = arg.match_info->min;
            arg.match_info->min = GRN_ID_NIL;
          }
        }
        grn_ctx_trace_log_push(ctx);
        if (grn_ii_select(ctx, ii, string, string_len, s, op, &arg)) {
          GRN_LOG(ctx,
                  GRN_LOG_ERROR,
                  "grn_ii_select on grn_ii_sel(fuzzy) failed!");
          goto exit;
        }
        GRN_LOG(ctx, GRN_LOG_INFO, "fuzzy: %d", GRN_HASH_SIZE(s));
        grn_ctx_trace_log_emit_uint32(ctx,
                                      "ii.select.fuzzy.n_hits",
                                      GRN_HASH_SIZE(s));
        grn_ctx_trace_log_pop(ctx);
        if (arg.match_info) {
          if (arg.match_info->flags & GRN_MATCH_INFO_GET_MIN_RECORD_ID) {
            if (min > GRN_ID_NIL && min < arg.match_info->min) {
              arg.match_info->min = min;
            }
          }
        }
      }
      if ((int64_t)GRN_HASH_SIZE(s) <= ctx->impl->match_escalation_threshold ||
          ctx->impl->force_match_escalation) {
        arg.mode = GRN_OP_UNSPLIT;
        if (arg.match_info) {
          if (arg.match_info->flags & GRN_MATCH_INFO_GET_MIN_RECORD_ID) {
            min = arg.match_info->min;
            arg.match_info->min = GRN_ID_NIL;
          }
        }
        if (grn_ii_select(ctx, ii, string, string_len, s, op, &arg)) {
          GRN_LOG(ctx,
                  GRN_LOG_ERROR,
                  "grn_ii_select on grn_ii_sel(unsplit) failed!");
          goto exit;
        }
        GRN_LOG(ctx, GRN_LOG_INFO, "unsplit: %d", GRN_HASH_SIZE(s));
        grn_ctx_trace_log_emit_uint32(ctx,
                                      "ii.select.unsplit.n_hits",
                                      GRN_HASH_SIZE(s));
        grn_ctx_trace_log_pop(ctx);
        if (arg.match_info) {
          if (arg.match_info->flags & GRN_MATCH_INFO_GET_MIN_RECORD_ID) {
            if (min > GRN_ID_NIL && min < arg.match_info->min) {
              arg.match_info->min = min;
            }
          }
        }
      }
      if ((int64_t)GRN_HASH_SIZE(s) <= ctx->impl->match_escalation_threshold ||
          ctx->impl->force_match_escalation) {
        arg.mode = GRN_OP_PARTIAL;
        if (arg.match_info) {
          if (arg.match_info->flags & GRN_MATCH_INFO_GET_MIN_RECORD_ID) {
            min = arg.match_info->min;
            arg.match_info->min = GRN_ID_NIL;
          }
        }
        grn_ctx_trace_log_push(ctx);
        if (grn_ii_select(ctx, ii, string, string_len, s, op, &arg)) {
          GRN_LOG(ctx,
                  GRN_LOG_ERROR,
                  "grn_ii_select on grn_ii_sel(partial) failed!");
          goto exit;
        }
        GRN_LOG(ctx, GRN_LOG_INFO, "partial: %d", GRN_HASH_SIZE(s));
        grn_ctx_trace_log_emit_uint32(ctx,
                                      "ii.select.partial.n_hits",
                                      GRN_HASH_SIZE(s));
        grn_ctx_trace_log_pop(ctx);
        if (arg.match_info) {
          if (arg.match_info->flags & GRN_MATCH_INFO_GET_MIN_RECORD_ID) {
            if (min > GRN_ID_NIL && min < arg.match_info->min) {
              arg.match_info->min = min;
            }
          }
        }
      }
    }
    GRN_LOG(ctx, GRN_LOG_INFO, "hits=%d", GRN_HASH_SIZE(s));
    grn_ctx_trace_log_emit_uint32(ctx, "ii.select.n_hits", GRN_HASH_SIZE(s));
    grn_ctx_trace_log_pop(ctx);
  }
exit:
  grn_ctx_trace_log_set_current_depth(ctx, start_depth);
  return ctx->rc;
}

extern "C" grn_rc
grn_ii_select_by_id(
  grn_ctx *ctx, grn_ii *ii, grn_id id, grn_hash *result_set, grn_operator op)
{
  GRN_API_ENTER;
  grn_rc rc = grn_ii_at(ctx, ii, id, result_set, op);
  GRN_API_RETURN(rc);
}

extern "C" grn_rc
grn_ii_at(grn_ctx *ctx, grn_ii *ii, grn_id id, grn_hash *s, grn_operator op)
{
  int rep = 0;
  grn_ii_cursor *c;
  if ((c = grn_ii_cursor_open(ctx,
                              ii,
                              id,
                              GRN_ID_NIL,
                              GRN_ID_MAX,
                              rep ? ii->n_elements : ii->n_elements - 1,
                              0))) {
    grn_result_set_add_ii_cursor(ctx, s, c, 1, 1, op);
    grn_ii_cursor_close(ctx, c);
  }
  return ctx->rc;
}

extern "C" void
grn_ii_resolve_sel_and(grn_ctx *ctx, grn_hash *s, grn_operator op)
{
  if (op == GRN_OP_AND &&
      !(ctx->flags & GRN_CTX_TEMPORARY_DISABLE_II_RESOLVE_SEL_AND)) {
    grn_id eid;
    grn_rset_recinfo *ri;
    grn_hash_cursor *c =
      grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0, -1, 0);
    if (c) {
      while ((eid = grn_hash_cursor_next(ctx, c))) {
        grn_hash_cursor_get_value(ctx, c, (void **)&ri);
        if ((ri->n_subrecs & GRN_RSET_UTIL_BIT)) {
          ri->n_subrecs &= ~GRN_RSET_UTIL_BIT;
        } else {
          grn_hash_delete_by_id(ctx, s, eid, NULL);
        }
      }
      grn_hash_cursor_close(ctx, c);
    }
  }
}

extern "C" void
grn_ii_cursor_inspect(grn_ctx *ctx, grn_ii_cursor *c, grn_obj *buf)
{
  grn_obj key_buf;
  char key[GRN_TABLE_MAX_KEY_SIZE];
  int key_size;
  int i = 0;
  grn_ii_cursor_next_options options = {/* .include_garbage = */ true};

  GRN_TEXT_PUTS(ctx, buf, "  #<");
  key_size =
    grn_table_get_key(ctx, c->ii->lexicon, c->id, key, GRN_TABLE_MAX_KEY_SIZE);
  GRN_OBJ_INIT(&key_buf, GRN_BULK, 0, c->ii->lexicon->header.domain);
  GRN_TEXT_SET(ctx, &key_buf, key, key_size);
  grn_inspect(ctx, buf, &key_buf);
  GRN_OBJ_FIN(ctx, &key_buf);

  GRN_TEXT_PUTS(ctx, buf, "\n    elements:[\n      ");
  while (grn_ii_cursor_next_internal(ctx, c, &options)) {
    grn_posting *pos = c->post;
    if (i > 0) {
      GRN_TEXT_PUTS(ctx, buf, ",\n      ");
    }
    i++;
    GRN_TEXT_PUTS(ctx, buf, "{status:");
    if (pos->tf && pos->sid) {
      GRN_TEXT_PUTS(ctx, buf, "available");
    } else {
      GRN_TEXT_PUTS(ctx, buf, "garbage");
    }
    GRN_TEXT_PUTS(ctx, buf, ", rid:");
    grn_text_lltoa(ctx, buf, pos->rid);
    GRN_TEXT_PUTS(ctx, buf, ", sid:");
    grn_text_lltoa(ctx, buf, pos->sid);
    GRN_TEXT_PUTS(ctx, buf, ", pos:");
    grn_text_lltoa(ctx, buf, pos->pos);
    GRN_TEXT_PUTS(ctx, buf, ", tf:");
    grn_text_lltoa(ctx, buf, pos->tf);
    GRN_TEXT_PUTS(ctx, buf, ", weight:");
    grn_text_f32toa(ctx, buf, grn_posting_get_weight_float(ctx, pos));
    GRN_TEXT_PUTS(ctx, buf, ", rest:");
    grn_text_lltoa(ctx, buf, pos->rest);
    GRN_TEXT_PUTS(ctx, buf, "}");
  }
  GRN_TEXT_PUTS(ctx, buf, "\n    ]\n  >");
}

extern "C" void
grn_ii_inspect_values(grn_ctx *ctx, grn_ii *ii, grn_obj *buf)
{
  grn_table_cursor *tc;
  GRN_TEXT_PUTS(ctx, buf, "[");
  if ((tc = grn_table_cursor_open(ctx,
                                  ii->lexicon,
                                  NULL,
                                  0,
                                  NULL,
                                  0,
                                  0,
                                  -1,
                                  GRN_CURSOR_ASCENDING))) {
    int i = 0;
    grn_id tid;
    grn_ii_cursor *c;
    while ((tid = grn_table_cursor_next(ctx, tc))) {
      if (i > 0) {
        GRN_TEXT_PUTS(ctx, buf, ",");
      }
      i++;
      GRN_TEXT_PUTS(ctx, buf, "\n");
      if ((c = grn_ii_cursor_open(ctx,
                                  ii,
                                  tid,
                                  GRN_ID_NIL,
                                  GRN_ID_MAX,
                                  ii->n_elements,
                                  GRN_OBJ_WITH_POSITION |
                                    GRN_OBJ_WITH_SECTION))) {
        grn_ii_cursor_inspect(ctx, c, buf);
        grn_ii_cursor_close(ctx, c);
      }
    }
    grn_table_cursor_close(ctx, tc);
  }
  GRN_TEXT_PUTS(ctx, buf, "]");
}

/********************** buffered index builder ***********************/

typedef struct {
  unsigned int sid; /* Section ID */
  uint32_t weight;  /* Weight */
  const char *p;    /* Value address */
  uint32_t len;     /* Value length */
  char *buf;        /* Buffer address */
  uint32_t cap;     /* Buffer size */
} ii_buffer_value;

/* ii_buffer_counter is associated with a combination of a block an a term. */
typedef struct {
  uint32_t nrecs;  /* Number of records or sections */
  uint32_t nposts; /* Number of occurrences */

  /* Information of the last value */
  grn_id last_rid;      /* Record ID */
  uint32_t last_sid;    /* Section ID */
  uint32_t last_tf;     /* Term frequency */
  uint32_t last_weight; /* Total weight */
  uint32_t last_pos;    /* Token position */

  /* Meaning of offset_* is different before/after encoding. */
  /* Before encoding: size in encoded sequence */
  /* After encoding: Offset in encoded sequence */
  uint32_t offset_rid;    /* Record ID */
  uint32_t offset_sid;    /* Section ID */
  uint32_t offset_tf;     /* Term frequency */
  uint32_t offset_weight; /* Weight */
  uint32_t offset_pos;    /* Token position */
} ii_buffer_counter;

typedef struct {
  off64_t head;
  off64_t tail;
  uint32_t nextsize;
  uint8_t *buffer;
  uint32_t buffersize;
  uint8_t *bufcur;
  uint32_t rest;
  grn_id tid;
  uint32_t nrecs;
  uint32_t nposts;
  grn_id *recs;
  uint32_t *tfs;
  uint32_t *posts;
} ii_buffer_block;

struct _grn_ii_buffer {
  grn_obj *lexicon;        /* Global lexicon */
  grn_obj *tmp_lexicon;    /* Temporary lexicon for each block */
  ii_buffer_block *blocks; /* Blocks */
  uint32_t nblocks;        /* Number of blocks */
  int tmpfd;               /* Descriptor of temporary file */
  char tmpfpath[PATH_MAX]; /* Path of temporary file */
  uint64_t update_buffer_size;

  // stuff for parsing
  off64_t filepos;             /* Write position of temporary file */
  grn_id *block_buf;           /* Buffer for the current block */
  size_t block_buf_size;       /* Size of block_buf */
  size_t block_pos;            /* Write position of block_buf */
  ii_buffer_counter *counters; /* Status of terms */
  uint32_t ncounters;          /* Number of counters */
  size_t total_size;
  size_t curr_size;
  ii_buffer_value *values;  /* Values in block */
  unsigned int nvalues;     /* Number of values in block */
  unsigned int max_nvalues; /* Size of values */
  grn_id last_rid;

  // stuff for merging
  grn_ii *ii;
  uint32_t lseg;
  uint32_t dseg;
  buffer *term_buffer;
  datavec data_vectors[MAX_N_ELEMENTS + 1];
  uint8_t *packed_buf;
  size_t packed_buf_size;
  size_t packed_len;
  size_t total_chunk_size;
};

/*
 * The temporary file format is roughly as follows:
 *
 * File  = Block...
 * Block = Unit...
 * Unit  = TermChunk (key order)
 *         NextUnitSize (The first unit size is kept on memory)
 * Chunk = Term...
 * Term  = ID (gtid)
 *         NumRecordsOrSections (nrecs), NumOccurrences (nposts)
 *         RecordID... (rid, diff)
 *         [SectionID... (sid, diff)]
 *         TermFrequency... (tf, diff)
 *         [Weight... (weight, diff)]
 *         [Position... (pos, diff)]
 */

/*
 * ==========================================================================
 * The following part provides constants, structures and functions for static
 * indexing.
 * ==========================================================================
 */

#define GRN_II_BUILDER_BUFFER_CHUNK_SIZE      (S_CHUNK >> 2)

#define GRN_II_BUILDER_MAX_LEXICON_CACHE_SIZE (1 << 24)

#define GRN_II_BUILDER_MIN_BLOCK_THRESHOLD    1
#define GRN_II_BUILDER_MAX_BLOCK_THRESHOLD    (1 << 28)

#define GRN_II_BUILDER_MIN_FILE_BUF_SIZE      (1 << 12)
#define GRN_II_BUILDER_MAX_FILE_BUF_SIZE      (1 << 30)

#define GRN_II_BUILDER_MIN_BLOCK_BUF_SIZE     (1 << 12)
#define GRN_II_BUILDER_MAX_BLOCK_BUF_SIZE     (1 << 30)

#define GRN_II_BUILDER_MIN_CHUNK_THRESHOLD    1
#define GRN_II_BUILDER_MAX_CHUNK_THRESHOLD    (1 << 28)

#define GRN_II_BUILDER_MIN_BUFFER_MAX_N_TERMS 1
#define GRN_II_BUILDER_MAX_BUFFER_MAX_N_TERMS                                  \
  ((S_SEGMENT - sizeof(buffer_header)) / sizeof(buffer_term))

struct grn_ii_builder_options {
  uint32_t lexicon_cache_size; /* Cache size of temporary lexicon */
  /* A block is flushed if builder->n reaches this value. */
  uint32_t block_threshold;
  uint32_t file_buf_size;  /* Buffer size for buffered output */
  uint32_t block_buf_size; /* Buffer size for buffered input */
  /* A chunk is flushed if chunk->n reaches this value. */
  uint32_t chunk_threshold;
  uint32_t buffer_max_n_terms; /* Maximum number of terms in each buffer */
};

static const grn_ii_builder_options grn_ii_builder_default_options = {
  0x80000,   /* lexicon_cache_size */
  0x4000000, /* block_threshold */
  0x10000,   /* file_buf_size */
  0x10000,   /* block_buf_size */
  0x1000,    /* chunk_threshold */
  0x3000,    /* buffer_max_n_terms */
};

/* grn_ii_builder_options_init fills options with the default options. */
void
grn_ii_builder_options_init(grn_ii_builder_options *options)
{
  *options = grn_ii_builder_default_options;
}

/* grn_ii_builder_options_fix fixes out-of-range options. */
static void
grn_ii_builder_options_fix(grn_ii_builder_options *options)
{
  if (options->lexicon_cache_size > GRN_II_BUILDER_MAX_LEXICON_CACHE_SIZE) {
    options->lexicon_cache_size = GRN_II_BUILDER_MAX_LEXICON_CACHE_SIZE;
  }

  if (options->block_threshold < GRN_II_BUILDER_MIN_BLOCK_THRESHOLD) {
    options->block_threshold = GRN_II_BUILDER_MIN_BLOCK_THRESHOLD;
  }
  if (options->block_threshold > GRN_II_BUILDER_MAX_BLOCK_THRESHOLD) {
    options->block_threshold = GRN_II_BUILDER_MAX_BLOCK_THRESHOLD;
  }

  if (options->file_buf_size < GRN_II_BUILDER_MIN_FILE_BUF_SIZE) {
    options->file_buf_size = GRN_II_BUILDER_MIN_FILE_BUF_SIZE;
  }
  if (options->file_buf_size > GRN_II_BUILDER_MAX_FILE_BUF_SIZE) {
    options->file_buf_size = GRN_II_BUILDER_MAX_FILE_BUF_SIZE;
  }

  if (options->block_buf_size < GRN_II_BUILDER_MIN_BLOCK_BUF_SIZE) {
    options->block_buf_size = GRN_II_BUILDER_MIN_BLOCK_BUF_SIZE;
  }
  if (options->block_buf_size > GRN_II_BUILDER_MAX_BLOCK_BUF_SIZE) {
    options->block_buf_size = GRN_II_BUILDER_MAX_BLOCK_BUF_SIZE;
  }

  if (options->chunk_threshold < GRN_II_BUILDER_MIN_CHUNK_THRESHOLD) {
    options->chunk_threshold = GRN_II_BUILDER_MIN_CHUNK_THRESHOLD;
  }
  if (options->chunk_threshold > GRN_II_BUILDER_MAX_CHUNK_THRESHOLD) {
    options->chunk_threshold = GRN_II_BUILDER_MAX_CHUNK_THRESHOLD;
  }

  if (options->buffer_max_n_terms < GRN_II_BUILDER_MIN_BUFFER_MAX_N_TERMS) {
    options->buffer_max_n_terms = GRN_II_BUILDER_MIN_BUFFER_MAX_N_TERMS;
  }
  if (options->buffer_max_n_terms > GRN_II_BUILDER_MAX_BUFFER_MAX_N_TERMS) {
    options->buffer_max_n_terms = GRN_II_BUILDER_MAX_BUFFER_MAX_N_TERMS;
  }
}

#define GRN_II_BUILDER_TERM_INPLACE_SIZE                                       \
  (sizeof(grn_ii_builder_term) - offsetof(grn_ii_builder_term, dummy))

typedef struct {
  grn_id rid;   /* Last record ID */
  uint32_t sid; /* Last section ID */
  /* Last position (GRN_OBJ_WITH_POSITION) or frequency. */
  uint32_t pos_or_freq;
  uint32_t offset; /* Buffer write offset */
  uint32_t size;   /* Buffer size */
  uint32_t dummy;  /* Padding */
  uint8_t *buf;    /* Buffer (to be freed) */
} grn_ii_builder_term;

/* grn_ii_builder_term_is_inplace returns whether a term buffer is inplace. */
static inline bool
grn_ii_builder_term_is_inplace(grn_ii_builder_term *term)
{
  return term->size == GRN_II_BUILDER_TERM_INPLACE_SIZE;
}

/* grn_ii_builder_term_get_buf returns a term buffer. */
static inline uint8_t *
grn_ii_builder_term_get_buf(grn_ii_builder_term *term)
{
  if (grn_ii_builder_term_is_inplace(term)) {
    return (uint8_t *)&term->dummy;
  } else {
    return term->buf;
  }
}

/*
 * grn_ii_builder_term_init initializes a term. Note that an initialized term
 * must be finalized by grn_ii_builder_term_fin.
 */
static void
grn_ii_builder_term_init(grn_ctx *ctx, grn_ii_builder_term *term)
{
  term->rid = GRN_ID_NIL;
  term->sid = 0;
  term->pos_or_freq = 0;
  term->offset = 0;
  term->size = GRN_II_BUILDER_TERM_INPLACE_SIZE;
}

/* grn_ii_builder_term_fin finalizes a term. */
static void
grn_ii_builder_term_fin(grn_ctx *ctx, grn_ii_builder_term *term)
{
  if (!grn_ii_builder_term_is_inplace(term)) {
    GRN_FREE(term->buf);
  }
}

/* grn_ii_builder_term_reinit reinitializes a term. */
static void
grn_ii_builder_term_reinit(grn_ctx *ctx, grn_ii_builder_term *term)
{
  grn_ii_builder_term_fin(ctx, term);
  grn_ii_builder_term_init(ctx, term);
}

/* grn_ii_builder_term_extend extends a term buffer. */
static grn_rc
grn_ii_builder_term_extend(grn_ctx *ctx, grn_ii_builder_term *term)
{
  uint8_t *buf;
  uint32_t size = term->size * 2;
  if (grn_ii_builder_term_is_inplace(term)) {
    buf = (uint8_t *)GRN_MALLOC(size);
    if (!buf) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "failed to allocate memory for term buffer: size = %u",
          size);
      return ctx->rc;
    }
    grn_memcpy(buf, &term->dummy, term->offset);
  } else {
    buf = (uint8_t *)GRN_REALLOC(term->buf, size);
    if (!buf) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "failed to reallocate memory for term buffer: size = %u",
          size);
      return ctx->rc;
    }
  }
  term->buf = buf;
  term->size = size;
  return GRN_SUCCESS;
}

/* grn_ii_builder_term_append appends an integer to a term buffer. */
static inline grn_rc
grn_ii_builder_term_append(grn_ctx *ctx,
                           grn_ii_builder_term *term,
                           uint64_t value)
{
  uint8_t *p;
  if (value < (uint64_t)1 << 5) {
    if (term->offset + 1 > term->size) {
      grn_rc rc = grn_ii_builder_term_extend(ctx, term);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
    }
    p = grn_ii_builder_term_get_buf(term) + term->offset;
    p[0] = (uint8_t)value;
    term->offset++;
    return GRN_SUCCESS;
  } else if (value < (uint64_t)1 << 13) {
    if (term->offset + 2 > term->size) {
      grn_rc rc = grn_ii_builder_term_extend(ctx, term);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
    }
    p = grn_ii_builder_term_get_buf(term) + term->offset;
    p[0] = (uint8_t)((value & 0x1f) | (1 << 5));
    p[1] = (uint8_t)(value >> 5);
    term->offset += 2;
    return GRN_SUCCESS;
  } else {
    uint8_t i, n;
    if (value < (uint64_t)1 << 21) {
      n = 3;
    } else if (value < (uint64_t)1 << 29) {
      n = 4;
    } else if (value < (uint64_t)1 << 37) {
      n = 5;
    } else if (value < (uint64_t)1 << 45) {
      n = 6;
    } else if (value < (uint64_t)1 << 53) {
      n = 7;
    } else {
      n = 8;
    }
    if (term->offset + n > term->size) {
      grn_rc rc = grn_ii_builder_term_extend(ctx, term);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
    }
    p = grn_ii_builder_term_get_buf(term) + term->offset;
    p[0] = (uint8_t)(value & 0x1f) | ((n - 1) << 5);
    value >>= 5;
    for (i = 1; i < n; i++) {
      p[i] = (uint8_t)value;
      value >>= 8;
    }
    term->offset += n;
    return GRN_SUCCESS;
  }
}

typedef struct {
  uint64_t offset; /* File offset */
  uint32_t rest;   /* Remaining size */
  uint8_t *buf;    /* Buffer (to be freed) */
  uint8_t *cur;    /* Current pointer */
  uint8_t *end;    /* End pointer */
  uint32_t tid;    /* Term ID */
} grn_ii_builder_block;

/*
 * grn_ii_builder_block_init initializes a block. Note that an initialized
 * block must be finalized by grn_ii_builder_block_fin.
 */
static void
grn_ii_builder_block_init(grn_ctx *ctx, grn_ii_builder_block *block)
{
  block->offset = 0;
  block->rest = 0;
  block->buf = NULL;
  block->cur = NULL;
  block->end = NULL;
  block->tid = GRN_ID_NIL;
}

/* grn_ii_builder_block_fin finalizes a block. */
static void
grn_ii_builder_block_fin(grn_ctx *ctx, grn_ii_builder_block *block)
{
  if (block->buf) {
    GRN_FREE(block->buf);
  }
}

/*
 * grn_ii_builder_block_next reads the next integer. Note that this function
 * returns GRN_END_OF_DATA if it reaches the end of a block.
 */
static inline grn_rc
grn_ii_builder_block_next(grn_ctx *ctx,
                          grn_ii_builder_block *block,
                          uint64_t *value)
{
  uint8_t n;
  if (block->cur == block->end) {
    return GRN_END_OF_DATA;
  }
  n = (*block->cur >> 5) + 1;
  if (n > block->end - block->cur) {
    return GRN_END_OF_DATA;
  }
  *value = 0;
  switch (n) {
  case 8:
    *value |= (uint64_t)block->cur[7] << 53;
  case 7:
    *value |= (uint64_t)block->cur[6] << 45;
  case 6:
    *value |= (uint64_t)block->cur[5] << 37;
  case 5:
    *value |= (uint64_t)block->cur[4] << 29;
  case 4:
    *value |= (uint64_t)block->cur[3] << 21;
  case 3:
    *value |= (uint64_t)block->cur[2] << 13;
  case 2:
    *value |= (uint64_t)block->cur[1] << 5;
  case 1:
    *value |= block->cur[0] & 0x1f;
    break;
  }
  block->cur += n;
  return GRN_SUCCESS;
}

typedef struct {
  grn_ii *ii;            /* Inverted index */
  uint32_t buf_id;       /* Buffer ID */
  uint32_t buf_seg_id;   /* Buffer segment ID */
  buffer *buf;           /* Buffer (to be unreferenced) */
  uint32_t chunk_id;     /* Chunk ID */
  uint32_t chunk_seg_id; /* Chunk segment ID */
  uint8_t *chunk;        /* Chunk (to be unreferenced) */
  uint32_t chunk_offset; /* Chunk write position */
  uint32_t chunk_size;   /* Chunk size */
} grn_ii_builder_buffer;

/*
 * grn_ii_builder_buffer_init initializes a buffer. Note that a buffer must be
 * finalized by grn_ii_builder_buffer_fin.
 */
static void
grn_ii_builder_buffer_init(grn_ctx *ctx, grn_ii_builder_buffer *buf, grn_ii *ii)
{
  buf->ii = ii;
  buf->buf_id = 0;
  buf->buf_seg_id = 0;
  buf->buf = NULL;
  buf->chunk_id = 0;
  buf->chunk_seg_id = 0;
  buf->chunk = NULL;
  buf->chunk_offset = 0;
  buf->chunk_size = 0;
}

/* grn_ii_builder_buffer_fin finalizes a buffer. */
static void
grn_ii_builder_buffer_fin(grn_ctx *ctx, grn_ii_builder_buffer *buf)
{
  if (buf->buf) {
    grn_io_seg_unref(ctx, buf->ii->seg, buf->buf_seg_id);
  }
  if (buf->chunk) {
    grn_io_seg_unref(ctx, buf->ii->chunk, buf->chunk_seg_id);
  }
}

/* grn_ii_builder_buffer_is_assigned returns whether a buffer is assigned. */
static bool
grn_ii_builder_buffer_is_assigned(grn_ctx *ctx, grn_ii_builder_buffer *buf)
{
  return buf->buf != NULL;
}

/* grn_ii_builder_buffer_assign assigns a buffer. */
static grn_rc
grn_ii_builder_buffer_assign(grn_ctx *ctx,
                             grn_ii_builder_buffer *buf,
                             size_t min_chunk_size)
{
  void *seg;
  size_t chunk_size;
  grn_rc rc;

  /* Create a buffer. */
  buf->buf_id = GRN_II_PSEG_NOT_ASSIGNED;
  rc = buffer_segment_new(ctx, buf->ii, &buf->buf_id);
  if (rc != GRN_SUCCESS) {
    if (ctx->rc != GRN_SUCCESS) {
      ERR(rc,
          "[ii][builder][buffer][assign] "
          "failed to allocate segment for buffer");
    }
    return rc;
  }
  buf->buf_seg_id = grn_ii_get_buffer_pseg_inline(buf->ii, buf->buf_id);
  seg = grn_io_seg_ref(ctx, buf->ii->seg, buf->buf_seg_id);
  if (!seg) {
    if (ctx->rc == GRN_SUCCESS) {
      ERR(GRN_UNKNOWN_ERROR,
          "[ii][builder][buffer][assign] "
          "failed to access buffer segment: buffer_id=<%u> segment_id=<%u>",
          buf->buf_id,
          buf->buf_seg_id);
    }
    return ctx->rc;
  }
  buf->buf = (buffer *)seg;

  /* Create a chunk. */
  chunk_size = GRN_II_BUILDER_BUFFER_CHUNK_SIZE;
  while (chunk_size < min_chunk_size) {
    chunk_size *= 2;
  }
  rc = chunk_new(ctx, buf->ii, &buf->chunk_id, chunk_size);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  const size_t seg_offset =
    (buf->chunk_id & ((1 << GRN_II_N_CHUNK_VARIATION) - 1))
    << GRN_II_W_LEAST_CHUNK;
  const uint32_t chunk_capacity =
    buf->ii->chunk->header->segment_size - seg_offset;
  if (chunk_capacity < chunk_size) {
    ERR(GRN_NOT_ENOUGH_SPACE,
        "[ii][builder][buffer][assign] "
        "too large chunk size: "
        "chunk_size=<%" GRN_FMT_SIZE "> "
        "chunk_capacity=<%u> "
        "chunk_id=<%u> "
        "segment_id=<%u>",
        chunk_size,
        chunk_capacity,
        buf->chunk_id,
        buf->chunk_seg_id);
    return ctx->rc;
  }

  buf->chunk_seg_id = buf->chunk_id >> GRN_II_N_CHUNK_VARIATION;
  seg = grn_io_seg_ref(ctx, buf->ii->chunk, buf->chunk_seg_id);
  if (!seg) {
    if (ctx->rc == GRN_SUCCESS) {
      ERR(GRN_UNKNOWN_ERROR,
          "[ii][builder][buffer][assign] "
          "failed to access chunk segment: chunk_id=<%u> segment_id=<%u>",
          buf->chunk_id,
          buf->chunk_seg_id);
    }
    return ctx->rc;
  }
  buf->chunk = ((uint8_t *)seg) + seg_offset;
  buf->chunk_offset = 0;
  buf->chunk_size = chunk_size;

  buf->buf->header.chunk = buf->chunk_id;
  buf->buf->header.chunk_size = chunk_size;
  buf->buf->header.buffer_free = S_SEGMENT - sizeof(buffer_header);
  buf->buf->header.nterms = 0;
  buf->buf->header.nterms_void = 0;
  buf->ii->header.common->total_chunk_size += chunk_size;
  return GRN_SUCCESS;
}

/* grn_ii_builder_buffer_flush flushes a buffer. */
static grn_rc
grn_ii_builder_buffer_flush(grn_ctx *ctx, grn_ii_builder_buffer *buf)
{
  grn_ii *ii;

  buf->buf->header.buffer_free = S_SEGMENT - sizeof(buffer_header) -
                                 buf->buf->header.nterms * sizeof(buffer_term);
  GRN_LOG(ctx,
          GRN_LOG_DEBUG,
          "[ii][builder][buffer][flush] "
          "n_terms=<%u> "
          "chunk_offset=<%u> "
          "chunk_size=<%u> "
          "total=<%" GRN_FMT_INT64U "KB>",
          buf->buf->header.nterms,
          buf->chunk_offset,
          buf->buf->header.chunk_size,
          buf->ii->header.common->total_chunk_size >> 10);

  ii = buf->ii;
  grn_ii_builder_buffer_fin(ctx, buf);
  grn_ii_builder_buffer_init(ctx, buf, ii);
  return GRN_SUCCESS;
}

typedef struct {
  grn_ii *ii;       /* Building inverted index */
  grn_id tid;       /* Term ID */
  uint32_t n;       /* Number of integers in buffers */
  grn_id rid;       /* Record ID */
  uint32_t rid_gap; /* Record ID gap */
  uint64_t pos_sum; /* Sum of position gaps */

  uint32_t offset;      /* Write offset */
  uint32_t size;        /* Buffer size */
  grn_id *rid_buf;      /* Buffer for record IDs (to be freed) */
  uint32_t *sid_buf;    /* Buffer for section IDs (to be freed) */
  uint32_t *freq_buf;   /* Buffer for frequencies (to be freed) */
  uint32_t *weight_buf; /* Buffer for weights (to be freed) */

  uint32_t pos_offset; /* Write offset of pos_buf */
  uint32_t pos_size;   /* Buffer size of pos_buf */
  uint32_t *pos_buf;   /* Buffer for positions (to be freed) */

  size_t enc_offset; /* Write offset of enc_buf */
  size_t enc_size;   /* Buffer size of enc_buf */
  uint8_t *enc_buf;  /* Buffer for encoded data (to be freed) */
} grn_ii_builder_chunk;

/*
 * grn_ii_builder_chunk_init initializes a chunk. Note that an initialized
 * chunk must be finalized by grn_ii_builder_chunk_fin.
 */
static void
grn_ii_builder_chunk_init(grn_ctx *ctx, grn_ii_builder_chunk *chunk, grn_ii *ii)
{
  chunk->ii = ii;

  chunk->tid = GRN_ID_NIL;
  chunk->n = 0;
  chunk->rid = GRN_ID_NIL;
  chunk->rid_gap = 0;
  chunk->pos_sum = 0;

  chunk->offset = 0;
  chunk->size = 0;
  chunk->rid_buf = NULL;
  chunk->sid_buf = NULL;
  chunk->freq_buf = NULL;
  chunk->weight_buf = NULL;

  chunk->pos_offset = 0;
  chunk->pos_size = 0;
  chunk->pos_buf = NULL;

  chunk->enc_offset = 0;
  chunk->enc_size = 0;
  chunk->enc_buf = NULL;
}

/* grn_ii_builder_chunk_fin finalizes a chunk. */
static void
grn_ii_builder_chunk_fin(grn_ctx *ctx, grn_ii_builder_chunk *chunk)
{
  if (chunk->enc_buf) {
    GRN_FREE(chunk->enc_buf);
  }
  if (chunk->pos_buf) {
    GRN_FREE(chunk->pos_buf);
  }
  if (chunk->weight_buf) {
    GRN_FREE(chunk->weight_buf);
  }
  if (chunk->freq_buf) {
    GRN_FREE(chunk->freq_buf);
  }
  if (chunk->sid_buf) {
    GRN_FREE(chunk->sid_buf);
  }
  if (chunk->rid_buf) {
    GRN_FREE(chunk->rid_buf);
  }
}

/*
 * grn_ii_builder_chunk_clear clears stats except rid and buffers except
 * enc_buf.
 */
static void
grn_ii_builder_chunk_clear(grn_ctx *ctx, grn_ii_builder_chunk *chunk)
{
  chunk->n = 0;
  chunk->rid_gap = 0;
  chunk->pos_sum = 0;
  chunk->offset = 0;
  chunk->pos_offset = 0;
}

/*
 * grn_ii_builder_chunk_extend_bufs extends buffers except pos_buf and enc_buf.
 */
static grn_rc
grn_ii_builder_chunk_extend_bufs(grn_ctx *ctx,
                                 grn_ii_builder_chunk *chunk,
                                 uint32_t ii_flags)
{
  uint32_t *buf;
  uint32_t size = chunk->size ? chunk->size * 2 : 1;
  size_t n_bytes = size * sizeof(uint32_t);

  buf = (uint32_t *)GRN_REALLOC(chunk->rid_buf, n_bytes);
  if (!buf) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "failed to allocate memory for record IDs: n_bytes = %" GRN_FMT_SIZE,
        n_bytes);
    return ctx->rc;
  }
  chunk->rid_buf = buf;

  if (ii_flags & GRN_OBJ_WITH_SECTION) {
    buf = (uint32_t *)GRN_REALLOC(chunk->sid_buf, n_bytes);
    if (!buf) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "failed to allocate memory for section IDs:"
          " n_bytes = %" GRN_FMT_SIZE,
          n_bytes);
      return ctx->rc;
    }
    chunk->sid_buf = buf;
  }

  buf = (uint32_t *)GRN_REALLOC(chunk->freq_buf, n_bytes);
  if (!buf) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "failed to allocate memory for frequencies: n_bytes = %" GRN_FMT_SIZE,
        n_bytes);
    return ctx->rc;
  }
  chunk->freq_buf = buf;

  if (ii_flags & GRN_OBJ_WITH_WEIGHT) {
    buf = (uint32_t *)GRN_REALLOC(chunk->weight_buf, n_bytes);
    if (!buf) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "failed to allocate memory for weights: n_bytes = %" GRN_FMT_SIZE,
          n_bytes);
      return ctx->rc;
    }
    chunk->weight_buf = buf;
  }

  chunk->size = size;
  return GRN_SUCCESS;
}

/* grn_ii_builder_chunk_extend_pos_buf extends pos_buf. */
static grn_rc
grn_ii_builder_chunk_extend_pos_buf(grn_ctx *ctx, grn_ii_builder_chunk *chunk)
{
  uint32_t *buf;
  uint32_t size = chunk->pos_size ? chunk->pos_size * 2 : 1;
  size_t n_bytes = size * sizeof(uint32_t);
  buf = (uint32_t *)GRN_REALLOC(chunk->pos_buf, n_bytes);
  if (!buf) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "failed to allocate memory for positions: n_bytes = %" GRN_FMT_SIZE,
        n_bytes);
    return ctx->rc;
  }
  chunk->pos_buf = buf;
  chunk->pos_size = size;
  return GRN_SUCCESS;
}

/*
 * grn_ii_builder_chunk_reserve_enc_buf estimates a size that is enough to
 * store encoded data and allocates memory to enc_buf.
 */
static grn_rc
grn_ii_builder_chunk_reserve_enc_buf(grn_ctx *ctx,
                                     grn_ii_builder_chunk *chunk,
                                     uint32_t n_cinfos)
{
  size_t rich_size =
    (chunk->n + 4) * sizeof(uint32_t) + n_cinfos * sizeof(chunk_info);
  if (chunk->enc_size < rich_size) {
    size_t size = chunk->enc_size ? chunk->enc_size * 2 : 1;
    while (size < rich_size) {
      size *= 2;
    }
    auto buf = static_cast<uint8_t *>(GRN_REALLOC(chunk->enc_buf, size));
    if (!buf) {
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "failed to allocate memory for encoding: size = %" GRN_FMT_SIZE,
          size);
      return ctx->rc;
    }
    chunk->enc_buf = buf;
    chunk->enc_size = size;
  }
  chunk->enc_offset = 0;
  return GRN_SUCCESS;
}

/* grn_ii_builder_chunk_encode encodes a chunk buffer. */
static void
grn_ii_builder_chunk_encode_buf(grn_ctx *ctx,
                                grn_ii_builder_chunk *chunk,
                                grn_codec_data *data)
{
  data->offset = chunk->enc_size - (data->output - chunk->enc_buf);
  if (data->flags & DATA_USE_P_FOR_ENC) {
    data->output = grn_enc_p_for(ctx, data);
  } else {
    data->output = grn_enc_byte(ctx, data);
  }
  chunk->enc_offset = data->output - chunk->enc_buf;
}

/* grn_ii_builder_chunk_encode encodes a chunk. */
static grn_rc
grn_ii_builder_chunk_encode(grn_ctx *ctx,
                            grn_ii_builder_chunk *chunk,
                            chunk_info *cinfos,
                            uint32_t n_cinfos)
{
  grn_rc rc;
  uint8_t *p;
  uint8_t shift = 0, use_p_for_enc_flags = 0;

  /* Choose an encoding. */
  uint32_t rid_flags = 0;
  if (data_records_use_p_for_enc(chunk->offset, chunk->rid)) {
    rid_flags |= DATA_USE_P_FOR_ENC;
  }
  uint32_t rest_flags = 0;
  if (data_sections_use_p_for_enc(chunk->offset)) {
    rest_flags |= DATA_USE_P_FOR_ENC;
  }
  uint32_t pos_flags = DATA_ODD;

  /* record IDs */
  if (rid_flags & DATA_USE_P_FOR_ENC) {
    use_p_for_enc_flags |= 1 << shift;
  }
  shift++;
  /* section IDs */
  if (chunk->sid_buf) {
    if (rest_flags & DATA_USE_P_FOR_ENC) {
      use_p_for_enc_flags |= 1 << shift;
    }
    shift++;
  }
  /* term frequencies */
  if (rest_flags & DATA_USE_P_FOR_ENC) {
    use_p_for_enc_flags |= 1 << shift;
  }
  shift++;
  /* weights */
  if (chunk->weight_buf) {
    if (rest_flags & DATA_USE_P_FOR_ENC) {
      use_p_for_enc_flags |= 1 << shift;
    }
    shift++;
  }
  /* positions */
  if (chunk->pos_buf) {
    if (data_positions_use_p_for_enc(chunk->pos_offset, chunk->pos_sum)) {
      pos_flags |= DATA_USE_P_FOR_ENC;
      use_p_for_enc_flags |= 1 << shift;
    }
    shift++;
  }

  rc = grn_ii_builder_chunk_reserve_enc_buf(ctx, chunk, n_cinfos);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  /* Encode a header. */
  p = chunk->enc_buf;
  if (n_cinfos > 0) {
    uint32_t i;
    GRN_B_ENC(n_cinfos, p);
    for (i = 0; i < n_cinfos; i++) {
      GRN_B_ENC(cinfos[i].segno, p);
      GRN_B_ENC(cinfos[i].size, p);
      GRN_B_ENC(cinfos[i].dgap, p);
    }
  }
  if (use_p_for_enc_flags) {
    GRN_B_ENC(use_p_for_enc_flags << 1, p);
    GRN_B_ENC(chunk->offset, p);
    if (chunk->pos_buf) {
      GRN_B_ENC(chunk->pos_offset - chunk->offset, p);
    }
  } else {
    GRN_B_ENC((chunk->offset << 1) | 1, p);
  }
  chunk->enc_offset = p - chunk->enc_buf;

  /* Encode a body. */
  grn_codec_data data = {};
  data.ii = chunk->ii;
  data.term_id = chunk->tid;
  data.values = chunk->rid_buf;
  data.n_values = chunk->offset;
  data.flags = rid_flags;
  data.output = chunk->enc_buf + chunk->enc_offset;
  grn_ii_builder_chunk_encode_buf(ctx, chunk, &data);
  data.flags = rest_flags;
  if (chunk->sid_buf) {
    data.values = chunk->sid_buf;
    grn_ii_builder_chunk_encode_buf(ctx, chunk, &data);
  }
  data.values = chunk->freq_buf;
  grn_ii_builder_chunk_encode_buf(ctx, chunk, &data);
  if (chunk->weight_buf) {
    data.values = chunk->weight_buf;
    grn_ii_builder_chunk_encode_buf(ctx, chunk, &data);
  }
  if (chunk->pos_buf) {
    data.values = chunk->pos_buf;
    data.n_values = chunk->pos_offset;
    data.flags = pos_flags;
    grn_ii_builder_chunk_encode_buf(ctx, chunk, &data);
  }

  return GRN_SUCCESS;
}

namespace grn::ii {
  template <typename Each>
  grn_rc
  table_each_by_key(grn_ctx *ctx, grn_obj *table, Each &&each)
  {
    switch (table->header.type) {
    case GRN_TABLE_HASH_KEY:
      {
        auto hash = reinterpret_cast<grn_hash *>(table);
        if (grn_hash_size(ctx, hash) == 0) {
          return GRN_SUCCESS;
        }

        // grn_hash can't use GRN_CURSOR_BY_KEY. So we sort by key
        // manually. This will not be an ignorable overhead but it's
        // an easy implementation for now. We may use an offline index
        // construction algorithm that doesn't rely on
        // GRN_CURSOR_BY_KEY in a future but we haven't done it.
        auto sorted = grn_array_create(ctx, nullptr, sizeof(grn_id), 0);
        if (!sorted) {
          return ctx->rc;
        }
        grn_table_sort_optarg arg = {
          GRN_TABLE_SORT_ASC | GRN_TABLE_SORT_BY_KEY,
          nullptr,
          nullptr,
          nullptr,
          0,
        };
        grn_hash_sort(ctx, hash, -1, sorted, &arg);
        grn_rc rc = GRN_SUCCESS;
        GRN_ARRAY_EACH_BEGIN(ctx, sorted, cursor, GRN_ID_NIL, GRN_ID_MAX, id)
        {
          grn_id hash_id = GRN_ID_NIL;
          auto value_size =
            grn_array_get_value(ctx,
                                sorted,
                                id,
                                reinterpret_cast<void *>(&hash_id));
          if (value_size == 0) {
            continue;
          }
          rc = each(hash_id);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
        GRN_ARRAY_EACH_END(ctx, cursor);
        grn_array_close(ctx, sorted);
        return rc;
      }
    case GRN_TABLE_PAT_KEY:
      {
        auto pat = reinterpret_cast<grn_pat *>(table);
        grn_rc rc = GRN_SUCCESS;
        GRN_PAT_EACH_BEGIN(ctx, pat, cursor, id)
        {
          rc = each(id);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
        GRN_PAT_EACH_END(ctx, cursor);
        return rc;
      }
    case GRN_TABLE_DAT_KEY:
      {
        auto dat = reinterpret_cast<grn_dat *>(table);
        grn_rc rc = GRN_SUCCESS;
        GRN_DAT_EACH_BEGIN(ctx, dat, cursor, id)
        {
          rc = each(id);
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
        GRN_DAT_EACH_END(ctx, cursor);
        return rc;
      }
    default:
      // must not happen
      return GRN_INVALID_ARGUMENT;
    }
  }

  // Builds a block from multiple records. You can build a block by
  // calling flush() after you append multiple records by
  // append_record().
  class BlockBuilder {
  public:
    BlockBuilder(grn_ctx *parent_ctx,
                 grn_ctx *ctx,
                 grn_ii *ii,
                 const grn_ii_builder_options *options,
                 grn_obj *src_table,
                 grn_obj **srcs,
                 grn_obj **src_token_columns,
                 uint32_t n_srcs,
                 uint8_t sid_bits)
      : parent_ctx_(parent_ctx),
        ctx_(ctx),
        ii_(ii),
        options_(options),
        lexicon_(nullptr),
        have_tokenizer_(false),
        have_normalizers_(false),
        get_key_optimizable_(false),
        n_(0),
        rid_(GRN_ID_NIL),
        sid_(0),
        pos_(0),
        terms_(nullptr),
        n_terms_(0),
        max_n_terms_(0),
        terms_size_(0),
        src_table_(src_table),
        srcs_(srcs),
        src_token_columns_(src_token_columns),
        n_srcs_(n_srcs),
        sid_bits_(sid_bits),
        values_(n_srcs),
        n_processed_records_(0)
    {
      for (uint32_t i = 0; i < n_srcs_; i++) {
        GRN_VOID_INIT(&values_[i]);
      }
    }

    ~BlockBuilder()
    {
      for (uint32_t i = 0; i < n_srcs_; i++) {
        GRN_OBJ_FIN(ctx_, &values_[i]);
      }
      if (lexicon_) {
        grn_obj_close(ctx_, lexicon_);
      }
      finalize_terms();
      if (parent_ctx_) {
        grn_ctx_release_child(parent_ctx_, ctx_);
      }
    }

    // Prepares this builder.
    //
    // This must be called once before you call append_record().
    grn_rc
    prepare()
    {
      auto rc = create_lexicon();
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      return GRN_SUCCESS;
    }

    // Appends a record.
    //
    // `rid` must be ordered by ascending. The following is valid:
    //
    //    block_builder.append_record(1);
    //    block_builder.append_record(2);
    //    block_builder.append_record(3);
    //
    // But the following is invalid:
    //
    //    block_builder.append_record(1);
    //    block_builder.append_record(3);
    //    block_builder.append_record(2); // 2 is smaller than 3
    grn_rc
    append_record(grn_id rid)
    {
      for (size_t i = 0; i < n_srcs_; i++) {
        grn_obj *value = &values_[i];
        grn_obj *src = srcs_[i];
        grn_obj *token_column = src_token_columns_[i];
        if (token_column) {
          auto rc = grn_obj_reinit_for(ctx_, value, token_column);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          if (!grn_obj_get_value(ctx_, token_column, rid, value)) {
            if (ctx_->rc == GRN_SUCCESS) {
              auto ctx = ctx_;
              ERR(GRN_UNKNOWN_ERROR, "failed to get tokens: rid = %u", rid);
            }
            return ctx_->rc;
          }
          auto sid = static_cast<uint32_t>(i + 1);
          rc = append_tokens(rid, sid, value);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
        } else {
          auto rc = grn_obj_reinit_for(ctx_, value, src);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          if (GRN_OBJ_TABLEP(src)) {
            auto len = grn_table_get_key2(ctx_, src, rid, value);
            if (len <= 0) {
              if (ctx_->rc == GRN_SUCCESS) {
                auto ctx = ctx_;
                ERR(GRN_UNKNOWN_ERROR,
                    "failed to get key: rid = %u, len = %d",
                    rid,
                    len);
              }
              return ctx_->rc;
            }
          } else {
            if (!grn_obj_get_value(ctx_, src, rid, value)) {
              if (ctx_->rc == GRN_SUCCESS) {
                auto ctx = ctx_;
                ERR(GRN_UNKNOWN_ERROR, "failed to get value: rid = %u", rid);
              }
              return ctx_->rc;
            }
          }
          auto sid = static_cast<uint32_t>(i + 1);
          rc = append_obj(src, rid, sid, value);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
        }
      }
      ++n_processed_records_;
      return GRN_SUCCESS;
    }

    // Whether we need to flush this block.
    bool
    need_flush()
    {
      return n_ >= options_->block_threshold;
    }

    // Whether there are any data in this block.
    bool
    have_data()
    {
      return n_ > 0;
    }

    // Number of processed records in this block.
    size_t
    n_processed_records()
    {
      return n_processed_records_;
    }

    // Flushes all terms in this block.
    template <typename FlushTerm>
    grn_rc
    flush(FlushTerm &&flush_term)
    {
      if (n_ == 0) {
        /* Do nothing if there are no output data. */
        return GRN_SUCCESS;
      }

      // Flush terms.
      //
      // This assumes that both of each block builder (here) and
      // reader (see Builder::commit()) use the same order to write
      // built blocks and read the written blocks. This is implemented
      // by sorting by key (term) in both of local lexicons and the
      // global lexicon (ii_->lexicon). Sorting by key is efficient
      // with grn_pat and grn_dat but not with grn_hash. So we may
      // change this algorithm later.
      auto rc = table_each_by_key(ctx_, lexicon_, [&](grn_id tid) {
        return flush_term(&(terms_[tid - 1]), lexicon_, tid);
      });

      /* Clear the current data. */
      rc = grn_table_truncate(ctx_, lexicon_);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      rid_ = GRN_ID_NIL;
      n_terms_ = 0;
      n_ = 0;
      n_processed_records_ = 0;
      return GRN_SUCCESS;
    }

  private:
    grn_ctx *parent_ctx_;
    grn_ctx *ctx_;
    grn_ii *ii_;
    const grn_ii_builder_options *options_;

    grn_obj *lexicon_;      /* Block lexicon (to be closed) */
    bool have_tokenizer_;   /* Whether lexicon has tokenizer */
    bool have_normalizers_; /* Whether lexicon has at least one normalizers */
    bool get_key_optimizable_; /* Whether grn_table_get_key() is optimizable */

    uint32_t n_;   /* Number of integers appended to the current block */
    grn_id rid_;   /* Record ID */
    uint32_t sid_; /* Section ID */
    uint32_t pos_; /* Position */

    grn_ii_builder_term *terms_; /* Terms (to be freed) */
    uint32_t n_terms_;           /* Number of distinct terms */
    uint32_t max_n_terms_;       /* Maximum number of distinct terms */
    uint32_t terms_size_;        /* Buffer size of terms */

    grn_obj *src_table_; /* Source table */
    // Source columns. This refers source columns in Builder. So this
    // should not be freed.
    grn_obj **srcs_;
    // Source token columns. This refers source token columns in
    // Builder. So this should not be freed.
    grn_obj **src_token_columns_;
    uint32_t n_srcs_;  /* Number of source columns */
    uint8_t sid_bits_; /* Number of bits for section ID */

    // Buffers for source column values. We use one grn_obj per column
    // to reuse effectively.
    std::vector<grn_obj> values_;

    size_t n_processed_records_; /* Number of processed records */

    // Creates a block lexicon.
    grn_rc
    create_lexicon()
    {
      grn_table_flags flags;
      grn_obj *domain = grn_ctx_at(ctx_, ii_->lexicon->header.domain);
      grn_obj *range = grn_ctx_at(ctx_, DB_OBJ(ii_->lexicon)->range);

      auto rc = grn_table_get_info(ctx_,
                                   ii_->lexicon,
                                   &flags,
                                   nullptr,
                                   nullptr,
                                   nullptr,
                                   nullptr);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      flags &= ~GRN_OBJ_PERSISTENT;
      lexicon_ =
        grn_table_create(ctx_, nullptr, 0, nullptr, flags, domain, range);
      if (!lexicon_) {
        if (ctx_->rc == GRN_SUCCESS) {
          auto ctx = ctx_;
          ERR(GRN_UNKNOWN_ERROR, "[index] failed to create a block lexicon");
        }
        return ctx_->rc;
      }
      {
        grn_obj tokenizer;
        GRN_TEXT_INIT(&tokenizer, 0);
        grn_table_get_default_tokenizer_string(ctx_, ii_->lexicon, &tokenizer);
        if (GRN_TEXT_LEN(&tokenizer) > 0) {
          have_tokenizer_ = true;
          rc = grn_obj_set_info(ctx_,
                                lexicon_,
                                GRN_INFO_DEFAULT_TOKENIZER,
                                &tokenizer);
        }
        GRN_OBJ_FIN(ctx_, &tokenizer);
      }
      if (rc == GRN_SUCCESS) {
        grn_obj normalizers;
        GRN_TEXT_INIT(&normalizers, 0);
        grn_table_get_normalizers_string(ctx_, ii_->lexicon, &normalizers);
        if (GRN_TEXT_LEN(&normalizers) > 0) {
          have_normalizers_ = true;
          rc = grn_obj_set_info(ctx_,
                                lexicon_,
                                GRN_INFO_NORMALIZERS,
                                &normalizers);
        }
        GRN_OBJ_FIN(ctx_, &normalizers);
      }
      if (rc == GRN_SUCCESS) {
        grn_obj token_filters;
        GRN_TEXT_INIT(&token_filters, 0);
        grn_table_get_token_filters_string(ctx_, ii_->lexicon, &token_filters);
        if (GRN_TEXT_LEN(&token_filters) > 0) {
          rc = grn_obj_set_info(ctx_,
                                lexicon_,
                                GRN_INFO_TOKEN_FILTERS,
                                &token_filters);
        }
        GRN_OBJ_FIN(ctx_, &token_filters);
      }
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      if ((flags & GRN_OBJ_TABLE_TYPE_MASK) == GRN_OBJ_TABLE_PAT_KEY) {
        auto lexicon_pat = reinterpret_cast<grn_pat *>(lexicon_);
        get_key_optimizable_ = !grn_pat_is_key_encoded(ctx_, lexicon_pat);
        if (options_->lexicon_cache_size) {
          rc = grn_pat_cache_enable(ctx_,
                                    lexicon_pat,
                                    options_->lexicon_cache_size);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
        }
      } else {
        get_key_optimizable_ = true;
      }
      return GRN_SUCCESS;
    }

    void
    finalize_terms()
    {
      if (!terms_) {
        return;
      }

      uint32_t i;
      for (i = 0; i < max_n_terms_; i++) {
        grn_ii_builder_term_fin(ctx_, &(terms_[i]));
      }
      auto ctx = ctx_;
      GRN_FREE(terms_);
      /* To avoid double finalization. */
      terms_ = nullptr;
    }

    // Extends a buffer for terms in order to make terms[n_terms - 1]
    // available.
    grn_rc
    extend_terms(uint32_t new_n_terms)
    {
      if (new_n_terms <= n_terms_) {
        return GRN_SUCCESS;
      }

      if (new_n_terms > max_n_terms_) {
        uint32_t i;
        if (new_n_terms > terms_size_) {
          /* Resize builder->terms_ for new terms. */
          uint32_t new_terms_size = terms_size_ ? terms_size_ * 2 : 1;
          while (new_terms_size < new_n_terms) {
            new_terms_size *= 2;
          }
          auto ctx = ctx_;
          auto new_n_bytes = new_terms_size * sizeof(grn_ii_builder_term);
          auto new_terms = static_cast<grn_ii_builder_term *>(
            GRN_REALLOC(terms_, new_n_bytes));
          if (!new_terms) {
            ERR(GRN_NO_MEMORY_AVAILABLE,
                "failed to allocate memory for terms: n_bytes = %" GRN_FMT_SIZE,
                new_n_bytes);
            return ctx->rc;
          }
          terms_ = new_terms;
          terms_size_ = new_terms_size;
        }
        /* Initialize new terms. */
        for (i = max_n_terms_; i < new_n_terms; i++) {
          grn_ii_builder_term_init(ctx_, &(terms_[i]));
        }
        max_n_terms_ = new_n_terms;
      }

      n_ += new_n_terms - n_terms_;
      n_terms_ = new_n_terms;
      return GRN_SUCCESS;
    }

    // Gets a term associated with tid.
    grn_rc
    get_term(grn_id tid, grn_ii_builder_term **term)
    {
      uint32_t n_terms = tid;
      if (n_terms > n_terms_) {
        grn_rc rc = extend_terms(n_terms);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }
      *term = &(terms_[tid - 1]);
      return GRN_SUCCESS;
    }

    // Appends a token.
    grn_rc
    append_token(
      grn_id rid, uint32_t sid, uint32_t weight, grn_id tid, uint32_t pos)
    {
      uint32_t ii_flags = ii_->header.common->flags;
      grn_ii_builder_term *term;
      {
        auto rc = get_term(tid, &term);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }
      if (rid != term->rid || sid != term->sid) {
        uint64_t rsid;
        if (term->rid != GRN_ID_NIL) {
          if (ii_flags & GRN_OBJ_WITH_POSITION) {
            /* Append the end of positions. */
            auto rc = grn_ii_builder_term_append(ctx_, term, 0);
            if (rc != GRN_SUCCESS) {
              return rc;
            }
            n_++;
          } else {
            /* Append a frequency if positions are not available. */
            auto rc = grn_ii_builder_term_append(ctx_, term, term->pos_or_freq);
            if (rc != GRN_SUCCESS) {
              return rc;
            }
            n_++;
          }
        }
        rsid =
          (static_cast<uint64_t>(rid - term->rid) << sid_bits_) | (sid - 1);
        auto rc = grn_ii_builder_term_append(ctx_, term, rsid);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        n_++;
        if (ii_flags & GRN_OBJ_WITH_WEIGHT) {
          auto rc = grn_ii_builder_term_append(ctx_, term, weight);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          n_++;
        }
        term->rid = rid;
        term->sid = sid;
        term->pos_or_freq = 0;
      }
      if (ii_flags & GRN_OBJ_WITH_POSITION) {
        auto rc =
          grn_ii_builder_term_append(ctx_, term, pos - term->pos_or_freq);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        n_++;
        term->pos_or_freq = pos;
      } else {
        term->pos_or_freq++;
      }
      return GRN_SUCCESS;
    }

    void
    start_value(grn_id rid, uint32_t sid)
    {
      if (rid != rid_) {
        rid_ = rid;
        sid_ = sid;
        pos_ = 1;
      } else if (sid != sid_) {
        sid_ = sid;
        pos_ = 1;
      } else if (have_tokenizer_) {
        /* Insert a space between values. */
        pos_++;
      }
    }

    grn_rc
    append_tokens(grn_id rid, uint32_t sid, grn_obj *tokens)
    {
      start_value(rid, sid);
      grn_obj *src_lexicon = ii_->lexicon;
      size_t n_tokens = grn_uvector_size(ctx_, tokens);
      size_t i;
      for (i = 0; i < n_tokens; i++) {
        float weight;
        grn_id tid;
        grn_id src_tid =
          grn_uvector_get_element_record(ctx_, tokens, i, &weight);
        uint32_t token_value_size;
        const char *token_value =
          _grn_table_key(ctx_, src_lexicon, src_tid, &token_value_size);

        switch (lexicon_->header.type) {
        case GRN_TABLE_PAT_KEY:
          tid = grn_pat_add(ctx_,
                            reinterpret_cast<grn_pat *>(lexicon_),
                            token_value,
                            token_value_size,
                            NULL,
                            NULL);
          break;
        case GRN_TABLE_DAT_KEY:
          tid = grn_dat_add(ctx_,
                            reinterpret_cast<grn_dat *>(lexicon_),
                            token_value,
                            token_value_size,
                            NULL,
                            NULL);
          break;
        case GRN_TABLE_HASH_KEY:
          tid = grn_hash_add(ctx_,
                             reinterpret_cast<grn_hash *>(lexicon_),
                             token_value,
                             token_value_size,
                             NULL,
                             NULL);
          break;
        default:
          /* This case must not be happen. */
          tid = GRN_ID_NIL;
          break;
        }

        if (tid == GRN_ID_NIL) {
          /* TODO: Token value may not be a string. */
          auto ctx = ctx_;
          ERR(GRN_INVALID_ARGUMENT,
              "[ii][builder][append-tokens] failed to add a token: <%.*s>",
              static_cast<int>(token_value_size),
              token_value);
          return ctx->rc;
        }
        uint32_t pos = pos_ + i;
        grn_rc rc =
          append_token(rid, sid, static_cast<uint32_t>(weight), tid, pos);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }
      return GRN_SUCCESS;
    }

    // Appends a value. Note that values must be appended in ascending
    // rid and sid order.
    grn_rc
    append_value(grn_obj *src,
                 grn_id rid,
                 uint32_t sid,
                 uint32_t weight,
                 const char *value,
                 uint32_t value_size,
                 grn_id value_domain,
                 bool force_as_is)
    {
      uint32_t pos = 0;
      grn_token_cursor *cursor;
      start_value(rid, sid);
      if (value_size == 0) {
        pos = pos_;
      } else {
        if (force_as_is || (!have_tokenizer_ && !have_normalizers_)) {
          grn_id tid;
          uint32_t max_key_size = 0;
          bool too_large_key = false;
          switch (lexicon_->header.type) {
          case GRN_TABLE_PAT_KEY:
            if (value_size >= GRN_TABLE_MAX_KEY_SIZE) {
              tid = GRN_ID_NIL;
              max_key_size = GRN_TABLE_MAX_KEY_SIZE;
              too_large_key = true;
            } else {
              tid = grn_pat_add(ctx_,
                                reinterpret_cast<grn_pat *>(lexicon_),
                                value,
                                value_size,
                                NULL,
                                NULL);
            }
            break;
          case GRN_TABLE_DAT_KEY:
            if (value_size >= GRN_TABLE_MAX_KEY_SIZE) {
              tid = GRN_ID_NIL;
              max_key_size = GRN_TABLE_MAX_KEY_SIZE;
              too_large_key = true;
            } else {
              tid = grn_dat_add(ctx_,
                                reinterpret_cast<grn_dat *>(lexicon_),
                                value,
                                value_size,
                                NULL,
                                NULL);
            }
            break;
          case GRN_TABLE_HASH_KEY:
            {
              auto lexicon_hash = reinterpret_cast<grn_hash *>(lexicon_);
              if (value_size > lexicon_hash->key_size) {
                tid = GRN_ID_NIL;
                max_key_size = lexicon_hash->key_size;
                too_large_key = true;
              } else {
                tid = grn_hash_add(ctx_,
                                   lexicon_hash,
                                   value,
                                   value_size,
                                   NULL,
                                   NULL);
              }
            }
            break;
          case GRN_TABLE_NO_KEY:
            tid = *reinterpret_cast<const grn_id *>(value);
            break;
          default:
            tid = GRN_ID_NIL;
            break;
          }
          if (too_large_key) {
            auto ctx = ctx_;
            GRN_DEFINE_NAME(src);
            grn_obj record;
            GRN_RECORD_INIT(&record, 0, DB_OBJ(src_table_)->id);
            GRN_RECORD_SET(ctx, &record, rid);
            grn_obj inspected_record;
            GRN_TEXT_INIT(&inspected_record, 0);
            grn_inspect(ctx, &inspected_record, &record);
            GRN_LOG(ctx,
                    GRN_LOG_WARNING,
                    "[ii][builder][append-value][%.*s] ignore too long token. "
                    "Token must be less than or equal to %u: <%u>: %.*s",
                    name_size,
                    name,
                    max_key_size,
                    value_size,
                    (int)GRN_TEXT_LEN(&inspected_record),
                    GRN_TEXT_VALUE(&inspected_record));
            GRN_OBJ_FIN(ctx, &inspected_record);
            GRN_OBJ_FIN(ctx, &record);
          }
          if (tid != GRN_ID_NIL) {
            pos = pos_;
            auto rc = append_token(rid, sid, weight, tid, pos);
            if (rc != GRN_SUCCESS) {
              return rc;
            }
          }
        } else {
          cursor = grn_token_cursor_open(ctx_,
                                         lexicon_,
                                         value,
                                         value_size,
                                         GRN_TOKEN_ADD,
                                         0);
          if (!cursor) {
            if (ctx_->rc == GRN_SUCCESS) {
              auto ctx = ctx_;
              ERR(GRN_UNKNOWN_ERROR,
                  "grn_token_cursor_open failed: value = <%.*s>",
                  value_size,
                  value);
            }
            return ctx_->rc;
          }
          grn_token_cursor_set_query_domain(ctx_, cursor, value_domain);
          while (cursor->status == GRN_TOKEN_CURSOR_DOING) {
            grn_id tid = grn_token_cursor_next(ctx_, cursor);
            if (tid != GRN_ID_NIL) {
              pos = pos_ + cursor->pos;
              auto rc = append_token(rid, sid, weight, tid, pos);
              if (rc != GRN_SUCCESS) {
                break;
              }
            }
          }
          grn_token_cursor_close(ctx_, cursor);
        }
      }
      pos_ = pos + 1;
      return ctx_->rc;
    }

    // Appends a BULK, UVECTOR or VECTOR object.
    grn_rc
    append_obj(grn_obj *src, grn_id rid, uint32_t sid, grn_obj *obj)
    {
      switch (obj->header.type) {
      case GRN_BULK:
        if (grn_id_maybe_table(ctx_, obj->header.domain)) {
          grn_id id = GRN_RECORD_VALUE(obj);
          grn_obj key_buffer;
          GRN_VOID_INIT(&key_buffer);
          uint32_t key_size;
          const char *key;
          if (get_key_optimizable_) {
            key = _grn_table_key(ctx_, ii_->lexicon, id, &key_size);
          } else {
            GRN_BULK_REWIND(&key_buffer);
            grn_table_get_key2(ctx_, ii_->lexicon, id, &key_buffer);
            key = GRN_BULK_HEAD(&key_buffer);
            key_size = GRN_BULK_VSIZE(&key_buffer);
          }
          grn_rc rc = append_value(src,
                                   rid,
                                   sid,
                                   0,
                                   key,
                                   key_size,
                                   ii_->lexicon->header.domain,
                                   true);
          GRN_OBJ_FIN(ctx_, &key_buffer);
          return rc;
        } else {
          return append_value(src,
                              rid,
                              sid,
                              0,
                              GRN_TEXT_VALUE(obj),
                              GRN_TEXT_LEN(obj),
                              obj->header.domain,
                              false);
        }
      case GRN_UVECTOR:
        {
          grn_rc rc = GRN_SUCCESS;
          uint32_t i, n_values = grn_uvector_size(ctx_, obj);
          if (grn_id_maybe_table(ctx_, obj->header.domain)) {
            grn_obj key_buffer;
            GRN_VOID_INIT(&key_buffer);
            for (i = 0; i < n_values; i++) {
              grn_id id;
              float weight;
              id = grn_uvector_get_element_record(ctx_, obj, i, &weight);
              const char *key = nullptr;
              uint32_t key_size = 0;
              if (id == GRN_ID_NIL) {
                key = nullptr;
                key_size = 0;
              } else if (get_key_optimizable_) {
                key = _grn_table_key(ctx_, ii_->lexicon, id, &key_size);
              } else {
                GRN_BULK_REWIND(&key_buffer);
                grn_table_get_key2(ctx_, ii_->lexicon, id, &key_buffer);
                key = GRN_BULK_HEAD(&key_buffer);
                key_size = GRN_BULK_VSIZE(&key_buffer);
              }
              rc = append_value(src,
                                rid,
                                sid,
                                (uint32_t)weight,
                                key,
                                key_size,
                                ii_->lexicon->header.domain,
                                true);
              if (rc != GRN_SUCCESS) {
                break;
              }
            }
            GRN_OBJ_FIN(ctx_, &key_buffer);
          } else {
            const char *p = GRN_BULK_HEAD(obj);
            uint32_t value_size = grn_uvector_element_size(ctx_, obj);
            for (i = 0; i < n_values; i++) {
              rc = append_value(src,
                                rid,
                                sid,
                                0,
                                p,
                                value_size,
                                obj->header.domain,
                                false);
              if (rc != GRN_SUCCESS) {
                break;
              }
              p += value_size;
            }
          }
          return rc;
        }
      case GRN_VECTOR:
        if (obj->u.v.body) {
          /*
           * Note that the following sections and n_sections don't correspond to
           * source columns.
           */
          int i, n_secs = obj->u.v.n_sections;
          grn_section *secs = obj->u.v.sections;
          const char *head = GRN_BULK_HEAD(obj->u.v.body);
          for (i = 0; i < n_secs; i++) {
            grn_rc rc;
            grn_section *sec = &secs[i];
            if (sec->length == 0) {
              continue;
            }
            if ((ii_->header.common->flags & GRN_OBJ_WITH_SECTION) &&
                have_tokenizer_) {
              sid = i + 1;
            }
            rc = append_value(src,
                              rid,
                              sid,
                              (uint32_t)(sec->weight),
                              head + sec->offset,
                              sec->length,
                              sec->domain,
                              false);
            if (rc != GRN_SUCCESS) {
              return rc;
            }
          }
        }
        return GRN_SUCCESS;
      default:
        {
          auto ctx = ctx_;
          ERR(GRN_INVALID_ARGUMENT, "[index] invalid object assigned as value");
          return ctx->rc;
        }
      }
    }
  };

  struct Builder {
    Builder(grn_ctx *ctx, grn_ii *ii, const grn_ii_builder_options *options)
      : ctx_(ctx),
        ii_(ii)
    {
      progress_needed_ = (grn_ctx_get_progress_callback(ctx_) != nullptr);
      if (progress_needed_) {
        progress_.type = GRN_PROGRESS_INDEX;
        progress_.value.index.phase = GRN_PROGRESS_INDEX_INITIALIZE;
        progress_.value.index.n_target_records = 0;
        progress_.value.index.n_processed_records = 0;
        progress_.value.index.n_target_terms = 0;
        progress_.value.index.n_processed_terms = 0;
        grn_ctx_call_progress_callback(ctx, &progress_);
      }

      if (options) {
        options_ = *options;
      } else {
        options_ = grn_ii_builder_default_options;
      }

      if (grn_ii_builder_block_threshold_force > 0) {
        options_.block_threshold = grn_ii_builder_block_threshold_force;
      }
      grn_ii_builder_options_fix(&options_);

      src_table_ = nullptr;
      n_records_ = 0;
      srcs_ = nullptr;
      src_token_columns_ = nullptr;
      n_srcs_ = 0;
      sid_bits_ = 0;
      sid_mask_ = 0;

      path_[0] = '\0';
      fd_ = -1;
      file_buf_ = nullptr;
      file_buf_offset_ = 0;

      blocks_ = nullptr;
      n_blocks_ = 0;
      blocks_size_ = 0;

      grn_ii_builder_buffer_init(ctx, &buf_, ii);
      grn_ii_builder_chunk_init(ctx, &chunk_, ii);

      df_ = 0;
      cinfos_ = nullptr;
      n_cinfos_ = 0;
      cinfos_size_ = 0;
    }

    ~Builder()
    {
      if (progress_needed_) {
        progress_.value.index.phase = GRN_PROGRESS_INDEX_FINALIZE;
        grn_ctx_call_progress_callback(ctx_, &progress_);
      }

      if (cinfos_) {
        auto ctx = ctx_;
        GRN_FREE(cinfos_);
      }
      grn_ii_builder_chunk_fin(ctx_, &chunk_);
      grn_ii_builder_buffer_fin(ctx_, &buf_);
      if (blocks_) {
        uint32_t i;
        for (i = 0; i < n_blocks_; i++) {
          grn_ii_builder_block_fin(ctx_, &(blocks_[i]));
        }
        auto ctx = ctx_;
        GRN_FREE(blocks_);
      }
      if (file_buf_) {
        auto ctx = ctx_;
        GRN_FREE(file_buf_);
      }
      if (fd_ != -1) {
        grn_close(fd_);
        if (grn_unlink(path_) == 0) {
          GRN_LOG(ctx_,
                  GRN_LOG_INFO,
                  "[ii][builder][fin] removed path: <%s>",
                  path_);
        } else {
          auto ctx = ctx_;
          ERRNO_ERR("[ii][builder][fin] failed to remove path: <%s>", path_);
        }
      }
      if (srcs_) {
        uint32_t i;
        for (i = 0; i < n_srcs_; i++) {
          grn_obj_unref(ctx_, srcs_[i]);
        }
        auto ctx = ctx_;
        GRN_FREE(srcs_);
      }
      if (src_token_columns_) {
        uint32_t i;
        for (i = 0; i < n_srcs_; i++) {
          grn_obj_unref(ctx_, src_token_columns_[i]);
        }
        auto ctx = ctx_;
        GRN_FREE(src_token_columns_);
      }
      if (src_table_) {
        grn_obj_unref(ctx_, src_table_);
      }
      if (progress_needed_) {
        progress_.value.index.phase = GRN_PROGRESS_INDEX_DONE;
        grn_ctx_call_progress_callback(ctx_, &progress_);
      }
    }

    // Flushes buffered data as a block.
    grn_rc
    flush_file_buf()
    {
      if (file_buf_offset_) {
        auto size = grn_write(fd_, file_buf_, file_buf_offset_);
        if (static_cast<uint64_t>(size) != file_buf_offset_) {
          auto ctx = ctx_;
          SERR("failed to write data: expected = %u, actual = %" GRN_FMT_INT64D,
               file_buf_offset_,
               static_cast<int64_t>(size));
        }
        file_buf_offset_ = 0;
      }
      return GRN_SUCCESS;
    }

    // Creates a temporary file and allocates memory for buffered
    // output.
    grn_rc
    create_file()
    {
      auto ctx = ctx_;
      grn_snprintf(path_,
                   PATH_MAX,
                   PATH_MAX,
                   "%sXXXXXX",
                   grn_io_path(ii_->seg));
      fd_ = grn_mkstemp(path_);
      if (fd_ == -1) {
        SERR("failed to create a temporary file: path = \"%s\"", path_);
        return ctx->rc;
      }
      file_buf_ =
        reinterpret_cast<uint8_t *>(GRN_MALLOC(options_.file_buf_size));
      if (!file_buf_) {
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "failed to allocate memory for buffered output: size = %u",
            options_.file_buf_size);
        return ctx->rc;
      }
      return GRN_SUCCESS;
    }

    // Flushes a term and clears it
    grn_rc
    flush_term(grn_ii_builder_term *term,
               grn_obj *local_lexicon,
               grn_id local_tid)
    {
      /* Append sentinels. */
      if (term->rid != GRN_ID_NIL) {
        grn_rc rc;
        if (ii_->header.common->flags & GRN_OBJ_WITH_POSITION) {

          rc = grn_ii_builder_term_append(ctx_, term, 0);
        } else {
          rc = grn_ii_builder_term_append(ctx_, term, term->pos_or_freq);
        }
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }
      {
        auto rc = grn_ii_builder_term_append(ctx_, term, 0);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }

      {
        /* Put the global term ID. */
        int key_size;
        char key[GRN_TABLE_MAX_KEY_SIZE];
        uint8_t *p;
        uint32_t rest, value;
        grn_id global_tid = GRN_ID_NIL;
        key_size = grn_table_get_key(ctx_,
                                     local_lexicon,
                                     local_tid,
                                     key,
                                     GRN_TABLE_MAX_KEY_SIZE);
        if (!key_size) {
          if (ctx_->rc == GRN_SUCCESS) {
            auto ctx = ctx_;
            ERR(GRN_UNKNOWN_ERROR, "failed to get key: tid = %u", local_tid);
          }
          return ctx_->rc;
        }
        /* Don't normalize key. */
        switch (ii_->lexicon->header.type) {
        case GRN_TABLE_PAT_KEY:
          GRN_TABLE_LOCK_BEGIN(ctx_, ii_->lexicon)
          {
            global_tid = grn_pat_add(ctx_,
                                     reinterpret_cast<grn_pat *>(ii_->lexicon),
                                     key,
                                     key_size,
                                     nullptr,
                                     nullptr);
          }
          GRN_TABLE_LOCK_END(ctx_);
          break;
        case GRN_TABLE_DAT_KEY:
          GRN_TABLE_LOCK_BEGIN(ctx_, ii_->lexicon)
          {
            global_tid = grn_dat_add(ctx_,
                                     reinterpret_cast<grn_dat *>(ii_->lexicon),
                                     key,
                                     key_size,
                                     nullptr,
                                     nullptr);
          }
          GRN_TABLE_LOCK_END(ctx_);
          break;
        case GRN_TABLE_HASH_KEY:
          GRN_TABLE_LOCK_BEGIN(ctx_, ii_->lexicon)
          {
            global_tid =
              grn_hash_add(ctx_,
                           reinterpret_cast<grn_hash *>(ii_->lexicon),
                           key,
                           key_size,
                           nullptr,
                           nullptr);
          }
          GRN_TABLE_LOCK_END(ctx_);
          break;
        default:
          global_tid = GRN_ID_NIL;
          break;
        }
        if (global_tid == GRN_ID_NIL) {
          if (ctx_->rc == GRN_SUCCESS) {
            auto ctx = ctx_;
            ERR(GRN_UNKNOWN_ERROR,
                "failed to get global term ID: tid = %u, key = \"%.*s\"",
                local_tid,
                key_size,
                key);
          }
          return ctx_->rc;
        }

        rest = options_.file_buf_size - file_buf_offset_;
        if (rest < 10) {
          auto rc = flush_file_buf();
          if (rc != GRN_SUCCESS) {
            return rc;
          }
        }
        value = global_tid;
        p = file_buf_ + file_buf_offset_;
        if (value < 1U << 5) {
          p[0] = static_cast<uint8_t>(value);
          file_buf_offset_++;
        } else if (value < 1U << 13) {
          p[0] = static_cast<uint8_t>((value & 0x1f) | (1 << 5));
          p[1] = static_cast<uint8_t>(value >> 5);
          file_buf_offset_ += 2;
        } else {
          uint8_t i, n;
          if (value < 1U << 21) {
            n = 3;
          } else if (value < 1U << 29) {
            n = 4;
          } else {
            n = 5;
          }
          p[0] = static_cast<uint8_t>(value & 0x1f) | ((n - 1) << 5);
          value >>= 5;
          for (i = 1; i < n; i++) {
            p[i] = static_cast<uint8_t>(value);
            value >>= 8;
          }
          file_buf_offset_ += n;
        }
      }

      /* Flush a term buffer. */
      auto term_buf = grn_ii_builder_term_get_buf(term);
      if (term->offset > options_.file_buf_size) {
        ssize_t size;
        auto rc = flush_file_buf();
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        size = grn_write(fd_, term_buf, term->offset);
        if (static_cast<uint64_t>(size) != term->offset) {
          auto ctx = ctx_;
          SERR("failed to write data: expected = %u, actual = %" GRN_FMT_INT64D,
               term->offset,
               static_cast<int64_t>(size));
        }
      } else {
        uint32_t rest = options_.file_buf_size - file_buf_offset_;
        if (term->offset <= rest) {
          grn_memcpy(file_buf_ + file_buf_offset_, term_buf, term->offset);
          file_buf_offset_ += term->offset;
        } else {
          grn_memcpy(file_buf_ + file_buf_offset_, term_buf, rest);
          file_buf_offset_ += rest;
          auto rc = flush_file_buf();
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          file_buf_offset_ = term->offset - rest;
          grn_memcpy(file_buf_, term_buf + rest, file_buf_offset_);
        }
      }
      grn_ii_builder_term_reinit(ctx_, term);
      return GRN_SUCCESS;
    }

    // Registers a block.
    grn_rc
    register_block()
    {
      uint64_t file_offset = grn_lseek(fd_, 0, SEEK_CUR);
      if (file_offset == static_cast<uint64_t>(-1)) {
        auto ctx = ctx_;
        SERR("failed to get file offset");
        return ctx->rc;
      }
      if (n_blocks_ >= blocks_size_) {
        uint32_t new_blocks_size = 1;
        while (new_blocks_size <= n_blocks_) {
          new_blocks_size *= 2;
        }
        auto ctx = ctx_;
        auto n_bytes = new_blocks_size * sizeof(grn_ii_builder_block);
        auto new_blocks = reinterpret_cast<grn_ii_builder_block *>(
          GRN_REALLOC(blocks_, n_bytes));
        if (!new_blocks) {
          ERR(GRN_NO_MEMORY_AVAILABLE,
              "failed to allocate memory for block: n_bytes = %" GRN_FMT_SIZE,
              n_bytes);
          return ctx->rc;
        }
        blocks_ = new_blocks;
        blocks_size_ = new_blocks_size;
      }
      auto block = &(blocks_[n_blocks_]);
      grn_ii_builder_block_init(ctx_, block);
      if (n_blocks_ == 0) {
        block->offset = 0;
      } else {
        auto prev_block = &(blocks_[n_blocks_ - 1]);
        block->offset = prev_block->offset + prev_block->rest;
      }
      block->rest = static_cast<uint32_t>(file_offset - block->offset);
      n_blocks_++;
      return GRN_SUCCESS;
    }

    // Flushes a block to a temporary file.
    grn_rc
    flush_block_builder(BlockBuilder *block_builder)
    {
      auto rc = block_builder->flush(
        [&](grn_ii_builder_term *term, grn_obj *lexicon, grn_id tid) {
          if (fd_ == -1) {
            auto rc = create_file();
            if (rc != GRN_SUCCESS) {
              return rc;
            }
          }
          return flush_term(term, lexicon, tid);
        });
      if (rc == GRN_SUCCESS) {
        rc = flush_file_buf();
      }
      if (rc == GRN_SUCCESS) {
        // Register a block in a temporary file.
        rc = register_block();
      }
      return rc;
    }

    // Reads values from source columns and appends the values in parallel.
    grn_rc
    append_srcs_parallel(grn::TaskExecutor *executor, size_t n_records_per_task)
    {
      std::mutex mutex;
      std::condition_variable cv;
      std::map<int, std::unique_ptr<BlockBuilder>> processed_block_builders;
      auto limit = static_cast<int>(n_records_per_task);
      for (int offset = 0; offset < static_cast<int>(n_records_);
           offset += limit) {
        auto execute = [&, offset, limit]() {
          auto child_ctx = grn_ctx_pull_child(ctx_);
          auto block_builder =
            std::make_unique<BlockBuilder>(ctx_,
                                           child_ctx,
                                           ii_,
                                           &options_,
                                           src_table_,
                                           srcs_,
                                           src_token_columns_,
                                           n_srcs_,
                                           sid_bits_);
          auto rc = block_builder->prepare();
          if (rc != GRN_SUCCESS) {
            return false;
          }
          auto cursor = grn_table_cursor_open(child_ctx,
                                              src_table_,
                                              nullptr,
                                              0,
                                              nullptr,
                                              0,
                                              offset,
                                              limit,
                                              GRN_CURSOR_BY_ID);
          if (cursor) {
            while (true) {
              auto rid = grn_table_cursor_next(child_ctx, cursor);
              if (rid == GRN_ID_NIL) {
                break;
              }
              rc = block_builder->append_record(rid);
              if (rc != GRN_SUCCESS) {
                break;
              }
            }
            grn_table_cursor_close(child_ctx, cursor);
          }
          {
            std::lock_guard<std::mutex> lock(mutex);
            processed_block_builders.insert(
              std::make_pair(offset, std::move(block_builder)));
          }
          cv.notify_one();
          return rc == GRN_SUCCESS;
        };
        if (!executor->execute(offset,
                               execute,
                               "[index][builder][append_src][parallel]")) {
          executor->wait_all();
          return ctx_->rc;
        }
      }

      for (int offset = 0; offset < static_cast<int>(n_records_);
           offset += limit) {
        std::unique_ptr<BlockBuilder> block_builder;
        {
          std::unique_lock<std::mutex> lock(mutex);
          cv.wait(lock, [&] {
            return ctx_->rc != GRN_SUCCESS ||
                   processed_block_builders.find(offset) !=
                     processed_block_builders.end();
          });
          if (ctx_->rc == GRN_SUCCESS) {
            auto iter = processed_block_builders.find(offset);
            block_builder = std::move(iter->second);
            processed_block_builders.erase(iter);
          }
        }
        if (ctx_->rc != GRN_SUCCESS) {
          break;
        }
        if (!block_builder) {
          break;
        }
        auto n_processed_records = block_builder->n_processed_records();
        if (block_builder->have_data()) {
          auto rc = flush_block_builder(block_builder.get());
          if (rc != GRN_SUCCESS) {
            break;
          }
        }
        if (progress_needed_) {
          progress_.value.index.n_processed_records += n_processed_records;
          grn_ctx_call_progress_callback(ctx_, &progress_);
        }
      }
      executor->wait_all();
      return ctx_->rc;
    }

    // Reads values from source columns and appends the values in sequential.
    grn_rc
    append_srcs_sequential()
    {
      BlockBuilder block_builder(nullptr,
                                 ctx_,
                                 ii_,
                                 &options_,
                                 src_table_,
                                 srcs_,
                                 src_token_columns_,
                                 n_srcs_,
                                 sid_bits_);
      auto rc = block_builder.prepare();
      if (rc != GRN_SUCCESS) {
        return rc;
      }

      /* Create a cursor to get records in the ID order. */
      GRN_TABLE_EACH_BEGIN_FLAGS(ctx_,
                                 src_table_,
                                 cursor,
                                 rid,
                                 GRN_CURSOR_BY_ID)
      {
        /* Read source values and append it. */
        rc = block_builder.append_record(rid);
        if (rc == GRN_SUCCESS && block_builder.need_flush()) {
          rc = flush_block_builder(&block_builder);
        }
        if (rc != GRN_SUCCESS) {
          break;
        }
        if (progress_needed_) {
          progress_.value.index.n_processed_records++;
          grn_ctx_call_progress_callback(ctx_, &progress_);
        }
      }
      GRN_TABLE_EACH_END(ctx_, cursor);
      if (rc == GRN_SUCCESS && block_builder.have_data()) {
        rc = flush_block_builder(&block_builder);
      }
      return rc;
    }

    // Reads values from source columns and appends the values.
    grn_rc
    append_srcs()
    {
      auto task_executor = grn_ctx_get_task_executor(ctx_);
      if (!task_executor->is_parallel()) {
        return append_srcs_sequential();
      }

      // This is a heuristic rule. We may want to revisit this.
      const size_t n_postings_per_source = 1024; // No reason.
      const size_t max_n_records_per_task =
        options_.block_threshold / (n_postings_per_source * n_srcs_);
      const size_t max_n_records_per_worker =
        n_records_ / task_executor->get_n_workers();
      const size_t min_n_records_per_task = 10240; // No reason.
      const size_t n_records_per_task =
        std::max(std::min(max_n_records_per_task, max_n_records_per_worker),
                 min_n_records_per_task);
      if (n_records_ < n_records_per_task) {
        return append_srcs_sequential();
      } else {
        return append_srcs_parallel(task_executor, n_records_per_task);
      }
    }

    // Sets a source table.
    grn_rc
    set_src_table()
    {
      src_table_ = grn_ctx_at(ctx_, DB_OBJ(ii_)->range);
      if (!src_table_) {
        if (ctx_->rc == GRN_SUCCESS) {
          auto ctx = ctx_;
          ERR(GRN_INVALID_ARGUMENT,
              "source table is null: range = %d",
              DB_OBJ(ii_)->range);
        }
        return ctx_->rc;
      }
      n_records_ = grn_table_size(ctx_, src_table_);
      return GRN_SUCCESS;
    }

    // Calculates sid_bits_ and sid_mask_.
    grn_rc
    set_sid_bits()
    {
      /* Calculate the number of bits required to represent a section ID. */
      if (n_srcs_ == 1 && grn_table_have_tokenizer(ctx_, ii_->lexicon) &&
          (srcs_[0]->header.flags & GRN_OBJ_COLUMN_VECTOR) != 0) {
        /* If the source column is a vector column and the index has a
         * tokenizer, the maximum sid equals to the maximum number of
         * elements. */
        size_t max_elems = 0;
        grn_table_cursor *cursor;
        grn_obj obj;
        cursor = grn_table_cursor_open(ctx_,
                                       src_table_,
                                       NULL,
                                       0,
                                       NULL,
                                       0,
                                       0,
                                       -1,
                                       GRN_CURSOR_BY_ID);
        if (!cursor) {
          if (ctx_->rc == GRN_SUCCESS) {
            auto ctx = ctx_;
            ERR(GRN_OBJECT_CORRUPT, "[index] failed to open table cursor");
          }
          return ctx_->rc;
        }
        GRN_TEXT_INIT(&obj, 0);
        for (;;) {
          grn_id rid = grn_table_cursor_next(ctx_, cursor);
          if (rid == GRN_ID_NIL) {
            break;
          }
          GRN_BULK_REWIND(&obj);
          if (!grn_obj_get_value(ctx_, srcs_[0], rid, &obj)) {
            continue;
          }
          if (obj.u.v.n_sections > max_elems) {
            max_elems = obj.u.v.n_sections;
          }
        }
        GRN_OBJ_FIN(ctx_, &obj);
        grn_table_cursor_close(ctx_, cursor);
        while ((static_cast<uint32_t>(1) << sid_bits_) < max_elems) {
          sid_bits_++;
        }
      }
      if (sid_bits_ == 0) {
        while ((static_cast<uint32_t>(1) << sid_bits_) < n_srcs_) {
          sid_bits_++;
        }
      }
      sid_mask_ = (static_cast<uint64_t>(1) << sid_bits_) - 1;
      return GRN_SUCCESS;
    }

    // Sets source columns.
    grn_rc
    set_srcs()
    {
      n_srcs_ = ii_->obj.source_size / sizeof(grn_id);
      auto source = static_cast<grn_id *>(ii_->obj.source);
      if (!source || n_srcs_ == 0) {
        auto ctx = ctx_;
        ERR(GRN_INVALID_ARGUMENT,
            "source is not available: source = %p, source_size = %u",
            ii_->obj.source,
            ii_->obj.source_size);
        return ctx->rc;
      }
      {
        auto ctx = ctx_;
        srcs_ = GRN_MALLOCN(grn_obj *, n_srcs_);
      }
      if (!srcs_) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      for (size_t i = 0; i < n_srcs_; i++) {
        srcs_[i] = grn_ctx_at(ctx_, source[i]);
        if (!srcs_[i]) {
          if (ctx_->rc == GRN_SUCCESS) {
            auto ctx = ctx_;
            ERR(GRN_OBJECT_CORRUPT, "source not found: id = %d", source[i]);
          }
          return ctx_->rc;
        }
      }
      {
        auto ctx = ctx_;
        src_token_columns_ = GRN_MALLOCN(grn_obj *, n_srcs_);
      }
      if (!src_token_columns_) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      {
        grn_obj token_columns;
        GRN_PTR_INIT(&token_columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
        for (size_t i = 0; i < n_srcs_; i++) {
          src_token_columns_[i] = nullptr;
          auto src = srcs_[i];
          grn_column_get_all_token_columns(ctx_, src, &token_columns);
          size_t n_token_columns = GRN_PTR_VECTOR_SIZE(&token_columns);
          for (size_t j = 0; j < n_token_columns; j++) {
            grn_obj *token_column = GRN_PTR_VALUE_AT(&token_columns, j);
            if (!src_token_columns_[i] &&
                DB_OBJ(token_column)->range == DB_OBJ(ii_->lexicon)->id) {
              src_token_columns_[i] = token_column;
            } else {
              grn_obj_unref(ctx_, token_column);
            }
          }
          GRN_BULK_REWIND(&token_columns);
        }
        GRN_OBJ_FIN(ctx_, &token_columns);
      }
      return set_sid_bits();
    }

    // Appends values in source columns.
    grn_rc
    append_source()
    {
      auto rc = set_src_table();
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      if (progress_needed_) {
        progress_.value.index.phase = GRN_PROGRESS_INDEX_LOAD;
        progress_.value.index.n_target_records = n_records_;
        grn_ctx_call_progress_callback(ctx_, &progress_);
      }
      if (n_records_ == 0) {
        /* Nothing to do because there are no values. */
        return ctx_->rc;
      }
      rc = set_srcs();
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      rc = append_srcs();
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      return GRN_SUCCESS;
    }

    // Reads the next data from a temporary file and fill a block
    // buffer.
    grn_rc
    fill_block(uint32_t block_id)
    {
      grn_ii_builder_block *block = &(blocks_[block_id]);
      if (!block->rest) {
        return GRN_END_OF_DATA;
      }
      if (!block->buf) {
        auto ctx = ctx_;
        block->buf =
          reinterpret_cast<uint8_t *>(GRN_MALLOC(options_.block_buf_size));
        if (!block->buf) {
          ERR(GRN_NO_MEMORY_AVAILABLE,
              "failed to allocate memory for buffered input: size = %u",
              options_.block_buf_size);
          return ctx->rc;
        }
      }

      /* Move the remaining data to the head. */
      uint32_t buf_rest = block->end - block->cur;
      if (buf_rest) {
        grn_memmove(block->buf, block->cur, buf_rest);
      }
      block->cur = block->buf;
      block->end = block->buf + buf_rest;

      /* Read the next data. */
      uint64_t file_offset = grn_lseek(fd_, block->offset, SEEK_SET);
      if (file_offset != block->offset) {
        auto ctx = ctx_;
        SERR("failed to seek file: expected = %" GRN_FMT_INT64U
             ", actual = %" GRN_FMT_INT64D,
             block->offset,
             file_offset);
        return ctx->rc;
      }
      buf_rest = options_.block_buf_size - buf_rest;
      if (block->rest < buf_rest) {
        buf_rest = block->rest;
      }
      ssize_t size = grn_read(fd_, block->end, buf_rest);
      if (size <= 0) {
        auto ctx = ctx_;
        SERR("failed to read data: expected = %u, actual = %" GRN_FMT_INT64D,
             buf_rest,
             static_cast<int64_t>(size));
        return ctx->rc;
      }
      block->offset += size;
      block->rest -= size;
      block->end += size;
      return GRN_SUCCESS;
    }

    // Reads the next value from a block.
    grn_rc
    read_from_block(uint32_t block_id, uint64_t *value)
    {
      grn_ii_builder_block *block = &(blocks_[block_id]);
      grn_rc rc = grn_ii_builder_block_next(ctx_, block, value);
      if (rc == GRN_SUCCESS) {
        return GRN_SUCCESS;
      } else if (rc == GRN_END_OF_DATA) {
        rc = fill_block(block_id);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        return grn_ii_builder_block_next(ctx_, block, value);
      }
      return rc;
    }

    // Tries to pack a chunk.
    grn_rc
    pack_chunk(bool *packed)
    {
      *packed = false;
      if (chunk_.offset != 1) { /* df != 1 */
        return GRN_SUCCESS;
      }
      if (chunk_.weight_buf && chunk_.weight_buf[0]) { /* weight != 0 */
        return GRN_SUCCESS;
      }
      if (chunk_.freq_buf[0] != 0) { /* freq != 1 */
        return GRN_SUCCESS;
      }
      grn_id rid = chunk_.rid_buf[0];
      uint32_t *a = nullptr;
      if (chunk_.sid_buf) {
        if (rid >= 0x100000) {
          return GRN_SUCCESS;
        }
        uint32_t sid = chunk_.sid_buf[0] + 1;
        if (sid >= 0x800) {
          return GRN_SUCCESS;
        }
        a = array_get(ctx_, ii_, chunk_.tid);
        if (!a) {
          auto ctx = ctx_;
          GRN_DEFINE_NAME(ii_);
          grn_obj token;
          GRN_TEXT_INIT(&token, 0);
          grn_ii_get_term(ctx, ii_, chunk_.tid, &token);
          MERR("[ii][builder][chunk][pack] failed to allocate an array: "
               "<%.*s>: "
               "<%.*s>(%u): "
               "(%u:%u)",
               name_size,
               name,
               static_cast<int>(GRN_TEXT_LEN(&token)),
               GRN_TEXT_VALUE(&token),
               chunk_.tid,
               rid,
               sid);
          GRN_OBJ_FIN(ctx, &token);
          return ctx->rc;
        }
        a[0] = POS_EMBED_RID_SID(rid, sid);
      } else {
        a = array_get(ctx_, ii_, chunk_.tid);
        if (!a) {
          auto ctx = ctx_;
          GRN_DEFINE_NAME(ii_);
          grn_obj token;
          GRN_TEXT_INIT(&token, 0);
          grn_ii_get_term(ctx, ii_, chunk_.tid, &token);
          MERR("[ii][builder][chunk][pack] failed to allocate an array: "
               "<%.*s>: "
               "<%.*s>(%u): "
               "(%u)",
               name_size,
               name,
               static_cast<int>(GRN_TEXT_LEN(&token)),
               GRN_TEXT_VALUE(&token),
               chunk_.tid,
               rid);
          GRN_OBJ_FIN(ctx, &token);
          return ctx->rc;
        }
        a[0] = POS_EMBED_RID(rid);
      }
      uint32_t pos = 0;
      if (chunk_.pos_buf) {
        pos = chunk_.pos_buf[0];
      }
      a[1] = pos;
      array_unref(ctx_, ii_, chunk_.tid);
      *packed = true;

      grn_ii_builder_chunk_clear(ctx_, &chunk_);
      return GRN_SUCCESS;
    }

    // Returns a new cinfo.
    grn_rc
    get_cinfo(chunk_info **cinfo)
    {
      if (n_cinfos_ == cinfos_size_) {
        uint32_t size = (cinfos_size_ > 0) ? (cinfos_size_ * 2) : 1;
        size_t n_bytes = size * sizeof(chunk_info);
        auto ctx = ctx_;
        auto cinfos =
          reinterpret_cast<chunk_info *>(GRN_REALLOC(cinfos_, n_bytes));
        if (!cinfos) {
          ERR(GRN_NO_MEMORY_AVAILABLE,
              "failed to allocate memory for cinfos: n_bytes = %" GRN_FMT_SIZE,
              n_bytes);
          return ctx->rc;
        }
        cinfos_ = cinfos;
        cinfos_size_ = size;
      }
      *cinfo = &(cinfos_[n_cinfos_++]);
      return GRN_SUCCESS;
    }

    // Flushes a chunk.
    grn_rc
    flush_chunk()
    {
      auto rc = grn_ii_builder_chunk_encode(ctx_, &chunk_, NULL, 0);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      uint8_t *in = chunk_.enc_buf;
      uint32_t in_size = chunk_.enc_offset;

      uint32_t chunk_id;
      rc = chunk_new(ctx_, ii_, &chunk_id, chunk_.enc_offset);
      if (rc != GRN_SUCCESS) {
        return rc;
      }

      /* Copy to the first segment. */
      uint32_t seg_id = chunk_id >> GRN_II_N_CHUNK_VARIATION;
      uint32_t seg_offset = (chunk_id & ((1 << GRN_II_N_CHUNK_VARIATION) - 1))
                            << GRN_II_W_LEAST_CHUNK;
      void *seg = grn_io_seg_ref(ctx_, ii_->chunk, seg_id);
      if (!seg) {
        if (ctx_->rc == GRN_SUCCESS) {
          auto ctx = ctx_;
          ERR(GRN_UNKNOWN_ERROR,
              "failed access chunk segment: chunk_id = %u, seg_id = %u",
              chunk_id,
              seg_id);
        }
        return ctx_->rc;
      }
      uint32_t seg_rest = S_CHUNK - seg_offset;
      if (in_size <= seg_rest) {
        grn_memcpy(static_cast<uint8_t *>(seg) + seg_offset, in, in_size);
        in_size = 0;
      } else {
        grn_memcpy(static_cast<uint8_t *>(seg) + seg_offset, in, seg_rest);
        in += seg_rest;
        in_size -= seg_rest;
      }
      grn_io_seg_unref(ctx_, ii_->chunk, seg_id);

      /* Copy to the next segments. */
      while (in_size) {
        seg_id++;
        seg = grn_io_seg_ref(ctx_, ii_->chunk, seg_id);
        if (!seg) {
          if (ctx_->rc == GRN_SUCCESS) {
            auto ctx = ctx_;
            ERR(GRN_UNKNOWN_ERROR,
                "failed access chunk segment: chunk_id = %u, seg_id = %u",
                chunk_id,
                seg_id);
          }
          return ctx_->rc;
        }
        if (in_size <= S_CHUNK) {
          grn_memcpy(seg, in, in_size);
          in_size = 0;
        } else {
          grn_memcpy(seg, in, S_CHUNK);
          in += S_CHUNK;
          in_size -= S_CHUNK;
        }
        grn_io_seg_unref(ctx_, ii_->chunk, seg_id);
      }

      /* Append a cinfo. */
      chunk_info *cinfo = nullptr;
      rc = get_cinfo(&cinfo);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      cinfo->segno = chunk_id;
      cinfo->size = chunk_.enc_offset;
      cinfo->dgap = chunk_.rid_gap;

      buf_.ii->header.common->total_chunk_size += chunk_.enc_offset;
      grn_ii_builder_chunk_clear(ctx_, &chunk_);
      return GRN_SUCCESS;
    }

    // Reads values from a block to a chunk.
    grn_rc
    read_to_chunk(uint32_t block_id)
    {
      const uint32_t ii_flags = ii_->header.common->flags;

      uint32_t rid = GRN_ID_NIL;
      uint32_t last_sid = 0;
      for (;;) {
        uint64_t value;
        auto rc = read_from_block(block_id, &value);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        if (!value) {
          break;
        }
        if (chunk_.offset == chunk_.size) {
          rc = grn_ii_builder_chunk_extend_bufs(ctx_, &chunk_, ii_flags);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
        }

        /* Read record ID. */
        uint32_t gap =
          static_cast<uint32_t>(value >> sid_bits_); /* In-block gap */
        uint32_t freq;
        if (gap) {
          if (chunk_.n >= options_.chunk_threshold) {
            const double threshold_scale = 1 + log(df_ + 1);
            const double threshold = options_.chunk_threshold * threshold_scale;
            if (chunk_.n >= threshold) {
              if (grn_logger_pass(ctx_, GRN_LOG_DEBUG)) {
                auto ctx = ctx_;
                GRN_DEFINE_NAME(ii_);
                grn_obj token;
                GRN_TEXT_INIT(&token, 0);
                grn_ii_get_term(ctx_, ii_, chunk_.tid, &token);
                GRN_LOG(ctx_,
                        GRN_LOG_DEBUG,
                        "[ii][builder][read-to-chunk] flush"
                        "<%.*s>: "
                        "<%.*s>(%u): "
                        "n=<%u> "
                        "df=<%u> "
                        "threshold=<%.1f>",
                        name_size,
                        name,
                        static_cast<int>(GRN_TEXT_LEN(&token)),
                        GRN_TEXT_VALUE(&token),
                        chunk_.tid,
                        chunk_.n,
                        df_,
                        threshold);
                GRN_OBJ_FIN(ctx, &token);
              }
              rc = flush_chunk();
              if (rc != GRN_SUCCESS) {
                return rc;
              }
            }
          }
          last_sid = 0;
        }
        rid += gap;
        gap = rid - chunk_.rid; /* Global gap */
        chunk_.rid_buf[chunk_.offset] = (chunk_.offset > 0) ? gap : rid;
        chunk_.n++;
        chunk_.rid = rid;
        chunk_.rid_gap += gap;
        df_++;

        /* Read section ID. */
        if (ii_flags & GRN_OBJ_WITH_SECTION) {
          auto sid = static_cast<uint32_t>((value & sid_mask_) + 1);
          chunk_.sid_buf[chunk_.offset] = sid - last_sid - 1;
          chunk_.n++;
          last_sid = sid;
        }

        /* Read weight. */
        if (ii_flags & GRN_OBJ_WITH_WEIGHT) {
          uint32_t weight;
          rc = read_from_block(block_id, &value);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          weight = static_cast<uint32_t>(value);
          chunk_.weight_buf[chunk_.offset] = weight;
          chunk_.n++;
        }

        /* Read positions or a frequency. */
        if (ii_flags & GRN_OBJ_WITH_POSITION) {
          uint32_t pos = -1;
          freq = 0;
          for (;;) {
            rc = read_from_block(block_id, &value);
            if (rc != GRN_SUCCESS) {
              return rc;
            }
            if (!value) {
              break;
            }
            if (chunk_.pos_offset == chunk_.pos_size) {
              rc = grn_ii_builder_chunk_extend_pos_buf(ctx_, &chunk_);
              if (rc != GRN_SUCCESS) {
                return rc;
              }
            }
            if (pos == static_cast<uint32_t>(-1)) {
              chunk_.pos_buf[chunk_.pos_offset] =
                static_cast<uint32_t>(value - 1);
              chunk_.pos_sum += value - 1;
            } else {
              chunk_.pos_buf[chunk_.pos_offset] = static_cast<uint32_t>(value);
              chunk_.pos_sum += value;
            }
            chunk_.n++;
            pos += static_cast<uint32_t>(value);
            chunk_.pos_offset++;
            freq++;
          }
        } else {
          rc = read_from_block(block_id, &value);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          freq = static_cast<uint32_t>(value);
        }
        chunk_.freq_buf[chunk_.offset] = freq - 1;
        chunk_.n++;
        chunk_.offset++;
      }

      uint64_t value;
      auto rc = read_from_block(block_id, &value);
      if (rc == GRN_SUCCESS) {
        blocks_[block_id].tid = static_cast<grn_id>(value);
      } else if (rc == GRN_END_OF_DATA) {
        blocks_[block_id].tid = GRN_ID_NIL;
      } else {
        return rc;
      }
      return GRN_SUCCESS;
    }

    // Registers chunks.
    grn_rc
    register_chunks()
    {
      auto rc = grn_ii_builder_chunk_encode(ctx_, &chunk_, cinfos_, n_cinfos_);
      if (rc != GRN_SUCCESS) {
        return rc;
      }

      if (!grn_ii_builder_buffer_is_assigned(ctx_, &buf_)) {
        rc = grn_ii_builder_buffer_assign(ctx_, &buf_, chunk_.enc_offset);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }
      uint32_t buf_tid = buf_.buf->header.nterms;
      if (buf_tid >= options_.buffer_max_n_terms ||
          buf_.chunk_size - buf_.chunk_offset < chunk_.enc_offset) {
        rc = grn_ii_builder_buffer_flush(ctx_, &buf_);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        rc = grn_ii_builder_buffer_assign(ctx_, &buf_, chunk_.enc_offset);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        buf_tid = 0;
      }
      buffer_term *buf_term = &(buf_.buf->terms[buf_tid]);
      buf_term->tid = chunk_.tid;
      if (n_cinfos_ > 0) {
        buf_term->tid |= CHUNK_SPLIT;
      }
      buf_term->size_in_buffer = 0;
      buf_term->pos_in_buffer = 0;
      buf_term->size_in_chunk = chunk_.enc_offset;
      buf_term->pos_in_chunk = buf_.chunk_offset;

      if (grn_logger_pass(ctx_, GRN_LOG_DEBUG)) {
        auto ctx = ctx_;
        GRN_DEFINE_NAME(ii_);
        grn_obj token;
        GRN_TEXT_INIT(&token, 0);
        grn_ii_get_term(ctx, ii_, chunk_.tid, &token);
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "[ii][builder][register][chunks] "
                "<%.*s>: "
                "<%.*s>(%u): "
                "n_chunks=<%u> "
                "chunk=<%u>/<%u>(%u)/(%u) "
                "encoded_chunk_size=<%" GRN_FMT_SIZE ">",
                name_size,
                name,
                static_cast<int>(GRN_TEXT_LEN(&token)),
                GRN_TEXT_VALUE(&token),
                chunk_.tid,
                n_cinfos_,
                buf_.chunk_offset,
                buf_.chunk_size,
                buf_.chunk_size - buf_.chunk_offset,
                buf_.ii->chunk->header->segment_size,
                chunk_.enc_offset);
        GRN_OBJ_FIN(ctx, &token);
      }
      grn_memcpy(buf_.chunk + buf_.chunk_offset,
                 chunk_.enc_buf,
                 chunk_.enc_offset);
      buf_.chunk_offset += chunk_.enc_offset;

      uint32_t *a = array_get(ctx_, ii_, chunk_.tid);
      if (!a) {
        auto ctx = ctx_;
        GRN_DEFINE_NAME(ii_);
        grn_obj token;
        GRN_TEXT_INIT(&token, 0);
        grn_ii_get_term(ctx_, ii_, chunk_.tid, &token);
        MERR("[ii][builder][register][chunks] "
             "failed to allocate an array in segment: "
             "<%.*s>: "
             "<%.*s>(%u): "
             "max_n_segments=<%u>",
             name_size,
             name,
             static_cast<int>(GRN_TEXT_LEN(&token)),
             GRN_TEXT_VALUE(&token),
             chunk_.tid,
             ii_->seg->header->max_segment);
        GRN_OBJ_FIN(ctx, &token);
        return ctx->rc;
      }
      a[0] = grn_ii_pos_pack(ii_,
                             buf_.buf_id,
                             POS_LOFFSET_HEADER + POS_LOFFSET_TERM * buf_tid);
      a[1] = df_;
      array_unref(ctx_, ii_, chunk_.tid);

      buf_.buf->header.nterms++;
      n_cinfos_ = 0;
      grn_ii_builder_chunk_clear(ctx_, &chunk_);
      return GRN_SUCCESS;
    }

    grn_rc
    commit()
    {
      if (progress_needed_) {
        progress_.value.index.phase = GRN_PROGRESS_INDEX_COMMIT;
        progress_.value.index.n_target_terms =
          grn_table_size(ctx_, ii_->lexicon);
        grn_ctx_call_progress_callback(ctx_, &progress_);
      }

      for (uint32_t i = 0; i < n_blocks_; i++) {
        uint64_t value;
        auto rc = read_from_block(i, &value);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        blocks_[i].tid = static_cast<grn_id>(value);
      }

      // See also the comment in BlockBuilder::flush().
      auto rc = table_each_by_key(ctx_, ii_->lexicon, [&](grn_id tid) {
        chunk_.tid = tid;
        chunk_.rid = GRN_ID_NIL;
        df_ = 0;
        for (uint32_t i = 0; i < n_blocks_; i++) {
          if (tid == blocks_[i].tid) {
            auto rc = read_to_chunk(i);
            if (rc != GRN_SUCCESS) {
              return rc;
            }
          }
        }
        if (chunk_.n == 0) {
          /* This term does not appear. */
          return GRN_SUCCESS;
        }
        if (n_cinfos_ == 0) {
          bool packed;
          auto rc = pack_chunk(&packed);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          if (packed) {
            return GRN_SUCCESS;
          }
        }
        auto rc = register_chunks();
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        if (progress_needed_) {
          progress_.value.index.n_processed_terms++;
          grn_ctx_call_progress_callback(ctx_, &progress_);
        }
        return GRN_SUCCESS;
      });
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      if (grn_ii_builder_buffer_is_assigned(ctx_, &buf_)) {
        rc = grn_ii_builder_buffer_flush(ctx_, &buf_);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
      }
      return GRN_SUCCESS;
    }

    grn_ctx *ctx_;
    bool progress_needed_;  /* Whether progress callback is needed for
                               performance */
    grn_progress progress_; /* Progress information */

    grn_ii *ii_;                     /* Building inverted index */
    grn_ii_builder_options options_; /* Options */

    grn_obj *src_table_;          /* Source table */
    unsigned int n_records_;      /* Number of records in source table */
    grn_obj **srcs_;              /* Source columns (to be freed) */
    grn_obj **src_token_columns_; /* Source token columns (to be freed) */
    uint32_t n_srcs_;             /* Number of source columns */
    uint8_t sid_bits_;            /* Number of bits for section ID */
    uint64_t sid_mask_;           /* Mask bits for section ID */

    /* A temporary file to save blocks. */
    char path_[PATH_MAX]; /* File path */
    int fd_;              /* File descriptor (to be closed) */
    uint8_t *file_buf_;   /* File buffer for buffered output (to be freed) */
    uint32_t file_buf_offset_; /* File buffer write offset */

    grn_ii_builder_block *blocks_; /* Blocks (to be freed) */
    uint32_t n_blocks_;            /* Number of blocks */
    uint32_t blocks_size_;         /* Buffer size of blocks */

    grn_ii_builder_buffer buf_;  /* Buffer (to be finalized) */
    grn_ii_builder_chunk chunk_; /* Chunk (to be finalized) */

    uint32_t df_;          /* Document frequency (number of sections) */
    chunk_info *cinfos_;   /* Chunk headers (to be freed) */
    uint32_t n_cinfos_;    /* Number of chunks */
    uint32_t cinfos_size_; /* Size of cinfos */
  };
} // namespace grn::ii

extern "C" grn_rc
grn_ii_build(grn_ctx *ctx, grn_ii *ii, const grn_ii_builder_options *options)
{
  grn_rc rc = GRN_SUCCESS;
  {
    grn::ii::Builder builder(ctx, ii, options);
    rc = builder.append_source();
    if (rc == GRN_SUCCESS) {
      rc = builder.commit();
    }
  }
  if (rc != GRN_SUCCESS) {
    grn_obj_flush(ctx, reinterpret_cast<grn_obj *>(ii));
  }
  return rc;
}

extern "C" grn_rc
grn_ii_wal_recover(grn_ctx *ctx, grn_ii *ii)
{
  if (GRN_CTX_GET_WAL_ROLE(ctx) != GRN_WAL_ROLE_PRIMARY) {
    return GRN_SUCCESS;
  }

  if (!grn_wal_exist(ctx, (grn_obj *)ii)) {
    if (grn_io_is_locked(ii->seg)) {
      grn_io_clear_lock(ii->seg);
      grn_obj_touch(ctx, (grn_obj *)ii, NULL);
      grn_obj_flush(ctx, (grn_obj *)ii);
    }
    return GRN_SUCCESS;
  }

  grn_obj_set_error(ctx,
                    (grn_obj *)ii,
                    GRN_FUNCTION_NOT_IMPLEMENTED,
                    GRN_ID_NIL,
                    "[ii][wal][recover]",
                    "not implemented");
  return ctx->rc;
}

extern "C" grn_rc
grn_ii_warm(grn_ctx *ctx, grn_ii *ii)
{
  grn_rc rc = grn_io_warm(ctx, ii->seg);
  if (rc == GRN_SUCCESS) {
    rc = grn_io_warm(ctx, ii->chunk);
  }
  return rc;
}
