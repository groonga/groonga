/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef GRN_II_H
#define GRN_II_H

/* "ii" is for inverted index */

#ifndef GROONGA_H
#include "groonga_in.h"
#endif /* GROONGA_H */

#ifndef GRN_HASH_H
#include "hash.h"
#endif /* GRN_HASH_H */

#ifndef GRN_IO_H
#include "io.h"
#endif /* GRN_IO_H */

#ifndef GRN_STORE_H
#include "store.h"
#endif /* GRN_STORE_H */

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _grn_ii grn_ii;

struct _grn_ii {
  grn_db_obj obj;
  grn_io *seg;
  grn_io *chunk;
  grn_obj *lexicon;
  grn_obj_flags lflags;
  grn_encoding encoding;
  uint32_t n_elements;
  struct grn_ii_header *header;
};

struct grn_ii_header;

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
  struct _grn_ii_pos *pos;
  struct _grn_ii_pos *tail;
  /* grn_vgram_vnode *vnodes; */
};

typedef struct _grn_ii_updspec grn_ii_updspec;

grn_ii *grn_ii_create(grn_ctx *ctx, const char *path, grn_obj *lexicon, uint32_t flags);
grn_ii *grn_ii_open(grn_ctx *ctx, const char *path, grn_obj *lexicon);
grn_rc grn_ii_close(grn_ctx *ctx, grn_ii *ii);
grn_rc grn_ii_remove(grn_ctx *ctx, const char *path);
grn_rc grn_ii_info(grn_ctx *ctx, grn_ii *ii, uint64_t *seg_size, uint64_t *chunk_size);
grn_rc grn_ii_update_one(grn_ctx *ctx, grn_ii *ii, uint32_t key, grn_ii_updspec *u,
                         grn_hash *h);
grn_rc grn_ii_delete_one(grn_ctx *ctx, grn_ii *ii, uint32_t key, grn_ii_updspec *u,
                         grn_hash *h);
grn_ii_updspec *grn_ii_updspec_open(grn_ctx *ctx, uint32_t rid, uint32_t sid);
grn_rc grn_ii_updspec_close(grn_ctx *ctx, grn_ii_updspec *u);
grn_rc grn_ii_updspec_add(grn_ctx *ctx, grn_ii_updspec *u, int pos, int32_t weight);
int grn_ii_updspec_cmp(grn_ii_updspec *a, grn_ii_updspec *b);

uint32_t grn_ii_estimate_size(grn_ctx *ctx, grn_ii *ii, uint32_t key);

void grn_ii_expire(grn_ctx *ctx, grn_ii *ii);

typedef struct {
  grn_id rid;
  uint32_t sid;
  uint32_t pos;
  uint32_t tf;
  uint32_t weight;
  uint32_t rest;
} grn_ii_posting;

typedef struct _grn_ii_cursor grn_ii_cursor;

grn_ii_cursor *grn_ii_cursor_open(grn_ctx *ctx, grn_ii *ii, grn_id tid,
                                  grn_id min, grn_id max, int nelements, int flags);
grn_ii_cursor *grn_ii_cursor_openv1(grn_ii *ii, uint32_t key);
grn_rc grn_ii_cursor_openv2(grn_ii_cursor **cursors, int ncursors);
grn_ii_posting *grn_ii_cursor_next(grn_ctx *ctx, grn_ii_cursor *c);
grn_ii_posting *grn_ii_cursor_next_pos(grn_ctx *ctx, grn_ii_cursor *c);
grn_rc grn_ii_cursor_close(grn_ctx *ctx, grn_ii_cursor *c);

uint32_t grn_ii_max_section(grn_ii *ii);

int grn_ii_check(grn_ii *ii);
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

typedef enum {
  GRN_SEL_EXACT = 0,
  GRN_SEL_PARTIAL,
  GRN_SEL_UNSPLIT,
  GRN_SEL_NEAR,
  GRN_SEL_NEAR2,
  GRN_SEL_SIMILAR,
  GRN_SEL_TERM_EXTRACT,
  GRN_SEL_PREFIX,
  GRN_SEL_SUFFIX
} grn_sel_mode;

struct _grn_select_optarg {
  grn_sel_mode mode;
  int similarity_threshold;
  int max_interval;
  int *weight_vector;
  int vector_size;
  int (*func)(grn_ctx *, grn_hash *, const void *, int, void *);
  void *func_arg;
  int max_size;
};

grn_rc grn_ii_column_update(grn_ctx *ctx, grn_ii *ii, grn_id id, unsigned int section,
                            grn_obj *oldvalue, grn_obj *newvalue);
grn_rc grn_ii_term_extract(grn_ctx *ctx, grn_ii *ii, const char *string,
                            unsigned int string_len, grn_hash *s,
                            grn_sel_operator op, grn_select_optarg *optarg);
grn_rc grn_ii_similar_search(grn_ctx *ctx, grn_ii *ii, const char *string, unsigned int string_len,
                              grn_hash *s, grn_sel_operator op, grn_select_optarg *optarg);
grn_rc grn_ii_select(grn_ctx *ctx, grn_ii *ii, const char *string, unsigned int string_len,
                     grn_hash *s, grn_sel_operator op, grn_select_optarg *optarg);
grn_rc grn_ii_sel(grn_ctx *ctx, grn_ii *ii, const char *string, unsigned int string_len,
                  grn_hash *s, grn_sel_operator op);

grn_rc grn_ii_query_select(grn_ctx *ctx, grn_ii *i, grn_query *q, grn_hash *r, grn_sel_operator op);

grn_rc grn_query_search(grn_ctx *ctx, grn_ii *i, grn_query *q,
                        grn_hash *r, grn_sel_operator op);

#ifdef __cplusplus
}
#endif

#endif /* GRN_II_H */
