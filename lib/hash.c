/*
  Copyright(C) 2009-2018  Brazil
  Copyright(C) 2018-2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_hash.h"
#include "grn_normalizer.h"
#include "grn_obj.h"
#include "grn_output.h"
#include "grn_posting.h"
#include "grn_selector.h"
#include "grn_store.h"
#include "grn_wal.h"

#include <string.h>
#include <limits.h>

/* grn_tiny_array */

/* Requirements: id != GRN_ID_NIL. */
grn_inline static int
grn_tiny_array_get_block_id(grn_id id)
{
  int most_significant_one_bit_offset;
  GRN_BIT_SCAN_REV(id, most_significant_one_bit_offset);
  return most_significant_one_bit_offset >> GRN_TINY_ARRAY_FACTOR;
}

/* Requirements: id != GRN_ID_NIL. */
grn_inline static void *
grn_tiny_array_get(grn_tiny_array *array, grn_id id) {
  const int block_id = grn_tiny_array_get_block_id(id);
  uint8_t * const block = (uint8_t *)array->blocks[block_id];
  if (block) {
    const size_t offset = GRN_TINY_ARRAY_GET_OFFSET(block_id);
    return block + (id - offset) * array->element_size;
  }
  return NULL;
}

/* Requirements: id != GRN_ID_NIL. */
grn_inline static void *
grn_tiny_array_put(grn_tiny_array *array, grn_id id) {
  const int block_id = grn_tiny_array_get_block_id(id);
  void ** const block = &array->blocks[block_id];
  const size_t offset = GRN_TINY_ARRAY_GET_OFFSET(block_id);
  if (!*block) {
    grn_ctx * const ctx = array->ctx;
    if (array->flags & GRN_TINY_ARRAY_THREADSAFE) {
      CRITICAL_SECTION_ENTER(array->lock);
    }
    if (!*block) {
      const size_t block_size =
          GRN_TINY_ARRAY_GET_BLOCK_SIZE(block_id) * array->element_size;
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
  return (uint8_t *)*block + (id - offset) * array->element_size;
}

grn_inline static void *
grn_tiny_array_at_inline(grn_tiny_array *array, grn_id id)
{
  return id ? grn_tiny_array_put(array, id) : NULL;
}

grn_inline static void *
grn_tiny_array_next(grn_tiny_array *array)
{
  return grn_tiny_array_put(array, array->max + 1);
}

void
grn_tiny_array_init(grn_ctx *ctx, grn_tiny_array *array,
                    uint16_t element_size, uint16_t flags)
{
  array->ctx = ctx;
  array->max = 0;
  array->element_size = element_size;
  array->flags = flags;
  memset(array->blocks, 0, sizeof(array->blocks));
  if (flags & GRN_TINY_ARRAY_THREADSAFE) {
    CRITICAL_SECTION_INIT(array->lock);
  }
}

void
grn_tiny_array_fin(grn_tiny_array *array)
{
  int block_id;
  grn_ctx * const ctx = array->ctx;
  for (block_id = 0; block_id < GRN_TINY_ARRAY_NUM_BLOCKS; block_id++) {
    if (array->blocks[block_id]) {
      if (array->flags & GRN_TINY_ARRAY_USE_MALLOC) {
        GRN_FREE(array->blocks[block_id]);
      } else {
        GRN_CTX_FREE(ctx, array->blocks[block_id]);
      }
      array->blocks[block_id] = NULL;
    }
  }
  if (array->flags & GRN_TINY_ARRAY_THREADSAFE) {
    CRITICAL_SECTION_FIN(array->lock);
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
  const uint8_t * const ptr = (const uint8_t *)element_address;
  uint32_t block_id, offset = 1;
  for (block_id = 0; block_id < GRN_TINY_ARRAY_NUM_BLOCKS; block_id++) {
    const uint32_t block_size = GRN_TINY_ARRAY_GET_BLOCK_SIZE(block_id);
    const uint8_t * const block = (const uint8_t *)array->blocks[block_id];
    if (block) {
      if (block <= ptr && ptr < (block + block_size * array->element_size)) {
        return offset + ((ptr - block) / array->element_size);
      }
    }
    offset += block_size;
  }
  return GRN_ID_NIL;
}

/* grn_tiny_bitmap */

static void
grn_tiny_bitmap_init(grn_ctx *ctx, grn_tiny_bitmap *bitmap)
{
  bitmap->ctx = ctx;
  memset(bitmap->blocks, 0, sizeof(bitmap->blocks));
}

static void
grn_tiny_bitmap_fin(grn_tiny_bitmap *bitmap)
{
  int block_id;
  grn_ctx * const ctx = bitmap->ctx;
  for (block_id = 0; block_id < GRN_TINY_ARRAY_NUM_BLOCKS; block_id++) {
    if (bitmap->blocks[block_id]) {
      GRN_CTX_FREE(ctx, bitmap->blocks[block_id]);
      bitmap->blocks[block_id] = NULL;
    }
  }
}

/* Requirements: bit_id != GRN_ID_NIL. */
grn_inline static uint8_t *
grn_tiny_bitmap_get_byte(grn_tiny_bitmap *bitmap, grn_id bit_id) {
  const uint32_t byte_id = (bit_id >> 3) + 1;
  const int block_id = grn_tiny_array_get_block_id(byte_id);
  uint8_t * const block = (uint8_t *)bitmap->blocks[block_id];
  if (block) {
    const size_t offset = GRN_TINY_ARRAY_GET_OFFSET(block_id);
    return block + byte_id - offset;
  }
  return NULL;
}

/* Requirements: bit_id != GRN_ID_NIL. */
grn_inline static uint8_t *
grn_tiny_bitmap_put_byte(grn_tiny_bitmap *bitmap, grn_id bit_id) {
  const uint32_t byte_id = (bit_id >> 3) + 1;
  const int block_id = grn_tiny_array_get_block_id(byte_id);
  void ** const block = &bitmap->blocks[block_id];
  const size_t offset = GRN_TINY_ARRAY_GET_OFFSET(block_id);
  if (!*block) {
    grn_ctx * const ctx = bitmap->ctx;
    *block = GRN_CTX_ALLOC(ctx, GRN_TINY_ARRAY_GET_BLOCK_SIZE(block_id));
    if (!*block) {
      return NULL;
    }
  }
  return (uint8_t *)*block + byte_id - offset;
}

/* Requirements: bit_id != GRN_ID_NIL. */
/* Return value: 1/0 on success, -1 on failure. */
grn_inline static int
grn_tiny_bitmap_get(grn_tiny_bitmap *bitmap, grn_id bit_id)
{
  uint8_t * const ptr = grn_tiny_bitmap_get_byte(bitmap, bit_id);
  return ptr ? ((*ptr >> (bit_id & 7)) & 1) : -1;
}

/* Requirements: bit_id != GRN_ID_NIL. */
/* Return value: 1/0 on success, -1 on failure. */
/* Note: A bitmap is extended if needed. */
grn_inline static int
grn_tiny_bitmap_put(grn_tiny_bitmap *bitmap, grn_id bit_id)
{
  uint8_t * const ptr = grn_tiny_bitmap_put_byte(bitmap, bit_id);
  return ptr ? ((*ptr >> (bit_id & 7)) & 1) : -1;
}

/* Requirements: bit_id != GRN_ID_NIL. */
grn_inline static uint8_t *
grn_tiny_bitmap_get_and_set(grn_tiny_bitmap *bitmap, grn_id bit_id,
                            grn_bool bit)
{
  uint8_t * const ptr = grn_tiny_bitmap_get_byte(bitmap, bit_id);
  if (ptr) {
    /* This branch will be removed because the given `bit' is constant. */
    if (bit) {
      *ptr |= 1 << (bit_id & 7);
    } else {
      *ptr &= ~(1 << (bit_id & 7));
    }
  }
  return ptr;
}

/* Requirements: bit_id != GRN_ID_NIL. */
/* Note: A bitmap is extended if needed. */
grn_inline static uint8_t *
grn_tiny_bitmap_put_and_set(grn_tiny_bitmap *bitmap, grn_id bit_id,
                            grn_bool bit)
{
  uint8_t * const ptr = grn_tiny_bitmap_put_byte(bitmap, bit_id);
  if (ptr) {
    /* This branch will be removed because the given `bit' is constant. */
    if (bit) {
      *ptr |= 1 << (bit_id & 7);
    } else {
      *ptr &= ~(1 << (bit_id & 7));
    }
  }
  return ptr;
}

/* grn_io_array */

#define GRN_ARRAY_MAX (GRN_ID_MAX - 8)

grn_inline static void *
grn_io_array_at_inline(grn_ctx *ctx, grn_io *io, uint32_t segment_id,
                       uint64_t offset, int flags)
{
  return grn_io_array_at(ctx, io, segment_id, offset, &flags);
}

/*
 * grn_io_array_bit_at() returns 1/0 on success, -1 on failure.
 */
grn_inline static int
grn_io_array_bit_at(grn_ctx *ctx, grn_io *io,
                    uint32_t segment_id, uint64_t offset)
{
  uint8_t * const ptr = (uint8_t *)grn_io_array_at_inline(
      ctx, io, segment_id, (offset >> 3) + 1, 0);
  return ptr ? ((*ptr >> (offset & 7)) & 1) : -1;
}

/*
 * The following functions, grn_io_array_bit_*(), return a non-NULL pointer on
 * success, a NULL pointer on failure.
 */
grn_inline static void *
grn_io_array_bit_on(grn_ctx *ctx, grn_io *io,
                    uint32_t segment_id, uint64_t offset)
{
  uint8_t * const ptr = (uint8_t *)grn_io_array_at_inline(
      ctx, io, segment_id, (offset >> 3) + 1, GRN_TABLE_ADD);
  if (ptr) {
    *ptr |= 1 << (offset & 7);
  }
  return ptr;
}

grn_inline static void *
grn_io_array_bit_off(grn_ctx *ctx, grn_io *io,
                     uint32_t segment_id, uint64_t offset)
{
  uint8_t * const ptr = (uint8_t *)grn_io_array_at_inline(
      ctx, io, segment_id, (offset >> 3) + 1, GRN_TABLE_ADD);
  if (ptr) {
    *ptr &= ~(1 << (offset & 7));
  }
  return ptr;
}

grn_inline static void *
grn_io_array_bit_flip(grn_ctx *ctx, grn_io *io,
                      uint32_t segment_id, uint64_t offset)
{
  uint8_t * const ptr = (uint8_t *)grn_io_array_at_inline(
      ctx, io, segment_id, (offset >> 3) + 1, GRN_TABLE_ADD);
  if (ptr) {
    *ptr ^= 1 << (offset & 7);
  }
  return ptr;
}

/* grn_array */

#define GRN_ARRAY_SEGMENT_SIZE 0x400000
#define GRN_ARRAY_STATUS_TRUNCATED                   (0x01<<0)
#define GRN_ARRAY_STATUS_GARBAGES_BUFFER_INITIALIZED (0x01<<1)
#define GRN_ARRAY_GARBAGES_BUFFER_SIZE 5

/* Header of grn_io-based grn_array. */
struct grn_array_header {
  uint32_t flags;
  uint32_t curr_rec;
  uint32_t value_size;
  uint32_t n_entries;
  uint32_t n_garbages;
  grn_id garbages;
  uint32_t lock;
  uint32_t status;
  uint32_t n_garbages_in_buffer;
  grn_id garbages_buffer[GRN_ARRAY_GARBAGES_BUFFER_SIZE];
  uint32_t reserved[1];
};

/*
 * A grn_io-based grn_array consists of the following 2 segments.
 * GRN_ARRAY_VALUE_SEGMENT: stores values.
 * GRN_ARRAY_BITMAP_SEGMENT: stores whether entries are valid or not.
 */
enum {
  GRN_ARRAY_VALUE_SEGMENT = 0,
  GRN_ARRAY_BITMAP_SEGMENT = 1
};

grn_inline static grn_bool
grn_array_is_io_array(grn_array *array)
{
  return array->io != NULL;
}

grn_inline static grn_bool
grn_array_can_use_value_for_garbage(grn_array *array)
{
  return array->value_size >= sizeof(grn_id);
}

/*
 * array must be grn_array_is_io_array(array) &&
 * !grn_array_can_use_value_for_garbage(array). The
 * grn_array_is_io_array(array) restriction may be removed when we
 * need to implement this for tiny array.
 */
grn_inline static void
grn_array_put_garbage_to_buffer(grn_ctx *ctx,
                                grn_array *array,
                                grn_id garbage_id)
{
  struct grn_array_header * const header = array->header;
  if (header->garbages != GRN_ID_NIL &&
      header->n_garbages_in_buffer < GRN_ARRAY_GARBAGES_BUFFER_SIZE) {
    int i;
    for (i = 0; i < GRN_ARRAY_GARBAGES_BUFFER_SIZE; i++) {
      if (header->garbages_buffer[i] == GRN_ID_NIL) {
        header->garbages_buffer[i] = header->garbages;
        header->n_garbages_in_buffer++;
        header->garbages = GRN_ID_NIL;
        break;
      }
    }
  }
  if (header->garbages == GRN_ID_NIL) {
    header->garbages = garbage_id;
  }
}

grn_inline static void *
grn_array_io_entry_at(grn_ctx *ctx, grn_array *array, grn_id id, int flags)
{
  return grn_io_array_at_inline(ctx, array->io, GRN_ARRAY_VALUE_SEGMENT, id, flags);
}

grn_inline static void *
grn_array_entry_at(grn_ctx *ctx, grn_array *array, grn_id id, int flags)
{
  if (grn_array_is_io_array(array)) {
    return grn_array_io_entry_at(ctx, array, id, flags);
  } else {
    return grn_tiny_array_at_inline(&array->array, id);
  }
}

/* grn_array_bitmap_at() returns 1/0 on success, -1 on failure. */
grn_inline static int
grn_array_bitmap_at(grn_ctx *ctx, grn_array *array, grn_id id)
{
  if (grn_array_is_io_array(array)) {
    int bit = grn_io_array_bit_at(ctx, array->io, GRN_ARRAY_BITMAP_SEGMENT, id);
    if (bit == 0 && !grn_array_can_use_value_for_garbage(array)) {
      grn_array_put_garbage_to_buffer(ctx, array, id);
    }
    return bit;
  } else {
    return grn_tiny_bitmap_put(&array->bitmap, id);
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
  array->n_garbages = &array->n_garbages_buf;
  array->n_entries = &array->n_entries_buf;
  array->n_garbages_buf = 0;
  array->n_entries_buf = 0;
  array->io = NULL;
  array->header = NULL;
  array->garbages = GRN_ID_NIL;
  grn_tiny_array_init(ctx, &array->array, value_size, GRN_TINY_ARRAY_CLEAR);
  grn_tiny_bitmap_init(ctx, &array->bitmap);
  return GRN_SUCCESS;
}

static grn_io *
grn_array_create_io_array(grn_ctx *ctx, const char *path, uint32_t value_size)
{
  uint32_t w_of_element = 0;
  grn_io_array_spec array_spec[2];

  while ((1U << w_of_element) < value_size) {
    w_of_element++;
  }

  array_spec[GRN_ARRAY_VALUE_SEGMENT].w_of_element = w_of_element;
  array_spec[GRN_ARRAY_VALUE_SEGMENT].max_n_segments =
      1U << (30 - (22 - w_of_element));
  array_spec[GRN_ARRAY_BITMAP_SEGMENT].w_of_element = 0;
  array_spec[GRN_ARRAY_BITMAP_SEGMENT].max_n_segments = 1U << (30 - (22 + 3));
  return grn_io_create_with_array(ctx, path, sizeof(struct grn_array_header),
                                  GRN_ARRAY_SEGMENT_SIZE, GRN_IO_AUTO,
                                  2, array_spec);
}

static grn_rc
grn_array_init_io_array(grn_ctx *ctx, grn_array *array, const char *path,
                        uint32_t value_size, uint32_t flags)
{
  grn_io *io;
  struct grn_array_header *header;

  io = grn_array_create_io_array(ctx, path, value_size);
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
  header->status = 0;
  header->n_garbages_in_buffer = 0;
  {
    int i;
    for (i = 0; i < GRN_ARRAY_GARBAGES_BUFFER_SIZE; i++) {
      header->garbages_buffer[i] = GRN_ID_NIL;
    }
  }
  header->status |= GRN_ARRAY_STATUS_GARBAGES_BUFFER_INITIALIZED;
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
    grn_array * const array = (grn_array *)GRN_CALLOC(sizeof(grn_array));
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
    grn_io * const io = grn_io_open(ctx, path, GRN_IO_AUTO);
    if (io) {
      struct grn_array_header * const header = grn_io_header(io);
      uint32_t io_type = grn_io_get_type(io);
      if (io_type == GRN_TABLE_NO_KEY) {
        grn_array * const array = (grn_array *)GRN_CALLOC(sizeof(grn_array));
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
            /* For old array created by Groonga that doesn't have
             * garbages_buffer. */
            if (!grn_array_can_use_value_for_garbage(array) &&
                !(header->status & GRN_ARRAY_STATUS_GARBAGES_BUFFER_INITIALIZED)) {
              header->n_garbages_in_buffer = 0;
              int i;
              for (i = 0; i < GRN_ARRAY_GARBAGES_BUFFER_SIZE; i++) {
                header->garbages_buffer[i] = GRN_ID_NIL;
              }
              header->status |= GRN_ARRAY_STATUS_GARBAGES_BUFFER_INITIALIZED;
            }
            return array;
          } else {
            GRN_LOG(ctx, GRN_LOG_NOTICE, "invalid array flags. (%x)", header->flags);
          }
          GRN_FREE(array);
        }
      } else {
        ERR(GRN_INVALID_FORMAT,
            "[table][array] file type must be %#04x: <%#04x>",
            GRN_TABLE_NO_KEY, io_type);
      }
      grn_io_close(ctx, io);
    }
  }
  return NULL;
}

/*
 * grn_array_error_if_truncated() logs an error and returns its error code if
 * an array is truncated by another process.
 * Otherwise, this function returns GRN_SUCCESS.
 * Note that `ctx` and `array` must be valid.
 *
 * FIXME: An array should be reopened if possible.
 */
static grn_rc
grn_array_error_if_truncated(grn_ctx *ctx, grn_array *array)
{
  if (array->header && (array->header->status & GRN_ARRAY_STATUS_TRUNCATED)) {
    ERR(GRN_FILE_CORRUPT,
        "array is truncated, please unmap or reopen the database");
    return GRN_FILE_CORRUPT;
  }
  return GRN_SUCCESS;
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
    grn_tiny_array_fin(&array->array);
    grn_tiny_bitmap_fin(&array->bitmap);
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

uint32_t
grn_array_size(grn_ctx *ctx, grn_array *array)
{
  if (grn_array_error_if_truncated(ctx, array) != GRN_SUCCESS) {
    return 0;
  }
  return *array->n_entries;
}

uint32_t
grn_array_get_flags(grn_ctx *ctx, grn_array *array)
{
  return array->header->flags;
}

grn_rc
grn_array_truncate(grn_ctx *ctx, grn_array *array)
{
  grn_rc rc;
  char *path = NULL;
  uint32_t value_size, flags;

  if (!ctx || !array) { return GRN_INVALID_ARGUMENT; }
  rc = grn_array_error_if_truncated(ctx, array);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (grn_array_is_io_array(array)) {
    const char * const io_path = grn_io_path(array->io);
    if (io_path && *io_path) {
      path = GRN_STRDUP(io_path);
      if (!path) {
        ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_path);
        return GRN_NO_MEMORY_AVAILABLE;
      }
    }
  }
  value_size = array->value_size;
  flags = array->obj.header.flags;

  if (grn_array_is_io_array(array)) {
    if (path) {
      /* Only an I/O array with a valid path uses the `truncated` flag. */
      array->header->status |= GRN_ARRAY_STATUS_TRUNCATED;
    }
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

grn_inline static grn_id
grn_array_get_max_id(grn_array *array)
{
  return grn_array_is_io_array(array) ? array->header->curr_rec : array->array.max;
}

grn_inline static void *
grn_array_get_value_inline(grn_ctx *ctx, grn_array *array, grn_id id)
{
  if (!ctx || !array) {
    return NULL;
  }
  if (grn_array_error_if_truncated(ctx, array) != GRN_SUCCESS) {
    return NULL;
  }
  if (*array->n_garbages) {
    /*
     * grn_array_bitmap_at() is a time-consuming function, so it is called only
     * when there are garbages in the array.
     */
    if (grn_array_bitmap_at(ctx, array, id) != 1) {
      return NULL;
    }
  } else if (id == 0 || id > grn_array_get_max_id(array)) {
    return NULL;
  }
  return grn_array_entry_at(ctx, array, id, 0);
}

int
grn_array_get_value(grn_ctx *ctx, grn_array *array, grn_id id, void *valuebuf)
{
  void * const value = grn_array_get_value_inline(ctx, array, id);
  if (value) {
    if (valuebuf) {
      grn_memcpy(valuebuf, value, array->value_size);
    }
    return array->value_size;
  }
  return 0;
}

void *
_grn_array_get_value(grn_ctx *ctx, grn_array *array, grn_id id)
{
  return grn_array_get_value_inline(ctx, array, id);
}

grn_inline static grn_rc
grn_array_set_value_inline(grn_ctx *ctx, grn_array *array, grn_id id,
                           const void *value, int flags)
{
  void *entry;
  entry = grn_array_entry_at(ctx, array, id, 0);
  if (!entry) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  switch ((flags & GRN_OBJ_SET_MASK)) {
  case GRN_OBJ_SET :
    grn_memcpy(entry, value, array->value_size);
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
    /* todo : support other types. */
    return GRN_INVALID_ARGUMENT;
  }
}

grn_rc
grn_array_set_value(grn_ctx *ctx, grn_array *array, grn_id id,
                    const void *value, int flags)
{
  grn_rc rc;

  if (!ctx || !array || !value) {
    return GRN_INVALID_ARGUMENT;
  }

  rc = grn_array_error_if_truncated(ctx, array);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (*array->n_garbages) {
    /*
     * grn_array_bitmap_at() is a time-consuming function, so it is called only
     * when there are garbages in the array.
     */
    if (grn_array_bitmap_at(ctx, array, id) != 1) {
      return GRN_INVALID_ARGUMENT;
    }
  } else if (id == 0 || id > grn_array_get_max_id(array)) {
    return GRN_INVALID_ARGUMENT;
  }
  return grn_array_set_value_inline(ctx, array, id, value, flags);
}

grn_rc
grn_array_delete_by_id(grn_ctx *ctx, grn_array *array, grn_id id,
                       grn_table_delete_optarg *optarg)
{
  grn_rc rc;
  if (!ctx || !array) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_array_error_if_truncated(ctx, array);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  if (grn_array_bitmap_at(ctx, array, id) != 1) {
    return GRN_INVALID_ARGUMENT;
  }

  {
    rc = GRN_SUCCESS;
    /* lock */
    if (grn_array_is_io_array(array)) {
      if (grn_array_can_use_value_for_garbage(array)) {
        struct grn_array_header * const header = array->header;
        void * const entry = grn_array_io_entry_at(ctx, array, id, 0);
        if (!entry) {
          rc = GRN_INVALID_ARGUMENT;
        } else {
          *((grn_id *)entry) = header->garbages;
          header->garbages = id;
        }
      } else {
        grn_array_put_garbage_to_buffer(ctx, array, id);
      }
      if (!rc) {
        (*array->n_entries)--;
        (*array->n_garbages)++;
        /*
         * The following grn_io_array_bit_off() fails iff a problem has
         * occurred after the above grn_array_bitmap_at(). That is to say,
         * an unexpected case.
         */
        grn_io_array_bit_off(ctx, array->io, GRN_ARRAY_BITMAP_SEGMENT, id);
      }
    } else {
      if (grn_array_can_use_value_for_garbage(array)) {
        void * const entry = grn_tiny_array_get(&array->array, id);
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
         * The following grn_io_array_bit_off() fails iff a problem has
         * occurred after the above grn_array_bitmap_at(). That is to say,
         * an unexpected case.
         */
        grn_tiny_bitmap_get_and_set(&array->bitmap, id, 0);
      }
    }
    /* unlock */
    return rc;
  }
}

grn_id
grn_array_at(grn_ctx *ctx, grn_array *array, grn_id id)
{
  if (grn_array_error_if_truncated(ctx, array) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  if (*array->n_garbages) {
    /*
     * grn_array_bitmap_at() is a time-consuming function, so it is called only
     * when there are garbages in the array.
     */
    if (grn_array_bitmap_at(ctx, array, id) != 1) {
      return GRN_ID_NIL;
    }
  } else if (id > grn_array_get_max_id(array)) {
    return GRN_ID_NIL;
  }
  return id;
}

grn_rc
grn_array_copy_sort_key(grn_ctx *ctx, grn_array *array,
                        grn_table_sort_key *keys, int n_keys)
{
  array->keys = (grn_table_sort_key *)GRN_MALLOCN(grn_table_sort_key, n_keys);
  if (!array->keys) {
    return ctx->rc;
  }
  grn_memcpy(array->keys, keys, sizeof(grn_table_sort_key) * n_keys);
  array->n_keys = n_keys;
  return GRN_SUCCESS;
}

void
grn_array_cursor_close(grn_ctx *ctx, grn_array_cursor *cursor)
{
  GRN_ASSERT(cursor->ctx == ctx);
  GRN_FREE(cursor);
}

grn_array_cursor *
grn_array_cursor_open(grn_ctx *ctx, grn_array *array, grn_id min, grn_id max,
                      int offset, int limit, int flags)
{
  grn_array_cursor *cursor;
  if (!array || !ctx) { return NULL; }
  if (grn_array_error_if_truncated(ctx, array) != GRN_SUCCESS) {
    return NULL;
  }

  cursor = (grn_array_cursor *)GRN_MALLOCN(grn_array_cursor, 1);
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
      if (grn_array_bitmap_at(ctx, cursor->array, cursor->curr_rec) == 1) {
        offset--;
      }
    }
  } else {
    cursor->curr_rec += cursor->dir * offset;
  }
  cursor->rest = (limit < 0) ? *(array->n_entries) : limit;
  return cursor;
}

grn_id
grn_array_cursor_next(grn_ctx *ctx, grn_array_cursor *cursor)
{
  if (cursor && cursor->rest) {
    while (cursor->curr_rec != cursor->tail) {
      cursor->curr_rec += cursor->dir;
      if (*cursor->array->n_garbages) {
        if (grn_array_bitmap_at(ctx, cursor->array, cursor->curr_rec) != 1) {
          continue;
        }
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
  grn_id max_id;
  if (grn_array_error_if_truncated(ctx, array) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  max_id = grn_array_get_max_id(array);
  while (++id <= max_id) {
    if (!*array->n_garbages ||
        grn_array_bitmap_at(ctx, array, id) == 1) {
      return id;
    }
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
  return grn_array_set_value_inline(ctx, cursor->array, cursor->curr_rec,
                                    value, flags);
}

grn_rc
grn_array_cursor_delete(grn_ctx *ctx, grn_array_cursor *cursor,
                        grn_table_delete_optarg *optarg)
{
  return grn_array_delete_by_id(ctx, cursor->array, cursor->curr_rec, optarg);
}

grn_inline static grn_id
grn_array_add_to_tiny_array(grn_ctx *ctx, grn_array *array, void **value)
{
  grn_id id = array->garbages;
  void *entry;
  if (id) {
    /* These operations fail iff the array is broken. */
    entry = grn_tiny_array_get(&array->array, id);
    if (!entry) {
      return GRN_ID_NIL;
    }
    array->garbages = *(grn_id *)entry;
    memset(entry, 0, array->value_size);
    (*array->n_garbages)--;
    if (!grn_tiny_bitmap_get_and_set(&array->bitmap, id, 1)) {
      /* Actually, it is difficult to recover from this error. */
      *(grn_id *)entry = array->garbages;
      array->garbages = id;
      (*array->n_garbages)++;
      return GRN_ID_NIL;
    }
  } else {
    id = array->array.max + 1;
    if (!grn_tiny_bitmap_put_and_set(&array->bitmap, id, 1)) {
      return GRN_ID_NIL;
    }
    entry = grn_tiny_array_put(&array->array, id);
    if (!entry) {
      grn_tiny_bitmap_get_and_set(&array->bitmap, id, 0);
      return GRN_ID_NIL;
    }
    array->array.max = id;
  }
  (*array->n_entries)++;
  if (value) { *value = entry; }
  return id;
}

grn_inline static grn_id
grn_array_add_to_io_array(grn_ctx *ctx, grn_array *array, void **value)
{
  grn_id id;
  void *entry = NULL;
  struct grn_array_header *header;
  if (grn_array_error_if_truncated(ctx, array) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  header = array->header;
  id = header->garbages;
  if (id) {
    if (grn_array_can_use_value_for_garbage(array)) {
      /* These operations fail iff the array is broken. */
      entry = grn_array_io_entry_at(ctx, array, id, GRN_TABLE_ADD);
      if (!entry) {
        return GRN_ID_NIL;
      }
      header->garbages = *(grn_id *)entry;
      memset(entry, 0, header->value_size);
      (*array->n_garbages)--;
      if (!grn_io_array_bit_on(ctx, array->io, GRN_ARRAY_BITMAP_SEGMENT, id)) {
        /* Actually, it is difficult to recover from this error. */
        *(grn_id *)entry = header->garbages;
        header->garbages = id;
        (*array->n_garbages)++;
        return GRN_ID_NIL;
      }
    } else {
      entry = grn_array_io_entry_at(ctx, array, id, GRN_TABLE_ADD);
      if (!entry) {
        return GRN_ID_NIL;
      }
      header->garbages = GRN_ID_NIL;
      (*array->n_garbages)--;
      if (!grn_io_array_bit_on(ctx, array->io, GRN_ARRAY_BITMAP_SEGMENT, id)) {
        /* Actually, it is difficult to recover from this error. */
        header->garbages = id;
        (*array->n_garbages)++;
        return GRN_ID_NIL;
      }
      if ((*array->n_garbages) > 0 &&
          header->n_garbages_in_buffer > 0) {
        int i;
        for (i = 0; i < GRN_ARRAY_GARBAGES_BUFFER_SIZE; i++) {
          if (header->garbages_buffer[i] != GRN_ID_NIL) {
            header->garbages = header->garbages_buffer[i];
            header->garbages_buffer[i] = GRN_ID_NIL;
            header->n_garbages_in_buffer--;
            break;
          }
        }
      }
    }
  } else {
    if (header->curr_rec >= GRN_ARRAY_MAX) { return GRN_ID_NIL; }
    id = header->curr_rec + 1;
    if (!grn_io_array_bit_on(ctx, array->io, GRN_ARRAY_BITMAP_SEGMENT, id)) {
      return GRN_ID_NIL;
    }
    entry = grn_array_io_entry_at(ctx, array, id, GRN_TABLE_ADD);
    if (!entry) {
      grn_io_array_bit_off(ctx, array->io, GRN_ARRAY_BITMAP_SEGMENT, id);
      return GRN_ID_NIL;
    }
    header->curr_rec = id;
  }
  (*array->n_entries)++;
  if (value) { *value = entry; }
  return id;
}

void
grn_array_clear_curr_rec(grn_ctx *ctx, grn_array *array)
{
  struct grn_array_header * const header = array->header;
  header->curr_rec = GRN_ID_NIL;
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

grn_rc
grn_array_warm(grn_ctx *ctx, grn_array *array)
{
  if (grn_array_is_io_array(array)) {
    return grn_io_warm(ctx, array->io);
  } else {
    return ctx->rc;
  }
}

/* grn_hash : hash table */

#define GRN_HASH_MAX_SEGMENT  0x400
#define GRN_HASH_HEADER_SIZE_NORMAL 0x9000
#define GRN_HASH_HEADER_SIZE_LARGE\
  (GRN_HASH_HEADER_SIZE_NORMAL +\
   (sizeof(grn_id) *\
    (GRN_HASH_MAX_KEY_SIZE_LARGE - GRN_HASH_MAX_KEY_SIZE_NORMAL)))
#define GRN_HASH_SEGMENT_SIZE 0x400000
#define GRN_HASH_KEY_MAX_N_SEGMENTS_NORMAL 0x400
#define GRN_HASH_KEY_MAX_N_SEGMENTS_LARGE 0x40000
#define W_OF_KEY_IN_A_SEGMENT 22
#define GRN_HASH_KEY_MAX_TOTAL_SIZE_NORMAL\
  (((uint64_t)(1) << W_OF_KEY_IN_A_SEGMENT) *\
   GRN_HASH_KEY_MAX_N_SEGMENTS_NORMAL - 1)
#define GRN_HASH_KEY_MAX_TOTAL_SIZE_LARGE\
  (((uint64_t)(1) << W_OF_KEY_IN_A_SEGMENT) *\
   GRN_HASH_KEY_MAX_N_SEGMENTS_LARGE - 1)
#define IDX_MASK_IN_A_SEGMENT 0xfffff

static uint32_t grn_hash_initial_max_offset = IDX_MASK_IN_A_SEGMENT + 1;

void
grn_hash_init_from_env(void)
{
  {
    /* Just for test. */
    char grn_hash_initial_max_offset_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_HASH_INITIAL_MAX_OFFSET",
               grn_hash_initial_max_offset_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_hash_initial_max_offset_env[0]) {
      grn_hash_initial_max_offset = atoi(grn_hash_initial_max_offset_env);
    }
  }
}

typedef struct {
  uint8_t key[4];
  uint8_t value[1];
} grn_plain_hash_entry;

typedef struct {
  uint32_t hash_value;
  uint8_t key_and_value[1];
} grn_rich_hash_entry;

typedef struct {
  uint32_t hash_value;
  uint16_t flag;
  uint16_t key_size;
  union {
    uint8_t buf[sizeof(uint32_t)];
    uint32_t offset;
  } key;
  uint8_t value[1];
} grn_io_hash_entry_normal;

typedef struct {
  uint32_t hash_value;
  uint16_t flag;
  uint16_t key_size;
  union {
    uint8_t buf[sizeof(uint64_t)];
    uint64_t offset;
  } key;
  uint8_t value[1];
} grn_io_hash_entry_large;

typedef struct {
  uint32_t hash_value;
  uint16_t flag;
  uint16_t key_size;
  union {
    uint8_t buf[sizeof(void *)];
    void *ptr;
  } key;
  uint8_t value[1];
} grn_tiny_hash_entry;

/*
 * hash_value is valid even if the entry is grn_plain_hash_entry. In this case,
 * its hash_value equals its key.
 * flag, key_size and key.buf are valid if the entry has a variable length key.
 */
typedef struct {
  uint32_t hash_value;
  uint16_t flag;
  uint16_t key_size;
} grn_hash_entry_header;

typedef union {
  uint32_t hash_value;
  grn_hash_entry_header header;
  grn_plain_hash_entry plain_entry;
  grn_rich_hash_entry rich_entry;
  grn_io_hash_entry_normal io_entry_normal;
  grn_io_hash_entry_large io_entry_large;
  grn_tiny_hash_entry tiny_entry;
} grn_hash_entry;

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

enum {
  GRN_HASH_KEY_SEGMENT    = 0,
  GRN_HASH_ENTRY_SEGMENT  = 1,
  GRN_HASH_INDEX_SEGMENT  = 2,
  GRN_HASH_BITMAP_SEGMENT = 3
};
#define GRN_HASH_N_SEGMENTS 4

grn_inline static grn_bool
grn_hash_is_io_hash(grn_hash *hash)
{
  return hash->io != NULL;
}

grn_inline static void *
grn_io_hash_entry_at(grn_ctx *ctx, grn_hash *hash, grn_id id, int flags)
{
  return grn_io_array_at_inline(ctx, hash->io, GRN_HASH_ENTRY_SEGMENT, id, flags);
}

/* todo : error handling */
grn_inline static void *
grn_hash_entry_at(grn_ctx *ctx, grn_hash *hash, grn_id id, int flags)
{
  if (grn_hash_is_io_hash(hash)) {
    return grn_io_hash_entry_at(ctx, hash, id, flags);
  } else {
    return grn_tiny_array_at_inline(&hash->a, id);
  }
}

grn_inline static grn_bool
grn_hash_bitmap_at(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  if (grn_hash_is_io_hash(hash)) {
    return grn_io_array_bit_at(ctx, hash->io, GRN_HASH_BITMAP_SEGMENT, id) == 1;
  } else {
    return grn_tiny_bitmap_put(&hash->bitmap, id) == 1;
  }
}

grn_inline static grn_id *
grn_io_hash_idx_at(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  return grn_io_array_at_inline(ctx, hash->io, GRN_HASH_INDEX_SEGMENT,
                                id, GRN_TABLE_ADD);
}

grn_inline static grn_id *
grn_hash_idx_at(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  if (grn_hash_is_io_hash(hash)) {
    id = (id & *hash->max_offset) + hash->header.common->idx_offset;
    return grn_io_hash_idx_at(ctx, hash, id);
  } else {
    return hash->index + (id & *hash->max_offset);
  }
}

grn_inline static void *
grn_io_hash_key_at(grn_ctx *ctx, grn_hash *hash, uint64_t pos)
{
  return grn_io_array_at_inline(ctx, hash->io, GRN_HASH_KEY_SEGMENT,
                                pos, GRN_TABLE_ADD);
}

#define HASH_IMMEDIATE 1

#define MAX_INDEX_SIZE ((GRN_HASH_MAX_SEGMENT * (IDX_MASK_IN_A_SEGMENT + 1)) >> 1)

grn_inline static uint16_t
grn_hash_entry_get_key_size(grn_hash *hash, grn_hash_entry *entry)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    return entry->header.key_size;
  } else {
    return hash->key_size;
  }
}

grn_inline static char *
grn_hash_entry_get_key(grn_ctx *ctx, grn_hash *hash, grn_hash_entry *entry)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (grn_hash_is_io_hash(hash)) {
      if (grn_hash_is_large_total_key_size(ctx, hash)) {
        if (entry->io_entry_large.flag & HASH_IMMEDIATE) {
          return (char *)entry->io_entry_large.key.buf;
        } else {
          return (char *)grn_io_hash_key_at(ctx, hash,
                                            entry->io_entry_large.key.offset);
        }
      } else {
        if (entry->io_entry_normal.flag & HASH_IMMEDIATE) {
          return (char *)entry->io_entry_normal.key.buf;
        } else {
          return (char *)grn_io_hash_key_at(ctx, hash,
                                            entry->io_entry_normal.key.offset);
        }
      }
    } else {
      if (entry->tiny_entry.flag & HASH_IMMEDIATE) {
        return (char *)entry->tiny_entry.key.buf;
      } else {
        return entry->tiny_entry.key.ptr;
      }
    }
  } else {
    if (hash->key_size == sizeof(uint32_t)) {
      return (char *)entry->plain_entry.key;
    } else {
      return (char *)entry->rich_entry.key_and_value;
    }
  }
}

grn_inline static void *
grn_hash_entry_get_value(grn_ctx *ctx, grn_hash *hash, grn_hash_entry *entry)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (grn_hash_is_io_hash(hash)) {
      if (grn_hash_is_large_total_key_size(ctx, hash)) {
        return entry->io_entry_large.value;
      } else {
        return entry->io_entry_normal.value;
      }
    } else {
      return entry->tiny_entry.value;
    }
  } else {
    if (hash->key_size == sizeof(uint32_t)) {
      return entry->plain_entry.value;
    } else {
      return entry->rich_entry.key_and_value + hash->key_size;
    }
  }
}

grn_inline static grn_rc
grn_io_hash_entry_put_key(grn_ctx *ctx, grn_hash *hash,
                          grn_hash_entry *entry,
                          const void *key, unsigned int key_size)
{
  grn_bool is_large_mode;
  bool key_exist;
  uint64_t key_offset;
  grn_io_hash_entry_normal *io_entry_normal = &(entry->io_entry_normal);
  grn_io_hash_entry_large *io_entry_large = &(entry->io_entry_large);

  is_large_mode = grn_hash_is_large_total_key_size(ctx, hash);

  if (is_large_mode) {
    key_exist = (io_entry_large->key_size > 0);
  } else {
    key_exist = (io_entry_normal->key_size > 0);
  }

  if (key_exist) {
    if (is_large_mode) {
      key_offset = io_entry_large->key.offset;
    } else {
      key_offset = io_entry_normal->key.offset;
    }
  } else {
    uint64_t segment_id;
    grn_hash_header_common *header;
    uint64_t curr_key;
    uint64_t max_total_size;

    header = hash->header.common;
    if (key_size >= GRN_HASH_SEGMENT_SIZE) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_INVALID_ARGUMENT,
          "[hash][key][put] too long key: <%.*s>: max=%u: key size=%u",
          name_size, name,
          GRN_HASH_SEGMENT_SIZE,
          key_size);
      return ctx->rc;
    }

    if (is_large_mode) {
      curr_key = header->curr_key_large;
      max_total_size = GRN_HASH_KEY_MAX_TOTAL_SIZE_LARGE;
    } else {
      curr_key = header->curr_key_normal;
      max_total_size = GRN_HASH_KEY_MAX_TOTAL_SIZE_NORMAL;
    }

    if (key_size > (max_total_size - curr_key)) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_NOT_ENOUGH_SPACE,
          "[hash][key][put] total key size is over: <%.*s>: "
          "max=%" GRN_FMT_INT64U ": "
          "current=%" GRN_FMT_INT64U ": "
          "new key size=%u",
          name_size, name,
          max_total_size,
          curr_key,
          key_size);
      return ctx->rc;
    }
    key_offset = curr_key;
    segment_id = (key_offset + key_size) >> W_OF_KEY_IN_A_SEGMENT;
    if ((key_offset >> W_OF_KEY_IN_A_SEGMENT) != segment_id) {
      key_offset = segment_id << W_OF_KEY_IN_A_SEGMENT;
      if (is_large_mode) {
        header->curr_key_large = key_offset;
      } else {
        header->curr_key_normal = key_offset;
      }
    }
    if (is_large_mode) {
      header->curr_key_large += key_size;
      io_entry_large->key.offset = key_offset;
    } else {
      header->curr_key_normal += key_size;
      io_entry_normal->key.offset = key_offset;
    }
  }

  {
    void * const key_ptr = grn_io_hash_key_at(ctx, hash, key_offset);
    if (!key_ptr) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_NO_MEMORY_AVAILABLE,
          "[hash][key][put] failed to allocate for new key: <%.*s>: "
          "new offset:%" GRN_FMT_INT64U " "
          "key size:%u",
          name_size, name,
          key_offset,
          key_size);
      return ctx->rc;
    }
    grn_memcpy(key_ptr, key, key_size);
  }
  return GRN_SUCCESS;
}

