/*
  Copyright (C) 2009-2016  Brazil
  Copyright (C) 2019-2022  Sutou Kouhei <kou@clear-code.com>

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

#define GRN_FUNC_SNIPPET_HTML_CACHE_NAME "$snippet_html"

static grn_obj *
snippet_exec(grn_ctx *ctx, grn_obj *snip, grn_obj *target,
             grn_obj *default_return_value,
             grn_user_data *user_data,
             const char *prefix, int prefix_length,
             const char *suffix, int suffix_length)
{
  uint32_t n_texts = 0;
  bool is_vector = false;
  if (grn_obj_is_text_family_vector(ctx, target)) {
    is_vector = true;
    n_texts = grn_vector_size(ctx, target);
    if (n_texts == 0) {
      return NULL;
    }
  } else if (grn_obj_is_text_family_bulk(ctx, target)) {
    if (GRN_TEXT_LEN(target) == 0) {
      return NULL;
    }
    n_texts = 1;
  } else {
    return NULL;
  }

  grn_obj *snippets = grn_plugin_proc_alloc(ctx,
                                            user_data,
                                            GRN_DB_SHORT_TEXT,
                                            GRN_OBJ_VECTOR);
  if (!snippets) {
    return NULL;
  }

  grn_obj snippet_buffer;
  GRN_TEXT_INIT(&snippet_buffer, 0);

  grn_rc rc = GRN_SUCCESS;
  uint32_t i;
  for (i = 0; i < n_texts; i++) {
    const char *text;
    uint32_t text_length;
    if (is_vector) {
      grn_id domain;
      text_length = grn_vector_get_element_float(ctx,
                                                 target,
                                                 i,
                                                 &text,
                                                 NULL,
                                                 &domain);
      if (!grn_type_id_is_text_family(ctx, domain)) {
        continue;
      }
      if (text_length == 0) {
        continue;
      }
    } else {
      text = GRN_TEXT_VALUE(target);
      text_length = (uint32_t)GRN_TEXT_LEN(target);
    }

    unsigned int i, n_results, max_tagged_length;
    rc = grn_snip_exec(ctx, snip,
                       text, text_length,
                       &n_results, &max_tagged_length);
    if (rc != GRN_SUCCESS) {
      break;
    }

    if (n_results == 0) {
      continue;
    }

    grn_bulk_space(ctx, &snippet_buffer,
                   (size_t)prefix_length +
                   max_tagged_length +
                   (size_t)suffix_length);
    for (i = 0; i < n_results; i++) {
      unsigned int snippet_length;

      GRN_BULK_REWIND(&snippet_buffer);
      if (prefix_length > 0) {
        GRN_TEXT_PUT(ctx, &snippet_buffer, prefix, prefix_length);
      }
      rc = grn_snip_get_result(ctx, snip, i,
                               GRN_TEXT_VALUE(&snippet_buffer) + prefix_length,
                               &snippet_length);
      if (rc != GRN_SUCCESS) {
        continue;
      }
      grn_strncat(GRN_TEXT_VALUE(&snippet_buffer),
                  GRN_BULK_WSIZE(&snippet_buffer),
                  suffix,
                  (size_t)suffix_length);
      grn_vector_add_element(ctx, snippets,
                             GRN_TEXT_VALUE(&snippet_buffer),
                             (uint32_t)prefix_length +
                             snippet_length +
                             (uint32_t)suffix_length,
                             0, GRN_DB_SHORT_TEXT);
    }
  }
  GRN_OBJ_FIN(ctx, &snippet_buffer);

  if (rc != GRN_SUCCESS) {
    return NULL;
  }

  if (grn_vector_size(ctx, snippets) == 0) {
    if (default_return_value) {
      return default_return_value;
    } else {
      return grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
    }
  }

  return snippets;
}

static grn_obj *
func_snippet(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  const char *tag = "[snippet]";
  grn_obj *snippets = NULL;
  grn_obj *snip = NULL;
  grn_obj cache_key;
  GRN_TEXT_INIT(&cache_key, 0);

#define N_REQUIRED_ARGS 1
#define KEYWORD_SET_SIZE 3
  if (nargs < N_REQUIRED_ARGS) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for %d..)",
                     tag,
                     nargs,
                     N_REQUIRED_ARGS);
    goto exit;
  }

  grn_obj *target = args[0];
  grn_proc_func_generate_cache_key(ctx, "snippet", args + 1, nargs - 1, &cache_key);

  grn_obj *snip_ptr = NULL;
  grn_obj *expression;
  grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);
  if (GRN_TEXT_LEN(&cache_key) <= GRN_TABLE_MAX_KEY_SIZE) {
    snip_ptr = grn_expr_get_var(ctx,
                                expression,
                                GRN_TEXT_VALUE(&cache_key),
                                GRN_TEXT_LEN(&cache_key));
    if (snip_ptr) {
      snip = GRN_PTR_VALUE(snip_ptr);
    } else {
      snip_ptr = grn_expr_get_or_add_var(ctx,
                                         expression,
                                         GRN_TEXT_VALUE(&cache_key),
                                         GRN_TEXT_LEN(&cache_key));
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
      GRN_OBJ_FIN(ctx, snip_ptr);
      GRN_PTR_INIT(snip_ptr, GRN_OBJ_OWN, GRN_DB_OBJECT);
    }
  }

  grn_obj *end_arg = args[nargs - 1];
  int64_t width = 200;
  int64_t max_n_results = 3;
  bool skip_leading_spaces = true;
  bool html_escape = false;
  grn_obj *prefix = NULL;
  grn_obj *suffix = NULL;
  grn_obj *normalizer_name = NULL;
  grn_obj *default_open_tag = NULL;
  grn_obj *default_close_tag = NULL;
  int n_args_without_option = nargs;
  grn_obj *default_return_value = NULL;
  grn_obj *delimiter_regexp = NULL;

  if (end_arg->header.type == GRN_TABLE_HASH_KEY) {
    grn_obj *options = end_arg;
    n_args_without_option--;
    grn_rc rc = grn_proc_options_parse(ctx,
                                       options,
                                       tag,
                                       "width",
                                       GRN_PROC_OPTION_VALUE_INT64,
                                       &width,
                                       "max_n_results",
                                       GRN_PROC_OPTION_VALUE_INT64,
                                       &max_n_results,
                                       "skip_leading_spaces",
                                       GRN_PROC_OPTION_VALUE_BOOL,
                                       &skip_leading_spaces,
                                       "html_escape",
                                       GRN_PROC_OPTION_VALUE_BOOL,
                                       &html_escape,
                                       "prefix",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &prefix,
                                       "suffix",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &suffix,
                                       "normalizer",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &normalizer_name,
                                       "default_open_tag",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &default_open_tag,
                                       "default_close_tag",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &default_close_tag,
                                       "default",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &default_return_value,
                                       "delimiter_regexp",
                                       GRN_PROC_OPTION_VALUE_RAW,
                                       &delimiter_regexp,
                                       NULL);
    if (rc != GRN_SUCCESS) {
      goto exit;
    }
  }

  if (default_return_value && default_return_value->header.type == GRN_PTR) {
    default_return_value = GRN_PTR_VALUE(default_return_value);
  }

  if (!snip) {
    int flags = 0;
    if (skip_leading_spaces) {
      flags |= GRN_SNIP_SKIP_LEADING_SPACES;
    }
    grn_snip_mapping *mapping = NULL;
    if (html_escape) {
      mapping = GRN_SNIP_MAPPING_HTML_ESCAPE;
    }
    snip = grn_snip_open(ctx,
                         flags,
                         width,
                         max_n_results,
                         default_open_tag ? GRN_TEXT_VALUE(default_open_tag) : NULL,
                         default_open_tag ? GRN_TEXT_LEN(default_open_tag) : 0,
                         default_close_tag ? GRN_TEXT_VALUE(default_close_tag) : NULL,
                         default_close_tag ? GRN_TEXT_LEN(default_close_tag) : 0,
                         mapping);
    if (!snip) {
      goto exit;
    }
    if (!normalizer_name) {
      grn_snip_set_normalizer(ctx, snip, GRN_NORMALIZER_AUTO);
    } else if (GRN_TEXT_LEN(normalizer_name) > 0) {
      grn_obj *normalizer;
      normalizer = grn_ctx_get(ctx,
                               GRN_TEXT_VALUE(normalizer_name),
                               GRN_TEXT_LEN(normalizer_name));
      if (!grn_obj_is_normalizer_proc(ctx, normalizer)) {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, normalizer);
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "%s not normalizer: <%.*s>",
                         tag,
                         (int)GRN_TEXT_LEN(&inspected),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        grn_obj_unlink(ctx, normalizer);
        goto exit;
      }
      grn_snip_set_normalizer(ctx, snip, normalizer);
      grn_obj_unlink(ctx, normalizer);
    }
    if ((!default_open_tag || GRN_TEXT_LEN(default_open_tag) == 0) &&
        (!default_close_tag || GRN_TEXT_LEN(default_close_tag) == 0)) {
      unsigned int i;
      unsigned int n_keyword_sets =
        (n_args_without_option - N_REQUIRED_ARGS) / KEYWORD_SET_SIZE;
      grn_obj **keyword_set_args = args + N_REQUIRED_ARGS;
      for (i = 0; i < n_keyword_sets; i++) {
        unsigned int index = i * KEYWORD_SET_SIZE;
        grn_rc rc = grn_snip_add_cond(ctx,
                                      snip,
                                      GRN_TEXT_VALUE(keyword_set_args[index]),
                                      GRN_TEXT_LEN(keyword_set_args[index]),
                                      GRN_TEXT_VALUE(keyword_set_args[index + 1]),
                                      GRN_TEXT_LEN(keyword_set_args[index + 1]),
                                      GRN_TEXT_VALUE(keyword_set_args[index + 2]),
                                      GRN_TEXT_LEN(keyword_set_args[index + 2]));
        if (rc != GRN_SUCCESS) {
          goto exit;
        }
      }
    } else {
      unsigned int n_keywords = n_args_without_option - N_REQUIRED_ARGS;
      if (n_keywords == 0) {
        grn_obj *condition = grn_expr_get_condition(ctx, expression);
        for (; condition; condition = grn_expr_get_parent(ctx, condition)) {
          grn_expr_snip_add_conditions(ctx,
                                       condition,
                                       snip,
                                       0,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL);
        }
      } else {
        unsigned int i;
        grn_obj **keyword_args = args + N_REQUIRED_ARGS;
        for (i = 0; i < n_keywords; i++) {
          grn_rc rc = grn_snip_add_cond(ctx, snip,
                                        GRN_TEXT_VALUE(keyword_args[i]),
                                        GRN_TEXT_LEN(keyword_args[i]),
                                        NULL, 0,
                                        NULL, 0);
          if (rc != GRN_SUCCESS) {
            goto exit;
          }
        }
      }
    }
    if (grn_obj_is_text_family_bulk(ctx, delimiter_regexp)) {
      grn_snip_set_delimiter_regexp(ctx,
                                    snip,
                                    GRN_TEXT_VALUE(delimiter_regexp),
                                    GRN_TEXT_LEN(delimiter_regexp));
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }
    if (snip_ptr) {
      GRN_PTR_SET(ctx, snip_ptr, snip);
    }
  }

  snippets = snippet_exec(ctx,
                          snip,
                          target,
                          default_return_value,
                          user_data,
                          prefix ? GRN_TEXT_VALUE(prefix) : NULL,
                          prefix ? GRN_TEXT_LEN(prefix) : 0,
                          suffix ? GRN_TEXT_VALUE(suffix) : NULL,
                          suffix ? GRN_TEXT_LEN(suffix) : 0);
#undef KEYWORD_SET_SIZE
#undef N_REQUIRED_ARGS

exit :
  GRN_OBJ_FIN(ctx, &cache_key);
  if (!snippets) {
    snippets = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  return snippets;
}

void
grn_proc_init_snippet(grn_ctx *ctx)
{
  grn_proc_create(ctx, "snippet", -1, GRN_PROC_FUNCTION,
                  func_snippet, NULL, NULL, 0, NULL);
}

static grn_obj *
func_snippet_html(grn_ctx *ctx, int nargs, grn_obj **args,
                  grn_user_data *user_data)
{
  grn_obj *snippets = NULL;

  if (nargs > 0) {
    grn_obj *target = args[0];
    grn_obj *expression = NULL;
    grn_obj *condition = NULL;
    grn_obj *snip = NULL;
    grn_obj *default_return_value = NULL;
    int flags = GRN_SNIP_SKIP_LEADING_SPACES;
    unsigned int width = 200;
    unsigned int max_n_results = 3;
    const char *open_tag = "<span class=\"keyword\">";
    const char *close_tag = "</span>";
    grn_snip_mapping *mapping = GRN_SNIP_MAPPING_HTML_ESCAPE;

    if (nargs > 1 && args[1]->header.type == GRN_TABLE_HASH_KEY) {
      grn_obj *options = args[1];
      void *key;
      uint32_t key_size;
      grn_obj *value;

      GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)options, cursor, id) {
        grn_raw_string option_name;

        grn_hash_cursor_get_key_value(ctx, cursor,
                                      &key, &key_size,
                                      (void **)&value);
        option_name.value = key;
        option_name.length = key_size;
        if (GRN_RAW_STRING_EQUAL_CSTRING(option_name, "default")) {
          if (value->header.type == GRN_PTR) {
            default_return_value = GRN_PTR_VALUE(value);
          } else {
            default_return_value = value;
          }
        } else {
          GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                           "snippet_html(): invalid option name: <%.*s>",
                           key_size, (char *)key);
          break;
        }
      } GRN_HASH_EACH_END(ctx, cursor);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }

    grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);
    condition = grn_expr_get_condition(ctx, expression);
    if (condition) {
      grn_obj *snip_ptr;
      snip_ptr = grn_expr_get_var(ctx, expression,
                                  GRN_FUNC_SNIPPET_HTML_CACHE_NAME,
                                  strlen(GRN_FUNC_SNIPPET_HTML_CACHE_NAME));
      if (snip_ptr) {
        snip = GRN_PTR_VALUE(snip_ptr);
      } else {
        snip_ptr =
          grn_expr_get_or_add_var(ctx, expression,
                                  GRN_FUNC_SNIPPET_HTML_CACHE_NAME,
                                  strlen(GRN_FUNC_SNIPPET_HTML_CACHE_NAME));
        GRN_OBJ_FIN(ctx, snip_ptr);
        GRN_PTR_INIT(snip_ptr, GRN_OBJ_OWN, GRN_DB_OBJECT);

        snip = grn_snip_open(ctx, flags, width, max_n_results,
                             open_tag, strlen(open_tag),
                             close_tag, strlen(close_tag),
                             mapping);
        if (snip) {
          grn_snip_set_normalizer(ctx, snip, GRN_NORMALIZER_AUTO);
          for (; condition; condition = grn_expr_get_parent(ctx, condition)) {
            grn_expr_snip_add_conditions(ctx, condition, snip,
                                         0, NULL, NULL, NULL, NULL);
          }
          GRN_PTR_SET(ctx, snip_ptr, snip);
        }
      }
    }

    if (snip) {
      snippets = snippet_exec(ctx, snip, target,
                              default_return_value,
                              user_data, NULL, 0, NULL, 0);
    }
  }

exit :
  if (!snippets) {
    snippets = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  return snippets;
}

void
grn_proc_init_snippet_html(grn_ctx *ctx)
{
  grn_proc_create(ctx, "snippet_html", -1, GRN_PROC_FUNCTION,
                  func_snippet_html, NULL, NULL, 0, NULL);
}
