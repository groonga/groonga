/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil
  Copyright(C) 2018-2020 Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"

#include "grn_db.h"
#include "grn_expr.h"
#include "grn_group.h"
#include "grn_table.h"
#include "grn_vector.h"

#ifdef GRN_WITH_XXHASH
# include <xxhash.h>
#endif

#define GRN_TABLE_GROUP_BY_KEY           0
#define GRN_TABLE_GROUP_BY_VALUE         1
#define GRN_TABLE_GROUP_BY_COLUMN_VALUE  2

#define GRN_TABLE_GROUP_FILTER_PREFIX    0
#define GRN_TABLE_GROUP_FILTER_SUFFIX    (1L<<2)

static bool grn_table_group_fallback_hash_value_enable = false;
static bool grn_table_group_all_hash_value_enable = false;

void
grn_group_init_from_env(void)
{
#ifdef GRN_WITH_XXHASH
  {
    char grn_table_group_fallback_hash_value_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_GROUP_FALLBACK_HASH_VALUE_ENABLE",
               grn_table_group_fallback_hash_value_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_table_group_fallback_hash_value_enable_env, "no") == 0) {
      grn_table_group_fallback_hash_value_enable = false;
    } else {
      grn_table_group_fallback_hash_value_enable = true;
    }
  }

  {
    char grn_table_group_all_hash_value_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_GROUP_ALL_HASH_VALUE_ENABLE",
               grn_table_group_all_hash_value_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_table_group_all_hash_value_enable_env, "yes") == 0) {
      grn_table_group_all_hash_value_enable = true;
    } else {
      grn_table_group_all_hash_value_enable = false;
    }
  }
#endif
}

grn_rc
grn_table_group_aggregator_init(grn_ctx *ctx,
                                grn_table_group_aggregator *aggregator)
{
  aggregator->output_column_name = NULL;
  aggregator->output_column_name_len = 0;
  aggregator->output_column_type = NULL;
  aggregator->output_column_flags = 0;
  aggregator->expression = NULL;
  aggregator->expression_len = 0;
  aggregator->aggregator_call = NULL;
  aggregator->aggregator_call_record = NULL;
  aggregator->data.group_id = GRN_ID_NIL;
  aggregator->data.source_id = GRN_ID_NIL;
  aggregator->data.group_table = NULL;
  aggregator->data.source_table = NULL;
  aggregator->data.output_column = NULL;
  aggregator->data.aggregator = NULL;
  GRN_PTR_INIT(&(aggregator->data.args), GRN_OBJ_VECTOR, GRN_ID_NIL);
  aggregator->data.user_data = NULL;
  return ctx->rc;
}

grn_table_group_aggregator *
grn_table_group_aggregator_open(grn_ctx *ctx)
{
  GRN_API_ENTER;
  grn_table_group_aggregator *aggregator = GRN_MALLOCN(grn_table_group_aggregator, 1);
  if (!aggregator) {
    ERR(ctx->rc,
        "[table-group-aggregator][open] failed to allocate: %s",
        ctx->errbuf);
    GRN_API_RETURN(NULL);
  }
  grn_table_group_aggregator_init(ctx, aggregator);
  GRN_API_RETURN(aggregator);
}

grn_rc
grn_table_group_aggregator_fin(grn_ctx *ctx,
                               grn_table_group_aggregator *aggregator)
{
  GRN_OBJ_FIN(ctx, &(aggregator->data.args));
  if (aggregator->aggregator_call) {
    grn_obj_close(ctx, aggregator->aggregator_call);
  }
  if (aggregator->data.output_column) {
    grn_obj_unref(ctx, aggregator->data.output_column);
  }
  if (aggregator->data.group_table) {
    grn_obj_unref(ctx, aggregator->data.group_table);
  }
  if (aggregator->data.source_table) {
    grn_obj_unref(ctx, aggregator->data.source_table);
  }
  if (aggregator->output_column_name) {
    GRN_FREE(aggregator->output_column_name);
  }
  if (aggregator->output_column_type) {
    grn_obj_unref(ctx, aggregator->output_column_type);
  }
  if (aggregator->expression) {
    GRN_FREE(aggregator->expression);
  }
  return ctx->rc;
}

