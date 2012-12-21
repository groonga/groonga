/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2012 Brazil

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
#include <string.h>
#include <ctype.h>
#include "ctx_impl.h"
#include "token.h"
#include "pat.h"
#include "dat.h"
#include "hash.h"
#include "string_in.h"
#include <groonga/tokenizer.h>

grn_obj *grn_uvector_tokenizer = NULL;

typedef struct {
  byte *curr;
  byte *tail;
  uint32_t unit;
  grn_obj curr_;
  grn_obj stat_;
} grn_uvector_tokenizer_info;

static grn_obj *
uvector_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_obj *str;
  grn_uvector_tokenizer_info *token;
  if (!(str = grn_ctx_pop(ctx))) {
    ERR(GRN_INVALID_ARGUMENT, "missing argument");
    return NULL;
  }
  if (!(token = GRN_MALLOC(sizeof(grn_uvector_tokenizer_info)))) { return NULL; }
  user_data->ptr = token;
  token->curr = GRN_TEXT_VALUE(str);
  token->tail = token->curr + GRN_TEXT_LEN(str);
  token->unit = sizeof(grn_id);
  GRN_TEXT_INIT(&token->curr_, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_UINT32_INIT(&token->stat_, 0);
  return NULL;
}

static grn_obj *
uvector_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_uvector_tokenizer_info *token = user_data->ptr;
  byte *p = token->curr + token->unit;
  if (token->tail < p) {
    GRN_TEXT_SET_REF(&token->curr_, token->curr, 0);
    GRN_UINT32_SET(ctx, &token->stat_, GRN_TOKENIZER_TOKEN_LAST);
  } else {
    GRN_TEXT_SET_REF(&token->curr_, token->curr, token->unit);
    token->curr = p;
    GRN_UINT32_SET(ctx, &token->stat_,
                   token->tail == p ? GRN_TOKENIZER_TOKEN_LAST : 0);
  }
  grn_ctx_push(ctx, &token->curr_);
  grn_ctx_push(ctx, &token->stat_);
  return NULL;
}

static grn_obj *
uvector_fin(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  GRN_FREE(user_data->ptr);
  return NULL;
}

typedef struct {
  const uint8_t *delimiter;
  uint32_t delimiter_len;
  const unsigned char *next;
  const unsigned char *end;
  grn_tokenizer_token token;
  grn_tokenizer_query *query;
  grn_bool have_tokenized_delimiter;
} grn_delimited_tokenizer;

static grn_obj *
delimited_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data,
               const uint8_t *delimiter, uint32_t delimiter_len)
{
  grn_tokenizer_query *query;
  unsigned int normalize_flags = 0;
  const char *normalized;
  unsigned int normalized_length_in_bytes;
  grn_delimited_tokenizer *tokenizer;

  query = grn_tokenizer_query_open(ctx, nargs, args, normalize_flags);
  if (!query) {
    return NULL;
  }

  if (!(tokenizer = GRN_MALLOC(sizeof(grn_delimited_tokenizer)))) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[tokenizer][delimit] "
        "memory allocation to grn_delimited_tokenizer failed");
    grn_tokenizer_query_close(ctx, query);
    return NULL;
  }
  user_data->ptr = tokenizer;

  tokenizer->query = query;

  tokenizer->have_tokenized_delimiter =
    grn_tokenizer_have_tokenized_delimiter(ctx,
                                           tokenizer->query->ptr,
                                           tokenizer->query->length,
                                           tokenizer->query->encoding);
  tokenizer->delimiter = delimiter;
  tokenizer->delimiter_len = delimiter_len;
  grn_string_get_normalized(ctx, tokenizer->query->normalized_query,
                            &normalized, &normalized_length_in_bytes,
                            NULL);
  tokenizer->next = (const unsigned char *)normalized;
  tokenizer->end = tokenizer->next + normalized_length_in_bytes;

  grn_tokenizer_token_init(ctx, &(tokenizer->token));

  return NULL;
}

