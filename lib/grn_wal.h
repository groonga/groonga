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
  GRN_WAL_EVENT_SET,
} grn_wal_event_type;

typedef enum {
  GRN_WAL_KEY_END,
  GRN_WAL_KEY_ID,
  GRN_WAL_KEY_EVENT,
  GRN_WAL_KEY_RECORD_ID,
  GRN_WAL_KEY_VALUE,
} grn_wal_key_type;

const char *
grn_wal_key_type_to_string(grn_wal_key_type type);

typedef enum {
  GRN_WAL_VALUE_NIL,
  GRN_WAL_VALUE_EVENT,
  GRN_WAL_VALUE_RECORD_ID,
  GRN_WAL_VALUE_INT64,
  GRN_WAL_VALUE_UINT64,
  GRN_WAL_VALUE_BINARY,
} grn_wal_value_type;

typedef enum {
  GRN_WAL_READER_VALUE_NIL,
  GRN_WAL_READER_VALUE_INT64,
  GRN_WAL_READER_VALUE_UINT64,
  GRN_WAL_READER_VALUE_BINARY,
} grn_wal_reader_value_type;

const char *
grn_wal_reader_value_type_to_string(grn_wal_reader_value_type type);

uint64_t
grn_wal_generate_id(grn_ctx *ctx);

grn_rc
grn_wal_add_entry(grn_ctx *ctx,
                  grn_obj *obj,
                  bool need_lock,
                  uint64_t *wal_id,
                  const char *tag,
                  grn_wal_key_type key_type,
                  ...);
grn_rc
grn_wal_add_entryv(grn_ctx *ctx,
                   grn_obj *obj,
                   bool need_lock,
                   uint64_t *wal_id,
                   const char *tag,
                   grn_wal_key_type key_type,
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
                          grn_wal_key_type key_type,
                          ...);

grn_rc
grn_wal_dump(grn_ctx *ctx, grn_obj *obj);


#ifdef __cplusplus
}
#endif
