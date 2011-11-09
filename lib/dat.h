/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2011 Brazil

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
#ifndef GRN_DAT_H
#define GRN_DAT_H

#ifndef GROONGA_IN_H
#include "groonga_in.h"
#endif /* GROONGA_IN_H */

#include "db.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _grn_dat {
  grn_db_obj obj;
  grn_io *io;
  struct grn_dat_header *header;
  uint32_t file_id;
  grn_encoding encoding;
  void *trie;
  void *old_trie;
  grn_obj *tokenizer;
  grn_critical_section lock;
};

struct grn_dat_header {
  uint32_t flags;
  grn_encoding encoding;
  grn_id tokenizer;
  uint32_t file_id;
};

struct _grn_dat_cursor {
  grn_db_obj obj;
  grn_dat *dat;
  void *cursor;
  const void *key;
  grn_id curr_rec;
};

grn_id grn_dat_curr_id(grn_ctx *ctx, grn_dat *dat);

grn_rc grn_dat_truncate(grn_ctx *ctx, grn_dat *dat);

const char *_grn_dat_key(grn_ctx *ctx, grn_dat *dat, grn_id id, uint32_t *key_size);

#ifdef __cplusplus
}
#endif

#endif /* GRN_DAT_H */
