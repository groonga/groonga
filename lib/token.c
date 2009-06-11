/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2009 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "groonga_in.h"
#include <string.h>
#include <ctype.h>
#include "token.h"
#include "pat.h"
#include "hash.h"

grn_obj *grn_uvector_tokenizer = NULL;

typedef struct {
  byte *curr;
  byte *tail;
  uint32_t unit;
  grn_obj curr_;
  grn_obj stat_;
} grn_uvector_tokenizer_info;

static grn_rc
uvector_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
            int argc, grn_proc_data *argv)
{
  grn_obj *str;
  grn_uvector_tokenizer_info *token;
  if (!(str = grn_ctx_pop(ctx))) { return GRN_INVALID_ARGUMENT; }
  if (!(token = GRN_MALLOC(sizeof(grn_uvector_tokenizer_info)))) { return ctx->rc; }
  user_data->ptr = token;
  token->curr = GRN_TEXT_VALUE(str);
  token->tail = token->curr + GRN_TEXT_LEN(str);
  token->unit = sizeof(grn_id);
  GRN_TEXT_INIT(&token->curr_, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_UINT32_INIT(&token->stat_, 0);
  return GRN_SUCCESS;
}

static grn_rc
uvector_next(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
             int argc, grn_proc_data *argv)
{
  grn_uvector_tokenizer_info *token = user_data->ptr;
  byte *p = token->curr + token->unit;
  argv[0].ptr = (void *)token->curr;
  if (token->tail < p) {
    GRN_TEXT_SET_REF(&token->curr_, token->curr, 0);
    GRN_UINT32_SET(ctx, &token->stat_, GRN_TOKEN_LAST);
  } else {
    GRN_TEXT_SET_REF(&token->curr_, token->curr, token->unit);
    token->curr = p;
    GRN_UINT32_SET(ctx, &token->stat_, token->tail == p ? GRN_TOKEN_LAST : 0);
  }
  grn_ctx_push(ctx, &token->curr_);
  grn_ctx_push(ctx, &token->stat_);
  return GRN_SUCCESS;
}

static grn_rc
uvector_fin(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
          int argc, grn_proc_data *argv)
{
  GRN_FREE(user_data->ptr);
  return GRN_SUCCESS;
}

typedef struct {
  grn_str *nstr;
  uint8_t *delimiter;
  uint32_t delimiter_len;
  int32_t pos;
  grn_encoding encoding;
  const unsigned char *next;
  const unsigned char *end;
  int32_t len;
  uint32_t tail;
  grn_obj curr_;
  grn_obj stat_;
} grn_delimited_tokenizer;

static grn_rc
delimited_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
               int argc, grn_proc_data *argv, uint8_t *delimiter, uint32_t delimiter_len)
{
  grn_obj *str;
  int nflags = 0;
  grn_delimited_tokenizer *token;
  grn_obj_flags table_flags;
  if (!(str = grn_ctx_pop(ctx))) { return GRN_INVALID_ARGUMENT; }
  if (!(token = GRN_MALLOC(sizeof(grn_delimited_tokenizer)))) { return ctx->rc; }
  user_data->ptr = token;
  token->delimiter = delimiter;
  token->delimiter_len = delimiter_len;
  token->pos = 0;
  grn_table_get_info(ctx, table, &table_flags, &token->encoding, NULL);
  nflags |= (table_flags & GRN_OBJ_KEY_NORMALIZE);
  if (!(token->nstr = grn_str_open_(ctx, GRN_TEXT_VALUE(str), GRN_TEXT_LEN(str),
                                    nflags, token->encoding))) {
    ERR(GRN_TOKENIZER_ERROR, "grn_str_open failed at grn_token_open");
    return ctx->rc;
  }
  token->next = (unsigned char *)token->nstr->norm;
  token->end = token->next + token->nstr->norm_blen;
  token->len = token->nstr->length;
  GRN_TEXT_INIT(&token->curr_, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_UINT32_INIT(&token->stat_, 0);
  return GRN_SUCCESS;
}

static grn_rc
delimited_next(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
               int argc, grn_proc_data *argv)
{
  size_t cl;
  grn_delimited_tokenizer *token = user_data->ptr;
  const unsigned char *p = token->next, *r;
  const unsigned char *e = token->end;
  for (r = p; r < e; r += cl) {
    if (!(cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      token->next = (unsigned char *)e;
      break;
    }
    if (r + token->delimiter_len <= e &&
        !memcmp(r, token->delimiter, token->delimiter_len)) {
      token->next = r + token->delimiter_len;
      break;
    }
  }
  GRN_TEXT_SET_REF(&token->curr_, p, r - p);
  GRN_UINT32_SET(ctx, &token->stat_, r == e ? GRN_TOKEN_LAST : 0);
  grn_ctx_push(ctx, &token->curr_);
  grn_ctx_push(ctx, &token->stat_);
  return GRN_SUCCESS;
}

static grn_rc
delimited_fin(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
          int argc, grn_proc_data *argv)
{
  grn_delimited_tokenizer *token = user_data->ptr;
  grn_str_close(ctx, token->nstr);
  GRN_FREE(token);
  return GRN_SUCCESS;
}

static grn_rc
delimit_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
            int argc, grn_proc_data *argv)
{ return delimited_init(ctx, table, user_data, argc, argv, " ", 1); }

/* mecab tokenizer */

#ifndef NO_MECAB

static mecab_t *sole_mecab;
static grn_mutex sole_mecab_lock;

#define SOLE_MECAB_CONFIRM do {\
  if (!sole_mecab) {\
    static char *argv[] = {"", "-Owakati"};\
    MUTEX_LOCK(sole_mecab_lock);\
    if (!sole_mecab) { sole_mecab = mecab_new(2, argv); }\
    MUTEX_UNLOCK(sole_mecab_lock);\
  }\
} while(0)

