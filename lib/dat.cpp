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
#include "groonga_in.h"
#include <string.h>
#include "str.h"
#include "io.h"
#include "dat.h"
#include "util.h"
#include "dat/trie.hpp"

extern "C" {

static void
grn_dat_init(grn_dat *dat) {
  dat->io = NULL;
  dat->header = NULL;
  dat->file_id = 0;
  dat->encoding = GRN_ENC_DEFAULT;
  dat->handle = NULL;
  dat->tokenizer = NULL;
}

static void
grn_dat_end(grn_dat *dat) {
  if (dat->handle) {
    delete static_cast<grn::dat::Trie *>(dat->handle);
    dat->handle = NULL;
  }
}

inline static void
gen_pathname(const char *path, char *buffer, int fno)
{
  size_t len = strlen(path);
  memcpy(buffer, path, len);
  if (fno) {
    buffer[len] = '.';
    grn_itoh(fno, buffer + len + 1, 3);
  } else {
    buffer[len] = '\0';
  }
}

grn_dat *
grn_dat_create(grn_ctx *ctx, const char *path, uint32_t key_size,
               uint32_t value_size, uint32_t flags)
{
  char path2[PATH_MAX];
  grn_dat *dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
  if (dat) {
    grn_dat_init(dat);
    if ((dat->io = grn_io_create(ctx, path, sizeof(struct grn_dat_header),
                                 4096, 0, grn_io_auto, GRN_IO_EXPIRE_SEGMENT))) {
      if ((dat->header = (struct grn_dat_header *)grn_io_header(dat->io))) {
        return dat;
      }
      grn_io_close(ctx, dat->io);
      grn_io_remove(ctx, path);
    }
    GRN_FREE(dat);
    dat = NULL;
  }
  return dat;
}

grn_dat *
grn_dat_open(grn_ctx *ctx, const char *path)
{
  grn_dat *dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
  if (dat) {
    grn_dat_init(dat);
    if ((dat->io = grn_io_open(ctx, path, grn_io_auto))) {
      if ((dat->header = (struct grn_dat_header *)grn_io_header(dat->io))) {
        dat->file_id = dat->header->file_id;
        dat->encoding = dat->header->encoding;
        dat->obj.header.flags = dat->header->flags;
        dat->tokenizer = grn_ctx_at(ctx, dat->header->tokenizer);
        return dat;
      }
      grn_io_close(ctx, dat->io);
    }
    GRN_FREE(dat);
    dat = NULL;
  }
  return dat;
}

grn_rc
grn_dat_close(grn_ctx *ctx, grn_dat *dat)
{
  grn_dat_end(dat);
  grn_io_close(ctx, dat->io);
  GRN_FREE(dat);
  return GRN_SUCCESS;
}

grn_rc
grn_dat_remove(grn_ctx *ctx, const char *path)
{
  /* FIXME: trie file must be removed */
  grn_io_remove(ctx, path);
  return ctx->rc;
}

static void
grn_dat_confirm_handle(grn_ctx *ctx, grn_dat *dat)
{
  int file_id = dat->header->file_id;
  if (!dat->handle || (dat->file_id != file_id)) {
    char buffer[PATH_MAX];
    gen_pathname(grn_io_path(dat->io), buffer, file_id);
    /* LOCK */
    grn_dat_handle handle = dat->handle;
    dat->handle = grn::dat::Trie::open(buffer);
    dat->file_id = file_id;
    /* UNLOCK */
    /* should be deleted after enough interval */
    if (handle) {
      delete static_cast<grn::dat::Trie *>(handle);
    }
  }
}

grn_id
grn_dat_get(grn_ctx *ctx, grn_dat *dat, const void *key,
            unsigned int key_size, void **value)
{
  grn_id id = GRN_ID_NIL;
  if (dat->header->file_id) {
    grn_dat_confirm_handle(ctx, dat);
    grn::dat::Key k;
    if (static_cast<grn::dat::Trie *>(dat->handle)->search(key, key_size, &k)) {
      id = k.id();
    }
  }
  return id;
}

grn_id
grn_dat_add(grn_ctx *ctx, grn_dat *dat, const void *key,
            unsigned int key_size, void **value, int *added)
{
  grn_id id = GRN_ID_NIL;
  int file_id = dat->header->file_id;
  if (!dat->header->file_id) {
    const char *path = grn_io_path(dat->io);
    file_id++;
    if (path && *path) {
      char buffer[PATH_MAX];
      gen_pathname(path, buffer, file_id);
      dat->handle = grn::dat::Trie::open(buffer);
    } else {
      dat->handle = grn::dat::Trie::open(NULL);
    }
    dat->file_id = dat->header->file_id = file_id;
  }
  grn_dat_confirm_handle(ctx, dat);
  grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
  if (trie == NULL) { goto exit; }
  try {
    grn::dat::Key k;
    bool res = trie->insert(key, key_size, &k);
    id = k.id();
    if (added) { *added = (int)res; }
  } catch (const grn::dat::Exception &ex) {
    char buffer[PATH_MAX];
    gen_pathname(grn_io_path(dat->io), buffer, ++file_id);
    /* LOCK */
    grn::dat::Trie *new_trie = grn::dat::Trie::create(*trie, buffer, trie->file_size() * 2);
    dat->handle = new_trie;
    dat->file_id = file_id;
    /* UNLOCK */
    /* should be deleted after enough interval */
    if (new_trie == NULL) { goto exit; }
    delete trie;
    grn::dat::Key k;
    bool res = new_trie->insert(key, key_size, &k);
    id = k.id();
  }
exit :
  return id;
}

int
grn_dat_get_key(grn_ctx *ctx, grn_dat *dat, grn_id id, void *keybuf, int bufsize)
{
  //  ith_key(UInt32 key_id, Key *key)
  return 0;
}

int
grn_dat_get_key2(grn_ctx *ctx, grn_dat *dat, grn_id id, grn_obj *bulk)
{
  return 0;
}

unsigned int
grn_dat_size(grn_ctx *ctx, grn_dat *dat)
{
  if (!dat || !dat->handle) { return 0; }
  return static_cast<grn::dat::Trie *>(dat->handle)->num_keys();
}

grn_dat_cursor *
grn_dat_cursor_open(grn_ctx *ctx, grn_dat *dat,
                    const void *min, unsigned int min_size,
                    const void *max, unsigned int max_size,
                    int offset, int limit, int flags)
{
  return NULL;
}

grn_id
grn_dat_cursor_next(grn_ctx *ctx, grn_dat_cursor *c)
{
  return GRN_ID_NIL;
}

void
grn_dat_cursor_close(grn_ctx *ctx, grn_dat_cursor *c)
{
}

int
grn_dat_cursor_get_key(grn_ctx *ctx, grn_dat_cursor *c, void **key)
{
  return 0;
}

grn_id
grn_dat_curr_id(grn_ctx *ctx, grn_dat *dat)
{
  return 0;
}

const char *
_grn_dat_key(grn_ctx *ctx, grn_dat *dat, grn_id id, uint32_t *key_size)
{
  return NULL;
}

}  // extern "C"
