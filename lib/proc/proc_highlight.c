/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2020-2023  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_proc.h"
#include "../grn_expr.h"

#include <groonga/plugin.h>
#include <string.h>

#define GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME "$highlight_html"

static bool
highlight_validate_text_arg(grn_ctx *ctx,
                            grn_obj *arg,
                            const char *function_name,
                            const char *label)
{
  if (grn_obj_is_text_family_bulk(ctx, arg)) {
    return true;
  }

  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, arg);
  GRN_PLUGIN_ERROR(ctx,
                   GRN_INVALID_ARGUMENT,
                   "%s(): %s must be text: %.*s",
                   function_name,
                   label,
                   (int)GRN_TEXT_LEN(&inspected),
                   GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);

  return false;
}

static grn_obj *
func_highlight(grn_ctx *ctx,
               int nargs,
               grn_obj **args,
               grn_user_data *user_data)
{
  const char *tag = "[highlight]";
  const char *function_name = "highlight";
  grn_obj *highlighted = NULL;

#define N_REQUIRED_ARGS 1
  if (nargs > N_REQUIRED_ARGS) {
    grn_obj *string = args[0];
    grn_obj *end_arg = args[nargs - 1];
    int n_args_without_option = nargs;

    grn_obj cache_key;
    GRN_TEXT_INIT(&cache_key, 0);
    grn_proc_func_generate_cache_key(ctx,
                                     function_name,
                                     args + 1,
                                     nargs - 1,
                                     &cache_key);

    grn_obj *expression;
    grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);

    grn_highlighter *highlighter = NULL;
    grn_obj *highlighter_ptr = NULL;
    if (GRN_TEXT_LEN(&cache_key) <= GRN_TABLE_MAX_KEY_SIZE) {
      highlighter_ptr =
        grn_expr_get_var(ctx,
                         expression,
                         GRN_TEXT_VALUE(&cache_key),
                         (unsigned int)GRN_TEXT_LEN(&cache_key));
      if (highlighter_ptr) {
        highlighter = (grn_highlighter *)GRN_PTR_VALUE(highlighter_ptr);
      } else {
        highlighter_ptr =
          grn_expr_get_or_add_var(ctx,
                                  expression,
                                  GRN_TEXT_VALUE(&cache_key),
                                  (unsigned int)GRN_TEXT_LEN(&cache_key));
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
        GRN_OBJ_FIN(ctx, highlighter_ptr);
        GRN_PTR_INIT(highlighter_ptr, GRN_OBJ_OWN, GRN_DB_OBJECT);
      }
    }
    GRN_OBJ_FIN(ctx, &cache_key);

    if (!highlighter) {
      highlighter = grn_highlighter_open(ctx);
      if (highlighter_ptr) {
        GRN_PTR_SET(ctx, highlighter_ptr, highlighter);
      }

      grn_highlighter_set_html_mode(ctx, highlighter, false);

      bool have_default_tag = false;

      if (end_arg->header.type == GRN_TABLE_HASH_KEY) {
        n_args_without_option--;

        grn_raw_string normalizers;
        GRN_RAW_STRING_SET_CSTRING(normalizers, "NormalizerAuto");
        bool html_escape = false;
        grn_raw_string default_open_tag = {NULL, 0};
        grn_raw_string default_close_tag = {NULL, 0};
        grn_proc_options_parse(ctx,
                               end_arg,
                               tag,
                               "normalizer",
                               GRN_PROC_OPTION_VALUE_RAW_STRING,
                               &normalizers,
                               "normalizers",
                               GRN_PROC_OPTION_VALUE_RAW_STRING,
                               &normalizers,
                               "html_escape",
                               GRN_PROC_OPTION_VALUE_BOOL,
                               &html_escape,
                               "default_open_tag",
                               GRN_PROC_OPTION_VALUE_RAW_STRING,
                               &default_open_tag,
                               "default_close_tag",
                               GRN_PROC_OPTION_VALUE_RAW_STRING,
                               &default_close_tag,
                               NULL);
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }

        grn_highlighter_set_normalizers(ctx,
                                        highlighter,
                                        normalizers.value,
                                        normalizers.length);
        grn_highlighter_set_html_mode(ctx, highlighter, html_escape);

        if (default_open_tag.length > 0) {
          have_default_tag = true;
          grn_highlighter_set_default_open_tag(ctx,
                                               highlighter,
                                               default_open_tag.value,
                                               default_open_tag.length);
        }

        if (default_close_tag.length > 0) {
          have_default_tag = true;
          grn_highlighter_set_default_close_tag(ctx,
                                                highlighter,
                                                default_close_tag.value,
                                                default_close_tag.length);
        }
      }

      grn_obj **keyword_args = args + N_REQUIRED_ARGS;
      uint32_t n_keyword_args =
        (uint32_t)(n_args_without_option - N_REQUIRED_ARGS);
      if (have_default_tag) {
        uint32_t i;
        for (i = 0; i < n_keyword_args; i++) {
          grn_obj *keyword = keyword_args[i];
          if (!highlight_validate_text_arg(ctx,
                                           keyword,
                                           function_name,
                                           "keyword")) {
            goto exit;
          }
          grn_highlighter_add_keyword(ctx,
                                      highlighter,
                                      GRN_TEXT_VALUE(keyword),
                                      GRN_TEXT_LEN(keyword));
        }
      } else {
        uint32_t i;
        for (i = 0; i < n_keyword_args; i += 3) {
          grn_obj *keyword = keyword_args[i];
          grn_obj *open_tag = keyword_args[i + 1];
          grn_obj *close_tag = keyword_args[i + 2];
          if (!highlight_validate_text_arg(ctx,
                                           keyword,
                                           function_name,
                                           "keyword")) {
            goto exit;
          }
          if (!highlight_validate_text_arg(ctx,
                                           open_tag,
                                           function_name,
                                           "open tag")) {
            goto exit;
          }
          if (!highlight_validate_text_arg(ctx,
                                           close_tag,
                                           function_name,
                                           "close tag")) {
            goto exit;
          }
          grn_highlighter_add_open_tag(ctx,
                                       highlighter,
                                       GRN_TEXT_VALUE(open_tag),
                                       GRN_TEXT_LEN(open_tag));
          grn_highlighter_add_close_tag(ctx,
                                        highlighter,
                                        GRN_TEXT_VALUE(close_tag),
                                        GRN_TEXT_LEN(close_tag));
          grn_highlighter_add_keyword(ctx,
                                      highlighter,
                                      GRN_TEXT_VALUE(keyword),
                                      GRN_TEXT_LEN(keyword));
        }
      }
    }

    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);
    grn_highlighter_highlight(ctx,
                              highlighter,
                              GRN_TEXT_VALUE(string),
                              (int64_t)GRN_TEXT_LEN(string),
                              highlighted);
  }
