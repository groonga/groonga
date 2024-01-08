/*
  Copyright (C) 2013-2018  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>

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

/* Don't use this header directly as much as possible! */

#pragma once

#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_SCAN_INFO_INITIAL_MAX_N_ARGS 16

struct _grn_scan_info {
  uint32_t start;
  uint32_t end;
  int32_t nargs;
  int flags;
  grn_operator op;
  grn_operator logical_op;
  float weight_factor;
  grn_obj sections;
  grn_obj weights;
  grn_obj index;
  grn_obj *query;
  grn_obj **args;
  grn_obj *initial_args[GRN_SCAN_INFO_INITIAL_MAX_N_ARGS];
  int max_interval;
  int additional_last_interval;
  grn_obj max_element_intervals;
  int min_interval;
  int similarity_threshold;
  int quorum_threshold;
  grn_obj scorers;
  grn_obj scorer_args_exprs;
  grn_obj scorer_args_expr_offsets;
  grn_obj start_positions;
  int32_t max_nargs;
};

#ifdef __cplusplus
}
#endif
