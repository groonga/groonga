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

/* ngram */

inline static grn_token *
grn_ngram_init(grn_token *token)
{
  return token;
}

inline static grn_id
grn_ngram_next(grn_token *token)
{
  grn_id tid;
  grn_obj *table = token->table;
  grn_ctx *ctx = token->ctx;
  uint_least8_t *cp = NULL;
  int32_t len = 0, pos;
  const unsigned char *p, *q, *r;
  if (token->status == grn_token_done) { return GRN_ID_NIL; }
  token->force_prefix = 0;
  for (p = token->next, pos = token->pos + token->skip; *p; p = r, pos++) {
    if (token->nstr->ctypes) { cp = token->nstr->ctypes + pos; }
    if (token->uni_alpha && GRN_NSTR_CTYPE(*cp) == grn_str_alpha) {
      for (len = 1, r = p;;len++) {
        size_t cl;
        if (!(cl = grn_str_charlen(ctx, (char *)r, token->encoding))) { break; }
        r += cl;
        if (GRN_NSTR_ISBLANK(*cp)) { break; }
        if (GRN_NSTR_CTYPE(*++cp) != grn_str_alpha) { break; }
      }
      {
        size_t blen = r - p;
        if (!blen) {
          token->status = grn_token_done;
          return GRN_ID_NIL;
        }
        token->curr = p;
        token->curr_size = blen;
        tid = grn_table_lookup(ctx, table, p, blen, &token->flags);
        token->skip = len;
      }
    } else if (token->uni_digit && GRN_NSTR_CTYPE(*cp) == grn_str_digit) {
      for (len = 1, r = p;;len++) {
        size_t cl;
        if (!(cl = grn_str_charlen(ctx, (char *)r, token->encoding))) { break; }
        r += cl;
        if (GRN_NSTR_ISBLANK(*cp)) { break; }
        if (GRN_NSTR_CTYPE(*++cp) != grn_str_digit) { break; }
      }
      {
        size_t blen = r - p;
        if (!blen) {
          token->status = grn_token_done;
          return GRN_ID_NIL;
        }
        token->curr = p;
        token->curr_size = (uint32_t)blen;
        tid = grn_table_lookup(ctx, table, p, blen, &token->flags);
        token->skip = len;
      }
    } else if (token->uni_symbol && GRN_NSTR_CTYPE(*cp) == grn_str_symbol) {
      for (len = 1, r = p;;len++) {
        size_t cl;
        if (!(cl = grn_str_charlen(ctx, (char *)r, token->encoding))) { break; }
        r += cl;
        if (GRN_NSTR_ISBLANK(*cp)) { break; }
        if (GRN_NSTR_CTYPE(*++cp) != grn_str_symbol) { break; }
      }
      {
        size_t blen = r - p;
        if (!blen) {
          token->status = grn_token_done;
          return GRN_ID_NIL;
        }
        token->curr = p;
        token->curr_size = (uint32_t)blen;
        tid = grn_table_lookup(ctx, table, p, blen, &token->flags);
        token->skip = len;
      }
    } else {
      size_t cl;
#ifdef PRE_DEFINED_UNSPLIT_WORDS
      {
        const unsigned char *key = NULL;
        // todo : grn_pat_lcp_search
        if ((tid = grn_sym_common_prefix_search(sym, p))) {
          if (!(key = _grn_sym_key(sym, tid))) {
            token->status = grn_token_not_found;
            return GRN_ID_NIL;
          }
          len = grn_str_len(key, token->encoding, NULL);
        }
        r = p + grn_str_charlen(ctx, p, token->encoding);
        if (tid && (len > 1 || r == p)) {
          if (r != p && pos + len - 1 <= token->tail) { continue; }
          p += strlen(key);
          if (!*p && !(token->flags & GRN_TABLE_ADD)) { token->status = grn_token_done; }
        }
      }
#endif /* PRE_DEFINED_UNSPLIT_WORDS */
      if (!(cl = grn_str_charlen(ctx, (char *)p, token->encoding))) {
        token->status = grn_token_done;
        return GRN_ID_NIL;
      }
      r = p + cl;
      {
        int blankp = 0;
        for (len = 1, q = r; len < token->ngram_unit; len++) {
          if (cp) {
            if (GRN_NSTR_ISBLANK(*cp)) { blankp++; break; }
            cp++;
          }
          if (!(cl = grn_str_charlen(ctx, (char *)q, token->encoding)) ||
              (token->uni_alpha && GRN_NSTR_CTYPE(*cp) == grn_str_alpha) ||
              (token->uni_digit && GRN_NSTR_CTYPE(*cp) == grn_str_digit) ||
              (token->uni_symbol && GRN_NSTR_CTYPE(*cp) == grn_str_symbol)) {
            break;
          }
          q += cl;
        }
        if (blankp && !(token->flags & GRN_TABLE_ADD)) { continue; }
      }
      if ((!cl || !*q) && !(token->flags & GRN_TABLE_ADD)) { token->status = grn_token_done; }
      if (len < token->ngram_unit) { token->force_prefix = 1; }
      {
        size_t blen = q - p;
        if (!blen) {
          token->status = grn_token_done;
          return GRN_ID_NIL;
        }
        token->curr = p;
        token->curr_size = (uint32_t)blen;
        tid = grn_table_lookup(ctx, table, p, blen, &token->flags);
        token->skip = 1;
      }
    }
    token->pos = pos;
    token->len = len;
    token->tail = pos + len - 1;
    token->next = r;
    // printf("tid=%d pos=%d tail=%d (%s) %s\n", tid, token->pos, token->tail, _grn_sym_key(sym, tid), r);
    // printf("tid=%d pos=%d tail=%d (%s)\n", tid, token->pos, token->tail, _grn_sym_key(sym, tid));
    if (!tid) {
      token->status = grn_token_not_found;
    } else {
      if (!*r) { token->status = grn_token_done; }
    }
    return tid;
  }
  token->status = grn_token_done;
  return GRN_ID_NIL;
}

