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
#include <cstring>
#include <new>
#include "str.h"
#include "io.h"
#include "dat.h"
#include "util.h"

/*
  When this code is compiled with MinGW, a macro "open" is defined to replace
  "open()" by "_open()". This causes a critical problem because grn::dat::Trie
  and grn::dat::CursorFactory have member functions named "open()". So, the
  macro must be undefined before the following #includes.
 */
#ifdef open
# undef open
#endif

#include "dat/trie.hpp"
#include "dat/cursor-factory.hpp"

namespace {

const uint32_t FILE_ID_LENGTH = 3;

class CriticalSection {
 public:
  CriticalSection() : lock_(NULL) {}
  explicit CriticalSection(grn_critical_section *lock) : lock_(lock) {
    CRITICAL_SECTION_ENTER(*lock_);
  }
  ~CriticalSection() {
    leave();
  }

  void enter(grn_critical_section *lock) {
    leave();
    lock_ = lock;
  }
  void leave() {
    if (lock_ != NULL) {
      CRITICAL_SECTION_LEAVE(*lock_);
      lock_ = NULL;
    }
  }

 private:
  grn_critical_section *lock_;

  // Disallows copy and assignment.
  CriticalSection(const CriticalSection &);
  CriticalSection &operator=(const CriticalSection &);
};

grn_rc grn_dat_translate_error_code(grn::dat::ErrorCode error_code) {
  switch (error_code) {
    case grn::dat::PARAM_ERROR: {
      return GRN_INVALID_ARGUMENT;
    }
    case grn::dat::IO_ERROR: {
      return GRN_INPUT_OUTPUT_ERROR;
    }
    case grn::dat::FORMAT_ERROR: {
      return GRN_INVALID_FORMAT;
    }
    case grn::dat::MEMORY_ERROR: {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    case grn::dat::SIZE_ERROR:
    case grn::dat::UNEXPECTED_ERROR:
    default: {
      return GRN_UNKNOWN_ERROR;
    }
  }
}

void
grn_dat_init(grn_ctx *, grn_dat *dat)
{
  GRN_DB_OBJ_SET_TYPE(dat, GRN_TABLE_DAT_KEY);
  dat->io = NULL;
  dat->header = NULL;
  dat->file_id = 0;
  dat->encoding = GRN_ENC_DEFAULT;
  dat->trie = NULL;
  dat->old_trie = NULL;
  dat->tokenizer = NULL;
  CRITICAL_SECTION_INIT(dat->lock);
}

void
grn_dat_fin(grn_ctx *ctx, grn_dat *dat)
{
  CRITICAL_SECTION_FIN(dat->lock);
  delete static_cast<grn::dat::Trie *>(dat->old_trie);
  delete static_cast<grn::dat::Trie *>(dat->trie);
  dat->old_trie = NULL;
  dat->trie = NULL;
  if (dat->io) {
    grn_io_close(ctx, dat->io);
    dat->io = NULL;
  }
}

void
grn_dat_generate_trie_path(const char *base_path, char *trie_path, uint32_t file_id)
{
  if (!base_path || !base_path[0]) {
    trie_path[0] = '\0';
    return;
  }
  const size_t len = std::strlen(base_path);
  std::memcpy(trie_path, base_path, len);
  trie_path[len] = '.';
  grn_itoh(file_id % (1U << (4 * FILE_ID_LENGTH)), trie_path + len + 1, FILE_ID_LENGTH);
}

bool
grn_dat_open_trie_if_needed(grn_ctx *ctx, grn_dat *dat)
{
  if (!dat) {
    ERR(GRN_INVALID_ARGUMENT, const_cast<char *>("dat is null"));
    return false;
  }

  const uint32_t file_id = dat->header->file_id;
  if (!file_id || (dat->trie && (file_id <= dat->file_id))) {
    // There is no need to open file.
    return true;
  }

  CriticalSection critical_section(&dat->lock);

  if (dat->trie && (file_id <= dat->file_id)) {
    // There is no need to open file if the new file has been opened by another thread.
    return true;
  }

  char trie_path[PATH_MAX];
  grn_dat_generate_trie_path(grn_io_path(dat->io), trie_path, file_id);
  grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
  grn::dat::Trie * const old_trie = static_cast<grn::dat::Trie *>(dat->old_trie);
  grn::dat::Trie * const new_trie = new (std::nothrow) grn::dat::Trie;
  if (!new_trie) {
    MERR(const_cast<char *>("new grn::dat::Trie failed"));
    return false;
  }

  try {
    new_trie->open(trie_path);
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::open failed"));
    delete new_trie;
    return false;
  }

  dat->old_trie = trie;
  dat->trie = new_trie;
  dat->file_id = file_id;

  critical_section.leave();

  delete old_trie;
  if (file_id >= 3) {
    grn_dat_generate_trie_path(grn_io_path(dat->io), trie_path, file_id - 2);
    grn_io_remove(ctx, trie_path);
  }
  return true;
}

bool grn_dat_rebuild_trie(grn_ctx *ctx, grn_dat *dat) {
  grn::dat::Trie * const new_trie = new (std::nothrow) grn::dat::Trie;
  if (!new_trie) {
    MERR(const_cast<char *>("new grn::dat::Trie failed"));
    return false;
  }

  const uint32_t file_id = dat->header->file_id;
  try {
    char trie_path[PATH_MAX];
    grn_dat_generate_trie_path(grn_io_path(dat->io), trie_path, file_id + 1);
    const grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
    new_trie->create(*trie, trie_path, trie->file_size() * 2);
//    new_trie->create(*trie, trie_path, (grn::dat::UInt64)(trie->file_size() * 1.5));
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::open failed"));
    delete new_trie;
    return false;
  }

  grn::dat::Trie * const old_trie = static_cast<grn::dat::Trie *>(dat->old_trie);
  dat->old_trie = dat->trie;
  dat->trie = new_trie;
  dat->header->file_id = dat->file_id = file_id + 1;

  delete old_trie;
  if (file_id >= 2) {
    char trie_path[PATH_MAX];
    grn_dat_generate_trie_path(grn_io_path(dat->io), trie_path, file_id - 1);
    grn_io_remove(ctx, trie_path);
  }
  return true;
}

void grn_dat_cursor_init(grn_ctx *, grn_dat_cursor *cursor) {
  GRN_DB_OBJ_SET_TYPE(cursor, GRN_CURSOR_TABLE_DAT_KEY);
  cursor->dat = NULL;
  cursor->cursor = NULL;
  cursor->key = &grn::dat::Key::invalid_key();
  cursor->curr_rec = GRN_ID_NIL;
}

void grn_dat_cursor_fin(grn_ctx *, grn_dat_cursor *cursor) {
  delete static_cast<grn::dat::Cursor *>(cursor->cursor);
  cursor->dat = NULL;
  cursor->cursor = NULL;
  cursor->key = &grn::dat::Key::invalid_key();
  cursor->curr_rec = GRN_ID_NIL;
}

}  // namespace

extern "C" {

grn_dat *
grn_dat_create(grn_ctx *ctx, const char *path, uint32_t,
               uint32_t, uint32_t flags)
{
  if (path) {
    if (path[0] == '\0') {
      path = NULL;
    } else if (std::strlen(path) >= (PATH_MAX - (FILE_ID_LENGTH + 1))) {
      ERR(GRN_FILENAME_TOO_LONG, const_cast<char *>("too long path"));
      return NULL;
    }
  }

  grn_dat * const dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
  if (!dat) {
    return NULL;
  }
  grn_dat_init(ctx, dat);
  dat->obj.header.flags = flags;

  dat->io = grn_io_create(ctx, path, sizeof(struct grn_dat_header),
                          4096, 0, grn_io_auto, GRN_IO_EXPIRE_SEGMENT);
  if (!dat->io) {
    GRN_FREE(dat);
    return NULL;
  }
  grn_io_set_type(dat->io, GRN_TABLE_DAT_KEY);

  dat->header = static_cast<struct grn_dat_header *>(grn_io_header(dat->io));
  if (!dat->header) {
    grn_io_close(ctx, dat->io);
    grn_io_remove(ctx, path);
    GRN_FREE(dat);
    return NULL;
  }
  const grn_encoding encoding = (ctx->encoding != GRN_ENC_DEFAULT) ?
      ctx->encoding : grn_gctx.encoding;
  dat->header->flags = flags;
  dat->header->encoding = encoding;
  dat->header->tokenizer = 0;
  dat->header->file_id = 0;
  dat->encoding = encoding;
  dat->tokenizer = grn_ctx_at(ctx, dat->header->tokenizer);
  return dat;
}

grn_dat *
grn_dat_open(grn_ctx *ctx, const char *path)
{
  if (path && (std::strlen(path) >= (PATH_MAX - (FILE_ID_LENGTH + 1)))) {
    ERR(GRN_FILENAME_TOO_LONG, const_cast<char *>("too long path"));
    return NULL;
  }

  grn_dat * const dat = static_cast<grn_dat *>(GRN_MALLOC(sizeof(grn_dat)));
  if (!dat) {
    return NULL;
  }

  grn_dat_init(ctx, dat);
  dat->io = grn_io_open(ctx, path, grn_io_auto);
  if (!dat->io) {
    GRN_FREE(dat);
    return NULL;
  }

  dat->header = (struct grn_dat_header *)grn_io_header(dat->io);
  if (!dat->header) {
    grn_io_close(ctx, dat->io);
    GRN_FREE(dat);
    return NULL;
  }
  dat->file_id = dat->header->file_id;
  dat->encoding = dat->header->encoding;
  dat->obj.header.flags = dat->header->flags;
  dat->tokenizer = grn_ctx_at(ctx, dat->header->tokenizer);
  return dat;
}

grn_rc
grn_dat_close(grn_ctx *ctx, grn_dat *dat)
{
  if (dat) {
    grn_dat_fin(ctx, dat);
    GRN_FREE(dat);
  }
  return GRN_SUCCESS;
}

grn_rc
grn_dat_remove(grn_ctx *ctx, const char *path)
{
  if (!path) {
    ERR(GRN_INVALID_ARGUMENT, const_cast<char *>("path is null"));
    return GRN_INVALID_ARGUMENT;
  }

  grn_dat * const dat = grn_dat_open(ctx, path);
  if (!dat) {
    return ctx->rc;
  }
  const uint32_t file_id = dat->header->file_id;
  grn_dat_close(ctx, dat);

  for (uint32_t i = file_id; i > 0; --i) {
    char trie_path[PATH_MAX];
    grn_dat_generate_trie_path(path, trie_path, i);
    if (grn_io_remove(ctx, trie_path) != GRN_SUCCESS) {
      break;
    }
  }
  return grn_io_remove(ctx, path);
}

grn_id
grn_dat_get(grn_ctx *ctx, grn_dat *dat, const void *key,
            unsigned int key_size, void **)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return GRN_ID_NIL;
  }
  const grn::dat::Trie * const trie = static_cast<const grn::dat::Trie *>(dat->trie);
  if (!trie) {
    return GRN_ID_NIL;
  }
  grn::dat::UInt32 key_pos;
  try {
    if (trie->search(key, key_size, &key_pos)) {
      return trie->get_key(key_pos).id();
    }
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::search failed"));
  }
  return GRN_ID_NIL;
}