typedef struct {
  grn_str *nstr;
  mecab_t *mecab;
  unsigned char *buf;
  unsigned char *next;
  unsigned char *end;
  grn_encoding encoding;
  grn_obj curr_;
  grn_obj stat_;
} grn_mecab_tokenizer;

static grn_rc
mecab_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
           int argc, grn_proc_data *argv)
{
  grn_obj *str;
  int nflags = 0;
  char *buf, *s, *p;
  char mecab_err[256];
  grn_obj_flags table_flags;
  grn_mecab_tokenizer *token;
  unsigned int bufsize, maxtrial = 10, len;
  if (!(str = grn_ctx_pop(ctx))) { return GRN_INVALID_ARGUMENT; }
  SOLE_MECAB_CONFIRM;
  if (!sole_mecab) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "mecab_new failed on grn_mecab_init");
    return GRN_TOKENIZER_ERROR;
  }
  if (!(token = GRN_MALLOC(sizeof(grn_mecab_tokenizer)))) { return ctx->rc; }
  user_data->ptr = token;
  token->mecab = sole_mecab;
  // if (!(token->mecab = mecab_new3())) {
  grn_table_get_info(ctx, table, &table_flags, &token->encoding, NULL);
  nflags |= (table_flags & GRN_OBJ_KEY_NORMALIZE);
  if (!(token->nstr = grn_str_open_(ctx, GRN_TEXT_VALUE(str), GRN_TEXT_LEN(str),
                                    nflags, token->encoding))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_str_open failed at grn_token_open");
    return GRN_TOKENIZER_ERROR;
  }
  len = token->nstr->norm_blen;
  mecab_err[sizeof(mecab_err) - 1] = '\0';
  for (bufsize = len * 2 + 1; maxtrial; bufsize *= 2, maxtrial--) {
    if(!(buf = GRN_MALLOC(bufsize + 1))) {
      GRN_LOG(ctx, GRN_LOG_ALERT, "buffer allocation on mecab_init failed !");
      GRN_FREE(token);
      return ctx->rc;
    }
    MUTEX_LOCK(sole_mecab_lock);
    s = mecab_sparse_tostr3(token->mecab, token->nstr->norm, len, buf, bufsize);
    if (!s) {
      strncpy(mecab_err, mecab_strerror(token->mecab), sizeof(mecab_err) - 1);
    }
    MUTEX_UNLOCK(sole_mecab_lock);
    if (s) { break; }
    GRN_FREE(buf);
    if (strstr(mecab_err, "output buffer overflow") == NULL) { break; }
  }
  if (!s) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "mecab_sparse_tostr failed len=%d bufsize=%d err=%s",
            len, bufsize, mecab_err);
    GRN_FREE(token);
    return GRN_TOKENIZER_ERROR;
  }
  // certain version of mecab returns trailing lf or spaces.
  for (p = buf + strlen(buf) - 1;
       buf <= p && (*p == '\n' || isspace(*(unsigned char *)p));
       p--) { *p = '\0'; }
  //grn_log("sparsed='%s'", s);
  token->buf = (unsigned char *)buf;
  token->next = (unsigned char *)buf;
  token->end = (unsigned char *)buf + strlen(buf);
  GRN_TEXT_INIT(&token->curr_, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_UINT32_INIT(&token->stat_, 0);
  return GRN_SUCCESS;
}