grn_rc
grn_table_group_aggregator_close(grn_ctx *ctx,
                                 grn_table_group_aggregator *aggregator)
{
  GRN_API_ENTER;
  grn_table_group_aggregator_fin(ctx, aggregator);
  GRN_FREE(aggregator);
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_table_group_aggregator_set_output_column_name(grn_ctx *ctx,
                                                  grn_table_group_aggregator *aggregator,
                                                  const char *name,
                                                  int32_t name_len)
{
  GRN_API_ENTER;
  if (aggregator->output_column_name) {
    GRN_FREE(aggregator->output_column_name);
  }
  if (name_len < 0) {
    if (name) {
      name_len = strlen(name);
    } else {
      name_len = 0;
    }
  }
  if (name_len == 0) {
    aggregator->output_column_name = NULL;
    aggregator->output_column_name_len = 0;
  } else {
    aggregator->output_column_name = GRN_MALLOCN(char, name_len);
    if (!aggregator->output_column_name) {
      aggregator->output_column_name_len = 0;
      ERR(ctx->rc,
          "[table-group-aggregator][output-column-name][set] failed to allocate: %s",
          ctx->errbuf);
      GRN_API_RETURN(ctx->rc);
    }
    grn_memcpy(aggregator->output_column_name, name, name_len);
    aggregator->output_column_name_len = name_len;
  }
  GRN_API_RETURN(ctx->rc);
}

const char *
grn_table_group_aggregator_get_output_column_name(grn_ctx *ctx,
                                                  grn_table_group_aggregator *aggregator,
                                                  uint32_t *len)
{
  GRN_API_ENTER;
  *len = aggregator->output_column_name_len;
  GRN_API_RETURN(aggregator->output_column_name);
}

grn_rc
grn_table_group_aggregator_set_output_column_type(grn_ctx *ctx,
                                                  grn_table_group_aggregator *aggregator,
                                                  grn_obj *type)
{
  GRN_API_ENTER;
  if (aggregator->output_column_type) {
    grn_obj_unref(ctx, aggregator->output_column_type);
  }
  aggregator->output_column_type = type;
  if (aggregator->output_column_type) {
    grn_obj_refer(ctx, aggregator->output_column_type);
  }
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_table_group_aggregator_get_output_column_type(grn_ctx *ctx,
                                                  grn_table_group_aggregator *aggregator)
{
  GRN_API_ENTER;
  GRN_API_RETURN(aggregator->output_column_type);
}

grn_rc
grn_table_group_aggregator_set_output_column_flags(grn_ctx *ctx,
                                                   grn_table_group_aggregator *aggregator,
                                                   grn_column_flags flags)
{
  GRN_API_ENTER;
  aggregator->output_column_flags = flags;
  GRN_API_RETURN(ctx->rc);
}

grn_column_flags
grn_table_group_aggregator_get_output_column_flags(grn_ctx *ctx,
                                                   grn_table_group_aggregator *aggregator)
{
  GRN_API_ENTER;
  GRN_API_RETURN(aggregator->output_column_flags);
}

grn_rc
grn_table_group_aggregator_set_expression(grn_ctx *ctx,
                                          grn_table_group_aggregator *aggregator,
                                          const char *expression,
                                          int32_t expression_len)
{
  GRN_API_ENTER;
  if (aggregator->expression) {
    GRN_FREE(aggregator->expression);
  }
  if (expression_len < 0) {
    if (expression) {
      expression_len = strlen(expression);
    } else {
      expression_len = 0;
    }
  }
  if (expression_len == 0) {
    aggregator->expression = NULL;
    aggregator->expression_len = 0;
  } else {
    aggregator->expression = GRN_MALLOCN(char, expression_len);
    if (!aggregator->expression) {
      aggregator->expression_len = 0;
      ERR(ctx->rc,
          "[table-group-aggregator][expression][set] failed to allocate: %s",
          ctx->errbuf);
      GRN_API_RETURN(ctx->rc);
    }
    grn_memcpy(aggregator->expression, expression, expression_len);
    aggregator->expression_len = expression_len;
  }
  GRN_API_RETURN(ctx->rc);
}

const char *
grn_table_group_aggregator_get_expression(grn_ctx *ctx,
                                          grn_table_group_aggregator *aggregator,
                                          uint32_t *expression_len)
{
  GRN_API_ENTER;
  *expression_len = aggregator->expression_len;
  GRN_API_RETURN(aggregator->expression);
}

grn_id
grn_aggregator_data_get_group_id(grn_ctx *ctx,
                                 grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(data->group_id);
}

grn_id
grn_aggregator_data_get_source_id(grn_ctx *ctx,
                                  grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(data->source_id);
}

grn_obj *
grn_aggregator_data_get_group_table(grn_ctx *ctx,
                                    grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(data->group_table);
}

grn_obj *
grn_aggregator_data_get_source_table(grn_ctx *ctx,
                                     grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(data->source_table);
}

grn_obj *
grn_aggregator_data_get_output_column(grn_ctx *ctx,
                                      grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(data->output_column);
}

grn_obj *
grn_aggregator_data_get_aggregator(grn_ctx *ctx,
                                   grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(data->aggregator);
}

grn_obj *
grn_aggregator_data_get_args(grn_ctx *ctx,
                             grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(&(data->args));
}

void *
grn_aggregator_data_get_user_data(grn_ctx *ctx,
                                  grn_aggregator_data *data)
{
  GRN_API_ENTER;
  GRN_API_RETURN(data->user_data);
}

static void
grn_table_group_aggregator_prepare(grn_ctx *ctx,
                                   grn_table_group_aggregator *aggregator,
                                   grn_obj *group_table)
{
  if (aggregator->data.group_table) {
    grn_obj_unref(ctx, aggregator->data.group_table);
  }
  aggregator->data.group_table = group_table;
  grn_obj_refer(ctx, aggregator->data.group_table);

  if (aggregator->data.source_table) {
    grn_obj_unref(ctx, aggregator->data.source_table);
  }
  aggregator->data.source_table =
    grn_ctx_at(ctx, grn_obj_get_range(ctx, aggregator->data.group_table));

  if (aggregator->output_column_name_len == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table-group-aggregator][prepare] output column name is missing");
    return;
  }

  if (aggregator->data.output_column) {
    grn_obj_unref(ctx, aggregator->data.output_column);
  }
  aggregator->data.output_column =
    grn_obj_column(ctx,
                   aggregator->data.group_table,
                   aggregator->output_column_name,
                   aggregator->output_column_name_len);
  if (grn_obj_is_accessor(ctx, aggregator->data.output_column)) {
    grn_obj_unlink(ctx, aggregator->data.output_column);
    aggregator->data.output_column = NULL;
  }

  if (!aggregator->data.output_column) {
    aggregator->data.output_column =
      grn_column_create(ctx,
                        aggregator->data.group_table,
                        aggregator->output_column_name,
                        aggregator->output_column_name_len,
                        NULL,
                        aggregator->output_column_flags,
                        aggregator->output_column_type);
    if (!aggregator->data.output_column) {
      ERR(GRN_INVALID_ARGUMENT,
          "[table-group-aggregator][prepare][%.*s] failed to create output column: %s",
          (int)(aggregator->output_column_name_len),
          aggregator->output_column_name,
          ctx->errbuf);
      return;
    }
  }

  if (aggregator->expression_len == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table-group-aggregator][prepare][%.*s] expression is missing",
        (int)(aggregator->output_column_name_len),
        aggregator->output_column_name);
    return;
  }

  if (aggregator->aggregator_call) {
    grn_obj_close(ctx, aggregator->aggregator_call);
  }
  GRN_EXPR_CREATE_FOR_QUERY(ctx,
                            aggregator->data.source_table,
                            aggregator->aggregator_call,
                            aggregator->aggregator_call_record);
  if (!aggregator->aggregator_call) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table-group-aggregator][prepare][%.*s] "
        "failed to create expression to aggregate: %s",
        (int)(aggregator->output_column_name_len),
        aggregator->output_column_name,
        ctx->errbuf);
    return;
  }

  grn_expr_parse(ctx,
                 aggregator->aggregator_call,
                 aggregator->expression,
                 aggregator->expression_len,
                 NULL,
                 GRN_OP_MATCH,
                 GRN_OP_AND,
                 GRN_EXPR_SYNTAX_SCRIPT);
  if (ctx->rc != GRN_SUCCESS) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table-group-aggregator][prepare][%.*s] "
        "failed to parse expression: <%.*s>: %s",
        (int)(aggregator->output_column_name_len),
        aggregator->output_column_name,
        (int)(aggregator->expression_len),
        aggregator->expression,
        ctx->errbuf);
    return;
  }

  grn_expr *expr = (grn_expr *)(aggregator->aggregator_call);
  if (expr->codes_curr == 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table-group-aggregator][prepare][%.*s] "
        "empty expression: <%.*s>",
        (int)(aggregator->output_column_name_len),
        aggregator->output_column_name,
        (int)(aggregator->expression_len),
        aggregator->expression);
    return;
  }
  aggregator->data.aggregator = expr->codes[0].value;

  if (!grn_obj_is_aggregator_proc(ctx, aggregator->data.aggregator)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, aggregator->data.aggregator);
    ERR(GRN_INVALID_ARGUMENT,
        "[table-group-aggregator][prepare][%.*s] "
        "must be an aggregator: <%.*s>: <%.*s>",
        (int)(aggregator->output_column_name_len),
        aggregator->output_column_name,
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected),
        (int)(aggregator->expression_len),
        aggregator->expression);
    GRN_OBJ_FIN(ctx, &inspected);
    return;
  }

  int32_t n = expr->codes_curr - 1;
  for (int32_t i = 1; i < n; i++) {
    /* TODO: Check op. */
    GRN_PTR_PUT(ctx, &(aggregator->data.args), expr->codes[i].value);
  }

  grn_aggregator_init_func *init =
    ((grn_proc *)aggregator->data.aggregator)->callbacks.aggregator.init;
  if (init) {
    aggregator->data.user_data = init(ctx, &(aggregator->data));
  } else {
    aggregator->data.user_data = NULL;
  }
}

