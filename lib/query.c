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

#include "groonga_in.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "snip.h"
#include "ql.h"
#include "hash.h"
#include "ii.h"

/* query string parser and executor */

#define DEFAULT_WEIGHT 5
#define DEFAULT_DECAYSTEP 2
#define DEFAULT_MAX_INTERVAL 10
#define DEFAULT_SIMILARITY_THRESHOLD 10
#define DEFAULT_TERM_EXTRACT_POLICY 0
#define DEFAULT_WEIGHT_VECTOR_SIZE 4096

struct _grn_query {
  grn_obj_header header;
  char *str;
  char *cur;
  char *str_end;
  grn_operator default_op;
  grn_select_optarg opt;
  grn_operator default_mode;
  int escalation_threshold;
  int escalation_decaystep;
  int weight_offset;
  grn_hash *weight_set;
  grn_encoding encoding;
  grn_cell *expr;
  int max_exprs;
  int cur_expr;
  int max_cells;
  int cur_cell;
  snip_cond *snip_conds;
  grn_cell cell_pool[1]; /* dummy */
};

inline static grn_cell *
cell_new(grn_query *q)
{
  if (q->cur_cell <= q->max_cells) {
    grn_cell *c = &q->cell_pool[q->cur_cell++];
    return c;
  }
  return NULL;
}

inline static void
cell_del(grn_query *q)
{
  if (q->cur_cell > 0) { q->cur_cell--; }
}

inline static grn_cell *
cons(grn_query *q, grn_cell *car, grn_cell *cdr)
{
  grn_cell *c;
  if ((c = cell_new(q))) {
    c->header.type = GRN_CELL_LIST;
    c->u.l.car = car;
    c->u.l.cdr = cdr;
    return c;
  } else {
    return NIL;
  }
}

inline static grn_cell *
token_new(grn_query *q, const char *start, const char *end)
{
  grn_cell *c;
  if (start >= end) { return NIL; }
  if ((c = cell_new(q))) {
    unsigned int len = end - start;
    c->header.type = GRN_CELL_STR;
    c->u.b.value = (char *)start;
    c->u.b.size = len;
    q->cur_expr++;
    return c;
  } else {
    return NIL;
  }
}

inline static grn_cell *
op_new(grn_query *q, int8_t op, int16_t weight, int8_t mode, int32_t option)
{
  grn_cell *c;
  if ((c = cell_new(q))) {
    c->header.type = GRN_CELL_OP;
    c->u.op.op = op;
    c->u.op.weight = weight;
    c->u.op.mode = mode;
    c->u.op.option = option;
    return c;
  } else {
    return NIL;
  }
}

inline static void
skip_space(grn_ctx *ctx, grn_query *q)
{
  unsigned int len;
  while (q->cur < q->str_end && grn_isspace(q->cur, q->encoding)) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, q->cur, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    q->cur += len;
  }
}

inline static grn_cell *
get_phrase(grn_ctx *ctx, grn_query *q)
{
  char *start, *s, *d;
  start = s = d = q->cur;
  while (1) {
    unsigned int len;
    if (s >= q->str_end) {
      q->cur = s;
      break;
    }
    len = grn_charlen(ctx, s, q->str_end);
    if (len == 0) {
      /* invalid string containing malformed multibyte char */
      return NULL;
    } else if (len == 1) {
      if (*s == GRN_QUERY_QUOTER) {
        q->cur = s + 1;
        break;
      } else if (*s == GRN_QUERY_ESCAPE && s + 1 < q->str_end) {
        s++;
        len = grn_charlen(ctx, s, q->str_end);
      }
    }
    while (len--) { *d++ = *s++; }
  }
  return token_new(q, start, d);
}

inline static grn_cell *
get_word(grn_ctx *ctx, grn_query *q, int *prefixp)
{
  char *start = q->cur, *end;
  unsigned int len;
  for (end = q->cur;; ) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, end, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    if (grn_isspace(end, q->encoding) ||
        *end == GRN_QUERY_PARENR) {
      q->cur = end;
      break;
    }
    if (*end == GRN_QUERY_PREFIX) {
      *prefixp = 1;
      q->cur = end + 1;
      break;
    }
    end += len;
  }
  return token_new(q, start, end);
}

