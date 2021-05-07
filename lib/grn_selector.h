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
#include "grn_ctx_impl.h"

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
grn_selector_data_set_default_tags(grn_ctx *ctx,
                                   grn_selector_data *data,
                                   grn_obj *tags);

grn_rc
grn_selector_data_current_set_default_tag_raw_no_validation(grn_ctx *ctx,
                                                            const char *tag,
                                                            uint32_t tag_length);

static grn_inline grn_rc
grn_selector_data_current_set_default_tag_raw(grn_ctx *ctx,
                                              const char *tag,
                                              uint32_t tag_length)
{
  grn_selector_data *data = ctx->impl->current_selector_data;
  if (!data) {
    return ctx->rc;
  }

  return grn_selector_data_current_set_default_tag_raw_no_validation(ctx,
                                                                     tag,
                                                                     tag_length);
}

grn_rc
grn_selector_data_current_add_score_no_validation(grn_ctx *ctx,
                                                  grn_obj *result_set,
                                                  grn_id result_set_record_id,
                                                  grn_id record_id,
                                                  double score);

static grn_inline grn_rc
grn_selector_data_current_add_score(grn_ctx *ctx,
                                    grn_obj *result_set,
                                    grn_id result_set_record_id,
                                    grn_id record_id,
                                    double score)
{
  grn_selector_data *data = ctx->impl->current_selector_data;
  if (!data) {
    return ctx->rc;
  }

  return grn_selector_data_current_add_score_no_validation(ctx,
                                                           result_set,
                                                           result_set_record_id,
                                                           record_id,
                                                           score);
}

#ifdef __cplusplus
}
#endif
