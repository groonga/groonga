/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009-2011 Brazil

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

#include <ctx.h>
#include <db.h>

#include <str.h>
#include <token.h>

#include <groonga.h>
#include <groonga/tokenizer.h>

#include <mecab.h>

#include <string.h>
#include <ctype.h>

static mecab_t *sole_mecab = NULL;
static grn_critical_section sole_mecab_lock;
static grn_encoding sole_mecab_encoding = GRN_ENC_NONE;

typedef struct {
  grn_str *nstr;
  mecab_t *mecab;
  char *buf;
  char *next;
  char *end;
  grn_encoding encoding;
  grn_tokenizer_token token;
  grn_bool have_tokenized_delimiter;
} grn_mecab_tokenizer;

static grn_encoding
translate_mecab_charset_to_grn_encoding(const char *charset)
{
  if (strcasecmp(charset, "euc-jp") == 0) {
    return GRN_ENC_EUC_JP;
  } else if (strcasecmp(charset, "utf-8") == 0 ||
             strcasecmp(charset, "utf8") == 0) {
    return GRN_ENC_UTF8;
  } else if (strcasecmp(charset, "shift_jis") == 0 ||
             strcasecmp(charset, "shift-jis") == 0 ||
             strcasecmp(charset, "sjis") == 0) {
    return GRN_ENC_SJIS;
  }
  return GRN_ENC_NONE;
}

static grn_encoding
get_mecab_encoding(mecab_t *mecab)
{
  grn_encoding encoding = GRN_ENC_NONE;
  const mecab_dictionary_info_t *dictionary_info;
  dictionary_info = mecab_dictionary_info(mecab);
  if (dictionary_info) {
    const char *charset = dictionary_info->charset;
    encoding = translate_mecab_charset_to_grn_encoding(charset);
  }
  return encoding;
}

/*
  This function is called for a full text search query or a document to be
  indexed. This means that both short/long strings are given.
  The return value of this function is ignored. When an error occurs in this
  function, `ctx->rc' is overwritten with an error code (not GRN_SUCCESS).
 */
static grn_obj *
mecab_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *str;
  int nflags = 0;
  char *buf, *p;
  const char *s;
  grn_obj *table = args[0];
  grn_obj_flags table_flags;
  grn_encoding table_encoding;
  grn_mecab_tokenizer *tokenizer;
  unsigned int bufsize, len;
  if (!(str = grn_ctx_pop(ctx))) {
    ERR(GRN_INVALID_ARGUMENT, "missing argument");
    return NULL;
  }
  if (!sole_mecab) {
    CRITICAL_SECTION_ENTER(sole_mecab_lock);
    if (!sole_mecab) {
      sole_mecab = mecab_new2("-Owakati");
      if (!sole_mecab) {
        ERR(GRN_TOKENIZER_ERROR, "mecab_new2 failed on grn_mecab_init: %s",
            mecab_strerror(NULL));
      } else {
        sole_mecab_encoding = get_mecab_encoding(sole_mecab);
      }
    }
    CRITICAL_SECTION_LEAVE(sole_mecab_lock);
  }
  if (!sole_mecab) {
    return NULL;
  }
  grn_table_get_info(ctx, table, &table_flags, &table_encoding, NULL);
  if (table_encoding != sole_mecab_encoding) {
    ERR(GRN_TOKENIZER_ERROR,
        "MeCab dictionary charset (%s) does not match the context encoding: <%s>",
        grn_enctostr(sole_mecab_encoding), grn_enctostr(table_encoding));
    return NULL;
  }
  if (!(tokenizer = GRN_MALLOC(sizeof(grn_mecab_tokenizer)))) { return NULL; }
  tokenizer->mecab = sole_mecab;
  tokenizer->encoding = table_encoding;
  nflags |= (table_flags & GRN_OBJ_KEY_NORMALIZE);
  if (!(tokenizer->nstr = grn_str_open_(ctx,
                                        GRN_TEXT_VALUE(str), GRN_TEXT_LEN(str),
                                        nflags, tokenizer->encoding))) {
    GRN_FREE(tokenizer);
    ERR(GRN_TOKENIZER_ERROR, "grn_str_open failed at grn_token_open");
    return NULL;
  }

  len = tokenizer->nstr->norm_blen;
  tokenizer->have_tokenized_delimiter =
    grn_tokenizer_have_tokenized_delimiter(ctx,
                                           tokenizer->nstr->norm, len,
                                           tokenizer->encoding);
  if (tokenizer->have_tokenized_delimiter) {
    tokenizer->buf = NULL;
    tokenizer->next = tokenizer->nstr->norm;
    tokenizer->end = tokenizer->next + len;
  } else {
    CRITICAL_SECTION_ENTER(sole_mecab_lock);
    s = mecab_sparse_tostr2(tokenizer->mecab, tokenizer->nstr->norm, len);
    if (!s) {
      ERR(GRN_TOKENIZER_ERROR, "mecab_sparse_tostr failed len=%d err=%s",
          len, mecab_strerror(tokenizer->mecab));
    } else {
      bufsize = strlen(s) + 1;
      if (!(buf = GRN_MALLOC(bufsize))) {
        GRN_LOG(ctx, GRN_LOG_ALERT, "buffer allocation on mecab_init failed !");
      } else {
        memcpy(buf, s, bufsize);
      }
    }
    CRITICAL_SECTION_LEAVE(sole_mecab_lock);
    if (!s || !buf) {
      grn_str_close(ctx, tokenizer->nstr);
      GRN_FREE(tokenizer);
      return NULL;
    }
    /* A certain version of mecab returns trailing lf or spaces. */
    for (p = buf + bufsize - 2;
         buf <= p && isspace(*(unsigned char *)p);
         p--) { *p = '\0'; }
    tokenizer->buf = buf;
    tokenizer->next = buf;
    tokenizer->end = p + 1;
  }
  user_data->ptr = tokenizer;

  grn_tokenizer_token_init(ctx, &(tokenizer->token));

  return NULL;
}

