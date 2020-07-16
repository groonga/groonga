/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2017-2018 Brazil
  Copyright(C) 2018-2020 Sutou Kouhei <kou@clear-code.com>

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
#include "grn_hash.h"
#include "grn_pat.h"

#include <stdio.h>

static const char *OPTION_NAME_DEFAULT_TOKENIZER = "default_tokenizer";
static const char *OPTION_NAME_NORMALIZER = "normalizer";
static const char *OPTION_NAME_TOKEN_FILTER = "token_filter";

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

static void
grn_table_module_set_options_without_lock(grn_ctx *ctx,
                                          grn_table_module *module,
                                          void *options,
                                          grn_option_revision revision,
                                          grn_close_func close_func)
{
  grn_table_module_fin_options(ctx, module);

  module->options = options;
  module->options_revision = revision;
  module->options_close_func = close_func;
}

void
grn_table_module_set_options(grn_ctx *ctx,
                             grn_table_module *module,
                             void *options,
                             grn_option_revision revision,
                             grn_close_func close_func)
{
  CRITICAL_SECTION_ENTER(module->lock);
  grn_table_module_set_options_without_lock(ctx,
                                            module,
                                            options,
                                            revision,
                                            close_func);
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
  unsigned int token_filter_index;
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
  case GRN_INFO_TOKEN_FILTERS :
    {
      grn_obj *token_filters = NULL;
      switch (table->header.type) {
      case GRN_TABLE_HASH_KEY :
        token_filters = &(((grn_hash *)table)->token_filters);
        break;
      case GRN_TABLE_PAT_KEY :
        token_filters = &(((grn_pat *)table)->token_filters);
        break;
      case GRN_TABLE_DAT_KEY :
        token_filters = &(((grn_dat *)table)->token_filters);
        break;
      default :
        break;
      }
      module =
        ((grn_table_module *)GRN_BULK_HEAD(token_filters)) +
        data->token_filter_index;
    }
    break;
  default :
    break;
  }

  GRN_TEXT_INIT(&raw_options, GRN_OBJ_VECTOR);
  CRITICAL_SECTION_ENTER(module->lock);
  revision = grn_obj_get_option_values(ctx,
                                       table,
                                       data->module_name,
                                       -1,
                                       module->options_revision,
                                       &raw_options);
  bool need_update = true;
  if (revision == GRN_OPTION_REVISION_UNCHANGED) {
    need_update = false;
  }
  if (revision == GRN_OPTION_REVISION_NONE && module->options) {
    need_update = false;
  }
  if (need_update) {
    options = data->open_options_func(ctx,
                                      module->proc,
                                      &raw_options,
                                      data->user_data);
    grn_table_module_set_options_without_lock(ctx,
                                              module,
                                              options,
                                              revision,
                                              data->close_options_func);
  }
  CRITICAL_SECTION_LEAVE(module->lock);
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

static grn_rc
grn_table_get_modules_string(grn_ctx *ctx,
                             grn_obj *table,
                             grn_obj *output,
                             grn_info_type type,
                             const char *module_name,
                             const char *context_tag)
{
  grn_obj procs;
  unsigned int i, n;

  GRN_API_ENTER;

  if (!grn_obj_is_lexicon(ctx, table)) {
    ERR(GRN_INVALID_ARGUMENT,
        "[table][%s][options][string] table must be key table: %s",
        context_tag,
        table ? grn_obj_type_to_string(table->header.type) : "(null)");
    GRN_API_RETURN(ctx->rc);
  }

  GRN_PTR_INIT(&procs, GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_obj_get_info(ctx, table, type, &procs);
  if (GRN_BULK_VSIZE(&procs) == 0) {
    GRN_OBJ_FIN(ctx, &procs);
    GRN_API_RETURN(ctx->rc);
  }

  n = grn_vector_size(ctx, &procs);
  if (n == 0) {
    GRN_API_RETURN(ctx->rc);
  }

  for (i = 0; i < n; i++) {
    char real_module_name[GRN_TABLE_MAX_KEY_SIZE];
    grn_obj *proc = GRN_PTR_VALUE_AT(&procs, i);

    if (i > 0) {
      GRN_TEXT_PUTS(ctx, output, ", ");
    }
    grn_snprintf(real_module_name,
                 GRN_TABLE_MAX_KEY_SIZE,
                 GRN_TABLE_MAX_KEY_SIZE,
                 "%s%u",
                 module_name,
                 i);
    grn_table_get_module_string_raw(ctx,
                                    table,
                                    output,
                                    proc,
                                    real_module_name);
  }

  GRN_OBJ_FIN(ctx, &procs);

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

grn_rc
grn_table_set_token_filter_options(grn_ctx *ctx,
                                   grn_obj *table,
                                   unsigned int i,
                                   grn_obj *options)
{
  char module_name[GRN_TABLE_MAX_KEY_SIZE];
  grn_snprintf(module_name,
               GRN_TABLE_MAX_KEY_SIZE,
               GRN_TABLE_MAX_KEY_SIZE,
               "%s%u",
               OPTION_NAME_TOKEN_FILTER,
               i);
  return grn_table_set_module_options(ctx,
                                      table,
                                      module_name,
                                      options,
                                      "token-filter");
}

grn_rc
grn_table_get_token_filter_options(grn_ctx *ctx,
                                   grn_obj *table,
                                   unsigned int i,
                                   grn_obj *options)
{
  char module_name[GRN_TABLE_MAX_KEY_SIZE];
  grn_snprintf(module_name,
               GRN_TABLE_MAX_KEY_SIZE,
               GRN_TABLE_MAX_KEY_SIZE,
               "%s%u",
               OPTION_NAME_TOKEN_FILTER,
               i);
  return grn_table_get_module_options(ctx,
                                      table,
                                      module_name,
                                      options,
                                      "token-filter");
}

void *
grn_table_cache_token_filter_options(grn_ctx *ctx,
                                     grn_obj *table,
                                     unsigned int i,
                                     grn_table_module_open_options_func open_options_func,
                                     grn_close_func close_options_func,
                                     void *user_data)
{
  grn_table_cache_data data;
  char module_name[GRN_TABLE_MAX_KEY_SIZE];
  grn_snprintf(module_name,
               GRN_TABLE_MAX_KEY_SIZE,
               GRN_TABLE_MAX_KEY_SIZE,
               "%s%u",
               OPTION_NAME_TOKEN_FILTER,
               i);

  memset(&data, 0, sizeof(data));
  data.context_tag = "token-filter";
  data.module_name = module_name;
  data.type = GRN_INFO_TOKEN_FILTERS;
  data.token_filter_index = i;
  data.open_options_func = open_options_func;
  data.close_options_func = close_options_func;
  data.user_data = user_data;
  return grn_table_cache_module_options(ctx, table, &data);
}

grn_rc
grn_table_get_token_filters_string(grn_ctx *ctx,
                                   grn_obj *table,
                                   grn_obj *output)
{
  return grn_table_get_modules_string(ctx,
                                      table,
                                      output,
                                      GRN_INFO_TOKEN_FILTERS,
                                      OPTION_NAME_TOKEN_FILTER,
                                      "token-filter");
}
