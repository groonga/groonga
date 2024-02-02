/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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
#include "../grn_ctx.h"
#include "../grn_token_cursor.h"

#include <groonga/plugin.h>

typedef struct {
  grn_id id;
  int32_t position;
  bool force_prefix_search;
  uint64_t source_offset;
  uint32_t source_length;
  uint32_t source_first_character_length;
  grn_obj metadata;
} tokenize_token;

static void
init_tokens(grn_ctx *ctx,
            grn_obj *tokens)
{
  GRN_VALUE_FIX_SIZE_INIT(tokens, GRN_OBJ_VECTOR, GRN_ID_NIL);
}

static void
fin_tokens(grn_ctx *ctx,
           grn_obj *tokens)
{
  size_t i;
  size_t n_tokens;

  n_tokens = GRN_BULK_VSIZE(tokens) / sizeof(tokenize_token);
  for (i = 0; i < n_tokens; i++) {
    tokenize_token *token;
    token = ((tokenize_token *)(GRN_BULK_HEAD(tokens))) + i;
    GRN_OBJ_FIN(ctx, &(token->metadata));
  }
  GRN_OBJ_FIN(ctx, tokens);
}

static void
output_tokens(grn_ctx *ctx,
              grn_obj *tokens,
              grn_obj *lexicon,
              grn_obj *index_column)
{
  size_t i, n_tokens, n_elements;
  grn_obj estimated_size;
  grn_bool have_source_location = GRN_FALSE;
  grn_bool have_metadata = GRN_FALSE;

  n_tokens = GRN_BULK_VSIZE(tokens) / sizeof(tokenize_token);
  n_elements = 4;
  if (index_column) {
    n_elements++;
    GRN_UINT32_INIT(&estimated_size, 0);
  }
  for (i = 0; i < n_tokens; i++) {
    tokenize_token *token;
    token = ((tokenize_token *)(GRN_BULK_HEAD(tokens))) + i;
    if (token->source_offset > 0 || token->source_length > 0) {
      have_source_location = GRN_TRUE;
    }
    if (grn_vector_size(ctx, &(token->metadata)) > 0) {
      have_metadata = GRN_TRUE;
    }
  }
  if (have_source_location) {
    n_elements += 3;
  }
  if (have_metadata) {
    n_elements += 1;
  }

  grn_ctx_output_array_open(ctx, "TOKENS", (int)n_tokens);
  for (i = 0; i < n_tokens; i++) {
    tokenize_token *token;
    char value[GRN_TABLE_MAX_KEY_SIZE];
    int value_size;

    token = ((tokenize_token *)(GRN_BULK_HEAD(tokens))) + i;

    grn_ctx_output_map_open(ctx, "TOKEN", (int)n_elements);

    grn_ctx_output_cstr(ctx, "value");
    value_size = grn_table_get_key(ctx, lexicon, token->id,
                                   value, GRN_TABLE_MAX_KEY_SIZE);
    grn_ctx_output_str(ctx, value, (size_t)value_size);

    grn_ctx_output_cstr(ctx, "position");
    grn_ctx_output_int32(ctx, token->position);

    /* For backward compatibility. */
    grn_ctx_output_cstr(ctx, "force_prefix");
    grn_ctx_output_bool(ctx, token->force_prefix_search);

    grn_ctx_output_cstr(ctx, "force_prefix_search");
    grn_ctx_output_bool(ctx, token->force_prefix_search);

    if (index_column) {
      GRN_BULK_REWIND(&estimated_size);
      grn_obj_get_value(ctx, index_column, token->id, &estimated_size);
      grn_ctx_output_cstr(ctx, "estimated_size");
      grn_ctx_output_int64(ctx, GRN_UINT32_VALUE(&estimated_size));
    }

    if (have_source_location) {
      grn_ctx_output_cstr(ctx, "source_offset");
      grn_ctx_output_uint64(ctx, token->source_offset);

      grn_ctx_output_cstr(ctx, "source_length");
      grn_ctx_output_uint32(ctx, token->source_length);

      grn_ctx_output_cstr(ctx, "source_first_character_length");
      grn_ctx_output_uint32(ctx, token->source_first_character_length);
    }

    if (have_metadata) {
      size_t i;
      size_t n_metadata;
      grn_obj value;

      n_metadata = grn_vector_size(ctx, &(token->metadata)) / 2;
      GRN_VOID_INIT(&value);
      grn_ctx_output_cstr(ctx, "metadata");
      grn_ctx_output_map_open(ctx, "METADATA", (int)n_metadata);
      for (i = 0; i < n_metadata; i++) {
        const char *raw_name;
        unsigned int raw_name_length;
        const char *raw_value;
        unsigned int raw_value_length;
        grn_id value_domain;

        raw_name_length = grn_vector_get_element(ctx,
                                                 &(token->metadata),
                                                 (uint32_t)(i * 2),
                                                 &raw_name,
                                                 NULL,
                                                 NULL);
        grn_ctx_output_str(ctx, raw_name, raw_name_length);

        raw_value_length = grn_vector_get_element(ctx,
                                                  &(token->metadata),
                                                  (uint32_t)(i * 2 + 1),
                                                  &raw_value,
                                                  NULL,
                                                  &value_domain);
        grn_obj_reinit(ctx, &value, value_domain, 0);
        grn_bulk_write(ctx, &value, raw_value, raw_value_length);
        grn_ctx_output_obj(ctx, &value, NULL);
      }
      grn_ctx_output_map_close(ctx);

      GRN_OBJ_FIN(ctx, &value);
    }

    grn_ctx_output_map_close(ctx);
  }

  if (index_column) {
    GRN_OBJ_FIN(ctx, &estimated_size);
  }

  grn_ctx_output_array_close(ctx);
}

