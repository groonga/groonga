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
func_suggest(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
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
                }
                grn_ii_cursor_close(ctx, icur);
              } else {
                ERR(GRN_UNKNOWN_ERROR, "cannot open cursor for index.");
                break;
              }
            }
            /* sort */
            {
              grn_obj *sorted;
              if ((sorted = grn_table_create(ctx, NULL, 0, NULL,
                                             GRN_OBJ_TABLE_NO_KEY, NULL, res))) {
                uint32_t nkeys;
                grn_table_sort_key *keys;
                if ((keys = grn_table_sort_key_from_str(ctx, CONST_STR_LEN("-score"), res, &nkeys))) {
                  grn_table_cursor *scur;
                  /* TODO: support offset limit */
                  grn_table_sort(ctx, res, 0, grn_table_size(ctx, res), sorted, keys, nkeys);
                  GRN_OUTPUT_ARRAY_OPEN("RESULT", -1);
                  if ((scur = grn_table_cursor_open(ctx, sorted, NULL, 0, NULL, 0, 0, -1, 0))) {
                    grn_id sid;
                    while ((sid = grn_table_cursor_next(ctx, scur))) {
                      grn_id stid;
                      unsigned int key_len;
                      char key[GRN_TABLE_MAX_KEY_SIZE];
                      grn_table_get_key(ctx, sorted, sid, &stid, sizeof(grn_id));
                      grn_table_get_key(ctx, res, stid, &stid, sizeof(grn_id));
                      key_len = grn_table_get_key(ctx, table, stid, key, GRN_TABLE_MAX_KEY_SIZE);
                      GRN_OUTPUT_STR(key, key_len);
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
                grn_obj_close(ctx, sorted);
              } else {
                ERR(GRN_UNKNOWN_ERROR, "cannot create temporary sort table.");
              }
            }
            grn_table_cursor_close(ctx, cur);
          } else {
            ERR(GRN_UNKNOWN_ERROR, "cannot open cursor for pk.");
          }
        } else {
          ERR(GRN_UNKNOWN_ERROR, "cannot find index for prefix search.");
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

  grn_proc_create(ctx, CONST_STR_LEN("suggest"), GRN_PROC_FUNCTION, func_suggest, NULL, NULL, 3, vars);

  return ctx->rc;
}

grn_rc
grn_module_fin_suggest(grn_ctx *ctx)
{
  return GRN_SUCCESS;
}
