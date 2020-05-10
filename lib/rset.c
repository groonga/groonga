/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2015 Brazil

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

uint32_t
grn_rset_recinfo_calc_values_size(grn_ctx *ctx, grn_table_group_flags flags)
{
  uint32_t size = 0;

  if (flags & GRN_TABLE_GROUP_CALC_MAX) {
    size += GRN_RSET_MAX_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_MIN) {
    size += GRN_RSET_MIN_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_SUM) {
    size += GRN_RSET_SUM_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_AVG) {
    size += GRN_RSET_AVG_SIZE;
  }

  return size;
}

grn_id
grn_rset_aggregated_value_get_type_id(grn_ctx *ctx, grn_obj *table)
{
  grn_obj *calc_target = DB_OBJ(table)->group.calc_target;
  if (!calc_target) {
    return GRN_DB_INT64;
  }

  if (grn_type_id_is_float_family(ctx, DB_OBJ(calc_target)->range)) {
    return GRN_DB_FLOAT;
  } else {
    return GRN_DB_INT64;
  }
}

typedef struct {
  grn_rset_recinfo *ri;
  byte *values;
  grn_table_group_flags flags;
  bool is_float_source;
  grn_obj value_int64;
  grn_obj value_float;
} grn_rset_recinfo_update_calc_values_data;

static void
grn_rset_recinfo_update_calc_values_bulk(
  grn_ctx *ctx,
  grn_rset_recinfo_update_calc_values_data *data,
  grn_obj *value,
  bool is_first_value)
{
  grn_obj *value_int64 = &(data->value_int64);
  grn_obj *value_float = &(data->value_float);
  bool need_int64_value = false;
  bool need_float_value = false;

  if (data->flags & (GRN_TABLE_GROUP_CALC_MAX |
                     GRN_TABLE_GROUP_CALC_MIN |
                     GRN_TABLE_GROUP_CALC_SUM)) {
    if (data->is_float_source) {
      need_float_value = true;
    } else {
      need_int64_value = true;
    }
  }
  if (data->flags & GRN_TABLE_GROUP_CALC_AVG) {
    need_float_value = true;
  }
  if (need_int64_value) {
    grn_obj_cast(ctx, value, value_int64, false);
  }
  if (need_float_value) {
    grn_obj_cast(ctx, value, value_float, false);
  }

  byte *values = data->values;
  if (data->flags & GRN_TABLE_GROUP_CALC_MAX) {
    grn_rset_aggregated_value *current_max = (grn_rset_aggregated_value *)values;
    if (data->is_float_source) {
      double value_raw = GRN_FLOAT_VALUE(value_float);
      if (is_first_value || value_raw > current_max->value_double) {
        current_max->value_double = value_raw;
      }
    } else {
      int64_t value_raw = GRN_INT64_VALUE(value_int64);
      if (is_first_value || value_raw > current_max->value_int64) {
        current_max->value_int64 = value_raw;
      }
    }
    values += GRN_RSET_MAX_SIZE;
  }
  if (data->flags & GRN_TABLE_GROUP_CALC_MIN) {
    grn_rset_aggregated_value *current_min = (grn_rset_aggregated_value *)values;
    if (data->is_float_source) {
      double value_raw = GRN_FLOAT_VALUE(value_float);
      if (is_first_value || value_raw < current_min->value_double) {
        current_min->value_double = value_raw;
      }
    } else {
      int64_t value_raw = GRN_INT64_VALUE(value_int64);
      if (is_first_value || value_raw < current_min->value_int64) {
        current_min->value_int64 = value_raw;
      }
    }
    values += GRN_RSET_MIN_SIZE;
  }
  if (data->flags & GRN_TABLE_GROUP_CALC_SUM) {
    grn_rset_aggregated_value *current_sum = (grn_rset_aggregated_value *)values;
    if (data->is_float_source) {
      double value_raw = GRN_FLOAT_VALUE(value_float);
      current_sum->value_double += value_raw;
    } else {
      int64_t value_raw = GRN_INT64_VALUE(value_int64);
      current_sum->value_int64 += value_raw;
    }
    values += GRN_RSET_SUM_SIZE;
  }
  if (data->flags & GRN_TABLE_GROUP_CALC_AVG) {
    double *current_average = (double *)values;
    uint64_t *n_values = (uint64_t *)(((double *)values) + 1);
    double value_raw = GRN_FLOAT_VALUE(value_float);
    (*n_values)++;
    *current_average += (value_raw - *current_average) / *n_values;
    values += GRN_RSET_AVG_SIZE;
  }
}

static void
grn_rset_recinfo_update_calc_values_uvector(
  grn_ctx *ctx,
  grn_rset_recinfo_update_calc_values_data *data,
  grn_obj *value)
{
  grn_obj element_value;
  unsigned int element_size;
  int i, n_elements;

  element_size = grn_uvector_element_size(ctx, value);
  if (element_size == 0) {
    return;
  }

  GRN_VALUE_FIX_SIZE_INIT(&element_value, 0, value->header.domain);
  n_elements = grn_vector_size(ctx, value);
  for (i = 0; i < n_elements; i++) {
    GRN_BULK_REWIND(&element_value);
    grn_bulk_write(ctx,
                   &element_value,
                   GRN_BULK_HEAD(value) + (element_size * i),
                   element_size);
    const bool is_first_value = (data->ri->n_subrecs == 1 && i == 0);
    grn_rset_recinfo_update_calc_values_bulk(ctx,
                                             data,
                                             &element_value,
                                             is_first_value);
  }
  GRN_OBJ_FIN(ctx, &element_value);
}

