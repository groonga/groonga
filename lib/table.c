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

static const char *OPTION_NAME_DEFAULT_TOKENIZER = "default_tokenizer";
static const char *OPTION_NAME_NORMALIZER = "normalizer";

grn_rc
grn_table_apply_expr(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *output_column,
                     grn_obj *expr)
{
  grn_expr_executor executor;

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

  grn_expr_executor_init(ctx, &executor, expr);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_API_RETURN(ctx->rc);
  }
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, table, cursor, id, GRN_CURSOR_BY_ID) {
    grn_obj *value;
    value = grn_expr_executor_exec(ctx, &executor, id);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    if (value) {
      grn_obj_set_value(ctx, output_column, id, value, GRN_OBJ_SET);
    }
  } GRN_TABLE_EACH_END(ctx, cursor);
  grn_expr_executor_fin(ctx, &executor);

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
grn_table_module_init(grn_ctx *ctx,
                      grn_table_module *module,
                      grn_id module_id)
{
  if (module_id == GRN_ID_NIL) {
    module->proc = NULL;
  } else {
    module->proc = grn_ctx_at(ctx, module_id);
  }
  module->options = NULL;
  module->options_revision = GRN_OPTION_REVISION_NONE;
  module->options_close_func = NULL;
  CRITICAL_SECTION_INIT(module->lock);
}

static void
grn_table_module_fin_options(grn_ctx *ctx,
                             grn_table_module *module)
{
  if (module->options && module->options_close_func) {
    module->options_close_func(ctx, module->options);
    module->options = NULL;
    module->options_revision = GRN_OPTION_REVISION_NONE;
    module->options_close_func = NULL;
  }
}

void
grn_table_module_fin(grn_ctx *ctx,
                     grn_table_module *module)
{
  grn_table_module_fin_options(ctx, module);
  CRITICAL_SECTION_FIN(module->lock);
}

void
grn_table_module_set_proc(grn_ctx *ctx,
                          grn_table_module *module,
                          grn_obj *proc)
{
  CRITICAL_SECTION_ENTER(module->lock);
  grn_table_module_fin_options(ctx, module);

  module->proc = proc;
  CRITICAL_SECTION_LEAVE(module->lock);
}

void
grn_table_module_set_options(grn_ctx *ctx,
                             grn_table_module *module,
                             void *options,
                             grn_option_revision revision,
                             grn_close_func close_func)
{
  CRITICAL_SECTION_ENTER(module->lock);
  grn_table_module_fin_options(ctx, module);

  module->options = options;
  module->options_revision = revision;
  module->options_close_func = close_func;
  CRITICAL_SECTION_LEAVE(module->lock);
}

static grn_rc
grn_table_set_module_options(grn_ctx *ctx,
                             grn_obj *table,
                             const char *module_name,
                             grn_obj *options,
                             const char *context_tag)
{
  GRN_API_ENTER;

  if (!grn_obj_is_lexicon(ctx, table)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][%s][options][set] table must be key table: %s",
        context_tag,
        table ? grn_obj_type_to_string(table->header.type) : "(null)");
    GRN_API_RETURN(ctx->rc);
  }

  if (options &&
      options->header.type == GRN_VECTOR &&
      grn_vector_size(ctx, options) > 0) {
    grn_obj_set_option_values(ctx,
                              table,
                              module_name,
                              -1,
                              options);
  } else {
    grn_obj current_options;
    GRN_VOID_INIT(&current_options);
    grn_obj_get_option_values(ctx,
                              table,
                              module_name,
                              -1,
                              GRN_OPTION_REVISION_NONE,
                              &current_options);
    if (current_options.header.type == GRN_VECTOR &&
        grn_vector_size(ctx, &current_options) > 1) {
      grn_obj empty_options;
      GRN_TEXT_INIT(&empty_options, GRN_OBJ_VECTOR);
      grn_obj_set_option_values(ctx,
                                table,
                                module_name,
                                -1,
                                &empty_options);
      GRN_OBJ_FIN(ctx, &empty_options);
    }
    GRN_OBJ_FIN(ctx, &current_options);
  }

  GRN_API_RETURN(ctx->rc);
}

