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

static void
grn_rset_recinfo_update_calc_values_bulk(grn_ctx *ctx,
                                         grn_table_group_flags flags,
                                         grn_bool is_first_value,
                                         byte *values,
                                         grn_obj *value,
                                         grn_obj *value_int64,
                                         grn_obj *value_float)
{
  bool is_float = false;
  if (value->header.domain == GRN_DB_FLOAT ||
      value->header.domain == GRN_DB_FLOAT32) {
    is_float = true;
  }

  if (flags & (GRN_TABLE_GROUP_CALC_MAX |
               GRN_TABLE_GROUP_CALC_MIN |
               GRN_TABLE_GROUP_CALC_SUM)) {
    if (is_float) {
      grn_obj_cast(ctx, value, value_float, GRN_FALSE);
    } else {
      grn_obj_cast(ctx, value, value_int64, GRN_FALSE);
    }
  }
  if (flags & GRN_TABLE_GROUP_CALC_AVG) {
    grn_obj_cast(ctx, value, value_float, GRN_FALSE);
  }

  if (flags & GRN_TABLE_GROUP_CALC_MAX) {
    bool *is_float_ = (bool *)(((double *)values) + 1);
    *is_float_ = is_float;

    if (is_float) {
      double current_max = *((double *)values);
      double value_raw = GRN_FLOAT_VALUE(value_float);
      if (is_first_value || value_raw > current_max) {
        *((double *)values) = value_raw;
      }
    } else {
      int64_t current_max = *((int64_t *)values);
      int64_t value_raw = GRN_INT64_VALUE(value_int64);
      if (is_first_value || value_raw > current_max) {
        *((int64_t *)values) = value_raw;
      }
    }
    values += GRN_RSET_MAX_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_MIN) {
    bool *is_float_ = (bool *)(((double *)values) + 1);
    *is_float_ = is_float;

    if (is_float) {
      double current_min = *((double *)values);
      double value_raw = GRN_FLOAT_VALUE(value_float);
      if (is_first_value || value_raw < current_min) {
        *((double *)values) = value_raw;
      }
    } else {
      int64_t current_min = *((int64_t *)values);
      int64_t value_raw = GRN_INT64_VALUE(value_int64);
      if (is_first_value || value_raw < current_min) {
        *((int64_t *)values) = value_raw;
      }
    }
    values += GRN_RSET_MIN_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_SUM) {
    bool *is_float_ = (bool *)(((double *)values) + 1);
    *is_float_ = is_float;

    if (is_float) {
      double value_raw = GRN_FLOAT_VALUE(value_float);
      *((double *)values) += value_raw;
    } else {
      int64_t value_raw = GRN_INT64_VALUE(value_int64);
      *((int64_t *)values) += value_raw;
    }
    values += GRN_RSET_SUM_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_AVG) {
    double *current_average = (double *)values;
    uint64_t *n_values = (uint64_t *)(((double *)values) + 1);
    double value_raw = GRN_FLOAT_VALUE(value_float);
    (*n_values)++;
    *current_average += (value_raw - *current_average) / *n_values;
    values += GRN_RSET_AVG_SIZE;
  }
}

static void
grn_rset_recinfo_update_calc_values_uvector(grn_ctx *ctx,
                                            grn_rset_recinfo *ri,
                                            grn_table_group_flags flags,
                                            byte *values,
                                            grn_obj *value,
                                            grn_obj *value_int64,
                                            grn_obj *value_float)
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
    grn_rset_recinfo_update_calc_values_bulk(ctx,
                                             flags,
                                             (ri->n_subrecs == 1 && i == 0),
                                             values,
                                             &element_value,
                                             value_int64,
                                             value_float);
  }
  GRN_OBJ_FIN(ctx, &element_value);
}