inline static grn_cell *
get_op(grn_query *q, grn_operator op, int weight)
{
  char *start, *end = q->cur;
  int mode, option;
  switch (*end) {
  case 'S' :
    mode = GRN_OP_SIMILAR;
    start = ++end;
    option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { option = DEFAULT_SIMILARITY_THRESHOLD; }
    q->cur = end;
    break;
  case 'N' :
    mode = GRN_OP_NEAR;
    start = ++end;
    option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { option = DEFAULT_MAX_INTERVAL; }
    q->cur = end;
    break;
  case 'n' :
    mode = GRN_OP_NEAR2;
    start = ++end;
    option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { option = DEFAULT_MAX_INTERVAL; }
    q->cur = end;
    break;
  case 'T' :
    mode = GRN_OP_TERM_EXTRACT;
    start = ++end;
    option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { option = DEFAULT_TERM_EXTRACT_POLICY; }
    q->cur = end;
    break;
  case 'X' : /* force exact mode */
    op = GRN_OP_AND;
    mode = GRN_OP_EXACT;
    option = 0;
    start = ++end;
    q->cur = end;
    break;
  default :
    return NIL;
  }
  return op_new(q, op, weight, mode, option);
}

static grn_cell *get_expr(grn_ctx *ctx, grn_query *q);

inline static grn_cell *
get_token(grn_ctx *ctx, grn_query *q)
{
  grn_cell *token = NIL;
  grn_operator op = q->default_op;
  {
    int weight = DEFAULT_WEIGHT, prefixp = 0, mode = -1, option = 0;
    skip_space(ctx, q);
    if (q->cur_expr >= q->max_exprs ||
        q->cur_cell >= q->max_cells ||
        q->cur >= q->str_end) { return NIL; }
    switch (*q->cur) {
    case '\0' :
      return NIL;
    case GRN_QUERY_PARENR :
      q->cur++;
      return NIL;
    case GRN_QUERY_QUOTEL :
      q->cur++;
      if ((token = get_phrase(ctx, q)) == NULL) {
	return NIL;
      }
      break;
    case GRN_QUERY_PREFIX :
      q->cur++;
      token = get_op(q, op, weight);
      break;
    case GRN_QUERY_AND :
      q->cur++;
      token = op_new(q, GRN_OP_AND, weight, mode, option);
      break;
    case GRN_QUERY_BUT :
      q->cur++;
      token = op_new(q, GRN_OP_BUT, weight, mode, option);
      break;
    case GRN_QUERY_ADJ_INC :
      q->cur++;
      if (weight < 127) { weight++; }
      token = op_new(q, GRN_OP_ADJUST, weight, mode, option);
      break;
    case GRN_QUERY_ADJ_DEC :
      q->cur++;
      if (weight > -128) { weight--; }
      token = op_new(q, GRN_OP_ADJUST, weight, mode, option);
      break;
    case GRN_QUERY_ADJ_NEG :
      q->cur++;
      token = op_new(q, GRN_OP_ADJUST, -1, mode, option);
      break;
    case GRN_QUERY_PARENL :
      q->cur++;
      token = get_expr(ctx, q);
      break;
    default :
      if ((token = get_word(ctx, q, &prefixp)) &&
          token->u.b.value[0] == 'O' &&
          token->u.b.value[1] == 'R' &&
          token->u.b.size == 2) {
        cell_del(q);
        q->cur_expr--;
        token = op_new(q, GRN_OP_OR, weight, mode, option);
      }
      break;
    }
  }
  return cons(q, token, NIL);
}

static grn_cell *
get_expr(grn_ctx *ctx, grn_query *q)
{
  grn_cell *r, *c, *c_;
  for (c = r = get_token(ctx, q); c != NIL; c = c_) {
    c_ = c->u.l.cdr = get_token(ctx, q);
  }
  return r;
}