static grn_rc
grn_table_get_module_options(grn_ctx *ctx,
                             grn_obj *table,
                             const char *module_name,
                             grn_obj *options,
                             const char *context_tag)
{
  GRN_API_ENTER;

  if (!grn_obj_is_lexicon(ctx, table)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][%s][options][get] table must be key table: %s",
        context_tag,
        table ? grn_obj_type_to_string(table->header.type) : "(null)");
    GRN_API_RETURN(ctx->rc);
  }

  grn_obj_get_option_values(ctx,
                            table,
                            module_name,
                            -1,
                            GRN_OPTION_REVISION_NONE,
                            options);
  GRN_API_RETURN(ctx->rc);
}

typedef struct {
  const char *context_tag;
  const char *module_name;
  grn_info_type type;
  grn_table_module_open_options_func open_options_func;
  grn_close_func close_options_func;
  void *user_data;
} grn_table_cache_data;

static void *
grn_table_cache_module_options(grn_ctx *ctx,
                               grn_obj *table,
                               grn_table_cache_data *data)
{
  grn_table_module *module = NULL;
  grn_option_revision revision;
  grn_obj raw_options;
  void *options = NULL;

  GRN_API_ENTER;

  if (!grn_obj_is_lexicon(ctx, table)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][%s][options][cache] table must be key table: %s",
        data->context_tag,
        table ? grn_obj_type_to_string(table->header.type) : "(null)");
    GRN_API_RETURN(NULL);
  }

  switch (data->type) {
  case GRN_INFO_DEFAULT_TOKENIZER :
    switch (table->header.type) {
    case GRN_TABLE_HASH_KEY :
      module = &(((grn_hash *)table)->tokenizer);
      break;
    case GRN_TABLE_PAT_KEY :
      module = &(((grn_pat *)table)->tokenizer);
      break;
    case GRN_TABLE_DAT_KEY :
      module = &(((grn_dat *)table)->tokenizer);
      break;
    default :
      break;
    }
    break;
  case GRN_INFO_NORMALIZER :
    switch (table->header.type) {
    case GRN_TABLE_HASH_KEY :
      module = &(((grn_hash *)table)->normalizer);
      break;
    case GRN_TABLE_PAT_KEY :
      module = &(((grn_pat *)table)->normalizer);
      break;
    case GRN_TABLE_DAT_KEY :
      module = &(((grn_dat *)table)->normalizer);
      break;
    default :
      break;
    }
    break;
  default :
    break;
  }

  GRN_TEXT_INIT(&raw_options, GRN_OBJ_VECTOR);
  revision = grn_obj_get_option_values(ctx,
                                       table,
                                       data->module_name,
                                       -1,
                                       module->options_revision,
                                       &raw_options);
  if ((revision == GRN_OPTION_REVISION_UNCHANGED) ||
      (revision == GRN_OPTION_REVISION_NONE && module->options)) {
    goto exit;
  }

  options = data->open_options_func(ctx,
                                    module->proc,
                                    &raw_options,
                                    data->user_data);
  grn_table_module_set_options(ctx,
                               module,
                               options,
                               revision,
                               data->close_options_func);

exit :
  GRN_OBJ_FIN(ctx, &raw_options);

  GRN_API_RETURN(module->options);
}

static void
grn_table_get_module_string_raw(grn_ctx *ctx,
                                grn_obj *table,
                                grn_obj *output,
                                grn_obj *proc,
                                const char *module_name)
{
  char name[GRN_TABLE_MAX_KEY_SIZE];
  unsigned int name_size;
  grn_obj options;
  unsigned int n = 0;

  name_size = grn_obj_name(ctx, proc, name, GRN_TABLE_MAX_KEY_SIZE);
  GRN_TEXT_PUT(ctx, output, name, name_size);

  GRN_VOID_INIT(&options);
  grn_obj_get_option_values(ctx,
                            table,
                            module_name,
                            -1,
                            GRN_OPTION_REVISION_NONE,
                            &options);
  if (options.header.type != GRN_DB_VOID) {
    n = grn_vector_size(ctx, &options);
  }
  if (n > 0) {
    unsigned int i;
    grn_obj option;

    GRN_VOID_INIT(&option);
    GRN_TEXT_PUTS(ctx, output, "(");
    for (i = 0; i < n; i++) {
      const char *value;
      unsigned int value_size;
      grn_id domain;

      if (i > 0) {
        GRN_TEXT_PUTS(ctx, output, ", ");
      }

      value_size = grn_vector_get_element(ctx,
                                          &options,
                                          i,
                                          &value,
                                          NULL,
                                          &domain);
      grn_obj_reinit(ctx, &option, domain, 0);
      grn_bulk_write(ctx, &option, value, value_size);
      grn_text_otoj(ctx, output, &option, NULL);
    }
    GRN_TEXT_PUTS(ctx, output, ")");
    GRN_OBJ_FIN(ctx, &option);
  }
  GRN_OBJ_FIN(ctx, &options);
}