grn_inline static grn_rc
grn_hash_entry_put_key(grn_ctx *ctx, grn_hash *hash,
                       grn_hash_entry *entry, uint32_t hash_value,
                       const void *key, unsigned int key_size)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (grn_hash_is_io_hash(hash)) {
      grn_bool is_large_mode;
      uint8_t *buffer;
      size_t buffer_size;
      uint16_t flag;

      is_large_mode = grn_hash_is_large_total_key_size(ctx, hash);
      if (is_large_mode) {
        buffer = entry->io_entry_large.key.buf;
        buffer_size = sizeof(entry->io_entry_large.key.buf);
      } else {
        buffer = entry->io_entry_normal.key.buf;
        buffer_size = sizeof(entry->io_entry_normal.key.buf);
      }

      if (key_size <= buffer_size) {
        grn_memcpy(buffer, key, key_size);
        flag = HASH_IMMEDIATE;
      } else {
        const grn_rc rc =
          grn_io_hash_entry_put_key(ctx, hash, entry, key, key_size);
        if (rc) {
          return rc;
        }
        flag = 0;
      }

      if (is_large_mode) {
        entry->io_entry_large.flag = flag;
        entry->io_entry_large.hash_value = hash_value;
        entry->io_entry_large.key_size = key_size;
      } else {
        entry->io_entry_normal.flag = flag;
        entry->io_entry_normal.hash_value = hash_value;
        entry->io_entry_normal.key_size = key_size;
      }
    } else {
      if (key_size <= sizeof(entry->tiny_entry.key.buf)) {
        grn_memcpy(entry->tiny_entry.key.buf, key, key_size);
        entry->tiny_entry.flag = HASH_IMMEDIATE;
      } else {
        grn_ctx * const ctx = hash->ctx;
        entry->tiny_entry.key.ptr = GRN_CTX_ALLOC(ctx, key_size);
        if (!entry->tiny_entry.key.ptr) {
          return GRN_NO_MEMORY_AVAILABLE;
        }
        grn_memcpy(entry->tiny_entry.key.ptr, key, key_size);
        entry->tiny_entry.flag = 0;
      }
      entry->tiny_entry.hash_value = hash_value;
      entry->tiny_entry.key_size = key_size;
    }
  } else {
    if (hash->key_size == sizeof(uint32_t)) {
      *(uint32_t *)entry->plain_entry.key = hash_value;
    } else {
      entry->rich_entry.hash_value = hash_value;
      grn_memcpy(entry->rich_entry.key_and_value, key, key_size);
    }
  }
  return GRN_SUCCESS;
}

