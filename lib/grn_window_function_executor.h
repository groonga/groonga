/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019-2020  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_window_function.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _grn_window_function_executor {
  grn_obj tag;
  grn_obj tables;
  grn_obj is_context_tables;
  grn_obj source;
  grn_obj sort_keys;
  grn_obj group_keys;
  grn_obj output_column_name;
  struct {
    grn_table_sort_key *sort_keys;
    size_t n_sort_keys;
    grn_table_sort_key *group_keys;
    size_t n_group_keys;
    grn_table_sort_key *window_sort_keys;
    size_t n_window_sort_keys;
    grn_obj *sorted;
  } context;
  struct {
    size_t n;
    grn_obj *previous;
    grn_obj *current;
  } values;
  grn_obj window_function_calls;
  grn_obj output_columns;
  grn_window window;
};

grn_rc
grn_window_function_executor_init(grn_ctx *ctx,
                                  grn_window_function_executor *executor);
grn_rc
grn_window_function_executor_fin(grn_ctx *ctx,
                                 grn_window_function_executor *executor);


#ifdef __cplusplus
}
#endif