static grn_obj *
delimited_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_delimited_tokenizer *tokenizer = user_data->ptr;

  if (tokenizer->have_tokenized_delimiter) {
    unsigned int rest_length;
    rest_length = tokenizer->end - tokenizer->next;
    tokenizer->next =
      grn_tokenizer_tokenized_delimiter_next(ctx,
                                             &(tokenizer->token),
                                             tokenizer->next,
                                             rest_length,
                                             tokenizer->query->encoding);
  } else {
    size_t cl;
    const unsigned char *p = tokenizer->next, *r;
    const unsigned char *e = tokenizer->end;
    grn_tokenizer_status status;
    for (r = p; r < e; r += cl) {
      if (!(cl = grn_charlen_(ctx, (char *)r, (char *)e,
                              tokenizer->query->encoding))) {
        tokenizer->next = (unsigned char *)e;
        break;
      }
      if (r + tokenizer->delimiter_len <= e &&
          !memcmp(r, tokenizer->delimiter, tokenizer->delimiter_len)) {
        tokenizer->next = r + tokenizer->delimiter_len;
        break;
      }
    }
    if (r == e) {
      status = GRN_TOKENIZER_LAST;
    } else {
      status = GRN_TOKENIZER_CONTINUE;
    }
    grn_tokenizer_token_push(ctx, &(tokenizer->token), p, r - p, status);
  }

  return NULL;
}

static grn_obj *
delimited_fin(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_delimited_tokenizer *tokenizer = user_data->ptr;
  grn_tokenizer_query_close(ctx, tokenizer->query);
  grn_tokenizer_token_fin(ctx, &(tokenizer->token));
  GRN_FREE(tokenizer);
  return NULL;
}

static grn_obj *
delimit_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  static const uint8_t delimiter[1] = {' '};
  return delimited_init(ctx, nargs, args, user_data, delimiter, 1);
}

static grn_obj *
delimit_null_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  static const uint8_t delimiter[1] = {'\0'};
  return delimited_init(ctx, nargs, args, user_data, delimiter, 1);
}

/* ngram tokenizer */

typedef struct {
  grn_tokenizer_token token;
  grn_tokenizer_query *query;
  grn_obj *nstr;
  uint8_t uni_alpha;
  uint8_t uni_digit;
  uint8_t uni_symbol;
  uint8_t ngram_unit;
  uint8_t ignore_blank;
  uint8_t overlap;
  int32_t pos;
  uint32_t skip;
  grn_encoding encoding;
  const unsigned char *next;
  const unsigned char *end;
  const uint_least8_t *ctypes;
  int32_t len;
  uint32_t tail;
} grn_ngram_tokenizer;

static grn_obj *
ngram_init(grn_ctx *ctx, grn_obj *table, grn_user_data *user_data, uint8_t ngram_unit,
           uint8_t uni_alpha, uint8_t uni_digit, uint8_t uni_symbol, uint8_t ignore_blank)
{
  grn_obj *str;
  grn_obj *normalizer = NULL;
  int nflags =
    GRN_STRING_REMOVE_BLANK |
    GRN_STRING_WITH_TYPES |
    GRN_STRING_REMOVE_TOKENIZED_DELIMITER;
  const char *normalized;
  unsigned int normalized_length_in_bytes;
  grn_ngram_tokenizer *token;
  grn_obj_flags table_flags;
  if (!(str = grn_ctx_pop(ctx))) {
    ERR(GRN_INVALID_ARGUMENT, "missing argument");
    return NULL;
  }
  if (!(token = GRN_MALLOC(sizeof(grn_ngram_tokenizer)))) { return NULL; }
  user_data->ptr = token;
  token->uni_alpha = uni_alpha;
  token->uni_digit = uni_digit;
  token->uni_symbol = uni_symbol;
  token->ngram_unit = ngram_unit;
  token->ignore_blank = ignore_blank;
  token->overlap = 0;
  token->pos = 0;
  token->skip = 0;
  grn_table_get_info(ctx, table, &table_flags, &token->encoding, NULL,
                     &normalizer);
  if (!(token->nstr = grn_string_open_(ctx,
                                       GRN_TEXT_VALUE(str), GRN_TEXT_LEN(str),
                                       normalizer, nflags, token->encoding))) {
    GRN_FREE(token);
    ERR(GRN_TOKENIZER_ERROR, "grn_string_open failed at grn_token_open");
    return NULL;
  }
  grn_string_get_normalized(ctx, token->nstr,
                            &normalized, &normalized_length_in_bytes,
                            &(token->len));
  token->next = (const unsigned char *)normalized;
  token->end = token->next + normalized_length_in_bytes;
  token->ctypes = grn_string_get_types(ctx, token->nstr);
  grn_tokenizer_token_init(ctx, &(token->token));
  return NULL;
}

