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
#include "output.h"
#include <string.h>

#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define VAR GRN_PROC_GET_VAR_BY_OFFSET

static grn_obj *
command_suggest(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *table;
  if ((table = grn_ctx_get(ctx, GRN_TEXT_VALUE(VAR(0)),
                           GRN_TEXT_LEN(VAR(0))))) {
    grn_obj *col;
    if ((col = grn_obj_column(ctx, table, GRN_TEXT_VALUE(VAR(1)),
                              GRN_TEXT_LEN(VAR(1))))) {
      grn_obj *res;
      if ((res = grn_table_create(ctx, NULL, 0, NULL,
                                   GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, table, NULL))) {
        grn_obj *sorted;
        if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                       GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
#if 1
          /* RK search */
          grn_obj *index;
          /* FIXME: support index selection */
          if (grn_column_index(ctx, col, GRN_OP_PREFIX,
                               &index, 1, NULL)) {
            grn_table_cursor *cur;
            if ((cur = grn_table_cursor_open(ctx, grn_ctx_at(ctx, index->header.domain),
                                             GRN_TEXT_VALUE(VAR(2)), GRN_TEXT_LEN(VAR(2)),
                                             NULL, 0,
                                             0, -1,
                                             GRN_CURSOR_PREFIX | GRN_CURSOR_RK))) {
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
                } else {
                  ERR(GRN_UNKNOWN_ERROR, "cannot open cursor for index.");
                  goto exit;
                }
              }
              grn_table_cursor_close(ctx, cur);
            } else {
              ERR(GRN_UNKNOWN_ERROR, "cannot open cursor for pk.");
              goto exit;
            }
          } else {
            ERR(GRN_UNKNOWN_ERROR, "cannot find index for prefix search.");
            goto exit;
          }
#else
          grn_select_optarg optarg;
          memset(&optarg, 0, sizeof(grn_select_optarg));
          optarg.mode = GRN_OP_SIMILAR;
          optarg.similarity_threshold = 1048576;

          grn_ii_select(ctx, (grn_ii *)grn_ctx_get(ctx, CONST_STR_LEN("SuggestBigram.suggest_key")),
                        GRN_TEXT_VALUE(VAR(2)), GRN_TEXT_LEN(VAR(2)),
                        (grn_hash *)res, GRN_OP_OR, &optarg);
          {
            /* exec _score = edit_distance(_key, "query string") for all records */
            grn_obj *var;
            grn_obj *expr;

            GRN_EXPR_CREATE_FOR_QUERY(ctx, res, expr, var);
            if (expr) {
              grn_table_cursor *tc;

              grn_expr_append_obj(ctx, expr,
                                  grn_obj_column(ctx, res, CONST_STR_LEN("_score")),
                                  GRN_OP_GET_VALUE, 1);
              grn_expr_append_obj(ctx, expr,
                                  grn_ctx_get(ctx, CONST_STR_LEN("edit_distance")),
                                  GRN_OP_PUSH, 1);
              grn_expr_append_obj(ctx, expr,
                                  grn_obj_column(ctx, res, CONST_STR_LEN("_key")),
                                  GRN_OP_GET_VALUE, 1);
              grn_expr_append_const(ctx, expr, VAR(2), GRN_OP_PUSH, 1);
              grn_expr_append_op(ctx, expr, GRN_OP_CALL, 2);
              grn_expr_append_op(ctx, expr, GRN_OP_ASSIGN, 2);

              if ((tc = grn_table_cursor_open(ctx, res, NULL, 0, NULL, 0, 0, -1, 0))) {
                while (!grn_table_cursor_next_o(ctx, tc, var)) {
                  grn_expr_exec(ctx, expr, 0);
                }
                grn_table_cursor_close(ctx, tc);
              }
              grn_expr_close(ctx, expr);
            } else {
              ERR(GRN_UNKNOWN_ERROR, "error on building expr. for calicurating edit distance");
              goto exit;
            }
          }
#endif
          /* sort */
          {
            uint32_t nkeys;
            grn_obj *score_col;
            grn_table_sort_key *keys;
            score_col = grn_obj_column(ctx, res, CONST_STR_LEN("_score"));
            /* FIXME: use grn_table_sort instead */
            if ((keys = grn_table_sort_key_from_str(ctx, CONST_STR_LEN("-_score"), res, &nkeys))) {
              grn_table_cursor *scur;
              /* TODO: support offset limit */
              grn_table_sort(ctx, res, 0, grn_table_size(ctx, res), sorted, keys, nkeys);
              GRN_OUTPUT_ARRAY_OPEN("RESULTS", -1);
              if ((scur = grn_table_cursor_open(ctx, sorted, NULL, 0, NULL, 0, 0, -1, 0))) {
                grn_id id;
                while ((id = grn_table_cursor_next(ctx, scur))) {
                  grn_id res_id;
                  unsigned int key_len;
                  char key[GRN_TABLE_MAX_KEY_SIZE];
                  grn_obj score_val;

                  GRN_OUTPUT_ARRAY_OPEN("RESULT", 2);
                  grn_table_get_key(ctx, sorted, id, &res_id, sizeof(grn_id));
                  grn_table_get_key(ctx, res, res_id, &id, sizeof(grn_id));
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
          }
exit:
          grn_obj_close(ctx, sorted);
        } else {
          ERR(GRN_UNKNOWN_ERROR, "cannot create temporary sort table.");
        }
        grn_obj_close(ctx, res);
      } else {
        ERR(GRN_UNKNOWN_ERROR, "cannot create temporary table.");
      }
    } else {
      ERR(GRN_INVALID_ARGUMENT, "invalid column.");
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "invalid table.");
  }
  return NULL;
}

