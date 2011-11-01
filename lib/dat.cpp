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
#include "dat/cursor-factory.hpp"

namespace {

void
grn_dat_init(grn_dat *dat) {
  GRN_DB_OBJ_SET_TYPE(dat, GRN_TABLE_DAT_KEY);
  dat->io = NULL;
  dat->header = NULL;
  dat->file_id = 0;
  dat->encoding = GRN_ENC_DEFAULT;
  dat->handle = NULL;
  dat->tokenizer = NULL;
}

void
grn_dat_end(grn_dat *dat) {
  if (dat->handle) {
#ifndef WIN32
    delete static_cast<grn::dat::Trie *>(dat->handle);
#endif
    dat->handle = NULL;
  }
}

void
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

void
grn_dat_confirm_handle(grn_ctx *ctx, grn_dat *dat)
{
#ifndef WIN32
  int file_id = dat->header->file_id;
  if (!dat->handle || (dat->file_id != file_id)) {
    char buffer[PATH_MAX];
    gen_pathname(grn_io_path(dat->io), buffer, file_id);
    /* LOCK */
    grn_dat_handle handle = dat->handle;
    grn::dat::Trie *new_trie = new grn::dat::Trie;
    new_trie->open(buffer);
    dat->handle = new_trie;
    dat->file_id = file_id;
    /* UNLOCK */
    /* should be deleted after enough interval */
    if (handle) {
      delete static_cast<grn::dat::Trie *>(handle);
    }
  }
#endif
}

}  // namespace