static grn_obj *
unigram_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 1, 1, 1, 1, 0); }

static grn_obj *
bigram_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 1, 1, 1, 0); }

static grn_obj *
trigram_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 3, 1, 1, 1, 0); }

static grn_obj *
bigrams_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 1, 1, 0, 0); }

static grn_obj *
bigramsa_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 0, 1, 0, 0); }

static grn_obj *
bigramsad_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 0, 0, 0, 0); }

static grn_obj *
bigrami_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 1, 1, 1, 1); }

static grn_obj *
bigramis_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 1, 1, 0, 1); }

static grn_obj *
bigramisa_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 0, 1, 0, 1); }

static grn_obj *
bigramisad_init(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{ return ngram_init(ctx, args[0], user_data, 2, 0, 0, 0, 1); }

static grn_obj *
ngram_next(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  size_t cl;
  grn_ngram_tokenizer *token = user_data->ptr;
  const unsigned char *p = token->next, *r = p, *e = token->end;
  int32_t len = 0, pos = token->pos + token->skip, status = 0;
  const uint_least8_t *cp = token->ctypes ? token->ctypes + pos : NULL;
  if (cp && token->uni_alpha && GRN_STR_CTYPE(*cp) == grn_str_alpha) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      if (/* !token->ignore_blank && */ GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != grn_str_alpha) { break; }
    }
    token->next = r;
    token->overlap = 0;
  } else if (cp && token->uni_digit && GRN_STR_CTYPE(*cp) == grn_str_digit) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      if (/* !token->ignore_blank && */ GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != grn_str_digit) { break; }
    }
    token->next = r;
    token->overlap = 0;
  } else if (cp && token->uni_symbol && GRN_STR_CTYPE(*cp) == grn_str_symbol) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      if (!token->ignore_blank && GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != grn_str_symbol) { break; }
    }
    token->next = r;
    token->overlap = 0;
  } else {
#ifdef PRE_DEFINED_UNSPLIT_WORDS
    const unsigned char *key = NULL;
    // todo : grn_pat_lcp_search
    if ((tid = grn_sym_common_prefix_search(sym, p))) {
      if (!(key = _grn_sym_key(sym, tid))) {
        token->status = GRN_TOKEN_NOT_FOUND;
        return NULL;
      }
      len = grn_str_len(key, token->encoding, NULL);
    }
    r = p + grn_charlen_(ctx, p, e, token->encoding);
    if (tid && (len > 1 || r == p)) {
      if (r != p && pos + len - 1 <= token->tail) { continue; }
      p += strlen(key);
      if (!*p && token->mode == GRN_TOKEN_GET) { token->status = GRN_TOKEN_DONE; }
    }
#endif /* PRE_DEFINED_UNSPLIT_WORDS */
    if ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      token->next = r;
      while (len < token->ngram_unit &&
             (cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
        if (cp) {
          if (!token->ignore_blank && GRN_STR_ISBLANK(*cp)) { break; }
          cp++;
          if ((token->uni_alpha && GRN_STR_CTYPE(*cp) == grn_str_alpha) ||
              (token->uni_digit && GRN_STR_CTYPE(*cp) == grn_str_digit) ||
              (token->uni_symbol && GRN_STR_CTYPE(*cp) == grn_str_symbol)) { break; }
        }
        len++;
        r += cl;
      }
      if (token->overlap) { status |= GRN_TOKENIZER_TOKEN_OVERLAP; }
      if (len < token->ngram_unit) { status |= GRN_TOKENIZER_TOKEN_UNMATURED; }
      token->overlap = (len > 1) ? 1 : 0;
    }
  }
  token->pos = pos;
  token->len = len;
  token->tail = pos + len - 1;
  if (p == r || token->next == e) {
    token->skip = 0;
    status |= GRN_TOKENIZER_TOKEN_LAST;
  } else {
    token->skip = token->overlap ? 1 : len;
  }
  if (r == e) { status |= GRN_TOKENIZER_TOKEN_REACH_END; }
  grn_tokenizer_token_push(ctx, &(token->token), p, r - p, status);
  return NULL;
}

