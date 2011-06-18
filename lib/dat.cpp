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
#include "dat.h"
#include "util.h"

namespace grn {
namespace dat {

class Trie {
 public:
  static Trie *create(const char *path) {
    return new Trie;
  }
  static Trie *open(const char *path) {
    return new Trie;
  }

  uint32_t num_keys() const {
    return 0;
  }

 private:
};

}  // namespace dat
}  // namespace grn

extern "C" {

static void
grn_dat_init(grn_dat *dat) {
  dat->handle = NULL;
}

static void
grn_dat_end(grn_dat *dat) {
  delete static_cast<grn::dat::Trie *>(dat->handle);
  dat->handle = NULL;
}

grn_dat *
grn_dat_create(grn_ctx *ctx, const char *path, unsigned int,
                        unsigned int, unsigned int) {
  grn_dat *dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
  if (dat == NULL) {
    return NULL;
  }
  grn_dat_init(dat);
  dat->handle = grn::dat::Trie::create(path);
  if (dat->handle == NULL) {
    GRN_FREE(dat);
    return NULL;
  }
  return dat;
}

grn_dat *
grn_dat_open(grn_ctx *ctx, const char *path) {
  grn_dat *dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
  if (dat == NULL) {
    return NULL;
  }
  grn_dat_init(dat);
  dat->handle = grn::dat::Trie::open(path);
  if (dat->handle == NULL) {
    GRN_FREE(dat);
    return NULL;
  }
  return dat;
}

grn_rc
grn_dat_close(grn_ctx *ctx, grn_dat *dat) {
  grn_dat_end(dat);
  GRN_FREE(dat);
  return GRN_SUCCESS;
}

grn_rc
grn_dat_remove(grn_ctx *ctx, const char *path) {
  return GRN_SUCCESS;
}

grn_id
grn_dat_get(grn_ctx *ctx, grn_dat *dat, const void *key,
                   unsigned int key_size, void **value) {
  return GRN_ID_NIL;
}

grn_id
grn_dat_add(grn_ctx *ctx, grn_dat *dat, const void *key,
                   unsigned int key_size, void **value, int *added) {
  return GRN_ID_NIL;
}

int
grn_dat_get_key(grn_ctx *ctx, grn_dat *dat, grn_id id,
                    void *keybuf, int bufsize) {
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
  if (dat == NULL) {
    return GRN_INVALID_ARGUMENT;
  }
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