static grn_rc
mecab_next(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
           int argc, grn_proc_data *argv)
{
  size_t cl;
  grn_mecab_tokenizer *token = user_data->ptr;
  const unsigned char *p = token->next, *r;
  const unsigned char *e = token->end;
  for (r = p; r < e; r += cl) {
    if (!(cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      token->next = (unsigned char *)e;
      break;
    }
    if (grn_isspace((const char *)r, token->encoding)) {
      const unsigned char *q = r;
      while ((cl = grn_isspace((const char *)q, token->encoding))) { q += cl; }
      token->next = (unsigned char *)q;
      break;
    }
  }
  GRN_TEXT_SET_REF(&token->curr_, p, r - p);
  GRN_UINT32_SET(ctx, &token->stat_, r == e ? GRN_TOKEN_LAST : 0);
  grn_ctx_push(ctx, &token->curr_);
  grn_ctx_push(ctx, &token->stat_);
  return GRN_SUCCESS;
}

static grn_rc
mecab_fin(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
          int argc, grn_proc_data *argv)
{
  grn_mecab_tokenizer *token = user_data->ptr;
  // if (token->mecab) { mecab_destroy(token->mecab); }
  grn_str_close(ctx, token->nstr);
  GRN_FREE(token->buf);
  GRN_FREE(token);
  return GRN_SUCCESS;
}

#endif /* NO_MECAB */

/* ngram tokenizer */

typedef struct {
  grn_str *nstr;
  uint8_t uni_alpha;
  uint8_t uni_digit;
  uint8_t uni_symbol;
  uint8_t ngram_unit;
  uint8_t overlap;
  int32_t pos;
  uint32_t skip;
  grn_encoding encoding;
  const unsigned char *next;
  const unsigned char *end;
  uint_least8_t *ctypes;
  int32_t len;
  uint32_t tail;
  grn_obj curr_;
  grn_obj stat_;
} grn_ngram_tokenizer;

static grn_rc
ngram_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
           int argc, grn_proc_data *argv, uint8_t ngram_unit)
{
  grn_obj *str;
  int nflags = GRN_STR_REMOVEBLANK|GRN_STR_WITH_CTYPES;
  grn_ngram_tokenizer *token;
  grn_obj_flags table_flags;
  if (!(str = grn_ctx_pop(ctx))) { return GRN_INVALID_ARGUMENT; }
  if (!(token = GRN_MALLOC(sizeof(grn_ngram_tokenizer)))) { return ctx->rc; }
  user_data->ptr = token;
  token->uni_alpha = 1;
  token->uni_digit = 1;
  token->uni_symbol = 1;
  token->ngram_unit = ngram_unit;
  token->overlap = 0;
  token->pos = 0;
  token->skip = 0;
  grn_table_get_info(ctx, table, &table_flags, &token->encoding, NULL);
  nflags |= (table_flags & GRN_OBJ_KEY_NORMALIZE);
  if (!(token->nstr = grn_str_open_(ctx, GRN_TEXT_VALUE(str), GRN_TEXT_LEN(str),
                                    nflags, token->encoding))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_str_open failed at grn_token_open");
    return GRN_TOKENIZER_ERROR;
  }
  token->next = (unsigned char *)token->nstr->norm;
  token->end = token->next + token->nstr->norm_blen;
  token->ctypes = token->nstr->ctypes;
  token->len = token->nstr->length;
  GRN_TEXT_INIT(&token->curr_, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_UINT32_INIT(&token->stat_, 0);
  return GRN_SUCCESS;
}

static grn_rc
unigram_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
             int argc, grn_proc_data *argv)
{ return ngram_init(ctx, table, user_data, argc, argv, 1); }

static grn_rc
bigram_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
            int argc, grn_proc_data *argv)
{ return ngram_init(ctx, table, user_data, argc, argv, 2); }

