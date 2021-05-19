/*
  Copyright(C) 2012-2018  Brazil
  Copyright(C) 2018-2020  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_tokenizer.h"

#include <string.h>

#include "grn_ctx.h"
#include "grn_db.h"
#include "grn_str.h"
#include "grn_string.h"
#include "grn_token_cursor.h"

/*
  Just for backward compatibility. See grn_plugin_charlen() instead.
 */
int
grn_tokenizer_charlen(grn_ctx *ctx, const char *str_ptr,
                      unsigned int str_length, grn_encoding encoding)
{
  return grn_plugin_charlen(ctx, str_ptr, str_length, encoding);
}

/*
  Just for backward compatibility. See grn_plugin_isspace() instead.
 */
int
grn_tokenizer_isspace(grn_ctx *ctx, const char *str_ptr,
                      unsigned int str_length, grn_encoding encoding)
{
  return grn_plugin_isspace(ctx, str_ptr, str_length, encoding);
}

grn_bool
grn_tokenizer_is_tokenized_delimiter(grn_ctx *ctx,
                                     const char *str_ptr,
                                     unsigned int str_length,
                                     grn_encoding encoding)
{
  if (encoding != GRN_ENC_UTF8) {
    return GRN_FALSE;
  }

  if (str_length != GRN_TOKENIZER_TOKENIZED_DELIMITER_UTF8_LEN) {
    return GRN_FALSE;
  }

  return memcmp(str_ptr,
                GRN_TOKENIZER_TOKENIZED_DELIMITER_UTF8,
                GRN_TOKENIZER_TOKENIZED_DELIMITER_UTF8_LEN) == 0;
}

grn_bool
grn_tokenizer_have_tokenized_delimiter(grn_ctx *ctx,
                                       const char *str_ptr,
                                       unsigned int str_length,
                                       grn_encoding encoding)
{
  int char_length;
  const char *current = str_ptr;
  const char *end = str_ptr + str_length;

  if (encoding != GRN_ENC_UTF8) {
    return GRN_FALSE;
  }

  if (str_length == 0) {
    return GRN_FALSE;
  }

  while ((char_length = grn_charlen_(ctx, current, end, encoding)) > 0) {
    if (grn_tokenizer_is_tokenized_delimiter(ctx,
                                             current, char_length,
                                             encoding)) {
      return GRN_TRUE;
    }
    current += char_length;
  }
  return GRN_FALSE;
}

static void
grn_tokenizer_query_ensure_normalized(grn_ctx *ctx, grn_tokenizer_query *query)
{
  if (!query->need_normalize) {
    return;
  }

  query->need_normalize = GRN_FALSE;

  if (query->normalized_query) {
    grn_obj_close(ctx, query->normalized_query);
  }
  query->normalized_query = grn_string_open_(ctx,
                                             query->ptr,
                                             query->length,
                                             query->lexicon,
                                             query->normalize_flags,
                                             query->encoding);
  if (!query->normalized_query) {
    query->have_tokenized_delimiter = GRN_FALSE;
    GRN_PLUGIN_ERROR(ctx, GRN_TOKENIZER_ERROR,
                     "[tokenizer][normalize] failed to open normalized string");
    return;
  }

  query->need_delimiter_check = GRN_TRUE;
}

static void
grn_tokenizer_query_ensure_have_tokenized_delimiter(grn_ctx *ctx,
                                                    grn_tokenizer_query *query)
{
  grn_tokenizer_query_ensure_normalized(ctx, query);

  if (!query->need_delimiter_check) {
    return;
  }

  query->need_delimiter_check = GRN_FALSE;

  if (query->flags & GRN_TOKEN_CURSOR_ENABLE_TOKENIZED_DELIMITER) {
    const char *normalized_string;
    unsigned int normalized_string_length;

    grn_string_get_normalized(ctx,
                              query->normalized_query,
                              &normalized_string,
                              &normalized_string_length,
                              NULL);
    query->have_tokenized_delimiter =
      grn_tokenizer_have_tokenized_delimiter(ctx,
                                             normalized_string,
                                             normalized_string_length,
                                             query->encoding);
  } else {
    query->have_tokenized_delimiter = GRN_FALSE;
  }
}

