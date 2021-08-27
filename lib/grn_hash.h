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

#pragma once

#include "grn.h"
#include "grn_ii_select_cursor.h"
#include "grn_io.h"
#include "grn_table_module.h"

#ifdef __cplusplus
extern "C" {
#endif

/**** grn_tiny_array ****/

/*
 * grn_tiny_array_init() accepts a logical OR of the following flags.
 * Note that other flags, such as (1 << 30), will be ignored.
 *
 * - GRN_TINY_ARRAY_CLEAR specifies to initialize a new block with zeros.
 *   It is valid only iff specified with GRN_TINY_ARRAY_USE_MALLOC.
 * - GRN_TINY_ARRAY_THREADSAFE specifies to create a critical section when
 *   allocating memory.
 * - GRN_TINY_ARRAY_USE_MALLOC specifies to use GRN_MALLOC/CALLOC/FREE instead
 *   of GRN_CTX_ALLOC/FREE.
 */
#define GRN_TINY_ARRAY_CLEAR      (1 << 0)
#define GRN_TINY_ARRAY_THREADSAFE (1 << 1)
#define GRN_TINY_ARRAY_USE_MALLOC (1 << 2)

/*
 * - GRN_TINY_ARRAY_FACTOR is the global parameter of grn_tiny_array.
 * - GRN_TINY_ARRAY_GET_OFFSET() returns the offset of a specified block.
 * - GRN_TINY_ARRAY_BASE_BLOCK_SIZE is the number of elements in the first
 *   block.
 * - GRN_TINY_ARRAY_GET_BLOCK_SIZE() returns the number of elements in a
 *   specified block.
 * - GRN_TINY_ARRAY_NUM_BLOCKS is the maximum number of blocks.
 */
#define GRN_TINY_ARRAY_FACTOR     0
#define GRN_TINY_ARRAY_GET_OFFSET(block_id) \
  (1 << ((block_id) << GRN_TINY_ARRAY_FACTOR))
#define GRN_TINY_ARRAY_BASE_BLOCK_SIZE \
  (GRN_TINY_ARRAY_GET_OFFSET(1) - GRN_TINY_ARRAY_GET_OFFSET(0))
#define GRN_TINY_ARRAY_GET_BLOCK_SIZE(block_id) \
  (GRN_TINY_ARRAY_BASE_BLOCK_SIZE * GRN_TINY_ARRAY_GET_OFFSET(block_id))
#define GRN_TINY_ARRAY_NUM_BLOCKS (32 >> GRN_TINY_ARRAY_FACTOR)

/*
 * grn_tiny_array uses several blocks to emulate an array.
 * The k-th block, blocks[k - 1], consists of 2^(k-1) elements.
 */
typedef struct _grn_tiny_array grn_tiny_array;

struct _grn_tiny_array {
  grn_ctx *ctx;
  grn_id max;
  uint16_t element_size;
  uint16_t flags;
  void *blocks[GRN_TINY_ARRAY_NUM_BLOCKS];
  grn_critical_section lock;
};

#define GRN_TINY_ARRAY_EACH(array, head, tail, key, value, block) do { \
  int _block_id; \
  const grn_id _head = (head); \
  const grn_id _tail = (tail); \
  for (_block_id = 0, (key) = (_head); \
       _block_id < GRN_TINY_ARRAY_NUM_BLOCKS && (key) <= (_tail); \
       _block_id++) { \
    int _id = GRN_TINY_ARRAY_GET_BLOCK_SIZE(_block_id); \
    (value) = (array)->blocks[_block_id]; \
    if (value) { \
      while (_id-- && (key) <= (_tail)) { \
        { \
          block \
        } \
        (key)++; \
        (value) = (void *)((byte *)(value) + (array)->element_size); \
      } \
    } else { \
      (key) += _id; \
    } \
  } \
} while (0)