static grn_obj *
func_suggest_preparer(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *obj;
  if (nargs == 5) {
    grn_obj buf, *item = args[2], *seq = args[3];
    grn_id id = GRN_UINT32_VALUE(args[0]);
    grn_id type = GRN_UINT32_VALUE(args[1]);
    int64_t time = GRN_TIME_VALUE(args[4]);
    grn_obj *items = grn_ctx_at(ctx, GRN_OBJ_GET_DOMAIN(item));
    grn_obj *freq = grn_obj_column(ctx, items, CONST_STR_LEN("freq"));
    grn_obj *seqs = grn_ctx_at(ctx, GRN_OBJ_GET_DOMAIN(seq));
    grn_obj *events = grn_obj_column(ctx, seqs, CONST_STR_LEN("events"));
    GRN_UINT32_INIT(&buf, 0);
    GRN_UINT32_SET(ctx, &buf, 1);
    grn_obj_set_value(ctx, freq, GRN_RECORD_VALUE(item), &buf, GRN_OBJ_INCR);
    GRN_OBJ_FIN(ctx, &buf);
    GRN_RECORD_INIT(&buf, 0, grn_obj_get_range(ctx, events));
    GRN_RECORD_SET(ctx, &buf, id);
    grn_obj_set_value(ctx, events, GRN_RECORD_VALUE(seq), &buf, GRN_OBJ_APPEND);
    GRN_OBJ_FIN(ctx, &buf);
  }
  if ((obj = GRN_PROC_ALLOC(GRN_DB_UINT32, 0))) {
    GRN_UINT32_SET(ctx, obj, 0);
  }
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
    {CONST_STR_LEN("table")},
    {CONST_STR_LEN("column")},
    {CONST_STR_LEN("query")}
  };
  GRN_TEXT_INIT(&vars[0].value, 0);
  GRN_TEXT_INIT(&vars[1].value, 0);
  GRN_TEXT_INIT(&vars[2].value, 0);
  grn_proc_create(ctx, CONST_STR_LEN("suggest"), GRN_PROC_COMMAND,
                  command_suggest, NULL, NULL, 3, vars);

  grn_proc_create(ctx, CONST_STR_LEN("suggest_preparer"), GRN_PROC_FUNCTION,
                  func_suggest_preparer, NULL, NULL, 0, NULL);
  return ctx->rc;
}

grn_rc
grn_module_fin_suggest(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