/*
 * grn_hash_entry_compare_key() returns GRN_TRUE if the entry key equals the
 * specified key, or GRN_FALSE otherwise.
 */
grn_inline static grn_bool
grn_hash_entry_compare_key(grn_ctx *ctx, grn_hash *hash,
                           grn_hash_entry *entry, uint32_t hash_value,
                           const void *key, unsigned int key_size)
{
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (entry->hash_value != hash_value ||
        entry->header.key_size != key_size) {
      return GRN_FALSE;
    }
    if (grn_hash_is_io_hash(hash)) {
      if (grn_hash_is_large_total_key_size(ctx, hash)) {
        if (entry->io_entry_large.flag & HASH_IMMEDIATE) {
          return !memcmp(key, entry->io_entry_large.key.buf, key_size);
        } else {
          const void * const entry_key_ptr =
              grn_io_hash_key_at(ctx, hash, entry->io_entry_large.key.offset);
          return !memcmp(key, entry_key_ptr, key_size);
        }
      } else {
        if (entry->io_entry_normal.flag & HASH_IMMEDIATE) {
          return !memcmp(key, entry->io_entry_normal.key.buf, key_size);
        } else {
          const void * const entry_key_ptr =
              grn_io_hash_key_at(ctx, hash, entry->io_entry_normal.key.offset);
          return !memcmp(key, entry_key_ptr, key_size);
        }
      }
    } else {
      if (entry->tiny_entry.flag & HASH_IMMEDIATE) {
        return !memcmp(key, entry->tiny_entry.key.buf, key_size);
      } else {
        return !memcmp(key, entry->tiny_entry.key.ptr, key_size);
      }
    }
  } else {
    if (entry->hash_value != hash_value) {
      return GRN_FALSE;
    }
    if (key_size == sizeof(uint32_t)) {
      return GRN_TRUE;
    } else {
      return !memcmp(key, entry->rich_entry.key_and_value, key_size);
    }
  }
}

grn_inline static char *
get_key(grn_ctx *ctx, grn_hash *hash, entry_str *n)
{
  return grn_hash_entry_get_key(ctx, hash, (grn_hash_entry *)n);
}

grn_inline static void *
get_value(grn_ctx *ctx, grn_hash *hash, entry_str *n)
{
  return grn_hash_entry_get_value(ctx, hash, (grn_hash_entry *)n);
}

#define GARBAGE (0xffffffff)

grn_inline static uint32_t
grn_io_hash_calculate_entry_size(uint32_t key_size, uint32_t value_size,
                                 uint32_t flags)
{
  if (flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (flags & GRN_OBJ_KEY_LARGE) {
      return (uintptr_t)((grn_io_hash_entry_large *)0)->value + value_size;
    } else {
      return (uintptr_t)((grn_io_hash_entry_normal *)0)->value + value_size;
    }
  } else {
    if (key_size == sizeof(uint32_t)) {
      return (uintptr_t)((grn_plain_hash_entry *)0)->value + value_size;
    } else {
      return (uintptr_t)((grn_rich_hash_entry *)0)->key_and_value
          + key_size + value_size;
    }
  }
}