grn_rc
grn_tokenizer_query_init(grn_ctx *ctx, grn_tokenizer_query *query)
{
  query->normalize_flags = 0;
  query->normalized_query = NULL;
  query->query_buf = NULL;
  query->ptr = NULL;
  query->length = 0;
  query->flags = 0;
  query->tokenize_mode = GRN_TOKENIZE_ADD;
  query->token_mode = query->tokenize_mode;
  query->lexicon = NULL;
  query->encoding = ctx->encoding;
  query->token_filter_index = 0;
  query->source_column = NULL;
  query->source_id = GRN_ID_NIL;
  query->index_column = NULL;

  query->need_normalize = GRN_TRUE;
  query->need_delimiter_check = GRN_TRUE;

  query->options = NULL;

  return ctx->rc;
}

grn_tokenizer_query *
grn_tokenizer_query_open(grn_ctx *ctx, int num_args, grn_obj **args,
                         uint32_t normalize_flags)
{
  grn_obj *flags;
  grn_obj *query_str;
  grn_obj *tokenize_mode;

  GRN_API_ENTER;

  flags = grn_ctx_pop(ctx);
  query_str = grn_ctx_pop(ctx);
  tokenize_mode = grn_ctx_pop(ctx);

  if (query_str == NULL) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "missing argument");
    GRN_API_RETURN(NULL);
  }

  if ((num_args < 1) || (args == NULL) || (args[0] == NULL)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "invalid NULL pointer");
    GRN_API_RETURN(NULL);
  }

  {
    grn_tokenizer_query * const query =
        GRN_PLUGIN_MALLOC(ctx, sizeof(grn_tokenizer_query));
    if (!query) {
      GRN_API_RETURN(NULL);
    }
    grn_tokenizer_query_init(ctx, query);
    grn_tokenizer_query_set_raw_string(ctx,
                                       query,
                                       GRN_TEXT_VALUE(query_str),
                                       GRN_TEXT_LEN(query_str));
    if (ctx->rc != GRN_SUCCESS) {
      GRN_PLUGIN_FREE(ctx, query);
      GRN_API_RETURN(NULL);
    }
    if (flags) {
      grn_tokenizer_query_set_flags(ctx, query, GRN_UINT32_VALUE(flags));
    }
    if (tokenize_mode) {
      grn_tokenizer_query_set_mode(ctx, query, GRN_UINT32_VALUE(tokenize_mode));
    }
    grn_tokenizer_query_set_normalize_flags(ctx, query, normalize_flags);
    grn_tokenizer_query_set_lexicon(ctx, query, args[0]);

    grn_tokenizer_query_ensure_have_tokenized_delimiter(ctx, query);

    GRN_API_RETURN(query);
  }
}

grn_tokenizer_query *
grn_tokenizer_query_create(grn_ctx *ctx, int num_args, grn_obj **args)
{
  return grn_tokenizer_query_open(ctx, num_args, args, 0);
}

void
grn_tokenizer_query_fin(grn_ctx *ctx, grn_tokenizer_query *query)
{
  if (query->normalized_query) {
    grn_obj_unlink(ctx, query->normalized_query);
  }
  if (query->query_buf) {
    GRN_PLUGIN_FREE(ctx, query->query_buf);
  }
}

void
grn_tokenizer_query_close(grn_ctx *ctx, grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  if (query) {
    grn_tokenizer_query_fin(ctx, query);
    GRN_PLUGIN_FREE(ctx, query);
  }
  GRN_API_RETURN();
}

void
grn_tokenizer_query_destroy(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_tokenizer_query_close(ctx, query);
}

grn_rc
grn_tokenizer_query_set_normalize_flags(grn_ctx *ctx,
                                        grn_tokenizer_query *query,
                                        uint32_t normalize_flags)
{
  GRN_API_ENTER;
  if (query->normalize_flags != normalize_flags) {
    query->normalize_flags = normalize_flags;
    query->need_normalize = GRN_TRUE;
  }
  GRN_API_RETURN(ctx->rc);
}

