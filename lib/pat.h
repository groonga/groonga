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
#ifndef GRN_PAT_H
#define GRN_PAT_H

#ifndef GROONGA_H
#include "groonga_in.h"
#endif /* GROONGA_H */

#include "db.h"
#include "hash.h"

#ifdef	__cplusplus
extern "C" {
#endif

#define GRN_PAT_MAX_KEY_SIZE                    GRN_TABLE_MAX_KEY_SIZE

typedef struct _grn_pat_scan_hit grn_pat_scan_hit;
typedef struct _grn_pat_cursor grn_pat_cursor;

struct _grn_pat {
  grn_db_obj obj;
  grn_io *io;
  struct grn_pat_header *header;
  grn_encoding encoding;
  uint32_t key_size;
  uint32_t value_size;
  grn_obj *tokenizer;
};

#define GRN_PAT_NDELINFOS 0x100

typedef struct {
  grn_id d;
  grn_id ld;
  uint32_t stat;
  uint32_t shared;
} grn_pat_delinfo;

struct grn_pat_header {
  uint32_t flags;
  grn_encoding encoding;
  uint32_t key_size;
  uint32_t value_size;
  grn_id tokenizer;
  uint32_t nrecords;
  uint32_t curr_rec;
  int32_t curr_key;
  int32_t curr_del;
  int32_t curr_del2;
  int32_t curr_del3;
  uint32_t reserved[1006];
  grn_pat_delinfo delinfos[GRN_PAT_NDELINFOS];
  grn_id garbages[GRN_PAT_MAX_KEY_SIZE + 1];
};

struct _grn_pat_scan_hit {
  grn_id id;
  unsigned int offset;
  unsigned int length;
};

typedef struct _grn_pat grn_pat;

struct _grn_pat_cursor_entry {
  grn_id id;
  uint16_t check;
};

typedef struct _grn_pat_cursor_entry grn_pat_cursor_entry;

struct _grn_pat_cursor {
  grn_db_obj obj;
  grn_id curr_rec;
  grn_pat *pat;
  grn_ctx *ctx;
  unsigned int size;
  unsigned int sp;
  grn_id limit;
  grn_pat_cursor_entry *ss;
  uint8_t curr_key[GRN_TABLE_MAX_KEY_SIZE];
};

grn_pat *grn_pat_create(grn_ctx *ctx, const char *path, uint32_t key_size,
                        uint32_t value_size, uint32_t flags, grn_encoding encoding);

grn_pat *grn_pat_open(grn_ctx *ctx, const char *path);

grn_rc grn_pat_close(grn_ctx *ctx, grn_pat *pat);

grn_rc grn_pat_remove(grn_ctx *ctx, const char *path);

grn_id grn_pat_lookup(grn_ctx *ctx, grn_pat *pat, const void *key, int key_size,
                       void **value, grn_search_flags *flags);

grn_rc grn_pat_prefix_search(grn_ctx *ctx, grn_pat *pat,
                             const void *key, uint32_t key_size, grn_hash *h);
grn_rc grn_pat_suffix_search(grn_ctx *ctx, grn_pat *pat,
                             const void *key, uint32_t key_size, grn_hash *h);
grn_id grn_pat_lcp_search(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size);

uint32_t grn_pat_size(grn_ctx *ctx, grn_pat *pat);

int grn_pat_get_key(grn_ctx *ctx, grn_pat *pat, grn_id id, void *keybuf, int bufsize);
int grn_pat_get_key2(grn_ctx *ctx, grn_pat *pat, grn_id id, grn_obj *bulk);
int grn_pat_get_value(grn_ctx *ctx, grn_pat *pat, grn_id id, void *valuebuf);
grn_rc grn_pat_set_value(grn_ctx *ctx, grn_pat *pat, grn_id id,
                         void *value, int flags);

grn_rc grn_pat_delete_by_id(grn_ctx *ctx, grn_pat *pat, grn_id id,
                            grn_table_delete_optarg *optarg);
grn_rc grn_pat_delete(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size,
                      grn_table_delete_optarg *optarg);
int grn_pat_delete_with_sis(grn_ctx *ctx, grn_pat *pat, grn_id id,
                      grn_table_delete_optarg *optarg);

int grn_pat_scan(grn_ctx *ctx, grn_pat *pat, const char *str, unsigned int str_len,
                 grn_pat_scan_hit *sh, unsigned int sh_size, const char **rest);

grn_pat_cursor *grn_pat_cursor_open(grn_ctx *ctx, grn_pat *pat,
                                    const void *min, uint32_t min_size,
                                    const void *max, uint32_t max_size, int flags);
grn_id grn_pat_cursor_next(grn_ctx *ctx, grn_pat_cursor *c);
void grn_pat_cursor_close(grn_ctx *ctx, grn_pat_cursor *c);

int grn_pat_cursor_get_key(grn_ctx *ctx, grn_pat_cursor *c, void **key);
int grn_pat_cursor_get_value(grn_ctx *ctx, grn_pat_cursor *c, void **value);
int grn_pat_cursor_get_key_value(grn_ctx *ctx, grn_pat_cursor *c,
                                 void **key, uint32_t *key_size, void **value);
grn_rc grn_pat_cursor_set_value(grn_ctx *ctx, grn_pat_cursor *c,
                                void *value, int flags);
grn_rc grn_pat_cursor_delete(grn_ctx *ctx, grn_pat_cursor *c,
                             grn_table_delete_optarg *optarg);

grn_id grn_pat_at(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size,
                  void **value);
grn_id grn_pat_get(grn_ctx *ctx, grn_pat *pat, const void *key, uint32_t key_size,
                   void **value, grn_search_flags *flags);
grn_id grn_pat_curr_id(grn_ctx *ctx, grn_pat *pat);

#define GRN_PAT_EACH(pat,id,key,key_size,value,block) do {\
  grn_pat_cursor *_sc = grn_pat_cursor_open(ctx, pat, NULL, 0, NULL, 0, 0);\
  if (_sc) {\
    grn_id id;\
    while ((id = grn_pat_cursor_next(ctx, _sc))) {\
      grn_pat_cursor_get_key_value(ctx, _sc, (void **)(key),\
                                    (key_size), (void **)(value));\
      block\
    }\
    grn_pat_cursor_close(ctx, _sc);\
  }\
} while (0)

/* private */
const char *_grn_pat_key(grn_ctx *ctx, grn_pat *pat, grn_id id, uint32_t *key_size);
grn_id grn_pat_next(grn_ctx *ctx, grn_pat *pat, grn_id id);
const char *grn_pat_get_value_(grn_ctx *ctx, grn_pat *pat, grn_id id, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif /* GRN_PAT_H */
