/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2012 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
#include "hash.h"
#include "output.h"
#include <string.h>
#include <limits.h>

#include "store.h"

/* grn_tiny_array */

inline static int
grn_tiny_array_get_block_id(grn_tiny_array *array, grn_id id)
{
  int most_significant_one_bit_offset;
  GRN_BIT_SCAN_REV(id, most_significant_one_bit_offset);
  return most_significant_one_bit_offset >> GRN_TINY_ARRAY_W;
}

inline static void *
grn_tiny_array_at_inline(grn_tiny_array *array, grn_id id)
{
  int block_id;
  void **block;
  size_t offset;
  if (!id) {
    return NULL;
  }
  block_id = grn_tiny_array_get_block_id(array, id);
  block = &array->elements[block_id];
  offset = GRN_TINY_ARRAY_R(block_id);
  if (!*block) {
    grn_ctx * const ctx = array->ctx;
    if (array->flags & GRN_TINY_ARRAY_THREADSAFE) {
      CRITICAL_SECTION_ENTER(array->lock);
    }
    if (!*block) {
      const size_t block_size =
          GRN_TINY_ARRAY_S * offset * array->element_size;
      if (array->flags & GRN_TINY_ARRAY_USE_MALLOC) {
        if (array->flags & GRN_TINY_ARRAY_CLEAR) {
          *block = GRN_CALLOC(block_size);
        } else {
          *block = GRN_MALLOC(block_size);
        }
      } else {
        *block = GRN_CTX_ALLOC(ctx, block_size);
      }
    }
    if (array->flags & GRN_TINY_ARRAY_THREADSAFE) {
      CRITICAL_SECTION_LEAVE(array->lock);
    }
    if (!*block) {
      return NULL;
    }
  }
  if (id > array->max) {
    array->max = id;
  }
  return (byte *)*block + (id - offset) * array->element_size;
}

inline static void *
grn_tiny_array_next(grn_tiny_array *array)
{
  return grn_tiny_array_at_inline(array, array->max + 1);
}

/*
 * grn_tiny_array_bit_at() returns the default value (0) when
 * grn_tiny_array_at_inline() returns NULL. So, it seems to work well.
 */
inline static grn_bool
grn_tiny_array_bit_at(grn_tiny_array *array, grn_id offset)
{
  uint8_t * const ptr =
      (uint8_t *)grn_tiny_array_at_inline(array, (offset >> 3) + 1);
  return ptr ? ((*ptr >> (offset & 7)) & 1) : 0;
}

inline static void *
grn_tiny_array_bit_on(grn_tiny_array *array, grn_id offset)
{
  uint8_t * const ptr =
      (uint8_t *)grn_tiny_array_at_inline(array, (offset >> 3) + 1);
  if (ptr) {
    *ptr |= 1 << (offset & 7);
  }
  return ptr;
}

inline static void *
grn_tiny_array_bit_off(grn_tiny_array *array, grn_id offset)
{
  uint8_t * const ptr =
      (uint8_t *)grn_tiny_array_at_inline(array, (offset >> 3) + 1);
  if (ptr) {
    *ptr &= ~(1 << (offset & 7));
  }
  return ptr;
}

inline static void *
grn_tiny_array_bit_flip(grn_tiny_array *array, grn_id offset)
{
  uint8_t * const ptr =
      (uint8_t *)grn_tiny_array_at_inline(array, (offset >> 3) + 1);
  if (ptr) {
    *ptr ^= 1 << (offset & 7);
  }
  return ptr;
}

void
grn_tiny_array_init(grn_ctx *ctx, grn_tiny_array *array,
                    uint16_t element_size, uint16_t flags)
{
  array->ctx = ctx;
  array->element_size = element_size;
  array->flags = flags;
  array->max = 0;
  if (flags & GRN_TINY_ARRAY_THREADSAFE) {
    CRITICAL_SECTION_INIT(array->lock);
  }
  memset(array->elements, 0, sizeof(array->elements));
}

void
grn_tiny_array_fin(grn_tiny_array *array)
{
  int block_id;
  grn_ctx * const ctx = array->ctx;
  for (block_id = 0; block_id < GRN_TINY_ARRAY_N; block_id++) {
    void ** const block = &array->elements[block_id];
    if (*block) {
      if (array->flags & GRN_TINY_ARRAY_USE_MALLOC) {
        GRN_FREE(*block);
      } else {
        GRN_CTX_FREE(ctx, *block);
      }
      *block = NULL;
    }
  }
}

void *
grn_tiny_array_at(grn_tiny_array *array, grn_id id)
{
  return grn_tiny_array_at_inline(array, id);
}

grn_id
grn_tiny_array_id(grn_tiny_array *array, const void *element_address)
{
  const byte * const ptr = (const byte *)element_address;
  uint32_t block_id, offset = 1;
  for (block_id = 0; block_id < GRN_TINY_ARRAY_N; block_id++) {
    const uint32_t block_size = GRN_TINY_ARRAY_S * GRN_TINY_ARRAY_R(block_id);
    const byte * const block = (const byte *)array->elements[block_id];
    if (block && block <= ptr &&
        ptr < (block + block_size * array->element_size)) {
      return offset + ((ptr - block) / array->element_size);
    }
    offset += block_size;
  }
  return GRN_ID_NIL;
}

/* grn_array */

#define GRN_ARRAY_HEADER_SIZE  0x9000
#define GRN_ARRAY_SEGMENT_SIZE 0x400000

struct grn_array_header {
  uint32_t flags;
  uint32_t curr_rec;
  uint32_t value_size;
  uint32_t n_entries;
  uint32_t n_garbages;
  grn_id garbages;
  uint32_t lock;
  uint32_t reserved[5];
};

enum {
  array_seg_value = 0,
  array_seg_bitmap = 1
};

inline static grn_bool
grn_array_is_io_array(grn_array *array)
{
  return array->io != NULL;
}

inline static void *
grn_array_io_entry_at(grn_ctx *ctx, grn_array *array, grn_id id, int flags)
{
  void *value;
  GRN_IO_ARRAY_AT(array->io, array_seg_value, id, &flags, value);
  return value;
}

inline static void *
grn_array_entry_at(grn_ctx *ctx, grn_array *array, grn_id id, int flags)
{
  if (grn_array_is_io_array(array)) {
    return grn_array_io_entry_at(ctx, array, id, flags);
  } else {
    return grn_tiny_array_at_inline(&array->a, id);
  }
}

inline static grn_bool
grn_array_bitmap_at(grn_ctx *ctx, grn_array *array, grn_id id)
{
  if (grn_array_is_io_array(array)) {
    grn_bool value;
    GRN_IO_ARRAY_BIT_AT(array->io, array_seg_bitmap, id, value);
    return value;
  } else {
    return grn_tiny_array_bit_at(&array->bitmap, id);
  }
}

static grn_rc
grn_array_init_tiny_array(grn_ctx *ctx, grn_array *array, const char *path,
                          uint32_t value_size, uint32_t flags)
{
  if (path) {
    ERR(GRN_INVALID_ARGUMENT, "failed to create tiny array");
    return ctx->rc;
  }
  array->obj.header.flags = flags;
  array->ctx = ctx;
  array->value_size = value_size;
  array->n_keys = 0;
  array->keys = NULL;
  array->n_garbages = &array->n_garbages_;
  array->n_entries = &array->n_entries_;
  array->n_garbages_ = 0;
  array->n_entries_ = 0;
  array->io = NULL;
  array->garbages = GRN_ID_NIL;
  grn_tiny_array_init(ctx, &array->a, value_size, GRN_TINY_ARRAY_CLEAR);
  grn_tiny_array_init(ctx, &array->bitmap, 1, GRN_TINY_ARRAY_CLEAR);
  return GRN_SUCCESS;
}

static grn_io *
grn_array_create_io_array(grn_ctx *ctx, grn_array *array, const char *path,
                          uint32_t value_size)
{
  uint32_t w_of_element = 0;
  grn_io_array_spec array_spec[2];

  while ((1U << w_of_element) < value_size) {
    w_of_element++;
  }

  array_spec[array_seg_value].w_of_element = w_of_element;
  array_spec[array_seg_value].max_n_segments = 1U << (30 - (22 - w_of_element));
  array_spec[array_seg_bitmap].w_of_element = 0;
  array_spec[array_seg_bitmap].max_n_segments = 1U << (30 - (22 + 3));
  return grn_io_create_with_array(ctx, path, sizeof(struct grn_array_header),
                                  GRN_ARRAY_SEGMENT_SIZE, grn_io_auto,
                                  2, array_spec);
}

static grn_rc
grn_array_init_io_array(grn_ctx *ctx, grn_array *array, const char *path,
                        uint32_t value_size, uint32_t flags)
{
  grn_io *io;
  struct grn_array_header *header;

  io = grn_array_create_io_array(ctx, array, path, value_size);
  if (!io) {
    return ctx->rc;
  }
  grn_io_set_type(io, GRN_TABLE_NO_KEY);

  header = grn_io_header(io);
  header->flags = flags;
  header->curr_rec = 0;
  header->lock = 0;
  header->value_size = value_size;
  header->n_entries = 0;
  header->n_garbages = 0;
  header->garbages = GRN_ID_NIL;

  array->obj.header.flags = flags;
  array->ctx = ctx;
  array->value_size = value_size;
  array->n_keys = 0;
  array->keys = NULL;
  array->n_garbages = &header->n_garbages;
  array->n_entries = &header->n_entries;
  array->io = io;
  array->header = header;
  array->lock = &header->lock;
  return GRN_SUCCESS;
}

static grn_rc
grn_array_init(grn_ctx *ctx, grn_array *array,
               const char *path, uint32_t value_size, uint32_t flags)
{
  if (flags & GRN_ARRAY_TINY) {
    return grn_array_init_tiny_array(ctx, array, path, value_size, flags);
  } else {
    return grn_array_init_io_array(ctx, array, path, value_size, flags);
  }
}

grn_array *
grn_array_create(grn_ctx *ctx, const char *path, uint32_t value_size, uint32_t flags)
{
  if (ctx) {
    grn_array * const array = (grn_array *)GRN_MALLOC(sizeof(grn_array));
    if (array) {
      GRN_DB_OBJ_SET_TYPE(array, GRN_TABLE_NO_KEY);
      if (!grn_array_init(ctx, array, path, value_size, flags)) {
        return array;
      }
      GRN_FREE(array);
    }
  }
  return NULL;
}

grn_array *
grn_array_open(grn_ctx *ctx, const char *path)
{
  if (ctx) {
    grn_io * const io = grn_io_open(ctx, path, grn_io_auto);
    if (io) {
      struct grn_array_header * const header = grn_io_header(io);
      if (grn_io_get_type(io) == GRN_TABLE_NO_KEY) {
        grn_array * const array = (grn_array *)GRN_MALLOC(sizeof(grn_array));
        if (array) {
          if (!(header->flags & GRN_ARRAY_TINY)) {
            GRN_DB_OBJ_SET_TYPE(array, GRN_TABLE_NO_KEY);
            array->obj.header.flags = header->flags;
            array->ctx = ctx;
            array->value_size = header->value_size;
            array->n_keys = 0;
            array->keys = NULL;
            array->n_garbages = &header->n_garbages;
            array->n_entries = &header->n_entries;
            array->io = io;
            array->header = header;
            array->lock = &header->lock;
            return array;
          } else {
            GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid array flags. (%x)", header->flags);
          }
          GRN_FREE(array);
        }
      } else {
        ERR(GRN_INVALID_FORMAT, "file type unmatch");
      }
      grn_io_close(ctx, io);
    }
  }
  return NULL;
}

