/*
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  GRN_WAL_EVENT_NIL,
  GRN_WAL_EVENT_SET_VALUE,
  GRN_WAL_EVENT_NEW_SEGMENT,
  GRN_WAL_EVENT_USE_SEGMENT,
  GRN_WAL_EVENT_REUSE_SEGMENT,
  GRN_WAL_EVENT_FREE_SEGMENT,
  GRN_WAL_EVENT_ADD_ENTRY,
  GRN_WAL_EVENT_ADD_SHARED_ENTRY,
  GRN_WAL_EVENT_REUSE_ENTRY,
  GRN_WAL_EVENT_REUSE_SHARED_ENTRY,
  GRN_WAL_EVENT_RESET_ENTRY,
  GRN_WAL_EVENT_ENABLE_ENTRY,
  GRN_WAL_EVENT_SET_ENTRY_KEY,
  GRN_WAL_EVENT_DELETE_ENTRY,
  GRN_WAL_EVENT_UPDATE_ENTRY,
  GRN_WAL_EVENT_REHASH,
  GRN_WAL_EVENT_DELETE_INFO_PHASE1,
  GRN_WAL_EVENT_DELETE_INFO_PHASE2,
  GRN_WAL_EVENT_DELETE_INFO_PHASE3,
} grn_wal_event;

const char *
grn_wal_event_to_string(grn_wal_event event);

typedef enum {
  GRN_WAL_SEGMENT_CHUNK,
  GRN_WAL_SEGMENT_SEQUENTIAL,
  GRN_WAL_SEGMENT_HUGE,
  GRN_WAL_SEGMENT_EINFO,
  GRN_WAL_SEGMENT_GINFO,
} grn_wal_segment_type;

const char *
grn_wal_segment_type_to_string(grn_wal_segment_type type);

typedef enum {
  GRN_WAL_KEY_END,
  GRN_WAL_KEY_ID,
  GRN_WAL_KEY_EVENT,
  GRN_WAL_KEY_RECORD_ID,
  GRN_WAL_KEY_RECORD_DIRECTION,
  GRN_WAL_KEY_ELEMENT_SIZE,
  GRN_WAL_KEY_KEY,
  GRN_WAL_KEY_KEY_SIZE,
  GRN_WAL_KEY_KEY_HASH_VALUE,
  GRN_WAL_KEY_KEY_OFFSET,
  GRN_WAL_KEY_SHARED_KEY_OFFSET,
  GRN_WAL_KEY_IS_SHARED,
  GRN_WAL_KEY_CHECK,
  GRN_WAL_KEY_NEW_KEY,
  GRN_WAL_KEY_PARENT_RECORD_ID,
  GRN_WAL_KEY_PARENT_RECORD_DIRECTION,
  GRN_WAL_KEY_PARENT_CHECK,
  GRN_WAL_KEY_GRANDPARENT_RECORD_ID,
  GRN_WAL_KEY_OTHERSIDE_RECORD_ID,
  GRN_WAL_KEY_OTHERSIDE_CHECK,
  GRN_WAL_KEY_LEFT_RECORD_ID,
  GRN_WAL_KEY_RIGHT_RECORD_ID,
  GRN_WAL_KEY_VALUE,
  GRN_WAL_KEY_INDEX_HASH_VALUE,
  GRN_WAL_KEY_SEGMENT,
  GRN_WAL_KEY_POSITION,
  GRN_WAL_KEY_SEGMENT_TYPE,
  GRN_WAL_KEY_SEGMENT_INFO,
  GRN_WAL_KEY_GARBAGE_SEGMENT,
  GRN_WAL_KEY_PREVIOUS_GARBAGE_SEGMENT,
  GRN_WAL_KEY_NEXT_GARBAGE_SEGMENT,
  GRN_WAL_KEY_GARBAGE_SEGMENT_HEAD,
  GRN_WAL_KEY_GARBAGE_SEGMENT_TAIL,
  GRN_WAL_KEY_GARBAGE_SEGMENT_N_RECORDS,
  GRN_WAL_KEY_NEXT_GARBAGE_RECORD_ID,
  GRN_WAL_KEY_N_GARBAGES,
  GRN_WAL_KEY_N_ENTRIES,
  GRN_WAL_KEY_MAX_OFFSET,
  GRN_WAL_KEY_EXPECTED_N_ENTRIES,
  GRN_WAL_KEY_DELETE_INFO_INDEX,
  GRN_WAL_KEY_DELETE_INFO_PHASE1_INDEX,
  GRN_WAL_KEY_DELETE_INFO_PHASE2_INDEX,
  GRN_WAL_KEY_DELETE_INFO_PHASE3_INDEX,
} grn_wal_key;

const char *
grn_wal_key_to_string(grn_wal_key key);

typedef enum {
  GRN_WAL_VALUE_NIL,
  GRN_WAL_VALUE_EVENT,
  GRN_WAL_VALUE_RECORD_ID,
  GRN_WAL_VALUE_SEGMENT_TYPE,
  GRN_WAL_VALUE_BOOLEAN,
  GRN_WAL_VALUE_INT32,
  GRN_WAL_VALUE_UINT32,
  GRN_WAL_VALUE_INT64,
  GRN_WAL_VALUE_UINT64,
  GRN_WAL_VALUE_BINARY,
} grn_wal_value_type;

typedef enum {
  GRN_WAL_READER_DATA_NIL,
  GRN_WAL_READER_DATA_BOOLEAN,
  GRN_WAL_READER_DATA_INT64,
  GRN_WAL_READER_DATA_UINT64,
  GRN_WAL_READER_DATA_FLOAT32,
  GRN_WAL_READER_DATA_FLOAT64,
  GRN_WAL_READER_DATA_STRING,
  GRN_WAL_READER_DATA_BINARY,
} grn_wal_reader_data_type;

const char *
grn_wal_reader_data_type_to_string(grn_wal_reader_data_type type);

typedef struct {
  grn_wal_reader_data_type type;
  union {
    bool boolean;
    int64_t int64;
    uint64_t uint64;
    float float32;
    double float64;
    struct {
      const char *data;
      size_t size;
    } string;
    struct {
      const void *data;
      size_t size;
    } binary;
  } content;
} grn_wal_reader_data;

typedef struct {
  uint64_t id;
  grn_wal_event event;
  grn_id record_id;
  uint8_t record_direction;
  uint32_t element_size;
  grn_wal_reader_data key;
  uint32_t key_hash_value;
  uint64_t key_offset;
  uint64_t shared_key_offset;
  bool is_shared;
  uint16_t check;
  grn_wal_reader_data new_key;
  grn_id parent_record_id;
  uint8_t parent_record_direction;
  uint16_t parent_check;
  grn_id grandparent_record_id;
  grn_id otherside_record_id;
  uint16_t otherside_check;
  grn_id left_record_id;
  grn_id right_record_id;
  grn_wal_reader_data value;
  uint32_t index_hash_value;
  uint32_t segment;
  uint32_t position;
  grn_wal_segment_type segment_type;
  uint32_t segment_info;
  uint32_t garbage_segment;
  uint32_t garbage_segment_head;
  uint32_t garbage_segment_tail;
  uint32_t garbage_segment_n_records;
  uint32_t previous_garbage_segment;
  uint32_t next_garbage_segment;
  grn_id next_garbage_record_id;
  uint32_t n_garbages;
  uint32_t n_entries;
  uint32_t max_offset;
  uint32_t expected_n_entries;
  uint32_t delete_info_index;
  uint32_t delete_info_phase1_index;
  uint32_t delete_info_phase2_index;
  uint32_t delete_info_phase3_index;
} grn_wal_reader_entry;

uint64_t
grn_wal_generate_id(grn_ctx *ctx);

grn_rc
grn_wal_add_entry(grn_ctx *ctx,
                  grn_obj *obj,
                  bool need_lock,
                  uint64_t *wal_id,
                  const char *tag,
                  grn_wal_key key,
                  ...);
grn_rc
grn_wal_add_entryv(grn_ctx *ctx,
                   grn_obj *obj,
                   bool need_lock,
                   uint64_t *wal_id,
                   const char *tag,
                   grn_wal_key key,
                   va_list args);
grn_rc
grn_wal_touch(grn_ctx *ctx,
              grn_obj *obj,
              bool need_lock,
              const char *tag);
bool
grn_wal_exist(grn_ctx *ctx, grn_obj *obj);
grn_rc
grn_wal_remove(grn_ctx *ctx,
               const char *path,
               const char *tag);
grn_rc
grn_wal_clear(grn_ctx *ctx,
              grn_obj *obj,
              bool need_lock,
              const char *tag);

typedef struct grn_wal_reader_ grn_wal_reader;
grn_wal_reader *
grn_wal_reader_open(grn_ctx *ctx,
                    grn_obj *obj,
                    const char *tag);
grn_rc
grn_wal_reader_close(grn_ctx *ctx,
                     grn_wal_reader *reader);
grn_rc
grn_wal_reader_read_entry(grn_ctx *ctx,
                          grn_wal_reader *reader,
                          grn_wal_reader_entry *entry);

void
grn_wal_set_recover_error(grn_ctx *ctx,
                          grn_rc rc,
                          grn_obj *object,
                          grn_wal_reader_entry *entry,
                          const char *tag,
                          const char *message);

grn_rc
grn_wal_dump(grn_ctx *ctx, grn_obj *obj);


#ifdef __cplusplus
}
#endif