static void
tokenize(grn_ctx *ctx,
         grn_obj *lexicon,
         grn_raw_string *string_raw,
         grn_tokenize_mode mode,
         unsigned int flags,
         grn_obj *tokens)
{
  grn_token_cursor *token_cursor;

  token_cursor =
    grn_token_cursor_open(ctx,
                          lexicon,
                          string_raw->value,
                          string_raw->length,
                          mode,
                          flags);
  if (!token_cursor) {
    return;
  }

  while (token_cursor->status == GRN_TOKEN_CURSOR_DOING) {
    grn_id token_id = grn_token_cursor_next(ctx, token_cursor);
    grn_token *token;
    tokenize_token *current_token;

    if (token_id == GRN_ID_NIL) {
      continue;
    }
    token = grn_token_cursor_get_token(ctx, token_cursor);
    grn_bulk_space(ctx, tokens, sizeof(tokenize_token));
    current_token = ((tokenize_token *)(GRN_BULK_CURR(tokens))) - 1;
    current_token->id = token_id;
    current_token->position = (int32_t)grn_token_get_position(ctx, token);
    current_token->force_prefix_search =
      grn_token_get_force_prefix_search(ctx, token);
    current_token->source_offset = grn_token_get_source_offset(ctx, token);
    current_token->source_length = grn_token_get_source_length(ctx, token);
    current_token->source_first_character_length =
      grn_token_get_source_first_character_length(ctx, token);

    {
      grn_obj *metadata;
      size_t n_metadata;
      size_t i;
      grn_obj name;
      grn_obj value;

      GRN_TEXT_INIT(&(current_token->metadata), GRN_OBJ_VECTOR);
      metadata = grn_token_get_metadata(ctx, token);
      n_metadata = grn_token_metadata_get_size(ctx, metadata);
      GRN_TEXT_INIT(&name, 0);
      GRN_VOID_INIT(&value);
      for (i = 0; i < n_metadata; i++) {
        GRN_BULK_REWIND(&name);
        GRN_BULK_REWIND(&value);
        grn_token_metadata_at(ctx, metadata, i, &name, &value);
        if (GRN_TEXT_LEN(&name) == 0) {
          continue;
        }
        grn_vector_add_element(ctx,
                               &(current_token->metadata),
                               GRN_BULK_HEAD(&name),
                               (uint32_t)GRN_BULK_VSIZE(&name),
                               0,
                               name.header.domain);
        grn_vector_add_element(ctx,
                               &(current_token->metadata),
                               GRN_BULK_HEAD(&value),
                               (uint32_t)GRN_BULK_VSIZE(&value),
                               0,
                               value.header.domain);
      }
      GRN_OBJ_FIN(ctx, &name);
      GRN_OBJ_FIN(ctx, &value);
    }
  }
  grn_token_cursor_close(ctx, token_cursor);
}

static grn_obj *
command_table_tokenize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_raw_string table_raw;
  grn_raw_string string_raw;
  grn_obj *flags_raw;
  grn_obj *mode_raw;
  grn_raw_string index_column_raw;

#define GET_VALUE(name)                                         \
  name ## _raw.value =                                          \
    grn_plugin_proc_get_var_string(ctx,                         \
                                   user_data,                   \
                                   #name,                       \
                                   strlen(#name),               \
                                   &(name ## _raw.length))

  GET_VALUE(table);
  GET_VALUE(string);
  flags_raw = grn_plugin_proc_get_var(ctx, user_data, "flags", strlen("flags"));
  mode_raw = grn_plugin_proc_get_var(ctx, user_data, "mode", strlen("mode"));
  GET_VALUE(index_column);

#undef GET_VALUE

  if (table_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[table_tokenize] table name is missing");
    return NULL;
  }

  if (string_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                     "[table_tokenize] string is missing");
    return NULL;
  }

  {
    uint32_t flags;
    grn_obj *lexicon;
    grn_obj *index_column = NULL;

    flags = grn_proc_get_value_token_cursor_flags(ctx,
                                                  flags_raw,
                                                  0,
                                                  "[table_tokenize][flags]");
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }

    lexicon = grn_ctx_get(ctx,
                          table_raw.value,
                          (int)(table_raw.length));
    if (!lexicon) {
      GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                       "[table_tokenize] nonexistent lexicon: <%.*s>",
                       (int)table_raw.length,
                       table_raw.value);
      return NULL;
    }

    if (index_column_raw.length > 0) {
      index_column = grn_obj_column(ctx, lexicon,
                                    index_column_raw.value,
                                    (uint32_t)(index_column_raw.length));
      if (!index_column) {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "[table_tokenize] nonexistent index column: <%.*s>",
                         (int)index_column_raw.length,
                         index_column_raw.value);
        goto exit;
      }
      if (index_column->header.type != GRN_COLUMN_INDEX) {
        GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT,
                         "[table_tokenize] "
                         "index column must be COLUMN_INDEX: <%.*s>",
                         (int)index_column_raw.length,
                         index_column_raw.value);
        goto exit;
      }
    }

    {
      grn_obj tokens;
      init_tokens(ctx, &tokens);
      grn_tokenize_mode mode =
        grn_proc_get_value_tokenize_mode(ctx,
                                         mode_raw,
                                         GRN_TOKENIZE_GET,
                                         "[table_tokenize][mode]");
      if (ctx->rc == GRN_SUCCESS) {
        tokenize(ctx, lexicon, &string_raw, mode, flags, &tokens);
        output_tokens(ctx, &tokens, lexicon, index_column);
      }
      fin_tokens(ctx, &tokens);
    }