grn_rc
grn_array_close(grn_ctx *ctx, grn_array *array)
{
  grn_rc rc = GRN_SUCCESS;
  if (!ctx || !array) { return GRN_INVALID_ARGUMENT; }
  if (array->keys) { GRN_FREE(array->keys); }
  if (grn_array_is_io_array(array)) {
    rc = grn_io_close(ctx, array->io);
  } else {
    GRN_ASSERT(ctx == array->ctx);
    grn_tiny_array_fin(&array->a);
    grn_tiny_array_fin(&array->bitmap);
  }
  GRN_FREE(array);
  return rc;
}

grn_rc
grn_array_remove(grn_ctx *ctx, const char *path)
{
  if (!ctx || !path) { return GRN_INVALID_ARGUMENT; }
  return grn_io_remove(ctx, path);
}

grn_rc
grn_array_truncate(grn_ctx *ctx, grn_array *array)
{
  grn_rc rc = GRN_SUCCESS;
  char *path = NULL;
  uint32_t value_size, flags;

  if (!ctx || !array) { return GRN_INVALID_ARGUMENT; }
  if (grn_array_is_io_array(array)) {
    const char * const io_path = grn_io_path(array->io);
    if (io_path && *io_path) {
      path = GRN_STRDUP(io_path);
      if (!path) {
        ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path.");
        return GRN_NO_MEMORY_AVAILABLE;
      }
    }
  }
  value_size = array->value_size;
  flags = array->obj.header.flags;

  if (grn_array_is_io_array(array)) {
    rc = grn_io_close(ctx, array->io);
    if (!rc) {
      array->io = NULL;
      if (path) {
        rc = grn_io_remove(ctx, path);
      }
    }
  }
  if (!rc) {
    rc = grn_array_init(ctx, array, path, value_size, flags);
  }
  if (path) { GRN_FREE(path); }
  return rc;
}

int
grn_array_get_value(grn_ctx *ctx, grn_array *array, grn_id id, void *valuebuf)
{
  if (ctx && array) {
    if (grn_array_bitmap_at(ctx, array, id)) {
      void * const entry = grn_array_entry_at(ctx, array, id, 0);
      if (entry) {
        if (valuebuf) {
          memcpy(valuebuf, entry, array->value_size);
        }
        return array->value_size;
      }
    }
  }
  return 0;
}

void *
_grn_array_get_value(grn_ctx *ctx, grn_array *array, grn_id id)
{
  if (ctx && array) {
    if (!grn_array_bitmap_at(ctx, array, id)) { return NULL; }
    return grn_array_entry_at(ctx, array, id, 0);
  }
  return NULL;
}

grn_rc
grn_array_set_value(grn_ctx *ctx, grn_array *array, grn_id id,
                    const void *value, int flags)
{
  if (!ctx || !array || !value) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!grn_array_bitmap_at(ctx, array, id)) {
    return GRN_INVALID_ARGUMENT;
  }

  {
    void * const entry = grn_array_entry_at(ctx, array, id, 0);
    if (!entry) {
      return GRN_NO_MEMORY_AVAILABLE;
    }

    switch ((flags & GRN_OBJ_SET_MASK)) {
    case GRN_OBJ_SET :
      memcpy(entry, value, array->value_size);
      return GRN_SUCCESS;
    case GRN_OBJ_INCR :
      switch (array->value_size) {
      case sizeof(int32_t) :
        *((int32_t *)entry) += *((int32_t *)value);
        return GRN_SUCCESS;
      case sizeof(int64_t) :
        *((int64_t *)entry) += *((int64_t *)value);
        return GRN_SUCCESS;
      default :
        return GRN_INVALID_ARGUMENT;
      }
      break;
    case GRN_OBJ_DECR :
      switch (array->value_size) {
      case sizeof(int32_t) :
        *((int32_t *)entry) -= *((int32_t *)value);
        return GRN_SUCCESS;
      case sizeof(int64_t) :
        *((int64_t *)entry) -= *((int64_t *)value);
        return GRN_SUCCESS;
      default :
        return GRN_INVALID_ARGUMENT;
      }
      break;
    default :
      // todo : support other types.
      return GRN_INVALID_ARGUMENT;
    }
  }
}

grn_rc
grn_array_delete_by_id(grn_ctx *ctx, grn_array *array, grn_id id,
                       grn_table_delete_optarg *optarg)
{
  if (!ctx || !array) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!grn_array_bitmap_at(ctx, array, id)) {
    return GRN_INVALID_ARGUMENT;
  }

  {
    grn_rc rc = GRN_SUCCESS;
    /* lock */
    if (grn_array_is_io_array(array)) {
      if (array->value_size >= sizeof(grn_id)) {
        struct grn_array_header * const header = array->header;
        void * const entry = grn_array_io_entry_at(ctx, array, id, 0);
        if (!entry) {
          rc = GRN_INVALID_ARGUMENT;
        } else {
          *((grn_id *)entry) = header->garbages;
          header->garbages = id;
        }
      }
      if (!rc) {
        (*array->n_entries)--;
        (*array->n_garbages)++;
        /*
         * The following GRN_IO_ARRAY_BIT_OFF() never fails because the above
         * grn_array_bitmap_at() returned 1 for the same ID.
         */
        GRN_IO_ARRAY_BIT_OFF(array->io, array_seg_bitmap, id);
      }
    } else {
      if (array->value_size >= sizeof(grn_id)) {
        void * const entry = grn_tiny_array_at_inline(&array->a, id);
        if (!entry) {
          rc = GRN_INVALID_ARGUMENT;
        } else {
          *((grn_id *)entry) = array->garbages;
          array->garbages = id;
        }
      }
      if (!rc) {
        (*array->n_entries)--;
        (*array->n_garbages)++;
        /*
         * The following grn_tiny_array_bit_off() never fails because the above
         * grn_array_bitmap_at() returned 1 for the same ID.
         */
        grn_tiny_array_bit_off(&array->bitmap, id);
      }
    }
    /* unlock */
    return rc;
  }
}

grn_id
grn_array_at(grn_ctx *ctx, grn_array *array, grn_id id)
{
  return grn_array_bitmap_at(ctx, array, id) ? id : GRN_ID_NIL;
}

grn_rc
grn_array_copy_sort_key(grn_ctx *ctx, grn_array *array,
                        grn_table_sort_key *keys, int n_keys)
{
  array->keys = GRN_MALLOCN(grn_table_sort_key, n_keys);
  if (!array->keys) {
    return ctx->rc;
  }
  memcpy(array->keys, keys, sizeof(grn_table_sort_key) * n_keys);
  array->n_keys = n_keys;
  return GRN_SUCCESS;
}

void
grn_array_cursor_close(grn_ctx *ctx, grn_array_cursor *cursor)
{
  GRN_ASSERT(cursor->ctx == ctx);
  GRN_FREE(cursor);
}

inline static grn_id
grn_array_get_max_id(grn_array *array)
{
  return grn_array_is_io_array(array) ? array->header->curr_rec : array->a.max;
}

grn_array_cursor *
grn_array_cursor_open(grn_ctx *ctx, grn_array *array, grn_id min, grn_id max,
                      int offset, int limit, int flags)
{
  grn_array_cursor *cursor;
  if (!array || !ctx) { return NULL; }

  cursor = GRN_MALLOCN(grn_array_cursor, 1);
  if (!cursor) { return NULL; }

  GRN_DB_OBJ_SET_TYPE(cursor, GRN_CURSOR_TABLE_NO_KEY);
  cursor->array = array;
  cursor->ctx = ctx;
  cursor->obj.header.flags = flags;
  cursor->obj.header.domain = GRN_ID_NIL;

  if (flags & GRN_CURSOR_DESCENDING) {
    cursor->dir = -1;
    if (max) {
      cursor->curr_rec = max;
      if (!(flags & GRN_CURSOR_LT)) { cursor->curr_rec++; }
    } else {
      cursor->curr_rec = grn_array_get_max_id(array) + 1;
    }
    if (min) {
      cursor->tail = min;
      if ((flags & GRN_CURSOR_GT)) { cursor->tail++; }
    } else {
      cursor->tail = GRN_ID_NIL + 1;
    }
    if (cursor->curr_rec < cursor->tail) { cursor->tail = cursor->curr_rec; }
  } else {
    cursor->dir = 1;
    if (min) {
      cursor->curr_rec = min;
      if (!(flags & GRN_CURSOR_GT)) { cursor->curr_rec--; }
    } else {
      cursor->curr_rec = GRN_ID_NIL;
    }
    if (max) {
      cursor->tail = max;
      if ((flags & GRN_CURSOR_LT)) { cursor->tail--; }
    } else {
      cursor->tail = grn_array_get_max_id(array);
    }
    if (cursor->tail < cursor->curr_rec) { cursor->tail = cursor->curr_rec; }
  }

  if (*array->n_garbages) {
    while (offset && cursor->curr_rec != cursor->tail) {
      cursor->curr_rec += cursor->dir;
      if (grn_array_bitmap_at(ctx, cursor->array, cursor->curr_rec)) { offset--; }
    }
  } else {
    cursor->curr_rec += cursor->dir * offset;
  }
  cursor->rest = (limit < 0) ? GRN_ID_MAX : limit;
  return cursor;
}

grn_id
grn_array_cursor_next(grn_ctx *ctx, grn_array_cursor *cursor)
{
  if (cursor && cursor->rest) {
    while (cursor->curr_rec != cursor->tail) {
      cursor->curr_rec += cursor->dir;
      if (*cursor->array->n_garbages) {
        if (!grn_array_bitmap_at(ctx, cursor->array, cursor->curr_rec)) { continue; }
      }
      cursor->rest--;
      return cursor->curr_rec;
    }
  }
  return GRN_ID_NIL;
}

grn_id
grn_array_next(grn_ctx *ctx, grn_array *array, grn_id id)
{
  grn_id max = grn_array_get_max_id(array);
  while (++id <= max) {
    if (grn_array_bitmap_at(ctx, array, id)) { return id; }
  }
  return GRN_ID_NIL;
}

int
grn_array_cursor_get_value(grn_ctx *ctx, grn_array_cursor *cursor, void **value)
{
  if (cursor && value) {
    void * const entry = grn_array_entry_at(ctx, cursor->array, cursor->curr_rec, 0);
    if (entry) {
      *value = entry;
      return cursor->array->value_size;
    }
  }
  return 0;
}

grn_rc
grn_array_cursor_set_value(grn_ctx *ctx, grn_array_cursor *cursor,
                           const void *value, int flags)
{
  return grn_array_set_value(ctx, cursor->array, cursor->curr_rec, value, flags);
}

grn_rc
grn_array_cursor_delete(grn_ctx *ctx, grn_array_cursor *cursor,
                        grn_table_delete_optarg *optarg)
{
  return grn_array_delete_by_id(ctx, cursor->array, cursor->curr_rec, optarg);
}

