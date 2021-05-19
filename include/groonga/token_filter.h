/*
  Copyright(C) 2014-2016 Brazil
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#include <groonga/tokenizer.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* Deprecated since 8.0.9. Use grn_token_filter_init_query_func instead. */
typedef void *grn_token_filter_init_func(grn_ctx *ctx,
                                         grn_obj *table,
                                         grn_tokenize_mode mode);

typedef void *grn_token_filter_init_query_func(grn_ctx *ctx,
                                               grn_tokenizer_query *query);

typedef void grn_token_filter_filter_func(grn_ctx *ctx,
                                          grn_token *current_token,
                                          grn_token *next_token,
                                          void *user_data);

typedef void grn_token_filter_fin_func(grn_ctx *ctx,
                                       void *user_data);


/*
  grn_token_filter_register() registers a plugin to the database which is
  associated with `ctx'. `plugin_name_ptr' and `plugin_name_length' specify the
  plugin name. Alphabetic letters ('A'-'Z' and 'a'-'z'), digits ('0'-'9') and
  an underscore ('_') are capable characters.

  `init', `filter' and `fin' specify the plugin functions.

  `init' is called for initializing a token_filter for a document or
  query.

  `filter' is called for filtering tokens one by one.

  `fin' is called for finalizing a token_filter.

  grn_token_filter_register() returns GRN_SUCCESS on success, an error
  code on failure.

  Deprecated since 8.0.9. Use grn_token_filter_create() and
  grn_token_filter_set_XXX_func() instead.
 */
GRN_PLUGIN_EXPORT grn_rc grn_token_filter_register(grn_ctx *ctx,
                                                   const char *plugin_name_ptr,
                                                   int plugin_name_length,
                                                   grn_token_filter_init_func *init,
                                                   grn_token_filter_filter_func *filter,
                                                   grn_token_filter_fin_func *fin);

GRN_PLUGIN_EXPORT grn_obj *
grn_token_filter_create(grn_ctx *ctx, const char *name, int name_length);

GRN_PLUGIN_EXPORT grn_rc
grn_token_filter_set_init_func(grn_ctx *ctx,
                               grn_obj *token_filter,
                               grn_token_filter_init_query_func *init);
GRN_PLUGIN_EXPORT grn_rc
grn_token_filter_set_filter_func(grn_ctx *ctx,
                                 grn_obj *token_filter,
                                 grn_token_filter_filter_func *filter);
GRN_PLUGIN_EXPORT grn_rc
grn_token_filter_set_fin_func(grn_ctx *ctx,
                              grn_obj *token_filter,
                              grn_token_filter_fin_func *fin);

#ifdef __cplusplus
}  /* extern "C" */
#endif  /* __cplusplus */