#undef N_REQUIRED_ARGS

exit:
  if (!highlighted) {
    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  return highlighted;
}

void
grn_proc_init_highlight(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "highlight",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_highlight,
                  NULL,
                  NULL,
                  0,
                  NULL);
}

static grn_obj *
func_highlight_full(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  const char *function_name = "highlight_full";
  grn_obj *highlighted = NULL;

#define N_REQUIRED_ARGS  3
#define KEYWORD_SET_SIZE 3
  if ((nargs >= (N_REQUIRED_ARGS + KEYWORD_SET_SIZE) &&
       (nargs - N_REQUIRED_ARGS) % KEYWORD_SET_SIZE == 0)) {
    grn_obj *string = args[0];

    grn_obj cache_key;
    GRN_TEXT_INIT(&cache_key, 0);
    grn_proc_func_generate_cache_key(ctx,
                                     function_name,
                                     args + 1,
                                     nargs - 1,
                                     &cache_key);

    grn_obj *expression;
    grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);

    grn_highlighter *highlighter = NULL;
    grn_obj *highlighter_ptr = NULL;
    if (GRN_TEXT_LEN(&cache_key) <= GRN_TABLE_MAX_KEY_SIZE) {
      highlighter_ptr =
        grn_expr_get_var(ctx,
                         expression,
                         GRN_TEXT_VALUE(&cache_key),
                         (unsigned int)GRN_TEXT_LEN(&cache_key));
      if (highlighter_ptr) {
        highlighter = (grn_highlighter *)GRN_PTR_VALUE(highlighter_ptr);
      } else {
        highlighter_ptr =
          grn_expr_get_or_add_var(ctx,
                                  expression,
                                  GRN_TEXT_VALUE(&cache_key),
                                  (unsigned int)GRN_TEXT_LEN(&cache_key));
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
        GRN_OBJ_FIN(ctx, highlighter_ptr);
        GRN_PTR_INIT(highlighter_ptr, GRN_OBJ_OWN, GRN_DB_OBJECT);
      }
    }
    GRN_OBJ_FIN(ctx, &cache_key);

    if (!highlighter) {
      grn_obj *normalizers = args[1];
      bool html_mode = GRN_BOOL_VALUE(args[2]);

      highlighter = grn_highlighter_open(ctx);
      if (highlighter_ptr) {
        GRN_PTR_SET(ctx, highlighter_ptr, highlighter);
      }

      grn_highlighter_set_normalizers(ctx,
                                      highlighter,
                                      GRN_TEXT_VALUE(normalizers),
                                      GRN_TEXT_LEN(normalizers));
      grn_highlighter_set_html_mode(ctx, highlighter, html_mode);

      grn_obj **keyword_set_args = args + N_REQUIRED_ARGS;
      int n_keyword_args = nargs - N_REQUIRED_ARGS;
      int n_keyword_sets = n_keyword_args / KEYWORD_SET_SIZE;

      int i;
      for (i = 0; i < n_keyword_sets; i++) {
        grn_obj *keyword = keyword_set_args[i * KEYWORD_SET_SIZE + 0];
        grn_obj *open_tag = keyword_set_args[i * KEYWORD_SET_SIZE + 1];
        grn_obj *close_tag = keyword_set_args[i * KEYWORD_SET_SIZE + 2];

        if (!highlight_validate_text_arg(ctx,
                                         keyword,
                                         function_name,
                                         "keyword")) {
          goto exit;
        }
        if (!highlight_validate_text_arg(ctx,
                                         open_tag,
                                         function_name,
                                         "open tag")) {
          goto exit;
        }
        if (!highlight_validate_text_arg(ctx,
                                         close_tag,
                                         function_name,
                                         "close tag")) {
          goto exit;
        }

        grn_highlighter_add_keyword(ctx,
                                    highlighter,
                                    GRN_TEXT_VALUE(keyword),
                                    GRN_TEXT_LEN(keyword));
        grn_highlighter_add_open_tag(ctx,
                                     highlighter,
                                     GRN_TEXT_VALUE(open_tag),
                                     GRN_TEXT_LEN(open_tag));
        grn_highlighter_add_close_tag(ctx,
                                      highlighter,
                                      GRN_TEXT_VALUE(close_tag),
                                      GRN_TEXT_LEN(close_tag));
      }
    }

    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);
    grn_highlighter_highlight(ctx,
                              highlighter,
                              GRN_TEXT_VALUE(string),
                              (int64_t)GRN_TEXT_LEN(string),
                              highlighted);
  }