static const char *
get_weight_vector(grn_ctx *ctx, grn_query *query, const char *source)
{
  const char *p;

  if (!query->opt.weight_vector &&
      !query->weight_set &&
      !(query->opt.weight_vector = GRN_CALLOC(sizeof(int) * DEFAULT_WEIGHT_VECTOR_SIZE))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "get_weight_vector malloc fail");
    return source;
  }
  for (p = source; p < query->str_end; ) {
    unsigned int key;
    int value;

    /* key, key is not zero */
    key = grn_atoui(p, query->str_end, &p);
    if (!key || key > GRN_ID_MAX) { break; }

    /* value */
    if (*p == ':') {
      p++;
      value = grn_atoi(p, query->str_end, &p);
    } else {
      value = 1;
    }

    if (query->weight_set) {
      int *pval;
      if (grn_hash_add(ctx, query->weight_set, &key, sizeof(unsigned int), (void **)&pval, NULL)) {
        *pval = value;
      }
    } else if (key < DEFAULT_WEIGHT_VECTOR_SIZE) {
      query->opt.weight_vector[key - 1] = value;
    } else {
      GRN_FREE(query->opt.weight_vector);
      query->opt.weight_vector = NULL;
      if (!(query->weight_set = grn_hash_create(ctx, NULL, sizeof(unsigned int), sizeof(int),
                                                0))) {
        return source;
      }
      p = source;           /* reparse */
      continue;
    }
    if (*p != ',') { break; }
    p++;
  }
  return p;
}

inline static void
get_pragma(grn_ctx *ctx, grn_query *q)
{
  char *start, *end = q->cur;
  while (end < q->str_end && *end == GRN_QUERY_PREFIX) {
    if (++end >= q->str_end) { break; }
    switch (*end) {
    case 'E' :
      start = ++end;
      q->escalation_threshold = grn_atoi(start, q->str_end, (const char **)&end);
      while (end < q->str_end && (isdigit(*end) || *end == '-')) { end++; }
      if (*end == ',') {
        start = ++end;
        q->escalation_decaystep = grn_atoi(start, q->str_end, (const char **)&end);
      }
      q->cur = end;
      break;
    case 'D' :
      start = ++end;
      while (end < q->str_end && *end != GRN_QUERY_PREFIX && !grn_isspace(end, q->encoding)) {
        end++;
      }
      if (end > start) {
        switch (*start) {
        case 'O' :
          q->default_op = GRN_OP_OR;
          break;
        case GRN_QUERY_AND :
          q->default_op = GRN_OP_AND;
          break;
        case GRN_QUERY_BUT :
          q->default_op = GRN_OP_BUT;
          break;
        case GRN_QUERY_ADJ_INC :
          q->default_op = GRN_OP_ADJUST;
          break;
        }
      }
      q->cur = end;
      break;
    case 'W' :
      start = ++end;
      end = (char *)get_weight_vector(ctx, q, start);
      q->cur = end;
      break;
    }
  }
}

static int
section_weight_cb(grn_ctx *ctx, grn_hash *r, const void *rid, int sid, void *arg)
{
  int *w;
  grn_hash *s = (grn_hash *)arg;
  if (s && grn_hash_get(ctx, s, &sid, sizeof(grn_id), (void **)&w)) {
    return *w;
  } else {
    return 0;
  }
}

