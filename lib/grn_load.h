/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2017  Brazil
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

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_JSON_LOAD_OPEN_BRACKET 0x40000000
#define GRN_JSON_LOAD_OPEN_BRACE   0x40000001

typedef struct grn_load_input_ {
  grn_content_type type;
  grn_raw_string table;
  grn_raw_string columns;
  grn_raw_string values;
  grn_raw_string if_exists;
  grn_raw_string each;
  grn_bool output_ids;
  grn_bool output_errors;
  grn_bool lock_table;
  uint32_t emit_level;
} grn_load_input;

void grn_load_internal(grn_ctx *ctx, grn_load_input *input);

typedef enum {
  GRN_LOADER_BEGIN = 0,
  GRN_LOADER_TOKEN,
  GRN_LOADER_STRING,
  GRN_LOADER_SYMBOL,
  GRN_LOADER_NUMBER,
  GRN_LOADER_STRING_ESC,
  GRN_LOADER_UNICODE0,
  GRN_LOADER_UNICODE1,
  GRN_LOADER_UNICODE2,
  GRN_LOADER_UNICODE3,
  GRN_LOADER_END
} grn_loader_stat;

/*
 * Status of target columns used in Format 1.
 * Target columns are specified via --columns or the first array in a Format 1
 * JSON object.
 */
typedef enum {
  GRN_LOADER_COLUMNS_UNSET = 0, /* Columns are not available. */
  GRN_LOADER_COLUMNS_SET,       /* Columns are available. */
  GRN_LOADER_COLUMNS_BROKEN     /* Columns are specified but broken. */
} grn_loader_columns_status;

typedef struct {
  grn_obj values;
  grn_obj level;
  grn_hash *columns;
  grn_obj ranges;
  grn_obj indexes;
  grn_obj ids;
  grn_obj return_codes;
  grn_obj error_messages;
  uint32_t emit_level;
  int32_t id_offset;  /* Position of _id in values or -1 if _id is N/A. */
  int32_t key_offset; /* Position of _key in values or -1 if _key is N/A. */
  grn_obj *last;
  grn_obj *table;
  grn_obj *ifexists;
  grn_obj *each;
  uint32_t unichar;
  uint32_t unichar_hi;
  uint32_t values_size;
  uint32_t n_records;
  uint32_t n_record_errors;
  uint32_t n_column_errors;
  grn_loader_stat stat;
  grn_content_type input_type;
  grn_loader_columns_status columns_status;
  grn_rc rc;
  char errbuf[GRN_CTX_MSGSIZE];
  grn_bool output_ids;
  grn_bool output_errors;
  grn_bool lock_table;
} grn_loader;

typedef struct {
  grn_obj *table;
  uint32_t depth;
  grn_obj *record_value;
  grn_id id;
  grn_obj *key;
  struct {
    const char *column_name;
    uint32_t column_name_size;
    grn_obj *column;
    grn_obj *value;
  } current;
} grn_loader_add_record_data;

void
grn_loader_save_error(grn_ctx *ctx, grn_loader *loader);
void
grn_loader_on_record_added(grn_ctx *ctx,
                           grn_loader *loader,
                           grn_id id);
void
grn_loader_on_column_set(grn_ctx *ctx,
                         grn_loader *loader,
                         grn_loader_add_record_data *data);
void
grn_loader_on_no_identifier_error(grn_ctx *ctx,
                                  grn_loader *loader,
                                  grn_obj *table);
grn_obj *
grn_loader_get_column(grn_ctx *ctx,
                      grn_loader *loader,
                      const char *name,
                      size_t name_length);
void
grn_loader_apply_each(grn_ctx *ctx,
                      grn_loader *loader,
                      grn_id id);

void
grn_p_loader(grn_ctx *ctx, grn_loader *loader);

#ifdef __cplusplus
}
#endif