uint32_t
grn_tokenizer_query_get_normalize_flags(grn_ctx *ctx,
                                        grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->normalize_flags);
}

grn_obj *
grn_tokenizer_query_get_normalized_string(grn_ctx *ctx,
                                          grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  grn_tokenizer_query_ensure_normalized(ctx, query);
  GRN_API_RETURN(query->normalized_query);
}

grn_rc
grn_tokenizer_query_set_raw_string(grn_ctx *ctx,
                                   grn_tokenizer_query *query,
                                   const char *string,
                                   size_t string_length)
{
  GRN_API_ENTER;

  if (query->query_buf) {
    GRN_PLUGIN_FREE(ctx, query->query_buf);
  }

  if (string_length == 0) {
    query->query_buf = NULL;
    query->ptr = NULL;
    query->length = 0;
    query->need_normalize = GRN_TRUE;
  } else {
    query->query_buf = (char *)GRN_PLUGIN_MALLOC(ctx, string_length + 1);
    if (!query->query_buf) {
      GRN_PLUGIN_ERROR(ctx, GRN_TOKENIZER_ERROR,
                       "[tokenizer][query] failed to duplicate query");
      GRN_API_RETURN(ctx->rc);
    }
    grn_memcpy(query->query_buf, string, string_length);
    query->query_buf[string_length] = '\0';
    query->ptr = query->query_buf;
    query->length = string_length;
  }

  GRN_API_RETURN(ctx->rc);
}

const char *
grn_tokenizer_query_get_raw_string(grn_ctx *ctx,
                                   grn_tokenizer_query *query,
                                   size_t *length)
{
  GRN_API_ENTER;
  if (length) {
    *length = query->length;
  }
  GRN_API_RETURN(query->ptr);
}

grn_encoding
grn_tokenizer_query_get_encoding(grn_ctx *ctx, grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->encoding);
}

grn_rc
grn_tokenizer_query_set_flags(grn_ctx *ctx,
                              grn_tokenizer_query *query,
                              uint32_t flags)
{
  GRN_API_ENTER;
  if (query->flags != flags) {
    query->flags = flags;
    query->need_normalize = GRN_TRUE;
  }
  GRN_API_RETURN(ctx->rc);
}

uint32_t
grn_tokenizer_query_get_flags(grn_ctx *ctx, grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->flags);
}

grn_bool
grn_tokenizer_query_have_tokenized_delimiter(grn_ctx *ctx,
                                             grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  grn_tokenizer_query_ensure_have_tokenized_delimiter(ctx, query);
  GRN_API_RETURN(query->have_tokenized_delimiter);
}

grn_rc
grn_tokenizer_query_set_mode(grn_ctx *ctx,
                             grn_tokenizer_query *query,
                             grn_tokenize_mode mode)
{
  GRN_API_ENTER;
  if (query->tokenize_mode != mode) {
    query->tokenize_mode = mode;
    query->token_mode = query->tokenize_mode;
    query->need_normalize = GRN_TRUE;
  }
  GRN_API_RETURN(ctx->rc);
}

grn_tokenize_mode
grn_tokenizer_query_get_mode(grn_ctx *ctx, grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->tokenize_mode);
}

grn_rc
grn_tokenizer_query_set_lexicon(grn_ctx *ctx,
                                grn_tokenizer_query *query,
                                grn_obj *lexicon)
{
  GRN_API_ENTER;

  if (query->lexicon != lexicon) {
    query->lexicon = lexicon;
    if (query->lexicon) {
      grn_table_get_info(ctx,
                         query->lexicon,
                         NULL,
                         &(query->encoding),
                         NULL,
                         NULL,
                         NULL);
    } else {
      query->encoding = ctx->encoding;
    }
    query->need_normalize = GRN_TRUE;
  }

  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_tokenizer_query_get_lexicon(grn_ctx *ctx, grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->lexicon);
}