#define GRN_TINY_ARRAY_EACH_BEGIN(array, head, tail, value) do { \
  int _block_id; \
  const grn_id _head = (head); \
  const grn_id _tail = (tail); \
  grn_id _current = _head; \
  for (_block_id = 0; \
       _block_id < GRN_TINY_ARRAY_NUM_BLOCKS && _current <= _tail; \
       _block_id++) { \
    int _n_elements = GRN_TINY_ARRAY_GET_BLOCK_SIZE(_block_id); \
    uint8_t *_block = (array)->blocks[_block_id]; \
    if (_block) { \
      int _i; \
      for (_i = 0; \
           _i < _n_elements && _current <= _tail; \
           _i++, _current++) { \
        void *value = _block + (array)->element_size * _i; \

#define GRN_TINY_ARRAY_EACH_END() \
      } \
    } else { \
      _current += _n_elements; \
    } \
  } \
} while (0)


GRN_API void grn_tiny_array_init(grn_ctx *ctx, grn_tiny_array *array,
                                 uint16_t element_size, uint16_t flags);
GRN_API void grn_tiny_array_fin(grn_tiny_array *array);
GRN_API void *grn_tiny_array_at(grn_tiny_array *array, grn_id id);
GRN_API grn_id grn_tiny_array_id(grn_tiny_array *array,
                                 const void *element_address);

/**** grn_tiny_bitmap ****/

typedef struct _grn_tiny_bitmap grn_tiny_bitmap;

struct _grn_tiny_bitmap {
  grn_ctx *ctx;
  void *blocks[GRN_TINY_ARRAY_NUM_BLOCKS];
};

/**** grn_array ****/

#define GRN_ARRAY_TINY        (0x01<<6)

/*
 * grn_array uses grn_io or grn_tiny_array to represent an array.
 *
 * To create a grn_tiny_array-based grn_array, specify the GRN_ARRAY_TINY flag
 * to grn_array_create(). Note that a grn_tiny_array-based grn_array is not
 * backed by a file.
 */
struct _grn_array {
  grn_db_obj obj;
  grn_ctx *ctx;
  uint32_t value_size;
  int32_t n_keys;
  grn_table_sort_key *keys;
  uint32_t *n_garbages;
  uint32_t *n_entries;

  /* For grn_io_array. */
  grn_io *io;
  struct grn_array_header *header;
  uint32_t *lock;
  bool wal_touched;

  /* For grn_tiny_array. */
  uint32_t n_garbages_buf;
  uint32_t n_entries_buf;
  grn_id garbages;
  grn_tiny_array array;
  grn_tiny_bitmap bitmap;
};

struct _grn_array_cursor {
  grn_db_obj obj;
  grn_array *array;
  grn_ctx *ctx;
  grn_id curr_rec;
  grn_id tail;
  unsigned int rest;
  int dir;
};

/*
 * grn_array_size() returns the number of entries in an array.
 * If the array was truncated by another process but `array` still refers to
 * the old one, this function returns 0.
 */
uint32_t grn_array_size(grn_ctx *ctx, grn_array *array);

uint32_t grn_array_get_flags(grn_ctx *ctx, grn_array *array);

grn_rc grn_array_truncate(grn_ctx *ctx, grn_array *array);
grn_rc grn_array_copy_sort_key(grn_ctx *ctx, grn_array *array,
                               grn_table_sort_key *keys, int n_keys);
grn_rc grn_array_wal_recover(grn_ctx *ctx, grn_array *array);
grn_rc grn_array_warm(grn_ctx *ctx, grn_array *array);

/**** grn_hash ****/

#define GRN_HASH_MAX_KEY_SIZE_NORMAL GRN_TABLE_MAX_KEY_SIZE
#define GRN_HASH_MAX_KEY_SIZE_LARGE  (0xffff)

#define GRN_HASH_IS_LARGE_KEY(hash)\
  ((hash)->key_size > GRN_HASH_MAX_KEY_SIZE_NORMAL)