grn_query *
grn_query_open(grn_ctx *ctx, const char *str, unsigned int str_len,
               grn_operator default_op, int max_exprs)
{
  grn_query *q;
  int max_cells = max_exprs * 4;
  if (!(q = GRN_MALLOC(sizeof(grn_query) + max_cells * sizeof(grn_cell) + str_len + 1))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_query_open malloc fail");
    return NULL;
  }
  q->header.type = GRN_QUERY;
  q->str = (char *)&q->cell_pool[max_cells];
  memcpy(q->str, str, str_len);
  q->str[str_len] = '\0';
  q->cur = q->str;
  q->str_end = q->str + str_len;
  q->default_op = default_op;
  q->encoding = ctx->encoding;
  q->max_exprs = max_exprs;
  q->max_cells = max_cells;
  q->cur_cell = 0;
  q->cur_expr = 0;
  q->escalation_threshold = GROONGA_DEFAULT_QUERY_ESCALATION_THRESHOLD;
  q->escalation_decaystep = DEFAULT_DECAYSTEP;
  q->weight_offset = 0;
  q->opt.weight_vector = NULL;
  q->weight_set = NULL;
  get_pragma(ctx, q);
  q->expr = get_expr(ctx, q);
  q->opt.vector_size = DEFAULT_WEIGHT_VECTOR_SIZE;
  q->opt.func = q->weight_set ? section_weight_cb : NULL;
  q->opt.func_arg = q->weight_set;
  q->snip_conds = NULL;
  return q;
}

unsigned int
grn_query_rest(grn_ctx *ctx, grn_query *q, const char ** const rest)
{
  if (!q) { return 0; }
  if (rest) {
    *rest = q->cur;
  }
  return (unsigned int)(q->str_end - q->cur);
}

grn_rc
grn_query_close(grn_ctx *ctx, grn_query *q)
{
  if (!q) { return GRN_INVALID_ARGUMENT; }
  if (q->opt.weight_vector) {
    GRN_FREE(q->opt.weight_vector);
  }
  if (q->weight_set) {
    grn_hash_close(ctx, q->weight_set);
  }
  if (q->snip_conds) {
    snip_cond *sc;
    for (sc = q->snip_conds; sc < q->snip_conds + q->cur_expr; sc++) {
      grn_snip_cond_close(ctx, sc);
    }
    GRN_FREE(q->snip_conds);
  }
  GRN_FREE(q);
  return GRN_SUCCESS;
}

/* FIXME: for test */
grn_rc
grn_query_str(grn_query *q, const char **str, unsigned int *len)
{
  if (str) { *str = q->str; }
  if (len) { *len = q->str_end - q->str; }
  return GRN_SUCCESS;
}

static void
scan_keyword(snip_cond *sc, grn_str *str, grn_id section,
             grn_operator op, grn_select_optarg *optarg,
             int *found, int *score)
{
  int tf;
  int w = 1;
  for (tf = 0; ; tf++) {
    grn_bm_tunedbm(sc, str, 0);
    if (sc->stopflag == SNIPCOND_STOP) { break; }
  }
  if (optarg->vector_size) {
    if (!optarg->weight_vector) {
      w = optarg->vector_size;
    } else if (section) {
      w = (section <= optarg->vector_size ?
                      optarg->weight_vector[section - 1] : 0);
    }
  }
  switch (op) {
  case GRN_OP_OR :
    if (tf) {
      *found = 1;
      *score += w * tf;
    }
    break;
  case GRN_OP_AND :
    if (tf) {
      *score += w * tf;
    } else {
      *found = 0;
    }
    break;
  case GRN_OP_BUT :
    if (tf) {
      *found = 0;
    }
    break;
  case GRN_OP_ADJUST :
    *score += w * tf;
  default :
    break;
  }
}

