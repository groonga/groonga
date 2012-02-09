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
#include "groonga/tokenizer.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "ctx.h"
#include "db.h"
#include "str.h"
#include "token.h"

void *grn_tokenizer_malloc(grn_ctx *ctx, size_t size, const char *file,
                           int line, const char *func) {
  return grn_malloc(ctx, size, file, line, func);
}

void grn_tokenizer_free(grn_ctx *ctx, void *ptr, const char *file,
                        int line, const char *func) {
  return grn_free(ctx, ptr, file, line, func);
}

/*
  grn_tokenizer_ctx_log() is a clone of grn_ctx_log() in ctx.c. The only
  difference is that grn_tokenizer_ctx_log() uses va_list instead of `...'.
 */
static void grn_tokenizer_ctx_log(grn_ctx *ctx, const char *format,
                                  va_list ap) {
  va_list aq;
  va_copy(aq, ap);
  vsnprintf(ctx->errbuf, GRN_CTX_MSGSIZE, format, aq);
  va_end(aq);
}

void grn_tokenizer_set_error(grn_ctx *ctx, grn_log_level level,
                             grn_rc error_code,
                             const char *file, int line, const char *func,
                             const char *format, ...) {
  ctx->errlvl = level;
  ctx->rc = error_code;
  ctx->errfile = file;
  ctx->errline = line;
  ctx->errfunc = func;
  grn_ctx_impl_err(ctx);

  {
    va_list ap;
    va_start(ap, format);
    grn_tokenizer_ctx_log(ctx, format, ap);
    va_end(ap);
  }
}

void grn_tokenizer_backtrace(grn_ctx *ctx) {
  BACKTRACE(ctx);
}

void grn_tokenizer_logtrace(grn_ctx *ctx, grn_log_level level) {
  if (level <= GRN_LOG_ERROR) {
    LOGTRACE(ctx, level);
  }
}

struct _grn_tokenizer_mutex {
  grn_critical_section critical_section;
};

grn_tokenizer_mutex *grn_tokenizer_mutex_create(grn_ctx *ctx) {
  grn_tokenizer_mutex * const mutex =
      GRN_TOKENIZER_MALLOC(ctx, sizeof(grn_tokenizer_mutex));
  if (mutex != NULL) {
    CRITICAL_SECTION_INIT(mutex->critical_section);
  }
  return mutex;
}

void grn_tokenizer_mutex_destroy(grn_ctx *ctx, grn_tokenizer_mutex *mutex) {
  if (mutex != NULL) {
    CRITICAL_SECTION_FIN(mutex->critical_section);
    GRN_TOKENIZER_FREE(ctx, mutex);
  }
}

void grn_tokenizer_mutex_lock(grn_ctx *ctx, grn_tokenizer_mutex *mutex) {
  if (mutex != NULL) {
    CRITICAL_SECTION_ENTER(mutex->critical_section);
  }
}

void grn_tokenizer_mutex_unlock(grn_ctx *ctx, grn_tokenizer_mutex *mutex) {
  if (mutex != NULL) {
    CRITICAL_SECTION_LEAVE(mutex->critical_section);
  }
}

/*
  grn_tokenizer_charlen() takes the length of a string, unlike grn_charlen_().
 */
int grn_tokenizer_charlen(grn_ctx *ctx, const char *str_ptr,
                          unsigned int str_length, grn_encoding encoding) {
  return grn_charlen_(ctx, str_ptr, str_ptr + str_length, encoding);
}

/*
  grn_tokenizer_isspace() takes the length of a string, unlike grn_isspace().
 */
