/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga/plugin.h>

static grn_rc
run_query(grn_ctx *ctx,
          grn_obj *table,
          int nargs,
          grn_obj **args,
          grn_obj *res,
          grn_operator op,
          grn_selector_data *selector_data)
{
  grn_rc rc = GRN_SUCCESS;
  const char *tag = "[query]";
  grn_obj *match_columns_string;
  grn_obj *query;
  grn_obj *query_expander_name = NULL;
  grn_operator default_mode = GRN_OP_MATCH;
  grn_operator default_operator = GRN_OP_AND;
  grn_expr_flags flags = GRN_EXPR_SYNTAX_QUERY;
  grn_expr_flags flags_specified = -1;
  grn_obj *match_columns = NULL;
  grn_obj *condition = NULL;
  grn_obj *dummy_variable;

  if (!(2 <= nargs && nargs <= 3)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "%s wrong number of arguments (%d for 2..3)",
                     tag,
                     nargs);
    rc = ctx->rc;
    goto exit;
  }

  match_columns_string = args[0];
  query = args[1];
  if (nargs > 2) {
    grn_obj *options = args[2];

    switch (options->header.type) {
    case GRN_BULK :
      query_expander_name = options;
      break;
    case GRN_TABLE_HASH_KEY :
#define OPTIONS                                                         \
      "expander",                                                       \
        GRN_PROC_OPTION_VALUE_RAW,                                      \
        &query_expander_name,                                           \
        "default_mode",                                                 \
        GRN_PROC_OPTION_VALUE_MODE,                                     \
        &default_mode,                                                  \
        "default_operator",                                             \
        GRN_PROC_OPTION_VALUE_OPERATOR,                                 \
        &default_operator,                                              \
        "flags",                                                        \
        GRN_PROC_OPTION_VALUE_EXPR_FLAGS,                               \
        &flags_specified
      if (selector_data) {
        rc = grn_selector_data_parse_options(ctx,
                                             selector_data,
                                             options,
                                             tag,
                                             OPTIONS,
                                             NULL);
      } else {
        rc = grn_proc_options_parse(ctx,
                                    options,
                                    tag,
                                    OPTIONS,
                                    NULL);
      }
#undef OPTIONS
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
      break;
    default :
      {
        grn_obj inspected;
        GRN_TEXT_INIT(&inspected, 0);
        grn_inspect(ctx, &inspected, options);
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "query(): "
                         "3rd argument must be string or object literal: <%.*s>",
                         (int)GRN_TEXT_LEN(&inspected),
                         GRN_TEXT_VALUE(&inspected));
        GRN_OBJ_FIN(ctx, &inspected);
      }
      rc = ctx->rc;
      goto exit;
    }
  }

  if (flags_specified == -1) {
    flags |= GRN_EXPR_ALLOW_PRAGMA | GRN_EXPR_ALLOW_COLUMN;
  } else {
    flags |= flags_specified;
  }

  if (match_columns_string->header.domain == GRN_DB_TEXT &&
      GRN_TEXT_LEN(match_columns_string) > 0) {
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, match_columns, dummy_variable);
    if (!match_columns) {
      rc = ctx->rc;
      goto exit;
    }

    grn_expr_parse(ctx, match_columns,
                   GRN_TEXT_VALUE(match_columns_string),
                   GRN_TEXT_LEN(match_columns_string),
                   NULL, GRN_OP_MATCH, GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      rc = ctx->rc;
      goto exit;
    }
  }

  if (query->header.domain == GRN_DB_TEXT && GRN_TEXT_LEN(query) > 0) {
    const char *query_string;
    unsigned int query_string_len;
    grn_obj expanded_query;

    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, condition, dummy_variable);
    if (!condition) {
      rc = ctx->rc;
      goto exit;
    }

    query_string = GRN_TEXT_VALUE(query);
    query_string_len = GRN_TEXT_LEN(query);

    GRN_TEXT_INIT(&expanded_query, 0);
    if (query_expander_name &&
        query_expander_name->header.domain == GRN_DB_TEXT &&
        GRN_TEXT_LEN(query_expander_name) > 0) {
      rc = grn_proc_syntax_expand_query(ctx,
                                        query_string, query_string_len,
                                        flags,
                                        GRN_TEXT_VALUE(query_expander_name),
                                        GRN_TEXT_LEN(query_expander_name),
                                        NULL, 0,
                                        NULL, 0,
                                        &expanded_query,
                                        "[query]");
      if (rc != GRN_SUCCESS) {
        GRN_OBJ_FIN(ctx, &expanded_query);
        goto exit;
      }
      query_string = GRN_TEXT_VALUE(&expanded_query);
      query_string_len = GRN_TEXT_LEN(&expanded_query);
    }
    grn_expr_parse(ctx,
                   condition,
                   query_string,
                   query_string_len,
                   match_columns,
                   default_mode,
                   default_operator,
                   flags);
    rc = ctx->rc;
    GRN_OBJ_FIN(ctx, &expanded_query);
    if (rc != GRN_SUCCESS) {
      goto exit;
    }
    grn_table_select(ctx, table, condition, res, op);
    rc = ctx->rc;
  }

