/*
  Copyright (C) 2020-2024  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_ctx_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _grn_selector_data {
  grn_obj *selector;
  grn_obj *expr;
  grn_obj *table;
  grn_obj *index;
  size_t n_args;
  grn_obj **args;
  float weight_factor;
  grn_obj *result_set;
  grn_operator op;
  grn_obj *score_table;
  grn_obj *score_column;
  grn_obj score;
  grn_obj *tags_table;
  grn_obj *tags_column;
  grn_obj tags;
  grn_obj default_tags;
  bool can_swap_result_set;
};

grn_rc
grn_selector_run(grn_ctx *ctx, grn_selector_data *data);

grn_rc
grn_selector_data_init(grn_ctx *ctx,
                       grn_selector_data *data,
                       grn_obj *selector,
                       grn_obj *expr,
                       grn_obj *table,
                       grn_obj *index,
                       size_t n_args,
                       grn_obj **args,
                       float weight_factor,
                       grn_obj *result_set,
                       grn_operator op);
grn_rc
grn_selector_data_fin(grn_ctx *ctx, grn_selector_data *data);

grn_rc
grn_selector_data_set_can_swap_result_set(grn_ctx *ctx,
                                          grn_selector_data *data,
                                          bool can);

grn_rc
grn_selector_data_set_default_tags(grn_ctx *ctx,
                                   grn_selector_data *data,
                                   grn_obj *tags);

grn_rc
grn_selector_data_current_set_default_tag_raw_no_validation(
  grn_ctx *ctx, const char *tag, uint32_t tag_length);

static inline grn_rc
grn_selector_data_current_set_default_tag_raw(grn_ctx *ctx,
                                              const char *tag,
                                              uint32_t tag_length)
{
  grn_selector_data *data = ctx->impl->current_selector_data;
  if (!data) {
    return ctx->rc;
  }

  return grn_selector_data_current_set_default_tag_raw_no_validation(
    ctx,
    tag,
    tag_length);
}

grn_rc
grn_selector_data_current_add_score_no_validation(grn_ctx *ctx,
                                                  grn_obj *result_set,
                                                  grn_id result_set_record_id,
                                                  grn_id record_id,
                                                  double score);

static inline grn_rc
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