static grn_rc
trigram_init(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
             int argc, grn_proc_data *argv)
{ return ngram_init(ctx, table, user_data, argc, argv, 3); }

static grn_rc
ngram_next(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
            int argc, grn_proc_data *argv)
{
  size_t cl;
  grn_ngram_tokenizer *token = user_data->ptr;
  const unsigned char *p = token->next, *r = p, *e = token->end;
  int32_t len = 0, pos = token->pos + token->skip, status = 0;
  uint_least8_t *cp = token->ctypes ? token->ctypes + pos : NULL;
  if (cp && token->uni_alpha && GRN_STR_CTYPE(*cp) == grn_str_alpha) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      if (GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != grn_str_alpha) { break; }
    }
    token->next = r;
    token->overlap = 0;
  } else if (cp && token->uni_digit && GRN_STR_CTYPE(*cp) == grn_str_digit) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      if (GRN_STR_ISBLANK(*cp)) { break; }
      if (GRN_STR_CTYPE(*++cp) != grn_str_digit) { break; }
    }
    token->next = r;
    token->overlap = 0;
  } else if (cp && token->uni_symbol && GRN_STR_CTYPE(*cp) == grn_str_symbol) {
    while ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      if (GRN_STR_ISBLANK(*cp)) { break; }
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
        token->status = grn_token_not_found;
        return GRN_ID_NIL;
      }
      len = grn_str_len(key, token->encoding, NULL);
    }
    r = p + grn_charlen_(ctx, p, e, token->encoding);
    if (tid && (len > 1 || r == p)) {
      if (r != p && pos + len - 1 <= token->tail) { continue; }
      p += strlen(key);
      if (!*p && !token->add) { token->status = grn_token_done; }
    }
#endif /* PRE_DEFINED_UNSPLIT_WORDS */
    if ((cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
      len++;
      r += cl;
      token->next = r;
      while (len < token->ngram_unit &&
             (cl = grn_charlen_(ctx, (char *)r, (char *)e, token->encoding))) {
        if (cp) {
          if (GRN_STR_ISBLANK(*cp)) { break; }
          cp++;
          if ((token->uni_alpha && GRN_STR_CTYPE(*cp) == grn_str_alpha) ||
              (token->uni_digit && GRN_STR_CTYPE(*cp) == grn_str_digit) ||
              (token->uni_symbol && GRN_STR_CTYPE(*cp) == grn_str_symbol)) { break; }
        }
        len++;
        r += cl;
      }
      if (token->overlap) { status |= GRN_TOKEN_OVERLAP; }
      if (len < token->ngram_unit) { status |= GRN_TOKEN_UNMATURED; }
      token->overlap = 1;
    }
  }
  token->pos = pos;
  token->len = len;
  token->tail = pos + len - 1;
  if (p == r || r == e) {
    token->skip = 0;
    status |= GRN_TOKEN_LAST;
  } else {
    token->skip = token->overlap ? 1 : len;
  }
  GRN_TEXT_SET_REF(&token->curr_, p, r - p);
  GRN_UINT32_SET(ctx, &token->stat_, status);
  grn_ctx_push(ctx, &token->curr_);
  grn_ctx_push(ctx, &token->stat_);
  return GRN_SUCCESS;
}

static grn_rc
ngram_fin(grn_ctx *ctx, grn_obj *table, grn_proc_data *user_data,
           int argc, grn_proc_data *argv)
{
  grn_ngram_tokenizer *token = user_data->ptr;
  grn_str_close(ctx, token->nstr);
  GRN_FREE(token);
  return GRN_SUCCESS;
}

/* external */

grn_rc
grn_token_init(void)
{
  static grn_proc _grn_uvector_tokenizer;
#ifndef NO_MECAB
  // char *arg[] = {"", "-Owakati"};
  // return mecab_load_dictionary(2, arg) ? GRN_SUCCESS : GRN_TOKENIZER_ERROR;
  sole_mecab = NULL;
  MUTEX_INIT(sole_mecab_lock);
#endif /* NO_MECAB */
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
#ifndef NO_MECAB
  if (sole_mecab) {
    mecab_destroy(sole_mecab);
    sole_mecab = NULL;
  }
  MUTEX_DESTROY(sole_mecab_lock);
#endif /* NO_MECAB */
  return GRN_SUCCESS;
}