/*
  This function returns tokens one by one.
 */
static grn_obj *
mecab_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  size_t cl;
  /* grn_obj *table = args[0]; */
  grn_mecab_tokenizer *tokenizer = user_data->ptr;
  char *p = tokenizer->next, *r;
  char *e = tokenizer->end;
  grn_encoding encoding = tokenizer->encoding;
  grn_tokenizer_status status;

  if (tokenizer->have_tokenized_delimiter) {
    for (r = p; r < e; r += cl) {
      cl = grn_charlen_(ctx, r, e, encoding);
      if (cl > 0) {
        if (grn_tokenizer_is_tokenized_delimiter(ctx, r, cl, encoding)) {
          tokenizer->next = r + cl;
          break;
        }
      } else {
        tokenizer->next = e;
        break;
      }
    }
  } else {
    for (r = p; r < e; r += cl) {
      if (!(cl = grn_charlen_(ctx, r, e, encoding))) {
        tokenizer->next = e;
        break;
      }
      if (grn_isspace(r, encoding)) {
        char *q = r;
        while ((cl = grn_isspace(q, encoding))) { q += cl; }
        tokenizer->next = q;
        break;
      }
    }
  }

  if (r == e) {
    status = GRN_TOKENIZER_LAST;
  } else {
    status = GRN_TOKENIZER_CONTINUE;
  }
  grn_tokenizer_token_push(ctx, &(tokenizer->token), p, r - p, status);
  return NULL;
}

/*
  This function finalizes a tokenization.
 */
static grn_obj *
mecab_fin(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_mecab_tokenizer *tokenizer = user_data->ptr;
  grn_tokenizer_token_fin(ctx, &(tokenizer->token));
  grn_str_close(ctx, tokenizer->nstr);
  if (tokenizer->buf) {
    GRN_FREE(tokenizer->buf);
  }
  GRN_FREE(tokenizer);
  return NULL;
}

static void
check_mecab_dictionary_encoding(grn_ctx *ctx)
{
#ifdef HAVE_MECAB_DICTIONARY_INFO_T
  mecab_t *mecab;

  mecab = mecab_new2("-Owakati");
  if (mecab) {
    grn_encoding encoding;
    int have_same_encoding_dictionary = 0;

    encoding = GRN_CTX_GET_ENCODING(ctx);
    have_same_encoding_dictionary = encoding == get_mecab_encoding(mecab);
    mecab_destroy(mecab);

    if (!have_same_encoding_dictionary) {
      ERR(GRN_TOKENIZER_ERROR,
          "MeCab has no dictionary that uses the context encoding: <%s>",
          grn_enctostr(encoding));
    }
  } else {
    ERR(GRN_TOKENIZER_ERROR,
        "mecab_new2 failed in check_mecab_dictionary_encoding: %s",
        mecab_strerror(NULL));
  }
#endif
}

/*
  This function initializes a plugin. This function fails if there is no
  dictionary that uses the context encoding of groonga.
 */
grn_rc
GRN_PLUGIN_INIT(grn_ctx *ctx)
{
  sole_mecab = NULL;
  CRITICAL_SECTION_INIT(sole_mecab_lock);

  check_mecab_dictionary_encoding(ctx);
  return ctx->rc;
}

/*
  This function registers a plugin to a database.
 */
grn_rc
GRN_PLUGIN_REGISTER(grn_ctx *ctx)
{
  grn_obj *obj;
  grn_expr_var vars[] = {
    {NULL, 0},
    {NULL, 0},
    {NULL, 0}
  };
  GRN_TEXT_INIT(&vars[0].value, 0);
  GRN_TEXT_INIT(&vars[1].value, 0);
  GRN_UINT32_INIT(&vars[2].value, 0);

  /*
    grn_proc_create() registers a plugin to a database which is associated
    with `ctx'. A returned object must not be finalized here.
   */
  obj = grn_proc_create(ctx, "TokenMecab", 10, GRN_PROC_TOKENIZER,
                        mecab_init, mecab_next, mecab_fin, 3, vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_MECAB) { return GRN_FILE_CORRUPT; }

  return GRN_SUCCESS;
}

/*
  This function finalizes a plugin.
 */
grn_rc
GRN_PLUGIN_FIN(grn_ctx *ctx)
{
  if (sole_mecab) {
    mecab_destroy(sole_mecab);
    sole_mecab = NULL;
  }
  CRITICAL_SECTION_FIN(sole_mecab_lock);

  return GRN_SUCCESS;
}