inline static grn_id
grn_array_add_to_tiny_array(grn_ctx *ctx, grn_array *array, void **value)
{
  grn_id id = array->garbages;
  void *entry;
  if (id) {
    entry = grn_tiny_array_at_inline(&array->a, id);
    array->garbages = *(grn_id *)entry;
    memset(entry, 0, array->value_size);
    (*array->n_garbages)--;
    grn_tiny_array_bit_on(&array->bitmap, id);
  } else {
    id = array->a.max + 1;
    if (!grn_tiny_array_bit_on(&array->bitmap, id)) {
      return GRN_ID_NIL;
    }
    entry = grn_tiny_array_at_inline(&array->a, id);
    if (!entry) {
      grn_tiny_array_bit_off(&array->bitmap, id);
      return GRN_ID_NIL;
    }
    array->a.max = id;
  }
  (*array->n_entries)++;
  if (value) { *value = entry; }
  return id;
}

inline static grn_id
grn_array_add_to_io_array(grn_ctx *ctx, grn_array *array, void **value)
{
  struct grn_array_header * const header = array->header;
  grn_id id = header->garbages;
  void *entry;
  if (id) {
    entry = grn_array_io_entry_at(ctx, array, id, GRN_TABLE_ADD);
    if (!entry) {
      return GRN_ID_NIL;
    }
    header->garbages = *(grn_id *)entry;
    memset(entry, 0, header->value_size);
    (*array->n_garbages)--;
    /* FIXME: GRN_IO_ARRAY_BIT_ON() may fail and cause a critical problem. */
    GRN_IO_ARRAY_BIT_ON(array->io, array_seg_bitmap, id);
  } else {
    id = header->curr_rec + 1;
    /* FIXME: GRN_IO_ARRAY_BIT_ON() may fail and cause a critical problem. */
    GRN_IO_ARRAY_BIT_ON(array->io, array_seg_bitmap, id);
    entry = grn_array_io_entry_at(ctx, array, id, GRN_TABLE_ADD);
    if (!entry) {
      GRN_IO_ARRAY_BIT_OFF(array->io, array_seg_bitmap, id);
      return GRN_ID_NIL;
    }
    header->curr_rec = id;
  }
  (*array->n_entries)++;
  if (value) { *value = entry; }
  return id;
}

grn_id
grn_array_add(grn_ctx *ctx, grn_array *array, void **value)
{
  if (ctx && array) {
    if (grn_array_is_io_array(array)) {
      return grn_array_add_to_io_array(ctx, array, value);
    } else {
      return grn_array_add_to_tiny_array(ctx, array, value);
    }
  }
  return GRN_ID_NIL;
}

/* grn_hash : hash table */

#define GRN_HASH_MAX_SEGMENT  0x400
#define GRN_HASH_HEADER_SIZE  0x9000
#define GRN_HASH_SEGMENT_SIZE 0x400000
#define W_OF_KEY_IN_A_SEGMENT 22
#define IDX_MASK_IN_A_SEGMENT 0xfffff

typedef struct {
  uint32_t key;
  uint8_t dummy[1];
} entry;

typedef struct {
  uint32_t key;
  uint16_t flag;
  uint16_t size;
  uint32_t str;
  uint8_t dummy[1];
} entry_str;

typedef struct {
  uint32_t key;
  uint16_t flag;
  uint16_t size;
  char *str;
  uint8_t dummy[1];
} entry_astr;

#define LOGICAL_MAX_SEGMENT ((GRN_HASH_MAX_SEGMENT) * 4)

#define IO_HASHP(hash) ((hash)->io)

enum {
  segment_key = 0,
  segment_entry = 1,
  segment_index = 2,
  segment_bitmap = 3
};

inline static void *
grn_hash_io_entry_at(grn_ctx *ctx, grn_hash *hash, grn_id id, int flags)
{
  void *e;
  GRN_IO_ARRAY_AT(hash->io, segment_entry, id, &flags, e);
  return e;
}

/* todo : error handling */
inline static void *
grn_hash_entry_at(grn_ctx *ctx, grn_hash *hash, grn_id id, int flags)
{
  if (IO_HASHP(hash)) {
    return grn_hash_io_entry_at(ctx, hash, id, flags);
  } else {
    return grn_tiny_array_at_inline(&hash->a, id);
  }
}

inline static grn_bool
grn_hash_bitmap_at(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  if (IO_HASHP(hash)) {
    grn_bool value;
    GRN_IO_ARRAY_BIT_AT(hash->io, segment_bitmap, id, value);
    return value;
  } else {
    return grn_tiny_array_bit_at(&hash->bitmap, id);
  }
}

inline static grn_id *
grn_hash_io_idx_at(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  int flags = GRN_TABLE_ADD;
  void *pp;
  GRN_IO_ARRAY_AT(hash->io, segment_index, id, &flags, pp);
  return pp;
}

#define IDX_AT(h,i) \
  (IO_HASHP(h) ?\
   grn_hash_io_idx_at(ctx, h, ((i) & *(h)->max_offset) + h->header->idx_offset) :\
   h->index + ((i) & *(h)->max_offset))

#define KEY_AT(hash,pos,ptr) do {\
  int flags = GRN_TABLE_ADD;\
  GRN_IO_ARRAY_AT(hash->io, segment_key, pos, &flags, ptr);\
} while (0)

#define HASH_IMMEDIATE 1

#define MAX_INDEX_SIZE ((GRN_HASH_MAX_SEGMENT * (IDX_MASK_IN_A_SEGMENT + 1)) >> 1)

inline static char *
get_key(grn_ctx *ctx, grn_hash *hash, entry_str *n)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (n->flag & HASH_IMMEDIATE) {
      return (char *)&n->str;
    } else {
      if (IO_HASHP(hash)) {
        char *res;
        KEY_AT(hash, n->str, res);
        return res;
      } else {
        return ((entry_astr *)n)->str;
      }
    }
  } else {
    if (hash->key_size == sizeof(uint32_t)) {
      return (char *)(&((entry *)n)->key);
    } else {
      return (char *)(((entry *)n)->dummy);
    }
  }
}

inline static void *
get_value(grn_hash *hash, entry_str *n)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (IO_HASHP(hash)) {
      return ((entry_str *)n)->dummy;
    } else {
      return ((entry_astr *)n)->dummy;
    }
  } else {
    if (hash->key_size == sizeof(uint32_t)) {
      return ((entry *)n)->dummy;
    } else {
      return &((entry *)n)->dummy[hash->key_size];
    }
  }
}

inline static void
put_key_(grn_ctx *ctx, grn_hash *hash, entry_str *n, const char *key, int len)
{
  uint32_t res, ts;
  if (n->size) {
    res = n->str;
  } else {
    if (len >= GRN_HASH_SEGMENT_SIZE) { return; /* error */ }
    res = hash->header->curr_key;
    ts = (res + len) >> W_OF_KEY_IN_A_SEGMENT;
    if (res >> W_OF_KEY_IN_A_SEGMENT != ts) {
      res = hash->header->curr_key = ts << W_OF_KEY_IN_A_SEGMENT;
    }
    hash->header->curr_key += len;
    n->str = res;
  }
  {
    uint8_t *dest;
    KEY_AT(hash, res, dest);
    if (!dest) { return; }
    memcpy(dest, key, len);
  }
}

inline static grn_rc
put_key(grn_ctx *ctx, grn_hash *hash, entry_str *n, uint32_t h,
        const char *key, unsigned int len)
{
  n->key = h;
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (IO_HASHP(hash)) {
      if (len <= sizeof(uint32_t)) {
        n->flag = HASH_IMMEDIATE;
        memcpy(&n->str, key, len);
      } else {
        n->flag = 0;
        put_key_(ctx, hash, n, key, len);
      }
    } else {
      if (len <= sizeof(char *)) {
        n->flag = HASH_IMMEDIATE;
        memcpy(&((entry_astr *)n)->str, key, len);
      } else {
        grn_ctx *ctx = hash->ctx;
        if (!(((entry_astr *)n)->str = GRN_CTX_ALLOC(ctx, len))) {
          return GRN_NO_MEMORY_AVAILABLE;
        }
        memcpy(((entry_astr *)n)->str, key, len);
        n->flag = 0;
      }
    }
    n->size = len;
  } else {
    if (hash->key_size != sizeof(uint32_t)) {
      memcpy(((entry *)n)->dummy, key, len);
    }
  }
  return GRN_SUCCESS;
}

inline static int
match_key(grn_ctx *ctx, grn_hash *hash, entry_str *ee, uint32_t h,
          const char *key, unsigned int len)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    return (ee->key == h &&
            ee->size == len &&
            !memcmp(key, get_key(ctx, hash, ee), len));
  } else {
    return (ee->key == h &&
            ((len == sizeof(uint32_t)) ||
             !memcmp(key, get_key(ctx, hash, ee), len)));
  }
}

#define GARBAGE (0xffffffff)

#define STEP(x) (((x) >> 2) | 0x1010101)

inline static grn_rc
io_hash_init(grn_hash *ih, grn_ctx *ctx, const char *path, uint32_t key_size,
             uint32_t value_size, uint32_t flags, grn_encoding encoding,
             uint32_t init_size)
{
  grn_io *io;
  struct grn_hash_header *header;
  uint32_t entry_size, w_of_element, m;
  for (m = IDX_MASK_IN_A_SEGMENT + 1; m < init_size * 2; m *= 2);
  if (flags & GRN_OBJ_KEY_VAR_SIZE) {
    entry_size = (intptr_t)(&((entry_str *)0)->dummy) + value_size;
  } else {
    if (key_size == sizeof(uint32_t)) {
      entry_size = (intptr_t)(&((entry *)0)->dummy) + value_size;
    } else {
      entry_size = (intptr_t)(&((entry *)0)->dummy) + key_size + value_size;
    }
  }
  w_of_element = 0;
  while ((1U << w_of_element) < entry_size) {
    w_of_element++;
  }
  {
    grn_io_array_spec array_spec[4];
    array_spec[segment_key].w_of_element = 0;
    array_spec[segment_key].max_n_segments = 0x400;
    array_spec[segment_entry].w_of_element = w_of_element;
    array_spec[segment_entry].max_n_segments = 1U << (30 - (22 - w_of_element));
    array_spec[segment_index].w_of_element = 2;
    array_spec[segment_index].max_n_segments = 1U << (30 - (22 - 2));
    array_spec[segment_bitmap].w_of_element = 0;
    array_spec[segment_bitmap].max_n_segments = 1U << (30 - (22 + 3));
    io = grn_io_create_with_array(ctx, path, GRN_HASH_HEADER_SIZE, GRN_HASH_SEGMENT_SIZE,
                                  grn_io_auto, 4, array_spec);
  }
  if (!io) { return GRN_NO_MEMORY_AVAILABLE; }
  if (encoding == GRN_ENC_DEFAULT) { encoding = ctx->encoding; }
  header = grn_io_header(io);
  grn_io_set_type(io, GRN_TABLE_HASH_KEY);
  header->flags = flags;
  header->encoding = encoding;
  header->key_size = key_size;
  header->curr_rec = 0;
  header->curr_key = 0;
  header->lock = 0;
  header->idx_offset = 0;
  header->value_size = value_size;
  header->entry_size = entry_size;
  header->max_offset = m - 1;
  header->n_entries = 0;
  header->n_garbages = 0;
  header->tokenizer = GRN_ID_NIL;
  ih->obj.header.flags = flags;
  ih->ctx = ctx;
  ih->key_size = key_size;
  ih->encoding = encoding;
  ih->value_size = value_size;
  ih->entry_size = entry_size;
  ih->n_garbages = &header->n_garbages;
  ih->n_entries = &header->n_entries;
  ih->max_offset = &header->max_offset;
  ih->io = io;
  ih->header = header;
  ih->lock = &header->lock;
  ih->tokenizer = NULL;
  return GRN_SUCCESS;
}

