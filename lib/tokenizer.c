/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2012 Brazil

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
#include "groonga_in.h"
#include "groonga/tokenizer.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ctx.h"
#include "db.h"
#include "str.h"
#include "string_in.h"
#include "token.h"

/*
  grn_tokenizer_charlen() takes the length of a string, unlike grn_charlen_().
 */
int
grn_tokenizer_charlen(grn_ctx *ctx, const char *str_ptr,
                      unsigned int str_length, grn_encoding encoding)
{
  return grn_charlen_(ctx, str_ptr, str_ptr + str_length, encoding);
}

/*
  grn_tokenizer_isspace() takes the length of a string, unlike grn_isspace().
 */
int
grn_tokenizer_isspace(grn_ctx *ctx, const char *str_ptr,
                      unsigned int str_length, grn_encoding encoding)
{
  if ((str_ptr == NULL) || (str_length == 0)) {
    return 0;
  }
  switch ((unsigned char)str_ptr[0]) {
  case ' ' :
  case '\f' :
  case '\n' :
  case '\r' :
  case '\t' :
  case '\v' :
    return 1;
  case 0x81 :
    if ((encoding == GRN_ENC_SJIS) && (str_length >= 2) &&
        ((unsigned char)str_ptr[1] == 0x40)) {
      return 2;
    }
    break;
  case 0xA1 :
    if ((encoding == GRN_ENC_EUC_JP) && (str_length >= 2) &&
        ((unsigned char)str_ptr[1] == 0xA1)) {
      return 2;
    }
    break;
  case 0xE3 :
    if ((encoding == GRN_ENC_UTF8) && (str_length >= 3) &&
        ((unsigned char)str_ptr[1] == 0x80) &&
        ((unsigned char)str_ptr[2] == 0x80)) {
      return 3;
    }
    break;
  default :
    break;
  }
  return 0;
}

grn_bool
grn_tokenizer_is_tokenized_delimiter(grn_ctx *ctx,
                                     const char *str_ptr,
                                     unsigned int str_length,
                                     grn_encoding encoding)
{
  const unsigned char *binary_string = str_ptr;

  if (encoding != GRN_ENC_UTF8) {
    return GRN_FALSE;
  }

  if (str_length != GRN_TOKENIZER_TOKENIZED_DELIMITER_UTF8_LEN) {
    return GRN_FALSE;
  }

  return memcmp(binary_string,
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

grn_tokenizer_query *
grn_tokenizer_query_open(grn_ctx *ctx, int num_args, grn_obj **args)
{
  grn_obj *query_str = grn_ctx_pop(ctx);
  if (query_str == NULL) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "missing argument");
    return NULL;
  }

  if ((num_args < 1) || (args == NULL) || (args[0] == NULL)) {
    GRN_PLUGIN_ERROR(ctx, GRN_INVALID_ARGUMENT, "invalid NULL pointer");
    return NULL;
  }

  {
    grn_tokenizer_query * const query =
        GRN_PLUGIN_MALLOC(ctx, sizeof(grn_tokenizer_query));
    if (query == NULL) {
      return NULL;
    }
    query->normalized_query = NULL;
    query->query_buf = NULL;

    {
      grn_obj * const table = args[0];
      grn_obj_flags table_flags;
      grn_encoding table_encoding;
      unsigned int query_length = GRN_TEXT_LEN(query_str);
      char *query_buf = (char *)GRN_PLUGIN_MALLOC(ctx, query_length + 1);
      if (query_buf == NULL) {
        GRN_PLUGIN_FREE(ctx, query);
        GRN_PLUGIN_ERROR(ctx, GRN_TOKENIZER_ERROR,
                         "[tokenizer] failed to duplicate query");
        return NULL;
      }
      grn_table_get_info(ctx, table, &table_flags, &table_encoding, NULL);
      {
        grn_obj *normalizer = NULL;
        int flags = 0;
        grn_obj *normalized_string;
        if (table_flags & GRN_OBJ_KEY_NORMALIZE) {
          normalizer = GRN_NORMALIZER_AUTO;
        }
        normalized_string = grn_string_open_(ctx,
                                             GRN_TEXT_VALUE(query_str),
                                             GRN_TEXT_LEN(query_str),
                                             normalizer,
                                             flags,
                                             table_encoding);
        if (!normalized_string) {
          GRN_PLUGIN_FREE(ctx, query);
          return NULL;
        }
        query->normalized_query = normalized_string;
        memcpy(query_buf, GRN_TEXT_VALUE(query_str), query_length);
        query_buf[query_length] = '\0';
        query->query_buf = query_buf;
        query->ptr = query_buf;
        query->length = query_length;
      }
      query->encoding = table_encoding;
    }
    return query;
  }
}

grn_tokenizer_query *
grn_tokenizer_query_create(grn_ctx *ctx, int num_args, grn_obj **args)
{
  return grn_tokenizer_query_open(ctx, num_args, args);
}

void
grn_tokenizer_query_close(grn_ctx *ctx, grn_tokenizer_query *query)
{
  if (query != NULL) {
    if (query->normalized_query != NULL) {
      grn_obj_unlink(ctx, query->normalized_query);
    }
    if (query->query_buf != NULL) {
      GRN_PLUGIN_FREE(ctx, query->query_buf);
    }
    GRN_PLUGIN_FREE(ctx, query);
  }
}

void
grn_tokenizer_query_destroy(grn_ctx *ctx, grn_tokenizer_query *query)
{
  grn_tokenizer_query_close(ctx, query);
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
}

void
grn_tokenizer_token_push(grn_ctx *ctx, grn_tokenizer_token *token,
                         const char *str_ptr, unsigned int str_length,
                         grn_tokenizer_status status)
{
  GRN_TEXT_SET_REF(&token->str, str_ptr, str_length);
  switch (status) {
  case GRN_TOKENIZER_CONTINUE :
    GRN_UINT32_SET(ctx, &token->status, 0);
    break;
  case GRN_TOKENIZER_LAST :
  default :
    GRN_UINT32_SET(ctx, &token->status, GRN_TOKEN_LAST);
    break;
  }
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
  const unsigned char *start = str_ptr;
  const unsigned char *current;
  const unsigned char *end = str_ptr + str_length;
  const char *next_start = NULL;
  unsigned int token_length;
  grn_tokenizer_status status;

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

grn_rc
grn_tokenizer_register(grn_ctx *ctx, const char *plugin_name_ptr,
                       unsigned int plugin_name_length,
                       grn_proc_func *init, grn_proc_func *next,
                       grn_proc_func *fin)
{
  grn_expr_var vars[] = {
    { NULL, 0 },
    { NULL, 0 },
    { NULL, 0 }
  };
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