/* TODO: delete overlapping logic with exec_query */
static grn_rc
scan_query(grn_ctx *ctx, grn_query *q, grn_str *nstr, grn_id section, grn_cell *c, snip_cond **sc,
           grn_operator op, int flags, int *found, int *score)
{
  int _found = 0, _score = 0;
  grn_cell *e, *ope = NIL;
  grn_operator op0 = GRN_OP_OR, *opp = &op0, op1 = q->default_op;
  while (c != NIL) {
    POP(e, c);
    switch (e->header.type) {
    case GRN_CELL_OP :
      if (opp == &op0 && e->u.op.op == GRN_OP_BUT) {
        POP(e, c);
      } else {
        ope = e;
        op1 = ope->u.op.op;
      }
      continue;
    case GRN_CELL_STR :
      if (ope != NIL) {
        q->opt.mode = ope->u.op.mode == -1 ? q->default_mode : ope->u.op.mode;
        q->opt.max_interval = q->opt.similarity_threshold = ope->u.op.option;
        if (!q->opt.weight_vector) {
          q->opt.vector_size = ope->u.op.weight + q->weight_offset;
        }
      } else {
        q->opt.mode = q->default_mode;
        q->opt.max_interval = DEFAULT_MAX_INTERVAL;
        q->opt.similarity_threshold = DEFAULT_SIMILARITY_THRESHOLD;
        if (!q->opt.weight_vector) {
          q->opt.vector_size = DEFAULT_WEIGHT + q->weight_offset;
        }
      }
      if ((flags & GRN_QUERY_SCAN_ALLOCCONDS)) {
        grn_rc rc;
        /* NOTE: GRN_SNIP_NORMALIZE = GRN_QUERY_SCAN_NORMALIZE */
        if ((rc = grn_snip_cond_init(ctx, *sc, e->u.b.value, e->u.b.size,
                                     q->encoding, flags & GRN_SNIP_NORMALIZE))) {
          return rc;
        }
      } else {
        grn_snip_cond_reinit(*sc);
      }
      scan_keyword(*sc, nstr, section, *opp, &q->opt, &_found, &_score);
      (*sc)++;
      break;
    case GRN_CELL_LIST :
      scan_query(ctx, q, nstr, section, e, sc, *opp, flags, &_found, &_score);
      break;
    default :
      GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid object assigned in query! (%d)", e->header.type);
      break;
    }
    opp = &op1;
    ope = NIL;
    op1 = q->default_op;
  }
  switch (op) {
  case GRN_OP_OR :
    *found |= _found;
    *score += _score;
    break;
  case GRN_OP_AND :
    *found &= _found;
    *score += _score;
    break;
  case GRN_OP_BUT :
    *found &= !_found;
    break;
  case GRN_OP_ADJUST :
    *score += _score;
    break;
  default :
    break;
  }
  return GRN_SUCCESS;
}