#define INITIAL_INDEX_SIZE 256U

inline static grn_rc
tiny_hash_init(grn_hash *ah, grn_ctx *ctx, const char *path, uint32_t key_size,
               uint32_t value_size, uint32_t flags, grn_encoding encoding)
{
  uint32_t entry_size;
  if (path) { return GRN_INVALID_ARGUMENT; }
  if (!(ah->index = GRN_CTX_ALLOC(ctx, INITIAL_INDEX_SIZE * sizeof(grn_id)))) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  if (flags & GRN_OBJ_KEY_VAR_SIZE) {
    entry_size = (intptr_t)(&((entry_astr *)0)->dummy) + value_size;
  } else {
    if (key_size == sizeof(uint32_t)) {
      entry_size = (intptr_t)(&((entry *)0)->dummy) + value_size;
    } else {
      entry_size = (intptr_t)(&((entry *)0)->dummy) + key_size + value_size;
    }
  }
  if (entry_size != sizeof(uint32_t)) {
    entry_size = ((entry_size + (sizeof(intptr_t)) - 1) & ~((sizeof(intptr_t)) - 1));
  }
  ah->obj.header.flags = flags;
  ah->ctx = ctx;
  ah->key_size = key_size;
  ah->encoding = encoding;
  ah->value_size = value_size;
  ah->entry_size = entry_size;
  ah->n_garbages = &ah->n_garbages_;
  ah->n_entries = &ah->n_entries_;
  ah->max_offset = &ah->max_offset_;
  ah->max_offset_ = INITIAL_INDEX_SIZE - 1;
  ah->io = NULL;
  ah->n_garbages_ = 0;
  ah->n_entries_ = 0;
  ah->garbages = GRN_ID_NIL;
  ah->tokenizer = NULL;
  grn_tiny_array_init(ctx, &ah->a, entry_size, GRN_TINY_ARRAY_CLEAR);
  grn_tiny_array_init(ctx, &ah->bitmap, 1, GRN_TINY_ARRAY_CLEAR);
  return GRN_SUCCESS;
}

inline static grn_hash *
_grn_hash_create(grn_ctx *ctx, grn_hash *hash, const char *path,
                 uint32_t key_size, uint32_t value_size, uint32_t flags)
{
  grn_encoding encoding = ctx->encoding;
  if (!((flags & GRN_HASH_TINY) ?
        tiny_hash_init(hash, ctx, path, key_size, value_size, flags, encoding) :
        io_hash_init(hash, ctx, path, key_size, value_size, flags, encoding, 0))) {
    return hash;
  }
  return NULL;
}

grn_hash *
grn_hash_create(grn_ctx *ctx, const char *path, uint32_t key_size, uint32_t value_size,
                uint32_t flags)
{
  grn_hash *hash;
  if (key_size > GRN_HASH_MAX_KEY_SIZE) { return NULL; }
  if (!(hash = GRN_MALLOC(sizeof(grn_hash)))) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(hash, GRN_TABLE_HASH_KEY);
  if (!_grn_hash_create(ctx, hash, path, key_size, value_size, flags)) {
    GRN_FREE(hash);
    return NULL;
  }
  return hash;
}

grn_hash *
grn_hash_open(grn_ctx *ctx, const char *path)
{
  grn_io *io = grn_io_open(ctx, path, grn_io_auto);
  if (io) {
    struct grn_hash_header *header = grn_io_header(io);
    if (grn_io_get_type(io) == GRN_TABLE_HASH_KEY) {
      grn_hash *hash = GRN_MALLOC(sizeof(grn_hash));
      if (hash) {
        if (!(header->flags & GRN_HASH_TINY)) {
          GRN_DB_OBJ_SET_TYPE(hash, GRN_TABLE_HASH_KEY);
          hash->obj.header.flags = header->flags;
          hash->ctx = ctx;
          hash->key_size = header->key_size;
          hash->encoding = header->encoding;
          hash->value_size = header->value_size;
          hash->entry_size = header->entry_size;
          hash->n_garbages = &header->n_garbages;
          hash->n_entries = &header->n_entries;
          hash->max_offset = &header->max_offset;
          hash->io = io;
          hash->header = header;
          hash->lock = &header->lock;
          hash->tokenizer = grn_ctx_at(ctx, header->tokenizer);
          return (grn_hash *)hash;
        } else {
          GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid hash flag. (%x)", header->flags);
        }
        GRN_FREE(hash);
      }
    } else {
      ERR(GRN_INVALID_FORMAT, "file type unmatch");
    }
    grn_io_close(ctx, io);
  }
  return NULL;
}

inline static grn_rc
tiny_hash_fin(grn_ctx *ctx, grn_hash *hash)
{
  if (hash->index) {
    if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
      uint32_t i;
      grn_id e, *sp;
      for (i = *hash->n_entries, sp = hash->index; i; sp++) {
        entry_astr *n;
        e = *sp;
        if (!e || (e == GARBAGE)) { continue; }
        n = grn_tiny_array_at_inline(&hash->a, e);
        GRN_ASSERT(n);
        i--;
        if (!n || (n->flag & HASH_IMMEDIATE)) { continue; }
        GRN_CTX_FREE(ctx, n->str);
      }
    }
    grn_tiny_array_fin(&hash->a);
    grn_tiny_array_fin(&hash->bitmap);
    GRN_CTX_FREE(ctx, hash->index);
    return GRN_SUCCESS;
  } else {
    return GRN_INVALID_ARGUMENT;
  }
}

grn_rc
grn_hash_close(grn_ctx *ctx, grn_hash *hash)
{
  grn_rc rc;
  if (!hash) { return GRN_INVALID_ARGUMENT; }
  if (IO_HASHP(hash)) {
    rc = grn_io_close(ctx, hash->io);
  } else {
    GRN_ASSERT(ctx == hash->ctx);
    rc = tiny_hash_fin(ctx, hash);
  }
  GRN_FREE(hash);
  return rc;
}

grn_rc
grn_hash_remove(grn_ctx *ctx, const char *path)
{
  if (!path) { return GRN_INVALID_ARGUMENT; }
  return grn_io_remove(ctx, path);
}

grn_rc
grn_hash_truncate(grn_ctx *ctx, grn_hash *hash)
{
  grn_rc rc;
  char *path;
  uint32_t key_size, value_size, flags;

  if (IO_HASHP(hash) &&
      (path = (char *)grn_io_path(hash->io)) && *path != '\0') {
    if (!(path = GRN_STRDUP(path))) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path.");
      return GRN_NO_MEMORY_AVAILABLE;
    }
  } else {
    path = NULL;
  }
  key_size = hash->key_size;
  value_size = hash->value_size;
  flags = hash->obj.header.flags;

  if (IO_HASHP(hash)) {
    if ((rc = grn_io_close(ctx, hash->io))) { goto exit; }
    hash->io = NULL;
    if (path && (rc = grn_io_remove(ctx, path))) { goto exit; }
  } else {
    rc = GRN_SUCCESS;
  }
  if (!_grn_hash_create(ctx, hash, path, key_size, value_size, flags)) {
    rc = GRN_UNKNOWN_ERROR;
  }
exit:
  if (path) { GRN_FREE(path); }
  return rc;
}

grn_rc
grn_hash_reset(grn_ctx *ctx, grn_hash *hash, uint32_t ne)
{
  entry *ee;
  grn_id e, *index = NULL, *sp = NULL, *dp;
  uint32_t n, n0 = *hash->n_entries, offs = 0, offd = 0;
  if (!ne) { ne = n0 * 2; }
  if (ne > INT_MAX) { return GRN_NO_MEMORY_AVAILABLE; }
  for (n = INITIAL_INDEX_SIZE; n <= ne; n *= 2);
  if (IO_HASHP(hash)) {
    uint32_t i;
    offs = hash->header->idx_offset;
    offd = MAX_INDEX_SIZE - offs;
    for (i = 0; i < n; i += (IDX_MASK_IN_A_SEGMENT + 1)) {
      dp = grn_hash_io_idx_at(ctx, hash, i + offd); // todo : use idx_at
      if (!dp) { return GRN_NO_MEMORY_AVAILABLE; }
      memset(dp, 0, GRN_HASH_SEGMENT_SIZE);
    }
  } else {
    GRN_ASSERT(ctx == hash->ctx);
    if (!(index = GRN_CTX_ALLOC(ctx, n * sizeof(grn_id)))) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    sp = hash->index;
  }
  {
    uint32_t i, j, k, m0 = *hash->max_offset, m = n - 1, s;
    for (k = 0, j = 0; k < n0 && j <= m0; j++, sp++) {
      if (IO_HASHP(hash) && !(j & IDX_MASK_IN_A_SEGMENT)) {
        sp = grn_hash_io_idx_at(ctx, hash, j + offs);
        if (!sp) { return GRN_NO_MEMORY_AVAILABLE; }
      }
      e = *sp;
      if (!e || (e == GARBAGE)) { continue; }
      ee = grn_hash_entry_at(ctx, hash, e, GRN_TABLE_ADD);
      if (!ee) { return GRN_NO_MEMORY_AVAILABLE; }
      for (i = ee->key, s = STEP(i); ; i += s) {
        if (IO_HASHP(hash)) {
          dp = grn_hash_io_idx_at(ctx, hash, (i & m) + offd);
          if (!dp) { return GRN_NO_MEMORY_AVAILABLE; }
        } else {
          dp = index + (i & m);
        }
        if (!*dp) { break; }
      }
      *dp = e;
      k++;
    }
    *hash->max_offset = m;
    *hash->n_garbages = 0;
  }
  if (IO_HASHP(hash)) {
    hash->header->idx_offset = offd;
  } else {
    grn_id *i0 = hash->index;
    hash->index = index;
    GRN_CTX_FREE(ctx, i0);
  }
  return GRN_SUCCESS;
}

inline static uint32_t
bin_hash(const uint8_t *p, uint32_t length)
{
  uint32_t r;
  for (r = 0; length--; p++) { r = (r * 1021) + *p; }
  return r;
}

inline static grn_id
entry_new(grn_ctx *ctx, grn_hash *hash, uint32_t size)
{
  grn_id e;
  if (IO_HASHP(hash)) {
    struct grn_hash_header *hh = hash->header;
    size -= 1;
    if ((e = hh->garbages[size])) {
      entry * const ee = grn_hash_io_entry_at(ctx, hash, e, GRN_TABLE_ADD);
      if (!ee) { return GRN_ID_NIL; }
      hh->garbages[size] = ee->key;
      if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
        /* keep ee->size && ee->str */
        memset(((entry_str *)ee)->dummy, 0, hh->value_size);
      } else {
        memset(ee, 0, hh->entry_size);
      }
    } else {
      e = ++hh->curr_rec;
    }
    GRN_IO_ARRAY_BIT_ON(hash->io, segment_bitmap, e);
  } else {
    if (hash->garbages) {
      entry *ee;
      e = hash->garbages;
      ee = grn_tiny_array_at_inline(&hash->a, e);
      hash->garbages = ee->key;
      memset(ee, 0, hash->entry_size);
    } else {
      e = hash->a.max + 1;
    }
    grn_tiny_array_bit_on(&hash->bitmap, e);
  }
  return e;
}