static void
grn_table_group_aggregator_update(grn_ctx *ctx,
                                  grn_table_group_aggregator *aggregator,
                                  grn_id group_id,
                                  grn_id source_id)
{
  grn_aggregator_update_func *update =
    ((grn_proc *)aggregator->data.aggregator)->callbacks.aggregator.update;
  if (!update) {
    return;
  }

  aggregator->data.group_id = group_id;
  aggregator->data.source_id = source_id;
  update(ctx, &(aggregator->data));
}

static void
grn_table_group_aggregator_postpare(grn_ctx *ctx,
                                    grn_table_group_aggregator *aggregator)
{
  grn_aggregator_fin_func *fin =
    ((grn_proc *)aggregator->data.aggregator)->callbacks.aggregator.fin;
  if (!fin) {
    return;
  }

  aggregator->data.group_id = GRN_ID_NIL;
  aggregator->data.source_id = GRN_ID_NIL;
  fin(ctx, &(aggregator->data));
}

grn_obj *
grn_aggregator_create(grn_ctx *ctx,
                      const char *name,
                      int name_size,
                      grn_aggregator_init_func *init,
                      grn_aggregator_update_func *update,
                      grn_aggregator_fin_func *fin)
{
  GRN_API_ENTER;

  if (name_size == -1) {
    name_size = strlen(name);
  }

  grn_obj *aggregator = grn_proc_create(ctx,
                                        name,
                                        name_size,
                                        GRN_PROC_AGGREGATOR,
                                        NULL, NULL, NULL, 0, NULL);
  if (!aggregator) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(GRN_WINDOW_FUNCTION_ERROR,
        "[aggregator][%.*s] failed to create proc: %s",
        name_size, name,
        errbuf);
    GRN_API_RETURN(NULL);
  }

  {
    grn_proc *proc = (grn_proc *)aggregator;
    proc->callbacks.aggregator.init = init;
    proc->callbacks.aggregator.update = update;
    proc->callbacks.aggregator.fin = fin;
  }

  GRN_API_RETURN(aggregator);
}

grn_inline static void
grn_table_group_add_subrec(grn_ctx *ctx,
                           grn_obj *table,
                           grn_id group_id,
                           grn_rset_recinfo *ri, double score,
                           grn_rset_posinfo *pi, int dir,
                           grn_obj *value_buffer)
{
  grn_table_add_subrec(ctx, table, ri, score, pi, dir);

  const grn_table_flags table_flags = DB_OBJ(table)->header.flags;
  const grn_table_group_flags group_flags = DB_OBJ(table)->group.flags;
  const bool need_recinfo_update = ((table_flags & GRN_OBJ_WITH_SUBREC) &&
                                    (group_flags & (GRN_TABLE_GROUP_CALC_MAX |
                                                    GRN_TABLE_GROUP_CALC_MIN |
                                                    GRN_TABLE_GROUP_CALC_SUM |
                                                    GRN_TABLE_GROUP_CALC_MEAN)));
  const bool need_aggregator_update = (group_flags & (GRN_TABLE_GROUP_CALC_AGGREGATOR));

  if (need_recinfo_update) {
    GRN_BULK_REWIND(value_buffer);
    grn_obj *calc_target = DB_OBJ(table)->group.calc_target;
    grn_obj_get_value(ctx, calc_target, pi->rid, value_buffer);
    grn_rset_recinfo_update_calc_values(ctx, ri, table, value_buffer);
    if (ctx->rc != GRN_SUCCESS) {
      return;
    }
  }

  if (need_aggregator_update) {
    uint32_t i;
    for (i = 0; i < DB_OBJ(table)->group.n_aggregators; i++) {
      grn_table_group_aggregator *aggregator = DB_OBJ(table)->group.aggregators[i];
      grn_table_group_aggregator_update(ctx, aggregator, group_id, pi->rid);
      if (ctx->rc != GRN_SUCCESS) {
        return;
      }
    }
  }
}

