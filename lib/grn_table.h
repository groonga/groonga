/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018 Brazil
  Copyright(C) 2019 Sutou Kouhei <kou@clear-code.com>

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

#pragma once

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  grn_obj *proc;
  void *options;
  grn_option_revision options_revision;
  grn_close_func options_close_func;
  grn_critical_section lock;
} grn_table_module;

void grn_table_module_init(grn_ctx *ctx,
                           grn_table_module *module,
                           grn_id module_id);
void grn_table_module_set_proc(grn_ctx *ctx,
                               grn_table_module *module,
                               grn_obj *proc);
void grn_table_module_set_options(grn_ctx *ctx,
                                  grn_table_module *module,
                                  void *options,
                                  grn_option_revision revision,
                                  grn_close_func close_func);
void grn_table_module_fin(grn_ctx *ctx,
                          grn_table_module *module);

double grn_table_get_score(grn_ctx *ctx,
                           grn_obj *table,
                           grn_id id);

#define GRN_TABLE_LOCK_BEGIN(ctx, table) do {                           \
  grn_io *io_ = grn_obj_get_io(ctx, table);                             \
  grn_bool locked_ = GRN_FALSE;                                         \
  grn_bool can_run_ = GRN_TRUE;                                         \
  if (io_ && !(io_->flags & GRN_IO_TEMPORARY)) {                        \
    if (grn_io_lock(ctx, io_, grn_lock_timeout) == GRN_SUCCESS) {       \
      locked_ = GRN_TRUE;                                               \
    } else {                                                            \
      can_run_ = GRN_FALSE;                                             \
    }                                                                   \
  }                                                                     \
  if (can_run_) {
#define GRN_TABLE_LOCK_END(ctx, table)          \
  }                                             \
  if (locked_) {                                \
    grn_io_unlock(io_);                         \
  }                                             \
} while (GRN_FALSE)

#ifdef __cplusplus
}
#endif