grn_id
grn_dat_add(grn_ctx *ctx, grn_dat *dat, const void *key,
            unsigned int key_size, void **, int *added)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return GRN_ID_NIL;
  }

  if (!dat->trie) {
    char trie_path[PATH_MAX];
    grn_dat_generate_trie_path(grn_io_path(dat->io), trie_path, 1);
    grn::dat::Trie * const new_trie = new (std::nothrow) grn::dat::Trie;
    if (!new_trie) {
      MERR(const_cast<char *>("new grn::dat::Trie failed"));
      return GRN_ID_NIL;
    }
    try {
      new_trie->create(trie_path);
    } catch (const grn::dat::Exception &ex) {
      ERR(grn_dat_translate_error_code(ex.code()),
          const_cast<char *>("grn::dat::Trie::create failed"));
      delete new_trie;
      return GRN_ID_NIL;
    }
    dat->trie = new_trie;
    dat->file_id = dat->header->file_id = 1;
  }

  grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
  try {
    grn::dat::UInt32 key_pos;
    const bool res = trie->insert(key, key_size, &key_pos);
    if (added) {
      *added = res ? 1 : 0;
    }
    return trie->get_key(key_pos).id();
  } catch (const grn::dat::SizeError &ex) {
    if (!grn_dat_rebuild_trie(ctx, dat)) {
      return GRN_ID_NIL;
    }
    grn::dat::Trie * const new_trie = static_cast<grn::dat::Trie *>(dat->trie);
    grn::dat::UInt32 key_pos;
    const bool res = new_trie->insert(key, key_size, &key_pos);
    if (added) {
      *added = res ? 1 : 0;
    }
    return new_trie->get_key(key_pos).id();
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::insert failed"));
    return GRN_ID_NIL;
  }
}

