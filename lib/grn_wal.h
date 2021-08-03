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
  GRN_WAL_KEY_VALUE,
  GRN_WAL_KEY_SEGMENT,
  GRN_WAL_KEY_SEGMENT_TYPE,
  GRN_WAL_KEY_SEGMENT_INFO,
  GRN_WAL_KEY_GARBAGE_SEGMENT,
  GRN_WAL_KEY_PREVIOUS_GARBAGE_SEGMENT,
  GRN_WAL_KEY_NEXT_GARBAGE_SEGMENT,
  GRN_WAL_KEY_GARBAGE_SEGMENT_HEAD,
  GRN_WAL_KEY_GARBAGE_SEGMENT_TAIL,
  GRN_WAL_KEY_GARBAGE_SEGMENT_N_RECORDS,
  GRN_WAL_KEY_N_GARBAGES,
  GRN_WAL_KEY_POSITION,
  GRN_WAL_KEY_ELEMENT_SIZE,
} grn_wal_key;

const char *
grn_wal_key_to_string(grn_wal_key key);

typedef enum {
  GRN_WAL_VALUE_NIL,
  GRN_WAL_VALUE_EVENT,
  GRN_WAL_VALUE_RECORD_ID,
  GRN_WAL_VALUE_SEGMENT_TYPE,
  GRN_WAL_VALUE_INT32,
  GRN_WAL_VALUE_UINT32,
  GRN_WAL_VALUE_INT64,
  GRN_WAL_VALUE_UINT64,
  GRN_WAL_VALUE_BINARY,
} grn_wal_value_type;

typedef enum {
  GRN_WAL_READER_VALUE_NIL,
  GRN_WAL_READER_VALUE_BOOLEAN,
  GRN_WAL_READER_VALUE_INT64,
  GRN_WAL_READER_VALUE_UINT64,
  GRN_WAL_READER_VALUE_FLOAT32,
  GRN_WAL_READER_VALUE_FLOAT64,
  GRN_WAL_READER_VALUE_STRING,
  GRN_WAL_READER_VALUE_BINARY,
} grn_wal_reader_value_type;

const char *
grn_wal_reader_value_type_to_string(grn_wal_reader_value_type type);

typedef struct {
  grn_wal_reader_value_type type;
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
  } data;
} grn_wal_reader_value;

typedef struct {
  uint64_t id;
  grn_wal_event event;
  grn_id record_id;
  uint32_t element_size;
  grn_wal_reader_value value;
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
  uint32_t n_garbages;
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
