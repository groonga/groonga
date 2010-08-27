/* Copyright(C) 2010 Brazil

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

#include "db.h"
#include "ii.h"
#include "token.h"
#include "output.h"
#include <string.h>

#define VAR GRN_PROC_GET_VAR_BY_OFFSET
#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define TEXT_VALUE_LEN(x) GRN_TEXT_VALUE(x), GRN_TEXT_LEN(x)

#define COMPLETE 1
#define CORRECT  2
#define SUGGEST  4

static int
grn_parse_suggest_types(const char *nptr, const char *end)
{
  int types = 0;
  while (nptr < end) {
    if (*nptr == '|') {
      nptr += 1;
      continue;
    }
    if (!memcmp(nptr, CONST_STR_LEN("complete"))) {
      types |= COMPLETE;
      nptr += sizeof("complete") - 1;
    } else if (!memcmp(nptr, CONST_STR_LEN("correct"))) {
      types |= CORRECT;
      nptr += sizeof("correct") - 1;
    } else if (!memcmp(nptr, CONST_STR_LEN("suggest"))) {
      types |= SUGGEST;
      nptr += sizeof("suggest") - 1;
    } else {
      break;
    }
  }
  return types;
}

static grn_id
cooccur_search(grn_ctx *ctx, grn_obj *table, grn_obj *query, grn_obj *res, int query_type)
{
  grn_id id = grn_table_get(ctx, table, TEXT_VALUE_LEN(query));
  if (id) {
    grn_ii_cursor *c;
    grn_obj *co = grn_obj_column(ctx, table, CONST_STR_LEN("co"));
    grn_obj *pairs = grn_ctx_at(ctx, grn_obj_get_range(ctx, co));
    grn_obj *items_freq = grn_obj_column(ctx, table, CONST_STR_LEN("freq"));
    grn_obj *pairs_freq, *pairs_post = grn_obj_column(ctx, pairs, CONST_STR_LEN("post"));
    switch (query_type) {
    case COMPLETE :
      pairs_freq = grn_obj_column(ctx, pairs, CONST_STR_LEN("freq0"));
      break;
    case CORRECT :
      pairs_freq = grn_obj_column(ctx, pairs, CONST_STR_LEN("freq1"));
      break;
    case SUGGEST :
      pairs_freq = grn_obj_column(ctx, pairs, CONST_STR_LEN("freq2"));
      break;
    default :
      return id;
    }
    if ((c = grn_ii_cursor_open(ctx, (grn_ii *)co, id, GRN_ID_NIL, GRN_ID_MAX,
                                ((grn_ii *)co)->n_elements - 1, 0))) {
      grn_ii_posting *p;
      grn_obj post, pair_freq, item_freq;
      GRN_RECORD_INIT(&post, 0, grn_obj_id(ctx, table));
      GRN_INT32_INIT(&pair_freq, 0);
      GRN_INT32_INIT(&item_freq, 0);
      while ((p = grn_ii_cursor_next(ctx, c))) {
        grn_id post_id;
        int pfreq, ifreq;
        GRN_BULK_REWIND(&post);
        GRN_BULK_REWIND(&pair_freq);
        GRN_BULK_REWIND(&item_freq);
        grn_obj_get_value(ctx, pairs_post, p->rid, &post);
        grn_obj_get_value(ctx, pairs_freq, p->rid, &pair_freq);
        post_id = GRN_RECORD_VALUE(&post);
        grn_obj_get_value(ctx, items_freq, post_id, &item_freq);
        pfreq = GRN_INT32_VALUE(&pair_freq);
        ifreq = GRN_INT32_VALUE(&item_freq);
        if (pfreq && ifreq) {
          grn_rset_recinfo *ri;
          uint32_t score = 1000/pfreq;
          /* put any formula if desired */
          if (grn_hash_add(ctx, (grn_hash *)res,
                           &post_id, sizeof(grn_id), (void **)&ri, NULL)) {
            ri->score += score;
          }
        }
      }
      GRN_OBJ_FIN(ctx, &post);
      GRN_OBJ_FIN(ctx, &pair_freq);
      GRN_OBJ_FIN(ctx, &item_freq);
      grn_ii_cursor_close(ctx, c);
    }
  }
  return id;
}