int
grn_dat_get_key(grn_ctx *ctx, grn_dat *dat, grn_id id, void *keybuf, int bufsize)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return 0;
  }
  const grn::dat::Trie * const trie = static_cast<const grn::dat::Trie *>(dat->trie);
  if (!trie) {
    return 0;
  }
  const grn::dat::Key &key = trie->ith_key(id);
  if (!key.is_valid()) {
    return 0;
  }
  if (keybuf && (bufsize >= (int)key.length())) {
    std::memcpy(keybuf, key.ptr(), key.length());
  }
  return (int)key.length();
}

int
grn_dat_get_key2(grn_ctx *ctx, grn_dat *dat, grn_id id, grn_obj *bulk)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return 0;
  }
  const grn::dat::Trie * const trie = static_cast<const grn::dat::Trie *>(dat->trie);
  if (!trie) {
    return 0;
  }
  const grn::dat::Key &key = trie->ith_key(id);
  if (!key.is_valid()) {
    return 0;
  }
  if (bulk->header.impl_flags & GRN_OBJ_REFER) {
    bulk->u.b.head = static_cast<char *>(const_cast<void *>(key.ptr()));
    bulk->u.b.curr = bulk->u.b.head + key.length();
  } else {
    grn_bulk_write(ctx, bulk, static_cast<const char *>(key.ptr()), key.length());
  }
  return (int)key.length();
}