/* mecab */

#ifndef NO_MECAB

static mecab_t *sole_mecab;
static grn_mutex sole_mecab_lock;

static char *grn_token_default_mecab_argv[] = {"", "-Owakati"};

static int grn_token_mecab_argc = 2;
static char **grn_token_mecab_argv = grn_token_default_mecab_argv;

#define SOLE_MECAB_CONFIRM do {\
  if (!sole_mecab) {\
    MUTEX_LOCK(sole_mecab_lock);\
    if (!sole_mecab) { sole_mecab = mecab_new(grn_token_mecab_argc, grn_token_mecab_argv); }\
    MUTEX_UNLOCK(sole_mecab_lock);\
  }\
} while(0)

inline static grn_token *
grn_mecab_init(grn_token *token)
{
  grn_ctx *ctx = token->ctx;
  unsigned int bufsize, maxtrial = 10, len;
  grn_nstr *nstr = token->nstr;
  char *buf, *s, *p;
  char mecab_err[256];
  // grn_log("(%s)", str);
  SOLE_MECAB_CONFIRM;
  if (!sole_mecab) {
    GRN_LOG(grn_log_alert, "mecab_new failed on grn_mecab_init");
    return NULL;
  }
  token->mecab = sole_mecab;
  // if (!(token->mecab = mecab_new3())) {
  len = nstr->norm_blen;
  mecab_err[sizeof(mecab_err) - 1] = '\0';
  for (bufsize = len * 2 + 1; maxtrial; bufsize *= 2, maxtrial--) {
    if(!(buf = GRN_MALLOC(bufsize + 1))) {
      GRN_LOG(grn_log_alert, "buffer allocation on grn_mecab_init failed !");
      GRN_FREE(token);
      return NULL;
    }
    MUTEX_LOCK(sole_mecab_lock);
    s = mecab_sparse_tostr3(token->mecab, (char *)nstr->norm, len, buf, bufsize);
    if (!s) {
      strncpy(mecab_err, mecab_strerror(token->mecab), sizeof(mecab_err) - 1);
    }
    MUTEX_UNLOCK(sole_mecab_lock);
    if (s) { break; }
    GRN_FREE(buf);
    if (strstr(mecab_err, "output buffer overflow") == NULL) {
      break;
    }
  }
  if (!s) {
    GRN_LOG(grn_log_alert, "mecab_sparse_tostr failed len=%d bufsize=%d err=%s", len, bufsize, mecab_err);
    grn_token_close(token);
    return NULL;
  }
  // certain version of mecab returns trailing lf or spaces.
  for (p = buf + strlen(buf) - 1; buf <= p && (*p == '\n' || isspace(*(unsigned char *)p)); p--) { *p = '\0'; }
  //grn_log("sparsed='%s'", s);
  token->orig = (unsigned char *)nstr->norm;
  token->buf = (unsigned char *)buf;
  token->next = (unsigned char *)buf;
  token->force_prefix = 0;
  return token;
}