static grn_io *
grn_io_hash_create_io(grn_ctx *ctx, const char *path,
                      uint32_t header_size, uint32_t entry_size,
                      uint32_t flags)
{
  uint32_t w_of_element = 0;
  grn_io_array_spec array_spec[GRN_HASH_N_SEGMENTS];

  while ((1U << w_of_element) < entry_size) {
    w_of_element++;
  }

  array_spec[GRN_HASH_KEY_SEGMENT].w_of_element = 0;
  if (flags & GRN_OBJ_KEY_LARGE) {
    array_spec[GRN_HASH_KEY_SEGMENT].max_n_segments =
      GRN_HASH_KEY_MAX_N_SEGMENTS_LARGE;
  } else {
    array_spec[GRN_HASH_KEY_SEGMENT].max_n_segments =
      GRN_HASH_KEY_MAX_N_SEGMENTS_NORMAL;
  }
  array_spec[GRN_HASH_ENTRY_SEGMENT].w_of_element = w_of_element;
  array_spec[GRN_HASH_ENTRY_SEGMENT].max_n_segments =
    1U << (30 - (22 - w_of_element));
  const uint32_t index_w_of_element = 2; /* log2(sizeof(grn_id)) */
  array_spec[GRN_HASH_INDEX_SEGMENT].w_of_element = index_w_of_element;
  array_spec[GRN_HASH_INDEX_SEGMENT].max_n_segments =
    1U << (30 - (22 - index_w_of_element));
  array_spec[GRN_HASH_BITMAP_SEGMENT].w_of_element = 0;
  array_spec[GRN_HASH_BITMAP_SEGMENT].max_n_segments = 1U << (30 - (22 + 3));
  return grn_io_create_with_array(ctx, path, header_size,
                                  GRN_HASH_SEGMENT_SIZE,
                                  GRN_IO_AUTO,
                                  GRN_HASH_N_SEGMENTS,
                                  array_spec);
}

static grn_rc
grn_io_hash_init(grn_ctx *ctx, grn_hash *hash, const char *path,
                 uint32_t key_size, uint32_t value_size, uint32_t flags,
                 grn_encoding encoding, uint32_t init_size)
{
  grn_io *io;
  grn_hash_header_common *header;
  uint32_t header_size, entry_size, max_offset;

  if (key_size <= GRN_HASH_MAX_KEY_SIZE_NORMAL) {
    header_size = GRN_HASH_HEADER_SIZE_NORMAL;
  } else {
    header_size = GRN_HASH_HEADER_SIZE_LARGE;
  }
  entry_size = grn_io_hash_calculate_entry_size(key_size, value_size, flags);

  io = grn_io_hash_create_io(ctx, path, header_size, entry_size, flags);
  if (!io) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_io_set_type(io, GRN_TABLE_HASH_KEY);

  max_offset = grn_hash_initial_max_offset;
  while (max_offset < init_size * 2) {
    max_offset *= 2;
  }
  max_offset--;

  if (encoding == GRN_ENC_DEFAULT) {
    encoding = ctx->encoding;
  }

  hash->key_size = key_size;

  header = grn_io_header(io);
  header->flags = flags;
  header->encoding = encoding;
  header->key_size = key_size;
  header->curr_rec = 0;
  header->curr_key_normal = 0;
  header->curr_key_large = 0;
  header->lock = 0;
  header->idx_offset = 0;
  header->value_size = value_size;
  header->entry_size = entry_size;
  header->max_offset = max_offset;
  header->n_entries = 0;
  header->n_garbages = 0;
  header->tokenizer = GRN_ID_NIL;
  grn_table_modules_init(ctx, &(hash->normalizers));
  if (header->flags & GRN_OBJ_KEY_NORMALIZE) {
    header->flags &= ~GRN_OBJ_KEY_NORMALIZE;
    header->normalizer = GRN_ID_NIL;
    grn_obj *normalizer = grn_ctx_get(ctx, GRN_NORMALIZER_AUTO_NAME, -1);
    grn_table_modules_add(ctx, &(hash->normalizers), normalizer);
  } else {
    header->normalizer = GRN_ID_NIL;
  }
  header->truncated = GRN_FALSE;
  grn_table_modules_init(ctx, &(hash->token_filters));
  GRN_PTR_INIT(&(hash->token_filter_procs), GRN_OBJ_VECTOR, GRN_ID_NIL);

  hash->obj.header.flags = (header->flags & GRN_OBJ_FLAGS_MASK);
  hash->ctx = ctx;
  hash->encoding = encoding;
  hash->value_size = value_size;
  hash->entry_size = entry_size;
  hash->n_garbages = &header->n_garbages;
  hash->n_entries = &header->n_entries;
  hash->max_offset = &header->max_offset;
  hash->io = io;
  hash->header.common = header;
  hash->lock = &header->lock;
  grn_table_module_init(ctx, &(hash->tokenizer), GRN_ID_NIL);
  return GRN_SUCCESS;
}

#define INITIAL_INDEX_SIZE 256U

static uint32_t
grn_tiny_hash_calculate_entry_size(uint32_t key_size, uint32_t value_size,
                                   uint32_t flags)
{
  uint32_t entry_size;
  if (flags & GRN_OBJ_KEY_VAR_SIZE) {
    entry_size = (uintptr_t)((grn_tiny_hash_entry *)0)->value + value_size;
  } else {
    if (key_size == sizeof(uint32_t)) {
      entry_size = (uintptr_t)((grn_plain_hash_entry *)0)->value + value_size;
    } else {
      entry_size = (uintptr_t)((grn_rich_hash_entry *)0)->key_and_value
          + key_size + value_size;
    }
  }
  if (entry_size != sizeof(uint32_t)) {
    entry_size += sizeof(uintptr_t) - 1;
    entry_size &= ~(sizeof(uintptr_t) - 1);
  }
  return entry_size;
}

static grn_rc
grn_tiny_hash_init(grn_ctx *ctx, grn_hash *hash, const char *path,
                   uint32_t key_size, uint32_t value_size, uint32_t flags,
                   grn_encoding encoding)
{
  uint32_t entry_size;

  if (path) {
    return GRN_INVALID_ARGUMENT;
  }
  hash->index = GRN_CTX_ALLOC(ctx, INITIAL_INDEX_SIZE * sizeof(grn_id));
  if (!hash->index) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  entry_size = grn_tiny_hash_calculate_entry_size(key_size, value_size, flags);
  hash->obj.header.flags = flags;
  hash->ctx = ctx;
  hash->key_size = key_size;
  hash->encoding = encoding;
  hash->value_size = value_size;
  hash->entry_size = entry_size;
  hash->n_garbages = &hash->n_garbages_;
  hash->n_entries = &hash->n_entries_;
  hash->max_offset = &hash->max_offset_;
  hash->max_offset_ = INITIAL_INDEX_SIZE - 1;
  hash->io = NULL;
  hash->header.common = NULL;
  hash->n_garbages_ = 0;
  hash->n_entries_ = 0;
  hash->garbages = GRN_ID_NIL;
  grn_table_module_init(ctx, &(hash->tokenizer), GRN_ID_NIL);
  grn_table_modules_init(ctx, &(hash->normalizers));
  grn_table_modules_init(ctx, &(hash->token_filters));
  GRN_PTR_INIT(&(hash->token_filter_procs), GRN_OBJ_VECTOR, GRN_ID_NIL);
  grn_tiny_array_init(ctx, &hash->a, entry_size, GRN_TINY_ARRAY_CLEAR);
  grn_tiny_bitmap_init(ctx, &hash->bitmap);
  return GRN_SUCCESS;
}

static grn_rc
grn_hash_init(grn_ctx *ctx, grn_hash *hash, const char *path,
              uint32_t key_size, uint32_t value_size, uint32_t flags)
{
  if (flags & GRN_HASH_TINY) {
    return grn_tiny_hash_init(ctx, hash, path, key_size, value_size,
                              flags, ctx->encoding);
  } else {
    return grn_io_hash_init(ctx, hash, path, key_size, value_size,
                            flags, ctx->encoding, 0);
  }
}

grn_hash *
grn_hash_create(grn_ctx *ctx, const char *path, uint32_t key_size, uint32_t value_size,
                uint32_t flags)
{
  grn_hash *hash;
  if (!ctx) {
    return NULL;
  }
  if (key_size > GRN_HASH_MAX_KEY_SIZE_LARGE) {
    return NULL;
  }
  hash = (grn_hash *)GRN_CALLOC(sizeof(grn_hash));
  if (!hash) {
    return NULL;
  }
  GRN_DB_OBJ_SET_TYPE(hash, GRN_TABLE_HASH_KEY);
  if (grn_hash_init(ctx, hash, path, key_size, value_size, flags)) {
    GRN_FREE(hash);
    return NULL;
  }
  return hash;
}

grn_hash *
grn_hash_open(grn_ctx *ctx, const char *path)
{
  if (ctx) {
    grn_io * const io = grn_io_open(ctx, path, GRN_IO_AUTO);
    if (io) {
      grn_hash_header_common * const header = grn_io_header(io);
      uint32_t io_type = grn_io_get_type(io);
      if (io_type == GRN_TABLE_HASH_KEY) {
        grn_hash * const hash = (grn_hash *)GRN_CALLOC(sizeof(grn_hash));
        if (hash) {
          if (!(header->flags & GRN_HASH_TINY)) {
            GRN_DB_OBJ_SET_TYPE(hash, GRN_TABLE_HASH_KEY);
            hash->ctx = ctx;
            hash->key_size = header->key_size;
            hash->encoding = header->encoding;
            hash->value_size = header->value_size;
            hash->entry_size = header->entry_size;
            hash->n_garbages = &header->n_garbages;
            hash->n_entries = &header->n_entries;
            hash->max_offset = &header->max_offset;
            hash->io = io;
            hash->header.common = header;
            hash->lock = &header->lock;
            grn_table_module_init(ctx, &(hash->tokenizer), header->tokenizer);
            grn_table_modules_init(ctx, &(hash->normalizers));
            if (header->flags & GRN_OBJ_KEY_NORMALIZE) {
              header->flags &= ~GRN_OBJ_KEY_NORMALIZE;
              header->normalizer = GRN_ID_NIL;
              grn_obj *normalizer =
                grn_ctx_get(ctx, GRN_NORMALIZER_AUTO_NAME, -1);
              grn_table_modules_add(ctx, &(hash->normalizers), normalizer);
            } else if (header->normalizer != GRN_ID_NIL) {
              grn_obj *normalizer = grn_ctx_at(ctx, header->normalizer);
              grn_table_modules_add(ctx, &(hash->normalizers), normalizer);
            }
            grn_table_modules_init(ctx, &(hash->token_filters));
            GRN_PTR_INIT(&(hash->token_filter_procs), GRN_OBJ_VECTOR, GRN_ID_NIL);
            hash->obj.header.flags = header->flags;
            return hash;
          } else {
            GRN_LOG(ctx, GRN_LOG_NOTICE,
                    "invalid hash flag. (%x)", header->flags);
          }
          GRN_FREE(hash);
        }
      } else {
        ERR(GRN_INVALID_FORMAT,
            "[table][hash] file type must be %#04x: <%#04x>",
            GRN_TABLE_HASH_KEY, io_type);
      }
      grn_io_close(ctx, io);
    }
  }
  return NULL;
}

/*
 * grn_hash_error_if_truncated() logs an error and returns its error code if
 * a hash is truncated by another process.
 * Otherwise, this function returns GRN_SUCCESS.
 * Note that `ctx` and `hash` must be valid.
 *
 * FIXME: A hash should be reopened if possible.
 */
static grn_rc
grn_hash_error_if_truncated(grn_ctx *ctx, grn_hash *hash)
{
  if (hash->header.common && hash->header.common->truncated) {
    ERR(GRN_FILE_CORRUPT,
        "hash is truncated, please unmap or reopen the database");
    return GRN_FILE_CORRUPT;
  }
  return GRN_SUCCESS;
}

static void
grn_hash_close_token_filters(grn_ctx *ctx, grn_hash *hash)
{
  grn_table_modules_fin(ctx, &(hash->token_filters));
  GRN_OBJ_FIN(ctx, &(hash->token_filter_procs));
}

static grn_rc
grn_io_hash_fin(grn_ctx *ctx, grn_hash *hash)
{
  grn_rc rc;

  if (hash->io->path[0] != '\0' &&
      grn_ctx_get_wal_role(ctx) == GRN_WAL_ROLE_PRIMARY) {
    grn_obj_flush(ctx, (grn_obj *)hash);
  }
  rc = grn_io_close(ctx, hash->io);
  grn_table_module_fin(ctx, &(hash->tokenizer));
  grn_table_modules_fin(ctx, &(hash->normalizers));
  grn_hash_close_token_filters(ctx, hash);
  return rc;
}

static grn_rc
grn_tiny_hash_fin(grn_ctx *ctx, grn_hash *hash)
{
  if (!hash->index) {
    return GRN_INVALID_ARGUMENT;
  }

  grn_table_module_fin(ctx, &(hash->tokenizer));
  grn_table_modules_fin(ctx, &(hash->normalizers));
  grn_hash_close_token_filters(ctx, hash);

  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    uint32_t num_remaining_entries = *hash->n_entries;
    grn_id *hash_ptr;
    for (hash_ptr = hash->index; num_remaining_entries; hash_ptr++) {
      const grn_id id = *hash_ptr;
      if (id && id != GARBAGE) {
        grn_tiny_hash_entry * const entry =
            (grn_tiny_hash_entry *)grn_tiny_array_get(&hash->a, id);
        GRN_ASSERT(entry);
        num_remaining_entries--;
        if (entry && !(entry->flag & HASH_IMMEDIATE)) {
          GRN_CTX_FREE(ctx, entry->key.ptr);
        }
      }
    }
  }
  grn_tiny_array_fin(&hash->a);
  grn_tiny_bitmap_fin(&hash->bitmap);
  GRN_CTX_FREE(ctx, hash->index);
  return GRN_SUCCESS;
}

static grn_rc
grn_hash_fin(grn_ctx *ctx, grn_hash *hash)
{
  if (grn_hash_is_io_hash(hash)) {
    return grn_io_hash_fin(ctx, hash);
  } else {
    GRN_ASSERT(ctx == hash->ctx);
    return grn_tiny_hash_fin(ctx, hash);
  }
}

grn_rc
grn_hash_close(grn_ctx *ctx, grn_hash *hash)
{
  grn_rc rc;
  if (!ctx || !hash) { return GRN_INVALID_ARGUMENT; }
  rc = grn_hash_fin(ctx, hash);
  GRN_FREE(hash);
  return rc;
}

grn_rc
grn_hash_remove(grn_ctx *ctx, const char *path)
{
  if (!ctx || !path) { return GRN_INVALID_ARGUMENT; }
  grn_rc wal_rc = grn_wal_remove(ctx, path, "[hash]");
  grn_rc io_rc = grn_io_remove(ctx, path);
  grn_rc rc = wal_rc;
  if (rc == GRN_SUCCESS) {
    rc = io_rc;
  }
  return rc;
}

grn_rc
grn_hash_truncate(grn_ctx *ctx, grn_hash *hash)
{
  grn_rc rc;
  char *path = NULL;
  uint32_t key_size, value_size, flags;

  if (!ctx || !hash) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_hash_error_if_truncated(ctx, hash);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  if (grn_hash_is_io_hash(hash)) {
    const char * const io_path = grn_io_path(hash->io);
    if (io_path && *io_path) {
      path = GRN_STRDUP(io_path);
      if (!path) {
        ERR(GRN_NO_MEMORY_AVAILABLE, "cannot duplicate path: <%s>", io_path);
        return GRN_NO_MEMORY_AVAILABLE;
      }
    }
  }
  key_size = hash->key_size;
  value_size = hash->value_size;
  flags = hash->obj.header.flags;

  if (grn_hash_is_io_hash(hash)) {
    if (path) {
      /* Only an I/O hash with a valid path uses the `truncated` flag. */
      hash->header.common->truncated = GRN_TRUE;
    }
    rc = grn_io_hash_fin(ctx, hash);
    if (!rc) {
      hash->io = NULL;
      if (path) {
        rc = grn_hash_remove(ctx, path);
      }
    }
  } else {
    rc = grn_tiny_hash_fin(ctx, hash);
  }
  if (rc == GRN_SUCCESS) {
    rc = grn_hash_init(ctx, hash, path, key_size, value_size, flags);
  }
  if (path) {
    GRN_FREE(path);
  }
  return rc;
}

typedef struct {
  grn_hash *hash;
  uint64_t wal_id;
  const char *tag;
  grn_wal_event event;
  grn_id record_id;
  const void *key;
  size_t key_size;
  uint32_t key_hash_value;
  uint64_t key_offset;
  uint32_t index_hash_value;
  const void *value;
  grn_id next_garbage_record_id;
  uint32_t n_entries;
  uint32_t n_garbages;
  uint32_t max_offset;
  uint32_t expected_n_entries;
} grn_hash_wal_add_entry_data;