typedef struct _grn_hash_header_common grn_hash_header_common;
typedef struct _grn_hash_header_normal grn_hash_header_normal;
typedef struct _grn_hash_header_large  grn_hash_header_large;

typedef struct _grn_hash_wal_add_entry_data grn_hash_wal_add_entry_data;

struct _grn_hash {
  grn_db_obj obj;
  grn_ctx *ctx;
  uint32_t key_size;
  grn_encoding encoding;
  uint32_t value_size;
  uint32_t entry_size;
  uint32_t *n_garbages;
  uint32_t *n_entries;
  uint32_t *max_offset;
  grn_table_module tokenizer;
  grn_obj normalizers;
  grn_obj token_filters;
  /* For backward compatibility */
  grn_obj token_filter_procs;

  /* For grn_io_hash. */
  grn_io *io;
  union {
    grn_hash_header_common *common;
    grn_hash_header_normal *normal;
    grn_hash_header_large  *large;
  } header;
  uint32_t *lock;
  // uint32_t nref;
  // unsigned int max_n_subrecs;
  // unsigned int record_size;
  // unsigned int subrec_size;
  // grn_rec_unit record_unit;
  // grn_rec_unit subrec_unit;
  // uint8_t arrayp;
  // grn_recordh *curr_rec;
  // grn_set_cursor *cursor;
  // int limit;
  // void *userdata;
  // grn_id subrec_id;
  grn_hash_wal_add_entry_data *wal_data;

  /* For grn_tiny_hash. */
  uint32_t max_offset_;
  uint32_t n_garbages_;
  uint32_t n_entries_;
  grn_id *index;
  grn_id garbages;
  grn_tiny_array a;
  grn_tiny_bitmap bitmap;
};

#define GRN_HASH_HEADER_COMMON_FIELDS\
  uint32_t flags;\
  grn_encoding encoding;\
  uint32_t key_size;\
  uint32_t value_size;\
  grn_id tokenizer;\
  uint32_t curr_rec;\
  uint32_t curr_key_normal;\
  uint32_t idx_offset;\
  uint32_t entry_size;\
  uint32_t max_offset;\
  uint32_t n_entries;\
  uint32_t n_garbages;\
  uint32_t lock;\
  grn_id normalizer;\
  uint32_t truncated;\
  uint64_t curr_key_large;\
  uint64_t wal_id;\
  uint32_t reserved[10]

struct _grn_hash_header_common {
  GRN_HASH_HEADER_COMMON_FIELDS;
};

struct _grn_hash_header_normal {
  GRN_HASH_HEADER_COMMON_FIELDS;
  grn_id garbages[GRN_HASH_MAX_KEY_SIZE_NORMAL];
};

struct _grn_hash_header_large {
  GRN_HASH_HEADER_COMMON_FIELDS;
  grn_id garbages[GRN_HASH_MAX_KEY_SIZE_LARGE];
};

struct _grn_hash_cursor {
  grn_db_obj obj;
  grn_hash *hash;
  grn_ctx *ctx;
  grn_id curr_rec;
  grn_id tail;
  unsigned int rest;
  int dir;
};

/* deprecated */

#define GRN_TABLE_SORT_BY_KEY      0
#define GRN_TABLE_SORT_BY_ID       (1L<<1)
#define GRN_TABLE_SORT_BY_VALUE    (1L<<2)
#define GRN_TABLE_SORT_RES_ID      0
#define GRN_TABLE_SORT_RES_KEY     (1L<<3)
#define GRN_TABLE_SORT_AS_BIN      0
#define GRN_TABLE_SORT_AS_NUMBER   (1L<<4)
#define GRN_TABLE_SORT_AS_SIGNED   0
#define GRN_TABLE_SORT_AS_UNSIGNED (1L<<5)
#define GRN_TABLE_SORT_AS_INT32    0
#define GRN_TABLE_SORT_AS_INT64    (1L<<6)
#define GRN_TABLE_SORT_NO_PROC     0
#define GRN_TABLE_SORT_WITH_PROC   (1L<<7)

