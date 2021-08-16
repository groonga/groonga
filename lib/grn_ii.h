/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2019-2021  Sutou Kouhei <kou@clear-code.com>

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

/* "ii" is for inverted index */

#include "grn.h"
#include "grn_hash.h"
#include "grn_io.h"
#include "grn_store.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _grn_ii_header_common grn_ii_header_common;
typedef struct _grn_ii_header_normal grn_ii_header_normal;
typedef struct _grn_ii_header_large  grn_ii_header_large;

struct _grn_ii {
  grn_db_obj obj;
  grn_io *seg;           /* I/O for a variety of segments */
  grn_io *chunk;         /* I/O for posting chunks */
  grn_obj *lexicon;      /* Lexicon table */
  grn_table_flags lflags;
  grn_encoding encoding; /* Character encoding */
                         /* This member is used for matching */
  uint32_t n_elements;   /* Number of elements in postings */
                         /* rid, [sid], tf, [weight] and [pos] */
  union {
    grn_ii_header_common *common;
    grn_ii_header_normal *normal;
    grn_ii_header_large *large;
  } header;

  bool wal_touched;
};

/* BGQ is buffer garbage queue? */
#define GRN_II_BGQSIZE 16
/* L is for logical? */
#define GRN_II_MAX_LSEG           0x10000
#define GRN_II_W_LSEG             16
#define GRN_II_W_LOFFSET          (32 - GRN_II_W_LSEG)
#define GRN_II_MAX_LSEG_EXTEND    0x10000
#define GRN_II_W_LSEG_LARGE       17
#define GRN_II_W_LOFFSET_LARGE    (32 - GRN_II_W_LSEG_LARGE)
#define GRN_II_W_TOTAL_CHUNK      40
/* Each chunk has 2^GRN_II_W_CHUNK size: 4MiB */
#define GRN_II_W_CHUNK            22
/* The smallest subchunk size: 256B */
#define GRN_II_W_LEAST_CHUNK      (GRN_II_W_TOTAL_CHUNK - 32)
#define GRN_II_MAX_CHUNK          (1 << (GRN_II_W_TOTAL_CHUNK - GRN_II_W_CHUNK))
/* The number of subchunk size patterns: 256B, 512B, ..., 2MiB */
#define GRN_II_N_CHUNK_VARIATION  (GRN_II_W_CHUNK - GRN_II_W_LEAST_CHUNK)

#define GRN_II_MAX_CHUNK_SMALL    (1 << (GRN_II_W_TOTAL_CHUNK - GRN_II_W_CHUNK - 8))
/* GRN_II_MAX_CHUNK_MEDIUM has enough space for the following source:
 *   * Single source.
 *   * Source is a fixed size column or _key of a table.
 *   * Source column is a scalar column.
 *   * Lexicon doesn't have tokenizer.
 */
#define GRN_II_MAX_CHUNK_MEDIUM   (1 << (GRN_II_W_TOTAL_CHUNK - GRN_II_W_CHUNK - 4))

#define GRN_II_PSEG_NOT_ASSIGNED  0xffffffff

#define GRN_II_HEADER_COMMON_FIELDS                     \
  uint64_t total_chunk_size;                            \
  uint64_t bmax;                                        \
  uint32_t flags;                                       \
  uint32_t amax;                                        \
  uint32_t smax;                                        \
  uint32_t param1;                                      \
  uint32_t param2;                                      \
  uint32_t pnext;                                       \
  uint32_t bgqhead;                                     \
  uint32_t bgqtail;                                     \
  uint32_t bgqbody[GRN_II_BGQSIZE];                     \
  uint32_t reserved[288];                               \
  uint32_t ainfo[GRN_II_MAX_LSEG]; /* array info */     \
  uint32_t binfo[GRN_II_MAX_LSEG]; /* buffer info */    \
  uint32_t free_chunks[GRN_II_N_CHUNK_VARIATION + 1];   \
  uint32_t garbages[GRN_II_N_CHUNK_VARIATION + 1];      \
  uint32_t ngarbages[GRN_II_N_CHUNK_VARIATION + 1];     \
  uint8_t chunks[GRN_II_MAX_CHUNK >> 3]

struct _grn_ii_header_common {
  GRN_II_HEADER_COMMON_FIELDS;
};

struct _grn_ii_header_normal {
  GRN_II_HEADER_COMMON_FIELDS;
};

struct _grn_ii_header_large {
  GRN_II_HEADER_COMMON_FIELDS;
  uint32_t ainfo_extend[GRN_II_MAX_LSEG_EXTEND]; /* array info (extended) */
  uint32_t binfo_extend[GRN_II_MAX_LSEG_EXTEND]; /* buffer info (extended) */
};

struct _grn_ii_pos {
  struct _grn_ii_pos *next;
  uint32_t pos;
};

struct _grn_ii_updspec {
  uint32_t rid;
  uint32_t sid;
  int32_t weight;
  int32_t tf;                 /* number of postings successfully stored to index */
  int32_t atf;                /* actual number of postings */
  int32_t offset;
  struct _grn_ii_pos *pos;
  struct _grn_ii_pos *tail;
  /* grn_vgram_vnode *vnodes; */
};

typedef struct _grn_ii_updspec grn_ii_updspec;

void grn_ii_init_from_env(void);

GRN_API grn_ii *grn_ii_create(grn_ctx *ctx, const char *path, grn_obj *lexicon,
                              uint32_t flags);
