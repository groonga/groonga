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
#ifndef NO_MECAB
#include <mecab.h>
#endif /* NO_MECAB */

#ifndef GRN_TOKEN_H
#define GRN_TOKEN_H

#ifndef GROONGA_H
#include "groonga_in.h"
#endif /* GROONGA_H */

#ifndef GRN_CTX_H
#include "ctx.h"
#endif /* GRN_CTX_H */

#ifndef GRN_DB_H
#include "db.h"
#endif /* GRN_DB_H */

#ifndef GRN_STR_H
#include "str.h"
#endif /* GRN_STR_H */

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct {
  grn_obj *table;
  const unsigned char *orig;
  const unsigned char *curr;
  uint32_t orig_blen;
  uint32_t curr_size;
  int32_t pos;
  int32_t add;
  uint8_t status;
  uint8_t force_prefix;
  grn_obj_flags table_flags;
  grn_encoding encoding;
  grn_obj *tokenizer;
  grn_proc_ctx pctx;
} grn_token;

enum {
  grn_token_doing = 0,
  grn_token_done,
  grn_token_not_found
};

#define GRN_TOKEN_LAST      (0x01L<<0)
#define GRN_TOKEN_OVERLAP   (0x01L<<1)
#define GRN_TOKEN_UNMATURED (0x01L<<2)

extern grn_obj *grn_uvector_tokenizer;

grn_rc grn_token_init(void);
grn_rc grn_token_fin(void);

grn_token *grn_token_open(grn_ctx *ctx, grn_obj *table, const char *str,
                          size_t str_len, int add);

grn_id grn_token_next(grn_ctx *ctx, grn_token *ng);
grn_rc grn_token_close(grn_ctx *ctx, grn_token *ng);

grn_rc grn_db_init_builtin_tokenizers(grn_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif /* GRN_TOKEN_H */