grn_rc
grn_dat_delete_by_id(grn_ctx *ctx, grn_dat *dat, grn_id id,
                     grn_table_delete_optarg *optarg)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return ctx->rc;
  } else if (!dat->trie) {
    return GRN_INVALID_ARGUMENT;
  }

  if (optarg && optarg->func) {
    const grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
    if (!trie->ith_entry(id).is_valid()) {
      return GRN_INVALID_ARGUMENT;
    } else if (!optarg->func(ctx, reinterpret_cast<grn_obj *>(dat), id, optarg->func_arg)) {
      return GRN_SUCCESS;
    }
  }

  try {
    grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
    if (!trie->remove(id)) {
      return GRN_INVALID_ARGUMENT;
    }
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::remove failed"));
    return GRN_INVALID_ARGUMENT;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_dat_delete(grn_ctx *ctx, grn_dat *dat, const void *key, unsigned int key_size,
               grn_table_delete_optarg *optarg)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return ctx->rc;
  } else if (!dat->trie) {
    return GRN_INVALID_ARGUMENT;
  }

  if (optarg && optarg->func) {
    try {
      const grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
      grn::dat::UInt32 key_pos;
      if (!trie->search(key, key_size, &key_pos)) {
        return GRN_INVALID_ARGUMENT;
      } else if (!optarg->func(ctx, reinterpret_cast<grn_obj *>(dat),
                               trie->get_key(key_pos).id(), optarg->func_arg)) {
        return GRN_SUCCESS;
      }
    } catch (const grn::dat::Exception &ex) {
      ERR(grn_dat_translate_error_code(ex.code()),
          const_cast<char *>("grn::dat::Trie::search failed"));
    }
  }

  try {
    grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
    if (!trie->remove(key, key_size)) {
      return GRN_INVALID_ARGUMENT;
    }
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::remove failed"));
    return GRN_INVALID_ARGUMENT;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_dat_update_by_id(grn_ctx *ctx, grn_dat *dat, grn_id src_key_id,
                     const void *dest_key, unsigned int dest_key_size)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return ctx->rc;
  } else if (!dat->trie) {
    return GRN_INVALID_ARGUMENT;
  }
  try {
    try {
      grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
      if (!trie->update(src_key_id, dest_key, dest_key_size)) {
        return GRN_INVALID_ARGUMENT;
      }
    } catch (const grn::dat::SizeError &ex) {
      if (!grn_dat_rebuild_trie(ctx, dat)) {
        return ctx->rc;
      }
      grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
      grn::dat::UInt32 key_pos;
      if (!trie->update(src_key_id, dest_key, dest_key_size)) {
        return GRN_INVALID_ARGUMENT;
      }
    }
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::update failed"));
    return GRN_INVALID_ARGUMENT;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_dat_update(grn_ctx *ctx, grn_dat *dat,
               const void *src_key, unsigned int src_key_size,
               const void *dest_key, unsigned int dest_key_size)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return ctx->rc;
  } else if (!dat->trie) {
    return GRN_INVALID_ARGUMENT;
  }
  try {
    try {
      grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
      if (!trie->update(src_key, src_key_size, dest_key, dest_key_size)) {
        return GRN_INVALID_ARGUMENT;
      }
    } catch (const grn::dat::SizeError &ex) {
      if (!grn_dat_rebuild_trie(ctx, dat)) {
        return ctx->rc;
      }
      grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
      grn::dat::UInt32 key_pos;
      if (!trie->update(src_key, src_key_size, dest_key, dest_key_size)) {
        return GRN_INVALID_ARGUMENT;
      }
    }
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::update failed"));
    return GRN_INVALID_ARGUMENT;
  }
  return GRN_SUCCESS;
}