GRN_API grn_ii *grn_ii_open(grn_ctx *ctx, const char *path, grn_obj *lexicon);
GRN_API grn_rc grn_ii_close(grn_ctx *ctx, grn_ii *ii);
GRN_API grn_rc grn_ii_remove(grn_ctx *ctx, const char *path);
grn_rc grn_ii_info(grn_ctx *ctx, grn_ii *ii, uint64_t *seg_size, uint64_t *chunk_size);
grn_rc grn_ii_update_one(grn_ctx *ctx, grn_ii *ii, uint32_t key, grn_ii_updspec *u,
                         grn_hash *h);
grn_rc grn_ii_delete_one(grn_ctx *ctx, grn_ii *ii, uint32_t key, grn_ii_updspec *u,
                         grn_hash *h);
grn_ii_updspec *grn_ii_updspec_open(grn_ctx *ctx, uint32_t rid, uint32_t sid);
grn_rc grn_ii_updspec_close(grn_ctx *ctx, grn_ii_updspec *u);
grn_rc grn_ii_updspec_add(grn_ctx *ctx, grn_ii_updspec *u, int pos, int32_t weight);
int grn_ii_updspec_cmp(grn_ii_updspec *a, grn_ii_updspec *b);

void grn_ii_expire(grn_ctx *ctx, grn_ii *ii);
grn_rc grn_ii_flush(grn_ctx *ctx, grn_ii *ii);
size_t grn_ii_get_disk_usage(grn_ctx *ctx, grn_ii *ii);

void grn_ii_set_visibility(grn_ctx *ctx, grn_ii *ii, bool is_visible);

grn_ii_cursor *grn_ii_cursor_openv1(grn_ii *ii, uint32_t key);
grn_rc grn_ii_cursor_openv2(grn_ii_cursor **cursors, int ncursors);

uint32_t grn_ii_get_array_pseg(grn_ii *ii, uint32_t lseg);
uint32_t grn_ii_get_buffer_pseg(grn_ii *ii, uint32_t pseg);
uint32_t grn_ii_n_logical_segments(grn_ii *ii);

uint32_t grn_ii_max_section(grn_ii *ii);

const char *grn_ii_path(grn_ii *ii);
grn_obj *grn_ii_lexicon(grn_ii *ii);

/*
grn_rc grn_ii_upd(grn_ctx *ctx, grn_ii *ii, grn_id rid, grn_vgram *vgram,
                   const char *oldvalue, unsigned int oldvalue_len,
                   const char *newvalue, unsigned int newvalue_len);
grn_rc grn_ii_update(grn_ctx *ctx, grn_ii *ii, grn_id rid, grn_vgram *vgram,
                      unsigned int section,
                      grn_values *oldvalues, grn_values *newvalues);
*/

typedef struct _grn_select_optarg grn_select_optarg;

struct _grn_select_optarg {
  grn_operator mode;
  int similarity_threshold;
  int max_interval;
  int *weight_vector;
  float *weight_vector_float;
  float weight_float;
  int vector_size;
  int (*func)(grn_ctx *, grn_hash *, const void *, int, void *);
  void *func_arg;
  int max_size;
  grn_obj *scorer;
  grn_obj *scorer_args_expr;
  unsigned int scorer_args_expr_offset;
  grn_fuzzy_search_optarg fuzzy;
  grn_match_info *match_info;
  int quorum_threshold;
  int additional_last_interval;
  grn_obj *query_options;
};

GRN_API grn_rc grn_ii_column_update(grn_ctx *ctx, grn_ii *ii, grn_id id,
                                    unsigned int section, grn_obj *oldvalue,
                                    grn_obj *newvalue, grn_obj *posting);
grn_rc grn_ii_term_extract(grn_ctx *ctx, grn_ii *ii, const char *string,
                            unsigned int string_len, grn_hash *s,
                            grn_operator op, grn_select_optarg *optarg);
grn_rc grn_ii_similar_search(grn_ctx *ctx, grn_ii *ii, const char *string, unsigned int string_len,
                              grn_hash *s, grn_operator op, grn_select_optarg *optarg);
GRN_API grn_rc grn_ii_select(grn_ctx *ctx, grn_ii *ii, const char *string, unsigned int string_len,
                             grn_hash *s, grn_operator op, grn_select_optarg *optarg);
grn_rc grn_ii_sel(grn_ctx *ctx, grn_ii *ii, const char *string, unsigned int string_len,
                  grn_hash *s, grn_operator op, grn_search_optarg *optarg);

void grn_ii_resolve_sel_and(grn_ctx *ctx, grn_hash *s, grn_operator op);

grn_rc grn_ii_at(grn_ctx *ctx, grn_ii *ii, grn_id id, grn_hash *s, grn_operator op);

void grn_ii_inspect_values(grn_ctx *ctx, grn_ii *ii, grn_obj *buf);
void grn_ii_cursor_inspect(grn_ctx *ctx, grn_ii_cursor *c, grn_obj *buf);

grn_rc grn_ii_truncate(grn_ctx *ctx, grn_ii *ii);
grn_rc grn_ii_build(grn_ctx *ctx, grn_ii *ii, uint64_t sparsity);

typedef struct grn_ii_builder_options grn_ii_builder_options;

grn_rc grn_ii_build2(grn_ctx *ctx, grn_ii *ii,
                     const grn_ii_builder_options *options);

grn_rc grn_ii_wal_recover(grn_ctx *ctx, grn_ii *ii);
grn_rc grn_ii_warm(grn_ctx *ctx, grn_ii *ii);

#ifdef __cplusplus
}
#endif