void
grn_rset_recinfo_update_calc_values(grn_ctx *ctx,
                                    grn_rset_recinfo *ri,
                                    grn_obj *table,
                                    grn_obj *value)
{
  grn_table_group_flags flags;
  byte *values;
  grn_obj value_int64;
  grn_obj value_float;

  flags = DB_OBJ(table)->flags.group;

  values = (((byte *)ri->subrecs) +
            GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                  DB_OBJ(table)->max_n_subrecs));

  GRN_INT64_INIT(&value_int64, 0);
  GRN_FLOAT_INIT(&value_float, 0);

  switch (value->header.type) {
  case GRN_BULK :
    grn_rset_recinfo_update_calc_values_bulk(ctx,
                                             flags,
                                             ri->n_subrecs == 1,
                                             values,
                                             value,
                                             &value_int64,
                                             &value_float);
    break;
  case GRN_UVECTOR :
    grn_rset_recinfo_update_calc_values_uvector(ctx,
                                                ri,
                                                flags,
                                                values,
                                                value,
                                                &value_int64,
                                                &value_float);
    break;
  default :
    break;
  }

  GRN_OBJ_FIN(ctx, &value_float);
  GRN_OBJ_FIN(ctx, &value_int64);
}

byte *
grn_rset_recinfo_get_max_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  grn_table_group_flags flags;
  byte *values;

  flags = DB_OBJ(table)->flags.group;
  if (!(flags & GRN_TABLE_GROUP_CALC_MAX)) {
    return NULL;
  }

  values = (((byte *)ri->subrecs) +
            GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                  DB_OBJ(table)->max_n_subrecs));

  return values;
}

int64_t
grn_rset_recinfo_get_max(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table)
{
  int64_t *max_address;

  max_address = (int64_t *)grn_rset_recinfo_get_max_(ctx, ri, table);
  if (max_address) {
    return *max_address;
  } else {
    return 0;
  }
}

double
grn_rset_recinfo_get_max_float(grn_ctx *ctx,
                               grn_rset_recinfo *ri,
                               grn_obj *table)
{
  double *max_address;

  max_address = (double *)grn_rset_recinfo_get_max_(ctx, ri, table);
  if (max_address) {
    return *max_address;
  } else {
    return 0;
  }
}

void
grn_rset_recinfo_set_max(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table,
                         int64_t max)
{
  int64_t *max_address;

  max_address = (int64_t *)grn_rset_recinfo_get_max_(ctx, ri, table);
  if (!max_address) {
    return;
  }

  *max_address = max;
}

void
grn_rset_recinfo_set_max_float(grn_ctx *ctx,
                               grn_rset_recinfo *ri,
                               grn_obj *table,
                               double max)
{
  double *max_address;

  max_address = (double *)grn_rset_recinfo_get_max_(ctx, ri, table);
  if (!max_address) {
    return;
  }

  *max_address = max;
}

bool
grn_rset_recinfo_is_max_float(grn_ctx *ctx,
                              grn_rset_recinfo *ri,
                              grn_obj *table)
{
  int64_t *max_address;

  max_address = (int64_t *)grn_rset_recinfo_get_max_(ctx, ri, table);
  if (max_address) {
    bool *is_float = (bool *)(((int64_t *)max_address) + 1);
    return *is_float;
  } else {
    return false;
  }
}

byte *
grn_rset_recinfo_get_min_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  grn_table_group_flags flags;
  byte *values;

  flags = DB_OBJ(table)->flags.group;
  if (!(flags & GRN_TABLE_GROUP_CALC_MIN)) {
    return NULL;
  }

  values = (((byte *)ri->subrecs) +
            GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                  DB_OBJ(table)->max_n_subrecs));

  if (flags & GRN_TABLE_GROUP_CALC_MAX) {
    values += GRN_RSET_MAX_SIZE;
  }

  return values;
}

int64_t
grn_rset_recinfo_get_min(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table)
{
  int64_t *min_address;

  min_address = (int64_t *)grn_rset_recinfo_get_min_(ctx, ri, table);
  if (min_address) {
    return *min_address;
  } else {
    return 0;
  }
}

double
grn_rset_recinfo_get_min_float(grn_ctx *ctx,
                               grn_rset_recinfo *ri,
                               grn_obj *table)
{
  double *min_address;

  min_address = (double *)grn_rset_recinfo_get_min_(ctx, ri, table);
  if (min_address) {
    return *min_address;
  } else {
    return 0;
  }
}

void
grn_rset_recinfo_set_min(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table,
                         int64_t min)
{
  int64_t *min_address;

  min_address = (int64_t *)grn_rset_recinfo_get_min_(ctx, ri, table);
  if (!min_address) {
    return;
  }

  *min_address = min;
}