typedef struct {
  bool record_id;
  bool key;
  bool key_size;
  bool key_hash_value;
  bool key_offset;
  bool index_hash_value;
  bool value;
  bool next_garbage_record_id;
  bool n_garbages;
  bool n_entries;
  bool max_offset;
  bool expected_n_entries;
} grn_hash_wal_add_entry_used;

static grn_rc
grn_hash_wal_add_entry_add_entry(grn_ctx *ctx,
                                 grn_hash_wal_add_entry_data *data,
                                 grn_hash_wal_add_entry_used *used)
{
  used->record_id = true;
  used->index_hash_value = true;
  used->n_entries = true;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_INDEX_HASH_VALUE,
                           GRN_WAL_VALUE_UINT32,
                           data->index_hash_value,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_hash_wal_add_entry_reuse_entry(grn_ctx *ctx,
                                   grn_hash_wal_add_entry_data *data,
                                   grn_hash_wal_add_entry_used *used)
{
  used->record_id = true;
  used->key_size = true;
  used->index_hash_value = true;
  used->n_garbages = true;
  used->n_entries = true;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_KEY_SIZE,
                           GRN_WAL_VALUE_UINT32,
                           data->key_size,

                           GRN_WAL_KEY_INDEX_HASH_VALUE,
                           GRN_WAL_VALUE_UINT32,
                           data->index_hash_value,

                           GRN_WAL_KEY_N_GARBAGES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_garbages,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_hash_wal_add_entry_reset_entry(grn_ctx *ctx,
                                   grn_hash_wal_add_entry_data *data,
                                   grn_hash_wal_add_entry_used *used)
{
  used->record_id = true;
  used->key_size = true;
  used->next_garbage_record_id = true;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_KEY_SIZE,
                           GRN_WAL_VALUE_UINT32,
                           data->key_size,

                           GRN_WAL_KEY_NEXT_GARBAGE_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->next_garbage_record_id,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_hash_wal_add_entry_enable_entry(grn_ctx *ctx,
                                    grn_hash_wal_add_entry_data *data,
                                    grn_hash_wal_add_entry_used *used)
{
  used->record_id = true;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_hash_wal_add_entry_set_entry_key(grn_ctx *ctx,
                                     grn_hash_wal_add_entry_data *data,
                                     grn_hash_wal_add_entry_used *used)
{
  used->record_id = true;
  used->key = true;
  used->key_size = true;
  used->key_hash_value = true;
  used->key_offset = true;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_KEY,
                           GRN_WAL_VALUE_BINARY,
                           data->key,
                           data->key_size,

                           GRN_WAL_KEY_KEY_HASH_VALUE,
                           GRN_WAL_VALUE_UINT32,
                           data->key_hash_value,

                           GRN_WAL_KEY_KEY_OFFSET,
                           GRN_WAL_VALUE_UINT64,
                           data->key_offset,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_hash_wal_add_entry_delete_entry(grn_ctx *ctx,
                                    grn_hash_wal_add_entry_data *data,
                                    grn_hash_wal_add_entry_used *used)
{
  used->record_id = true;
  used->key_size = true;
  used->index_hash_value = true;
  used->n_garbages = true;
  used->n_entries = true;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_KEY_SIZE,
                           GRN_WAL_VALUE_UINT32,
                           data->key_size,

                           GRN_WAL_KEY_INDEX_HASH_VALUE,
                           GRN_WAL_VALUE_UINT32,
                           data->index_hash_value,

                           GRN_WAL_KEY_N_GARBAGES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_garbages,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_hash_wal_add_entry_rehash(grn_ctx *ctx,
                              grn_hash_wal_add_entry_data *data,
                              grn_hash_wal_add_entry_used *used)
{
  used->n_entries = true;
  used->max_offset = true;
  used->expected_n_entries = true;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->n_entries,

                           GRN_WAL_KEY_MAX_OFFSET,
                           GRN_WAL_VALUE_UINT32,
                           data->max_offset,

                           GRN_WAL_KEY_EXPECTED_N_ENTRIES,
                           GRN_WAL_VALUE_UINT32,
                           data->expected_n_entries,

                           GRN_WAL_KEY_END);
}

static grn_rc
grn_hash_wal_add_entry_set_value(grn_ctx *ctx,
                                 grn_hash_wal_add_entry_data *data,
                                 grn_hash_wal_add_entry_used *used)
{
  used->record_id = true;
  used->value = true;
  size_t value_size = data->hash->value_size;
  return grn_wal_add_entry(ctx,
                           (grn_obj *)(data->hash),
                           false,
                           &(data->wal_id),
                           data->tag,

                           GRN_WAL_KEY_EVENT,
                           GRN_WAL_VALUE_EVENT,
                           data->event,

                           GRN_WAL_KEY_RECORD_ID,
                           GRN_WAL_VALUE_RECORD_ID,
                           data->record_id,

                           GRN_WAL_KEY_VALUE,
                           GRN_WAL_VALUE_BINARY,
                           data->value,
                           value_size,

                           GRN_WAL_KEY_END);
}

static void
grn_hash_wal_add_entry_format_deatils(grn_ctx *ctx,
                                      grn_hash_wal_add_entry_data *data,
                                      grn_hash_wal_add_entry_used *used,
                                      grn_obj *details)
{
  grn_text_printf(ctx,
                  details,
                  "event:%u<%s> ",
                  data->event,
                  grn_wal_event_to_string(data->event));
  if (used->record_id) {
    grn_text_printf(ctx, details, "record-id:%u ", data->record_id);
  }
  if (used->key_size) {
    grn_text_printf(ctx, details, "key-size:%" GRN_FMT_SIZE " ", data->key_size);
  }
  if (used->key_hash_value) {
    grn_text_printf(ctx, details, "key-hash-value:%u ", data->key_hash_value);
  }
  if (used->key_offset) {
    grn_text_printf(ctx,
                    details,
                    "key-offset:%" GRN_FMT_INT64U " ",
                    data->key_offset);
  }
  if (used->index_hash_value) {
    grn_text_printf(ctx,
                    details,
                    "index-hash-value:%u ",
                    data->index_hash_value);
  }
  if (used->value) {
    grn_text_printf(ctx, details, "value-size:%u ", data->hash->value_size);
  }
  if (used->next_garbage_record_id) {
    grn_text_printf(ctx,
                    details,
                    "next-garbage-record-id:%u ",
                    data->next_garbage_record_id);
  }
  if (used->n_garbages) {
    grn_text_printf(ctx, details, "n-garbages:%u ", data->n_garbages);
  }
  if (used->n_entries) {
    grn_text_printf(ctx, details, "n-entries:%u ", data->n_entries);
  }
  if (used->max_offset) {
    grn_text_printf(ctx, details, "max-offset:%u ", data->max_offset);
  }
  if (used->expected_n_entries) {
    grn_text_printf(ctx,
                    details,
                    "expected-n-entries:%u ",
                    data->expected_n_entries);
  }
}

static grn_rc
grn_hash_wal_add_entry(grn_ctx *ctx, grn_hash_wal_add_entry_data *data)
{
  if (grn_ctx_get_wal_role(ctx) == GRN_WAL_ROLE_NONE) {
    return GRN_SUCCESS;
  }

  if (!data->hash->io) {
    return GRN_SUCCESS;
  }
  if (data->hash->io->path[0] == '\0') {
    return GRN_SUCCESS;
  }

  grn_rc rc = GRN_SUCCESS;
  const char *usage = "";
  grn_hash_wal_add_entry_used used = {0};

  switch (data->event) {
  case GRN_WAL_EVENT_ADD_ENTRY :
    usage = "adding entry";
    rc = grn_hash_wal_add_entry_add_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_REUSE_ENTRY :
    usage = "reusing entry";
    rc = grn_hash_wal_add_entry_reuse_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_RESET_ENTRY :
    usage = "resetting entry";
    rc = grn_hash_wal_add_entry_reset_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_ENABLE_ENTRY :
    usage = "enabling entry";
    rc = grn_hash_wal_add_entry_enable_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_SET_ENTRY_KEY :
    usage = "setting entry key";
    rc = grn_hash_wal_add_entry_set_entry_key(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_DELETE_ENTRY :
    usage = "deleting entry";
    rc = grn_hash_wal_add_entry_delete_entry(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_REHASH :
    usage = "rehash";
    rc = grn_hash_wal_add_entry_rehash(ctx, data, &used);
    break;
  case GRN_WAL_EVENT_SET_VALUE :
    usage = "setting value";
    rc = grn_hash_wal_add_entry_set_value(ctx, data, &used);
    break;
  default :
    usage = "not implemented event";
    rc = GRN_FUNCTION_NOT_IMPLEMENTED;
    break;
  }

  if (rc != GRN_SUCCESS) {
    grn_obj details;
    GRN_TEXT_INIT(&details, 0);
    grn_hash_wal_add_entry_format_deatils(ctx, data, &used, &details);
    grn_obj_set_error(ctx,
                      (grn_obj *)(data->hash),
                      rc,
                      data->record_id,
                      data->tag,
                      "failed to add WAL entry for %s: %.*s",
                      usage,
                      (int)GRN_TEXT_LEN(&details),
                      GRN_TEXT_VALUE(&details));
    GRN_OBJ_FIN(ctx, &details);
  }

  return rc;
}

grn_inline static void
grn_hash_wal_set_wal_id(grn_ctx *ctx, grn_hash *hash, uint64_t wal_id)
{
  if (!grn_hash_is_io_hash(hash)) {
    return;
  }
  hash->header.common->wal_id = wal_id;
}

grn_inline static uint32_t
grn_hash_calculate_hash_value(const void *ptr, uint32_t size)
{
  uint32_t i;
  uint32_t hash_value = 0;
  for (i = 0; i < size; i++) {
    hash_value = (hash_value * 1021) + ((const uint8_t *)ptr)[i];
  }
  return hash_value;
}

grn_inline static uint32_t
grn_hash_calculate_step(uint32_t hash_value)
{
  return (hash_value >> 2) | 0x1010101;
}

static grn_rc
grn_hash_rehash(grn_ctx *ctx, grn_hash *hash, uint32_t expected_n_entries)
{
  grn_id *new_index = NULL;
  uint32_t new_index_size = INITIAL_INDEX_SIZE;
  grn_id *src_ptr = NULL, *dest_ptr = NULL;
  uint32_t src_offset = 0, dest_offset = 0;
  const uint32_t n_entries = *hash->n_entries;
  const uint32_t max_offset = *hash->max_offset;

  if (expected_n_entries == 0) {
    expected_n_entries = n_entries * 2;
    if (grn_id_maybe_table(ctx, hash->obj.header.domain)) {
      grn_obj *domain = grn_ctx_at(ctx, hash->obj.header.domain);
      if (grn_obj_is_table(ctx, domain)) {
        const unsigned int n_source_records = grn_table_size(ctx, domain);
        if (n_source_records + 1 > expected_n_entries) {
          expected_n_entries = n_source_records + 1;
        }
      }
      grn_obj_unref(ctx, domain);
    }
  }
  if (expected_n_entries > INT_MAX) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  while (new_index_size <= expected_n_entries) {
    new_index_size *= 2;
  }

  {
    grn_log_level log_level;
    if (grn_hash_is_io_hash(hash)) {
      log_level = GRN_LOG_INFO;
    } else {
      log_level = GRN_LOG_DEBUG;
    }
    if (grn_logger_pass(ctx, log_level)) {
      GRN_DEFINE_NAME(hash);
      GRN_LOG(ctx, log_level,
              "[hash][rehash][%.*s] <%u> -> <%u>: "
              "max-offset:<%u> "
              "n-garbages:<%u>",
              name_size, name,
              n_entries,
              new_index_size,
              max_offset,
              *hash->n_garbages);
    }
  }

  if (grn_hash_is_io_hash(hash)) {
    uint32_t i;
    src_offset = hash->header.common->idx_offset;
    dest_offset = MAX_INDEX_SIZE - src_offset;
    for (i = 0; i < new_index_size; i += (IDX_MASK_IN_A_SEGMENT + 1)) {
      /*
       * The following grn_io_hash_idx_at() allocates memory for a new segment
       * and returns a pointer to the new segment. It's actually bad manners
       * but faster than calling grn_io_hash_idx_at() for each element.
       */
      dest_ptr = grn_io_hash_idx_at(ctx, hash, i + dest_offset);
      if (!dest_ptr) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      memset(dest_ptr, 0, GRN_HASH_SEGMENT_SIZE);
    }
  } else {
    GRN_ASSERT(ctx == hash->ctx);
    new_index = GRN_CTX_ALLOC(ctx, new_index_size * sizeof(grn_id));
    if (!new_index) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    src_ptr = hash->index;
  }

  {
    uint32_t src_pos, count;
    const uint32_t new_max_offset = new_index_size - 1;
    for (count = 0, src_pos = 0; count < n_entries && src_pos <= max_offset;
         src_pos++, src_ptr++) {
      uint32_t i, step;
      grn_id entry_id;
      grn_hash_entry *entry;
      if (grn_hash_is_io_hash(hash) && !(src_pos & IDX_MASK_IN_A_SEGMENT)) {
        src_ptr = grn_io_hash_idx_at(ctx, hash, src_pos + src_offset);
        if (!src_ptr) {
          return GRN_NO_MEMORY_AVAILABLE;
        }
      }
      entry_id = *src_ptr;
      if (!entry_id || (entry_id == GARBAGE)) {
        continue;
      }
      entry = grn_hash_entry_at(ctx, hash, entry_id, GRN_TABLE_ADD);
      if (!entry) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      step = grn_hash_calculate_step(entry->hash_value);
      for (i = entry->hash_value; ; i += step) {
        i &= new_max_offset;
        if (grn_hash_is_io_hash(hash)) {
          dest_ptr = grn_io_hash_idx_at(ctx, hash, i + dest_offset);
          if (!dest_ptr) {
            return GRN_NO_MEMORY_AVAILABLE;
          }
        } else {
          dest_ptr = new_index + i;
        }
        if (!*dest_ptr) {
          break;
        }
      }
      *dest_ptr = entry_id;
      count++;
    }
    *hash->max_offset = new_max_offset;
    *hash->n_garbages = 0;
    if (grn_hash_is_io_hash(hash)) {
      if (GRN_HASH_IS_LARGE_KEY(hash)) {
        grn_id *garbages = hash->header.large->garbages;
        memset(garbages, 0, GRN_HASH_MAX_KEY_SIZE_LARGE);
      } else {
        grn_id *garbages = hash->header.normal->garbages;
        memset(garbages, 0, GRN_HASH_MAX_KEY_SIZE_NORMAL);
      }
    } else {
      hash->garbages = GRN_ID_NIL;
    }
  }

  if (grn_hash_is_io_hash(hash)) {
    hash->header.common->idx_offset = dest_offset;
  } else {
    grn_id * const old_index = hash->index;
    hash->index = new_index;
    GRN_CTX_FREE(ctx, old_index);
  }

  return GRN_SUCCESS;
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
      grn_nanosleep(GRN_LOCK_WAIT_TIME_NANOSECOND);
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

uint32_t
grn_hash_size(grn_ctx *ctx, grn_hash *hash)
{
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return 0;
  }
  return *hash->n_entries;
}

grn_inline static grn_hash_entry *
grn_io_hash_reuse_entry(grn_ctx *ctx,
                        grn_hash *hash,
                        grn_id id,
                        uint32_t key_size,
                        grn_id *index,
                        const char *tag)
{
  grn_hash_entry *entry = grn_io_hash_entry_at(ctx, hash, id, GRN_TABLE_ADD);
  if (!entry) {
    grn_obj_set_error(ctx,
                      (grn_obj *)hash,
                      GRN_INVALID_ARGUMENT,
                      id,
                      tag,
                      "failed to reuse entry");
    return NULL;
  }
  *index = id;
  (*(hash->n_garbages))--;
  (*(hash->n_entries))++;
  return entry;
}

grn_inline static void
grn_io_hash_reset_entry(grn_ctx *ctx,
                        grn_hash *hash,
                        grn_hash_entry *entry,
                        grn_id next_garbage_id,
                        uint32_t key_size,
                        const char *tag)
{
  grn_hash_header_common * const header = hash->header.common;
  grn_id *garbages;
  if (GRN_HASH_IS_LARGE_KEY(hash)) {
    garbages = hash->header.large->garbages;
  } else {
    garbages = hash->header.normal->garbages;
  }
  uint32_t garbage_index = key_size - 1;
  garbages[garbage_index] = next_garbage_id;
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    /* keep entry->io_entry's hash_value, flag, key_size and key. */
    if (grn_hash_is_large_total_key_size(ctx, hash)) {
      memset(entry->io_entry_large.value, 0, header->value_size);
    } else {
      memset(entry->io_entry_normal.value, 0, header->value_size);
    }
  } else {
    memset(entry, 0, header->entry_size);
  }
}

grn_inline static grn_hash_entry *
grn_io_hash_add_entry(grn_ctx *ctx,
                      grn_hash *hash,
                      grn_id id,
                      grn_id *index,
                      const char *tag)
{
  grn_hash_entry *entry = grn_io_hash_entry_at(ctx, hash, id, GRN_TABLE_ADD);
  if (!entry) {
    grn_obj_set_error(ctx,
                      (grn_obj *)hash,
                      GRN_INVALID_ARGUMENT,
                      id,
                      tag,
                      "failed to add entry");
    return NULL;
  }
  hash->header.common->curr_rec = id;
  *index = id;
  (*(hash->n_entries))++;
  return entry;
}

grn_inline static bool
grn_io_hash_enable_entry(grn_ctx *ctx,
                         grn_hash *hash,
                         grn_id id,
                         const char *tag)
{
  if (!grn_io_array_bit_on(ctx, hash->io, GRN_HASH_BITMAP_SEGMENT, id)) {
    grn_obj_set_error(ctx,
                      (grn_obj *)hash,
                      GRN_INVALID_ARGUMENT,
                      id,
                      tag,
                      "failed to set a bit for the entry");
    return false;
  }
  return true;
}

grn_inline static grn_id
grn_io_hash_add(grn_ctx *ctx,
                grn_hash *hash,
                uint32_t hash_value,
                const void *key,
                uint32_t key_size,
                void **value,
                grn_id *index,
                uint32_t index_hash_value)
{
  grn_hash_header_common * const header = hash->header.common;
  grn_hash_wal_add_entry_data wal_data = {0};
  wal_data.hash = hash;
  wal_data.tag = "[hash][io][add]";
  wal_data.key = key;
  wal_data.key_size = key_size;
  wal_data.key_hash_value = hash_value;
  wal_data.index_hash_value = index_hash_value;

  grn_id *garbages;
  if (GRN_HASH_IS_LARGE_KEY(hash)) {
    garbages = hash->header.large->garbages;
  } else {
    garbages = hash->header.normal->garbages;
  }

  grn_hash_entry *entry;
  uint32_t garbage_index = key_size - 1;
  grn_id garbage_id = garbages[garbage_index];
  if (garbage_id != GRN_ID_NIL) {
    wal_data.event = GRN_WAL_EVENT_REUSE_ENTRY;
    wal_data.record_id = garbage_id;
    wal_data.n_garbages = *(hash->n_garbages);
    wal_data.n_entries = *(hash->n_entries);
    if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
      return GRN_ID_NIL;
    }
    entry = grn_io_hash_reuse_entry(ctx,
                                    hash,
                                    wal_data.record_id,
                                    wal_data.key_size,
                                    index,
                                    wal_data.tag);
    if (!entry) {
      return GRN_ID_NIL;
    }
    wal_data.event = GRN_WAL_EVENT_RESET_ENTRY;
    wal_data.next_garbage_record_id = *((grn_id *)entry);
    if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
      return GRN_ID_NIL;
    }
    grn_io_hash_reset_entry(ctx,
                            hash,
                            entry,
                            wal_data.next_garbage_record_id,
                            wal_data.key_size,
                            wal_data.tag);
  } else {
    wal_data.event = GRN_WAL_EVENT_ADD_ENTRY;
    wal_data.record_id = header->curr_rec + 1;
    wal_data.n_entries = *(hash->n_entries);
    if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
      return GRN_ID_NIL;
    }
    entry = grn_io_hash_add_entry(ctx,
                                  hash,
                                  wal_data.record_id,
                                  index,
                                  wal_data.tag);
    if (!entry) {
      return GRN_ID_NIL;
    }
  }

  wal_data.event = GRN_WAL_EVENT_ENABLE_ENTRY;
  if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  if (!grn_io_hash_enable_entry(ctx, hash, wal_data.record_id, wal_data.tag)) {
    return GRN_ID_NIL;
  }

  wal_data.event = GRN_WAL_EVENT_SET_ENTRY_KEY;
  if (grn_hash_is_large_total_key_size(ctx, hash)) {
    wal_data.key_offset = header->curr_key_large;
  } else {
    wal_data.key_offset = header->curr_key_normal;
  }
  if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    goto exit;
  }
  grn_rc rc = grn_hash_entry_put_key(ctx,
                                     hash,
                                     entry,
                                     hash_value,
                                     key,
                                     key_size);
  if (rc != GRN_SUCCESS) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect_key(ctx, &inspected, (grn_obj *)hash, key, key_size);
    grn_obj_set_error(ctx,
                      (grn_obj *)hash,
                      rc,
                      wal_data.record_id,
                      wal_data.tag,
                      "failed to put key: <%.*s>",
                      (int)GRN_TEXT_LEN(&inspected),
                      GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    goto exit;
  }

  if (value) {
    *value = grn_hash_entry_get_value(ctx, hash, entry);
  }

exit :
  if (ctx->rc != GRN_SUCCESS && entry) {
    rc = grn_hash_delete_by_id(ctx, hash, wal_data.record_id, NULL);
    if (rc != GRN_SUCCESS) {
      grn_obj_set_error(ctx,
                        (grn_obj *)hash,
                        rc,
                        wal_data.record_id,
                        wal_data.tag,
                        "failed to delete by id");
    }
    return GRN_ID_NIL;
  }

  return wal_data.record_id;
}

grn_inline static grn_id
grn_tiny_hash_add(grn_ctx *ctx,
                  grn_hash *hash,
                  uint32_t hash_value,
                  const void *key,
                  unsigned int key_size,
                  void **value,
                  grn_id *index)
{
  grn_id entry_id;
  grn_hash_entry *entry;
  if (hash->garbages) {
    entry_id = hash->garbages;
    entry = (grn_hash_entry *)grn_tiny_array_get(&hash->a, entry_id);
    hash->garbages = *(grn_id *)entry;
    (*hash->n_garbages)--;
    memset(entry, 0, hash->entry_size);
  } else {
    entry_id = hash->a.max + 1;
    entry = (grn_hash_entry *)grn_tiny_array_put(&hash->a, entry_id);
    if (!entry) {
      return GRN_ID_NIL;
    }
  }
  *index = entry_id;
  (*hash->n_entries)++;

  if (!grn_tiny_bitmap_put_and_set(&hash->bitmap, entry_id, 1)) {
    /* TODO: error handling. */
  }

  if (grn_hash_entry_put_key(ctx, hash, entry, hash_value, key, key_size)) {
    /* TODO: error handling. */
  }

  if (value) {
    *value = grn_hash_entry_get_value(ctx, hash, entry);
  }
  return entry_id;
}

grn_inline static grn_rc
grn_hash_ensure_rehash(grn_ctx *ctx,
                       grn_hash *hash,
                       uint32_t threshold,
                       const char *tag)
{
  if (threshold < *hash->max_offset) {
    return GRN_SUCCESS;
  }

  if (*hash->max_offset > (1 << 29)) {
    GRN_DEFINE_NAME(hash);
    ERR(GRN_TOO_LARGE_OFFSET,
        "%s[%.*s] hash table size limit",
        tag,
        name_size, name);
    return ctx->rc;
  }

  grn_hash_wal_add_entry_data wal_data = {0};
  wal_data.hash = hash;
  wal_data.tag = tag;
  wal_data.event = GRN_WAL_EVENT_REHASH;
  wal_data.n_entries = *(hash->n_entries);
  wal_data.max_offset = *(hash->max_offset);
  wal_data.expected_n_entries = 0;
  if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return ctx->rc;
  }
  grn_rc rc = grn_hash_rehash(ctx, hash, wal_data.expected_n_entries);
  if (rc != GRN_SUCCESS) {
    grn_obj_set_error(ctx,
                      (grn_obj *)hash,
                      rc,
                      GRN_ID_NIL,
                      tag,
                      "failed to rehash: "
                      "n-entries:%u "
                      "max-offset:%u",
                      wal_data.n_entries,
                      wal_data.max_offset);
    return rc;
  }
  grn_hash_wal_set_wal_id(ctx, hash, wal_data.wal_id);

  return GRN_SUCCESS;
}

grn_inline static grn_id
grn_hash_add_entry(grn_ctx *ctx,
                   grn_hash *hash,
                   uint32_t hash_value,
                   const void *key,
                   uint32_t key_size,
                   void **value,
                   int *added,
                   bool is_record,
                   const char *tag)
{
  uint32_t i;
  const uint32_t step = grn_hash_calculate_step(hash_value);
  grn_id id, *index, *garbage_index = NULL;
  uint32_t index_hash_value = 0;
  uint32_t garbage_index_hash_value = 0;
  for (i = hash_value; ; i += step) {
    index = grn_hash_idx_at(ctx, hash, i);
    if (!index) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] failed to detect index: <%u>",
          tag,
          name_size, name,
          i);
      return GRN_ID_NIL;
    }
    index_hash_value = i;
    id = *index;
    if (id == GRN_ID_NIL) {
      break;
    }
    if (id == GARBAGE) {
      if (!garbage_index) {
        garbage_index = index;
        garbage_index_hash_value = index_hash_value;
      }
      continue;
    }

    grn_hash_entry *entry = grn_hash_entry_at(ctx, hash, id, GRN_TABLE_ADD);
    if (!entry) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%*.s] failed to find an entry: <%u>",
          tag,
          name_size, name,
          id);
      return GRN_ID_NIL;
    }
    if (grn_hash_entry_compare_key(ctx,
                                   hash,
                                   entry,
                                   hash_value,
                                   key,
                                   key_size)) {
      if (value) {
        *value = grn_hash_entry_get_value(ctx, hash, entry);
      }
      if (added) {
        *added = 0;
      }
      return id;
    }
  }

  grn_id *target_index;
  uint32_t target_index_hash_value;
  if (garbage_index) {
    target_index = garbage_index;
    target_index_hash_value = garbage_index_hash_value;
  } else {
    target_index = index;
    target_index_hash_value = index_hash_value;
  }
  if (grn_hash_is_io_hash(hash)) {
    id = grn_io_hash_add(ctx,
                         hash,
                         hash_value,
                         key,
                         key_size,
                         value,
                         target_index,
                         target_index_hash_value);
  } else {
    id = grn_tiny_hash_add(ctx,
                           hash,
                           hash_value,
                           key,
                           key_size,
                           value,
                           target_index);
  }
  if (id == GRN_ID_NIL) {
    GRN_DEFINE_NAME(hash);
    if (is_record) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] failed to add: <%u>",
          tag,
          name_size, name,
          *((grn_id *)key));
    } else {
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] failed to add: <%.*s>",
          tag,
          name_size, name,
          (int)key_size, (const char *)key);
    }
    return GRN_ID_NIL;
  }
  if (added) {
    *added = 1;
  }
  return id;
}

