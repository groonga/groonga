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

typedef struct _grn_dat grn_dat;
typedef struct _grn_dat_cursor grn_dat_cursor;

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
  grn_id curr_rec;
};

GRN_API grn_dat *grn_dat_create(grn_ctx *ctx, const char *path, unsigned int key_size,
                                unsigned int value_size, unsigned int flags);

GRN_API grn_dat *grn_dat_open(grn_ctx *ctx, const char *path);

GRN_API grn_rc grn_dat_close(grn_ctx *ctx, grn_dat *dat);

GRN_API grn_rc grn_dat_remove(grn_ctx *ctx, const char *path);

GRN_API grn_id grn_dat_get(grn_ctx *ctx, grn_dat *dat, const void *key,
                           unsigned int key_size, void **value);
GRN_API grn_id grn_dat_add(grn_ctx *ctx, grn_dat *dat, const void *key,
                           unsigned int key_size, void **value, int *added);

GRN_API int grn_dat_get_key(grn_ctx *ctx, grn_dat *dat, grn_id id, void *keybuf, int bufsize);
GRN_API int grn_dat_get_key2(grn_ctx *ctx, grn_dat *dat, grn_id id, grn_obj *bulk);

GRN_API grn_rc grn_dat_delete_by_id(grn_ctx *ctx, grn_dat *dat, grn_id id,
                                    grn_table_delete_optarg *optarg);
GRN_API grn_rc grn_dat_delete(grn_ctx *ctx, grn_dat *dat, const void *key, unsigned int key_size,
                              grn_table_delete_optarg *optarg);

//GRN_API grn_rc grn_dat_update_by_id(grn_ctx *ctx, grn_dat *dat, grn_id id,
//                                    const void *key, unsigned int key_size);
//GRN_API grn_rc grn_dat_update(grn_ctx *ctx, grn_dat *dat,
//                              const void *src_key, unsigned int src_key_size,
//                              const void *dest_key, unsigned int dest_key_size);

GRN_API unsigned int grn_dat_size(grn_ctx *ctx, grn_dat *dat);

GRN_API grn_dat_cursor *grn_dat_cursor_open(grn_ctx *ctx, grn_dat *dat,
                                            const void *min, unsigned int min_size,
                                            const void *max, unsigned int max_size,
                                            int offset, int limit, int flags);
GRN_API grn_id grn_dat_cursor_next(grn_ctx *ctx, grn_dat_cursor *c);
GRN_API void grn_dat_cursor_close(grn_ctx *ctx, grn_dat_cursor *c);

GRN_API int grn_dat_cursor_get_key(grn_ctx *ctx, grn_dat_cursor *c, void **key);
//GRN_API grn_rc grn_dat_cursor_delete(grn_ctx *ctx, grn_dat_cursor *c,
//                                     grn_table_delete_optarg *optarg);

grn_id grn_dat_curr_id(grn_ctx *ctx, grn_dat *dat);

const char *_grn_dat_key(grn_ctx *ctx, grn_dat *dat, grn_id id, uint32_t *key_size);

#ifdef __cplusplus
}
#endif

#endif /* GRN_DAT_H */