grn_rc
grn_hash_lock(grn_ctx *ctx, grn_hash *hash, int timeout)
{
  static int _ncalls = 0, _ncolls = 0;
  uint32_t count;
  _ncalls++;
  for (count = 0;; count++) {
    uint32_t lock;
    GRN_ATOMIC_ADD_EX(hash->lock, 1, lock);
    if (lock) {
      GRN_ATOMIC_ADD_EX(hash->lock, -1, lock);
      if (!timeout || (timeout > 0 && timeout == count)) { break; }
      if (!(++_ncolls % 1000000) && (_ncolls > _ncalls)) {
        if (_ncolls < 0 || _ncalls < 0) {
          _ncolls = 0; _ncalls = 0;
        } else {
          GRN_LOG(ctx, GRN_LOG_NOTICE, "hash(%p) collisions(%d/%d)", hash, _ncolls, _ncalls);
        }
      }
      usleep(1000);
      continue;
    }
    return GRN_SUCCESS;
  }
  ERR(GRN_RESOURCE_DEADLOCK_AVOIDED, "grn_hash_lock");
  return ctx->rc;
}

grn_rc
grn_hash_unlock(grn_ctx *ctx, grn_hash *hash)
{
  uint32_t lock;
  GRN_ATOMIC_ADD_EX(hash->lock, -1, lock);
  return GRN_SUCCESS;
}

grn_rc
grn_hash_clear_lock(grn_ctx *ctx, grn_hash *hash)
{
  *hash->lock = 0;
  return GRN_SUCCESS;
}

grn_id
grn_hash_add(grn_ctx *ctx, grn_hash *hash, const void *key,
             unsigned int key_size, void **value, int *added)
{
  entry_str *ee;
  uint32_t h, i, m, s;
  grn_id e, *ep, *np = NULL;
  if (!key || !key_size) { return GRN_ID_NIL; }
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (key_size > hash->key_size) {
      ERR(GRN_INVALID_ARGUMENT, "too long key");
      return GRN_ID_NIL;
    }
    h = bin_hash((unsigned char *)key, key_size);
  } else {
    if (key_size != hash->key_size) {
      ERR(GRN_INVALID_ARGUMENT, "key size unmatch");
      return GRN_ID_NIL;
    }
    if (key_size == sizeof(uint32_t)) {
      h = *((uint32_t *)key);
    } else {
      h = bin_hash((unsigned char *)key, key_size);
    }
  }
  s = STEP(h);
  /* lock */
  if ((*hash->n_entries + *hash->n_garbages) * 2 > *hash->max_offset) {
    grn_hash_reset(ctx, hash, 0);
  }
  m = *hash->max_offset;
  for (i = h; ; i += s) {
    if (!(ep = IDX_AT(hash, i))) { return GRN_ID_NIL; }
    if (!(e = *ep)) { break; }
    if (e == GARBAGE) {
      if (!np) { np = ep; }
      continue;
    }
    ee = grn_hash_entry_at(ctx, hash, e, GRN_TABLE_ADD);
    if (!ee) { return GRN_ID_NIL; }
    if (match_key(ctx, hash, ee, h, key, key_size)) {
      if (added) { *added = 0; }
      goto exit;
    }
  }
  if (!(e = entry_new(ctx, hash, key_size))) { /* unlock */ return GRN_ID_NIL; }
  ee = grn_hash_entry_at(ctx, hash, e, GRN_TABLE_ADD);
  if (!ee) { return GRN_ID_NIL; }
  put_key(ctx, hash, ee, h, key, key_size);
  if (np) {
    (*hash->n_garbages)--;
    ep = np;
  }
  *ep = e;
  (*hash->n_entries)++;
  if (added) { *added = 1; }
exit :
  /* unlock */
  if (value) { *value = get_value(hash,ee); }
  return e;
}

grn_id
grn_hash_get(grn_ctx *ctx, grn_hash *hash, const void *key,
             unsigned int key_size, void **value)
{
  grn_id e, *ep;
  uint32_t h, i, m, s;
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (key_size > hash->key_size) { return GRN_ID_NIL; }
    h = bin_hash((unsigned char *)key, key_size);
  } else {
    if (key_size != hash->key_size) { return GRN_ID_NIL; }
    if (key_size == sizeof(uint32_t)) {
      h = *((uint32_t *)key);
    } else {
      h = bin_hash((unsigned char *)key, key_size);
    }
  }
  s = STEP(h);
  m = *hash->max_offset;
  for (i = h; ; i += s) {
    if (!(ep = IDX_AT(hash, i))) { return GRN_ID_NIL; }
    if (!(e = *ep)) { break; }
    if (e == GARBAGE) { continue; }
    {
      entry_str * const ee = grn_hash_entry_at(ctx, hash, e, 0);
      if (ee && match_key(ctx, hash, ee, h, key, key_size)) {
        if (value) { *value = get_value(hash, ee); }
        return e;
      }
    }
  }
  return GRN_ID_NIL;
}

