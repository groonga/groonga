/*
  Copyright (C) 2018  Brazil
  Copyright (C) 2019-2024  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

double grn_table_get_score(grn_ctx *ctx,
                           grn_obj *table,
                           grn_id id);

grn_obj *
grn_table_create_with_max_n_subrecs(grn_ctx *ctx,
                                    const char *name,
                                    unsigned int name_size,
                                    const char *path,
                                    grn_table_flags flags,
                                    grn_obj *key_type,
                                    grn_obj *value_type,
                                    uint32_t max_n_subrecs,
                                    uint32_t additional_value_size);

grn_rc
grn_table_setoperation_with_weight_factor(grn_ctx *ctx,
                                          grn_obj *table1,
                                          grn_obj *table2,
                                          grn_obj *res,
                                          grn_operator op,
                                          float weight_factor);

grn_id
grn_table_find_reference_object_raw(grn_ctx *ctx, grn_id table_id);

grn_hash *
grn_table_all_columns(grn_ctx *ctx, grn_obj *table);

#define GRN_TABLE_LOCK_BEGIN(ctx, table) do {                           \
  grn_io *io_ = grn_obj_get_io(ctx, table);                             \
  bool locked_ = false;                                                 \
  bool can_run_ = true;                                                 \
  if (io_ && !(io_->flags & GRN_IO_TEMPORARY)) {                        \
    if (grn_io_lock(ctx, io_, grn_lock_timeout) == GRN_SUCCESS) {       \
      locked_ = true;                                                   \
    } else {                                                            \
      can_run_ = false;                                                 \
    }                                                                   \
  }                                                                     \
  if (can_run_) {
#define GRN_TABLE_LOCK_END(ctx)                 \
  }                                             \
  if (locked_) {                                \
    grn_io_unlock(ctx, io_);                    \
  }                                             \
} while (false)

#ifdef __cplusplus
}
#endif