typedef struct _grn_table_sort_optarg grn_table_sort_optarg;

struct _grn_table_sort_optarg {
  grn_table_sort_flags flags;
  int (*compar)(grn_ctx *ctx,
                grn_obj *table1, void *target1, unsigned int target1_size,
                grn_obj *table2, void *target2, unsigned int target2_size,
                void *compare_arg);
  void *compar_arg;
  grn_obj *proc;
  int offset;
};

void grn_hash_init_from_env(void);

GRN_API int grn_hash_sort(grn_ctx *ctx, grn_hash *hash, int limit,
                          grn_array *result, grn_table_sort_optarg *optarg);

grn_rc grn_hash_lock(grn_ctx *ctx, grn_hash *hash, int timeout);
grn_rc grn_hash_unlock(grn_ctx *ctx, grn_hash *hash);
grn_rc grn_hash_clear_lock(grn_ctx *ctx, grn_hash *hash);

#define GRN_HASH_SIZE(hash) (*((hash)->n_entries))

/* private */
typedef enum {
  GRN_REC_DOCUMENT = 0,
  GRN_REC_SECTION,
  GRN_REC_POSITION,
  GRN_REC_USERDEF,
  GRN_REC_NONE
} grn_rec_unit;

GRN_API grn_rc grn_hash_truncate(grn_ctx *ctx, grn_hash *hash);

int grn_rec_unit_size(grn_rec_unit unit, int rec_size);

const char * _grn_hash_key(grn_ctx *ctx, grn_hash *hash, grn_id id, uint32_t *key_size);

grn_rc
grn_hash_add_table_cursor(grn_ctx *ctx,
                          grn_hash *hash,
                          grn_table_cursor *cursor,
                          double score,
                          grn_operator op);
grn_rc
grn_hash_add_index_cursor(grn_ctx *ctx,
                          grn_hash *hash,
                          grn_obj *cursor,
                          double additional_score,
                          double weight,
                          grn_operator op);
grn_rc
grn_hash_add_ii_cursor(grn_ctx *ctx,
                       grn_hash *hash,
                       grn_ii_cursor *cursor,
                       double additional_score,
                       double weight,
                       grn_operator op);
grn_rc
grn_hash_add_ii_select_cursor(grn_ctx *ctx,
                              grn_hash *hash,
                              grn_ii_select_cursor *cursor,
                              grn_operator op);

int grn_hash_get_key_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                           void *keybuf, int bufsize, void *valuebuf);

int _grn_hash_get_key_value(grn_ctx *ctx, grn_hash *hash, grn_id id,
                            void **key, void **value);

grn_id grn_hash_next(grn_ctx *ctx, grn_hash *hash, grn_id id);

/* only valid for hash tables, GRN_OBJ_KEY_VAR_SIZE && GRN_HASH_TINY */
const char *_grn_hash_strkey_by_val(void *v, uint16_t *size);

const char *grn_hash_get_value_(grn_ctx *ctx, grn_hash *hash, grn_id id, uint32_t *size);

grn_rc grn_hash_remove(grn_ctx *ctx, const char *path);
grn_rc grn_array_remove(grn_ctx *ctx, const char *path);

grn_id grn_hash_at(grn_ctx *ctx, grn_hash *hash, grn_id id);
grn_id grn_array_at(grn_ctx *ctx, grn_array *array, grn_id id);

void grn_hash_check(grn_ctx *ctx, grn_hash *hash);

grn_bool grn_hash_is_large_total_key_size(grn_ctx *ctx, grn_hash *hash);

uint64_t grn_hash_total_key_size(grn_ctx *ctx, grn_hash *hash);
uint64_t grn_hash_max_total_key_size(grn_ctx *ctx, grn_hash *hash);

grn_rc grn_hash_wal_recover(grn_ctx *ctx, grn_hash *hash);
grn_rc grn_hash_warm(grn_ctx *ctx, grn_hash *hash);

#ifdef __cplusplus
}
#endif