typedef struct {
  grn_obj bulk;
  grn_obj value_buffer;
  bool need_resolved_id;
  grn_obj *table;
  grn_obj *key;
  grn_obj *range;
  grn_ra_cache cache;
  grn_obj *res;
  bool with_subrec;
  bool is_reference_column;
} single_key_records_data;

static grn_inline grn_id
grn_table_group_single_key_records_resolve_id(grn_ctx *ctx,
                                              single_key_records_data *data,
                                              grn_table_cursor *cursor,
                                              grn_id id)
{
  if (data->need_resolved_id) {
    void *key;
    grn_table_cursor_get_key(ctx, cursor, &key);
    return *((grn_id *)key);
  } else {
    return id;
  }
}

static grn_rc
grn_table_group_single_key_records_foreach_fix_size(grn_ctx *ctx,
                                                    grn_table_cursor *cursor,
                                                    grn_id id,
                                                    void *user_data)
{
  single_key_records_data *data = user_data;
  grn_ra *ra = (grn_ra *)(data->key);
  grn_rset_recinfo *ri = NULL;

  if (data->with_subrec) {
    grn_table_cursor_get_value(ctx, cursor, (void **)&ri);
  }
  grn_id value_id =
    grn_table_group_single_key_records_resolve_id(ctx, data, cursor, id);
  void *column_value = grn_ra_ref_cache(ctx, ra, value_id, &(data->cache));
  if (data->is_reference_column) {
    grn_id reference_id = *((grn_id *)column_value);
    if (reference_id == GRN_ID_NIL) {
      return ctx->rc;
    }
    if (grn_table_at(ctx, data->range, reference_id) == GRN_ID_NIL) {
      return ctx->rc;
    }
  }
  void *value;
  grn_id group_id;
  if (data->res->header.type == GRN_TABLE_HASH_KEY) {
    group_id = grn_hash_add(ctx,
                            (grn_hash *)(data->res),
                            column_value,
                            ra->header->element_size,
                            &value,
                            NULL);
  } else {
    group_id = grn_table_add_v(ctx,
                               data->res,
                               column_value,
                               ra->header->element_size,
                               &value,
                               NULL);
  }
  if (group_id == GRN_ID_NIL) {
    return ctx->rc;
  }
  grn_table_group_add_subrec(ctx,
                             data->res,
                             group_id,
                             value,
                             ri ? ri->score : 0,
                             (grn_rset_posinfo *)&id,
                             0,
                             &(data->value_buffer));
  return ctx->rc;
}

static grn_rc
grn_table_group_single_key_records_foreach_var_size_reference(
  grn_ctx *ctx,
  grn_table_cursor *cursor,
  grn_id id,
  void *user_data)
{
  single_key_records_data *data = user_data;
  grn_ja *ja = (grn_ja *)(data->key);
  grn_rset_recinfo *ri = NULL;

  if (data->with_subrec) {
    grn_table_cursor_get_value(ctx, cursor, (void **)&ri);
  }
  grn_id value_id =
    grn_table_group_single_key_records_resolve_id(ctx, data, cursor, id);
  grn_io_win jw;
  uint32_t value_size;
  grn_id *references = grn_ja_ref(ctx, ja, value_id, &jw, &value_size);
  uint32_t n_references = value_size / sizeof(grn_id);
  uint32_t i;
  for (i = 0; i < n_references; i++) {
    grn_id reference = references[i];
    if (reference == GRN_ID_NIL) {
      continue;
    }
    void *value;
    grn_id group_id;
    if (data->res->header.type == GRN_TABLE_HASH_KEY) {
      group_id = grn_hash_add(ctx,
                              (grn_hash *)(data->res),
                              &reference,
                              sizeof(grn_id),
                              &value,
                              NULL);
    } else {
      group_id = grn_table_add_v(ctx,
                                 data->res,
                                 &reference,
                                 sizeof(grn_id),
                                 &value,
                                 NULL);
    }
    if (group_id == GRN_ID_NIL) {
      continue;
    }
    grn_table_group_add_subrec(ctx,
                               data->res,
                               group_id,
                               value,
                               ri ? ri->score : 0,
                               (grn_rset_posinfo *)&id,
                               0,
                               &(data->value_buffer));
  }
  grn_ja_unref(ctx, &jw);

  return ctx->rc;
}

static grn_rc
grn_table_group_single_key_records_foreach(grn_ctx *ctx,
                                           grn_table_cursor *cursor,
                                           grn_id id,
                                           void *user_data)
{
  single_key_records_data *data = user_data;
  grn_obj *bulk = &(data->bulk);
  grn_obj *value_buffer = &(data->value_buffer);
  void *value;
  grn_rset_recinfo *ri = NULL;

  GRN_BULK_REWIND(bulk);
  if (data->with_subrec) {
    grn_table_cursor_get_value(ctx, cursor, (void **)&ri);
  }
  grn_id value_id =
    grn_table_group_single_key_records_resolve_id(ctx, data, cursor, id);
  grn_obj_get_value(ctx, data->key, value_id, bulk);
  switch (data->bulk.header.type) {
  case GRN_UVECTOR :
    {
      unsigned int element_size;
      uint8_t *elements;
      int i, n_elements;

      element_size = grn_uvector_element_size(ctx, bulk);
      elements = GRN_BULK_HEAD(bulk);
      n_elements = GRN_BULK_VSIZE(bulk) / element_size;
      for (i = 0; i < n_elements; i++) {
        uint8_t *element = elements + (element_size * i);

        if (data->is_reference_column) {
          grn_id reference_id = *((grn_id *)element);
          if (reference_id == GRN_ID_NIL) {
            return ctx->rc;
          }
        }

        grn_id group_id =
          grn_table_add_v(ctx, data->res, element, element_size, &value, NULL);
        if (group_id == GRN_ID_NIL) {
          return ctx->rc;
        }

        grn_table_group_add_subrec(ctx,
                                   data->res, group_id, value,
                                   ri ? ri->score : 0,
                                   (grn_rset_posinfo *)&id, 0,
                                   value_buffer);
      }
    }
    break;
  case GRN_VECTOR :
    {
      unsigned int i, n_elements;
      n_elements = grn_vector_size(ctx, bulk);
      for (i = 0; i < n_elements; i++) {
        const char *content;
        unsigned int content_length;
        content_length = grn_vector_get_element(ctx, bulk, i,
                                                &content, NULL, NULL);
        grn_id group_id =
        grn_table_add_v(ctx, data->res, content, content_length, &value, NULL);
        if (group_id == GRN_ID_NIL) {
          return ctx->rc;
        }
        grn_table_group_add_subrec(ctx, data->res, group_id, value,
                                   ri ? ri->score : 0,
                                   (grn_rset_posinfo *)&id, 0,
                                   value_buffer);
        }
    }
    break;
  case GRN_BULK :
    {
      grn_id group_id;
      if ((!data->is_reference_column ||
           *((grn_id *)GRN_BULK_HEAD(bulk))) &&
          (group_id = grn_table_add_v(ctx, data->res,
                                      GRN_BULK_HEAD(bulk),
                                      GRN_BULK_VSIZE(bulk),
                                      &value, NULL))) {
        grn_table_group_add_subrec(ctx, data->res, group_id, value,
                                   ri ? ri->score : 0,
                                   (grn_rset_posinfo *)&id, 0,
                                   &(data->value_buffer));
      }
    }
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT, "invalid column");
    break;
  }
  return ctx->rc;
}