extern "C" {

grn_dat *
grn_dat_create(grn_ctx *ctx, const char *path, uint32_t key_size,
               uint32_t value_size, uint32_t flags)
{
  char path2[PATH_MAX];
  grn_dat *dat = NULL;
#ifndef WIN32
  dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
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
#endif
  return dat;
}

grn_dat *
grn_dat_open(grn_ctx *ctx, const char *path)
{
  grn_dat *dat = NULL;
#ifndef WIN32
  dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
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
#endif
  return dat;
}

grn_rc
grn_dat_close(grn_ctx *ctx, grn_dat *dat)
{
  if (dat) {
    grn_dat_end(dat);
    grn_io_close(ctx, dat->io);
    GRN_FREE(dat);
  }
  return GRN_SUCCESS;
}

grn_rc
grn_dat_remove(grn_ctx *ctx, const char *path)
{
  /* FIXME: trie file must be removed */
  grn_io_remove(ctx, path);
  return ctx->rc;
}

grn_id
grn_dat_get(grn_ctx *ctx, grn_dat *dat, const void *key,
            unsigned int key_size, void **value)
{
  grn_id id = GRN_ID_NIL;
#ifndef WIN32
  if (dat && dat->header->file_id) {
    const grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
    grn_dat_confirm_handle(ctx, dat);
    grn::dat::UInt32 key_pos;
    if (trie->search(key, key_size, &key_pos)) {
      id = trie->get_key(key_pos).id();
    }
  }
#endif
  return id;
}

grn_id
grn_dat_add(grn_ctx *ctx, grn_dat *dat, const void *key,
            unsigned int key_size, void **value, int *added)
{
  grn_id id = GRN_ID_NIL;
#ifndef WIN32
  if (dat) {
    int file_id = dat->header->file_id;
    if (!dat->header->file_id) {
      const char *path = grn_io_path(dat->io);
      file_id++;
      if (path && *path) {
        char buffer[PATH_MAX];
        gen_pathname(path, buffer, file_id);
        grn::dat::Trie *new_trie = new grn::dat::Trie;
        new_trie->create(buffer);
        dat->handle = new_trie;
      } else {
        grn::dat::Trie *new_trie = new grn::dat::Trie;
        new_trie->create(NULL);
        dat->handle = new_trie;
      }
      dat->file_id = dat->header->file_id = file_id;
    }
    grn_dat_confirm_handle(ctx, dat);
    grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
    if (trie == NULL) { goto exit; }
    try {
      grn::dat::UInt32 key_pos;
      bool res = trie->insert(key, key_size, &key_pos);
      id = trie->get_key(key_pos).id();
      if (added) { *added = (int)res; }
    } catch (const grn::dat::Exception &ex) {
      char buffer[PATH_MAX];
      gen_pathname(grn_io_path(dat->io), buffer, ++file_id);
      /* LOCK */
      grn::dat::Trie *new_trie = new grn::dat::Trie;
      new_trie->create(*trie, buffer, trie->file_size() * 2);
      dat->handle = new_trie;
      dat->header->file_id = dat->file_id = file_id;
      /* UNLOCK */
      /* should be deleted after enough interval */
      if (new_trie == NULL) { goto exit; }
      delete trie;
      grn::dat::UInt32 key_pos;
      bool res = new_trie->insert(key, key_size, &key_pos);
      id = new_trie->get_key(key_pos).id();
    }
  }
#endif
exit :
  return id;
}

int
grn_dat_get_key(grn_ctx *ctx, grn_dat *dat, grn_id id, void *keybuf, int bufsize)
{
  int len = 0;
#ifndef WIN32
  if (dat && dat->header->file_id) {
    try {
      grn_dat_confirm_handle(ctx, dat);
      grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
      const grn::dat::Key &k = trie->ith_key(id);
      len = k.length();
      if (keybuf && bufsize >= len) {
        memcpy(keybuf, k.ptr(), len);
      }
    } catch (const grn::dat::Exception &ex) {
      len = 0;
    }
  }
#endif
  return len;
}

int
grn_dat_get_key2(grn_ctx *ctx, grn_dat *dat, grn_id id, grn_obj *bulk)
{
  int len = 0;
#ifndef WIN32
  if (dat && dat->header->file_id) {
    try {
      grn_dat_confirm_handle(ctx, dat);
      grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
      const grn::dat::Key &k = trie->ith_key(id);
      len = k.length();
      const char *key = static_cast<const char *>(k.ptr());
      if (bulk->header.impl_flags & GRN_OBJ_REFER) {
        bulk->u.b.head = (char *)key;
        bulk->u.b.curr = (char *)key + len;
      } else {
        grn_bulk_write(ctx, bulk, key, len);
      }
    } catch (const grn::dat::Exception &ex) {
      len = 0;
    }
  }
#endif
  return len;
}

unsigned int
grn_dat_size(grn_ctx *ctx, grn_dat *dat)
{
#ifndef WIN32
  if (dat && dat->handle) {
    return static_cast<grn::dat::Trie *>(dat->handle)->num_keys();
  }
#endif
  return 0;
}

grn_dat_cursor *
grn_dat_cursor_open(grn_ctx *ctx, grn_dat *dat,
                    const void *min, unsigned int min_size,
                    const void *max, unsigned int max_size,
                    int offset, int limit, int flags)
{
  grn_dat_cursor *dc = NULL;
#ifndef WIN32
  dc = static_cast<grn_dat_cursor *>(GRN_MALLOC(sizeof(grn_dat_cursor)));
  if (dc) {
    dc->cursor = NULL;
    GRN_DB_OBJ_SET_TYPE(dc, GRN_CURSOR_TABLE_DAT_KEY);
    if ((flags & GRN_CURSOR_BY_ID)) {
      grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
      grn::dat::Cursor *cursor = grn::dat::CursorFactory::open(*trie,
          min, min_size, max, max_size, offset, limit,
          grn::dat::ID_RANGE_CURSOR |
          ((flags & GRN_CURSOR_DESCENDING) ? grn::dat::DESCENDING_CURSOR : 0) |
          ((flags & GRN_CURSOR_GT) ? grn::dat::EXCEPT_LOWER_BOUND : 0) |
          ((flags & GRN_CURSOR_LT) ? grn::dat::EXCEPT_UPPER_BOUND : 0));
      dc->cursor = cursor;
    } else {
      if ((flags & GRN_CURSOR_PREFIX)) {
        if (max && max_size) {
//          if ((dat->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE)) {
            grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
            grn::dat::Cursor *cursor = grn::dat::CursorFactory::open(*trie,
                NULL, min_size, max, max_size, offset, limit,
                grn::dat::PREFIX_CURSOR | grn::dat::DESCENDING_CURSOR);
            dc->cursor = cursor;
//          } else {
//            /* todo: near */
//          }
        } else {
          if (min && min_size) {
            if (flags & GRN_CURSOR_RK) {
              /* todo: rk search */
            } else {
              grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
              grn::dat::Cursor *cursor = grn::dat::CursorFactory::open(*trie,
                  min, min_size, NULL, 0, offset, limit,
                  grn::dat::PREDICTIVE_CURSOR |
                  ((flags & GRN_CURSOR_DESCENDING) ? grn::dat::DESCENDING_CURSOR : 0) |
                  ((flags & GRN_CURSOR_GT) ? grn::dat::EXCEPT_EXACT_MATCH : 0));
              dc->cursor = cursor;
            }
          }
        }
      } else {
        grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
        grn::dat::Cursor *cursor = grn::dat::CursorFactory::open(*trie,
            min, min_size, max, max_size, offset, limit,
            grn::dat::KEY_RANGE_CURSOR |
            ((flags & GRN_CURSOR_DESCENDING) ? grn::dat::DESCENDING_CURSOR : 0) |
            ((flags & GRN_CURSOR_GT) ? grn::dat::EXCEPT_LOWER_BOUND : 0) |
            ((flags & GRN_CURSOR_LT) ? grn::dat::EXCEPT_UPPER_BOUND : 0));
        dc->cursor = cursor;
      }
    }
//    if (flags & GRN_CURSOR_DESCENDING) {
//      if (min && min_size) {
//        /* todo */
//      }
//      if (max && max_size) {
//        /* todo */
//      } else {
//        /* todo */
//      }
//    } else {
//      if (max && max_size) {
//        /* todo */
//      }
//      if (min && min_size) {
//        /* todo */
//      } else {
//        /* todo */
//      }
//    }
    if (dc->cursor) {
      dc->dat = dat;
      /* open stuff */
    } else {
      GRN_FREE(dc);
      dc = NULL;
    }
  }
#endif
  return dc;
}

grn_id
grn_dat_cursor_next(grn_ctx *ctx, grn_dat_cursor *c)
{
  grn_id id = GRN_ID_NIL;
#ifndef WIN32
  if (c && c->cursor) {
    grn::dat::Cursor *cursor = static_cast<grn::dat::Cursor *>(c->cursor);
    const grn::dat::Key &k = cursor->next();
    if (k.is_valid()) {
      id = c->curr_rec = k.id();
    } else {
      c->curr_rec = GRN_ID_NIL;
    }
  }
#endif
  return id;
}

void
grn_dat_cursor_close(grn_ctx *ctx, grn_dat_cursor *c)
{
#ifndef WIN32
  if (c && c->cursor) {
    grn::dat::Cursor *cursor = static_cast<grn::dat::Cursor *>(c->cursor);
    delete cursor;
    GRN_FREE(c);
  }
#endif
}

int
grn_dat_cursor_get_key(grn_ctx *ctx, grn_dat_cursor *c, void **key)
{
  return 0;
}

grn_id
grn_dat_curr_id(grn_ctx *ctx, grn_dat *dat)
{
  grn_id id = GRN_ID_NIL;
#ifndef WIN32
  if (dat && dat->header->file_id) {
    grn_dat_confirm_handle(ctx, dat);
    grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
    if (trie) {
      id = trie->num_keys();
    }
  }
#endif
  return id;
}

const char *
_grn_dat_key(grn_ctx *ctx, grn_dat *dat, grn_id id, uint32_t *key_size)
{
  const char *key = NULL;
#ifndef WIN32
  if (dat && dat->header->file_id) {
    try {
      grn_dat_confirm_handle(ctx, dat);
      grn::dat::Trie *trie = static_cast<grn::dat::Trie *>(dat->handle);
      const grn::dat::Key &k = trie->ith_key(id);
      *key_size = k.length();
      key = static_cast<const char *>(k.ptr());
    } catch (const grn::dat::Exception &ex) {
      key = NULL;
    }
  }
#endif
  return key;
}

}  // extern "C"