grn_token *
grn_token_open(grn_ctx *ctx, grn_obj *table, const char *str, size_t str_len, int add)
{
  grn_token *token;
  grn_encoding encoding;
  grn_obj *tokenizer;
  if (grn_table_get_info(ctx, table, NULL, &encoding, &tokenizer)) { return NULL; }
  if (!(token = GRN_MALLOC(sizeof(grn_token)))) { return NULL; }
  token->table = table;
  token->add = add;
  token->encoding = encoding;
  token->tokenizer = tokenizer;
  token->orig = str;
  token->orig_blen = str_len;
  token->curr = NULL;
  token->curr_size = 0;
  token->pos = -1;
  token->status = grn_token_doing;
  token->force_prefix = 0;
  if (tokenizer) {
    grn_obj str_;
    GRN_TEXT_INIT(&str_, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET_REF(&str_, str, str_len);
    token->pctx.user_data.ptr = NULL;
    token->pctx.obj = table;
    token->pctx.hooks = NULL;
    token->pctx.currh = NULL;
    token->pctx.phase = PROC_INIT;
    grn_ctx_push(ctx, &str_);
    ((grn_proc *)tokenizer)->funcs[PROC_INIT](ctx, table, &token->pctx.user_data,
                                              3, token->pctx.data);
    grn_obj_close(ctx, &str_);
  }
  if (ctx->rc) {
    GRN_FREE(token);
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
  while (token->status != grn_token_done) {
    if (tokenizer) {
      grn_obj *curr_, *stat_;
      ((grn_proc *)tokenizer)->funcs[PROC_NEXT](ctx, table, &token->pctx.user_data,
                                                4, token->pctx.data);
      stat_ = grn_ctx_pop(ctx);
      curr_ = grn_ctx_pop(ctx);
      token->curr = GRN_TEXT_VALUE(curr_);
      token->curr_size = GRN_TEXT_LEN(curr_);
      status = GRN_UINT32_VALUE(stat_);
      token->status = (status & GRN_TOKEN_LAST) ? grn_token_done : grn_token_doing;
      token->force_prefix = 0;
      if (status & GRN_TOKEN_UNMATURED) {
        if (status & GRN_TOKEN_OVERLAP) {
          if (!token->add) { continue; }
        } else {
          if (status & GRN_TOKEN_LAST) { token->force_prefix = 1; }
        }
      }
    } else {
      token->curr = token->orig;
      token->curr_size = token->orig_blen;
      token->status = grn_token_done;
    }
    if (token->add) {
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
    if (tid == GRN_ID_NIL) { token->status = grn_token_not_found; }
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
      ((grn_proc *)token->tokenizer)->funcs[PROC_FIN](ctx, token->table,
                                                      &token->pctx.user_data,
                                                      0, token->pctx.data);
    }
    GRN_FREE(token);
    return GRN_SUCCESS;
  } else {
    return GRN_INVALID_ARGUMENT;
  }
}

grn_rc
grn_db_init_builtin_tokenizers(grn_ctx *ctx)
{
  grn_obj *obj, results[2];
  GRN_UINT32_INIT(&results[0], 0);
  GRN_TEXT_INIT(&results[1], 0);

  obj = grn_proc_create(ctx, "<token:delimit>", 15, NULL,
                        delimit_init, delimited_next, delimited_fin, 1, 2, results);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_DELIMIT) { return GRN_FILE_CORRUPT; }
  obj = grn_proc_create(ctx, "<token:unigram>", 15, NULL,
                        unigram_init, ngram_next, ngram_fin, 1, 2, results);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_UNIGRAM) { return GRN_FILE_CORRUPT; }
  obj = grn_proc_create(ctx, "<token:bigram>", 14, NULL,
                        bigram_init, ngram_next, ngram_fin, 1, 2, results);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_BIGRAM) { return GRN_FILE_CORRUPT; }
  obj = grn_proc_create(ctx, "<token:trigram>", 15, NULL,
                        trigram_init, ngram_next, ngram_fin, 1, 2, results);
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_TRIGRAM) { return GRN_FILE_CORRUPT; }
#ifndef NO_MECAB
  obj = grn_proc_create(ctx, "<token:mecab>", 13, NULL,
                        mecab_init, mecab_next, mecab_fin, 1, 2, results);
#endif /* NO_MECAB */
  if (!obj || ((grn_db_obj *)obj)->id != GRN_DB_MECAB) { return GRN_FILE_CORRUPT; }
  return GRN_SUCCESS;
}
