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

#define GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME    "$highlight_html"

static void
grn_pat_tag_keys_put_original_text(grn_ctx *ctx, grn_obj *output,
                                   const char *text, unsigned int length,
                                   grn_bool use_html_escape)
{
  if (use_html_escape) {
    grn_text_escape_xml(ctx, output, text, length);
  } else {
    GRN_TEXT_PUT(ctx, output, text, length);
  }
}

static grn_rc
grn_pat_tag_keys(grn_ctx *ctx, grn_obj *keywords,
                 const char *string, unsigned int string_length,
                 const char **open_tags, unsigned int *open_tag_lengths,
                 const char **close_tags, unsigned int *close_tag_lengths,
                 unsigned int n_tags,
                 grn_obj *highlighted,
                 grn_bool use_html_escape)
{
  while (string_length > 0) {
#define MAX_N_HITS 1024
    grn_pat_scan_hit hits[MAX_N_HITS];
    const char *rest;
    unsigned int i, n_hits;
    unsigned int previous = 0;

    n_hits = grn_pat_scan(ctx, (grn_pat *)keywords,
                          string, string_length,
                          hits, MAX_N_HITS, &rest);
    for (i = 0; i < n_hits; i++) {
      unsigned int nth_tag;
      if (hits[i].offset - previous > 0) {
        grn_pat_tag_keys_put_original_text(ctx,
                                           highlighted,
                                           string + previous,
                                           hits[i].offset - previous,
                                           use_html_escape);
      }
      nth_tag = ((hits[i].id - 1) % n_tags);
      GRN_TEXT_PUT(ctx, highlighted,
                   open_tags[nth_tag], open_tag_lengths[nth_tag]);
      grn_pat_tag_keys_put_original_text(ctx,
                                         highlighted,
                                         string + hits[i].offset,
                                         hits[i].length,
                                         use_html_escape);
      GRN_TEXT_PUT(ctx, highlighted,
                   close_tags[nth_tag], close_tag_lengths[nth_tag]);
      previous = hits[i].offset + hits[i].length;
    }
    if (string_length - previous > 0) {
      grn_pat_tag_keys_put_original_text(ctx,
                                         highlighted,
                                         string + previous,
                                         string_length - previous,
                                         use_html_escape);
    }
    string_length -= rest - string;
    string = rest;
#undef MAX_N_HITS
  }

  return GRN_SUCCESS;
}