static void
grn_table_group_single_key_records(grn_ctx *ctx, grn_obj *table,
                                   grn_obj *key, grn_table_group_result *result)
{
  single_key_records_data data;
  grn_table_cursor *cursor;

  GRN_TEXT_INIT(&(data.bulk), 0);
  GRN_VOID_INIT(&(data.value_buffer));
  data.table = table;
  data.key = key;
  data.res = result->table;
  if ((cursor = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0))) {
    data.need_resolved_id = false;
    if (key->header.type == GRN_ACCESSOR) {
      grn_accessor *a = (grn_accessor *)key;
      if (a->action == GRN_ACCESSOR_GET_KEY &&
          a->next && a->next->action == GRN_ACCESSOR_GET_COLUMN_VALUE &&
          a->next->obj && !a->next->next) {
        data.key = a->next->obj;
        data.need_resolved_id = true;
      }
    }
    data.with_subrec = (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC);
    grn_id range_id = grn_obj_get_range(ctx, data.key);
    data.range = grn_ctx_at(ctx, range_id);
    data.is_reference_column = grn_obj_is_table(ctx, data.range);
    if (data.key->header.type == GRN_COLUMN_FIX_SIZE) {
      GRN_RA_CACHE_INIT((grn_ra *)(data.key), &(data.cache));
      grn_table_cursor_foreach(
        ctx,
        cursor,
        grn_table_group_single_key_records_foreach_fix_size,
        &data);
      GRN_RA_CACHE_FIN((grn_ra *)(data.key), &(data.cache));
    } else if (data.key->header.type == GRN_COLUMN_VAR_SIZE &&
               data.is_reference_column) {
      grn_table_cursor_foreach(
        ctx,
        cursor,
        grn_table_group_single_key_records_foreach_var_size_reference,
        &data);
    } else {
      grn_table_cursor_foreach(ctx,
                               cursor,
                               grn_table_group_single_key_records_foreach,
                               &data);
    }
    grn_obj_unref(ctx, data.range);
    grn_table_cursor_close(ctx, cursor);
  }
  GRN_OBJ_FIN(ctx, &(data.value_buffer));
  GRN_OBJ_FIN(ctx, &(data.bulk));
}

#define GRN_TABLE_GROUP_ALL_NAME     "_all"
#define GRN_TABLE_GROUP_ALL_NAME_LEN (sizeof(GRN_TABLE_GROUP_ALL_NAME) - 1)

static void
grn_table_group_all_records(grn_ctx *ctx, grn_obj *table,
                            grn_table_group_result *result)
{
  grn_obj value_buffer;
  grn_table_cursor *tc;
  grn_obj *res = result->table;

  GRN_VOID_INIT(&value_buffer);
  if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0))) {
    void *value;
    grn_id group_id;
    if ((group_id = grn_table_add_v(ctx, res,
                                    GRN_TABLE_GROUP_ALL_NAME,
                                    GRN_TABLE_GROUP_ALL_NAME_LEN,
                                    &value, NULL))) {
      grn_id id;
      while ((id = grn_table_cursor_next(ctx, tc))) {
        grn_rset_recinfo *ri = NULL;
        if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
          grn_table_cursor_get_value(ctx, tc, (void **)&ri);
        }
        grn_table_group_add_subrec(ctx, res, group_id, value,
                                   ri ? ri->score : 0,
                                   (grn_rset_posinfo *)&id, 0,
                                   &value_buffer);
      }
    }
    grn_table_cursor_close(ctx, tc);
  }
  GRN_OBJ_FIN(ctx, &value_buffer);
}

