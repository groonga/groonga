/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2020-2023  Sutou Kouhei <kou@clear-code.com>

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

typedef enum {
  GRN_PROC_INVALID = 0,
  GRN_PROC_TOKENIZER,
  GRN_PROC_COMMAND,
  GRN_PROC_FUNCTION,
  GRN_PROC_HOOK,
  GRN_PROC_NORMALIZER,
  GRN_PROC_TOKEN_FILTER,
  GRN_PROC_SCORER,
  GRN_PROC_WINDOW_FUNCTION,
  GRN_PROC_AGGREGATOR,
} grn_proc_type;

GRN_API grn_obj *
grn_proc_create(grn_ctx *ctx,
                const char *name,
                int name_size,
                grn_proc_type type,
                grn_proc_func *init,
                grn_proc_func *next,
                grn_proc_func *fin,
                unsigned int nvars,
                grn_expr_var *vars);
GRN_API grn_obj *
grn_proc_get_info(grn_ctx *ctx,
                  grn_user_data *user_data,
                  grn_expr_var **vars,
                  unsigned int *nvars,
                  grn_obj **caller);
GRN_API grn_proc_type
grn_proc_get_type(grn_ctx *ctx, grn_obj *proc);

typedef grn_rc (*grn_proc_option_value_parse_func)(grn_ctx *ctx,
                                                   const char *name,
                                                   grn_obj *value,
                                                   const char *tag,
                                                   void *user_data);

typedef enum {
  GRN_PROC_OPTION_VALUE_RAW,
  GRN_PROC_OPTION_VALUE_MODE,
  GRN_PROC_OPTION_VALUE_OPERATOR,
  GRN_PROC_OPTION_VALUE_EXPR_FLAGS,
  GRN_PROC_OPTION_VALUE_INT32,
  GRN_PROC_OPTION_VALUE_UINT32,
  GRN_PROC_OPTION_VALUE_INT64,
  GRN_PROC_OPTION_VALUE_BOOL,
  GRN_PROC_OPTION_VALUE_FUNC,
  GRN_PROC_OPTION_VALUE_TOKENIZE_MODE,
  GRN_PROC_OPTION_VALUE_TOKEN_CURSOR_FLAGS,
  GRN_PROC_OPTION_VALUE_DOUBLE,
  GRN_PROC_OPTION_VALUE_RAW_STRING,
} grn_proc_option_value_type;

GRN_API grn_rc
grn_proc_prefixed_options_parse(grn_ctx *ctx,
                                grn_obj *options,
                                const char *prefix,
                                const char *tag,
                                const char *name,
                                ...);

GRN_API grn_rc
grn_proc_prefixed_options_parsev(grn_ctx *ctx,
                                 grn_obj *options,
                                 const char *prefix,
                                 const char *tag,
                                 const char *name,
                                 va_list args);

GRN_API grn_rc
grn_proc_options_parse(
  grn_ctx *ctx, grn_obj *options, const char *tag, const char *name, ...);

GRN_API grn_rc
grn_proc_options_parsev(grn_ctx *ctx,
                        grn_obj *options,
                        const char *tag,
                        const char *name,
                        va_list args);

GRN_API grn_rc
grn_proc_func_generate_cache_key(grn_ctx *ctx,
                                 const char *function_name,
                                 grn_obj **args,
                                 int n_args,
                                 grn_obj *cache_key);

#ifdef __cplusplus
}
#endif
