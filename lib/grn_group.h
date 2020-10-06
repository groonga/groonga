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

#pragma once

#include "grn.h"

#ifdef  __cplusplus
extern "C" {
#endif

void grn_group_init_from_env(void);

struct _grn_aggregator_data {
  grn_id group_id;
  grn_id source_id;
  grn_obj *group_table;
  grn_obj *source_table;
  grn_obj *output_column;
  grn_obj *aggregator;
  grn_obj args;
  void *user_data;
};

struct _grn_table_group_aggregator {
  char *output_column_name;
  uint32_t output_column_name_len;
  grn_obj *output_column_type;
  grn_column_flags output_column_flags;
  char *expression;
  uint32_t expression_len;
  grn_obj *aggregator_call;
  grn_obj *aggregator_call_record;
  grn_aggregator_data data;
};

grn_rc
grn_table_group_aggregator_init(grn_ctx *ctx,
                                grn_table_group_aggregator *aggregator);
grn_rc
grn_table_group_aggregator_fin(grn_ctx *ctx,
                               grn_table_group_aggregator *aggregator);

#ifdef __cplusplus
}
#endif