void
grn_rset_recinfo_set_min_float(grn_ctx *ctx,
                               grn_rset_recinfo *ri,
                               grn_obj *table,
                               double min)
{
  double *min_address;

  min_address = (double *)grn_rset_recinfo_get_min_(ctx, ri, table);
  if (!min_address) {
    return;
  }

  *min_address = min;
}

bool
grn_rset_recinfo_is_min_float(grn_ctx *ctx,
                              grn_rset_recinfo *ri,
                              grn_obj *table)
{
  int64_t *min_address;

  min_address = (int64_t *)grn_rset_recinfo_get_min_(ctx, ri, table);
  if (min_address) {
    bool *is_float = (bool *)(((int64_t *)min_address) + 1);
    return *is_float;
  } else {
    return false;
  }
}

byte *
grn_rset_recinfo_get_sum_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  grn_table_group_flags flags;
  byte *values;

  flags = DB_OBJ(table)->flags.group;
  if (!(flags & GRN_TABLE_GROUP_CALC_SUM)) {
    return NULL;
  }

  values = (((byte *)ri->subrecs) +
            GRN_RSET_SUBRECS_SIZE(DB_OBJ(table)->subrec_size,
                                  DB_OBJ(table)->max_n_subrecs));

  if (flags & GRN_TABLE_GROUP_CALC_MAX) {
    values += GRN_RSET_MAX_SIZE;
  }
  if (flags & GRN_TABLE_GROUP_CALC_MIN) {
    values += GRN_RSET_MIN_SIZE;
  }

  return values;
}

int64_t
grn_rset_recinfo_get_sum(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table)
{
  int64_t *sum_address;

  sum_address = (int64_t *)grn_rset_recinfo_get_sum_(ctx, ri, table);
  if (sum_address) {
    return *sum_address;
  } else {
    return 0;
  }
}

double
grn_rset_recinfo_get_sum_float(grn_ctx *ctx,
                               grn_rset_recinfo *ri,
                               grn_obj *table)
{
  double *sum_address;

  sum_address = (double *)grn_rset_recinfo_get_sum_(ctx, ri, table);
  if (sum_address) {
    return *sum_address;
  } else {
    return 0;
  }
}

void
grn_rset_recinfo_set_sum(grn_ctx *ctx,
                         grn_rset_recinfo *ri,
                         grn_obj *table,
                         int64_t sum)
{
  int64_t *sum_address;

  sum_address = (int64_t *)grn_rset_recinfo_get_sum_(ctx, ri, table);
  if (!sum_address) {
    return;
  }

  *sum_address = sum;
}

void
grn_rset_recinfo_set_sum_float(grn_ctx *ctx,
                               grn_rset_recinfo *ri,
                               grn_obj *table,
                               double sum)
{
  double *sum_address;

  sum_address = (double *)grn_rset_recinfo_get_sum_(ctx, ri, table);
  if (!sum_address) {
    return;
  }

  *sum_address = sum;
}

bool
grn_rset_recinfo_is_sum_float(grn_ctx *ctx,
                              grn_rset_recinfo *ri,
                              grn_obj *table)
{
  int64_t *sum_address;

  sum_address = (int64_t *)grn_rset_recinfo_get_sum_(ctx, ri, table);
  if (sum_address) {
    bool *is_float = (bool *)(((int64_t *)sum_address) + 1);
    return *is_float;
  } else {
    return false;
  }
}

double *
grn_rset_recinfo_get_avg_(grn_ctx *ctx,
                          grn_rset_recinfo *ri,
                          grn_obj *table)
{
  grn_table_group_flags flags;
  byte *values;

  flags = DB_OBJ(table)->flags.group;
  if (!(flags & GRN_TABLE_GROUP_CALC_AVG)) {
    return NULL;
  }

  values = (((byte *)ri->subrecs) +
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
  double *avg_address;

  avg_address = grn_rset_recinfo_get_avg_(ctx, ri, table);
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
  double *avg_address;

  avg_address = grn_rset_recinfo_get_avg_(ctx, ri, table);
  if (!avg_address) {
    return;
  }

  *avg_address = avg;
}