inline static grn_id
grn_mecab_next(grn_token *token)
{
  grn_id tid;
  grn_obj *table = token->table;
  grn_ctx *ctx = token->ctx;
  int32_t len, offset = token->offset + token->len;
  const unsigned char *p;
  if (token->status == grn_token_done) { return GRN_ID_NIL; }
  for (p = token->next, len = 0;;) {
    size_t cl;
    if (!(cl = grn_str_charlen(ctx, (char *)p, token->encoding)) ||
        grn_isspace((const char *)p, token->encoding)) {
      break;
    }
    p += cl;
    len++;
  }
  if (!len) {
    token->status = grn_token_done;
    return GRN_ID_NIL;
  }
  token->curr = token->next;
  token->curr_size = (uint32_t)(p - token->next);
  tid = grn_table_lookup(ctx, table, token->curr, token->curr_size, &token->flags);
  {
    int cl;
    while ((cl = grn_isspace((const char *)p, token->encoding))) { p += cl; }
    token->next = p;
    token->offset = offset;
    token->len = len;
  }
  if (tid == GRN_ID_NIL) {
    token->status = grn_token_not_found;
  } else {
    if (!*p) { token->status = grn_token_done; }
  }
  token->pos++;
  return tid;
}

grn_rc
grn_token_set_mecab_args(grn_ctx *ctx, int argc, char **argv)
{
  grn_token_mecab_argc = argc;
  grn_token_mecab_argv = argv;
  if (sole_mecab) {
    GRN_LOG(grn_log_alert, "mecab already initialized");
    return grn_invalid_argument;
  }
  SOLE_MECAB_CONFIRM;
  return grn_success;
}

#endif /* NO_MECAB */

/* delimited */

inline static grn_token *
grn_delimited_init(grn_token *token)
{
  int cl;
  const char *p = token->nstr->norm;
  const char *pe = token->nstr->norm + token->nstr->norm_blen;
  token->orig = (unsigned char *)p;
  while ((cl = grn_isspace((const char *)p, token->encoding))) {
    p += cl;
    if (pe <= p) {
      token->status = grn_token_done;
      break;
    }
  }
  token->next = (unsigned char *)p;
  return token;
}

inline static grn_id
grn_delimited_next(grn_token *token)
{
  grn_id tid;
  grn_obj *table = token->table;
  grn_ctx *ctx = token->ctx;
  int32_t len, offset = token->offset + token->len;
  const unsigned char *p;
  if (token->status == grn_token_done) { return GRN_ID_NIL; }
  for (p = token->next, len = 0;;) {
    size_t cl;
    if (!(cl = grn_str_charlen(ctx, (char *)p, token->encoding)) ||
        grn_isspace((const char *)p, token->encoding)) {
      break;
    }
    p += cl;
    len++;
  }
  if (!len) {
    token->status = grn_token_done;
    return GRN_ID_NIL;
  }
  token->curr = token->next;
  token->curr_size = (uint32_t)(p - token->next);
  tid = grn_table_lookup(ctx, table, token->curr, token->curr_size, &token->flags);
  {
    int cl;
    while ((cl = grn_isspace((const char *)p, token->encoding))) { p += cl; }
    token->next = p;
    token->offset = offset;
    token->len = len;
  }
  if (tid == GRN_ID_NIL) {
    token->status = grn_token_not_found;
  } else {
    if (!*p) { token->status = grn_token_done; }
  }
  token->pos++;
  return tid;
}

/* external */

grn_rc
grn_token_init(void)
{
#ifndef NO_MECAB
  // char *arg[] = {"", "-Owakati"};
  // return mecab_load_dictionary(2, arg) ? grn_success : grn_external_error;
  sole_mecab = NULL;
  MUTEX_INIT(sole_mecab_lock);
#endif /* NO_MECAB */
  return grn_success;
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
  return grn_success;
}