static grn_obj *
ngram_fin(grn_ctx *ctx, int nargs, grn_obj **args, grn_user_data *user_data)
{
  grn_ngram_tokenizer *token = user_data->ptr;
  grn_tokenizer_token_fin(ctx, &(token->token));
  grn_obj_close(ctx, token->nstr);
  GRN_FREE(token);
  return NULL;
}

/* external */

grn_rc
grn_token_init(void)
{
  static grn_proc _grn_uvector_tokenizer;
  _grn_uvector_tokenizer.obj.db = NULL;
  _grn_uvector_tokenizer.obj.id = GRN_ID_NIL;
  _grn_uvector_tokenizer.obj.header.domain = GRN_ID_NIL;
  _grn_uvector_tokenizer.obj.range = GRN_ID_NIL;
  _grn_uvector_tokenizer.funcs[PROC_INIT] = uvector_init;
  _grn_uvector_tokenizer.funcs[PROC_NEXT] = uvector_next;
  _grn_uvector_tokenizer.funcs[PROC_FIN] = uvector_fin;
  grn_uvector_tokenizer = (grn_obj *)&_grn_uvector_tokenizer;
  return GRN_SUCCESS;
}

grn_rc
grn_token_fin(void)
{
  return GRN_SUCCESS;
}

grn_token *
grn_token_open(grn_ctx *ctx, grn_obj *table, const char *str, size_t str_len,
               grn_token_mode mode)
{
  grn_token *token;
  grn_encoding encoding;
  grn_obj *tokenizer;
  grn_obj *normalizer;
  grn_obj_flags table_flags;
  if (grn_table_get_info(ctx, table, &table_flags, &encoding, &tokenizer,
                         &normalizer)) {
    return NULL;
  }
  if (!(token = GRN_MALLOC(sizeof(grn_token)))) { return NULL; }
  token->table = table;
  token->mode = mode;
  token->encoding = encoding;
  token->tokenizer = tokenizer;
  token->orig = str;
  token->orig_blen = str_len;
  token->curr = NULL;
  token->nstr = NULL;
  token->curr_size = 0;
  token->pos = -1;
  token->status = GRN_TOKEN_DOING;
  token->force_prefix = 0;
  if (tokenizer) {
    grn_obj str_;
    GRN_TEXT_INIT(&str_, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET_REF(&str_, str, str_len);
    token->pctx.caller = NULL;
    token->pctx.user_data.ptr = NULL;
    token->pctx.proc = (grn_proc *)tokenizer;
    token->pctx.hooks = NULL;
    token->pctx.currh = NULL;
    token->pctx.phase = PROC_INIT;
    grn_ctx_push(ctx, &str_);
    ((grn_proc *)tokenizer)->funcs[PROC_INIT](ctx, 1, &table, &token->pctx.user_data);
    grn_obj_close(ctx, &str_);
  } else {
    int nflags = 0;
    token->nstr = grn_string_open_(ctx, str, str_len,
                                   normalizer, nflags, token->encoding);
    if (token->nstr) {
      const char *normalized;
      grn_string_get_normalized(ctx, token->nstr,
                                &normalized, &(token->curr_size), NULL);
      token->curr = (const unsigned char *)normalized;
    } else {
      ERR(GRN_TOKENIZER_ERROR, "grn_string_open failed at grn_token_open");
    }
  }
  if (ctx->rc) {
    grn_token_close(ctx, token);
    token = NULL;
  }
  return token;
}

grn_id
grn_token_next(grn_ctx *ctx, grn_token *token)
{
  int status;
  grn_id tid = GRN_ID_NIL;
  grn_obj *table = token->table;
  grn_obj *tokenizer = token->tokenizer;
  while (token->status != GRN_TOKEN_DONE) {
    if (tokenizer) {
      grn_obj *curr_, *stat_;
      ((grn_proc *)tokenizer)->funcs[PROC_NEXT](ctx, 1, &table, &token->pctx.user_data);
      stat_ = grn_ctx_pop(ctx);
      curr_ = grn_ctx_pop(ctx);
      token->curr = GRN_TEXT_VALUE(curr_);
      token->curr_size = GRN_TEXT_LEN(curr_);
      status = GRN_UINT32_VALUE(stat_);
      token->status = ((status & GRN_TOKENIZER_TOKEN_LAST) ||
                       (token->mode == GRN_TOKEN_GET &&
                        (status & GRN_TOKENIZER_TOKEN_REACH_END)))
        ? GRN_TOKEN_DONE : GRN_TOKEN_DOING;
      token->force_prefix = 0;
      if (status & GRN_TOKENIZER_TOKEN_UNMATURED) {
        if (status & GRN_TOKENIZER_TOKEN_OVERLAP) {
          if (token->mode == GRN_TOKEN_GET) { token->pos++; continue; }
        } else {
          if (status & GRN_TOKENIZER_TOKEN_LAST) { token->force_prefix = 1; }
        }
      }
    } else {
      token->status = GRN_TOKEN_DONE;
    }
    if (token->mode == GRN_TOKEN_ADD) {
      switch (table->header.type) {
      case GRN_TABLE_PAT_KEY :
        if (grn_io_lock(ctx, ((grn_pat *)table)->io, 10000000)) {
          tid = GRN_ID_NIL;
        } else {
          tid = grn_pat_add(ctx, (grn_pat *)table, token->curr, token->curr_size,
                            NULL, NULL);
          grn_io_unlock(((grn_pat *)table)->io);
        }
        break;
      case GRN_TABLE_DAT_KEY :
        if (grn_io_lock(ctx, ((grn_dat *)table)->io, 10000000)) {
          tid = GRN_ID_NIL;
        } else {
          tid = grn_dat_add(ctx, (grn_dat *)table, token->curr, token->curr_size,
                            NULL, NULL);
          grn_io_unlock(((grn_dat *)table)->io);
        }
        break;
      case GRN_TABLE_HASH_KEY :
        if (grn_io_lock(ctx, ((grn_hash *)table)->io, 10000000)) {
          tid = GRN_ID_NIL;
        } else {
          tid = grn_hash_add(ctx, (grn_hash *)table, token->curr, token->curr_size,
                             NULL, NULL);
          grn_io_unlock(((grn_hash *)table)->io);
        }
        break;
      case GRN_TABLE_NO_KEY :
        if (token->curr_size == sizeof(grn_id)) {
          tid = *((grn_id *)token->curr);
        } else {
          tid = GRN_ID_NIL;
        }
        break;
      }
    } else {
      switch (table->header.type) {
      case GRN_TABLE_PAT_KEY :
        tid = grn_pat_get(ctx, (grn_pat *)table, token->curr, token->curr_size, NULL);
        break;
      case GRN_TABLE_DAT_KEY :
        tid = grn_dat_get(ctx, (grn_dat *)table, token->curr, token->curr_size, NULL);
        break;
      case GRN_TABLE_HASH_KEY :
        tid = grn_hash_get(ctx, (grn_hash *)table, token->curr, token->curr_size, NULL);
        break;
      case GRN_TABLE_NO_KEY :
        if (token->curr_size == sizeof(grn_id)) {
          tid = *((grn_id *)token->curr);
        } else {
          tid = GRN_ID_NIL;
        }
        break;
      }
    }
    if (tid == GRN_ID_NIL && token->status != GRN_TOKEN_DONE) {
      token->status = GRN_TOKEN_NOT_FOUND;
    }
    token->pos++;
    break;
  }
  return tid;
}

grn_rc
grn_token_close(grn_ctx *ctx, grn_token *token)
{
  if (token) {
    if (token->tokenizer) {
      ((grn_proc *)token->tokenizer)->funcs[PROC_FIN](ctx, 1, &token->table,
                                                      &token->pctx.user_data);
    }
    if (token->nstr) {
      grn_obj_close(ctx, token->nstr);
    }
    GRN_FREE(token);
    return GRN_SUCCESS;
  } else {
    return GRN_INVALID_ARGUMENT;
  }
}

grn_rc
grn_db_init_mecab_tokenizer(grn_ctx *ctx)
{
  switch (GRN_CTX_GET_ENCODING(ctx)) {
  case GRN_ENC_EUC_JP :
  case GRN_ENC_UTF8 :
  case GRN_ENC_SJIS :
    return grn_plugin_register(ctx, "tokenizers/mecab");
  default :
    return GRN_OPERATION_NOT_SUPPORTED;
  }
}

#define DEF_TOKENIZER(name, init, next, fin, vars)\
  (grn_proc_create(ctx, (name), (sizeof(name) - 1),\
                   GRN_PROC_TOKENIZER, (init), (next), (fin), 3, (vars)))

grn_rc
grn_db_init_builtin_tokenizers(grn_ctx *ctx)
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

  obj = DEF_TOKENIZER("TokenDelimit",
                      delimit_init, delimited_next, delimited_fin, vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_DELIMIT) { return GRN_FILE_CORRUPT; }
  obj = DEF_TOKENIZER("TokenUnigram",
                      unigram_init, ngram_next, ngram_fin, vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_UNIGRAM) { return GRN_FILE_CORRUPT; }
  obj = DEF_TOKENIZER("TokenBigram",
                      bigram_init, ngram_next, ngram_fin, vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_BIGRAM) { return GRN_FILE_CORRUPT; }
  obj = DEF_TOKENIZER("TokenTrigram",
                      trigram_init, ngram_next, ngram_fin, vars);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_TRIGRAM) { return GRN_FILE_CORRUPT; }

  DEF_TOKENIZER("TokenBigramSplitSymbol",
                bigrams_init, ngram_next, ngram_fin, vars);
  DEF_TOKENIZER("TokenBigramSplitSymbolAlpha",
                bigramsa_init, ngram_next, ngram_fin, vars);
  DEF_TOKENIZER("TokenBigramSplitSymbolAlphaDigit",
                bigramsad_init, ngram_next, ngram_fin, vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlank",
                bigrami_init, ngram_next, ngram_fin, vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlankSplitSymbol",
                bigramis_init, ngram_next, ngram_fin, vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlankSplitSymbolAlpha",
                bigramisa_init, ngram_next, ngram_fin, vars);
  DEF_TOKENIZER("TokenBigramIgnoreBlankSplitSymbolAlphaDigit",
                bigramisad_init, ngram_next, ngram_fin, vars);
  DEF_TOKENIZER("TokenDelimitNull",
                delimit_null_init, delimited_next, delimited_fin, vars);
  return GRN_SUCCESS;
}