const char *
_grn_hash_key(grn_ctx *ctx, grn_hash *hash, grn_id id, uint32_t *key_size)
{
  entry_str *ee;
  if (!grn_hash_bitmap_at(ctx, hash, id)) {
    *key_size = 0;
    return NULL;
  }
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (!ee) {
    *key_size = 0;
    return NULL;
  }
  *key_size = (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : hash->key_size;
  return get_key(ctx, hash, ee);
}

int
grn_hash_get_key(grn_ctx *ctx, grn_hash *hash, grn_id id, void *keybuf, int bufsize)
{
  int key_size;
  entry_str *ee;
  if (!grn_hash_bitmap_at(ctx, hash, id)) { return 0; }
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (!ee) { return 0; }
  key_size = (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : hash->key_size;
  if (bufsize >= key_size) { memcpy(keybuf, get_key(ctx, hash, ee), key_size); }
  return key_size;
}

int
grn_hash_get_key2(grn_ctx *ctx, grn_hash *hash, grn_id id, grn_obj *bulk)
{
  int key_size;
  char *key;
  entry_str *ee;
  if (!grn_hash_bitmap_at(ctx, hash, id)) { return 0; }
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (!ee) { return 0; }
  key_size = (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : hash->key_size;
  key = get_key(ctx, hash, ee);
  if (bulk->header.impl_flags & GRN_OBJ_REFER) {
    bulk->u.b.head = key;
    bulk->u.b.curr = key + key_size;
  } else {
    grn_bulk_write(ctx, bulk, key, key_size);
  }
  return key_size;
}

int
grn_hash_get_value(grn_ctx *ctx, grn_hash *hash, grn_id id, void *valuebuf)
{
  void *v;
  entry_str *ee;
  if (!grn_hash_bitmap_at(ctx, hash, id)) { return 0; }
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (ee && (v = get_value(hash, ee))) {
    if (valuebuf) { memcpy(valuebuf, v, hash->value_size); }
    return hash->value_size;
  }
  return 0;
}

const char *
grn_hash_get_value_(grn_ctx *ctx, grn_hash *hash, grn_id id, uint32_t *size)
{
  entry_str *ee;
  const char *value = NULL;
  if (!grn_hash_bitmap_at(ctx, hash, id)) { return NULL; }
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (ee && (value = get_value(hash, ee))) {
    *size = hash->value_size;
  }
  return value;
}

int
grn_hash_get_key_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                       void *keybuf, int bufsize, void *valuebuf)
{
  void *v;
  int key_size;
  entry_str *ee;
  if (!grn_hash_bitmap_at(ctx, hash, id)) { return 0; }
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (!ee) { return 0; }
  key_size = (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : hash->key_size;
  if (bufsize >= key_size) { memcpy(keybuf, get_key(ctx, hash, ee), key_size); }
  if (valuebuf && (v = get_value(hash, ee))) {
    memcpy(valuebuf, v, hash->value_size);
  }
  return key_size;
}

int
_grn_hash_get_key_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                        void **key, void **value)
{
  int key_size;
  entry_str *ee;
  if (!grn_hash_bitmap_at(ctx, hash, id)) { return 0; }
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (!ee) { return 0; }
  key_size = (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : hash->key_size;
  *key = get_key(ctx, hash, ee);
  return (*value = get_value(hash, ee)) ? key_size : 0;
}

grn_rc
grn_hash_set_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                   const void *value, int flags)
{
  if (value) {
    void *v;
    entry_str *ee;
    if (!grn_hash_bitmap_at(ctx, hash, id)) { return GRN_INVALID_ARGUMENT; }
    ee = grn_hash_entry_at(ctx, hash, id, 0);
    if (ee && (v = get_value(hash, ee))) {
      switch ((flags & GRN_OBJ_SET_MASK)) {
      case GRN_OBJ_SET :
        memcpy(v, value, hash->value_size);
        return GRN_SUCCESS;
      case GRN_OBJ_INCR :
        switch (hash->value_size) {
        case sizeof(int32_t) :
          *((int32_t *)v) += *((int32_t *)value);
          return GRN_SUCCESS;
        case sizeof(int64_t) :
          *((int64_t *)v) += *((int64_t *)value);
          return GRN_SUCCESS;
        default :
          return GRN_INVALID_ARGUMENT;
        }
        break;
      case GRN_OBJ_DECR :
        switch (hash->value_size) {
        case sizeof(int32_t) :
          *((int32_t *)v) -= *((int32_t *)value);
          return GRN_SUCCESS;
        case sizeof(int64_t) :
          *((int64_t *)v) -= *((int64_t *)value);
          return GRN_SUCCESS;
        default :
          return GRN_INVALID_ARGUMENT;
        }
        break;
      default :
        ERR(GRN_INVALID_ARGUMENT, "flags = %d", flags);
        return ctx->rc;
      }
    } else {
      return GRN_NO_MEMORY_AVAILABLE;
    }
  }
  return GRN_INVALID_ARGUMENT;
}

#define DELETE_IT do {\
  *ep = GARBAGE;\
  if (IO_HASHP(hash)) {\
    uint32_t size = key_size - 1;\
    struct grn_hash_header *hh = hash->header;\
    ee->key = hh->garbages[size];\
    hh->garbages[size] = e;\
    GRN_IO_ARRAY_BIT_OFF(hash->io, segment_bitmap, e);\
  } else {\
    ee->key = hash->garbages;\
    hash->garbages = e;\
    if ((hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) && !(ee->flag & HASH_IMMEDIATE)) {\
      grn_ctx *ctx = hash->ctx;\
      GRN_CTX_FREE(ctx, ((entry_astr *)ee)->str);\
    }\
    grn_tiny_array_bit_off(&hash->bitmap, e);\
  }\
  (*hash->n_entries)--;\
  (*hash->n_garbages)++;\
  rc = GRN_SUCCESS;\
} while (0)

grn_rc
grn_hash_delete_by_id(grn_ctx *ctx, grn_hash *hash, grn_id id,
                      grn_table_delete_optarg *optarg)
{
  entry_str *ee;
  grn_rc rc = GRN_INVALID_ARGUMENT;
  if (!hash || !id) { return rc; }
  /* lock */
  ee = grn_hash_entry_at(ctx, hash, id, 0);
  if (ee) {
    grn_id e, *ep;
    uint32_t i, key_size, h = ee->key, s = STEP(h);
    key_size = (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : hash->key_size;
    for (i = h; ; i += s) {
      if (!(ep = IDX_AT(hash, i))) { return GRN_NO_MEMORY_AVAILABLE; }
      if (!(e = *ep)) { break; }
      if (e == id) {
        DELETE_IT;
        break;
      }
    }
  }
  /* unlock */
  return rc;
}

grn_rc
grn_hash_delete(grn_ctx *ctx, grn_hash *hash, const void *key, uint32_t key_size,
                grn_table_delete_optarg *optarg)
{
  uint32_t h, i, m, s;
  grn_rc rc = GRN_INVALID_ARGUMENT;
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (key_size > hash->key_size) { return GRN_INVALID_ARGUMENT; }
    h = bin_hash((unsigned char *)key, key_size);
  } else {
    if (key_size != hash->key_size) { return GRN_INVALID_ARGUMENT; }
    if (key_size == sizeof(uint32_t)) {
      h = *((uint32_t *)key);
    } else {
      h = bin_hash((unsigned char *)key, key_size);
    }
  }
  s = STEP(h);
  {
    grn_id e, *ep;
    /* lock */
    m = *hash->max_offset;
    for (i = h; ; i += s) {
      if (!(ep = IDX_AT(hash, i))) { return GRN_NO_MEMORY_AVAILABLE; }
      if (!(e = *ep)) { break; }
      if (e == GARBAGE) { continue; }
      {
        entry_str * const ee = grn_hash_entry_at(ctx, hash, e, 0);
        if (ee && match_key(ctx, hash, ee, h, key, key_size)) {
          DELETE_IT;
          break;
        }
      }
    }
    /* unlock */
    return rc;
  }
}

/* only valid for hash tables, GRN_OBJ_KEY_VAR_SIZE && GRN_HASH_TINY */
const char *
_grn_hash_strkey_by_val(void *v, uint16_t *size)
{
  entry_astr *n = (entry_astr *)((uintptr_t)v -
                                 (uintptr_t)&((entry_astr *)0)->dummy);
  *size = n->size;
  return (n->flag & HASH_IMMEDIATE) ? (char *)&n->str : n->str;
}

void
grn_hash_cursor_close(grn_ctx *ctx, grn_hash_cursor *c)
{
  GRN_ASSERT(c->ctx == ctx);
  GRN_FREE(c);
}

#define HASH_CURR_MAX(hash) ((IO_HASHP(hash)) ? (hash)->header->curr_rec : (hash)->a.max)

grn_hash_cursor *
grn_hash_cursor_open(grn_ctx *ctx, grn_hash *hash,
                     const void *min, uint32_t min_size,
                     const void *max, uint32_t max_size,
                     int offset, int limit, int flags)
{
  grn_hash_cursor *c;
  if (!hash || !ctx) { return NULL; }
  if (!(c = GRN_MALLOCN(grn_hash_cursor, 1))) { return NULL; }
  GRN_DB_OBJ_SET_TYPE(c, GRN_CURSOR_TABLE_HASH_KEY);
  c->hash = hash;
  c->ctx = ctx;
  c->obj.header.flags = flags;
  c->obj.header.domain = GRN_ID_NIL;
  if (flags & GRN_CURSOR_DESCENDING) {
    c->dir = -1;
    if (max) {
      if (!(c->curr_rec = grn_hash_get(ctx, hash, max, max_size, NULL))) {
        c->tail = GRN_ID_NIL;
        goto exit;
      }
      if (!(flags & GRN_CURSOR_LT)) { c->curr_rec++; }
    } else {
      c->curr_rec = HASH_CURR_MAX(hash) + 1;
    }
    if (min) {
      if (!(c->tail = grn_hash_get(ctx, hash, min, min_size, NULL))) {
        c->curr_rec = GRN_ID_NIL;
        goto exit;
      }
      if ((flags & GRN_CURSOR_GT)) { c->tail++; }
    } else {
      c->tail = GRN_ID_NIL + 1;
    }
    if (c->curr_rec < c->tail) { c->tail = c->curr_rec; }
  } else {
    c->dir = 1;
    if (min) {
      if (!(c->curr_rec = grn_hash_get(ctx, hash, min, min_size, NULL))) {
        c->tail = GRN_ID_NIL;
        goto exit;
      }
      if (!(flags & GRN_CURSOR_GT)) { c->curr_rec--; }
    } else {
      c->curr_rec = GRN_ID_NIL;
    }
    if (max) {
      if (!(c->tail = grn_hash_get(ctx, hash, max, max_size, NULL))) {
        c->curr_rec = GRN_ID_NIL;
        goto exit;
      }
      if ((flags & GRN_CURSOR_LT)) { c->tail--; }
    } else {
      c->tail = HASH_CURR_MAX(hash);
    }
    if (c->tail < c->curr_rec) { c->tail = c->curr_rec; }
  }
  if (*hash->n_entries != HASH_CURR_MAX(hash)) {
    while (offset && c->curr_rec != c->tail) {
      c->curr_rec += c->dir;
      if (grn_hash_bitmap_at(ctx, c->hash, c->curr_rec)) { offset--; }
    }
  } else {
    c->curr_rec += c->dir * offset;
  }
exit :
  c->rest = (limit < 0) ? GRN_ID_MAX : limit;
  return c;
}

grn_id
grn_hash_cursor_next(grn_ctx *ctx, grn_hash_cursor *c)
{
  if (c && c->rest) {
    while (c->curr_rec != c->tail) {
      c->curr_rec += c->dir;
      if (*c->hash->n_entries != HASH_CURR_MAX(c->hash)) {
        if (!grn_hash_bitmap_at(ctx, c->hash, c->curr_rec)) { continue; }
      }
      c->rest--;
      return c->curr_rec;
    }
  }
  return GRN_ID_NIL;
}

grn_id
grn_hash_next(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  grn_id max = HASH_CURR_MAX(hash);
  while (++id <= max) {
    if (grn_hash_bitmap_at(ctx, hash, id)) { return id; }
  }
  return GRN_ID_NIL;
}

grn_id
grn_hash_at(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  return grn_hash_bitmap_at(ctx, hash, id) ? id : GRN_ID_NIL;
}

int
grn_hash_cursor_get_key(grn_ctx *ctx, grn_hash_cursor *c, void **key)
{
  int key_size;
  entry_str *ee;
  if (!c) { return 0; }
  ee = grn_hash_entry_at(ctx, c->hash, c->curr_rec, 0);
  if (!ee) { return 0; }
  key_size = (c->hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : c->hash->key_size;
  *key = get_key(ctx, c->hash, ee);
  return key_size;
}

int
grn_hash_cursor_get_value(grn_ctx *ctx, grn_hash_cursor *c, void **value)
{
  void *v;
  entry_str *ee;
  if (!c) { return 0; }
  ee = grn_hash_entry_at(ctx, c->hash, c->curr_rec, 0);
  if (ee && (v = get_value(c->hash, ee))) {
    *value = v;
    return c->hash->value_size;
  }
  return 0;
}

int
grn_hash_cursor_get_key_value(grn_ctx *ctx, grn_hash_cursor *c,
                              void **key, uint32_t *key_size, void **value)
{
  entry_str *ee;
  if (!c) { return GRN_INVALID_ARGUMENT; }
  ee = grn_hash_entry_at(ctx, c->hash, c->curr_rec, 0);
  if (!ee) { return GRN_INVALID_ARGUMENT; }
  if (key_size) {
    *key_size = (c->hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) ? ee->size : c->hash->key_size;
  }
  if (key) { *key = get_key(ctx, c->hash, ee); }
  if (value) { *value = get_value(c->hash, ee); }
  return c->hash->value_size;
}

grn_rc
grn_hash_cursor_set_value(grn_ctx *ctx, grn_hash_cursor *c,
                          const void *value, int flags)
{
  if (!c) { return GRN_INVALID_ARGUMENT; }
  return grn_hash_set_value(ctx, c->hash, c->curr_rec, value, flags);
}

grn_rc
grn_hash_cursor_delete(grn_ctx *ctx, grn_hash_cursor *c,
                       grn_table_delete_optarg *optarg)
{
  if (!c) { return GRN_INVALID_ARGUMENT; }
  return grn_hash_delete_by_id(ctx, c->hash, c->curr_rec, optarg);
}

/* sort */

#define PREPARE_VAL(e,ep,es) do {\
  if ((arg->flags & GRN_TABLE_SORT_BY_VALUE)) {\
    ep = ((const uint8_t *)(get_value(hash, (entry_str *)(e))));\
    es = hash->value_size;\
  } else {\
    ep = ((const uint8_t *)(get_key(ctx, hash, (entry_str *)(e))));\
    es = ((hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE)\
          ? ((entry_str *)(e))->size : hash->key_size); \
  }\
  ep += arg->offset;\
  es -= arg->offset;\
} while (0)

#define COMPARE_VAL_(ap,as,bp,bs)\
  (arg->compar\
   ? arg->compar(ctx,\
                 (grn_obj *)hash, (void *)(ap), as,\
                 (grn_obj *)hash, (void *)(bp), bs, arg->compar_arg)\
   : ((arg->flags & GRN_TABLE_SORT_AS_NUMBER)\
      ? ((arg->flags & GRN_TABLE_SORT_AS_UNSIGNED)\
         ? ((arg->flags & GRN_TABLE_SORT_AS_INT64)\
            ? *((uint64_t *)(ap)) > *((uint64_t *)(bp))\
            : *((uint32_t *)(ap)) > *((uint32_t *)(bp)))\
         : ((arg->flags & GRN_TABLE_SORT_AS_INT64)\
            ? *((int64_t *)(ap)) > *((int64_t *)(bp))\
            : *((int32_t *)(ap)) > *((int32_t *)(bp))))\
      : grn_str_greater(ap, as, bp, bs)))

#define COMPARE_VAL(ap,as,bp,bs)\
  ((dir) ? COMPARE_VAL_((bp),(bs),(ap),(as)) : COMPARE_VAL_((ap),(as),(bp),(bs)))

inline static entry **
pack(grn_ctx *ctx, grn_hash *hash, entry **res, grn_table_sort_optarg *arg, int dir)
{
  uint32_t n;
  uint32_t cs, es;
  const uint8_t *cp, *ep;
  entry **head, **tail, *e, *c;
  grn_id id, m = HASH_CURR_MAX(hash);
  for (id = m >> 1;;id = (id == m) ? 1 : id + 1) {
    if (grn_hash_bitmap_at(ctx, hash, id)) { break; }
  }
  c = grn_hash_entry_at(ctx, hash, id, 0);
  if (!c) { return NULL; }
  PREPARE_VAL(c, cp, cs);
  head = res;
  n = *hash->n_entries - 1;
  tail = res + n;
  while (n--) {
    do {
      id = (id == m) ? 1 : id + 1;
    } while (!grn_hash_bitmap_at(ctx, hash, id));
    e = grn_hash_entry_at(ctx, hash, id, 0);
    if (!e) { return NULL; }
    PREPARE_VAL(e, ep, es);
    if (COMPARE_VAL(cp, cs, ep, es)) {
      *head++ = e;
    } else {
      *tail-- = e;
    }
  }
  *head = c;
  return *hash->n_entries > 2 ? head : NULL;
}

inline static void
swap(entry **a, entry **b)
{
  entry *c_ = *a;
  *a = *b;
  *b = c_;
}

#define SWAP(a,ap,as,b,bp,bs) do {\
  const uint8_t *cp_ = ap;\
  uint32_t cs_ = as;\
  ap = bp; bp = cp_;\
  as = bs; bs = cs_;\
  swap(a,b);\
} while (0)