static grn_obj *
func_highlight_full(grn_ctx *ctx, int nargs, grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *highlighted = NULL;

#define N_REQUIRED_ARGS 3
#define KEYWORD_SET_SIZE 3
  if ((nargs >= (N_REQUIRED_ARGS + KEYWORD_SET_SIZE) &&
      (nargs - N_REQUIRED_ARGS) % KEYWORD_SET_SIZE == 0)) {
    grn_obj *string = args[0];
    grn_obj *normalizer_name = args[1];
    grn_obj *use_html_escape = args[2];
    grn_obj **keyword_set_args = args + N_REQUIRED_ARGS;
    unsigned int n_keyword_sets = (nargs - N_REQUIRED_ARGS) / KEYWORD_SET_SIZE;
    unsigned int i;
    grn_obj open_tags;
    grn_obj open_tag_lengths;
    grn_obj close_tags;
    grn_obj close_tag_lengths;
    grn_obj *keywords;

    keywords = grn_table_create(ctx, NULL, 0, NULL,
                                GRN_OBJ_TABLE_PAT_KEY,
                                grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                                NULL);

    if (GRN_TEXT_LEN(normalizer_name)) {
      grn_obj *normalizer;
      normalizer = grn_ctx_get(ctx,
                               GRN_TEXT_VALUE(normalizer_name),
                               GRN_TEXT_LEN(normalizer_name));
      if (!grn_obj_is_normalizer_proc(ctx, normalizer)) {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, normalizer);
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
            "highlight_full(): not normalizer: <%.*s>",
            (int)GRN_TEXT_LEN(&inspected),
            GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
        grn_obj_unlink(ctx, normalizer);
        grn_obj_unlink(ctx, keywords);
        return NULL;
      }
      grn_obj_set_info(ctx, keywords, GRN_INFO_NORMALIZER, normalizer);
      grn_obj_unlink(ctx, normalizer);
    }

    GRN_OBJ_INIT(&open_tags, GRN_BULK, 0, GRN_DB_VOID);
    GRN_OBJ_INIT(&open_tag_lengths, GRN_BULK, 0, GRN_DB_VOID);
    GRN_OBJ_INIT(&close_tags, GRN_BULK, 0, GRN_DB_VOID);
    GRN_OBJ_INIT(&close_tag_lengths, GRN_BULK, 0, GRN_DB_VOID);
    for (i = 0; i < n_keyword_sets; i++) {
      grn_obj *keyword   = keyword_set_args[i * KEYWORD_SET_SIZE + 0];
      grn_obj *open_tag  = keyword_set_args[i * KEYWORD_SET_SIZE + 1];
      grn_obj *close_tag = keyword_set_args[i * KEYWORD_SET_SIZE + 2];

      grn_table_add(ctx, keywords,
                    GRN_TEXT_VALUE(keyword),
                    GRN_TEXT_LEN(keyword),
                    NULL);

      {
        const char *open_tag_content = GRN_TEXT_VALUE(open_tag);
        grn_bulk_write(ctx, &open_tags,
                       (const char *)(&open_tag_content),
                       sizeof(char *));
      }
      {
        unsigned int open_tag_length = GRN_TEXT_LEN(open_tag);
        grn_bulk_write(ctx, &open_tag_lengths,
                       (const char *)(&open_tag_length),
                       sizeof(unsigned int));
      }
      {
        const char *close_tag_content = GRN_TEXT_VALUE(close_tag);
        grn_bulk_write(ctx, &close_tags,
                       (const char *)(&close_tag_content),
                       sizeof(char *));
      }
      {
        unsigned int close_tag_length = GRN_TEXT_LEN(close_tag);
        grn_bulk_write(ctx, &close_tag_lengths,
                       (const char *)(&close_tag_length),
                       sizeof(unsigned int));
      }
    }

    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);
    grn_pat_tag_keys(ctx, keywords,
                     GRN_TEXT_VALUE(string), GRN_TEXT_LEN(string),
                     (const char **)GRN_BULK_HEAD(&open_tags),
                     (unsigned int *)GRN_BULK_HEAD(&open_tag_lengths),
                     (const char **)GRN_BULK_HEAD(&close_tags),
                     (unsigned int *)GRN_BULK_HEAD(&close_tag_lengths),
                     n_keyword_sets,
                     highlighted,
                     GRN_BOOL_VALUE(use_html_escape));

    grn_obj_unlink(ctx, keywords);
    grn_obj_unlink(ctx, &open_tags);
    grn_obj_unlink(ctx, &open_tag_lengths);
    grn_obj_unlink(ctx, &close_tags);
    grn_obj_unlink(ctx, &close_tag_lengths);
  }
#undef N_REQUIRED_ARGS
#undef KEYWORD_SET_SIZE

  if (!highlighted) {
    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  return highlighted;
}

void
grn_proc_init_highlight_full(grn_ctx *ctx)
{
  grn_proc_create(ctx, "highlight_full", -1, GRN_PROC_FUNCTION,
                  func_highlight_full, NULL, NULL, 0, NULL);
}