static grn_rc
alloc_snip_conds(grn_ctx *ctx, grn_query *q)
{
  if (!(q->snip_conds = GRN_CALLOC(sizeof(snip_cond) * q->cur_expr))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "snip_cond allocation failed");
    return GRN_NO_MEMORY_AVAILABLE;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_query_scan(grn_ctx *ctx, grn_query *q, const char **strs, unsigned int *str_lens, unsigned int nstrs,
               int flags, int *found, int *score)
{
  unsigned int i;
  grn_rc rc;
  if (!q || !strs || !nstrs) { return GRN_INVALID_ARGUMENT; }
  *found = *score = 0;
  if (!q->snip_conds) {
    if ((rc = alloc_snip_conds(ctx, q))) { return rc; }
    flags |= GRN_QUERY_SCAN_ALLOCCONDS;
  } else if (flags & GRN_QUERY_SCAN_ALLOCCONDS) {
    GRN_LOG(ctx, GRN_LOG_WARNING, "invalid flags specified on grn_query_scan");
    return GRN_INVALID_ARGUMENT;
  }
  for (i = 0; i < nstrs; i++) {
    grn_str *n;
    snip_cond *sc = q->snip_conds;
    int f = GRN_STR_WITH_CHECKS | GRN_STR_REMOVEBLANK;
    if (flags & GRN_QUERY_SCAN_NORMALIZE) { f |= GRN_STR_NORMALIZE; }
    n = grn_str_open(ctx, *(strs + i), *(str_lens + i), f);
    if (!n) { return GRN_NO_MEMORY_AVAILABLE; }
    if ((rc = scan_query(ctx, q, n, i + 1, q->expr, &sc, GRN_OP_OR, flags, found, score))) {
      grn_str_close(ctx, n);
      return rc;
    }
    flags &= ~GRN_QUERY_SCAN_ALLOCCONDS;
    grn_str_close(ctx, n);
  }
  return GRN_SUCCESS;
}

/* TODO: delete overlapping logic with exec_query */
static grn_rc
snip_query(grn_ctx *ctx, grn_query *q, grn_snip *snip, grn_cell *c, grn_operator op,
           unsigned int n_tags, int c_but,
           const char **opentags, unsigned int *opentag_lens,
           const char **closetags, unsigned int *closetag_lens)
{
  grn_cell *e, *ope = NIL;
  grn_operator op0 = GRN_OP_OR, *opp = &op0, op1 = q->default_op;
  while (c != NIL) {
    POP(e, c);
    switch (e->header.type) {
    case GRN_CELL_OP :
      ope = e;
      op1 = ope->u.op.op;
      continue;
    case GRN_CELL_STR :
      if (ope != NIL) {
        q->opt.mode = ope->u.op.mode == -1 ? q->default_mode : ope->u.op.mode;
      } else {
        q->opt.mode = q->default_mode;
      }
      if (!(c_but ^ (*opp == GRN_OP_BUT))) {
        grn_rc rc;
        unsigned int i = snip->cond_len % n_tags;
        if ((rc = grn_snip_add_cond(ctx, snip, e->u.b.value, e->u.b.size,
                                    opentags[i], opentag_lens[i],
                                    closetags[i], closetag_lens[i]))) {
          return rc;
        }
      }
      break;
    case GRN_CELL_LIST :
      snip_query(ctx, q, snip, e, *opp, n_tags, (*opp == GRN_OP_BUT) ? c_but ^ 1 : c_but,
                 opentags, opentag_lens, closetags, closetag_lens);
      break;
    default :
      GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid object assigned in query!! (%d)", e->header.type);
      break;
    }
    opp = &op1;
    ope = NIL;
    op1 = q->default_op;
  }
  return GRN_SUCCESS;
}

grn_snip *
grn_query_snip(grn_ctx *ctx, grn_query *query, int flags,
               unsigned int width, unsigned int max_results,
               unsigned int n_tags,
               const char **opentags, unsigned int *opentag_lens,
               const char **closetags, unsigned int *closetag_lens,
               grn_snip_mapping *mapping)
{
  grn_snip *res;
  if (!(res = grn_snip_open(ctx, flags, width, max_results,
                            NULL, 0, NULL, 0, mapping))) {
    return NULL;
  }
  if (snip_query(ctx, query, res, query->expr, GRN_OP_OR, n_tags, 0,
                 opentags, opentag_lens, closetags, closetag_lens)) {
    grn_snip_close(ctx, res);
    return NULL;
  }
  return res;
}

static void
exec_search(grn_ctx *ctx, grn_ii *i, grn_query *q, grn_cell *c,
            grn_hash *r, grn_operator op)
{
  grn_hash *s;
  grn_cell *e, *ope = NIL;
  int n = *r->n_entries;
  grn_operator op0 = GRN_OP_OR, *opp = &op0, op1 = q->default_op;
  if (!n && op != GRN_OP_OR) { return; }
  if (n) {
    s = grn_hash_create(ctx, NULL, r->key_size, r->value_size, r->obj.header.flags);
    s->obj.header.impl_flags = 0;
    s->obj.header.domain = r->obj.header.domain;
    s->obj.range = r->obj.range;
    s->obj.max_n_subrecs = r->obj.max_n_subrecs;
    s->obj.subrec_size = r->obj.subrec_size;
    s->obj.subrec_offset = r->obj.subrec_offset;
    s->obj.id = r->obj.id;
    s->obj.db = r->obj.db;
    s->obj.source = r->obj.source;
    s->obj.source_size = r->obj.source_size;
    /*
    grn_hook_entry entry;
    for (entry = 0; entry < N_HOOK_ENTRIES; entry++) {
      s->obj.hooks[entry] = NULL;
    }
    */
  } else {
    s = r;
  }
  while (c != NIL) {
    POP(e, c);
    switch (e->header.type) {
    case GRN_CELL_OP :
      if (opp == &op0 && e->u.op.op == GRN_OP_BUT) {
        POP(e, c);
      } else {
        ope = e;
        op1 = ope->u.op.op;
      }
      continue;
    case GRN_CELL_STR :
      if (ope != NIL) {
        q->opt.mode = ope->u.op.mode == -1 ? q->default_mode : ope->u.op.mode;
        q->opt.max_interval = q->opt.similarity_threshold = ope->u.op.option;
        if (!q->opt.weight_vector) {
          q->opt.vector_size = ope->u.op.weight + q->weight_offset;
        }
        if (ope->u.op.mode == GRN_OP_SIMILAR) {
          q->opt.max_interval = q->default_mode;
        }
      } else {
        q->opt.mode = q->default_mode;
        q->opt.max_interval = DEFAULT_MAX_INTERVAL;
        q->opt.similarity_threshold = DEFAULT_SIMILARITY_THRESHOLD;
        if (!q->opt.weight_vector) {
          q->opt.vector_size = DEFAULT_WEIGHT + q->weight_offset;
        }
      }
      if (grn_ii_select(ctx, i, e->u.b.value, e->u.b.size, s, *opp, &q->opt)) {
        GRN_LOG(ctx, GRN_LOG_ERROR, "grn_inv_select on exec_search failed !");
        return;
      }
      break;
    case GRN_CELL_LIST :
      exec_search(ctx, i, q, e, s, *opp);
      break;
    default :
      GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid object assigned in query (%d)", e->header.type);
      break;
    }
    opp = &op1;
    ope = NIL;
    op1 = q->default_op;
  }
  if (n) {
    grn_table_setoperation(ctx, (grn_obj *)r, (grn_obj *)s, (grn_obj *)r, op);
    grn_hash_close(ctx, s);
  }
}

grn_rc
grn_query_search(grn_ctx *ctx, grn_ii *i, grn_query *q, grn_hash *r, grn_operator op)
{
  int p = q->escalation_threshold;
  // dump_query(q, q->expr, 0);
  // grn_log("escalation_threshold=%d", p);
  if (p >= 0 || (-p & 1)) {
    q->default_mode = GRN_OP_EXACT;
    exec_search(ctx, i, q, q->expr, r, op);
    GRN_LOG(ctx, GRN_LOG_INFO, "hits(exact)=%d", *r->n_entries);
  }
  if ((p >= 0) ? (p >= *r->n_entries) : (-p & 2)) {
    q->weight_offset -= q->escalation_decaystep;
    q->default_mode = GRN_OP_UNSPLIT;
    exec_search(ctx, i, q, q->expr, r, op);
    GRN_LOG(ctx, GRN_LOG_INFO, "hits(unsplit)=%d", *r->n_entries);
  }
  if ((p >= 0) ? (p >= *r->n_entries) : (-p & 4)) {
    q->weight_offset -= q->escalation_decaystep;
    q->default_mode = GRN_OP_PARTIAL;
    exec_search(ctx, i, q, q->expr, r, op);
    GRN_LOG(ctx, GRN_LOG_INFO, "hits(partial)=%d", *r->n_entries);
  }
  return GRN_SUCCESS;
}

/**** procs ****/

static grn_rc
selector(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 14) {
    grn_search(ctx, outbuf, GRN_CONTENT_JSON,
               GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value),
               GRN_TEXT_VALUE(&vars[1].value), GRN_TEXT_LEN(&vars[1].value),
               grn_atoi(GRN_TEXT_VALUE(&vars[2].value), GRN_BULK_CURR(&vars[2].value), NULL),
               grn_atoi(GRN_TEXT_VALUE(&vars[3].value), GRN_BULK_CURR(&vars[3].value), NULL),
               GRN_TEXT_VALUE(&vars[4].value), GRN_TEXT_LEN(&vars[4].value),
               GRN_TEXT_VALUE(&vars[5].value), GRN_TEXT_LEN(&vars[5].value),
               GRN_TEXT_VALUE(&vars[6].value), GRN_TEXT_LEN(&vars[6].value),
               GRN_TEXT_VALUE(&vars[7].value), GRN_TEXT_LEN(&vars[7].value),
               GRN_TEXT_VALUE(&vars[8].value), GRN_TEXT_LEN(&vars[8].value),
               GRN_TEXT_VALUE(&vars[9].value), GRN_TEXT_LEN(&vars[9].value),
               grn_atoi(GRN_TEXT_VALUE(&vars[10].value), GRN_BULK_CURR(&vars[10].value), NULL),
               grn_atoi(GRN_TEXT_VALUE(&vars[11].value), GRN_BULK_CURR(&vars[11].value), NULL),
               GRN_TEXT_VALUE(&vars[12].value), GRN_TEXT_LEN(&vars[12].value),
               GRN_TEXT_VALUE(&vars[13].value), GRN_TEXT_LEN(&vars[13].value));
  }
  grn_ctx_push(ctx, outbuf);
  return ctx->rc;
}