#undef MODE_NAME_EQUAL

exit:
    grn_obj_unlink(ctx, lexicon);
    if (index_column) {
      grn_obj_unlink(ctx, index_column);
    }
  }

  return NULL;
}

void
grn_proc_init_table_tokenize(grn_ctx *ctx)
{
  grn_expr_var vars[5];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "table", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "string", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "mode", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "index_column", -1);
  grn_plugin_command_create(ctx,
                            "table_tokenize", -1,
                            command_table_tokenize,
                            5,
                            vars);
}

static grn_obj *
command_tokenize(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  const char *context_tag = "[tokenize]";
  grn_raw_string tokenizer_raw;
  grn_raw_string string_raw;
  grn_raw_string normalizer_raw;
  grn_obj *flags_raw;
  grn_obj *mode_raw;
  grn_raw_string token_filters_raw;

#define GET_VALUE(name)                                         \
  name ## _raw.value =                                          \
    grn_plugin_proc_get_var_string(ctx,                         \
                                   user_data,                   \
                                   #name,                       \
                                   strlen(#name),               \
                                   &(name ## _raw.length))

  GET_VALUE(tokenizer);
  GET_VALUE(string);
  GET_VALUE(normalizer);
  flags_raw = grn_plugin_proc_get_var(ctx, user_data, "flags", strlen("flags"));
  mode_raw = grn_plugin_proc_get_var(ctx, user_data, "mode", strlen("mode"));
  GET_VALUE(token_filters);

#undef GET_VALUE

  if (tokenizer_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s tokenizer name is missing",
                     context_tag);
    return NULL;
  }

  if (string_raw.length == 0) {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "%s string is missing",
                     context_tag);
    return NULL;
  }

  {
    uint32_t flags;
    grn_obj *lexicon;

    flags = grn_proc_get_value_token_cursor_flags(ctx,
                                                  flags_raw,
                                                  0,
                                                  "[tokenize][flags]");
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }

    lexicon = grn_proc_lexicon_open(ctx,
                                    &tokenizer_raw,
                                    &normalizer_raw,
                                    &token_filters_raw,
                                    context_tag);
    if (!lexicon) {
      return NULL;
    }

    {
      grn_obj tokens;
      init_tokens(ctx, &tokens);
      grn_tokenize_mode mode =
        grn_proc_get_value_tokenize_mode(ctx,
                                         mode_raw,
                                         GRN_TOKENIZE_ADD,
                                         "[tokenize][mode]");
      if (ctx->rc == GRN_SUCCESS) {
        tokenize(ctx, lexicon, &string_raw, GRN_TOKENIZE_ADD, flags, &tokens);
        if (mode != GRN_TOKENIZE_ADD) {
          GRN_BULK_REWIND(&tokens);
          tokenize(ctx, lexicon, &string_raw, mode, flags, &tokens);
        }
        output_tokens(ctx, &tokens, lexicon, NULL);
      }
      fin_tokens(ctx, &tokens);
    }
#undef MODE_NAME_EQUAL

    grn_obj_unlink(ctx, lexicon);
  }

  return NULL;
}

void
grn_proc_init_tokenize(grn_ctx *ctx)
{
  grn_expr_var vars[6];

  grn_plugin_expr_var_init(ctx, &(vars[0]), "tokenizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[1]), "string", -1);
  grn_plugin_expr_var_init(ctx, &(vars[2]), "normalizer", -1);
  grn_plugin_expr_var_init(ctx, &(vars[3]), "flags", -1);
  grn_plugin_expr_var_init(ctx, &(vars[4]), "mode", -1);
  grn_plugin_expr_var_init(ctx, &(vars[5]), "token_filters", -1);
  grn_plugin_command_create(ctx,
                            "tokenize", -1,
                            command_tokenize,
                            6,
                            vars);
}