unsigned int
grn_dat_size(grn_ctx *ctx, grn_dat *dat)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return 0;
  }
  const grn::dat::Trie * const trie = static_cast<const grn::dat::Trie *>(dat->trie);
  if (trie) {
    return trie->num_keys();
  }
  return 0;
}

grn_dat_cursor *
grn_dat_cursor_open(grn_ctx *ctx, grn_dat *dat,
                    const void *min, unsigned int min_size,
                    const void *max, unsigned int max_size,
                    int offset, int limit, int flags)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return NULL;
  }

  grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
  if (!trie) {
    grn_dat_cursor * const dc =
        static_cast<grn_dat_cursor *>(GRN_MALLOC(sizeof(grn_dat_cursor)));
    if (dc) {
      grn_dat_cursor_init(ctx, dc);
    }
    return dc;
  }

  grn_dat_cursor * const dc =
      static_cast<grn_dat_cursor *>(GRN_MALLOC(sizeof(grn_dat_cursor)));
  if (!dc) {
    return NULL;
  }
  grn_dat_cursor_init(ctx, dc);

  try {
    if ((flags & GRN_CURSOR_BY_ID) != 0) {
      dc->cursor = grn::dat::CursorFactory::open(*trie,
          min, min_size, max, max_size, offset, limit,
          grn::dat::ID_RANGE_CURSOR |
          ((flags & GRN_CURSOR_DESCENDING) ? grn::dat::DESCENDING_CURSOR : 0) |
          ((flags & GRN_CURSOR_GT) ? grn::dat::EXCEPT_LOWER_BOUND : 0) |
          ((flags & GRN_CURSOR_LT) ? grn::dat::EXCEPT_UPPER_BOUND : 0));
    } else if ((flags & GRN_CURSOR_PREFIX) != 0) {
      if (max && max_size) {
        if ((dat->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) != 0) {
          dc->cursor = grn::dat::CursorFactory::open(*trie,
              NULL, min_size, max, max_size, offset, limit,
              grn::dat::PREFIX_CURSOR | grn::dat::DESCENDING_CURSOR);
        } else {
          // TODO: near
        }
      } else if (min && min_size) {
        if ((flags & GRN_CURSOR_RK) != 0) {
          // TODO: rk search
        } else {
          dc->cursor = grn::dat::CursorFactory::open(*trie,
              min, min_size, NULL, 0, offset, limit,
              grn::dat::PREDICTIVE_CURSOR |
              ((flags & GRN_CURSOR_DESCENDING) ? grn::dat::DESCENDING_CURSOR : 0) |
              ((flags & GRN_CURSOR_GT) ? grn::dat::EXCEPT_EXACT_MATCH : 0));
        }
      }
    } else {
      dc->cursor = grn::dat::CursorFactory::open(*trie,
          min, min_size, max, max_size, offset, limit,
          grn::dat::KEY_RANGE_CURSOR |
          ((flags & GRN_CURSOR_DESCENDING) ? grn::dat::DESCENDING_CURSOR : 0) |
          ((flags & GRN_CURSOR_GT) ? grn::dat::EXCEPT_LOWER_BOUND : 0) |
          ((flags & GRN_CURSOR_LT) ? grn::dat::EXCEPT_UPPER_BOUND : 0));
    }
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::CursorFactory::open failed"));
    GRN_FREE(dc);
    return NULL;
  }
  if (!dc->cursor) {
    ERR(GRN_INVALID_ARGUMENT, const_cast<char *>("unsupported query"));
    GRN_FREE(dc);
    return NULL;
  }
  dc->dat = dat;
  return dc;
}

