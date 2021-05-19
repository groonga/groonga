/*
  Copyright(C) 2016-2017 Brazil
  Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>

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

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  grn_obj *table;
  bool is_context_table;
  grn_obj *window_function_call;
  grn_proc *window_function;
  grn_obj *arguments;
  grn_obj *output_column;
  grn_obj *ids;
  ssize_t current_index;
} grn_window_shard;

struct _grn_window {
  grn_window_shard *shards;
  size_t n_shards;
  ssize_t current_shard;
  grn_window_direction direction;
  bool is_sorted;
};

grn_rc grn_window_init(grn_ctx *ctx,
                       grn_window *window);
grn_rc grn_window_fin(grn_ctx *ctx, grn_window *window);
grn_rc grn_window_reset(grn_ctx *ctx, grn_window *window);
grn_rc grn_window_add_record(grn_ctx *ctx,
                             grn_window *window,
                             grn_obj *table,
                             bool is_context_table,
                             grn_id record_id,
                             grn_obj *window_function_call,
                             grn_obj *output_column);
bool grn_window_is_empty(grn_ctx *ctx, grn_window *window);
grn_rc grn_window_set_is_sorted(grn_ctx *ctx,
                                grn_window *window,
                                bool is_sorted);
grn_rc grn_window_execute(grn_ctx *ctx, grn_window *window);

#ifdef __cplusplus
}
#endif