static grn_rc
define_selector(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  grn_obj *outbuf = grn_ctx_pop(ctx);
  if (grn_proc_create(ctx,
                      GRN_TEXT_VALUE(&vars[0].value),
                      GRN_TEXT_LEN(&vars[0].value),
                      NULL, selector, NULL, NULL, nvars - 1, vars + 1)) {
    GRN_TEXT_PUT(ctx, outbuf, GRN_TEXT_VALUE(&vars[0].value), GRN_TEXT_LEN(&vars[0].value));
  }
  grn_ctx_push(ctx, outbuf);
  return ctx->rc;
}

static grn_rc
loader(grn_ctx *ctx, grn_obj *obj, grn_user_data *user_data)
{
  uint32_t nvars;
  grn_obj *outbuf = grn_ctx_pop(ctx);
  grn_expr_var *vars = grn_proc_vars(ctx, user_data, &nvars);
  if (nvars == 4) {
    grn_load(ctx, GRN_CONTENT_JSON,
             GRN_TEXT_VALUE(&vars[1].value), GRN_TEXT_LEN(&vars[1].value),
             GRN_TEXT_VALUE(&vars[2].value), GRN_TEXT_LEN(&vars[2].value),
             GRN_TEXT_VALUE(&vars[3].value), GRN_TEXT_LEN(&vars[3].value));
    if (!GRN_BULK_VSIZE(&ctx->impl->loader.level)) {
      grn_text_itoa(ctx, outbuf, ctx->impl->loader.nrecords);
    }
  }
  grn_ctx_push(ctx, outbuf);
  return ctx->rc;
}