#undef KEYWORD_SET_SIZE
#undef N_REQUIRED_ARGS

exit:
  if (!highlighted) {
    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  return highlighted;
}

void
grn_proc_init_highlight_full(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "highlight_full",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_highlight_full,
                  NULL,
                  NULL,
                  0,
                  NULL);
}

static grn_highlighter *
func_highlight_html_create_highlighter(grn_ctx *ctx, grn_obj *expression)
{
  grn_highlighter *highlighter;
  grn_obj *condition = NULL;

  highlighter = grn_highlighter_open(ctx);

  condition = grn_expr_get_condition(ctx, expression);

  for (; condition; condition = grn_expr_get_parent(ctx, condition)) {
    uint32_t i, n_keywords;
    grn_obj current_keywords;
    GRN_TEXT_INIT(&current_keywords, GRN_OBJ_VECTOR);
    grn_expr_get_keywords(ctx, condition, &current_keywords);

    n_keywords = grn_vector_size(ctx, &current_keywords);
    for (i = 0; i < n_keywords; i++) {
      const char *keyword;
      unsigned int keyword_size;
      keyword_size =
        grn_vector_get_element(ctx, &current_keywords, i, &keyword, NULL, NULL);
      grn_highlighter_add_keyword(ctx, highlighter, keyword, keyword_size);
    }
    GRN_OBJ_FIN(ctx, &current_keywords);
  }

  return highlighter;
}

static grn_obj *
func_highlight_html(grn_ctx *ctx,
                    int nargs,
                    grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *highlighted = NULL;
  grn_obj *string;
  grn_obj *lexicon = NULL;
  grn_obj *expression = NULL;
  grn_highlighter *highlighter;
  grn_obj *highlighter_ptr;

  if (!(1 <= nargs && nargs <= 2)) {
    GRN_PLUGIN_ERROR(
      ctx,
      GRN_INVALID_ARGUMENT,
      "highlight_html(): wrong number of arguments (%d for 1..2)",
      nargs);
    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
    return highlighted;
  }

  string = args[0];
  if (nargs == 2) {
    lexicon = args[1];
  }

  grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);

  highlighter_ptr =
    grn_expr_get_var(ctx,
                     expression,
                     GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME,
                     strlen(GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME));
  if (highlighter_ptr) {
    highlighter = (grn_highlighter *)GRN_PTR_VALUE(highlighter_ptr);
  } else {
    highlighter_ptr =
      grn_expr_get_or_add_var(ctx,
                              expression,
                              GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME,
                              strlen(GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME));
    GRN_OBJ_FIN(ctx, highlighter_ptr);
    GRN_PTR_INIT(highlighter_ptr, GRN_OBJ_OWN, GRN_DB_OBJECT);

    highlighter = func_highlight_html_create_highlighter(ctx, expression);
    GRN_PTR_SET(ctx, highlighter_ptr, highlighter);

    grn_highlighter_set_lexicon(ctx, highlighter, lexicon);
  }

  highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);
  grn_highlighter_highlight(ctx,
                            highlighter,
                            GRN_TEXT_VALUE(string),
                            (int64_t)GRN_TEXT_LEN(string),
                            highlighted);

  return highlighted;
}

void
grn_proc_init_highlight_html(grn_ctx *ctx)
{
  grn_proc_create(ctx,
                  "highlight_html",
                  -1,
                  GRN_PROC_FUNCTION,
                  func_highlight_html,
                  NULL,
                  NULL,
                  0,
                  NULL);
}
