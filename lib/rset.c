/*
  Copyright (C) 2009-2015  Brazil
  Copyright (C) 2020-2022  Sutou Kouhei <kou@clear-code.com>

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

grn_inline static void
grn_rset_subrecs_push(byte *subrecs,
                      size_t size,
                      size_t n_subrecs,
                      double score,
                      void *body,
                      int dir)
{
  byte *v;
  double *c2;
  size_t n = n_subrecs - 1;
  size_t n2;
  while (n > 0) {
    n2 = (n - 1) >> 1;
    c2 = GRN_RSET_SUBRECS_NTH(subrecs,size,n2);
    if (GRN_RSET_SUBRECS_CMP(score, *c2, dir) >= 0) { break; }
    GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
    n = n2;
  }
  v = subrecs + n * (GRN_RSET_SCORE_SIZE + size);
  *((double *)v) = score;
  grn_memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
}

grn_inline static void
grn_rset_subrecs_replace_min(byte *subrecs,
                             size_t size,
                             size_t n_subrecs,
                             double score,
                             void *body,
                             int dir)
{
  byte *v;
  size_t n = 0;
  size_t n1;
  size_t n2;
  double *c1, *c2;
  for (;;) {
    n1 = n * 2 + 1;
    n2 = n1 + 1;
    c1 = n1 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n1) : NULL;
    c2 = n2 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n2) : NULL;
    if (c1 && GRN_RSET_SUBRECS_CMP(score, *c1, dir) > 0) {
      if (c2 &&
          GRN_RSET_SUBRECS_CMP(score, *c2, dir) > 0 &&
          GRN_RSET_SUBRECS_CMP(*c1, *c2, dir) > 0) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c1);
        n = n1;
      }
    } else {
      if (c2 && GRN_RSET_SUBRECS_CMP(score, *c2, dir) > 0) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        break;
      }
    }
  }
  v = subrecs + n * (GRN_RSET_SCORE_SIZE + size);
  grn_memcpy(v, &score, GRN_RSET_SCORE_SIZE);
  grn_memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
}

void
grn_rset_add_subrec(grn_ctx *ctx,
                    grn_rset_recinfo *ri,
                    grn_obj *table,
                    double score,
                    grn_rset_posinfo *pi,
                    int dir)
{
  uint32_t limit = DB_OBJ(table)->max_n_subrecs;
  if (limit == 0) {
    return;
  }

  if (!pi) {
    return;
  }

  size_t subrec_size = DB_OBJ(table)->subrec_size;
  size_t n_subrecs = GRN_RSET_N_SUBRECS(ri);
  byte *body = (byte *)pi + DB_OBJ(table)->subrec_offset;
  if (limit < n_subrecs) {
    if (GRN_RSET_SUBRECS_CMP(score, *((double *)(ri->subrecs)), dir) > 0) {
      grn_rset_subrecs_replace_min((byte *)ri->subrecs,
                                   subrec_size,
                                   limit,
                                   score,
                                   body,
                                   dir);
    }
  } else {
    grn_rset_subrecs_push((byte *)ri->subrecs,
                          subrec_size,
                          n_subrecs,
                          score,
                          body,
                          dir);
  }
}

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
  if (flags & GRN_TABLE_GROUP_CALC_MEAN) {
    size += GRN_RSET_MEAN_SIZE;
  }

  return size;
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
  if (data->flags & GRN_TABLE_GROUP_CALC_MEAN) {
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
  if (data->flags & GRN_TABLE_GROUP_CALC_MEAN) {
    double *current_mean = (double *)values;
    uint64_t *n_values = (uint64_t *)(((double *)values) + 1);
    double value_raw = GRN_FLOAT_VALUE(value_float);
    (*n_values)++;
    *current_mean += (value_raw - *current_mean) / (double)(*n_values);
    values += GRN_RSET_MEAN_SIZE;
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
  size_t i, n_elements;

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
    (DB_OBJ(table)->group.aggregated_value_type_id == GRN_DB_FLOAT);
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
grn_rset_recinfo_get_mean_(grn_ctx *ctx,
                           grn_rset_recinfo *ri,
                           grn_obj *table)
{
  const grn_table_group_flags flags = DB_OBJ(table)->group.flags;
  if (!(flags & GRN_TABLE_GROUP_CALC_MEAN)) {
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
grn_rset_recinfo_get_mean(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  double *mean_address = grn_rset_recinfo_get_mean_(ctx, ri, table);
  if (mean_address) {
    return *mean_address;
  } else {
    return 0;
  }
}

void
grn_rset_recinfo_set_mean(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table,
                          double mean)
{
  double *mean_address = grn_rset_recinfo_get_mean_(ctx, ri, table);
  if (!mean_address) {
    return;
  }
  *mean_address = mean;
}
