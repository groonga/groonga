/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2020  Sutou Kouhei <kou@clear-code.com>

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

#ifdef  __cplusplus
extern "C" {
#endif

typedef void *(*grn_table_module_open_options_func)(grn_ctx *ctx,
                                                    grn_obj *proc,
                                                    grn_obj *values,
                                                    void *user_data);
/* Deprecated since 8.0.9. Use grn_table_module_option_options_func instead. */
typedef grn_table_module_open_options_func grn_tokenizer_open_options_func;

GRN_API grn_rc
grn_table_set_default_tokenizer_options(grn_ctx *ctx,
                                        grn_obj *table,
                                        grn_obj *options);

GRN_API grn_rc
grn_table_get_default_tokenizer_options(grn_ctx *ctx,
                                        grn_obj *table,
                                        grn_obj *options);

GRN_API void *
grn_table_cache_default_tokenizer_options(grn_ctx *ctx,
                                          grn_obj *table,
                                          grn_table_module_open_options_func open_options_func,
                                          grn_close_func close_options_func,
                                          void *user_data);

GRN_API grn_rc
grn_table_get_default_tokenizer_string(grn_ctx *ctx,
                                       grn_obj *table,
                                       grn_obj *output);


/* Deprecated since 8.0.9. Use grn_table_module_open_options_func instead. */
typedef grn_table_module_open_options_func grn_normalizer_open_options_func;

GRN_API grn_rc
grn_table_set_normalizer_options(grn_ctx *ctx,
                                 grn_obj *table,
                                 grn_obj *options);

GRN_API grn_rc
grn_table_get_normalizer_options(grn_ctx *ctx,
                                 grn_obj *table,
                                 grn_obj *options);

/* TODO: Remove string argument. It's needless. */
GRN_API void *
grn_table_cache_normalizer_options(grn_ctx *ctx,
                                   grn_obj *table,
                                   grn_obj *string,
                                   grn_table_module_open_options_func open_options_func,
                                   grn_close_func close_options_func,
                                   void *user_data);

GRN_API grn_rc
grn_table_get_normalizer_string(grn_ctx *ctx,
                                grn_obj *table,
                                grn_obj *output);


GRN_API grn_rc
grn_table_set_token_filter_options(grn_ctx *ctx,
                                   grn_obj *table,
                                   unsigned int i,
                                   grn_obj *options);

GRN_API grn_rc
grn_table_get_token_filter_options(grn_ctx *ctx,
                                   grn_obj *table,
                                   unsigned int i,
                                   grn_obj *options);

GRN_API void *
grn_table_cache_token_filter_options(grn_ctx *ctx,
                                     grn_obj *table,
                                     unsigned int i,
                                     grn_table_module_open_options_func open_options_func,
                                     grn_close_func close_options_func,
                                     void *user_data);

GRN_API grn_rc
grn_table_get_token_filters_string(grn_ctx *ctx,
                                   grn_obj *table,
                                   grn_obj *output);

#ifdef __cplusplus
}
#endif