grn_rc
grn_tokenizer_query_set_token_filter_index(grn_ctx *ctx,
                                           grn_tokenizer_query *query,
                                           unsigned int index)
{
  GRN_API_ENTER;
  query->token_filter_index = index;
  GRN_API_RETURN(ctx->rc);
}

unsigned int
grn_tokenizer_query_get_token_filter_index(grn_ctx *ctx,
                                           grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->token_filter_index);
}

grn_rc
grn_tokenizer_query_set_source_column(grn_ctx *ctx,
                                      grn_tokenizer_query *query,
                                      grn_obj *column)
{
  GRN_API_ENTER;
  query->source_column = column;
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_tokenizer_query_get_source_column(grn_ctx *ctx,
                                      grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->source_column);
}

grn_rc
grn_tokenizer_query_set_source_id(grn_ctx *ctx,
                                  grn_tokenizer_query *query,
                                  grn_id id)
{
  GRN_API_ENTER;
  query->source_id = id;
  GRN_API_RETURN(ctx->rc);
}

grn_id
grn_tokenizer_query_get_source_id(grn_ctx *ctx,
                                  grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->source_id);
}

grn_obj *
grn_tokenizer_query_get_index_column(grn_ctx *ctx,
                                     grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->index_column);
}

grn_rc
grn_tokenizer_query_set_index_column(grn_ctx *ctx,
                                     grn_tokenizer_query *query,
                                     grn_obj *column)
{
  GRN_API_ENTER;
  query->index_column = column;
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_tokenizer_query_get_options(grn_ctx *ctx,
                                grn_tokenizer_query *query)
{
  GRN_API_ENTER;
  GRN_API_RETURN(query->options);
}

grn_rc
grn_tokenizer_query_set_options(grn_ctx *ctx,
                                grn_tokenizer_query *query,
                                grn_obj *options)
{
  GRN_API_ENTER;
  query->options = options;
  GRN_API_RETURN(ctx->rc);
}

void
grn_tokenizer_token_init(grn_ctx *ctx, grn_tokenizer_token *token)
{
  GRN_TEXT_INIT(&token->str, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_UINT32_INIT(&token->status, 0);
}

void
grn_tokenizer_token_fin(grn_ctx *ctx, grn_tokenizer_token *token)
{
  GRN_OBJ_FIN(ctx, &(token->str));
  GRN_OBJ_FIN(ctx, &(token->status));
}

void
grn_tokenizer_token_push(grn_ctx *ctx, grn_tokenizer_token *token,
                         const char *str_ptr, unsigned int str_length,
                         grn_token_status status)
{
  GRN_TEXT_SET_REF(&token->str, str_ptr, str_length);
  GRN_UINT32_SET(ctx, &token->status, status);
  grn_ctx_push(ctx, &token->str);
  grn_ctx_push(ctx, &token->status);
}

const char *
grn_tokenizer_tokenized_delimiter_next(grn_ctx *ctx,
                                       grn_tokenizer_token *token,
                                       const char *str_ptr,
                                       unsigned int str_length,
                                       grn_encoding encoding)
{
  size_t char_length = 0;
  const char *start = str_ptr;
  const char *current;
  const char *end = str_ptr + str_length;
  const char *next_start = NULL;
  unsigned int token_length;
  grn_token_status status;

  for (current = start; current < end; current += char_length) {
    char_length = grn_charlen_(ctx, current, end, encoding);
    if (char_length == 0) {
      break;
    }
    if (grn_tokenizer_is_tokenized_delimiter(ctx, current, char_length,
                                             encoding)) {
      next_start = str_ptr + (current - start + char_length);
      break;
    }
  }

  token_length = current - start;
  if (current == end) {
    status = GRN_TOKENIZER_LAST;
  } else {
    status = GRN_TOKENIZER_CONTINUE;
  }
  grn_tokenizer_token_push(ctx, token, start, token_length, status);

  return next_start;
}

const char *
grn_tokenizer_next_by_tokenized_delimiter(grn_ctx *ctx,
                                          grn_token *token,
                                          const char *str_ptr,
                                          unsigned int str_length,
                                          grn_encoding encoding)
{
  size_t char_length = 0;
  const char *start = str_ptr;
  const char *current;
  const char *end = str_ptr + str_length;
  const char *next_start = NULL;

  for (current = start; current < end; current += char_length) {
    char_length = grn_charlen_(ctx, current, end, encoding);
    if (char_length == 0) {
      break;
    }
    if (grn_tokenizer_is_tokenized_delimiter(ctx, current, char_length,
                                             encoding)) {
      next_start = str_ptr + (current - start + char_length);
      break;
    }
  }

  grn_token_set_data(ctx, token, start, current - start);
  if (current == end) {
    grn_token_set_status(ctx, token, GRN_TOKEN_LAST);
  } else {
    grn_token_set_status(ctx, token, GRN_TOKEN_CONTINUE);
  }

  return next_start;
}

grn_rc
grn_tokenizer_register(grn_ctx *ctx, const char *plugin_name_ptr,
                       unsigned int plugin_name_length,
                       grn_proc_func *init, grn_proc_func *next,
                       grn_proc_func *fin)
{
  grn_expr_var vars[3];
  memset(vars, 0, sizeof(vars));
  GRN_TEXT_INIT(&vars[0].value, 0);
  GRN_TEXT_INIT(&vars[1].value, 0);
  GRN_UINT32_INIT(&vars[2].value, 0);

  {
    /*
      grn_proc_create() registers a plugin to the database which is associated
      with `ctx'. A returned object must not be finalized here.
     */
    grn_obj * const obj = grn_proc_create(ctx, plugin_name_ptr,
                                          plugin_name_length,
                                          GRN_PROC_TOKENIZER,
                                          init, next, fin, 3, vars);
    if (obj == NULL) {
      GRN_PLUGIN_ERROR(ctx, GRN_TOKENIZER_ERROR, "grn_proc_create() failed");
      return ctx->rc;
    }
  }
  return GRN_SUCCESS;
}

grn_obj *
grn_tokenizer_create(grn_ctx *ctx,
                     const char *name,
                     int name_length)
{
  grn_obj *tokenizer;

  GRN_API_ENTER;
  tokenizer = grn_proc_create(ctx,
                              name,
                              name_length,
                              GRN_PROC_TOKENIZER,
                              NULL,
                              NULL,
                              NULL,
                              0,
                              NULL);
  if (!tokenizer) {
    if (name_length < 0) {
      name_length = strlen(name);
    }
    GRN_PLUGIN_ERROR(ctx,
                     GRN_TOKENIZER_ERROR,
                     "[tokenizer][create] failed to create: <%.*s>",
                     name_length, name);
  }

  GRN_API_RETURN(tokenizer);
}

grn_rc
grn_tokenizer_set_init_func(grn_ctx *ctx,
                            grn_obj *tokenizer,
                            grn_tokenizer_init_func *init)
{
  GRN_API_ENTER;
  if (tokenizer) {
    ((grn_proc *)tokenizer)->callbacks.tokenizer.init = init;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[tokenizer][init][set] tokenizer is NULL");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_tokenizer_set_next_func(grn_ctx *ctx,
                            grn_obj *tokenizer,
                            grn_tokenizer_next_func *next)
{
  GRN_API_ENTER;
  if (tokenizer) {
    ((grn_proc *)tokenizer)->callbacks.tokenizer.next = next;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[tokenizer][next][set] tokenizer is NULL");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_tokenizer_set_fin_func(grn_ctx *ctx,
                           grn_obj *tokenizer,
                           grn_tokenizer_fin_func *fin)
{
  GRN_API_ENTER;
  if (tokenizer) {
    ((grn_proc *)tokenizer)->callbacks.tokenizer.fin = fin;
  } else {
    GRN_PLUGIN_ERROR(ctx,
                     GRN_INVALID_ARGUMENT,
                     "[tokenizer][fin][set] tokenizer is NULL");
  }
  GRN_API_RETURN(ctx->rc);
}
