/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2016 Brazil
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

#define GRN_FUNC_SNIPPET_HTML_CACHE_NAME      "$snippet_html"

static grn_obj *
snippet_exec(grn_ctx *ctx, grn_obj *snip, grn_obj *text,
             grn_user_data *user_data,
             const char *prefix, int prefix_length,
             const char *suffix, int suffix_length)
{
  grn_rc rc;
  unsigned int i, n_results, max_tagged_length;
  grn_obj snippet_buffer;
  grn_obj *snippets;

  if (GRN_TEXT_LEN(text) == 0) {
    return NULL;
  }

  rc = grn_snip_exec(ctx, snip,
                     GRN_TEXT_VALUE(text), GRN_TEXT_LEN(text),
                     &n_results, &max_tagged_length);
  if (rc != GRN_SUCCESS) {
    return NULL;
  }

  if (n_results == 0) {
    return grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  snippets = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_SHORT_TEXT, GRN_OBJ_VECTOR);
  if (!snippets) {
    return NULL;
  }

  GRN_TEXT_INIT(&snippet_buffer, 0);
  grn_bulk_space(ctx, &snippet_buffer, prefix_length + max_tagged_length + suffix_length);
  for (i = 0; i < n_results; i++) {
    unsigned int snippet_length;

    GRN_BULK_REWIND(&snippet_buffer);
    if (prefix_length) {
      GRN_TEXT_PUT(ctx, &snippet_buffer, prefix, prefix_length);
    }
    rc = grn_snip_get_result(ctx, snip, i,
                             GRN_TEXT_VALUE(&snippet_buffer) + prefix_length,
                             &snippet_length);
    if (rc == GRN_SUCCESS) {
      grn_vector_add_element(ctx, snippets,
                             strncat(GRN_TEXT_VALUE(&snippet_buffer), suffix, suffix_length),
                             prefix_length + snippet_length + suffix_length,
                             0, GRN_DB_SHORT_TEXT);
    }
  }
  GRN_OBJ_FIN(ctx, &snippet_buffer);

  return snippets;
}

static grn_obj *
func_snippet_html(grn_ctx *ctx, int nargs, grn_obj **args,
                  grn_user_data *user_data)
{
  grn_obj *snippets = NULL;

  /* TODO: support parameters */
  if (nargs == 1) {
    grn_obj *text = args[0];
    grn_obj *expression = NULL;
    grn_obj *condition_ptr = NULL;
    grn_obj *condition = NULL;
    grn_obj *snip = NULL;
    int flags = GRN_SNIP_SKIP_LEADING_SPACES;
    unsigned int width = 200;
    unsigned int max_n_results = 3;
    const char *open_tag = "<span class=\"keyword\">";
    const char *close_tag = "</span>";
    grn_snip_mapping *mapping = GRN_SNIP_MAPPING_HTML_ESCAPE;

    grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);
    condition_ptr = grn_expr_get_var(ctx, expression,
                                     GRN_SELECT_INTERNAL_VAR_CONDITION,
                                     strlen(GRN_SELECT_INTERNAL_VAR_CONDITION));
    if (condition_ptr) {
      condition = GRN_PTR_VALUE(condition_ptr);
    }

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
          grn_expr_snip_add_conditions(ctx, condition, snip,
                                       0, NULL, NULL, NULL, NULL);
          GRN_PTR_SET(ctx, snip_ptr, snip);
        }
      }
    }

    if (snip) {
      snippets = snippet_exec(ctx, snip, text, user_data, NULL, 0, NULL, 0);
    }
  }

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