grn_id
grn_hash_add(grn_ctx *ctx, grn_hash *hash, const void *key,
             uint32_t key_size, void **value, int *added)
{
  const char *tag = "[hash][add]";
  uint32_t hash_value;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  if (!key) {
    GRN_DEFINE_NAME(hash);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] key must not NULL",
        tag,
        name_size, name);
    return GRN_ID_NIL;
  }
  if (!key_size) {
    GRN_DEFINE_NAME(hash);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] key size must not zero",
        tag,
        name_size, name);
    return GRN_ID_NIL;
  }
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (key_size > hash->key_size) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] too long key",
          tag,
          name_size, name);
      return GRN_ID_NIL;
    }
    hash_value = grn_hash_calculate_hash_value(key, key_size);
  } else {
    if (key_size != hash->key_size) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%*.s] key size unmatch",
          tag,
          name_size, name);
      return GRN_ID_NIL;
    }
    if (key_size == sizeof(uint32_t)) {
      hash_value = *((uint32_t *)key);
    } else {
      hash_value = grn_hash_calculate_hash_value(key, key_size);
    }
  }

  /* lock */
  uint32_t reset_threshold = *hash->n_entries + *hash->n_garbages;
  if (!grn_id_maybe_table(ctx, hash->obj.header.domain)) {
    reset_threshold *= 2;
  }
  grn_rc rc = grn_hash_ensure_rehash(ctx, hash, reset_threshold, tag);
  if (rc != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }

  grn_id id = grn_hash_add_entry(ctx,
                                 hash,
                                 hash_value,
                                 key,
                                 key_size,
                                 value,
                                 added,
                                 false,
                                 tag);
  /* unlock */
  return id;
}

grn_inline static grn_id
grn_hash_get_internal(grn_ctx *ctx,
                      grn_hash *hash,
                      uint32_t hash_value,
                      const void *key,
                      unsigned int key_size,
                      void **value)
{
  uint32_t i;
  const uint32_t step = grn_hash_calculate_step(hash_value);
  for (i = hash_value; ; i += step) {
    grn_id id;
    grn_id * const index = grn_hash_idx_at(ctx, hash, i);
    if (!index) {
      return GRN_ID_NIL;
    }
    id = *index;
    if (!id) {
      return GRN_ID_NIL;
    }
    if (id != GARBAGE) {
      grn_hash_entry * const entry = grn_hash_entry_at(ctx, hash, id, 0);
      if (entry) {
        if (grn_hash_entry_compare_key(ctx, hash, entry, hash_value,
                                       key, key_size)) {
          if (value) {
            *value = grn_hash_entry_get_value(ctx, hash, entry);
          }
          return id;
        }
      }
    }
  }
}

grn_id
grn_hash_get(grn_ctx *ctx, grn_hash *hash, const void *key,
             unsigned int key_size, void **value)
{
  uint32_t hash_value;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (key_size > hash->key_size) {
      return GRN_ID_NIL;
    }
    hash_value = grn_hash_calculate_hash_value(key, key_size);
  } else {
    if (key_size != hash->key_size) {
      return GRN_ID_NIL;
    }
    if (key_size == sizeof(uint32_t)) {
      hash_value = *((uint32_t *)key);
    } else {
      hash_value = grn_hash_calculate_hash_value(key, key_size);
    }
  }

  return grn_hash_get_internal(ctx, hash, hash_value, key, key_size, value);
}

static grn_rc
grn_hash_add_records_validate(grn_ctx *ctx,
                              grn_hash *hash,
                              grn_operator op,
                              const char *tag)
{
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return ctx->rc;
  }
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    GRN_DEFINE_NAME(hash);
    ERR(GRN_INVALID_ARGUMENT,
        "%s[%.*s] must not be variable key size",
        tag,
        name_size, name);
    return ctx->rc;
  } else {
    const uint32_t key_size = sizeof(grn_id);
    if (hash->key_size != key_size) {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] key size unmatch: <%u> != <%u>",
          tag,
          name_size, name,
          hash->key_size,
          key_size);
      return ctx->rc;
    }
  }
  switch (op) {
  case GRN_OP_AND :
  case GRN_OP_OR :
    break;
  default :
    {
      GRN_DEFINE_NAME(hash);
      ERR(GRN_INVALID_ARGUMENT,
          "%s[%.*s] operator must be GRN_OP_AND or GRN_OP_OR: <%s>",
          tag,
          name_size, name,
          grn_operator_to_string(op));
      return ctx->rc;
    }
  }

  return GRN_SUCCESS;
}

