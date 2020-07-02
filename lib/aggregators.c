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

#include "grn_db.h"
#include "grn_aggregators.h"
#include "grn_bulk.h"

static void *
aggregator_sum_init(grn_ctx *ctx, grn_aggregator_data *data)
{
  grn_obj *args = grn_aggregator_data_get_args(ctx, data);
  size_t n_args = GRN_PTR_VECTOR_SIZE(args);
  if (n_args != 1) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "aggregator_sum(): wrong number of arguments "
                     "(%" GRN_FMT_SIZE " for 1)",
                     n_args);
    return NULL;
  }

  grn_obj *target = GRN_PTR_VALUE_AT(args, 0);
  if (!grn_obj_is_number_family_scalar_column(ctx, target)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "aggregator_sum(): target must be numeric column: %.*s",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  return NULL;
}

static grn_rc
aggregator_sum_update(grn_ctx *ctx, grn_aggregator_data *data)
{
  grn_obj *args = grn_aggregator_data_get_args(ctx, data);
  grn_obj *target = GRN_PTR_VALUE_AT(args, 0);
  grn_obj current_value;
  GRN_VOID_INIT(&current_value);
  grn_id source_id = grn_aggregator_data_get_source_id(ctx, data);
  grn_obj_get_value(ctx, target, source_id, &current_value);

  grn_obj *output_column = grn_aggregator_data_get_output_column(ctx, data);
  grn_id group_id = grn_aggregator_data_get_group_id(ctx, data);
  grn_obj_set_value(ctx, output_column, group_id, &current_value, GRN_OBJ_INCR);
  GRN_OBJ_FIN(ctx, &current_value);

  return ctx->rc;
}

typedef struct {
  uint32_t n_values;
  double mean;
} aggregator_mean_value;

typedef struct {
  grn_hash *values;
} aggregator_mean_data;

static void *
aggregator_mean_init(grn_ctx *ctx, grn_aggregator_data *data)
{
  grn_obj *args = grn_aggregator_data_get_args(ctx, data);
  size_t n_args = GRN_PTR_VECTOR_SIZE(args);
  if (n_args != 1) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "aggregator_mean(): wrong number of arguments "
                     "(%" GRN_FMT_SIZE " for 1)",
                     n_args);
    return NULL;
  }

  grn_obj *target = GRN_PTR_VALUE_AT(args, 0);
  if (!grn_obj_is_number_family_scalar_column(ctx, target)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_limited(ctx, &inspected, target);
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "aggregator_mean(): target must be numeric column: %.*s",
                     (int)GRN_TEXT_LEN(&inspected),
                     GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    return NULL;
  }

  aggregator_mean_data *mean_data = GRN_MALLOCN(aggregator_mean_data, 1);
  if (!mean_data) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "aggregator_mean(): failed to allocate: %s",
                     ctx->errbuf);
    return NULL;
  }

  mean_data->values = grn_hash_create(ctx,
                                      NULL,
                                      sizeof(grn_id),
                                      sizeof(aggregator_mean_value),
                                      GRN_OBJ_TABLE_HASH_KEY);
  if (!mean_data->values) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "aggregator_mean(): failed to create hash table: %s",
                     ctx->errbuf);
    GRN_FREE(mean_data);
    return NULL;
  }

  return mean_data;
}

static grn_rc
aggregator_mean_update(grn_ctx *ctx, grn_aggregator_data *data)
{
  aggregator_mean_data *mean_data =
    grn_aggregator_data_get_user_data(ctx, data);
  grn_id group_id = grn_aggregator_data_get_group_id(ctx, data);
  void *value;
  int added;
  grn_id value_id = grn_hash_add(ctx,
                                 mean_data->values,
                                 &group_id,
                                 sizeof(grn_id),
                                 &value,
                                 &added);
  if (value_id == GRN_ID_NIL) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "aggregator_mean(): failed to add ID: <%u>: %s",
                     group_id,
                     ctx->errbuf);
    return ctx->rc;
  }

  aggregator_mean_value *mean_value = value;
  if (added) {
    mean_value->n_values = 0;
    mean_value->mean = 0.0;
  }

  grn_obj *args = grn_aggregator_data_get_args(ctx, data);
  grn_obj *target = GRN_PTR_VALUE_AT(args, 0);
  grn_obj current_value;
  GRN_VOID_INIT(&current_value);
  grn_id source_id = grn_aggregator_data_get_source_id(ctx, data);
  grn_obj_get_value(ctx, target, source_id, &current_value);
  double current_value_float = grn_bulk_get_float(ctx, &current_value);
  GRN_OBJ_FIN(ctx, &current_value);

  mean_value->n_values++;
  mean_value->mean +=
    (current_value_float - mean_value->mean) / mean_value->n_values;

  return ctx->rc;
}

static grn_rc
aggregator_mean_fin(grn_ctx *ctx, grn_aggregator_data *data)
{
  aggregator_mean_data *mean_data =
    grn_aggregator_data_get_user_data(ctx, data);

  grn_obj *output_column = grn_aggregator_data_get_output_column(ctx, data);
  grn_obj mean_float;
  GRN_FLOAT_INIT(&mean_float, 0);
  GRN_HASH_EACH_BEGIN(ctx, mean_data->values, cursor, id) {
    void *key;
    unsigned int key_size;
    void *value;
    grn_hash_cursor_get_key_value(ctx, cursor, &key, &key_size, &value);
    grn_id group_id = *((grn_id *)key);
    aggregator_mean_value *mean_value = value;
    GRN_FLOAT_SET(ctx, &mean_float, mean_value->mean);
    grn_obj_set_value(ctx,
                      output_column,
                      group_id,
                      &mean_float,
                      GRN_OBJ_SET);
  } GRN_HASH_EACH_END(ctx, cursor);
  GRN_OBJ_FIN(ctx, &mean_float);

  grn_hash_close(ctx, mean_data->values);
  GRN_FREE(mean_data);

  return ctx->rc;
}

grn_rc
grn_db_init_builtin_aggregators(grn_ctx *ctx)
{
  grn_aggregator_create(ctx,
                        "aggregator_sum", -1,
                        aggregator_sum_init,
                        aggregator_sum_update,
                        NULL);

  grn_aggregator_create(ctx,
                        "aggregator_mean", -1,
                        aggregator_mean_init,
                        aggregator_mean_update,
                        aggregator_mean_fin);

  return GRN_SUCCESS;
}