grn_rc
grn_table_group_with_range_gap(grn_ctx *ctx, grn_obj *table,
                               grn_table_sort_key *group_key,
                               grn_obj *res, uint32_t range_gap)
{
  grn_obj *key = group_key->key;
  if (key->header.type == GRN_ACCESSOR) {
    grn_accessor *a = (grn_accessor *)key;
    if (a->action == GRN_ACCESSOR_GET_KEY &&
        a->next && a->next->action == GRN_ACCESSOR_GET_COLUMN_VALUE &&
        a->next->obj && !a->next->next) {
      grn_obj *range = grn_ctx_at(ctx, grn_obj_get_range(ctx, key));
      int idp = GRN_OBJ_TABLEP(range);
      grn_table_cursor *tc;
      if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL,
                                      0, 0, -1, 0))) {
        switch (a->next->obj->header.type) {
        case GRN_COLUMN_FIX_SIZE :
          {
            grn_id id;
            grn_ra *ra = (grn_ra *)a->next->obj;
            unsigned int element_size = (ra)->header->element_size;
            grn_ra_cache cache;
            GRN_RA_CACHE_INIT(ra, &cache);
            while ((id = grn_table_cursor_next(ctx, tc))) {
              void *v, *value;
              grn_id *id_;
              uint32_t key_size;
              grn_rset_recinfo *ri = NULL;
              if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
                grn_table_cursor_get_value(ctx, tc, (void **)&ri);
              }
              id_ = (grn_id *)_grn_table_key(ctx, table, id, &key_size);
              v = grn_ra_ref_cache(ctx, ra, *id_, &cache);
              if (idp && *((grn_id *)v) &&
                  grn_table_at(ctx, range, *((grn_id *)v)) == GRN_ID_NIL) {
                continue;
              }
              if ((!idp || *((grn_id *)v))) {
                grn_id id;
                if (element_size == sizeof(uint32_t)) {
                  uint32_t quantized = (*(uint32_t *)v);
                  quantized -= quantized % range_gap;
                  id = grn_table_add_v(ctx, res, &quantized,
                                       element_size, &value, NULL);
                } else {
                  id = grn_table_add_v(ctx, res, v,
                                       element_size, &value, NULL);
                }
                if (id) {
                  grn_table_add_subrec(ctx,
                                       res,
                                       value,
                                       ri ? ri->score : 0,
                                       (grn_rset_posinfo *)&id,
                                       0);
                }
              }
            }
            GRN_RA_CACHE_FIN(ra, &cache);
          }
          break;
        case GRN_COLUMN_VAR_SIZE :
          if (idp) { /* todo : support other type */
            grn_id id;
            grn_ja *ja = (grn_ja *)a->next->obj;
            while ((id = grn_table_cursor_next(ctx, tc))) {
              grn_io_win jw;
              unsigned int len = 0;
              void *value;
              grn_id *v, *id_;
              uint32_t key_size;
              grn_rset_recinfo *ri = NULL;
              if (DB_OBJ(table)->header.flags & GRN_OBJ_WITH_SUBREC) {
                grn_table_cursor_get_value(ctx, tc, (void **)&ri);
              }
              id_ = (grn_id *)_grn_table_key(ctx, table, id, &key_size);
              if ((v = grn_ja_ref(ctx, ja, *id_, &jw, &len))) {
                while (len) {
                  if ((*v != GRN_ID_NIL) &&
                      grn_table_add_v(ctx, res, v, sizeof(grn_id), &value, NULL)) {
                    grn_table_add_subrec(ctx,
                                         res,
                                         value,
                                         ri ? ri->score : 0,
                                         (grn_rset_posinfo *)&id,
                                         0);
                  }
                  v++;
                  len -= sizeof(grn_id);
                }
                grn_ja_unref(ctx, &jw);
              }
            }
          } else {
            return 0;
          }
          break;
        default :
          return 0;
        }
        grn_table_cursor_close(ctx, tc);
        GRN_TABLE_GROUPED_ON(res);
        return 1;
      }
    }
  }
  return 0;
}

typedef struct {
  grn_obj *table;
  grn_table_sort_key *keys;
  int n_keys;
  grn_table_group_result *results;
  int n_results;
  grn_obj bulk;
  grn_obj vector;
  grn_obj vector_pack_header;
  grn_obj vector_pack_footer;
  grn_id id;
  grn_rset_recinfo *ri;
} grn_table_group_multi_keys_data;

static grn_inline void
grn_table_group_multi_keys_add_record(grn_ctx *ctx,
                                      grn_table_group_multi_keys_data *data)
{
  grn_obj *bulk = &(data->bulk);
  grn_obj *vector = &(data->vector);
  grn_obj *vector_pack_header = &(data->vector_pack_header);
  grn_obj *vector_pack_footer = &(data->vector_pack_footer);
  int r;
  grn_table_group_result *rp;

  for (r = 0, rp = data->results; r < data->n_results; r++, rp++) {
    void *value;
    int end;

    if (rp->key_end > data->n_keys) {
      end = data->n_keys;
    } else {
      end = rp->key_end + 1;
    }
    GRN_BULK_REWIND(vector_pack_header);
    GRN_BULK_REWIND(vector_pack_footer);
    grn_obj *body = grn_vector_pack(ctx,
                                    vector,
                                    rp->key_begin,
                                    end - rp->key_begin,
                                    0,
                                    vector_pack_header,
                                    vector_pack_footer);
    GRN_BULK_REWIND(bulk);
#define VECTOR_VALUE_MARK 0
#define HASH_VALUE_MARK 1
#ifdef GRN_WITH_XXHASH
    bool need_hash_value = false;
    if (grn_table_group_all_hash_value_enable) {
      need_hash_value = true;
    } else if (grn_table_group_fallback_hash_value_enable) {
      const size_t total_size =
        sizeof(uint8_t) +
        GRN_TEXT_LEN(vector_pack_header) +
        GRN_TEXT_LEN(body) +
        GRN_TEXT_LEN(vector_pack_footer);
      if (total_size >= GRN_TABLE_MAX_KEY_SIZE) {
        need_hash_value = true;
      } else {
        GRN_UINT8_PUT(ctx, bulk, VECTOR_VALUE_MARK);
      }
    }
#endif
    GRN_TEXT_PUT(ctx,
                 bulk,
                 GRN_TEXT_VALUE(vector_pack_header),
                 GRN_TEXT_LEN(vector_pack_header));
    GRN_TEXT_PUT(ctx, bulk, GRN_TEXT_VALUE(body), GRN_TEXT_LEN(body));
    GRN_TEXT_PUT(ctx,
                 bulk,
                 GRN_TEXT_VALUE(vector_pack_footer),
                 GRN_TEXT_LEN(vector_pack_footer));
#ifdef GRN_WITH_XXHASH
    if (need_hash_value) {
      XXH128_hash_t hash_value = XXH3_128bits(GRN_TEXT_VALUE(bulk),
                                              GRN_TEXT_LEN(bulk));
      GRN_BULK_REWIND(bulk);
      GRN_UINT8_PUT(ctx, bulk, HASH_VALUE_MARK);
      GRN_UINT64_PUT(ctx, bulk, hash_value.low64);
      GRN_UINT64_PUT(ctx, bulk, hash_value.high64);
    }
#endif
#undef VECTOR_VALUE_MARK
#undef HASH_VALUE_MARK

    grn_id group_id = grn_table_add_v(ctx, rp->table,
                                      GRN_BULK_HEAD(bulk), GRN_BULK_VSIZE(bulk),
                                      &value, NULL);
    if (group_id != GRN_ID_NIL) {
      grn_table_group_add_subrec(ctx, rp->table, group_id, value,
                                 data->ri ? data->ri->score : 0,
                                 (grn_rset_posinfo *)&(data->id), 0,
                                 bulk);
    }
  }
}

