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

grn_rc
grn_selector_run(grn_ctx *ctx,
                 grn_obj *selector,
                 grn_obj *expr,
                 grn_obj *table,
                 grn_obj *index,
                 size_t n_args,
                 grn_obj **args,
                 grn_obj *res,
                 grn_operator op);

grn_rc
grn_selector_data_current_add_score(grn_ctx *ctx,
                                    grn_obj *result_set,
                                    grn_id result_set_record_id,
                                    grn_id record_id,
                                    double score);

#ifdef __cplusplus
}
#endif