inline static entry **
part(grn_ctx *ctx, entry **b, entry **e, grn_table_sort_optarg *arg, grn_hash *hash, int dir)
{
  entry **c;
  const uint8_t *bp, *cp, *ep;
  uint32_t bs, cs, es;
  intptr_t d = e - b;
  PREPARE_VAL(*b, bp, bs);
  PREPARE_VAL(*e, ep, es);
  if (COMPARE_VAL(bp, bs, ep, es)) {
    SWAP(b, bp, bs, e, ep, es);
  }
  if (d < 2) { return NULL; }
  c = b + (d >> 1);
  PREPARE_VAL(*c, cp, cs);
  if (COMPARE_VAL(bp, bs, cp, cs)) {
    SWAP(b, bp, bs, c, cp, cs);
  } else {
    if (COMPARE_VAL(cp, cs, ep, es)) {
      SWAP(c, cp, cs, e, ep, es);
    }
  }
  if (d < 3) { return NULL; }
  b++;
  swap(b, c);
  c = b;
  PREPARE_VAL(*c, cp, cs);
  for (;;) {
    do {
      b++;
      PREPARE_VAL(*b, bp, bs);
    } while (COMPARE_VAL(cp, cs, bp, bs));
    do {
      e--;
      PREPARE_VAL(*e, ep, es);
    } while (COMPARE_VAL(ep, es, cp, cs));
    if (b >= e) { break; }
    SWAP(b, bp, bs, e, ep, es);
  }
  SWAP(c, cp, cs, e, ep, es);
  return e;
}

static void
_sort(grn_ctx *ctx, entry **head, entry **tail, int limit,
      grn_table_sort_optarg *arg, grn_hash *hash, int dir)
{
  entry **c;
  if (head < tail && (c = part(ctx, head, tail, arg, hash, dir))) {
    intptr_t rest = limit - 1 - (c - head);
    _sort(ctx, head, c - 1, limit, arg, hash, dir);
    if (rest > 0) { _sort(ctx, c + 1, tail, (int)rest, arg, hash, dir); }
  }
}

static void
sort(grn_ctx *ctx,
     grn_hash *hash, entry **res, int limit, grn_table_sort_optarg *arg, int dir)
{
  entry **c = pack(ctx, hash, res, arg, dir);
  if (c) {
    intptr_t rest = limit - 1 - (c - res);
    _sort(ctx, res, c - 1, limit, arg, hash, dir);
    if (rest > 0 ) {
      _sort(ctx, c + 1, res + *hash->n_entries - 1, (int)rest, arg, hash, dir);
    }
  }
}

typedef struct {
  grn_id id;
  int32_t v;
} val32;

#define PREPARE_VAL32(id,e,ep) do {\
  (ep)->id = id;\
  (ep)->v = (arg->flags & GRN_TABLE_SORT_BY_ID)\
    ? (int32_t) id\
    : (*((int32_t *)((byte *)((arg->flags & GRN_TABLE_SORT_BY_VALUE)\
                              ? get_value(hash, (e))\
                              : get_key(ctx, hash, (e))) + arg->offset)));\
} while (0)

#define COMPARE_VAL32_(ap,bp) \
  (arg->compar\
   ? arg->compar(ctx,\
                 (grn_obj *)hash, (void *)&(ap)->v, sizeof(uint32_t),\
                 (grn_obj *)hash, (void *)&(bp)->v, sizeof(uint32_t),\
                 arg->compar_arg)\
   : ((arg->flags & GRN_TABLE_SORT_AS_NUMBER)\
      ? ((arg->flags & GRN_TABLE_SORT_AS_UNSIGNED)\
         ? *((uint32_t *)&(ap)->v) > *((uint32_t *)&(bp)->v)\
         : *((int32_t *)&(ap)->v) > *((int32_t *)&(bp)->v))\
      : memcmp(&(ap)->v, &(bp)->v, sizeof(uint32_t)) > 0))

#define COMPARE_VAL32(ap,bp)\
  ((dir) ? COMPARE_VAL32_((bp),(ap)) : COMPARE_VAL32_((ap),(bp)))

inline static val32 *
pack_val32(grn_ctx *ctx, grn_hash *hash, val32 *res, grn_table_sort_optarg *arg, int dir)
{
  uint32_t n;
  entry_str *e, *c;
  val32 *head, *tail, cr, er;
  grn_id id, m = HASH_CURR_MAX(hash);
  for (id = m >> 1;;id = (id == m) ? 1 : id + 1) {
    if (grn_hash_bitmap_at(ctx, hash, id)) { break; }
  }
  c = grn_hash_entry_at(ctx, hash, id, 0);
  if (!c) { return NULL; }
  PREPARE_VAL32(id, c, &cr);
  head = res;
  n = *hash->n_entries - 1;
  tail = res + n;
  while (n--) {
    do {
      id = (id == m) ? 1 : id + 1;
    } while (!grn_hash_bitmap_at(ctx, hash, id));
    e = grn_hash_entry_at(ctx, hash, id, 0);
    if (!e) { return NULL; }
    PREPARE_VAL32(id, e, &er);
    if (COMPARE_VAL32(&cr, &er)) {
      *head++ = er;
    } else {
      *tail-- = er;
    }
  }
  *head = cr;
  return *hash->n_entries > 2 ? head : NULL;
}

#define SWAP_VAL32(ap,bp) do {\
  val32 cr_ = *ap;\
  *ap = *bp;\
  *bp = cr_;\
} while (0)

inline static val32 *
part_val32(grn_ctx *ctx,
           val32 *b, val32 *e, grn_table_sort_optarg *arg, grn_hash *hash, int dir)
{
  val32 *c;
  intptr_t d = e - b;
  if (COMPARE_VAL32(b, e)) { SWAP_VAL32(b, e); }
  if (d < 2) { return NULL; }
  c = b + (d >> 1);
  if (COMPARE_VAL32(b, c)) {
    SWAP_VAL32(b, c);
  } else {
    if (COMPARE_VAL32(c, e)) { SWAP_VAL32(c, e); }
  }
  if (d < 3) { return NULL; }
  b++;
  SWAP_VAL32(b, c);
  c = b;
  for (;;) {
    do { b++; } while (COMPARE_VAL32(c, b));
    do { e--; } while (COMPARE_VAL32(e, c));
    if (b >= e) { break; }
    SWAP_VAL32(b, e);
  }
  SWAP_VAL32(c, e);
  return e;
}

static void
_sort_val32(grn_ctx *ctx, val32 *head, val32 *tail, int limit,
      grn_table_sort_optarg *arg, grn_hash *hash, int dir)
{
  val32 *c;
  if (head < tail && (c = part_val32(ctx, head, tail, arg, hash, dir))) {
    intptr_t rest = limit - 1 - (c - head);
    _sort_val32(ctx, head, c - 1, limit, arg, hash, dir);
    if (rest > 0) { _sort_val32(ctx, c + 1, tail, (int)rest, arg, hash, dir); }
  }
}

static void
sort_val32(grn_ctx *ctx,
           grn_hash *hash, val32 *res, int limit, grn_table_sort_optarg *arg, int dir)
{
  val32 *c = pack_val32(ctx, hash, res, arg, dir);
  if (c) {
    intptr_t rest = limit - 1 - (c - res);
    _sort_val32(ctx, res, c - 1, limit, arg, hash, dir);
    if (rest > 0 ) {
      _sort_val32(ctx, c + 1, res + *hash->n_entries - 1, (int)rest, arg, hash, dir);
    }
  }
}

inline static grn_id
entry2id(grn_ctx *ctx, grn_hash *hash, entry *e)
{
  entry *e2;
  grn_id id, *ep;
  uint32_t i, h = e->key, s = STEP(h);
  for (i = h; ; i += s) {
    if (!(ep = IDX_AT(hash, i))) { return GRN_ID_NIL; }
    if (!(id = *ep)) { break; }
    if (id != GARBAGE) {
      e2 = grn_hash_entry_at(ctx, hash, id, 0);
      if (!e2) { return GRN_ID_NIL; }
      if (e2 == e) { break; }
    }
  }
  return id;
}

int
grn_hash_sort(grn_ctx *ctx, grn_hash *hash,
              int limit, grn_array *result, grn_table_sort_optarg *optarg)
{
  entry **res;
  if (!result || !*hash->n_entries) { return 0; }
  if (!(res = GRN_MALLOC(sizeof(entry *) * *hash->n_entries))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "allocation of entries failed on grn_hash_sort !");
    return 0;
  }
  if (limit < 0) {
    limit += *hash->n_entries + 1;
    if (limit < 0) {
      GRN_LOG(ctx, GRN_LOG_ALERT, "limit is too small in grn_hash_sort !");
      return 0;
    }
  }
  if (limit > *hash->n_entries) { limit = *hash->n_entries; }
  //  hash->limit = limit;
  if (optarg) {
    int dir = (optarg->flags & GRN_TABLE_SORT_DESC);
    if ((optarg->flags & GRN_TABLE_SORT_BY_ID) ||
        (optarg->flags & GRN_TABLE_SORT_BY_VALUE)
        ? ((hash->value_size - optarg->offset) == sizeof(uint32_t))
        : (!(hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE)
           && hash->key_size == sizeof(uint32_t))) {
      if (sizeof(entry *) != sizeof(val32)) {
        GRN_FREE(res);
        if (!(res = GRN_MALLOC(sizeof(val32) * *hash->n_entries))) {
          GRN_LOG(ctx, GRN_LOG_ALERT, "allocation of entries failed on grn_hash_sort !");
          return 0;
        }
      }
      sort_val32(ctx, hash, (val32 *)res, limit, optarg, dir);
      {
        int i;
        grn_id *v;
        val32 *rp = (val32 *)res;
        for (i = 0; i < limit; i++, rp++) {
          if (!grn_array_add(ctx, result, (void **)&v)) { break; }
          if (!(*v = rp->id)) { break; }
        }
        GRN_FREE(res);
        return i;
      }
    } else {
      sort(ctx, hash, res, limit, optarg, dir);
    }
  } else {
    grn_table_sort_optarg opt = {0, NULL, NULL, NULL, 0};
    sort(ctx, hash, res, limit, &opt, 0);
  }
  {
    int i;
    grn_id *v;
    entry **rp = res;
    for (i = 0; i < limit; i++, rp++) {
      if (!grn_array_add(ctx, result, (void **)&v)) { break; }
      if (!(*v = entry2id(ctx, hash, *rp))) { break; }
    }
    GRN_FREE(res);
    return i;
  }
}