static void
grn_table_group_multi_keys_scalar_records(grn_ctx *ctx,
                                          grn_table_group_multi_keys_data *data)
{
  grn_table_cursor *tc = grn_table_cursor_open(ctx,
                                               data->table,
                                               NULL, 0,
                                               NULL, 0,
                                               0, -1, 0);
  if (!tc) {
    return;
  }

  grn_obj *bulk = &(data->bulk);
  grn_obj *vector = &(data->vector);
  const grn_obj_flags flags = DB_OBJ(data->table)->header.flags;
  const bool with_subrec = (flags & GRN_OBJ_WITH_SUBREC);
  while ((data->id = grn_table_cursor_next(ctx, tc))) {
    int k;
    grn_table_sort_key *kp;

    if (with_subrec) {
      grn_table_cursor_get_value(ctx, tc, (void **)&(data->ri));
    }

    GRN_BULK_REWIND(vector);
    for (k = 0, kp = data->keys; k < data->n_keys; k++, kp++) {
      GRN_BULK_REWIND(bulk);
      grn_obj_get_value(ctx, kp->key, data->id, bulk);
      grn_vector_add_element(ctx, vector,
                             GRN_BULK_HEAD(bulk), GRN_BULK_VSIZE(bulk),
                             0,
                             bulk->header.domain);
    }

    grn_table_group_multi_keys_add_record(ctx, data);
  }
  grn_table_cursor_close(ctx, tc);
}

static grn_inline void
grn_table_group_multi_keys_vector_record(grn_ctx *ctx,
                                         grn_table_group_multi_keys_data *data,
                                         grn_obj *key_buffers,
                                         int nth_key)
{
  grn_obj *vector = &(data->vector);

  int k;
  grn_table_sort_key *kp;
  for (k = nth_key, kp = &(data->keys[nth_key]);
       k < data->n_keys;
       k++, kp++) {
    grn_obj *key_buffer = &(key_buffers[k]);
    switch (key_buffer->header.type) {
    case GRN_UVECTOR :
      {
        unsigned int n_vector_elements;
        unsigned int element_size;
        grn_id domain;
        uint8_t *elements;
        unsigned int i, n_elements;

        n_vector_elements = grn_vector_size(ctx, vector);
        domain = key_buffer->header.domain;
        elements = GRN_BULK_HEAD(key_buffer);
        element_size = grn_uvector_element_size(ctx, key_buffer);
        n_elements = GRN_BULK_VSIZE(key_buffer) / element_size;
        for (i = 0; i < n_elements; i++) {
          const char *element_id = elements + (element_size * i);
          grn_vector_add_element(ctx, vector,
                                 element_id, sizeof(grn_id),
                                 0,
                                 domain);
          grn_table_group_multi_keys_vector_record(ctx,
                                                   data,
                                                   key_buffers,
                                                   k + 1);
          while (grn_vector_size(ctx, vector) != n_vector_elements) {
            const char *content;
            grn_vector_pop_element(ctx, vector, &content, NULL, NULL);
          }
        }
        return;
      }
      break;
    case GRN_VECTOR :
      {
        unsigned int n_vector_elements;
        unsigned int i, n_key_elements;

        n_vector_elements = grn_vector_size(ctx, vector);
        n_key_elements = grn_vector_size(ctx, key_buffer);
        for (i = 0; i < n_key_elements; i++) {
          const char *content;
          unsigned int content_length;
          grn_id domain;
          content_length = grn_vector_get_element(ctx, key_buffer, i,
                                                  &content, NULL, &domain);
          grn_vector_add_element(ctx, vector,
                                 content, content_length,
                                 0,
                                 domain);
          grn_table_group_multi_keys_vector_record(ctx,
                                                   data,
                                                   key_buffers,
                                                   k + 1);
          while (grn_vector_size(ctx, vector) != n_vector_elements) {
            grn_vector_pop_element(ctx, vector, &content, NULL, NULL);
          }
        }
        return;
      }
      break;
    default :
      grn_vector_add_element(ctx, vector,
                             GRN_BULK_HEAD(key_buffer),
                             GRN_BULK_VSIZE(key_buffer),
                             0,
                             key_buffer->header.domain);
    }
  }

  if (k == data->n_keys) {
    grn_table_group_multi_keys_add_record(ctx, data);
  }
}

static void
grn_table_group_multi_keys_vector_records(grn_ctx *ctx,
                                          grn_table_group_multi_keys_data *data)
{
  grn_table_cursor *tc = grn_table_cursor_open(ctx,
                                               data->table,
                                               NULL, 0,
                                               NULL, 0,
                                               0, -1, 0);
  if (!tc) {
    return;
  }

  grn_obj *key_buffers = GRN_MALLOCN(grn_obj, data->n_keys);
  if (!key_buffers) {
    grn_table_cursor_close(ctx, tc);
    return;
  }
  int k;
  for (k = 0; k < data->n_keys; k++) {
    GRN_VOID_INIT(&(key_buffers[k]));
  }

  grn_obj *vector = &(data->vector);
  const grn_obj_flags flags = (DB_OBJ(data->table)->header.flags);
  const bool with_subrec = (flags & GRN_OBJ_WITH_SUBREC);
  while ((data->id = grn_table_cursor_next(ctx, tc))) {
    if (with_subrec) {
      grn_table_cursor_get_value(ctx, tc, (void **)&(data->ri));
    }

    grn_table_sort_key *kp;
    for (k = 0, kp = data->keys; k < data->n_keys; k++, kp++) {
      grn_obj *key_buffer = &(key_buffers[k]);
      GRN_BULK_REWIND(key_buffer);
      grn_obj_get_value(ctx, kp->key, data->id, key_buffer);
    }

    GRN_BULK_REWIND(vector);
    grn_table_group_multi_keys_vector_record(ctx, data, key_buffers, 0);
  }
  for (k = 0; k < data->n_keys; k++) {
    GRN_OBJ_FIN(ctx, &(key_buffers[k]));
  }
  GRN_FREE(key_buffers);
  grn_table_cursor_close(ctx, tc);
}