exit :
  if (match_columns) {
    grn_obj_unlink(ctx, match_columns);
  }
  if (condition) {
    grn_obj_unlink(ctx, condition);
  }

  return rc;
}

static grn_obj *
func_query(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_proc_selector_to_function_data data;

  if (grn_proc_selector_to_function_data_init(ctx, &data, user_data)) {
    grn_rc rc;
    rc = run_query(ctx, data.table, nargs, args, data.records, GRN_OP_AND, NULL);
    if (rc == GRN_SUCCESS) {
      grn_proc_selector_to_function_data_selected(ctx, &data);
    }
  }
  grn_proc_selector_to_function_data_fin(ctx, &data);

  return data.found;
}

static grn_rc
selector_query(grn_ctx *ctx, grn_obj *table, grn_obj *index,
               int nargs, grn_obj **args,
               grn_obj *res, grn_operator op)
{
  return run_query(ctx,
                   table,
                   nargs - 1,
                   args + 1,
                   res,
                   op,
                   grn_selector_data_get(ctx));
}

void
grn_proc_init_query(grn_ctx *ctx)
{
  grn_obj *selector_proc = grn_proc_create(ctx,
                                           "query", -1,
                                           GRN_PROC_FUNCTION,
                                           func_query,
                                           NULL,
                                           NULL,
                                           0,
                                           NULL);
  grn_proc_set_selector(ctx, selector_proc, selector_query);
  grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
}

static grn_obj *
command_query_expand(grn_ctx *ctx, int nargs, grn_obj **args,
                     grn_user_data *user_data)
{
  const char *expander;
  size_t expander_size;
  const char *query;
  size_t query_size;
  const char *flags_raw;
  size_t flags_raw_size;
  grn_expr_flags flags = GRN_EXPR_SYNTAX_QUERY;
  const char *term_column;
  size_t term_column_size;
  const char *expanded_term_column;
  size_t expanded_term_column_size;
  grn_obj expanded_query;

  expander = grn_plugin_proc_get_var_string(ctx,
                                            user_data,
                                            "expander",
                                            -1,
                                            &expander_size);
  query = grn_plugin_proc_get_var_string(ctx,
                                         user_data,
                                         "query",
                                         -1,
                                         &query_size);
  flags_raw = grn_plugin_proc_get_var_string(ctx,
                                             user_data,
                                             "flags",
                                             -1,
                                             &flags_raw_size);
  term_column = grn_plugin_proc_get_var_string(ctx,
                                            user_data,
                                            "term_column",
                                            -1,
                                            &term_column_size);
  expanded_term_column =
    grn_plugin_proc_get_var_string(ctx,
                                   user_data,
                                   "expanded_term_column",
                                   -1,
                                   &expanded_term_column_size);

  if (flags_raw_size > 0) {
    grn_obj flags_bulk;
    GRN_TEXT_INIT(&flags_bulk, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET(ctx,
                 &flags_bulk,
                 flags_raw,
                 flags_raw_size);
    flags |= grn_proc_expr_query_flags_parse(ctx,
                                             &flags_bulk,
                                             "[query][expand]");
  } else {
    flags |= GRN_EXPR_ALLOW_PRAGMA | GRN_EXPR_ALLOW_COLUMN;
  }

  if (ctx->rc != GRN_SUCCESS) {
    return NULL;
  }

  GRN_TEXT_INIT(&expanded_query, 0);
  grn_proc_syntax_expand_query(ctx,
                               query,
                               query_size,
                               flags,
                               expander,
                               expander_size,
                               term_column,
                               term_column_size,
                               expanded_term_column,
                               expanded_term_column_size,
                               &expanded_query,
                               "[query][expand]");
  if (ctx->rc == GRN_SUCCESS) {
    grn_ctx_output_str(ctx,
                       GRN_TEXT_VALUE(&expanded_query),
                       GRN_TEXT_LEN(&expanded_query));
  }
  GRN_OBJ_FIN(ctx, &expanded_query);

  return NULL;
}

void
grn_proc_init_query_expand(grn_ctx *ctx)
{
  grn_expr_var vars[5];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "expander", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "query", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "term_column", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "expanded_term_column", -1);
  grn_plugin_command_create(ctx,
                            "query_expand", -1,
                            command_query_expand,
                            5,
                            vars);
}