grn_inline static grn_rc
grn_hash_add_record(grn_ctx *ctx,
                    grn_hash *hash,
                    grn_posting_internal *posting,
                    grn_operator op,
                    const char *tag)
{
  const grn_id key = posting->rid;
  const uint32_t key_size = sizeof(grn_id);
  const uint32_t hash_value = key;
  void *value = NULL;
  grn_id id;
  if (op == GRN_OP_OR) {
    id = grn_hash_add_entry(ctx,
                            hash,
                            hash_value,
                            &key,
                            key_size,
                            &value,
                            NULL,
                            true,
                            tag);
  } else {
    id = grn_hash_get_internal(ctx,
                               hash,
                               hash_value,
                               &key,
                               key_size,
                               &value);
  }
  if (id == GRN_ID_NIL) {
    return ctx->rc;
  }

  if (hash->obj.header.flags & GRN_OBJ_WITH_SUBREC) {
    grn_rset_recinfo *recinfo = value;
    if (op == GRN_OP_AND) {
      recinfo->n_subrecs |= GRN_RSET_UTIL_BIT;
    }
    grn_rset_posinfo posinfo = {
      posting->rid,
      posting->sid,
      posting->pos,
    };
    float weight = posting->weight_float * posting->scale;
    grn_table_add_subrec(ctx,
                         (grn_obj *)hash,
                         recinfo,
                         weight,
                         &posinfo,
                         1);
    grn_selector_data_current_add_score(ctx,
                                        (grn_obj *)hash,
                                        id,
                                        posinfo.rid,
                                        weight);
  }
  return ctx->rc;
}

grn_rc
grn_hash_add_table_cursor(grn_ctx *ctx,
                          grn_hash *hash,
                          grn_table_cursor *cursor,
                          double score,
                          grn_operator op)
{
  const char *tag = "[hash][add-table-cursor]";
  grn_rc rc = grn_hash_add_records_validate(ctx, hash, op, tag);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  /* lock */
  if (op == GRN_OP_OR) {
    grn_obj *table = grn_table_cursor_table(ctx, cursor);
    const uint32_t reset_threshold = grn_table_size(ctx, table);
    rc = grn_hash_ensure_rehash(ctx, hash, reset_threshold, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  grn_posting_internal posting = {0};
  posting.weight_float = score;
  posting.scale = 1.0;
  while ((posting.rid = grn_table_cursor_next(ctx, cursor))) {
    rc = grn_hash_add_record(ctx, hash, &posting, op, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  /* unlock */

  return ctx->rc;
}

grn_rc
grn_hash_add_index_cursor(grn_ctx *ctx,
                          grn_hash *hash,
                          grn_obj *cursor,
                          double additional_score,
                          double weight,
                          grn_operator op)
{
  const char *tag = "[hash][add-index-cursor]";
  grn_rc rc = grn_hash_add_records_validate(ctx, hash, op, tag);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  /* lock */
  if (op == GRN_OP_OR) {
    grn_obj *index_column = grn_index_cursor_get_index_column(ctx, cursor);
    grn_obj *table = grn_ctx_at(ctx, DB_OBJ(index_column)->range);
    const uint32_t reset_threshold = grn_table_size(ctx, table);
    grn_obj_unref(ctx, table);
    rc = grn_hash_ensure_rehash(ctx, hash, reset_threshold, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  grn_id term_id;
  grn_posting *posting;
  while ((posting = grn_index_cursor_next(ctx, cursor, &term_id))) {
    grn_posting_internal posting_new = *((grn_posting_internal *)posting);
    posting_new.weight_float += additional_score;
    posting_new.weight_float *= weight;
    rc = grn_hash_add_record(ctx, hash, &posting_new, op, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  /* unlock */

  return ctx->rc;
}

grn_rc
grn_hash_add_ii_cursor(grn_ctx *ctx,
                       grn_hash *hash,
                       grn_ii_cursor *cursor,
                       double additional_score,
                       double weight,
                       grn_operator op)
{
  const char *tag = "[hash][add-ii-cursor]";
  grn_rc rc = grn_hash_add_records_validate(ctx, hash, op, tag);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  /* lock */
  if (op == GRN_OP_OR) {
    grn_ii *ii = grn_ii_cursor_get_ii(ctx, cursor);
    grn_obj *table = grn_ctx_at(ctx, DB_OBJ(ii)->range);
    const uint32_t reset_threshold = grn_table_size(ctx, table);
    grn_obj_unref(ctx, table);
    rc = grn_hash_ensure_rehash(ctx, hash, reset_threshold, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  grn_posting *posting;
  while ((posting = grn_ii_cursor_next(ctx, cursor))) {
    grn_posting_internal posting_new = *((grn_posting_internal *)posting);
    posting_new.weight_float += additional_score;
    posting_new.weight_float *= weight;
    rc = grn_hash_add_record(ctx, hash, &posting_new, op, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  /* unlock */

  return ctx->rc;
}

grn_rc
grn_hash_add_ii_select_cursor(grn_ctx *ctx,
                              grn_hash *hash,
                              grn_ii_select_cursor *cursor,
                              grn_operator op)
{
  const char *tag = "[hash][add-ii-select-cursor]";
  grn_rc rc = grn_hash_add_records_validate(ctx, hash, op, tag);
  if (rc != GRN_SUCCESS) {
    return rc;
  }

  /* lock */
  if (op == GRN_OP_OR) {
    grn_ii *ii = grn_ii_select_cursor_get_ii(ctx, cursor);
    grn_obj *table = grn_ctx_at(ctx, DB_OBJ(ii)->range);
    const uint32_t reset_threshold = grn_table_size(ctx, table);
    grn_obj_unref(ctx, table);
    rc = grn_hash_ensure_rehash(ctx, hash, reset_threshold, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }

  grn_ii_select_cursor_posting *posting;
  while ((posting = grn_ii_select_cursor_next(ctx, cursor))) {
    grn_posting_internal posting_new;
    posting_new.rid = posting->rid;
    posting_new.sid = posting->sid;
    posting_new.pos = posting->pos;
    posting_new.weight_float = posting->score;
    posting_new.scale = 1.0;
    rc = grn_hash_add_record(ctx, hash, &posting_new, op, tag);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  /* unlock */

  return ctx->rc;
}

grn_inline static grn_hash_entry *
grn_hash_get_entry(grn_ctx *ctx, grn_hash *hash, grn_id id)
{
  if (!grn_hash_bitmap_at(ctx, hash, id)) {
    return NULL;
  }
  return grn_hash_entry_at(ctx, hash, id, 0);
}

const char *
_grn_hash_key(grn_ctx *ctx, grn_hash *hash, grn_id id, uint32_t *key_size)
{
  grn_hash_entry * const entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    *key_size = 0;
    return NULL;
  }
  *key_size = grn_hash_entry_get_key_size(hash, entry);
  return grn_hash_entry_get_key(ctx, hash, entry);
}

int
grn_hash_get_key(grn_ctx *ctx, grn_hash *hash, grn_id id, void *keybuf, int bufsize)
{
  int key_size;
  grn_hash_entry *entry;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return 0;
  }
  entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    return 0;
  }
  key_size = grn_hash_entry_get_key_size(hash, entry);
  if (bufsize >= key_size) {
    grn_memcpy(keybuf, grn_hash_entry_get_key(ctx, hash, entry), key_size);
  }
  return key_size;
}

int
grn_hash_get_key2(grn_ctx *ctx, grn_hash *hash, grn_id id, grn_obj *bulk)
{
  int key_size;
  char *key;
  grn_hash_entry *entry;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return 0;
  }
  entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    return 0;
  }
  key_size = grn_hash_entry_get_key_size(hash, entry);
  key = grn_hash_entry_get_key(ctx, hash, entry);
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
  void *value;
  grn_hash_entry *entry;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return 0;
  }
  entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    return 0;
  }
  value = grn_hash_entry_get_value(ctx, hash, entry);
  if (!value) {
    return 0;
  }
  if (valuebuf) {
    grn_memcpy(valuebuf, value, hash->value_size);
  }
  return hash->value_size;
}

const char *
grn_hash_get_value_(grn_ctx *ctx, grn_hash *hash, grn_id id, uint32_t *size)
{
  const void *value;
  grn_hash_entry *entry;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return NULL;
  }
  entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    return NULL;
  }
  value = grn_hash_entry_get_value(ctx, hash, entry);
  if (!value) {
    return NULL;
  }
  if (size) {
    *size = hash->value_size;
  }
  return (const char *)value;
}

int
grn_hash_get_key_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                       void *keybuf, int bufsize, void *valuebuf)
{
  void *value;
  int key_size;
  grn_hash_entry *entry;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return 0;
  }
  entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    return 0;
  }
  key_size = grn_hash_entry_get_key_size(hash, entry);
  if (bufsize >= key_size) {
    grn_memcpy(keybuf, grn_hash_entry_get_key(ctx, hash, entry), key_size);
  }
  value = grn_hash_entry_get_value(ctx, hash, entry);
  if (!value) {
    return 0;
  }
  if (valuebuf) {
    grn_memcpy(valuebuf, value, hash->value_size);
  }
  return key_size;
}

int
_grn_hash_get_key_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                        void **key, void **value)
{
  int key_size;
  grn_hash_entry *entry;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return 0;
  }
  entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    return 0;
  }
  key_size = grn_hash_entry_get_key_size(hash, entry);
  *key = grn_hash_entry_get_key(ctx, hash, entry);
  *value = grn_hash_entry_get_value(ctx, hash, entry);
  return *value ? key_size : 0;
}

static void
grn_hash_set_value_raw(grn_ctx *ctx,
                       grn_hash *hash,
                       void *entry_value,
                       const void *value)
{
  grn_memcpy(entry_value, value, hash->value_size);
}

grn_rc
grn_hash_set_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                   const void *value, int flags)
{
  void *entry_value;
  grn_hash_entry *entry;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return GRN_ID_NIL;
  }
  if (!value) {
    return GRN_INVALID_ARGUMENT;
  }
  entry = grn_hash_get_entry(ctx, hash, id);
  if (!entry) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  entry_value = grn_hash_entry_get_value(ctx, hash, entry);
  if (!entry_value) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  grn_hash_wal_add_entry_data wal_data = {0};
  wal_data.hash = hash;
  wal_data.tag = "[hash][set-value]";
  wal_data.event = GRN_WAL_EVENT_SET_VALUE;
  wal_data.record_id = id;
  int32_t int32_value;
  int64_t int64_value;

  switch (flags & GRN_OBJ_SET_MASK) {
  case GRN_OBJ_SET :
    wal_data.value = value;
    break;
  case GRN_OBJ_INCR :
    switch (hash->value_size) {
    case sizeof(int32_t) :
      int32_value = *((int32_t *)entry_value) + *((int32_t *)value);
      wal_data.value = &int32_value;
      break;
    case sizeof(int64_t) :
      int64_value = *((int64_t *)entry_value) + *((int64_t *)value);
      wal_data.value = &int64_value;
      break;
    default :
      return GRN_INVALID_ARGUMENT;
    }
    break;
  case GRN_OBJ_DECR :
    switch (hash->value_size) {
    case sizeof(int32_t) :
      int32_value = *((int32_t *)entry_value) - *((int32_t *)value);
      wal_data.value = &int32_value;
      break;
    case sizeof(int64_t) :
      int64_value = *((int64_t *)entry_value) - *((int64_t *)value);
      wal_data.value = &int64_value;
      break;
    default :
      return GRN_INVALID_ARGUMENT;
    }
    break;
  default :
    grn_obj_set_error(ctx,
                      (grn_obj *)hash,
                      GRN_INVALID_ARGUMENT,
                      wal_data.record_id,
                      wal_data.tag,
                      "set flag must be "
                      "GRN_OBJ_SET, GRN_OBJ_INCR or GRN_OBJ_DECR: "
                      "%s<%d|%d>",
                      grn_obj_set_flag_to_string(flags),
                      flags & GRN_OBJ_SET_MASK,
                      flags);
    return ctx->rc;
  }

  if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return ctx->rc;
  }
  grn_hash_set_value_raw(ctx, hash, entry_value, wal_data.value);
  return ctx->rc;
}

typedef struct {
  grn_hash *hash;
  const char *tag;
  grn_id index_hash_value;
  grn_id *index;
  grn_id id;
  grn_hash_entry *entry;
  uint32_t key_size;
} grn_hash_delete_data;

grn_inline static grn_rc
grn_hash_delete_entry(grn_ctx *ctx, grn_hash_delete_data *data)
{
  grn_hash *hash = data->hash;
  *(data->index) = GARBAGE;
  if (grn_hash_is_io_hash(hash)) {
    grn_id *garbages;
    if (GRN_HASH_IS_LARGE_KEY(hash)) {
      garbages = hash->header.large->garbages;
    } else {
      garbages = hash->header.normal->garbages;
    }
    uint32_t garbage_index = data->key_size - 1;
    *((grn_id *)(data->entry)) = garbages[garbage_index];
    garbages[garbage_index] = data->id;
    grn_io_array_bit_off(ctx, hash->io, GRN_HASH_BITMAP_SEGMENT, data->id);
  } else {
    *((grn_id *)(data->entry)) = hash->garbages;
    hash->garbages = data->id;
    if ((hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) &&
        !(data->entry->header.flag & HASH_IMMEDIATE)) {
      grn_ctx *ctx = hash->ctx;
      GRN_CTX_FREE(ctx, data->entry->tiny_entry.key.ptr);
    }
    grn_tiny_bitmap_get_and_set(&hash->bitmap, data->id, 0);
  }
  (*(hash->n_garbages))++;
  (*(hash->n_entries))--;
  return GRN_SUCCESS;
}

grn_inline static grn_rc
grn_hash_delete_internal(grn_ctx *ctx, grn_hash_delete_data *data)
{
  grn_hash_wal_add_entry_data wal_data = {0};
  wal_data.hash = data->hash;
  wal_data.tag = data->tag;
  wal_data.event = GRN_WAL_EVENT_DELETE_ENTRY;
  wal_data.record_id = data->id;
  wal_data.key_size = data->key_size;
  wal_data.index_hash_value = data->index_hash_value;
  wal_data.n_garbages = *(data->hash->n_garbages);
  wal_data.n_entries = *(data->hash->n_entries);
  if (grn_hash_wal_add_entry(ctx, &wal_data) != GRN_SUCCESS) {
    return ctx->rc;
  }
  grn_rc rc = grn_hash_delete_entry(ctx, data);
  if (rc == GRN_SUCCESS) {
    grn_hash_wal_set_wal_id(ctx, data->hash, wal_data.wal_id);
  }
  return ctx->rc;
}

grn_rc
grn_hash_delete_by_id(grn_ctx *ctx, grn_hash *hash, grn_id id,
                      grn_table_delete_optarg *optarg)
{
  grn_hash_delete_data data = {0};
  data.hash = hash;
  data.tag = "[hash][delete][id]";
  data.id = id;
  if (!hash || id == GRN_ID_NIL) { return GRN_INVALID_ARGUMENT; }
  grn_rc rc = grn_hash_error_if_truncated(ctx, hash);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = GRN_INVALID_ARGUMENT;
  /* lock */
  data.entry = grn_hash_entry_at(ctx, hash, data.id, 0);
  if (data.entry) {
    uint32_t hash_value = data.entry->hash_value;
    uint32_t step = grn_hash_calculate_step(hash_value);
    if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
      data.key_size = data.entry->header.key_size;
    } else {
      data.key_size = hash->key_size;
    }
    for (data.index_hash_value = hash_value;
         ;
         data.index_hash_value += step) {
      data.index = grn_hash_idx_at(ctx, hash, data.index_hash_value);
      if (!data.index) { return GRN_NO_MEMORY_AVAILABLE; }
      if (*(data.index) == GRN_ID_NIL) { break; }
      if (*(data.index) != id) { continue; }
      rc = grn_hash_delete_internal(ctx, &data);
      break;
    }
  }
  /* unlock */
  return rc;
}

grn_rc
grn_hash_delete(grn_ctx *ctx, grn_hash *hash, const void *key, uint32_t key_size,
                grn_table_delete_optarg *optarg)
{
  grn_hash_delete_data data = {0};
  data.hash = hash;
  data.tag = "[hash][delete][key]";
  data.key_size = key_size;
  grn_rc rc = grn_hash_error_if_truncated(ctx, hash);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = GRN_INVALID_ARGUMENT;
  uint32_t hash_value;
  if (hash->obj.header.flags & GRN_OBJ_KEY_VAR_SIZE) {
    if (key_size > hash->key_size) { return GRN_INVALID_ARGUMENT; }
    hash_value = grn_hash_calculate_hash_value(key, key_size);
  } else {
    if (key_size != hash->key_size) { return GRN_INVALID_ARGUMENT; }
    if (key_size == sizeof(uint32_t)) {
      hash_value = *((uint32_t *)key);
    } else {
      hash_value = grn_hash_calculate_hash_value(key, key_size);
    }
  }
  uint32_t step = grn_hash_calculate_step(hash_value);
  {
    /* lock */
    for (data.index_hash_value = hash_value;
         ;
         data.index_hash_value += step) {
      data.index = grn_hash_idx_at(ctx, hash, data.index_hash_value);
      if (!data.index) { return GRN_NO_MEMORY_AVAILABLE; }
      data.id = *(data.index);
      if (data.id == GRN_ID_NIL) { break; }
      if (data.id == GARBAGE) { continue; }
      data.entry = grn_hash_entry_at(ctx, hash, data.id, 0);
      if (!data.entry) {
        continue;
      }
      if (grn_hash_entry_compare_key(ctx,
                                     hash,
                                     data.entry,
                                     hash_value,
                                     key,
                                     key_size)) {
        rc = grn_hash_delete_internal(ctx, &data);
        break;
      }
    }
    /* unlock */
    return rc;
  }
}