void
grn_rset_recinfo_update_calc_values(grn_ctx *ctx,
                                    grn_rset_recinfo *ri,
                                    grn_obj *table,
                                    grn_obj *value)
{
  grn_rset_recinfo_update_calc_values_data data;

  data.ri = ri;
  data.values = (((byte *)ri->subrecs) +
                 GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                       DB_OBJ(table)->max_n_subrecs));
  data.flags = DB_OBJ(table)->group.flags;
  data.is_float_source =
    (grn_rset_aggregated_value_get_type_id(ctx, table) == GRN_DB_FLOAT);
  GRN_INT64_INIT(&(data.value_int64), 0);
  GRN_FLOAT_INIT(&(data.value_float), 0);

  switch (value->header.type) {
  case GRN_BULK :
    grn_rset_recinfo_update_calc_values_bulk(ctx,
                                             &data,
                                             value,
                                             ri->n_subrecs == 1);
    break;
  case GRN_UVECTOR :
    grn_rset_recinfo_update_calc_values_uvector(ctx, &data, value);
    break;
  default :
    break;
  }

  GRN_OBJ_FIN(ctx, &(data.value_float));
  GRN_OBJ_FIN(ctx, &(data.value_int64));
}

grn_rset_aggregated_value *
grn_rset_recinfo_get_max_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  const grn_table_group_flags flags = DB_OBJ(table)->group.flags;
  if (!(flags & GRN_TABLE_GROUP_CALC_MAX)) {
    return NULL;
  }

  byte *values = (((byte *)ri->subrecs) +
                  GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                        DB_OBJ(table)->max_n_subrecs));
  return (grn_rset_aggregated_value *)values;
}

grn_rset_aggregated_value
grn_rset_recinfo_get_max(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table)
{
  grn_rset_aggregated_value *max_address;

  max_address = grn_rset_recinfo_get_max_(ctx, ri, table);
  if (max_address) {
    return *max_address;
  } else {
    grn_rset_aggregated_value value = {0};
    return value;
  }
}

void
grn_rset_recinfo_set_max(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table,
                         grn_rset_aggregated_value max)
{
  grn_rset_aggregated_value *max_address =
    grn_rset_recinfo_get_max_(ctx, ri, table);
  if (!max_address) {
    return;
  }
  *max_address = max;
}

grn_rset_aggregated_value *
grn_rset_recinfo_get_min_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  const grn_table_group_flags flags = DB_OBJ(table)->group.flags;
  if (!(flags & GRN_TABLE_GROUP_CALC_MIN)) {
    return NULL;
  }

  byte *values = (((byte *)ri->subrecs) +
                  GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                        DB_OBJ(table)->max_n_subrecs));

  if (flags & GRN_TABLE_GROUP_CALC_MAX) {
    values += GRN_RSET_MAX_SIZE;
  }

  return (grn_rset_aggregated_value *)values;
}

grn_rset_aggregated_value
grn_rset_recinfo_get_min(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table)
{
  grn_rset_aggregated_value *min_address =
    grn_rset_recinfo_get_min_(ctx, ri, table);
  if (min_address) {
    return *min_address;
  } else {
    grn_rset_aggregated_value value = {0};
    return value;
  }
}

void
grn_rset_recinfo_set_min(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table,
                         grn_rset_aggregated_value min)
{
  grn_rset_aggregated_value *min_address =
    grn_rset_recinfo_get_min_(ctx, ri, table);
  if (!min_address) {
    return;
  }
  *min_address = min;
}

grn_rset_aggregated_value *
grn_rset_recinfo_get_sum_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  const grn_table_group_flags flags = DB_OBJ(table)->group.flags;
  if (!(flags & GRN_TABLE_GROUP_CALC_SUM)) {
    return NULL;
  }

  byte *values = (((byte *)ri->subrecs) +
                  GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                        DB_OBJ(table)->max_n_subrecs));

  if (flags & GRN_TABLE_GROUP_CALC_MAX) {
    values += GRN_RSET_MAX_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_MIN) {
    values += GRN_RSET_MIN_SIZE;
  }

  return (grn_rset_aggregated_value *)values;
}

grn_rset_aggregated_value
grn_rset_recinfo_get_sum(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table)
{
  grn_rset_aggregated_value *sum_address =
    grn_rset_recinfo_get_sum_(ctx, ri, table);
  if (sum_address) {
    return *sum_address;
  } else {
    grn_rset_aggregated_value value = {0};
    return value;
  }
}

void
grn_rset_recinfo_set_sum(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table,
                         grn_rset_aggregated_value sum)
{
  grn_rset_aggregated_value *sum_address =
    grn_rset_recinfo_get_sum_(ctx, ri, table);
  if (!sum_address) {
    return;
  }
  *sum_address = sum;
}

double *
grn_rset_recinfo_get_avg_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  const grn_table_group_flags flags = DB_OBJ(table)->group.flags;
  if (!(flags & GRN_TABLE_GROUP_CALC_AVG)) {
    return NULL;
  }

  byte *values = (((byte *)ri->subrecs) +
                  GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                        DB_OBJ(table)->max_n_subrecs));

  if (flags & GRN_TABLE_GROUP_CALC_MAX) {
    values += GRN_RSET_MAX_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_MIN) {
    values += GRN_RSET_MIN_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_SUM) {
    values += GRN_RSET_SUM_SIZE;
  }

  return (double *)values;
}

double
grn_rset_recinfo_get_avg(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table)
{
  double *avg_address = grn_rset_recinfo_get_avg_(ctx, ri, table);
  if (avg_address) {
    return *avg_address;
  } else {
    return 0;
  }
}

void
grn_rset_recinfo_set_avg(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table,
                         double avg)
{
  double *avg_address = grn_rset_recinfo_get_avg_(ctx, ri, table);
  if (!avg_address) {
    return;
  }
  *avg_address = avg;
}