grn_token *
grn_token_open(grn_ctx *ctx, grn_obj *table, const char *str, size_t str_len,
               grn_search_flags flags)
{
  grn_nstr *nstr;
  int nflag, type;
  grn_token *token;
  grn_encoding encoding;
  grn_obj_flags table_flags;
  uint8_t ngram_unit;
  if (grn_table_get_info(ctx, table, &table_flags, &encoding, &ngram_unit)) { return NULL; }
  type = table_flags & GRN_OBJ_TOKEN_MASK;
  nflag = (type == GRN_OBJ_TOKEN_NGRAM ? GRN_STR_REMOVEBLANK|GRN_STR_WITH_CTYPES : 0);
  if (table_flags & GRN_OBJ_KEY_NORMALIZE) {
    if (!(nstr = grn_nstr_open(ctx, str, str_len, encoding, nflag))) {
      GRN_LOG(grn_log_alert, "grn_nstr_open failed at grn_token_open");
      return NULL;
    }
  } else {
    if (!(nstr = grn_fakenstr_open(ctx, str, str_len, encoding, nflag))) {
      GRN_LOG(grn_log_alert, "grn_fakenstr_open failed at grn_token_open");
      return NULL;
    }
  }
  if (!(token = GRN_MALLOC(sizeof(grn_token)))) { return NULL; }
  token->ctx = ctx;
  token->table = table;
  token->flags = flags;
  token->nstr = nstr;
  token->table_flags = table_flags;
  token->encoding = encoding;

#ifndef NO_MECAB
  token->mecab = NULL;
#endif /* NO_MECAB */
  token->buf = NULL;
  token->curr = NULL;
  token->curr_size = 0;
  token->pos = -1;
  token->skip = 1;
  token->tail = 0;
  token->status = grn_token_doing;

  token->orig = (unsigned char *)nstr->norm;
  token->orig_blen = nstr->norm_blen;
  token->next = (unsigned char *)nstr->norm;
  token->uni_alpha = (nstr->ctypes && !(table_flags & GRN_OBJ_KEY_SPLIT_ALPHA));
  token->uni_digit = (nstr->ctypes && !(table_flags & GRN_OBJ_KEY_SPLIT_DIGIT));
  token->uni_symbol = (nstr->ctypes && !(table_flags & GRN_OBJ_KEY_SPLIT_SYMBOL));
  token->force_prefix = 0;
  token->ngram_unit = ngram_unit;
  token->offset = 0;
  token->len = 0;

  switch (type) {
  case GRN_OBJ_TOKEN_MECAB :
#ifdef NO_MECAB
    GRN_FREE(token);
    return NULL;
#else /* NO_MECAB */
    return grn_mecab_init(token);
#endif /* NO_MECAB */
  case GRN_OBJ_TOKEN_NGRAM :
    return grn_ngram_init(token);
  case GRN_OBJ_TOKEN_DELIMITED :
    return grn_delimited_init(token);
  default :
    GRN_FREE(token);
    return NULL;
  }
}

grn_rc
grn_token_next(grn_token *token)
{
  /* if (!token) { return grn_invalid_argument; } */
  switch ((token->table_flags & GRN_OBJ_TOKEN_MASK)) {
  case GRN_OBJ_TOKEN_MECAB :
#ifdef NO_MECAB
    return grn_invalid_argument;
#else /* NO_MECAB */
    return grn_mecab_next(token);
#endif /* NO_MECAB */
  case GRN_OBJ_TOKEN_NGRAM :
    return grn_ngram_next(token);
  case GRN_OBJ_TOKEN_DELIMITED :
    return grn_delimited_next(token);
  default :
    return grn_invalid_argument;
  }
}

grn_rc
grn_token_close(grn_token *token)
{
  if (token) {
    grn_ctx *ctx = token->ctx;
    if (token->nstr) { grn_nstr_close(token->nstr); }
    // if (token->mecab) { mecab_destroy(token->mecab); }
    if (token->buf) { GRN_FREE(token->buf); }
    GRN_FREE(token);
    return grn_success;
  } else {
    return grn_invalid_argument;
  }
}

grn_rc
grn_token_validate(grn_ctx *ctx, grn_obj *table)
{
  grn_rc rc;
  grn_encoding encoding;
  grn_obj_flags table_flags;
  uint8_t ngram_unit;
  if ((rc = grn_table_get_info(ctx, table, &table_flags, &encoding, &ngram_unit))) {
    return rc;
  }
#ifndef NO_MECAB
#ifdef USE_MECAB_DICINFO
  if ((table_flags & GRN_OBJ_TOKEN_MASK) == GRN_OBJ_TOKEN_MECAB) {
    grn_encoding enc;
    const mecab_dictionary_info_t *di;

    SOLE_MECAB_CONFIRM;
    if (!sole_mecab) {
      GRN_LOG(grn_log_alert, "mecab_new failed on grn_token_validate");
      return grn_external_error;
    }
    di = mecab_dictionary_info(sole_mecab);
    if (!di || !di->charset) {
      GRN_LOG(grn_log_alert, "mecab_dictionary_info failed on grn_token_validate");
      return grn_external_error;
    }
    switch (di->charset[0]) {
      case 'u':
        enc = grn_enc_utf8;
        break;
      case 'e':
        enc = grn_enc_euc_jp;
        break;
      case 'c': /* cp932 */
      case 's':
        enc = grn_enc_sjis;
        break;
      default:
        GRN_LOG(grn_log_alert, "unknown encoding %s on grn_token_validate", di->charset);
        return grn_external_error;
    }
    if (enc != encoding) {
      GRN_LOG(grn_log_alert,
              "dictionary encoding %s is differ from table encoding %s",
              di->charset, grn_enctostr(encoding));
      return grn_abnormal_error;
    }
  }
#endif /* USE_MECAB_DICINFO */
#endif /* NO_MECAB */
  return grn_success;
}
