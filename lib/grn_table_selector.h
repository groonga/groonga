/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013-2018  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_db.h"
#include "grn_scanner.h"

#ifdef __cplusplus
extern "C" {
#endif

void
grn_table_selector_init_from_env(void);

typedef struct {
  grn_scanner *scanner;
  grn_obj *expr;
  grn_obj *variable;
  int nth_scan_info;
  scan_info *scan_info;
  grn_obj *initial_result_set;
  grn_obj *result_set;
  grn_id min_id;
  grn_bool is_first_unskipped_scan_info;
  grn_bool is_skipped;
  grn_search_optarg search_options;
} grn_table_selector_data;

struct _grn_table_selector {
  grn_obj *table;
  grn_obj *expr;
  grn_operator op;
  grn_id min_id;
  bool use_sequential_scan;
  grn_obj *query_options;
  grn_table_selector_data data;
};

void
grn_table_selector_init(grn_ctx *ctx,
                        grn_table_selector *selector,
                        grn_obj *table,
                        grn_obj *expr,
                        grn_operator op);
void
grn_table_selector_fin(grn_ctx *ctx,
                       grn_table_selector *selector);

#ifdef __cplusplus
}
#endif