void
grn_hash_cursor_close(grn_ctx *ctx, grn_hash_cursor *c)
{
  GRN_ASSERT(c->ctx == ctx);
  GRN_FREE(c);
}

#define HASH_CURR_MAX(hash) \
  ((grn_hash_is_io_hash(hash)) ? (hash)->header.common->curr_rec : (hash)->a.max)

grn_hash_cursor *
grn_hash_cursor_open(grn_ctx *ctx, grn_hash *hash,
                     const void *min, uint32_t min_size,
                     const void *max, uint32_t max_size,
                     int offset, int limit, int flags)
{
  grn_hash_cursor *c;
  if (!hash || !ctx) { return NULL; }
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return NULL;
  }
  if (!(c = GRN_CALLOC(sizeof(grn_hash_cursor)))) { return NULL; }
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
  c->rest = (limit < 0) ? GRN_ARRAY_MAX : limit;
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
  if (ee && (v = get_value(ctx, c->hash, ee))) {
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
  if (value) { *value = get_value(ctx, c->hash, ee); }
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
    ep = ((const uint8_t *)(get_value(ctx, hash, (entry_str *)(e))));\
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

grn_inline static entry **
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

grn_inline static void
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

grn_inline static entry **
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
                              ? get_value(ctx, hash, (e))\
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

grn_inline static val32 *
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

grn_inline static val32 *
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

grn_inline static grn_id
entry2id(grn_ctx *ctx, grn_hash *hash, entry *e)
{
  entry *e2;
  grn_id id, *ep;
  uint32_t i, h = e->key, s = grn_hash_calculate_step(h);
  for (i = h; ; i += s) {
    if (!(ep = grn_hash_idx_at(ctx, hash, i))) { return GRN_ID_NIL; }
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
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return 0;
  }
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
  /*  hash->limit = limit; */
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
  grn_hash_header_common *h = hash->header.common;
  if (grn_hash_error_if_truncated(ctx, hash) != GRN_SUCCESS) {
    return;
  }
  GRN_OUTPUT_ARRAY_OPEN("RESULT", 1);
  GRN_OUTPUT_MAP_OPEN("SUMMARY", 26);
  GRN_OUTPUT_CSTR("flags");
  grn_itoh(h->flags, buf, 8);
  GRN_OUTPUT_STR(buf, 8);
  GRN_OUTPUT_CSTR("key_size");
  GRN_OUTPUT_INT64(hash->key_size);
  GRN_OUTPUT_CSTR("value_size");
  GRN_OUTPUT_INT64(hash->value_size);
  GRN_OUTPUT_CSTR("tokenizer");
  GRN_OUTPUT_INT64(h->tokenizer);
  GRN_OUTPUT_CSTR("normalizer");
  GRN_OUTPUT_INT64(h->normalizer);
  GRN_OUTPUT_CSTR("curr_rec");
  GRN_OUTPUT_INT64(h->curr_rec);
  GRN_OUTPUT_CSTR("curr_key_normal");
  GRN_OUTPUT_UINT64(h->curr_key_normal);
  GRN_OUTPUT_CSTR("curr_key_large");
  GRN_OUTPUT_UINT64(h->curr_key_large);
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
  if (record_unit != GRN_REC_USERDEF && subrec_unit != GRN_REC_USERDEF) {
    subrec_size -= record_size;
  }
  if (!hash) { return GRN_INVALID_ARGUMENT; }
  if (record_size < 0) { return GRN_INVALID_ARGUMENT; }
  if ((default_flags & GRN_HASH_TINY)) {
    rc = grn_tiny_hash_init(ctx, hash, NULL, record_size,
                            max_n_subrecs * (GRN_RSET_SCORE_SIZE + subrec_size),
                            default_flags, GRN_ENC_NONE);
  } else {
    rc = grn_io_hash_init(ctx, hash, NULL, record_size,
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
  if (grn_hash_is_io_hash(hash)) {
    rc = grn_io_close(ctx, hash->io);
  } else {
    GRN_ASSERT(ctx == hash->ctx);
    rc = grn_tiny_hash_fin(ctx, hash);
  }
  return rc;
}

grn_inline static void
subrecs_push(byte *subrecs, int size, int n_subrecs, int score, void *body, int dir)
{
  byte *v;
  int *c2;
  int n = n_subrecs - 1, n2;
  while (n) {
    n2 = (n - 1) >> 1;
    c2 = GRN_RSET_SUBRECS_NTH(subrecs,size,n2);
    if (GRN_RSET_SUBRECS_CMP(score, *c2, dir) > 0) { break; }
    GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
    n = n2;
  }
  v = subrecs + n * (size + GRN_RSET_SCORE_SIZE);
  *((int *)v) = score;
  grn_memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
}

grn_inline static void
subrecs_replace_min(byte *subrecs, int size, int n_subrecs, int score, void *body, int dir)
{
  byte *v;
  int n = 0, n1, n2, *c1, *c2;
  for (;;) {
    n1 = n * 2 + 1;
    n2 = n1 + 1;
    c1 = n1 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n1) : NULL;
    c2 = n2 < n_subrecs ? GRN_RSET_SUBRECS_NTH(subrecs,size,n2) : NULL;
    if (c1 && GRN_RSET_SUBRECS_CMP(score, *c1, dir) > 0) {
      if (c2 &&
          GRN_RSET_SUBRECS_CMP(score, *c2, dir) > 0 &&
          GRN_RSET_SUBRECS_CMP(*c1, *c2, dir) > 0) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c1);
        n = n1;
      }
    } else {
      if (c2 && GRN_RSET_SUBRECS_CMP(score, *c2, dir) > 0) {
        GRN_RSET_SUBRECS_COPY(subrecs,size,n,c2);
        n = n2;
      } else {
        break;
      }
    }
  }
  v = subrecs + n * (size + GRN_RSET_SCORE_SIZE);
  grn_memcpy(v, &score, GRN_RSET_SCORE_SIZE);
  grn_memcpy(v + GRN_RSET_SCORE_SIZE, body, size);
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
      if (GRN_RSET_SUBRECS_CMP(score, *ri->subrecs, dir) > 0) {
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
    unit = GRN_REC_USERDEF;
    rsize = optarg->key_size;
    funcp = optarg->func ? 1 : 0;
    dir = (optarg->mode == grn_sort_ascending) ? -1 : 1;
  } else {
    unit = GRN_REC_DOCUMENT;
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
  grn_memcpy(&h, s, sizeof(grn_hash));
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
grn_rhash_subrec_info(grn_ctx *ctx, grn_hash *s, grn_id rh, int index,
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
    ri = get_value(ctx, s, ee);
    if (!pi || !ri) { return GRN_INVALID_ARGUMENT; }
  }
  if (index >= ri->n_subrecs) { return GRN_INVALID_ARGUMENT; }
  p = (int *)(ri->subrecs + index * unit_size);
  if (score) { *score = p[0]; }
  if (subrec) { *subrec = &p[1]; }
  switch (s->record_unit) {
  case GRN_REC_DOCUMENT :
    if (rid) { *rid = pi->rid; }
    if (section) { *section = (s->subrec_unit != GRN_REC_USERDEF) ? p[1] : 0; }
    if (pos) { *pos = (s->subrec_unit == GRN_REC_POSITION) ? p[2] : 0; }
    break;
  case GRN_REC_SECTION :
    if (rid) { *rid = pi->rid; }
    if (section) { *section = pi->sid; }
    if (pos) { *pos = (s->subrec_unit == GRN_REC_POSITION) ? p[1] : 0; }
    break;
  default :
    pi = (grn_rset_posinfo *)&p[1];
    switch (s->subrec_unit) {
    case GRN_REC_DOCUMENT :
      if (rid) { *rid = pi->rid; }
      if (section) { *section = 0; }
      if (pos) { *pos = 0; }
      break;
    case GRN_REC_SECTION :
      if (rid) { *rid = pi->rid; }
      if (section) { *section = pi->sid; }
      if (pos) { *pos = 0; }
      break;
    case GRN_REC_POSITION :
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

grn_bool
grn_hash_is_large_total_key_size(grn_ctx *ctx, grn_hash *hash)
{
  return (hash->header.common->flags & GRN_OBJ_KEY_LARGE) == GRN_OBJ_KEY_LARGE;
}

uint64_t
grn_hash_total_key_size(grn_ctx *ctx, grn_hash *hash)
{
  if (grn_hash_is_large_total_key_size(ctx, hash)) {
    return hash->header.common->curr_key_large;
  } else {
    return hash->header.common->curr_key_normal;
  }
}

uint64_t
grn_hash_max_total_key_size(grn_ctx *ctx, grn_hash *hash)
{
  if (grn_hash_is_large_total_key_size(ctx, hash)) {
    return GRN_HASH_KEY_MAX_TOTAL_SIZE_LARGE;
  } else {
    return GRN_HASH_KEY_MAX_TOTAL_SIZE_NORMAL;
  }
}

grn_rc
grn_hash_wal_recover(grn_ctx *ctx, grn_hash *hash)
{
  if (!hash->io) {
    return GRN_SUCCESS;
  }
  if (hash->io->path[0] == '\0') {
    return GRN_SUCCESS;
  }

  const char *wal_error_tag = "[hash]";
  const char *tag = "[hash][recover]";
  grn_wal_reader *reader = grn_wal_reader_open(ctx, (grn_obj *)hash, tag);
  if (!reader) {
    return ctx->rc;
  }

  grn_io_clear_lock(hash->io);

  grn_id partial_record_id = GRN_ID_NIL;
  while (true) {
    grn_wal_reader_entry entry = {0};
    grn_rc rc = grn_wal_reader_read_entry(ctx, reader, &entry);
    if (rc != GRN_SUCCESS) {
      break;
    }
    switch (entry.event) {
    case GRN_WAL_EVENT_ADD_ENTRY :
      {
        grn_id *index = grn_hash_idx_at(ctx, hash, entry.index_hash_value);
        if (!index) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer index");
          break;
        }
        *(hash->n_entries) = entry.n_entries;
        if (grn_io_hash_add_entry(ctx,
                                  hash,
                                  entry.record_id,
                                  index,
                                  tag)) {
          partial_record_id = entry.record_id;
        }
      }
      break;
    case GRN_WAL_EVENT_REUSE_ENTRY :
      {
        grn_id *index = grn_hash_idx_at(ctx, hash, entry.index_hash_value);
        if (!index) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer index");
          break;
        }
        *(hash->n_garbages) = entry.n_garbages;
        *(hash->n_entries) = entry.n_entries;
        if (grn_io_hash_reuse_entry(ctx,
                                    hash,
                                    entry.record_id,
                                    entry.key.content.uint64,
                                    index,
                                    tag)) {
          partial_record_id = entry.record_id;
        }
      }
      break;
    case GRN_WAL_EVENT_RESET_ENTRY :
      {
        grn_hash_entry *hash_entry =
          grn_io_hash_entry_at(ctx, hash, entry.record_id, GRN_TABLE_ADD);
        if (!hash_entry) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer hash entry");
          break;
        }
        grn_io_hash_reset_entry(ctx,
                                hash,
                                hash_entry,
                                entry.next_garbage_record_id,
                                entry.key.content.uint64,
                                tag);
      }
      break;
    case GRN_WAL_EVENT_ENABLE_ENTRY :
      grn_io_hash_enable_entry(ctx,
                               hash,
                               entry.record_id,
                               tag);
      break;
    case GRN_WAL_EVENT_SET_ENTRY_KEY :
      {
        grn_hash_entry *hash_entry =
          grn_io_hash_entry_at(ctx, hash, entry.record_id, GRN_TABLE_ADD);
        if (!hash_entry) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer hash entry");
          break;
        }
        if (grn_hash_is_large_total_key_size(ctx, hash)) {
          hash->header.common->curr_key_large = entry.key_offset;
        } else {
          hash->header.common->curr_key_normal = entry.key_offset;
        }
        rc = grn_hash_entry_put_key(ctx,
                                    hash,
                                    hash_entry,
                                    entry.key_hash_value,
                                    entry.key.content.binary.data,
                                    entry.key.content.binary.size);
        if (rc == GRN_SUCCESS) {
          partial_record_id = GRN_ID_NIL;
        }
      }
      break;
    case GRN_WAL_EVENT_DELETE_ENTRY :
      {
        grn_hash_delete_data data;
        data.hash = hash;
        data.tag = tag;
        data.index_hash_value = entry.index_hash_value;
        data.index = grn_hash_idx_at(ctx, hash, data.index_hash_value);
        if (!data.index) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer index");
          break;
        }
        data.id = entry.record_id;
        data.entry = grn_hash_entry_at(ctx, hash, data.id, 0);
        if (!data.entry) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer hash entry");
          break;
        }
        data.key_size = entry.key.content.uint64;
        *(hash->n_garbages) = entry.n_garbages;
        *(hash->n_entries) = entry.n_entries;
        if (grn_hash_delete_entry(ctx, &data) == GRN_SUCCESS) {
          if (partial_record_id == data.id) {
            partial_record_id = GRN_ID_NIL;
          }
        }
      }
      break;
    case GRN_WAL_EVENT_REHASH :
      *(hash->n_entries) = entry.n_entries;
      *(hash->max_offset) = entry.max_offset;
      grn_hash_rehash(ctx, hash, entry.expected_n_entries);
      break;
    case GRN_WAL_EVENT_SET_VALUE :
      {
        grn_hash_entry *hash_entry =
          grn_io_hash_entry_at(ctx, hash, entry.record_id, GRN_TABLE_ADD);
        if (!hash_entry) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer hash entry");
          break;
        }
        void *hash_entry_value = grn_hash_entry_get_value(ctx, hash, hash_entry);
        if (!hash_entry_value) {
          grn_wal_set_recover_error(ctx,
                                    GRN_NO_MEMORY_AVAILABLE,
                                    (grn_obj *)hash,
                                    &entry,
                                    wal_error_tag,
                                    "failed to refer hash entry value");
          break;
        }
        grn_hash_set_value_raw(ctx,
                               hash,
                               hash_entry_value,
                               entry.value.content.binary.data);
      }
      break;
    default :
      grn_wal_set_recover_error(ctx,
                                GRN_FUNCTION_NOT_IMPLEMENTED,
                                (grn_obj *)hash,
                                &entry,
                                wal_error_tag,
                                "not implemented yet");
      break;
    }
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
    grn_hash_wal_set_wal_id(ctx, hash, entry.id);
  }
  grn_wal_reader_close(ctx, reader);

  if (partial_record_id != GRN_ID_NIL) {
    grn_obj_log(ctx,
                (grn_obj *)hash,
                GRN_LOG_WARNING,
                partial_record_id,
                tag,
                "there is a partially added record");
    /* This adds a WAL entry but it will be removed by the following
     * grn_obj_flush(). */
    grn_rc rc = grn_hash_delete_by_id(ctx, hash, partial_record_id, NULL);
    if (rc != GRN_SUCCESS) {
      grn_obj_set_error(ctx,
                        (grn_obj *)hash,
                        rc,
                        partial_record_id,
                        tag,
                        "failed to delete a partially added record");
    }
  }

  if (ctx->rc == GRN_SUCCESS) {
    grn_obj_touch(ctx, (grn_obj *)hash, NULL);
    grn_obj_flush(ctx, (grn_obj *)hash);
  }

  return ctx->rc;
}

grn_rc
grn_hash_warm(grn_ctx *ctx, grn_hash *hash)
{
  if (grn_hash_is_io_hash(hash)) {
    return grn_io_warm(ctx, hash->io);
  } else {
    return GRN_SUCCESS;
  }
}