grn_id
grn_dat_cursor_next(grn_ctx *ctx, grn_dat_cursor *c)
{
  if (!c || !c->cursor) {
    return GRN_ID_NIL;
  }
  try {
    grn::dat::Cursor * const cursor = static_cast<grn::dat::Cursor *>(c->cursor);
    const grn::dat::Key &key = cursor->next();
    c->key = &key;
    c->curr_rec = key.is_valid() ? key.id() : GRN_ID_NIL;
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Cursor::next failed"));
    return GRN_ID_NIL;
  }
  return c->curr_rec;
}

void
grn_dat_cursor_close(grn_ctx *ctx, grn_dat_cursor *c)
{
  if (c) {
    grn_dat_cursor_fin(ctx, c);
    GRN_FREE(c);
  }
}

int
grn_dat_cursor_get_key(grn_ctx *ctx, grn_dat_cursor *c, const void **key)
{
  if (c) {
    const grn::dat::Key &key_ref = *static_cast<const grn::dat::Key *>(c->key);
    if (key_ref.is_valid()) {
      *key = key_ref.ptr();
      return (int)key_ref.length();
    }
  }
  return 0;
}

grn_rc
grn_dat_cursor_delete(grn_ctx *ctx, grn_dat_cursor *c,
                      grn_table_delete_optarg *optarg)
{
  if (!c || !c->cursor) {
    return GRN_INVALID_ARGUMENT;
  } else if (!grn_dat_open_trie_if_needed(ctx, c->dat)) {
    return ctx->rc;
  }
  grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(c->dat->trie);
  if (!trie) {
    return GRN_INVALID_ARGUMENT;
  }
  try {
    if (trie->remove(c->curr_rec)) {
      return GRN_SUCCESS;
    }
  } catch (const grn::dat::Exception &ex) {
    ERR(grn_dat_translate_error_code(ex.code()),
        const_cast<char *>("grn::dat::Trie::remove failed"));
    return GRN_INVALID_ARGUMENT;
  }
  return GRN_INVALID_ARGUMENT;
}

grn_id
grn_dat_curr_id(grn_ctx *ctx, grn_dat *dat)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return GRN_ID_NIL;
  }
  const grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
  if (trie) {
    return trie->max_key_id();
  }
  return GRN_ID_NIL;
}

grn_rc
grn_dat_truncate(grn_ctx *ctx, grn_dat *dat)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return ctx->rc;
  }
  const grn::dat::Trie * const trie = static_cast<const grn::dat::Trie *>(dat->trie);
  if (!trie || !trie->max_key_id()) {
    return GRN_SUCCESS;
  }

  char trie_path[PATH_MAX];
  grn_dat_generate_trie_path(grn_io_path(dat->io), trie_path, dat->header->file_id + 1);
  try {
    grn::dat::Trie().create(trie_path);
  } catch (const grn::dat::Exception &ex) {
    const grn_rc error_code = grn_dat_translate_error_code(ex.code());
    ERR(error_code, const_cast<char *>("grn::dat::Trie::create failed"));
    return error_code;
  }
  ++dat->header->file_id;
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return ctx->rc;
  }
  return GRN_SUCCESS;
}

const char *
_grn_dat_key(grn_ctx *ctx, grn_dat *dat, grn_id id, uint32_t *key_size)
{
  if (!grn_dat_open_trie_if_needed(ctx, dat)) {
    return NULL;
  }
  const grn::dat::Trie * const trie = static_cast<grn::dat::Trie *>(dat->trie);
  if (!trie) {
    return NULL;
  }
  const grn::dat::Key &key = trie->ith_key(id);
  if (!key.is_valid()) {
    return NULL;
  }
  *key_size = key.length();
  return static_cast<const char *>(key.ptr());
}

}  // extern "C"
