/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_db.h"
#include "grn_ii.h"
#include "grn_posting.h"
#include "grn_selector.h"

struct _grn_selector_data {
  grn_obj *selector;
  grn_obj *expr;
  grn_obj *table;
  grn_obj *index;
  size_t n_args;
  grn_obj **args;
  grn_obj *result_set;
  grn_operator op;
  grn_obj *score_table;
  grn_obj *score_column;
  grn_obj score;
};

grn_selector_data *
grn_selector_data_get(grn_ctx *ctx)
{
  return ctx->impl->current_selector_data;
}

grn_rc
grn_selector_run(grn_ctx *ctx,
                 grn_obj *selector,
                 grn_obj *expr,
                 grn_obj *table,
                 grn_obj *index,
                 size_t n_args,
                 grn_obj **args,
                 grn_obj *result_set,
                 grn_operator op)
{
  grn_selector_data data;
  data.selector = selector;
  data.expr = expr;
  data.table = table;
  data.index = index;
  data.n_args = n_args;
  data.args = args;
  data.result_set = result_set;
  data.op = op;
  data.score_table = NULL;
  data.score_column = NULL;
  GRN_FLOAT_INIT(&(data.score), 0);

  grn_selector_data *previous_data = ctx->impl->current_selector_data;
  ctx->impl->current_selector_data = &data;
  grn_proc *proc = (grn_proc *)selector;
  grn_rc rc = proc->callbacks.function.selector(ctx,
                                                table,
                                                index,
                                                n_args,
                                                args,
                                                result_set,
                                                op);
  ctx->impl->current_selector_data = previous_data;

  GRN_OBJ_FIN(ctx, &(data.score));
  if (data.score_table) {
    if (data.score_table != data.result_set) {
      grn_obj_unref(ctx, data.score_table);
    }
    data.score_table = NULL;
  }

  return rc;
}

grn_obj *
grn_selector_data_get_selector(grn_ctx *ctx,
                               grn_selector_data *data)
{
  return data->selector;
}

grn_obj *
grn_selector_data_get_expr(grn_ctx *ctx,
                           grn_selector_data *data)
{
  return data->expr;
}

grn_obj *
grn_selector_data_get_table(grn_ctx *ctx,
                            grn_selector_data *data)
{
  return data->table;
}

grn_obj *
grn_selector_data_get_index(grn_ctx *ctx,
                            grn_selector_data *data)
{
  return data->index;
}

grn_obj **
grn_selector_data_get_args(grn_ctx *ctx,
                           grn_selector_data *data,
                           size_t *n_args)
{
  if (n_args) {
    *n_args = data->n_args;
  }
  return data->args;
}

grn_obj *
grn_selector_data_get_result_set(grn_ctx *ctx,
                                 grn_selector_data *data)
{
  return data->result_set;
}

grn_operator
grn_selector_data_get_op(grn_ctx *ctx,
                         grn_selector_data *data)
{
  return data->op;
}

grn_rc
grn_selector_data_parse_score_column_option_value(grn_ctx *ctx,
                                                  const char *name,
                                                  grn_obj *value,
                                                  const char *tag,
                                                  void *user_data)
{
  grn_selector_data *data = user_data;
  if (value->header.type == GRN_PTR) {
    value = GRN_PTR_VALUE(value);
  }
  switch (value->header.type) {
  case GRN_COLUMN_FIX_SIZE :
    {
      if (value->header.domain == data->result_set->header.domain) {
        data->score_column = value;
      } else {
        data->score_table = grn_ctx_at(ctx, value->header.domain);
        if (data->score_table->header.domain == DB_OBJ(data->table)->id) {
          data->score_column = value;
        } else {
          grn_obj inspected;
          GRN_TEXT_INIT(&inspected, 0);
          grn_inspect(ctx, &inspected, value);
          ERR(GRN_INVALID_ARGUMENT,
              "%s[%s] unrelated score column: %.*s",
              tag,
              name,
              (int)GRN_TEXT_LEN(&inspected),
              GRN_TEXT_VALUE(&inspected));
          GRN_OBJ_FIN(ctx, &inspected);
        }
      }
    }
    break;
  default :
    if (grn_obj_is_text_family_bulk(ctx, value)) {
      data->score_column = grn_obj_column(ctx,
                                          data->result_set,
                                          GRN_TEXT_VALUE(value),
                                          GRN_TEXT_LEN(value));
      data->score_table = data->result_set;
    } else {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%s] must be a fixed size column or "
          "column name in result set: %.*s",
          tag,
          name,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
    break;
  }
  return ctx->rc;
}

bool
grn_selector_data_have_score_column(grn_ctx *ctx,
                                    grn_selector_data *data)
{
  return data->score_column != NULL;
}

grn_rc
grn_selector_data_current_add_score_no_validation(grn_ctx *ctx,
                                                  grn_obj *result_set,
                                                  grn_id result_set_record_id,
                                                  grn_id record_id,
                                                  double score)
{
  grn_selector_data *data = ctx->impl->current_selector_data;

  if (data->result_set != result_set) {
    return ctx->rc;
  }

  if (!data->score_column) {
    return ctx->rc;
  }

  grn_id score_id;
  if (data->score_table) {
    if (data->score_table == data->result_set) {
      score_id = result_set_record_id;
    } else {
      score_id = grn_table_get(ctx,
                               data->score_table,
                               &record_id,
                               sizeof(grn_id));
      if (score_id == GRN_ID_NIL) {
        return ctx->rc;
      }
    }
  } else {
    score_id = record_id;
  }

  GRN_FLOAT_SET(ctx, &(data->score), score);
  grn_obj_set_value(ctx,
                    data->score_column,
                    score_id,
                    &(data->score),
                    GRN_OBJ_INCR);
  return ctx->rc;
}

grn_rc
grn_selector_data_on_token_found(grn_ctx *ctx,
                                 grn_selector_data *data,
                                 grn_obj *index,
                                 grn_id token_id,
                                 double additional_score)
{
  GRN_API_ENTER;
  grn_ii *ii = (grn_ii *)index;
  grn_ii_cursor *cursor = grn_ii_cursor_open(ctx,
                                             ii,
                                             token_id,
                                             GRN_ID_NIL,
                                             GRN_ID_MAX,
                                             ii->n_elements - 1,
                                             0);
  if (!cursor) {
    GRN_API_RETURN(ctx->rc);
  }

  grn_result_set_add_ii_cursor(ctx,
                               (grn_hash *)(data->result_set),
                               cursor,
                               additional_score,
                               1,
                               data->op);
  grn_ii_cursor_close(ctx, cursor);

  GRN_API_RETURN(ctx->rc);
}