static void
output(grn_ctx *ctx, grn_obj *table, grn_obj *res, int limit, grn_id tid)
{
  grn_obj *sorted;
  if ((sorted = grn_table_create(ctx, NULL, 0, NULL, GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
    uint32_t nkeys;
    grn_obj *score_col;
    grn_table_sort_key *keys;
    score_col = grn_obj_column(ctx, res, CONST_STR_LEN("_score"));
    /* FIXME: use grn_table_sort instead */
    if ((keys = grn_table_sort_key_from_str(ctx, CONST_STR_LEN("_score"), res, &nkeys))) {
      grn_table_cursor *scur;
      /* TODO: support offset limit */
      grn_table_sort(ctx, res, 0, limit + 1, sorted, keys, nkeys);
      GRN_OUTPUT_ARRAY_OPEN("RESULTS", -1);
      if ((scur = grn_table_cursor_open(ctx, sorted, NULL, 0, NULL, 0, 0, -1, 0))) {
        grn_id id;
        while ((id = grn_table_cursor_next(ctx, scur))) {
          grn_id res_id;
          unsigned int key_len;
          char key[GRN_TABLE_MAX_KEY_SIZE];
          grn_obj score_val;

          grn_table_get_key(ctx, sorted, id, &res_id, sizeof(grn_id));
          grn_table_get_key(ctx, res, res_id, &id, sizeof(grn_id));
          if (id == tid) { continue; }
          GRN_OUTPUT_ARRAY_OPEN("RESULT", 2);
          key_len = grn_table_get_key(ctx, table, id, key, GRN_TABLE_MAX_KEY_SIZE);
          GRN_OUTPUT_STR(key, key_len);

          GRN_INT32_INIT(&score_val, 0);
          grn_obj_get_value(ctx, score_col, res_id, &score_val);
          GRN_OUTPUT_INT32(GRN_INT32_VALUE(&score_val));
          GRN_OUTPUT_ARRAY_CLOSE();
        }
        grn_table_cursor_close(ctx, scur);
      } else {
        ERR(GRN_UNKNOWN_ERROR, "cannot open sorted cursor.");
      }
      GRN_OUTPUT_ARRAY_CLOSE();
      grn_table_sort_key_close(ctx, keys, nkeys);
    } else {
      ERR(GRN_UNKNOWN_ERROR, "cannot sort.");
    }
    grn_obj_unlink(ctx, score_col);
    grn_obj_unlink(ctx, sorted);
  } else {
    ERR(GRN_UNKNOWN_ERROR, "cannot create temporary sort table.");
  }
}

static void
complete(grn_ctx *ctx, grn_obj *table, grn_obj *col, grn_obj *query)
{
  grn_obj *res;
  if ((res = grn_table_create(ctx, NULL, 0, NULL,
                              GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, NULL))) {
    grn_id tid = GRN_ID_NIL;
    if (GRN_TEXT_LEN(query)) {
      grn_str *norm;
      grn_table_cursor *cur;
      if ((norm = grn_str_open(ctx, TEXT_VALUE_LEN(query), GRN_STR_NORMALIZE))) {
        /* RK search + prefix search */
        grn_obj *index;
        /* FIXME: support index selection */
        if (grn_column_index(ctx, col, GRN_OP_PREFIX, &index, 1, NULL)) {
          if ((cur = grn_table_cursor_open(ctx, grn_ctx_at(ctx, index->header.domain),
                                           norm->norm, norm->norm_blen, NULL, 0, 0, -1,
                                           GRN_CURSOR_PREFIX|GRN_CURSOR_RK))) {
            grn_id id;
            while ((id = grn_table_cursor_next(ctx, cur))) {
              grn_ii_cursor *icur;
              if ((icur = grn_ii_cursor_open(ctx, (grn_ii *)index, id,
                                             GRN_ID_NIL, GRN_ID_MAX, 1, 0))) {
                grn_ii_posting *p;
                while ((p = grn_ii_cursor_next(ctx, icur))) {
                  grn_hash_add(ctx, (grn_hash *)res, &p->rid, sizeof(grn_id), NULL, NULL);
                  /* FIXME: execute _score = score */
                }
                grn_ii_cursor_close(ctx, icur);
              }
            }
            grn_table_cursor_close(ctx, cur);
          } else {
            ERR(GRN_UNKNOWN_ERROR, "cannot open cursor for pk.");
          }
        } else {
          ERR(GRN_UNKNOWN_ERROR, "cannot find index for prefix search.");
        }
        grn_str_close(ctx, norm);
      }
      tid = cooccur_search(ctx, table, query, res, COMPLETE);
      if (!grn_table_size(ctx, res) &&
          (cur = grn_table_cursor_open(ctx, table, TEXT_VALUE_LEN(query),
                                       NULL, 0, 0, -1, GRN_CURSOR_PREFIX))) {
        grn_id id;
        while ((id = grn_table_cursor_next(ctx, cur))) {
          grn_hash_add(ctx, (grn_hash *)res, &id, sizeof(grn_id), NULL, NULL);
        }
        grn_table_cursor_close(ctx, cur);
      }
    }
    output(ctx, table, res, 10, tid);
    grn_obj_close(ctx, res);
  } else {
    ERR(GRN_UNKNOWN_ERROR, "cannot create temporary table.");
  }
}

static void
correct(grn_ctx *ctx, grn_obj *table, grn_obj *query)
{
  grn_obj *res;
  if ((res = grn_table_create(ctx, NULL, 0, NULL,
                              GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, NULL))) {
    if (GRN_TEXT_LEN(query)) {
      grn_obj *key, *index;
      if ((key = grn_obj_column(ctx, table, CONST_STR_LEN("_key")))) {
        if (grn_column_index(ctx, key, GRN_OP_MATCH, &index, 1, NULL)) {
#if 0
          grn_select_optarg optarg;
          memset(&optarg, 0, sizeof(grn_select_optarg));
          optarg.mode = GRN_OP_SIMILAR;
          optarg.similarity_threshold = 10;
          grn_ii_select(ctx, (grn_ii *)index, TEXT_VALUE_LEN(query),
                        (grn_hash *)res, GRN_OP_OR, &optarg);
          {
            /* exec _score = edit_distance(_key, "query string") for all records */
            grn_obj *var;
            grn_obj *expr;

            GRN_EXPR_CREATE_FOR_QUERY(ctx, res, expr, var);
            if (expr) {
              grn_table_cursor *tc;
              grn_obj *score = grn_obj_column(ctx, res, CONST_STR_LEN("_score"));
              grn_obj *key = grn_obj_column(ctx, res, CONST_STR_LEN("_key"));
              grn_expr_append_obj(ctx, expr,
                                  score,
                                  GRN_OP_GET_VALUE, 1);
              grn_expr_append_obj(ctx, expr,
                                  grn_ctx_get(ctx, CONST_STR_LEN("edit_distance")),
                                  GRN_OP_PUSH, 1);
              grn_expr_append_obj(ctx, expr,
                                  key,
                                  GRN_OP_GET_VALUE, 1);
              grn_expr_append_const(ctx, expr, query, GRN_OP_PUSH, 1);
              grn_expr_append_op(ctx, expr, GRN_OP_CALL, 2);
              grn_expr_append_op(ctx, expr, GRN_OP_ASSIGN, 2);

              if ((tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0, -1, 0))) {
                while (!grn_table_cursor_next_o(ctx, tc, var)) {
                  grn_expr_exec(ctx, expr, 0);
                }
                grn_table_cursor_close(ctx, tc);
              }
              grn_obj_unlink(ctx, score);
              grn_obj_unlink(ctx, key);
              grn_obj_unlink(ctx, expr);
            } else {
              ERR(GRN_UNKNOWN_ERROR,
                  "error on building expr. for calicurating edit distance");
            }
          }
#endif
          grn_obj_unlink(ctx, index);
        }
        grn_obj_unlink(ctx, key);
      }
    }
    output(ctx, table, res, 10, cooccur_search(ctx, table, query, res, CORRECT));
    grn_obj_close(ctx, res);
  } else {
    ERR(GRN_UNKNOWN_ERROR, "cannot create temporary table.");
  }
}

static void
suggest(grn_ctx *ctx, grn_obj *table, grn_obj *query)
{
  grn_obj *res;
  if ((res = grn_table_create(ctx, NULL, 0, NULL,
                              GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, NULL))) {
    output(ctx, table, res, 10, cooccur_search(ctx, table, query, res, SUGGEST));
    grn_obj_close(ctx, res);
  } else {
    ERR(GRN_UNKNOWN_ERROR, "cannot create temporary table.");
  }
}

static grn_obj *
command_suggest(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table, *col;
  int types = grn_parse_suggest_types(GRN_TEXT_VALUE(VAR(0)), GRN_BULK_CURR(VAR(0)));
  if ((table = grn_ctx_get(ctx, TEXT_VALUE_LEN(VAR(1))))) {
    GRN_OUTPUT_MAP_OPEN("RESULT_SET", -1);
    if (types & COMPLETE) {
      if ((col = grn_obj_column(ctx, table, TEXT_VALUE_LEN(VAR(2))))) {
        GRN_OUTPUT_CSTR("complete");
        complete(ctx, table, col, VAR(3));
      } else {
        ERR(GRN_INVALID_ARGUMENT, "invalid column.");
      }
    }
    if (types & CORRECT) {
      GRN_OUTPUT_CSTR("correct");
      correct(ctx, table, VAR(3));
    }
    if (types & SUGGEST) {
      GRN_OUTPUT_CSTR("suggest");
      suggest(ctx, table, VAR(3));
    }
    GRN_OUTPUT_MAP_CLOSE();
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid table.");
  }
  return NULL;
}

static grn_obj *
func_suggest_preparer(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  int r = 0;
  grn_obj *obj;
  if (nargs == 6) {
    grn_obj v1, pre_events;
    grn_id post_event = GRN_RECORD_VALUE(args[0]);
    grn_id post_type = GRN_RECORD_VALUE(args[1]);
    grn_id post_item = GRN_RECORD_VALUE(args[2]);
    grn_id seq = GRN_RECORD_VALUE(args[3]);
    int64_t post_time = GRN_TIME_VALUE(args[4]);
    grn_obj *pairs = args[5];
    if (post_event && post_item && seq) {
      grn_obj *items = grn_ctx_at(ctx, GRN_OBJ_GET_DOMAIN(args[2]));
      grn_obj *items_freq = grn_obj_column(ctx, items, CONST_STR_LEN("freq"));
      grn_obj *items_last = grn_obj_column(ctx, items, CONST_STR_LEN("last"));
      grn_obj *seqs = grn_ctx_at(ctx, GRN_OBJ_GET_DOMAIN(args[3]));
      grn_obj *seqs_events = grn_obj_column(ctx, seqs, CONST_STR_LEN("events"));
      grn_obj *events = grn_ctx_at(ctx, grn_obj_get_range(ctx, seqs_events));
      grn_obj *events_type = grn_obj_column(ctx, events, CONST_STR_LEN("type"));
      grn_obj *events_time = grn_obj_column(ctx, events, CONST_STR_LEN("time"));
      grn_obj *events_item = grn_obj_column(ctx, events, CONST_STR_LEN("item"));
      grn_obj *pairs_pre = grn_obj_column(ctx, pairs, CONST_STR_LEN("pre"));
      grn_obj *pairs_post = grn_obj_column(ctx, pairs, CONST_STR_LEN("post"));
      grn_obj *pairs_freq0 = grn_obj_column(ctx, pairs, CONST_STR_LEN("freq0"));
      grn_obj *pairs_freq1 = grn_obj_column(ctx, pairs, CONST_STR_LEN("freq1"));
      grn_obj *pairs_freq2 = grn_obj_column(ctx, pairs, CONST_STR_LEN("freq2"));
      GRN_UINT32_INIT(&v1, 0);
      GRN_UINT32_SET(ctx, &v1, 1);
      GRN_RECORD_INIT(&pre_events, 0, grn_obj_id(ctx, events));
      grn_obj_set_value(ctx, items_freq, post_item, &v1, GRN_OBJ_INCR);
      grn_obj_set_value(ctx, items_last, post_item, args[4], GRN_OBJ_SET);
      if (post_type) {
        int added;
        grn_id pid, tid, *ep, *es;
        grn_obj pre_type, pre_time, pre_item;
        uint64_t key, key_ = ((uint64_t)post_item) << 32;
        grn_obj_get_value(ctx, seqs_events, seq, &pre_events);
        ep = (grn_id *)GRN_BULK_CURR(&pre_events);
        es = (grn_id *)GRN_BULK_HEAD(&pre_events);
        GRN_RECORD_INIT(&pre_type, 0, grn_obj_get_range(ctx, events_type));
        GRN_TIME_INIT(&pre_time, 0);
        GRN_RECORD_INIT(&pre_item, 0, grn_obj_get_range(ctx, events_item));
        while (es < ep--) {
          GRN_BULK_REWIND(&pre_type);
          GRN_BULK_REWIND(&pre_time);
          GRN_BULK_REWIND(&pre_item);
          grn_obj_get_value(ctx, events_type, *ep, &pre_type);
          grn_obj_get_value(ctx, events_time, *ep, &pre_time);
          grn_obj_get_value(ctx, events_item, *ep, &pre_item);
          if (GRN_TIME_VALUE(&pre_time) + 60000000 < post_time) {
            r = (int)((post_time - GRN_TIME_VALUE(&pre_time))/1000000);
            break;
          }
          key = key_ + GRN_RECORD_VALUE(&pre_item);
          pid = grn_table_add(ctx, pairs, &key, sizeof(uint64_t), &added);
          if (added) {
            grn_obj_set_value(ctx, pairs_pre, pid, &pre_item, GRN_OBJ_SET);
            grn_obj_set_value(ctx, pairs_post, pid, args[2], GRN_OBJ_SET);
          }
          if (GRN_RECORD_VALUE(&pre_type)) {
            grn_obj_set_value(ctx, pairs_freq1, pid, &v1, GRN_OBJ_INCR);
            break;
          } else {
            grn_obj_set_value(ctx, pairs_freq0, pid, &v1, GRN_OBJ_INCR);
          }
        }
        {
          char keybuf[GRN_TABLE_MAX_KEY_SIZE];
          int keylen = grn_table_get_key(ctx, items, post_item,
                                         keybuf, GRN_TABLE_MAX_KEY_SIZE);
          grn_token *token = grn_token_open(ctx, items, keybuf, keylen, 1);
          if (token) {
            while ((tid = grn_token_next(ctx, token)) && tid != post_item) {
              key = key_ + tid;
              pid = grn_table_add(ctx, pairs, &key, sizeof(uint64_t), &added);
              if (added) {
                GRN_RECORD_SET(ctx, &pre_item, tid);
                grn_obj_set_value(ctx, pairs_pre, pid, &pre_item, GRN_OBJ_SET);
                grn_obj_set_value(ctx, pairs_post, pid, args[2], GRN_OBJ_SET);
              }
              grn_obj_set_value(ctx, pairs_freq2, pid, &v1, GRN_OBJ_INCR);
            }
            grn_token_close(ctx, token);
          }
        }
        GRN_OBJ_FIN(ctx, &pre_type);
        GRN_OBJ_FIN(ctx, &pre_time);
        GRN_OBJ_FIN(ctx, &pre_item);
        GRN_BULK_REWIND(&pre_events);
      }
      GRN_RECORD_SET(ctx, &pre_events, post_event);
      grn_obj_set_value(ctx, seqs_events, seq, &pre_events, GRN_OBJ_APPEND);
      GRN_OBJ_FIN(ctx, &pre_events);
      GRN_OBJ_FIN(ctx, &v1);
    }
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_UINT32, 0))) { GRN_UINT32_SET(ctx, obj, r); }
  return obj;
}

grn_rc
grn_module_init_suggest(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}

grn_rc
grn_module_register_suggest(grn_ctx *ctx)
{
  /* TODO: offset/limit */
  grn_expr_var vars[] = {
    {CONST_STR_LEN("types")},
    {CONST_STR_LEN("table")},
    {CONST_STR_LEN("column")},
    {CONST_STR_LEN("query")}
  };
  GRN_TEXT_INIT(&vars[0].value, 0);
  GRN_TEXT_INIT(&vars[1].value, 0);
  GRN_TEXT_INIT(&vars[2].value, 0);
  GRN_TEXT_INIT(&vars[3].value, 0);
  grn_proc_create(ctx, CONST_STR_LEN("suggest"), GRN_PROC_COMMAND,
                  command_suggest, NULL, NULL, 4, vars);

  grn_proc_create(ctx, CONST_STR_LEN("suggest_preparer"), GRN_PROC_FUNCTION,
                  func_suggest_preparer, NULL, NULL, 0, NULL);
  return ctx->rc;
}

grn_rc
grn_module_fin_suggest(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