static void
grn_table_group_update_aggregated_value_type_id(grn_ctx *ctx,
                                                grn_obj *table)
{
  if (DB_OBJ(table)->group.aggregated_value_type_id == GRN_DB_FLOAT) {
    return;
  }

  grn_obj *calc_target = DB_OBJ(table)->group.calc_target;
  if (!calc_target) {
    return;
  }

  if (grn_type_id_is_float_family(ctx, grn_obj_get_range(ctx, calc_target))) {
    DB_OBJ(table)->group.aggregated_value_type_id = GRN_DB_FLOAT;
  }
}

grn_rc
grn_table_group(grn_ctx *ctx, grn_obj *table,
                grn_table_sort_key *keys, int n_keys,
                grn_table_group_result *results, int n_results)
{
  grn_rc rc = GRN_SUCCESS;
  bool group_by_all_records = false;
  if (n_keys == 0 && n_results == 1) {
    group_by_all_records = GRN_TRUE;
  } else if (!table || !n_keys || !n_results) {
    ERR(GRN_INVALID_ARGUMENT, "table or n_keys or n_results is void");
    return GRN_INVALID_ARGUMENT;
  }
  GRN_API_ENTER;
  {
    int k, r;
    grn_table_sort_key *kp;
    grn_table_group_result *rp;
    for (k = 0, kp = keys; k < n_keys; k++, kp++) {
      if ((kp->flags & GRN_TABLE_GROUP_BY_COLUMN_VALUE) && !kp->key) {
        ERR(GRN_INVALID_ARGUMENT, "column missing in (%d)", k);
        goto exit;
      }
    }
    for (r = 0, rp = results; r < n_results; r++, rp++) {
      if (!rp->table) {
        grn_table_flags flags;
        grn_obj *key_type = NULL;
        uint32_t additional_value_size;

        flags = GRN_OBJ_TABLE_HASH_KEY|
          GRN_OBJ_WITH_SUBREC|
          GRN_OBJ_UNIT_USERDEF_DOCUMENT;
        if (group_by_all_records) {
          key_type = grn_ctx_at(ctx, GRN_DB_SHORT_TEXT);
        } else if (n_keys == 1) {
          key_type = grn_ctx_at(ctx, grn_obj_get_range(ctx, keys[0].key));
        } else {
          flags |= GRN_OBJ_KEY_VAR_SIZE;
        }
        additional_value_size = grn_rset_recinfo_calc_values_size(ctx,
                                                                  rp->flags);
        rp->table = grn_table_create_with_max_n_subrecs(ctx, NULL, 0, NULL,
                                                        flags,
                                                        key_type, table,
                                                        rp->max_n_subrecs,
                                                        additional_value_size);
        if (!rp->table) {
          goto exit;
        }
      }
      DB_OBJ(rp->table)->group.flags = rp->flags;
      DB_OBJ(rp->table)->group.calc_target = rp->calc_target;
      grn_table_group_update_aggregated_value_type_id(ctx, rp->table);
      if (rp->flags & GRN_TABLE_GROUP_CALC_AGGREGATOR) {
        DB_OBJ(rp->table)->group.aggregators = rp->aggregators;
        DB_OBJ(rp->table)->group.n_aggregators = rp->n_aggregators;
      } else {
        DB_OBJ(rp->table)->group.aggregators = NULL;
        DB_OBJ(rp->table)->group.n_aggregators = 0;
      }
      {
        uint32_t i;
        for (i = 0; i < DB_OBJ(rp->table)->group.n_aggregators; i++) {
          grn_table_group_aggregator *aggregator =
            DB_OBJ(rp->table)->group.aggregators[i];
          grn_table_group_aggregator_prepare(ctx, aggregator, rp->table);
          if (ctx->rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
    }
    if (group_by_all_records) {
      grn_table_group_all_records(ctx, table, results);
    } else if (n_keys == 1 && n_results == 1) {
      grn_table_group_single_key_records(ctx, table, keys->key, results);
    } else {
      grn_bool have_vector = GRN_FALSE;
      for (k = 0, kp = keys; k < n_keys; k++, kp++) {
        grn_id range_id;
        grn_obj_flags range_flags = 0;
        grn_obj_get_range_info(ctx, kp->key, &range_id, &range_flags);
        if (range_flags == GRN_OBJ_VECTOR) {
          have_vector = GRN_TRUE;
          break;
        }
      }
      grn_table_group_multi_keys_data data;
      data.table = table;
      data.keys = keys;
      data.n_keys = n_keys;
      data.results = results;
      data.n_results = n_results;
      GRN_TEXT_INIT(&(data.bulk), 0);
      GRN_OBJ_INIT(&(data.vector), GRN_VECTOR, 0, GRN_DB_VOID);
      GRN_TEXT_INIT(&(data.vector_pack_header), 0);
      GRN_TEXT_INIT(&(data.vector_pack_footer), 0);
      data.id = GRN_ID_NIL;
      data.ri = NULL;
      if (have_vector) {
        grn_table_group_multi_keys_vector_records(ctx, &data);
      } else {
        grn_table_group_multi_keys_scalar_records(ctx, &data);
      }
      GRN_OBJ_FIN(ctx, &(data.vector_pack_footer));
      GRN_OBJ_FIN(ctx, &(data.vector_pack_header));
      GRN_OBJ_FIN(ctx, &(data.vector));
      GRN_OBJ_FIN(ctx, &(data.bulk));
    }
    for (r = 0, rp = results; r < n_results; r++, rp++) {
      GRN_TABLE_GROUPED_ON(rp->table);
      DB_OBJ(rp->table)->group.calc_target = NULL;
      {
        uint32_t i;
        for (i = 0; i < DB_OBJ(rp->table)->group.n_aggregators; i++) {
          grn_table_group_aggregator *aggregator =
            DB_OBJ(rp->table)->group.aggregators[i];
          grn_table_group_aggregator_postpare(ctx, aggregator);
        }
      }
    }
  }
exit :
  GRN_API_RETURN(rc);
}
