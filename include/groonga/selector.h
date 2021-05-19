/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2020-2021  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef grn_rc grn_selector_func(grn_ctx *ctx, grn_obj *table, grn_obj *index,
                                 int nargs, grn_obj **args,
                                 grn_obj *res, grn_operator op);

GRN_API grn_rc grn_proc_set_selector(grn_ctx *ctx, grn_obj *proc,
                                     grn_selector_func selector);
GRN_API grn_rc grn_proc_set_selector_operator(grn_ctx *ctx,
                                              grn_obj *proc,
                                              grn_operator selector_op);
GRN_API grn_operator grn_proc_get_selector_operator(grn_ctx *ctx,
                                                    grn_obj *proc);

typedef struct _grn_selector_data grn_selector_data;

GRN_API grn_selector_data *
grn_selector_data_get(grn_ctx *ctx);
GRN_API grn_obj *
grn_selector_data_get_selector(grn_ctx *ctx,
                               grn_selector_data *data);
GRN_API grn_obj *
grn_selector_data_get_expr(grn_ctx *ctx,
                           grn_selector_data *data);
GRN_API grn_obj *
grn_selector_data_get_table(grn_ctx *ctx,
                            grn_selector_data *data);
GRN_API grn_obj *
grn_selector_data_get_index(grn_ctx *ctx,
                            grn_selector_data *data);
GRN_API grn_obj **
grn_selector_data_get_args(grn_ctx *ctx,
                           grn_selector_data *data,
                           size_t *n_args);
GRN_API grn_obj *
grn_selector_data_get_result_set(grn_ctx *ctx,
                                 grn_selector_data *data);
GRN_API grn_operator
grn_selector_data_get_op(grn_ctx *ctx,
                         grn_selector_data *data);


GRN_API grn_rc
grn_selector_data_parse_score_column_option_value(grn_ctx *ctx,
                                                  const char *name,
                                                  grn_obj *value,
                                                  const char *tag,
                                                  void *data);

GRN_API grn_rc
grn_selector_data_parse_tags_option_value(grn_ctx *ctx,
                                          const char *name,
                                          grn_obj *value,
                                          const char *tag,
                                          void *data);

GRN_API grn_rc
grn_selector_data_parse_tags_column_option_value(grn_ctx *ctx,
                                                 const char *name,
                                                 grn_obj *value,
                                                 const char *tag,
                                                 void *data);

#define grn_selector_data_parse_options(ctx,                            \
                                        data,                           \
                                        options,                        \
                                        tag,                            \
                                        ...)                            \
  grn_proc_options_parse((ctx),                                         \
                         (options),                                     \
                         (tag),                                         \
                         "score_column",                                \
                         GRN_PROC_OPTION_VALUE_FUNC,                    \
                         grn_selector_data_parse_score_column_option_value, \
                         (data),                                        \
                         "tags",                                        \
                         GRN_PROC_OPTION_VALUE_FUNC,                    \
                         grn_selector_data_parse_tags_option_value,     \
                         (data),                                        \
                         "tags_column",                                 \
                         GRN_PROC_OPTION_VALUE_FUNC,                    \
                         grn_selector_data_parse_tags_column_option_value, \
                         (data),                                        \
                         __VA_ARGS__)

GRN_API bool
grn_selector_data_have_score_column(grn_ctx *ctx,
                                    grn_selector_data *data);

GRN_API bool
grn_selector_data_have_tags_column(grn_ctx *ctx,
                                   grn_selector_data *data);

GRN_API grn_rc
grn_selector_data_on_token_found(grn_ctx *ctx,
                                 grn_selector_data *data,
                                 grn_obj *index,
                                 grn_id token_id,
                                 double additional_score);

#ifdef __cplusplus
}
#endif