void
grn_hash_check(grn_ctx *ctx, grn_hash *hash)
{
  char buf[8];
  struct grn_hash_header *h = hash->header;
  GRN_OUTPUT_ARRAY_OPEN("RESULT", 1);
  GRN_OUTPUT_MAP_OPEN("SUMMARY", 24);
  GRN_OUTPUT_CSTR("flags");
  grn_itoh(h->flags, buf, 8);
  GRN_OUTPUT_STR(buf, 8);
  GRN_OUTPUT_CSTR("key_size");
  GRN_OUTPUT_INT64(hash->key_size);
  GRN_OUTPUT_CSTR("value_size");
  GRN_OUTPUT_INT64(hash->value_size);
  GRN_OUTPUT_CSTR("tokenizer");
  GRN_OUTPUT_INT64(h->tokenizer);
  GRN_OUTPUT_CSTR("curr_rec");
  GRN_OUTPUT_INT64(h->curr_rec);
  GRN_OUTPUT_CSTR("curr_key");
  GRN_OUTPUT_INT64(h->curr_key);
  GRN_OUTPUT_CSTR("idx_offset");
  GRN_OUTPUT_INT64(h->idx_offset);
  GRN_OUTPUT_CSTR("entry_size");
  GRN_OUTPUT_INT64(hash->entry_size);
  GRN_OUTPUT_CSTR("max_offset");
  GRN_OUTPUT_INT64(*hash->max_offset);
  GRN_OUTPUT_CSTR("n_entries");
  GRN_OUTPUT_INT64(*hash->n_entries);
  GRN_OUTPUT_CSTR("n_garbages");
  GRN_OUTPUT_INT64(*hash->n_garbages);
  GRN_OUTPUT_CSTR("lock");
  GRN_OUTPUT_INT64(h->lock);
  GRN_OUTPUT_MAP_CLOSE();
  GRN_OUTPUT_ARRAY_CLOSE();
}

/* rhash : grn_hash with subrecs */

#ifdef USE_GRN_INDEX2

static uint32_t default_flags = GRN_HASH_TINY;

grn_rc
grn_rhash_init(grn_ctx *ctx, grn_hash *hash, grn_rec_unit record_unit, int record_size,
               grn_rec_unit subrec_unit, int subrec_size, unsigned int max_n_subrecs)
{
  grn_rc rc;
  record_size = grn_rec_unit_size(record_unit, record_size);
  subrec_size = grn_rec_unit_size(subrec_unit, subrec_size);
  if (record_unit != grn_rec_userdef && subrec_unit != grn_rec_userdef) {
    subrec_size -= record_size;
  }
  if (!hash) { return GRN_INVALID_ARGUMENT; }
  if (record_size < 0) { return GRN_INVALID_ARGUMENT; }
  if ((default_flags & GRN_HASH_TINY)) {
    rc = tiny_hash_init(hash, ctx, NULL, record_size,
                        max_n_subrecs * (GRN_RSET_SCORE_SIZE + subrec_size),
                        default_flags, GRN_ENC_NONE);
  } else {
    rc = io_hash_init(hash, ctx, NULL, record_size,
                      max_n_subrecs * (GRN_RSET_SCORE_SIZE + subrec_size),
                      default_flags, GRN_ENC_NONE, 0);
  }
  if (rc) { return rc; }
  hash->record_unit = record_unit;
  hash->subrec_unit = subrec_unit;
  hash->subrec_size = subrec_size;
  hash->max_n_subrecs = max_n_subrecs;
  return rc;
}

grn_rc
grn_rhash_fin(grn_ctx *ctx, grn_hash *hash)
{
  grn_rc rc;
  if (IO_HASHP(hash)) {
    rc = grn_io_close(ctx, hash->io);
  } else {
    GRN_ASSERT(ctx == hash->ctx);
    rc = tiny_hash_fin(ctx, hash);
  }
  return rc;
}

inline static void
subrecs_push(byte *subrecs, int size, int n_subrecs, int score, void *body, int dir)
{
  byte *v;
  int *c2;
  int n = n_subrecs - 1, n2;
  while (n) {
    n2 = (n - 1) >> 1;
    c2 = GRN_RSET_SUBRECS_NTH(subrecs,size,n2);
    if (GRN_RSET_SUBRECS_CMP(score, *c2, dir)) { break; }
    GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
    n = n2;
  }
  v = subrecs + n * (size + GRN_RSET_SCORE_SIZE);
  *((int *)v) = score;
  memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
}

inline static void
subrecs_replace_min(byte *subrecs, int size, int n_subrecs, int score, void *body, int dir)
{
  byte *v;
  int n = 0, n1, n2, *c1, *c2;
  for (;;) {
    n1 = n * 2 + 1;
    n2 = n1 + 1;
    c1 = n1 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n1) : NULL;
    c2 = n2 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n2) : NULL;
    if (c1 && GRN_RSET_SUBRECS_CMP(score, *c1, dir)) {
      if (c2 &&
          GRN_RSET_SUBRECS_CMP(score, *c2, dir) &&
          GRN_RSET_SUBRECS_CMP(*c1, *c2, dir)) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c1);
        n = n1;
      }
    } else {
      if (c2 && GRN_RSET_SUBRECS_CMP(score, *c2, dir)) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        break;
      }
    }
  }
  v = subrecs + n * (size + GRN_RSET_SCORE_SIZE);
  memcpy(v, &score, GRN_RSET_SCORE_SIZE);
  memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
}

void
grn_rhash_add_subrec(grn_hash *s, grn_rset_recinfo *ri, int score, void *body, int dir)
{
  int limit = s->max_n_subrecs;
  ri->score += score;
  ri->n_subrecs += 1;
  if (limit) {
    int ssize = s->subrec_size;
    int n_subrecs = GRN_RSET_N_SUBRECS(ri);
    if (limit < n_subrecs) {
      if (GRN_RSET_SUBRECS_CMP(score, *ri->subrecs, dir)) {
        subrecs_replace_min(ri->subrecs, ssize, limit, score, body, dir);
      }
    } else {
      subrecs_push(ri->subrecs, ssize, n_subrecs, score, body, dir);
    }
  }
}

grn_hash *
grn_rhash_group(grn_hash *s, int limit, grn_group_optarg *optarg)
{
  grn_ctx *ctx = s->ctx;
  grn_hash *g, h;
  grn_rset_recinfo *ri;
  grn_rec_unit unit;
  grn_hash_cursor *c;
  grn_id rh;
  byte *key, *ekey, *gkey = NULL;
  int funcp, dir;
  unsigned int rsize;
  if (!s || !s->index) { return NULL; }
  if (optarg) {
    unit = grn_rec_userdef;
    rsize = optarg->key_size;
    funcp = optarg->func ? 1 : 0;
    dir = (optarg->mode == grn_sort_ascending) ? -1 : 1;
  } else {
    unit = grn_rec_document;
    rsize = grn_rec_unit_size(unit, sizeof(grn_id));
    funcp = 0;
    dir = 1;
  }
  if (funcp) {
    gkey = GRN_MALLOC(rsize ? rsize : 8192);
    if (!gkey) {
      GRN_LOG(ctx, GRN_LOG_ALERT, "allocation for gkey failed !");
      return NULL;
    }
  } else {
    if (s->key_size <= rsize) { return NULL; }
  }
  if (!(c = grn_hash_cursor_open(s->ctx, s, NULL, 0, NULL, -1, 0))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_hash_cursor_open on grn_hash_group failed !");
    if (gkey) { GRN_FREE(gkey); }
    return NULL;
  }
  memcpy(&h, s, sizeof(grn_hash));
  g = s;
  s = &h;
  if (grn_rhash_init(ctx, g, unit, rsize, s->record_unit, s->key_size, limit)) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "grn_rhash_init in grn_hash_group failed !");
    grn_hash_cursor_close(s->ctx, c);
    if (gkey) { GRN_FREE(gkey); }
    return NULL;
  }
  while ((rh = grn_hash_cursor_next(ctx, c))) {
    grn_hash_cursor_get_key_value(ctx, c, (void **)&key, NULL, (void **)&ri);
    if (funcp) {
      if (optarg->func((grn_records *)s,
                       (grn_recordh *)(intptr_t)rh, gkey, optarg->func_arg)) { continue; }
      ekey = key;
    } else {
      gkey = key;
      ekey = key + rsize;
    }
    {
      grn_rset_recinfo *gri;
      if (grn_hash_add(ctx, g, gkey, rsize, (void **)&gri, NULL)) {
        grn_rhash_add_subrec(g, gri, ri->score, ekey, dir);
      }
    }
  }
  grn_hash_cursor_close(s->ctx, c);
  grn_rhash_fin(s->ctx, s);
  if (funcp) { GRN_FREE(gkey); }
  return g;
}

grn_rc
grn_rhash_subrec_info(grn_hash *s, grn_id rh, int index,
                      grn_id *rid, int *section, int *pos, int *score, void **subrec)
{
  grn_rset_posinfo *pi;
  grn_rset_recinfo *ri;
  int *p, unit_size = GRN_RSET_SCORE_SIZE + s->subrec_size;
  if (!s || !rh || index < 0) { return GRN_INVALID_ARGUMENT; }
  if ((unsigned int)index >= s->max_n_subrecs) { return GRN_INVALID_ARGUMENT; }
  {
    entry_str *ee;
    if (!grn_hash_bitmap_at(ctx, s, rh)) { return GRN_INVALID_ARGUMENT; }
    ee = grn_hash_entry_at(ctx, s, rh, 0);
    if (!ee) { return GRN_INVALID_ARGUMENT; }
    pi = (grn_rset_posinfo *)get_key(ctx, s, ee);
    ri = get_value(s, ee);
    if (!pi || !ri) { return GRN_INVALID_ARGUMENT; }
  }
  if (index >= ri->n_subrecs) { return GRN_INVALID_ARGUMENT; }
  p = (int *)(ri->subrecs + index * unit_size);
  if (score) { *score = p[0]; }
  if (subrec) { *subrec = &p[1]; }
  switch (s->record_unit) {
  case grn_rec_document :
    if (rid) { *rid = pi->rid; }
    if (section) { *section = (s->subrec_unit != grn_rec_userdef) ? p[1] : 0; }
    if (pos) { *pos = (s->subrec_unit == grn_rec_position) ? p[2] : 0; }
    break;
  case grn_rec_section :
    if (rid) { *rid = pi->rid; }
    if (section) { *section = pi->sid; }
    if (pos) { *pos = (s->subrec_unit == grn_rec_position) ? p[1] : 0; }
    break;
  default :
    pi = (grn_rset_posinfo *)&p[1];
    switch (s->subrec_unit) {
    case grn_rec_document :
      if (rid) { *rid = pi->rid; }
      if (section) { *section = 0; }
      if (pos) { *pos = 0; }
      break;
    case grn_rec_section :
      if (rid) { *rid = pi->rid; }
      if (section) { *section = pi->sid; }
      if (pos) { *pos = 0; }
      break;
    case grn_rec_position :
      if (rid) { *rid = pi->rid; }
      if (section) { *section = pi->sid; }
      if (pos) { *pos = pi->pos; }
      break;
    default :
      if (rid) { *rid = 0; }
      if (section) { *section = 0; }
      if (pos) { *pos = 0; }
      break;
    }
    break;
  }
  return GRN_SUCCESS;
}
#endif /* USE_GRN_INDEX2 */