#define DEF_VAR(v,name_str) {\
  (v).name = (name_str);\
  (v).name_size = strlen(name_str);\
  GRN_TEXT_INIT(&(v).value, 0);\
}

void
grn_db_init_builtin_query(grn_ctx *ctx)
{
  grn_expr_var vars[15];
  DEF_VAR(vars[0], "name");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "match_column");
  DEF_VAR(vars[3], "offset");
  DEF_VAR(vars[4], "hits");
  DEF_VAR(vars[5], "output_columns");
  DEF_VAR(vars[6], "query");
  DEF_VAR(vars[7], "filter");
  DEF_VAR(vars[8], "foreach");
  DEF_VAR(vars[9], "sortby");
  DEF_VAR(vars[10], "drilldown");
  DEF_VAR(vars[11], "drilldown_offset");
  DEF_VAR(vars[12], "drilldown_hits");
  DEF_VAR(vars[13], "drilldown_output_columns");
  DEF_VAR(vars[14], "drilldown_sortby");
  grn_proc_create(ctx, "/q/define_selector", 18, NULL, define_selector, NULL, NULL, 15, vars);
  DEF_VAR(vars[0], "input_type");
  DEF_VAR(vars[1], "table");
  DEF_VAR(vars[2], "columns");
  DEF_VAR(vars[3], "values");
  grn_proc_create(ctx, "/q/loader", 9, NULL, loader, NULL, NULL, 4, vars);
}
