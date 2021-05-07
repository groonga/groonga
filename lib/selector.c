/* -*- c-basic-offset: 2 -*- */
/*
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
  grn_obj *tags_table;
  grn_obj *tags_column;
  grn_obj tags;
  grn_obj default_tags;
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
  data.tags_table = NULL;
  data.tags_column = NULL;
  GRN_TEXT_INIT(&(data.tags), GRN_OBJ_VECTOR);
  GRN_TEXT_INIT(&(data.default_tags), GRN_OBJ_VECTOR);

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
  GRN_OBJ_FIN(ctx, &(data.default_tags));
  GRN_OBJ_FIN(ctx, &(data.tags));
  if (data.tags_table) {
    if (data.tags_table != data.result_set) {
      grn_obj_unref(ctx, data.tags_table);
    }
    data.tags_table = NULL;
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

static grn_rc
grn_selector_data_set_tags_internal(grn_ctx *ctx,
                                    grn_selector_data *data,
                                    grn_obj *tags,
                                    grn_obj *new_tags,
                                    const char *tag,
                                    const char *name)
{
  if (grn_obj_is_text_family_bulk(ctx, new_tags)) {
    GRN_BULK_REWIND(tags);
    if (GRN_TEXT_LEN(new_tags) > 0) {
      grn_vector_add_element(ctx,
                             tags,
                             GRN_TEXT_VALUE(new_tags),
                             GRN_TEXT_LEN(new_tags),
                             0,
                             GRN_DB_TEXT);
    }
  } else if (grn_obj_is_vector(ctx, new_tags)) {
    GRN_BULK_REWIND(tags);
    uint32_t n = grn_vector_size(ctx, new_tags);
    uint32_t i;
    for (i = 0; i < n; i++) {
      const char *element;
      grn_id domain;
      uint32_t element_size = grn_vector_get_element(ctx,
                                                     new_tags,
                                                     i,
                                                     &element,
                                                     NULL,
                                                     &domain);
      if (grn_type_id_is_text_family(ctx, domain) && element_size > 0) {
        grn_vector_add_element(ctx,
                               tags,
                               element,
                               element_size,
                               0,
                               domain);
      }
    }
  } else {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, new_tags);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%s] must be "
        "a ShortText bulk or a ShortText vector: %.*s",
        tag,
        name,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
  }
  return ctx->rc;
}

grn_rc
grn_selector_data_parse_tags_option_value(grn_ctx *ctx,
                                          const char *name,
                                          grn_obj *value,
                                          const char *tag,
                                          void *user_data)
{
  grn_selector_data *data = user_data;
  return grn_selector_data_set_tags_internal(ctx,
                                             data,
                                             &(data->tags),
                                             value,
                                             tag,
                                             name);
}

grn_rc
grn_selector_data_parse_tags_column_option_value(grn_ctx *ctx,
                                                 const char *name,
                                                 grn_obj *value,
                                                 const char *tag,
                                                 void *user_data)
{
  grn_selector_data *data = user_data;
  if (value->header.type == GRN_PTR) {
    value = GRN_PTR_VALUE(value);
  }
  if (grn_obj_is_vector_column(ctx, value) &&
      (grn_type_id_is_text_family(ctx, DB_OBJ(value)->range) ||
       grn_id_maybe_table(ctx, DB_OBJ(value)->range))) {
    if (value->header.domain == data->result_set->header.domain) {
      data->tags_column = value;
    } else {
      data->tags_table = grn_ctx_at(ctx, value->header.domain);
      if (data->tags_table->header.domain == DB_OBJ(data->table)->id) {
        data->tags_column = value;
      } else {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, value);
        ERR(GRN_INVALID_ARGUMENT,
            "%s[%s] unrelated tags column: %.*s",
            tag,
            name,
            (int)GRN_TEXT_LEN(&inspected),
            GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        }
    }
  } else {
    if (grn_obj_is_text_family_bulk(ctx, value)) {
      data->tags_column = grn_obj_column(ctx,
                                         data->result_set,
                                         GRN_TEXT_VALUE(value),
                                         GRN_TEXT_LEN(value));
      data->tags_table = data->result_set;
    } else {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, value);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%s] must be "
          "a ShortText vector column, "
          "a reference vector column or "
          "a column name in result set: %.*s",
          tag,
          name,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
  }
  return ctx->rc;
}

bool
grn_selector_data_have_tags_column(grn_ctx *ctx,
                                   grn_selector_data *data)
{
  return data->tags_column != NULL;
}

grn_rc
grn_selector_data_set_default_tags(grn_ctx *ctx,
                                   grn_selector_data *data,
                                   grn_obj *tags)
{
  return grn_selector_data_set_tags_internal(ctx,
                                             data,
                                             &(data->default_tags),
                                             tags,
                                             "[selector-data]",
                                             "default-tags");
}

grn_rc
grn_selector_data_current_set_default_tag_raw_no_validation(grn_ctx *ctx,
                                                            const char *tag,
                                                            uint32_t tag_length)
{
  grn_selector_data *data = ctx->impl->current_selector_data;

  if (!data->tags_column) {
    return ctx->rc;
  }

  if (tag_length == 0) {
    return ctx->rc;
  }

  grn_obj tag_bulk;
  GRN_TEXT_INIT(&tag_bulk, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_SET(ctx, &tag_bulk, tag, tag_length);
  grn_rc rc = grn_selector_data_set_default_tags(ctx, data, &tag_bulk);
  GRN_OBJ_FIN(ctx, &tag_bulk);
  return rc;
}

static grn_inline void
grn_selector_data_add_score(grn_ctx *ctx,
                            grn_selector_data *data,
                            grn_obj *result_set,
                            grn_id result_set_record_id,
                            grn_id record_id,
                            double score)
{
  if (!data->score_column) {
    return;
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
        return;
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
}

static grn_inline void
grn_selector_data_append_tags(grn_ctx *ctx,
                              grn_selector_data *data,
                              grn_obj *result_set,
                              grn_id result_set_record_id,
                              grn_id record_id)
{
  if (!data->tags_column) {
    return;
  }

  grn_obj *tags = NULL;
  if (grn_vector_size(ctx, &(data->tags)) > 0) {
    tags = &(data->tags);
  } else if (grn_vector_size(ctx, &(data->default_tags)) > 0) {
    tags = &(data->default_tags);
  }
  if (!tags) {
    return;
  }

  grn_id tags_id;
  if (data->tags_table) {
    if (data->tags_table == data->result_set) {
      tags_id = result_set_record_id;
    } else {
      tags_id = grn_table_get(ctx,
                              data->tags_table,
                              &record_id,
                              sizeof(grn_id));
      if (tags_id == GRN_ID_NIL) {
        return;
      }
    }
  } else {
    tags_id = record_id;
  }

  grn_obj_set_value(ctx,
                    data->tags_column,
                    tags_id,
                    tags,
                    GRN_OBJ_APPEND);
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

  grn_selector_data_add_score(ctx,
                              data,
                              result_set,
                              result_set_record_id,
                              record_id,
                              score);
  if (ctx->rc == GRN_SUCCESS) {
    grn_selector_data_append_tags(ctx,
                                  data,
                                  result_set,
                                  result_set_record_id,
                                  record_id);
  }
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