int grn_tokenizer_isspace(grn_ctx *ctx, const char *str_ptr,
                          unsigned int str_length, grn_encoding encoding) {
  if ((str_ptr == NULL) || (str_length == 0)) {
    return 0;
  }
  switch ((unsigned char)str_ptr[0]) {
    case ' ':
    case '\f':
    case '\n':
    case '\r':
    case '\t':
    case '\v': {
      return 1;
    }
    case 0x81: {
      if ((encoding == GRN_ENC_SJIS) && (str_length >= 2) &&
          ((unsigned char)str_ptr[1] == 0x40)) {
        return 2;
      }
      break;
    }
    case 0xA1: {
      if ((encoding == GRN_ENC_EUC_JP) && (str_length >= 2) &&
          ((unsigned char)str_ptr[1] == 0xA1)) {
        return 2;
      }
      break;
    }
    case 0xE3: {
      if ((encoding == GRN_ENC_UTF8) && (str_length >= 3) &&
          ((unsigned char)str_ptr[1] == 0x80) &&
          ((unsigned char)str_ptr[2] == 0x80)) {
        return 3;
      }
      break;
    }
    default:
      break;
  }
  return 0;
}

grn_tokenizer_query *grn_tokenizer_query_create(grn_ctx *ctx,
                                                int num_args, grn_obj **args) {
  grn_obj *query_str = grn_ctx_pop(ctx);
  if (query_str == NULL) {
    GRN_TOKENIZER_ERROR(ctx, GRN_INVALID_ARGUMENT, "missing argument");
    return NULL;
  }

  if ((num_args < 1) || (args == NULL) || (args[0] == NULL)) {
    GRN_TOKENIZER_ERROR(ctx, GRN_INVALID_ARGUMENT, "invalid NULL pointer");
    return NULL;
  }

  {
    grn_tokenizer_query * const query =
        GRN_TOKENIZER_MALLOC(ctx, sizeof(grn_tokenizer_query));
    if (query == NULL) {
      return NULL;
    }

    {
      grn_obj * const table = args[0];
      grn_obj_flags table_flags;
      grn_encoding table_encoding;
      grn_table_get_info(ctx, table, &table_flags, &table_encoding, NULL);
      {
        grn_str * const str = grn_str_open_(ctx, GRN_TEXT_VALUE(query_str),
                                            GRN_TEXT_LEN(query_str),
                                            table_flags & GRN_OBJ_KEY_NORMALIZE,
                                            table_encoding);
        if (str == NULL) {
          GRN_TOKENIZER_FREE(ctx, query);
          return NULL;
        }
        query->str = str;
      }
      query->ptr = query->str->norm;
      query->length = query->str->norm_blen;
      query->encoding = table_encoding;
    }
    return query;
  }
}

void grn_tokenizer_query_destroy(grn_ctx *ctx, grn_tokenizer_query *query) {
  if (query != NULL) {
    if (query->str != NULL) {
      grn_str_close(ctx, query->str);
    }
    GRN_TOKENIZER_FREE(ctx, query);
  }
}

void grn_tokenizer_token_init(grn_ctx *ctx, grn_tokenizer_token *token) {
  GRN_TEXT_INIT(&token->str, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_UINT32_INIT(&token->status, 0);
}

void grn_tokenizer_token_fin(grn_ctx *ctx, grn_tokenizer_token *token) {
}

void grn_tokenizer_token_push(grn_ctx *ctx, grn_tokenizer_token *token,
                              const char *str_ptr, unsigned int str_length,
                              grn_tokenizer_status status) {
  GRN_TEXT_SET_REF(&token->str, str_ptr, str_length);
  switch (status) {
    case GRN_TOKENIZER_CONTINUE: {
      GRN_UINT32_SET(ctx, &token->status, 0);
      break;
    }
    case GRN_TOKENIZER_LAST:
    default: {
      GRN_UINT32_SET(ctx, &token->status, GRN_TOKEN_LAST);
      break;
    }
  }
  grn_ctx_push(ctx, &token->str);
  grn_ctx_push(ctx, &token->status);
}

grn_rc grn_tokenizer_register(grn_ctx *ctx, const char *plugin_name_ptr,
                              unsigned int plugin_name_length,
                              grn_proc_func *init, grn_proc_func *next,
                              grn_proc_func *fin) {
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
      GRN_TOKENIZER_ERROR(ctx, GRN_TOKENIZER_ERROR,
                          "grn_proc_create() failed");
      return ctx->rc;
    }
  }
  return GRN_SUCCESS;
}