static grn_rc
grn_table_get_module_string(grn_ctx *ctx,
                            grn_obj *table,
                            grn_obj *output,
                            grn_info_type type,
                            const char *module_name,
                            const char *context_tag)
{
  grn_obj *proc;

  GRN_API_ENTER;

  if (!grn_obj_is_lexicon(ctx, table)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][%s][options][string] table must be key table: %s",
        context_tag,
        table ? grn_obj_type_to_string(table->header.type) : "(null)");
    GRN_API_RETURN(ctx->rc);
  }

  proc = grn_obj_get_info(ctx, table, type, NULL);
  if (!proc) {
    GRN_API_RETURN(ctx->rc);
  }

  grn_table_get_module_string_raw(ctx, table, output, proc, module_name);

  GRN_API_RETURN(ctx->rc);
}


  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_table_set_default_tokenizer_options(grn_ctx *ctx,
                                        grn_obj *table,
                                        grn_obj *options)
{
  return grn_table_set_module_options(ctx,
                                      table,
                                      OPTION_NAME_DEFAULT_TOKENIZER,
                                      options,
                                      "default-tokenizer");
}

grn_rc
grn_table_get_default_tokenizer_options(grn_ctx *ctx,
                                        grn_obj *table,
                                        grn_obj *options)
{
  return grn_table_get_module_options(ctx,
                                      table,
                                      OPTION_NAME_DEFAULT_TOKENIZER,
                                      options,
                                      "default-tokenizer");
}

void *
grn_table_cache_default_tokenizer_options(grn_ctx *ctx,
                                          grn_obj *table,
                                          grn_table_module_open_options_func open_options_func,
                                          grn_close_func close_options_func,
                                          void *user_data)
{
  grn_table_cache_data data;

  memset(&data, 0, sizeof(data));
  data.context_tag = "default-tokenizer";
  data.module_name = OPTION_NAME_DEFAULT_TOKENIZER;
  data.type = GRN_INFO_DEFAULT_TOKENIZER;
  data.open_options_func = open_options_func;
  data.close_options_func = close_options_func;
  data.user_data = user_data;
  return grn_table_cache_module_options(ctx, table, &data);
}

grn_rc
grn_table_get_default_tokenizer_string(grn_ctx *ctx,
                                       grn_obj *table,
                                       grn_obj *output)
{
  return grn_table_get_module_string(ctx,
                                     table,
                                     output,
                                     GRN_INFO_DEFAULT_TOKENIZER,
                                     OPTION_NAME_DEFAULT_TOKENIZER,
                                     "default-tokenizer");
}

grn_rc
grn_table_set_normalizer_options(grn_ctx *ctx,
                                 grn_obj *table,
                                 grn_obj *options)
{
  return grn_table_set_module_options(ctx,
                                      table,
                                      OPTION_NAME_NORMALIZER,
                                      options,
                                      "normalizer");
}

grn_rc
grn_table_get_normalizer_options(grn_ctx *ctx,
                                 grn_obj *table,
                                 grn_obj *options)
{
  return grn_table_get_module_options(ctx,
                                      table,
                                      OPTION_NAME_NORMALIZER,
                                      options,
                                      "normalizer");
}

void *
grn_table_cache_normalizer_options(grn_ctx *ctx,
                                   grn_obj *table,
                                   /* TODO: Remove me. */
                                   grn_obj *string,
                                   grn_table_module_open_options_func open_options_func,
                                   grn_close_func close_options_func,
                                   void *user_data)
{
  grn_table_cache_data data;

  memset(&data, 0, sizeof(data));
  data.context_tag = "normalizer";
  data.module_name = OPTION_NAME_NORMALIZER;
  data.type = GRN_INFO_NORMALIZER;
  data.open_options_func = open_options_func;
  data.close_options_func = close_options_func;
  data.user_data = user_data;
  return grn_table_cache_module_options(ctx, table, &data);
}

grn_rc
grn_table_get_normalizer_string(grn_ctx *ctx,
                                grn_obj *table,
                                grn_obj *output)
{
  return grn_table_get_module_string(ctx,
                                     table,
                                     output,
                                     GRN_INFO_NORMALIZER,
                                     OPTION_NAME_NORMALIZER,
                                     "normalizer");
}
