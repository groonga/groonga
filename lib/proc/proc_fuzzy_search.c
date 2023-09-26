/*
  Copyright (C) 2009-2016  Brazil
  Copyright (C) 2019-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_db.h"
#include "../grn_ii.h"
#include "../grn_proc.h"
#include "../grn_rset.h"
#include "../grn_posting.h"

#include <groonga/plugin.h>

#include <string.h>

#define DIST(ox,oy) (dists[((lx + 1) * (oy)) + (ox)])

static uint32_t
calc_edit_distance(grn_ctx *ctx, char *sx, char *ex, char *sy, char *ey, int flags)
{
  uint32_t d = 0;
  int cx;
  int cy;
  uint32_t lx, ly, *dists;
  char *px, *py;
  for (px = sx, lx = 0;
       px < ex && (cx = grn_charlen(ctx, px, ex));
       px += cx, lx++) {
  }
  for (py = sy, ly = 0;
       py < ey && (cy = grn_charlen(ctx, py, ey));
       py += cy, ly++) {
  }
  if ((dists = GRN_PLUGIN_MALLOC(ctx, (lx + 1) * (ly + 1) * sizeof(uint32_t)))) {
    uint32_t x, y;
    for (x = 0; x <= lx; x++) { DIST(x, 0) = x; }
    for (y = 0; y <= ly; y++) { DIST(0, y) = y; }
    for (x = 1, px = sx; x <= lx; x++, px += cx) {
      cx = grn_charlen(ctx, px, ex);
      for (y = 1, py = sy; y <= ly; y++, py += cy) {
        cy = grn_charlen(ctx, py, ey);
        if (cx == cy && !memcmp(px, py, (size_t)cx)) {
          DIST(x, y) = DIST(x - 1, y - 1);
        } else {
          uint32_t a = DIST(x - 1, y) + 1;
          uint32_t b = DIST(x, y - 1) + 1;
          uint32_t c = DIST(x - 1, y - 1) + 1;
          DIST(x, y) = ((a < b) ? ((a < c) ? a : c) : ((b < c) ? b : c));
          if (flags & GRN_TABLE_FUZZY_SEARCH_WITH_TRANSPOSITION &&
              x > 1 && y > 1 && cx == cy &&
              memcmp(px, py - cy, (size_t)cx) == 0 &&
              memcmp(px - cx, py, (size_t)cx) == 0) {
            uint32_t t = DIST(x - 2, y - 2) + 1;
            DIST(x, y) = ((DIST(x, y) < t) ? DIST(x, y) : t);
          }
        }
      }
    }
    d = DIST(lx, ly);
    GRN_PLUGIN_FREE(ctx, dists);
  }
  return d;
}

static grn_obj *
func_edit_distance(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
#define N_REQUIRED_ARGS 2
#define MAX_ARGS 3
  uint32_t d = 0;
  int flags = 0;
  grn_obj *obj;
  if (nargs >= N_REQUIRED_ARGS && nargs <= MAX_ARGS) {
    if (nargs == MAX_ARGS && GRN_BOOL_VALUE(args[2])) {
      flags |= GRN_TABLE_FUZZY_SEARCH_WITH_TRANSPOSITION;
    }
    d = calc_edit_distance(ctx,
                           GRN_TEXT_VALUE(args[0]),
                           GRN_BULK_CURR(args[0]),
                           GRN_TEXT_VALUE(args[1]),
                           GRN_BULK_CURR(args[1]),
                           flags);
  }
  if ((obj = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, d);
  }
  return obj;
#undef N_REQUIRED_ARGS
#undef MAX_ARGS
}

void
grn_proc_init_edit_distance(grn_ctx *ctx)
{
  grn_proc_create(ctx, "edit_distance", -1, GRN_PROC_FUNCTION,
                  func_edit_distance, NULL, NULL, 0, NULL);
}

#define SCORE_HEAP_SIZE 256

typedef struct {
  grn_id id;
  uint32_t score;
} score_heap_node;

typedef struct {
  uint32_t n_entries;
  uint32_t limit;
  score_heap_node *nodes;
} score_heap;

static grn_inline score_heap *
score_heap_open(grn_ctx *ctx, uint32_t max)
{
  score_heap *h = GRN_PLUGIN_MALLOC(ctx, sizeof(score_heap));
  if (!h) { return NULL; }
  h->nodes = GRN_PLUGIN_MALLOC(ctx, sizeof(score_heap_node) * max);
  if (!h->nodes) {
    GRN_PLUGIN_FREE(ctx, h);
    return NULL;
  }
  h->n_entries = 0;
  h->limit = max;
  return h;
}

static grn_inline grn_bool
score_heap_push(grn_ctx *ctx, score_heap *h, grn_id id, uint32_t score)
{
  uint32_t n, n2;
  score_heap_node node = {id, score};
  score_heap_node node2;
  if (h->n_entries >= h->limit) {
    uint32_t max = h->limit * 2;
    score_heap_node *nodes;
    nodes = GRN_PLUGIN_REALLOC(ctx, h->nodes, sizeof(score_heap) * max);
    if (!nodes) {
      return GRN_FALSE;
    }
    h->limit = max;
    h->nodes = nodes;
  }
  h->nodes[h->n_entries] = node;
  n = h->n_entries++;
  while (n) {
    n2 = (n - 1) >> 1;
    if (h->nodes[n2].score <= h->nodes[n].score) { break; }
    node2 = h->nodes[n];
    h->nodes[n] = h->nodes[n2];
    h->nodes[n2] = node2;
    n = n2;
  }
  return GRN_TRUE;
}

static grn_inline void
score_heap_close(grn_ctx *ctx, score_heap *h)
{
  GRN_PLUGIN_FREE(ctx, h->nodes);
  GRN_PLUGIN_FREE(ctx, h);
}

static grn_rc
sequential_fuzzy_search(grn_ctx *ctx, grn_obj *table, grn_obj *column, grn_obj *query,
                        uint32_t max_distance, uint32_t prefix_match_size,
                        uint32_t max_expansion, int flags, grn_obj *res, grn_operator op)
{
  grn_table_cursor *tc;
  char *sx = GRN_TEXT_VALUE(query);
  char *ex = GRN_BULK_CURR(query);

  if (op == GRN_OP_AND) {
    tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
  } else {
    tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, GRN_CURSOR_BY_ID);
  }
  if (tc) {
    grn_id id;
    grn_obj value;
    score_heap *heap;
    uint32_t i, n;
    GRN_TEXT_INIT(&value, 0);

    heap = score_heap_open(ctx, SCORE_HEAP_SIZE);
    if (!heap) {
      grn_table_cursor_close(ctx, tc);
      grn_obj_unlink(ctx, &value);
      return GRN_NO_MEMORY_AVAILABLE;
    }

    while ((id = grn_table_cursor_next(ctx, tc))) {
      unsigned int distance = 0;
      grn_obj *domain;
      grn_id record_id;

      if (op == GRN_OP_AND) {
        grn_id *key;
        grn_table_cursor_get_key(ctx, tc, (void **)&key);
        record_id = *key;
      } else {
        record_id = id;
      }
      GRN_BULK_REWIND(&value);
      grn_obj_get_value(ctx, column, record_id, &value);
      domain = grn_ctx_at(ctx, ((&value))->header.domain);
      if ((&(value))->header.type == GRN_VECTOR) {
        n = grn_vector_size(ctx, &value);
        for (i = 0; i < n; i++) {
          unsigned int length;
          const char *vector_value = NULL;
          length = grn_vector_get_element(ctx, &value, i, &vector_value, NULL, NULL);

          if (!prefix_match_size ||
              (prefix_match_size > 0 && length >= prefix_match_size &&
               !memcmp(sx, vector_value, prefix_match_size))) {
            distance = calc_edit_distance(ctx, sx, ex,
                                          (char *)vector_value,
                                          (char *)vector_value + length, flags);
            if (distance <= max_distance) {
              score_heap_push(ctx, heap, record_id, distance);
              break;
            }
          }
        }
      } else if ((&(value))->header.type == GRN_UVECTOR &&
                  grn_obj_is_table(ctx, domain)) {
        n = grn_vector_size(ctx, &value);
        for (i = 0; i < n; i++) {
          grn_id rid;
          char key_name[GRN_TABLE_MAX_KEY_SIZE];
          int key_length;
          rid = grn_uvector_get_element(ctx, &value, i, NULL);
          key_length = grn_table_get_key(ctx, domain, rid, key_name, GRN_TABLE_MAX_KEY_SIZE);

          if (!prefix_match_size ||
              (prefix_match_size > 0 &&
               (uint32_t)key_length >= prefix_match_size &&
               !memcmp(sx, key_name, prefix_match_size))) {
            distance = calc_edit_distance(ctx, sx, ex,
                                          key_name, key_name + key_length, flags);
            if (distance <= max_distance) {
              score_heap_push(ctx, heap, record_id, distance);
              break;
            }
          }
        }
      } else {
        if (grn_obj_is_reference_column(ctx, column)) {
          grn_id rid;
          char key_name[GRN_TABLE_MAX_KEY_SIZE];
          int key_length;
          rid = GRN_RECORD_VALUE(&value);
          key_length = grn_table_get_key(ctx, domain, rid, key_name, GRN_TABLE_MAX_KEY_SIZE);
          if (!prefix_match_size ||
              (prefix_match_size > 0 &&
               (uint32_t)key_length >= prefix_match_size &&
               !memcmp(sx, key_name, prefix_match_size))) {
            distance = calc_edit_distance(ctx, sx, ex,
                                          key_name, key_name + key_length, flags);
            if (distance <= max_distance) {
              score_heap_push(ctx, heap, record_id, distance);
            }
          }
        } else {
          if (!prefix_match_size ||
              (prefix_match_size > 0 && GRN_TEXT_LEN(&value) >= prefix_match_size &&
               !memcmp(sx, GRN_TEXT_VALUE(&value), prefix_match_size))) {
            distance = calc_edit_distance(ctx, sx, ex,
                                          GRN_TEXT_VALUE(&value),
                                          GRN_BULK_CURR(&value), flags);
            if (distance <= max_distance) {
              score_heap_push(ctx, heap, record_id, distance);
            }
          }
        }
      }
      grn_obj_unlink(ctx, domain);
    }
    grn_table_cursor_close(ctx, tc);
    grn_obj_unlink(ctx, &value);

    for (i = 0; i < heap->n_entries; i++) {
      if (max_expansion > 0 && (size_t)i >= max_expansion) {
        break;
      }
      {
        grn_posting_internal posting = {0};
        posting.rid = heap->nodes[i].id;
        posting.sid = 1;
        posting.pos = 0;
        posting.weight_float = (float)(max_distance - heap->nodes[i].score + 1);
        grn_ii_posting_add_float(ctx,
                                 (grn_posting *)(&posting),
                                 (grn_hash *)res,
                                 op);
      }
    }
    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, op);
    score_heap_close(ctx, heap);
  }

  return GRN_SUCCESS;
}

typedef struct {
  grn_obj *target;
  grn_obj *query;
  uint32_t max_distance;
  uint32_t prefix_length;
  uint32_t prefix_match_size;
  uint32_t max_expansions;
  int flags;
} fuzzy_search_data;

static grn_rc
selector_fuzzy_search_execute(grn_ctx *ctx,
                              grn_obj *index,
                              grn_operator op,
                              grn_obj *res,
                              grn_operator logical_op,
                              void *user_data)
{
  grn_rc rc = GRN_SUCCESS;
  fuzzy_search_data *data = user_data;
  grn_obj *target = index;
  bool use_sequential_search = false;

  if (grn_obj_is_accessor(ctx, target)) {
    if (grn_obj_is_key_accessor(ctx, target) &&
        ((grn_accessor *)target)->obj->header.type == GRN_TABLE_PAT_KEY) {
      target = ((grn_accessor *)target)->obj;
      use_sequential_search = false;
    } else {
      use_sequential_search = true;
    }
  } else if (target) {
    use_sequential_search = true;
    if (target->header.type == GRN_TABLE_PAT_KEY) {
      use_sequential_search = false;
    } else {
      grn_obj *lexicon;
      lexicon = grn_ctx_at(ctx, target->header.domain);
      if (lexicon && lexicon->header.type == GRN_TABLE_PAT_KEY) {
        use_sequential_search = false;
      }
    }
  } else {
    use_sequential_search = true;
  }

  if (data->prefix_length) {
    const char *s = GRN_TEXT_VALUE(data->query);
    const char *e = GRN_BULK_CURR(data->query);
    const char *p;
    int cl = 0;
    unsigned int length = 0;
    for (p = s; p < e && (cl = grn_charlen(ctx, p, e)); p += cl) {
      length++;
      if (length > data->prefix_length) {
        break;
      }
    }
    data->prefix_match_size = (uint32_t)(p - s);
  }

  if (use_sequential_search) {
    grn_obj *table;
    table = grn_ctx_at(ctx, res->header.domain);
    rc = sequential_fuzzy_search(ctx,
                                 table,
                                 data->target,
                                 data->query,
                                 data->max_distance,
                                 data->prefix_match_size,
                                 data->max_expansions,
                                 data->flags,
                                 res,
                                 logical_op);
    goto exit;
  }

  if (!target) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "fuzzy_search(): "
                     "column must be COLUMN_INDEX or TABLE_PAT_KEY: <%.*s>",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    rc = ctx->rc;
    GRN_OBJ_FIN(ctx, &inspected);
  } else {
    grn_search_optarg options = {0};
    options.mode = GRN_OP_FUZZY;
    options.fuzzy.prefix_match_size = data->prefix_match_size;
    options.fuzzy.max_distance = data->max_distance;
    options.fuzzy.max_expansion = data->max_expansions;
    options.fuzzy.flags = data->flags;
    grn_obj_search(ctx, target, data->query, res, logical_op, &options);
  }

exit :
  return rc;
}

static grn_rc
selector_fuzzy_search(grn_ctx *ctx, grn_obj *table, grn_obj *index,
                      int nargs, grn_obj **args,
                      grn_obj *res, grn_operator op)
{
  const char *tag = "[fuzzy-search]";
  grn_rc rc = GRN_SUCCESS;

  if ((nargs - 1) < 2) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 2..)",
                     tag,
                     nargs - 1);
    rc = ctx->rc;
    goto exit;
  }

  fuzzy_search_data data;
  data.target = args[1];
  data.query = args[2];
  data.max_distance = 1;
  data.prefix_length = 0;
  data.prefix_match_size = 0;
  data.max_expansions = 0;
  data.flags = 0;

  if (nargs == 4) {
    grn_obj *options = args[3];

    switch (options->header.type) {
    case GRN_BULK :
      data.max_distance = GRN_UINT32_VALUE(options);
      break;
    case GRN_TABLE_HASH_KEY :
      {
        bool with_transposition = false;
        rc = grn_proc_options_parse(ctx,
                                    options,
                                    tag,
                                    "max_distance",
                                    GRN_PROC_OPTION_VALUE_UINT32,
                                    &(data.max_distance),
                                    "prefix_length",
                                    GRN_PROC_OPTION_VALUE_UINT32,
                                    &(data.prefix_length),
                                    /* Deprecated since 13.0.8.
                                     * Keep this for backward compatibility. */
                                    "max_expansion",
                                    GRN_PROC_OPTION_VALUE_UINT32,
                                    &(data.max_expansions),
                                    "max_expansions",
                                    GRN_PROC_OPTION_VALUE_UINT32,
                                    &(data.max_expansions),
                                    "with_transposition",
                                    GRN_PROC_OPTION_VALUE_BOOL,
                                    &with_transposition,
                                    NULL);
        if (rc != GRN_SUCCESS) {
          goto exit;
        }
        if (with_transposition) {
          data.flags |= GRN_TABLE_FUZZY_SEARCH_WITH_TRANSPOSITION;
        }
      }
      break;
    default :
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "%s "
                       "3rd argument must be integer or object literal: <%.*s>",
                       tag,
                       (int)GRN_TEXT_LEN(options),
                       GRN_TEXT_VALUE(options));
      goto exit;
    }
  }

  if (grn_obj_is_accessor(ctx, data.target)) {
    rc = grn_accessor_execute(ctx,
                              data.target,
                              selector_fuzzy_search_execute,
                              &data,
                              GRN_OP_FUZZY,
                              res,
                              op);
  } else {
    grn_index_datum index_datum;
    unsigned int n_index_datum;
    if (grn_obj_is_index_column(ctx, data.target)) {
      index_datum.index = data.target;
      n_index_datum = 1;
    } else {
      n_index_datum = grn_column_find_index_data(ctx,
                                                 data.target,
                                                 GRN_OP_FUZZY,
                                                 &index_datum,
                                                 1);
    }
    if (n_index_datum == 0) {
      index_datum.index = NULL;
    }
    rc = selector_fuzzy_search_execute(ctx,
                                       index_datum.index,
                                       GRN_OP_FUZZY,
                                       res,
                                       op,
                                       &data);
  }

exit :
  return rc;
}

void
grn_proc_init_fuzzy_search(grn_ctx *ctx)
{
  grn_obj *selector_proc;

  selector_proc = grn_proc_create(ctx, "fuzzy_search", -1,
                                  GRN_PROC_FUNCTION,
                                  NULL, NULL, NULL, 0, NULL);
  grn_proc_set_selector(ctx, selector_proc, selector_fuzzy_search);
  grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
}
