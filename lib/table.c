/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017-2018 Brazil

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

#include "grn.h"
#include "grn_ctx.h"
#include "grn_dat.h"
#include "grn_expr_executor.h"
#include "grn_hash.h"
#include "grn_pat.h"

grn_rc
grn_table_apply_expr(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *output_column,
                     grn_obj *expr)
{
  grn_expr_executor *executor;

  GRN_API_ENTER;

  if (!grn_obj_is_data_column(ctx, output_column)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, output_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply-expr] output column isn't data column: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  if (!grn_obj_is_expr(ctx, expr)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, expr);
    ERR(GRN_INVALID_ARGUMENT,
        "[table][apply-expr] expr is invalid: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }

  executor = grn_expr_executor_open(ctx, expr);
  if (!executor) {
    GRN_API_RETURN(ctx->rc);
  }
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, table, cursor, id, GRN_CURSOR_BY_ID) {
    grn_obj *value;
    value = grn_expr_executor_exec(ctx, executor, id);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    if (value) {
      grn_obj_set_value(ctx, output_column, id, value, GRN_OBJ_SET);
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  grn_expr_executor_close(ctx, executor);

  GRN_API_RETURN(ctx->rc);
}

grn_id
grn_table_find_reference_object(grn_ctx *ctx, grn_obj *table)
{
  grn_id table_id;
  grn_id reference_object_id = GRN_ID_NIL;

  GRN_API_ENTER;

  if (!grn_obj_is_table(ctx, table)) {
    GRN_API_RETURN(GRN_ID_NIL);
  }

  table_id = DB_OBJ(table)->id;

  GRN_DB_SPEC_EACH_BEGIN(ctx, cursor, id, spec) {
    if (id == table_id) {
      continue;
    }

    switch (spec->header.type) {
    case GRN_TABLE_HASH_KEY :
    case GRN_TABLE_PAT_KEY :
    case GRN_TABLE_DAT_KEY :
      if (spec->header.domain == table_id) {
        reference_object_id = id;
      }
      break;
    case GRN_COLUMN_VAR_SIZE :
    case GRN_COLUMN_FIX_SIZE :
      if (spec->header.domain == table_id) {
        break;
      }
      if (spec->range == table_id) {
        reference_object_id = id;
      }
      break;
    default :
      break;
    }

    if (reference_object_id != GRN_ID_NIL) {
      break;
    }
  } GRN_DB_SPEC_EACH_END(ctx, cursor);

  GRN_API_RETURN(reference_object_id);
}

void
grn_table_tokenizer_init(grn_ctx *ctx,
                         grn_table_tokenizer *tokenizer,
                         grn_id tokenizer_id)
{
  if (tokenizer_id == GRN_ID_NIL) {
    tokenizer->proc = NULL;
  } else {
    tokenizer->proc = grn_ctx_at(ctx, tokenizer_id);
  }
  tokenizer->options = NULL;
  tokenizer->options_revision = GRN_OPTION_REVISION_NONE;
  tokenizer->options_close_func = NULL;
}

static void
grn_table_tokenizer_fin_options(grn_ctx *ctx,
                                grn_table_tokenizer *tokenizer)
{
  if (tokenizer->options && tokenizer->options_close_func) {
    tokenizer->options_close_func(ctx, tokenizer->options);
    tokenizer->options = NULL;
    tokenizer->options_revision = GRN_OPTION_REVISION_NONE;
    tokenizer->options_close_func = NULL;
  }
}

void
grn_table_tokenizer_fin(grn_ctx *ctx,
                        grn_table_tokenizer *tokenizer)
{
  grn_table_tokenizer_fin_options(ctx, tokenizer);
}

void
grn_table_tokenizer_set_proc(grn_ctx *ctx,
                             grn_table_tokenizer *tokenizer,
                             grn_obj *proc)
{
  grn_table_tokenizer_fin_options(ctx, tokenizer);

  tokenizer->proc = proc;
}

void
grn_table_tokenizer_set_options(grn_ctx *ctx,
                                grn_table_tokenizer *tokenizer,
                                void *options,
                                grn_option_revision revision,
                                grn_close_func close_func)
{
  grn_table_tokenizer_fin_options(ctx, tokenizer);

  tokenizer->options = options;
  tokenizer->options_revision = revision;
  if (options) {
    tokenizer->options_close_func = close_func;
  }
}

void *
grn_table_cache_tokenizer_options(grn_ctx *ctx,
                                  grn_obj *table,
                                  grn_tokenizer_open_options_func open_options_func,
                                  grn_close_func close_options_func,
                                  void *user_data)
{
  grn_table_tokenizer *tokenizer;
  grn_option_revision revision;
  grn_obj raw_options;
  void *options;

  GRN_API_ENTER;

  if (!table) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][tokenizer-options][set] table is NULL");
    GRN_API_RETURN(NULL);
  }

  switch (table->header.type) {
  case GRN_TABLE_HASH_KEY :
    tokenizer = &(((grn_hash *)table)->tokenizer);
    break;
  case GRN_TABLE_PAT_KEY :
    tokenizer = &(((grn_pat *)table)->tokenizer);
    break;
  case GRN_TABLE_DAT_KEY :
    tokenizer = &(((grn_dat *)table)->tokenizer);
    break;
  default :
    ERR(GRN_INVALID_ARGUMENT,
        "[table][tokenizer-options][set] table must key table: %s",
        grn_obj_type_to_string(table->header.type));
    GRN_API_RETURN(NULL);
    break;
  }

  GRN_VOID_INIT(&raw_options);
  revision = grn_obj_get_option_values(ctx,
                                       table,
                                       "tokenizer",
                                       -1,
                                       tokenizer->options_revision,
                                       &raw_options);
  if ((revision == GRN_OPTION_REVISION_UNCHANGED) ||
      (revision == GRN_OPTION_REVISION_NONE && tokenizer->options)) {
    goto exit;
  }

  options = open_options_func(ctx, table, &raw_options, user_data);
  grn_table_tokenizer_set_options(ctx,
                                  tokenizer,
                                  options,
                                  revision,
                                  close_options_func);

exit :
  GRN_OBJ_FIN(ctx, &raw_options);

  GRN_API_RETURN(tokenizer->options);
}