static grn_obj *
func_highlight_html_create_keywords_table(grn_ctx *ctx, grn_obj *expression)
{
  grn_obj *keywords;
  grn_obj *condition_ptr = NULL;
  grn_obj *condition = NULL;

  keywords = grn_table_create(ctx, NULL, 0, NULL,
                              GRN_OBJ_TABLE_PAT_KEY,
                              grn_ctx_at(ctx, GRN_DB_SHORT_TEXT),
                              NULL);

  {
    grn_obj *normalizer;
    normalizer = grn_ctx_get(ctx, "NormalizerAuto", -1);
    grn_obj_set_info(ctx, keywords, GRN_INFO_NORMALIZER, normalizer);
    grn_obj_unlink(ctx, normalizer);
  }

  condition_ptr = grn_expr_get_var(ctx, expression,
                                   GRN_SELECT_INTERNAL_VAR_CONDITION,
                                   strlen(GRN_SELECT_INTERNAL_VAR_CONDITION));
  if (condition_ptr) {
    condition = GRN_PTR_VALUE(condition_ptr);
  }

  if (condition) {
    size_t i, n_keywords;
    grn_obj current_keywords;
    GRN_PTR_INIT(&current_keywords, GRN_OBJ_VECTOR, GRN_ID_NIL);
    grn_expr_get_keywords(ctx, condition, &current_keywords);

    n_keywords = GRN_BULK_VSIZE(&current_keywords) / sizeof(grn_obj *);
    for (i = 0; i < n_keywords; i++) {
      grn_obj *keyword;
      keyword = GRN_PTR_VALUE_AT(&current_keywords, i);
      grn_table_add(ctx, keywords,
                    GRN_TEXT_VALUE(keyword),
                    GRN_TEXT_LEN(keyword),
                    NULL);
    }
    grn_obj_unlink(ctx, &current_keywords);
  }

  return keywords;
}

static grn_obj *
func_highlight_html(grn_ctx *ctx, int nargs, grn_obj **args,
                    grn_user_data *user_data)
{
  grn_obj *highlighted = NULL;

#define N_REQUIRED_ARGS 1
  if (nargs == N_REQUIRED_ARGS) {
    grn_obj *string = args[0];
    grn_obj *expression = NULL;
    grn_obj *keywords;
    grn_obj *keywords_ptr;
    grn_bool use_html_escape = GRN_TRUE;
    unsigned int n_keyword_sets = 1;
    const char *open_tags[1];
    unsigned int open_tag_lengths[1];
    const char *close_tags[1];
    unsigned int close_tag_lengths[1];

    grn_proc_get_info(ctx, user_data, NULL, NULL, &expression);

    keywords_ptr = grn_expr_get_var(ctx, expression,
                                    GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME,
                                    strlen(GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME));
    if (keywords_ptr) {
      keywords = GRN_PTR_VALUE(keywords_ptr);
    } else {
      keywords_ptr =
        grn_expr_get_or_add_var(ctx, expression,
                                GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME,
                                strlen(GRN_FUNC_HIGHLIGHT_HTML_CACHE_NAME));
      GRN_OBJ_FIN(ctx, keywords_ptr);
      GRN_PTR_INIT(keywords_ptr, GRN_OBJ_OWN, GRN_DB_OBJECT);

      keywords = func_highlight_html_create_keywords_table(ctx, expression);
      GRN_PTR_SET(ctx, keywords_ptr, keywords);
    }

    open_tags[0] = "<span class=\"keyword\">";
    open_tag_lengths[0] = strlen("<span class=\"keyword\">");
    close_tags[0]  = "</span>";
    close_tag_lengths[0] = strlen("</span>");

    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_TEXT, 0);

    grn_pat_tag_keys(ctx, keywords,
                     GRN_TEXT_VALUE(string), GRN_TEXT_LEN(string),
                     open_tags,
                     open_tag_lengths,
                     close_tags,
                     close_tag_lengths,
                     n_keyword_sets,
                     highlighted,
                     use_html_escape);

  }
#undef N_REQUIRED_ARGS

  if (!highlighted) {
    highlighted = grn_plugin_proc_alloc(ctx, user_data, GRN_DB_VOID, 0);
  }

  return highlighted;
}

void
grn_proc_init_highlight_html(grn_ctx *ctx)
{
  grn_proc_create(ctx, "highlight_html", -1, GRN_PROC_FUNCTION,
                  func_highlight_html, NULL, NULL, 0, NULL);
}
