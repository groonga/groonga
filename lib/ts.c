/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

/* TS is an acronym for "Turbo Selector". */

#ifdef GRN_WITH_TS

#include "grn_ts.h"

#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "grn_ctx_impl.h"
#include "grn_dat.h"
#include "grn_db.h"
#include "grn_geo.h"
#include "grn_hash.h"
#include "grn_output.h"
#include "grn_pat.h"
#include "grn_store.h"
#include "grn_str.h"

/*-------------------------------------------------------------
 * Miscellaneous.
 */

enum { GRN_TS_BATCH_SIZE = 1024 };

/*-------------------------------------------------------------
 * Built-in data kinds.
 */

/* grn_ts_bool_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_bool_is_valid(grn_ts_bool value) {
  return GRN_TRUE;
}

/* grn_ts_int_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_int_is_valid(grn_ts_int value) {
  return GRN_TRUE;
}

/* grn_ts_float_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_float_is_valid(grn_ts_float value) {
  return !isnan(value);
}

/* grn_ts_time_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_time_is_valid(grn_ts_time value) {
  return GRN_TRUE;
}

/* grn_ts_text_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_text_is_valid(grn_ts_text value) {
  return value.ptr || !value.size;
}

/* grn_ts_geo_point_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_geo_point_is_valid(grn_ts_geo_point value) {
  return ((value.latitude >= GRN_GEO_MIN_LATITUDE) &&
          (value.latitude <= GRN_GEO_MAX_LATITUDE)) &&
         ((value.longitude >= GRN_GEO_MIN_LONGITUDE) &&
          (value.longitude <= GRN_GEO_MAX_LONGITUDE));
}

#define GRN_TS_VECTOR_IS_VALID(type)\
  if (value.size) {\
    size_t i;\
    if (!value.ptr) {\
      return GRN_FALSE;\
    }\
    for (i = 0; i < value.size; i++) {\
      if (!grn_ts_ ## type ## _is_valid(value.ptr[i])) {\
        return GRN_FALSE;\
      }\
    }\
  }\
  return GRN_TRUE;
/* grn_ts_bool_vector_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_bool_vector_is_valid(grn_ts_bool_vector value) {
  GRN_TS_VECTOR_IS_VALID(bool)
}

/* grn_ts_int_vector_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_int_vector_is_valid(grn_ts_int_vector value) {
  GRN_TS_VECTOR_IS_VALID(int)
}

/* grn_ts_float_vector_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_float_vector_is_valid(grn_ts_float_vector value) {
  GRN_TS_VECTOR_IS_VALID(float)
}

/* grn_ts_time_vector_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_time_vector_is_valid(grn_ts_time_vector value) {
  GRN_TS_VECTOR_IS_VALID(time)
}

/* grn_ts_text_vector_is_valid() returns whether a value is valid or not. */
inline static grn_bool
grn_ts_text_vector_is_valid(grn_ts_text_vector value) {
  GRN_TS_VECTOR_IS_VALID(text)
}

/*
 * grn_ts_geo_point_vector_is_valid() returns whether a value is valid or
 * not.
 */
inline static grn_bool
grn_ts_geo_point_vector_is_valid(grn_ts_geo_point_vector value) {
  GRN_TS_VECTOR_IS_VALID(geo_point)
}
#undef GRN_TS_VECTOR_IS_VALID

/* grn_ts_bool_zero() returns a zero. */
inline static grn_ts_bool
grn_ts_bool_zero(void) {
  return GRN_FALSE;
}

/* grn_ts_int_zero() returns a zero. */
inline static grn_ts_int
grn_ts_int_zero(void) {
  return 0;
}

/* grn_ts_float_zero() returns a zero. */
inline static grn_ts_float
grn_ts_float_zero(void) {
  return 0.0;
}

/* grn_ts_time_zero() returns a zero. */
inline static grn_ts_time
grn_ts_time_zero(void) {
  return 0;
}

/* grn_ts_text_zero() returns a zero. */
inline static grn_ts_text
grn_ts_text_zero(void) {
  return (grn_ts_text){ NULL, 0 };
}

/* grn_ts_geo_point_zero() returns a zero. */
inline static grn_ts_geo_point
grn_ts_geo_point_zero(void) {
  return (grn_ts_geo_point){ 0, 0 };
}

/* grn_ts_geo_point_zero() returns a zero. */
inline static grn_ts_ref
grn_ts_ref_zero(void) {
  return (grn_ts_ref){ 0, 0.0 };
}

/* grn_ts_bool_vector_zero() returns a zero. */
inline static grn_ts_bool_vector
grn_ts_bool_vector_zero(void) {
  return (grn_ts_bool_vector){ NULL, 0 };
}

/* grn_ts_int_vector_zero() returns a zero. */
inline static grn_ts_int_vector
grn_ts_int_vector_zero(void) {
  return (grn_ts_int_vector){ NULL, 0 };
}

/* grn_ts_float_vector_zero() returns a zero. */
inline static grn_ts_float_vector
grn_ts_float_vector_zero(void) {
  return (grn_ts_float_vector){ NULL, 0 };
}

/* grn_ts_time_vector_zero() returns a zero. */
inline static grn_ts_time_vector
grn_ts_time_vector_zero(void) {
  return (grn_ts_time_vector){ NULL, 0 };
}

/* grn_ts_text_vector_zero() returns a zero. */
inline static grn_ts_text_vector
grn_ts_text_vector_zero(void) {
  return (grn_ts_text_vector){ NULL, 0 };
}

/* grn_ts_geo_point_vector_zero() returns a zero. */
inline static grn_ts_geo_point_vector
grn_ts_geo_point_vector_zero(void) {
  return (grn_ts_geo_point_vector){ NULL, 0 };
}

/* grn_ts_ref_vector_zero() returns a zero. */
inline static grn_ts_ref_vector
grn_ts_ref_vector_zero(void) {
  return (grn_ts_ref_vector){ NULL, 0 };
}

/* grn_ts_bool_output() outputs a value. */
static grn_rc
grn_ts_bool_output(grn_ctx *ctx, grn_ts_bool value) {
  if (value) {
    return grn_bulk_write(ctx, ctx->impl->outbuf, "true", 4);
  } else {
    return grn_bulk_write(ctx, ctx->impl->outbuf, "false", 5);
  }
}

/* grn_ts_int_output() outputs a value. */
static grn_rc
grn_ts_int_output(grn_ctx *ctx, grn_ts_int value) {
  return grn_text_lltoa(ctx, ctx->impl->outbuf, value);
}

/* grn_ts_float_output() outputs a value. */
static grn_rc
grn_ts_float_output(grn_ctx *ctx, grn_ts_float value) {
  return grn_text_ftoa(ctx, ctx->impl->outbuf, value);
}

/* grn_ts_time_output() outputs a value. */
static grn_rc
grn_ts_time_output(grn_ctx *ctx, grn_ts_time value) {
  return grn_text_ftoa(ctx, ctx->impl->outbuf, value * 0.000001);
}

/* grn_ts_text_output() outputs a value. */
static grn_rc
grn_ts_text_output(grn_ctx *ctx, grn_ts_text value) {
  return grn_text_esc(ctx, ctx->impl->outbuf, value.ptr, value.size);
}

/* grn_ts_geo_point_output() outputs a value. */
static grn_rc
grn_ts_geo_point_output(grn_ctx *ctx, grn_ts_geo_point value) {
  grn_rc rc = grn_bulk_write(ctx, ctx->impl->outbuf, "\"", 1);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_text_itoa(ctx, ctx->impl->outbuf, value.latitude);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_bulk_write(ctx, ctx->impl->outbuf, "x", 1);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_text_itoa(ctx, ctx->impl->outbuf, value.longitude);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return grn_bulk_write(ctx, ctx->impl->outbuf, "\"", 1);
}

#define GRN_TS_VECTOR_OUTPUT(kind)\
  size_t i;\
  grn_rc rc = grn_bulk_write(ctx, ctx->impl->outbuf, "[", 1);\
  if (rc != GRN_SUCCESS) {\
    return rc;\
  }\
  for (i = 0; i < value.size; ++i) {\
    if (i) {\
      rc = grn_bulk_write(ctx, ctx->impl->outbuf, ",", 1);\
      if (rc != GRN_SUCCESS) {\
        return rc;\
      }\
    }\
    rc = grn_ts_ ## kind ## _output(ctx, value.ptr[i]);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
  }\
  return grn_bulk_write(ctx, ctx->impl->outbuf, "]", 1);
/* grn_ts_bool_vector_output() outputs a value. */
static grn_rc
grn_ts_bool_vector_output(grn_ctx *ctx, grn_ts_bool_vector value) {
  GRN_TS_VECTOR_OUTPUT(bool)
}

/* grn_ts_int_vector_output() outputs a value. */
static grn_rc
grn_ts_int_vector_output(grn_ctx *ctx, grn_ts_int_vector value) {
  GRN_TS_VECTOR_OUTPUT(int)
}

/* grn_ts_float_vector_output() outputs a value. */
static grn_rc
grn_ts_float_vector_output(grn_ctx *ctx, grn_ts_float_vector value) {
  GRN_TS_VECTOR_OUTPUT(float)
}

/* grn_ts_time_vector_output() outputs a value. */
static grn_rc
grn_ts_time_vector_output(grn_ctx *ctx, grn_ts_time_vector value) {
  GRN_TS_VECTOR_OUTPUT(time)
}

/* grn_ts_text_vector_output() outputs a value. */
static grn_rc
grn_ts_text_vector_output(grn_ctx *ctx, grn_ts_text_vector value) {
  GRN_TS_VECTOR_OUTPUT(text)
}

/* grn_ts_geo_point_vector_output() outputs a value. */
static grn_rc
grn_ts_geo_point_vector_output(grn_ctx *ctx, grn_ts_geo_point_vector value) {
  GRN_TS_VECTOR_OUTPUT(geo_point)
}
#undef GRN_TS_VECTOR_OUTPUT

/* grn_ts_data_type_to_kind() returns a kind associated with a type. */
static grn_ts_data_kind
grn_ts_data_type_to_kind(grn_ts_data_type type) {
  switch (type) {
    case GRN_DB_VOID: {
      return GRN_TS_VOID;
    }
    case GRN_DB_BOOL: {
      return GRN_TS_BOOL;
    }
    case GRN_DB_INT8:
    case GRN_DB_INT16:
    case GRN_DB_INT32:
    case GRN_DB_INT64:
    case GRN_DB_UINT8:
    case GRN_DB_UINT16:
    case GRN_DB_UINT32:
    case GRN_DB_UINT64: {
      return GRN_TS_INT;
    }
    case GRN_DB_FLOAT: {
      return GRN_TS_FLOAT;
    }
    case GRN_DB_TIME: {
      return GRN_TS_TIME;
    }
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      return GRN_TS_TEXT;
    }
    case GRN_DB_TOKYO_GEO_POINT:
    case GRN_DB_WGS84_GEO_POINT: {
      return GRN_TS_GEO_POINT;
    }
    default: {
      return GRN_TS_REF;
    }
  }
}

/* grn_ts_data_kind_to_type() returns a type associated with a kind. */
static grn_ts_data_type
grn_ts_data_kind_to_type(grn_ts_data_kind kind) {
  switch (kind & ~GRN_TS_VECTOR_FLAG) {
    case GRN_TS_BOOL: {
      return GRN_DB_BOOL;
    }
    case GRN_TS_INT: {
      return GRN_DB_INT64;
    }
    case GRN_TS_FLOAT: {
      return GRN_DB_FLOAT;
    }
    case GRN_TS_TIME: {
      return GRN_DB_TIME;
    }
    case GRN_TS_TEXT: {
      return GRN_DB_TEXT;
    }
    case GRN_TS_GEO_POINT: {
      /* GRN_DB_TOKYO_GEO_POINT or GRN_DB_WGS84_GEO_POINT. */
      return GRN_DB_VOID;
    }
    default: {
      return GRN_DB_VOID;
    }
  }
}

/*-------------------------------------------------------------
 * Groonga objects.
 */

/*
 * grn_ts_obj_increment_ref_count() increments the reference count of an
 * object.
 */
static grn_rc
grn_ts_obj_increment_ref_count(grn_ctx *ctx, grn_obj *obj) {
  grn_obj *obj_clone = grn_ctx_at(ctx, grn_obj_id(ctx, obj));
  if (!obj_clone) {
    return GRN_INVALID_ARGUMENT;
  }
  if (obj_clone != obj) {
    grn_obj_unlink(ctx, obj_clone);
    return GRN_INVALID_ARGUMENT;
  }
  return GRN_SUCCESS;
}

/* grn_ts_obj_is_table() returns whether an object is a column or not */
static grn_bool
grn_ts_obj_is_table(grn_ctx *ctx, grn_obj *obj) {
  return grn_obj_is_table(ctx, obj);
}

/* grn_ts_obj_is_column() returns whether an object is a column or not */
static grn_bool
grn_ts_obj_is_column(grn_ctx *ctx, grn_obj *obj) {
  switch (obj->header.type) {
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE: {
      return GRN_TRUE;
    }
    /* GRN_COLUMN_INDEX is not supported. */
    default: {
      return GRN_FALSE;
    }
  }
}

/* grn_ts_ja_get_value() appends a value into buf. */
static grn_rc
grn_ts_ja_get_value(grn_ctx *ctx, grn_ja *ja, grn_id id,
                    grn_obj *buf, size_t *value_size) {
  grn_rc rc, tmp_rc;
  uint32_t size;
  grn_io_win iw;
  char *ptr = (char *)grn_ja_ref(ctx, ja, id, &iw, &size);
  if (!ptr) {
    if (value_size) {
      *value_size = 0;
    }
    return GRN_SUCCESS;
  }
  rc = grn_bulk_write(ctx, buf, ptr, size);
  tmp_rc = grn_ja_unref(ctx, &iw);
  if (rc == GRN_SUCCESS) {
    if (tmp_rc == GRN_SUCCESS) {
      if (value_size) {
        *value_size = size;
      }
    } else {
      /* Discard the value read. */
      grn_bulk_resize(ctx, buf, GRN_BULK_VSIZE(buf) - size);
      rc = tmp_rc;
    }
  }
  return rc;
}

/* grn_ts_table_has_key() returns whether a table has _key or not. */
static grn_bool
grn_ts_table_has_key(grn_ctx *ctx, grn_obj *table) {
  switch (table->header.type) {
    case GRN_TABLE_HASH_KEY:
    case GRN_TABLE_PAT_KEY:
    case GRN_TABLE_DAT_KEY: {
      return GRN_TRUE;
    }
    default: {
      return GRN_FALSE;
    }
  }
}

/* grn_ts_table_has_value() returns whether a table has _value or not. */
static grn_bool
grn_ts_table_has_value(grn_ctx *ctx, grn_obj *table) {
  return DB_OBJ(table)->range != GRN_DB_VOID;
}

#define GRN_TS_TABLE_GET_KEY_CASE_BLOCK(TYPE, type)\
  case GRN_TABLE_ ## TYPE ## _KEY: {\
    uint32_t key_size;\
    grn_ ## type *type = (grn_ ## type *)table;\
    const void *key = _grn_ ## type ## _key(ctx, type, id, &key_size);\
    if (size) {\
      *size = key_size;\
    }\
    return key;\
  }
/* grn_ts_table_get_key() gets a reference to a key (_key). */
static const void *
grn_ts_table_get_key(grn_ctx *ctx, grn_obj *table, grn_id id, size_t *size) {
  switch (table->header.type) {
    GRN_TS_TABLE_GET_KEY_CASE_BLOCK(HASH, hash)
    GRN_TS_TABLE_GET_KEY_CASE_BLOCK(PAT, pat)
    GRN_TS_TABLE_GET_KEY_CASE_BLOCK(DAT, dat)
    /* GRN_TABLE_NO_KEY does not support _key. */
    default: {
      return NULL;
    }
  }
}
#undef GRN_TS_TABLE_GET_KEY_CASE_BLOCK

/* grn_ts_table_get_value() gets a reference to a value (_value). */
static const void *
grn_ts_table_get_value(grn_ctx *ctx, grn_obj *table, grn_id id) {
  switch (table->header.type) {
    case GRN_TABLE_HASH_KEY: {
      uint32_t size;
      return grn_hash_get_value_(ctx, (grn_hash *)table, id, &size);
    }
    case GRN_TABLE_PAT_KEY: {
      uint32_t size;
      return grn_pat_get_value_(ctx, (grn_pat *)table, id, &size);
    }
    /* GRN_TABLE_DAT_KEY does not support _value. */
    case GRN_TABLE_NO_KEY: {
      return _grn_array_get_value(ctx, (grn_array *)table, id);
    }
    default: {
      return NULL;
    }
  }
}

/*-------------------------------------------------------------
 * grn_ts_expr_node.
 */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
} grn_ts_expr_id_node;

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
} grn_ts_expr_score_node;

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_obj *table;
  grn_obj buf;
} grn_ts_expr_key_node;

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_obj *table;
} grn_ts_expr_value_node;

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  union {
    grn_ts_bool bool_value;
    grn_ts_int int_value;
    grn_ts_float float_value;
    grn_ts_time time_value;
    grn_ts_text text_value;
    grn_ts_geo_point geo_point_value;
    grn_ts_bool_vector bool_vector_value;
    grn_ts_int_vector int_vector_value;
    grn_ts_float_vector float_vector_value;
    grn_ts_time_vector time_vector_value;
    grn_ts_text_vector text_vector_value;
    grn_ts_geo_point_vector geo_point_vector_value;
  } content;
  char *text_buf;
  void *vector_buf;
} grn_ts_expr_const_node;

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_obj *column;
  grn_obj buf;
  union {
    void *buf;
    struct {
      grn_ts_text *ptr;
      size_t size;
    } text_buf;
    struct {
      grn_ts_ref *ptr;
      size_t size;
    } ref_buf;
  } body;
} grn_ts_expr_column_node;

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_ts_op_type op_type;
  // TODO
} grn_ts_expr_op_node;

/* grn_ts_expr_node_fin() finalizes a node. */
static grn_rc
grn_ts_expr_node_fin(grn_ctx *ctx, grn_ts_expr_node *node) {
  switch (node->type) {
    case GRN_TS_EXPR_ID_NODE:
    case GRN_TS_EXPR_SCORE_NODE: {
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_KEY_NODE: {
      grn_ts_expr_key_node *key_node = (grn_ts_expr_key_node *)node;
      GRN_OBJ_FIN(ctx, &key_node->buf);
      if (key_node->table) {
        grn_obj_unlink(ctx, key_node->table);
      }
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_VALUE_NODE: {
      grn_ts_expr_value_node *value_node = (grn_ts_expr_value_node *)node;
      if (value_node->table) {
        grn_obj_unlink(ctx, value_node->table);
      }
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_CONST_NODE: {
      grn_ts_expr_const_node *const_node = (grn_ts_expr_const_node *)node;
      if (const_node->vector_buf) {
        GRN_FREE(const_node->vector_buf);
      }
      if (const_node->text_buf) {
        GRN_FREE(const_node->text_buf);
      }
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_COLUMN_NODE: {
      grn_ts_expr_column_node *column_node = (grn_ts_expr_column_node *)node;
      if (column_node->body.buf) {
        GRN_FREE(column_node->body.buf);
      }
      GRN_OBJ_FIN(ctx, &column_node->buf);
      if (column_node->column) {
        grn_obj_unlink(ctx, column_node->column);
      }
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_OP_NODE: {
      // TODO: Unlink objects and free memory.
      return GRN_SUCCESS;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_expr_node_close() destroys a node. */
static grn_rc
grn_ts_expr_node_close(grn_ctx *ctx, grn_ts_expr_node *node) {
  grn_rc rc;
  if (!node) {
    return GRN_SUCCESS;
  }
  rc = grn_ts_expr_node_fin(ctx, node);
  GRN_FREE(node);
  return rc;
}

/* grn_ts_expr_id_node_open() creates a node associated with ID (_id). */
static grn_rc
grn_ts_expr_id_node_open(grn_ctx *ctx, grn_ts_expr_node **node) {
  grn_ts_expr_id_node *new_node = GRN_MALLOCN(grn_ts_expr_id_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  new_node->type = GRN_TS_EXPR_ID_NODE;
  new_node->data_kind = GRN_TS_INT;
  new_node->data_type = GRN_DB_UINT32;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_score_node_open() creates a node associated with score
 * (_score).
 */
static grn_rc
grn_ts_expr_score_node_open(grn_ctx *ctx, grn_ts_expr_node **node) {
  grn_ts_expr_score_node *new_node = GRN_MALLOCN(grn_ts_expr_score_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  new_node->type = GRN_TS_EXPR_SCORE_NODE;
  new_node->data_kind = GRN_TS_FLOAT;
  new_node->data_type = GRN_DB_FLOAT;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_key_node_open() creates a node associated with key (_key). */
static grn_rc
grn_ts_expr_key_node_open(grn_ctx *ctx, grn_obj *table,
                          grn_ts_expr_node **node) {
  grn_rc rc;
  grn_ts_expr_key_node *new_node;
  if (!grn_ts_table_has_key(ctx, table)) {
    return GRN_INVALID_ARGUMENT;
  }
  new_node = GRN_MALLOCN(grn_ts_expr_key_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_node);
    return rc;
  }
  new_node->type = GRN_TS_EXPR_KEY_NODE;
  new_node->data_kind = grn_ts_data_type_to_kind(table->header.domain);
  new_node->data_type = table->header.domain;
  new_node->table = table;
  GRN_TEXT_INIT(&new_node->buf, 0);
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_value_node_open() creates a node associated with value
 * (_value).
 */
static grn_rc
grn_ts_expr_value_node_open(grn_ctx *ctx, grn_obj *table,
                            grn_ts_expr_node **node) {
  grn_rc rc;
  grn_ts_expr_value_node *new_node;
  if (!grn_ts_table_has_value(ctx, table)) {
    return GRN_INVALID_ARGUMENT;
  }
  new_node = GRN_MALLOCN(grn_ts_expr_value_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  memset(new_node, 0, sizeof(*new_node));
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_node);
    return rc;
  }
  new_node->type = GRN_TS_EXPR_VALUE_NODE;
  new_node->data_kind = grn_ts_data_type_to_kind(DB_OBJ(table)->range);
  new_node->data_type = DB_OBJ(table)->range;
  new_node->table = table;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_CONST_NODE_OPEN_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    node->content.kind ## _value = *(const grn_ts_ ## kind *)value;\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_const_node_init_scalar() initializes a const scalar node. */
static grn_rc
grn_ts_expr_const_node_init_scalar(grn_ctx *ctx, grn_ts_expr_const_node *node,
                                   const void *value) {
  switch (node->data_kind) {
    GRN_TS_EXPR_CONST_NODE_OPEN_CASE_BLOCK(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_OPEN_CASE_BLOCK(INT, int)
    GRN_TS_EXPR_CONST_NODE_OPEN_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_OPEN_CASE_BLOCK(TIME, time)
    case GRN_TS_TEXT: {
      grn_ts_text text_value;
      char *text_buf;
      text_value = *(const grn_ts_text *)value;
      if (!text_value.size) {
        node->content.text_value.ptr = NULL;
        node->content.text_value.size = 0;
        return GRN_SUCCESS;
      }
      text_buf = (char *)GRN_MALLOC(text_value.size);
      if (!text_buf) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      node->text_buf = text_buf;
      grn_memcpy(text_buf, text_value.ptr, text_value.size);
      node->content.text_value.ptr = text_buf;
      node->content.text_value.size = text_value.size;
      return GRN_SUCCESS;
    }
    GRN_TS_EXPR_CONST_NODE_OPEN_CASE_BLOCK(GEO_POINT, geo_point)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_CONST_NODE_OPEN_CASE_BLOCK

#define GRN_TS_EXPR_CONST_NODE_OPEN_VECTOR_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND ## _VECTOR: {\
    grn_ts_ ## kind ## _vector vector;\
    grn_ts_ ## kind *vector_buf;\
    vector = *(const grn_ts_ ## kind ## _vector *)value;\
    if (!vector.size) {\
      node->content.kind ## _vector_value.ptr = NULL;\
      node->content.kind ## _vector_value.size = 0;\
      return GRN_SUCCESS;\
    }\
    vector_buf = GRN_MALLOCN(grn_ts_ ## kind, vector.size);\
    if (!vector_buf) {\
      return GRN_NO_MEMORY_AVAILABLE;\
    }\
    node->vector_buf = vector_buf;\
    grn_memcpy(vector_buf, vector.ptr,\
               sizeof(grn_ts_ ## kind) * vector.size);\
    node->content.kind ## _vector_value.ptr = vector_buf;\
    node->content.kind ## _vector_value.size = vector.size;\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_const_node_init_vector() initializes a const vector node. */
static grn_rc
grn_ts_expr_const_node_init_vector(grn_ctx *ctx, grn_ts_expr_const_node *node,
                                   const void *value) {
  switch (node->data_kind) {
    GRN_TS_EXPR_CONST_NODE_OPEN_VECTOR_CASE_BLOCK(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_OPEN_VECTOR_CASE_BLOCK(INT, int)
    GRN_TS_EXPR_CONST_NODE_OPEN_VECTOR_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_OPEN_VECTOR_CASE_BLOCK(TIME, time)
    case GRN_TS_TEXT_VECTOR: {
      size_t i, offset = 0, total_size = 0;
      grn_ts_text_vector vector;
      grn_ts_text *vector_buf;
      char *text_buf;
      vector = *(const grn_ts_text_vector *)value;
      if (!vector.size) {
        node->content.text_vector_value.ptr = NULL;
        node->content.text_vector_value.size = 0;
        return GRN_SUCCESS;
      }
      vector_buf = GRN_MALLOCN(grn_ts_text, vector.size);
      if (!vector_buf) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      node->vector_buf = vector_buf;
      for (i = 0; i < vector.size; i++) {
        total_size += vector.ptr[i].size;
      }
      if (total_size) {
        text_buf = (char *)GRN_MALLOC(total_size);
        if (!text_buf) {
          return GRN_NO_MEMORY_AVAILABLE;
        }
        node->text_buf = text_buf;
      }
      for (i = 0; i < vector.size; i++) {
        grn_memcpy(text_buf + offset, vector.ptr[i].ptr, vector.ptr[i].size);
        vector_buf[i].ptr = text_buf + offset;
        vector_buf[i].size = vector.ptr[i].size;
        offset += vector.ptr[i].size;
      }
      node->content.text_vector_value.ptr = vector_buf;
      node->content.text_vector_value.size = vector.size;
      return GRN_SUCCESS;
    }
    GRN_TS_EXPR_CONST_NODE_OPEN_VECTOR_CASE_BLOCK(GEO_POINT, geo_point)
    default: {
      return GRN_UNKNOWN_ERROR;
    }
  }
}
#undef GRN_TS_EXPR_CONST_NODE_OPEN_VECTOR_CASE_BLOCK

/* grn_ts_expr_const_node_open() creates a node associated with a const. */
static grn_rc
grn_ts_expr_const_node_open(grn_ctx *ctx, grn_ts_data_kind kind,
                            const void *value, grn_ts_expr_node **node) {
  grn_rc rc = GRN_SUCCESS;
  grn_ts_expr_const_node *new_node = GRN_MALLOCN(grn_ts_expr_const_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  new_node->type = GRN_TS_EXPR_CONST_NODE;
  new_node->data_kind = kind;
  new_node->data_type = grn_ts_data_kind_to_type(kind);
  new_node->text_buf = NULL;
  new_node->vector_buf = NULL;
  if (kind & GRN_TS_VECTOR_FLAG) {
    rc = grn_ts_expr_const_node_init_vector(ctx, new_node, value);
  } else {
    rc = grn_ts_expr_const_node_init_scalar(ctx, new_node, value);
  }
  if (rc != GRN_SUCCESS) {
    grn_ts_expr_node_close(ctx, (grn_ts_expr_node *)new_node);
    return rc;
  }
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(TYPE)\
  case GRN_DB_ ## TYPE: {\
    GRN_ ## TYPE ## _INIT(&new_node->buf, GRN_OBJ_VECTOR);\
    break;\
  }
/* grn_ts_expr_column_node_open() creates a node associated with a column. */
static grn_rc
grn_ts_expr_column_node_open(grn_ctx *ctx, grn_obj *column,
                             grn_ts_expr_node **node) {
  grn_rc rc;
  grn_ts_expr_column_node *new_node = GRN_MALLOCN(grn_ts_expr_column_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  memset(new_node, 0, sizeof(*new_node));
  rc = grn_ts_obj_increment_ref_count(ctx, column);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_node);
    return rc;
  }
  new_node->type = GRN_TS_EXPR_COLUMN_NODE;
  new_node->data_kind = grn_ts_data_type_to_kind(DB_OBJ(column)->range);
  if (column->header.type == GRN_COLUMN_VAR_SIZE) {
    grn_obj_flags type = column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK;
    if (type == GRN_OBJ_COLUMN_VECTOR) {
      new_node->data_kind |= GRN_TS_VECTOR_FLAG;
    }
  }
  new_node->data_type = DB_OBJ(column)->range;
  new_node->column = column;
  if (new_node->data_kind & GRN_TS_VECTOR_FLAG) {
    switch (new_node->data_type) {
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(BOOL)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(INT8)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(INT16)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(INT32)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(INT64)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(UINT8)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(UINT16)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(UINT32)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(UINT64)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(FLOAT)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(TIME)
      case GRN_DB_SHORT_TEXT:
      case GRN_DB_TEXT:
      case GRN_DB_LONG_TEXT: {
        GRN_TEXT_INIT(&new_node->buf, 0);
        break;
      }
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(TOKYO_GEO_POINT)
      GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK(WGS84_GEO_POINT)
      default: {
        break;
      }
    }
  } else {
    GRN_TEXT_INIT(&new_node->buf, 0);
  }
  new_node->body.buf = NULL;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}
#undef GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE_BLOCK

/* grn_ts_expr_id_node_evaluate() outputs IDs. */
static grn_rc
grn_ts_expr_id_node_evaluate(grn_ctx *ctx, grn_ts_expr_id_node *node,
                             const grn_ts_record *in, size_t n_in,
                             void *out) {
  size_t i;
  grn_ts_int *out_ptr = (grn_ts_int *)out;
  for (i = 0; i < n_in; i++) {
    out_ptr[i] = (grn_ts_int)in[i].id;
  }
  return GRN_SUCCESS;
}

/* grn_ts_expr_score_node_evaluate() outputs scores. */
static grn_rc
grn_ts_expr_score_node_evaluate(grn_ctx *ctx, grn_ts_expr_score_node *node,
                                const grn_ts_record *in, size_t n_in,
                                void *out) {
  size_t i;
  grn_ts_float *out_ptr = (grn_ts_float *)out;
  for (i = 0; i < n_in; i++) {
    out_ptr[i] = (grn_ts_float)in[i].score;
  }
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    size_t i;\
    grn_ts_ ## kind *out_ptr = (grn_ts_ ## kind *)out;\
    for (i = 0; i < n_in; i++) {\
      const grn_ts_ ## kind *key;\
      size_t key_size;\
      key = (const grn_ts_ ## kind *)grn_ts_table_get_key(ctx, node->table,\
                                                          in[i].id,\
                                                          &key_size);\
      if (key && (key_size == sizeof(*key))) {\
        out_ptr[i] = *key;\
      } else {\
        out_ptr[i] = grn_ts_ ## kind ## _zero();\
      }\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(TYPE, type)\
  case GRN_DB_ ## TYPE: {\
    size_t i;\
    grn_ts_int *out_ptr = (grn_ts_int *)out;\
    for (i = 0; i < n_in; i++) {\
      const type ## _t *key;\
      size_t key_size;\
      key = (const type ## _t *)grn_ts_table_get_key(ctx, node->table,\
                                                     in[i].id, &key_size);\
      if (key && (key_size == sizeof(*key))) {\
        out_ptr[i] = (grn_ts_int)*key;\
      } else {\
        out_ptr[i] = grn_ts_int_zero();\
      }\
    }\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_key_node_evaluate() outputs keys. */
static grn_rc
grn_ts_expr_key_node_evaluate(grn_ctx *ctx, grn_ts_expr_key_node *node,
                              const grn_ts_record *in, size_t n_in,
                              void *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE_BLOCK(BOOL, bool)
    case GRN_TS_INT: {
      switch (node->data_type) {
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(INT8, int8)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(INT16, int16)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(INT32, int32)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(INT64, int64)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(UINT8, uint8)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(UINT16, uint16)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(UINT32, uint32)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK(UINT64, uint64)
        default: {
          return GRN_OBJECT_CORRUPT;
        }
      }
    }
    GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE_BLOCK(TIME, time)
    case GRN_TS_TEXT: {
      size_t i;
      char *buf_ptr;
      grn_ts_text *out_ptr = (grn_ts_text *)out;
      GRN_BULK_REWIND(&node->buf);
      for (i = 0; i < n_in; i++) {
        const char *key;
        size_t key_size;
        key = (const char *)grn_ts_table_get_key(ctx, node->table, in[i].id,
                                                 &key_size);
        if (key && key_size) {
          grn_rc rc = grn_bulk_write(ctx, &node->buf, key, key_size);
          if (rc == GRN_SUCCESS) {
            out_ptr[i].size = key_size;
          }
        }
      }
      buf_ptr = GRN_BULK_HEAD(&node->buf);
      for (i = 0; i < n_in; i++) {
        out_ptr[i].ptr = buf_ptr;
        buf_ptr += out_ptr[i].size;
      }
      return GRN_SUCCESS;
    }
    GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE_BLOCK(GEO_POINT, geo_point)
    case GRN_TS_REF: {
      size_t i;
      grn_ts_ref *out_ptr = (grn_ts_ref *)out;
      for (i = 0; i < n_in; i++) {
        const grn_ts_id *id;
        size_t key_size;
        id = (const grn_ts_id *)grn_ts_table_get_key(ctx, node->table,
                                                     in[i].id, &key_size);
        if (id && (key_size == sizeof(*id))) {
          out_ptr[i].id = *id;
          out_ptr[i].score = in[i].score;
        } else {
          out_ptr[i] = grn_ts_ref_zero();
        }
      }
      return GRN_SUCCESS;
    }
    default: {
      return GRN_OBJECT_CORRUPT;
    }
  }
}
#undef GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE_BLOCK
#undef GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE_BLOCK

#define GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    size_t i;\
    grn_ts_ ## kind *out_ptr = (grn_ts_ ## kind *)out;\
    for (i = 0; i < n_in; i++) {\
      const grn_ts_ ## kind *value;\
      value = (const grn_ts_ ## kind *)grn_ts_table_get_value(ctx,\
                                                              node->table,\
                                                              in[i].id);\
      out_ptr[i] = value ? *value : grn_ts_ ## kind ## _zero();\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(TYPE, type)\
  case GRN_DB_ ## TYPE: {\
    size_t i;\
    grn_ts_int *out_ptr = (grn_ts_int *)out;\
    for (i = 0; i < n_in; i++) {\
      const type ## _t *value;\
      value = (const type ## _t *)grn_ts_table_get_value(ctx, node->table,\
                                                         in[i].id);\
      out_ptr[i] = value ? (grn_ts_int)*value : grn_ts_int_zero();\
    }\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_value_node_evaluate() outputs values. */
static grn_rc
grn_ts_expr_value_node_evaluate(grn_ctx *ctx, grn_ts_expr_value_node *node,
                                const grn_ts_record *in, size_t n_in,
                                void *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE_BLOCK(BOOL, bool)
    case GRN_TS_INT: {
      switch (node->data_type) {
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(INT8, int8)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(INT16, int16)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(INT32, int32)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(INT64, int64)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(UINT8, uint8)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(UINT16, uint16)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(UINT32, uint32)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK(UINT64, uint64)
        default: {
          return GRN_OBJECT_CORRUPT;
        }
      }
    }
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE_BLOCK(TIME, time)
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE_BLOCK(GEO_POINT, geo_point)
    case GRN_TS_REF: {
      size_t i;
      grn_ts_ref *out_ptr = (grn_ts_ref *)out;
      for (i = 0; i < n_in; i++) {
        const grn_ts_id *value;
        value = (const grn_ts_id *)grn_ts_table_get_value(ctx, node->table,
                                                          in[i].id);
        if (value) {
          out_ptr[i].id = *value;
          out_ptr[i].score = in[i].score;
        } else {
          out_ptr[i] = grn_ts_ref_zero();
        }
      }
      return GRN_SUCCESS;
    }
    default: {
      return GRN_OBJECT_CORRUPT;
    }
  }
}
#undef GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE_BLOCK
#undef GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE_BLOCK

#define GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    size_t i;\
    grn_ts_ ## kind *out_ptr = (grn_ts_ ## kind *)out;\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i] = node->content.kind ## _value;\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK(KIND, kind)\
  GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(KIND ## _VECTOR, kind ## _vector)
/* grn_ts_expr_const_node_evaluate() outputs consts. */
static grn_rc
grn_ts_expr_const_node_evaluate(grn_ctx *ctx, grn_ts_expr_const_node *node,
                                const grn_ts_record *in, size_t n_in,
                                void *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(INT, int)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(TIME, time)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(TEXT, text)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK(GEO_POINT, geo_point)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK(INT, int)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK(TIME, time)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK(TEXT, text)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK(GEO_POINT, geo_point)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE_BLOCK
#undef GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE_BLOCK

#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    size_t i;\
    grn_ts_ ## kind *out_ptr = (grn_ts_ ## kind *)out;\
    grn_ra *ra = (grn_ra *)node->column;\
    grn_ra_cache cache;\
    GRN_RA_CACHE_INIT(ra, &cache);\
    for (i = 0; i < n_in; i++) {\
      grn_ts_ ## kind *ptr = NULL;\
      if (in[i].id) {\
        ptr = (grn_ts_ ## kind *)grn_ra_ref_cache(ctx, ra, in[i].id, &cache);\
      }\
      out_ptr[i] = ptr ? *ptr : grn_ts_ ## kind ## _zero();\
    }\
    GRN_RA_CACHE_FIN(ra, &cache);\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(TYPE, type)\
  case GRN_DB_ ## TYPE: {\
    size_t i;\
    grn_ts_int *out_ptr = (grn_ts_int *)out;\
    grn_ra *ra = (grn_ra *)node->column;\
    grn_ra_cache cache;\
    GRN_RA_CACHE_INIT(ra, &cache);\
    for (i = 0; i < n_in; i++) {\
      type ## _t *ptr = NULL;\
      if (in[i].id) {\
        ptr = (type ## _t *)grn_ra_ref_cache(ctx, ra, in[i].id, &cache);\
      }\
      out_ptr[i] = ptr ? (grn_ts_int)*ptr : grn_ts_int_zero();\
    }\
    GRN_RA_CACHE_FIN(ra, &cache);\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_column_node_evaluate_scalar() outputs scalar column values. */
static grn_rc
grn_ts_expr_column_node_evaluate_scalar(grn_ctx *ctx,
                                        grn_ts_expr_column_node *node,
                                        const grn_ts_record *in, size_t n_in,
                                        void *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE_BLOCK(BOOL, bool)
    case GRN_TS_INT: {
      switch (node->data_type) {
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(INT8, int8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(INT16, int16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(INT32, int32)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(INT64, int64)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(UINT8, uint8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(UINT16, uint16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(UINT32, uint32)
        /* The behavior is undefined if a value is greater than 2^63 - 1. */
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK(UINT64, uint64)
        default: {
          return GRN_OBJECT_CORRUPT;
        }
      }
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE_BLOCK(TIME, time)
    case GRN_TS_TEXT: {
      size_t i, size;
      char *buf_ptr;
      grn_ja *ja = (grn_ja *)node->column;
      grn_ts_text *out_ptr = (grn_ts_text *)out;
      /* Read column values into node->buf and save the size of each value. */
      GRN_BULK_REWIND(&node->buf);
      for (i = 0; i < n_in; i++) {
        grn_rc rc = grn_ts_ja_get_value(ctx, ja, in[i].id, &node->buf, &size);
        out_ptr[i].size = (rc == GRN_SUCCESS) ? size : 0;
      }
      buf_ptr = GRN_BULK_HEAD(&node->buf);
      for (i = 0; i < n_in; i++) {
        out_ptr[i].ptr = buf_ptr;
        buf_ptr += out_ptr[i].size;
      }
      return GRN_SUCCESS;
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE_BLOCK(GEO_POINT, geo_point)
    case GRN_TS_REF: {
      size_t i;
      grn_ts_ref *out_ptr = (grn_ts_ref *)out;
      grn_ra *ra = (grn_ra *)node->column;
      grn_ra_cache cache;
      GRN_RA_CACHE_INIT(ra, &cache);
      for (i = 0; i < n_in; i++) {
        grn_ts_id *ptr = NULL;
        if (in[i].id) {
          ptr = (grn_ts_id *)grn_ra_ref_cache(ctx, ra, in[i].id, &cache);
        }
        out_ptr[i].id = ptr ? *ptr : GRN_ID_NIL;
        out_ptr[i].score = in[i].score;
      }
      GRN_RA_CACHE_FIN(ra, &cache);
      return GRN_SUCCESS;
    }
    default: {
      return GRN_OBJECT_CORRUPT;
    }
  }
}
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE_BLOCK
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE_BLOCK

/*
 * grn_ts_expr_column_node_evaluate_text_vector() outputs text vector column
 * values.
 */
static grn_rc
grn_ts_expr_column_node_evaluate_text_vector(grn_ctx *ctx,
                                             grn_ts_expr_column_node *node,
                                             const grn_ts_record *in,
                                             size_t n_in, void *out) {
  size_t i, j, total_n_bytes = 0, total_size = 0;
  char *buf_ptr;
  grn_ts_text *text_ptr;
  grn_ts_text_vector *out_ptr = (grn_ts_text_vector *)out;
  /* Read encoded values into node->buf and save the size of each value. */
  GRN_BULK_REWIND(&node->buf);
  for (i = 0; i < n_in; i++) {
    char *ptr;
    size_t n_bytes, size;
    grn_rc rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,
                                    &node->buf, &n_bytes);
    if (rc == GRN_SUCCESS) {
      ptr = GRN_BULK_HEAD(&node->buf) + total_n_bytes;
      GRN_B_DEC(size, ptr);
    } else {
      n_bytes = 0;
      size = 0;
    }
    grn_memcpy(&out_ptr[i].ptr, &n_bytes, sizeof(n_bytes));
    out_ptr[i].size = size;
    total_n_bytes += n_bytes;
    total_size += size;
  }
  /* Resize node->body.text_buf. */
  if (node->body.text_buf.size < total_size) {
    size_t n_bytes = sizeof(grn_ts_text) * total_size;
    grn_ts_text *new_buf;
    new_buf = (grn_ts_text *)GRN_REALLOC(node->body.text_buf.ptr, n_bytes);
    if (!new_buf) {
      for (i = 0; i < n_in; i++) {
        out_ptr[i] = grn_ts_text_vector_zero();
      }
      return GRN_SUCCESS;
    }
    node->body.text_buf.ptr = new_buf;
    node->body.text_buf.size = total_size;
  }
  /* Decode values and compose the result. */
  buf_ptr = GRN_BULK_HEAD(&node->buf);
  text_ptr = node->body.text_buf.ptr;
  for (i = 0; i < n_in; i++) {
    char *ptr = buf_ptr;
    size_t n_bytes, size;
    grn_memcpy(&n_bytes, &out_ptr[i].ptr, sizeof(n_bytes));
    buf_ptr += n_bytes;
    GRN_B_DEC(size, ptr);
    out_ptr[i].ptr = text_ptr;
    for (j = 0; j < out_ptr[i].size; j++) {
      GRN_B_DEC(text_ptr[j].size, ptr);
    }
    for (j = 0; j < out_ptr[i].size; j++) {
      text_ptr[j].ptr = ptr;
      ptr += text_ptr[j].size;
    }
    text_ptr += out_ptr[i].size;
  }
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_column_node_evaluate_ref_vector() outputs ref vector column
 * values.
 */
static grn_rc
grn_ts_expr_column_node_evaluate_ref_vector(grn_ctx *ctx,
                                             grn_ts_expr_column_node *node,
                                             const grn_ts_record *in,
                                             size_t n_in, void *out) {
  size_t i, j, offset = 0;
  grn_ts_id *buf_ptr;
  grn_ts_ref *ref_ptr;
  grn_ts_ref_vector *out_ptr = (grn_ts_ref_vector *)out;
  /* Read column values into node->buf and save the size of each value. */
  GRN_BULK_REWIND(&node->buf);
  for (i = 0; i < n_in; i++) {
    size_t size;
    grn_rc rc;
    rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,
                             &node->buf, &size);
    if (rc == GRN_SUCCESS) {
      out_ptr[i].size = size / sizeof(grn_ts_id);
      offset += out_ptr[i].size;
    } else {
      out_ptr[i].size = 0;
    }
  }
  /* Resize node->body.ref_buf. */
  if (node->body.ref_buf.size < offset) {
    size_t n_bytes = sizeof(grn_ts_ref) * offset;
    grn_ts_ref *new_buf;
    new_buf = (grn_ts_ref *)GRN_REALLOC(node->body.ref_buf.ptr, n_bytes);
    if (!new_buf) {
      for (i = 0; i < n_in; i++) {
        out_ptr[i] = grn_ts_ref_vector_zero();
      }
      return GRN_SUCCESS;
    }
    node->body.ref_buf.ptr = new_buf;
    node->body.ref_buf.size = offset;
  }
  /* Compose the result. */
  buf_ptr = (grn_ts_id *)GRN_BULK_HEAD(&node->buf);
  ref_ptr = node->body.ref_buf.ptr;
  for (i = 0; i < n_in; i++) {
    out_ptr[i].ptr = ref_ptr;
    for (j = 0; j < out_ptr[i].size; j++, buf_ptr++, ref_ptr++) {
      ref_ptr->id = *buf_ptr;
      ref_ptr->score = in[i].score;
    }
  }
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND ## _VECTOR: {\
    size_t i;\
    grn_ts_ ## kind *buf_ptr;\
    grn_ts_ ## kind ## _vector *out_ptr = (grn_ts_ ## kind ## _vector *)out;\
    /* Read column values into node->buf and save the size of each value. */\
    GRN_BULK_REWIND(&node->buf);\
    for (i = 0; i < n_in; i++) {\
      size_t size;\
      grn_rc rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,\
                                      &node->buf, &size);\
      if (rc == GRN_SUCCESS) {\
        out_ptr[i].size = size / sizeof(grn_ts_ ## kind);\
      } else {\
        out_ptr[i].size = 0;\
      }\
    }\
    buf_ptr = (grn_ts_ ## kind *)GRN_BULK_HEAD(&node->buf);\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i].ptr = buf_ptr;\
      buf_ptr += out_ptr[i].size;\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(TYPE, type)\
  case GRN_DB_ ## TYPE: {\
    size_t i, j;\
    grn_obj src_buf;\
    grn_ts_int *buf_ptr;\
    grn_ts_int_vector *out_ptr = (grn_ts_int_vector *)out;\
    GRN_ ## TYPE ## _INIT(&src_buf, GRN_OBJ_VECTOR);\
    /*
     * Read column values into src_buf and typecast the values to grn_ts_int.
     * Then, store the grn_ts_int values into node->buf and save the size of
     * each value.
     */\
    GRN_BULK_REWIND(&node->buf);\
    for (i = 0; i < n_in; i++) {\
      grn_rc rc;\
      size_t size;\
      GRN_BULK_REWIND(&src_buf);\
      rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,\
                               &src_buf, &size);\
      if (rc == GRN_SUCCESS) {\
        type ## _t *src_ptr = (type ## _t *)GRN_BULK_HEAD(&src_buf);\
        out_ptr[i].size = size / sizeof(type ## _t);\
        for (j = 0; j < out_ptr[i].size; j++) {\
          grn_ts_int value = (grn_ts_int)src_ptr[j];\
          grn_rc rc = grn_bulk_write(ctx, &node->buf, (char *)&value,\
                                     sizeof(value));\
          if (rc != GRN_SUCCESS) {\
            if (j) {\
              /* Cancel written values. */\
              size_t size_written = sizeof(value) * j;\
              grn_bulk_resize(ctx, &node->buf,\
                              GRN_BULK_VSIZE(&node->buf) - size_written);\
            }\
            out_ptr[i].size = 0;\
            break;\
          }\
        }\
      } else {\
        out_ptr[i].size = 0;\
      }\
    }\
    buf_ptr = (grn_ts_int *)GRN_BULK_HEAD(&node->buf);\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i].ptr = buf_ptr;\
      buf_ptr += out_ptr[i].size;\
    }\
    GRN_OBJ_FIN(ctx, &src_buf);\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_column_node_evaluate_vector() outputs vector column values. */
static grn_rc
grn_ts_expr_column_node_evaluate_vector(grn_ctx *ctx,
                                        grn_ts_expr_column_node *node,
                                        const grn_ts_record *in, size_t n_in,
                                        void *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE_BLOCK(BOOL, bool)
    case GRN_TS_INT_VECTOR: {
      switch (node->data_type) {
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(INT8, int8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(INT16, int16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(INT32, int32)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(INT64, int64)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(UINT8, uint8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(UINT16, uint16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(UINT32, uint32)
        /* The behavior is undefined if a value is greater than 2^63 - 1. */
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK(UINT64, uint64)
        default: {
          return GRN_OBJECT_CORRUPT;
        }
      }
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE_BLOCK(TIME, time)
    case GRN_TS_TEXT_VECTOR: {
      return grn_ts_expr_column_node_evaluate_text_vector(ctx, node, in, n_in,
                                                          out);
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE_BLOCK(GEO_POINT, geo_point)
    case GRN_TS_REF_VECTOR: {
      return grn_ts_expr_column_node_evaluate_ref_vector(ctx, node, in, n_in,
                                                         out);
    }
    default: {
      return GRN_OBJECT_CORRUPT;
    }
  }
}
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE_BLOCK
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE_BLOCK

/* grn_ts_expr_column_node_evaluate() outputs column values. */
static grn_rc
grn_ts_expr_column_node_evaluate(grn_ctx *ctx, grn_ts_expr_column_node *node,
                                 const grn_ts_record *in, size_t n_in,
                                 void *out) {
  if (node->data_kind & GRN_TS_VECTOR_FLAG) {
    return grn_ts_expr_column_node_evaluate_vector(ctx, node, in, n_in, out);
  } else {
    return grn_ts_expr_column_node_evaluate_scalar(ctx, node, in, n_in, out);
  }
}

/* grn_ts_expr_op_node_evaluate() outputs results of an operator. */
static grn_rc
grn_ts_expr_op_node_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                             const grn_ts_record *in, size_t n_in,
                             void *out) {
  // TODO
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(TYPE, type)\
  case GRN_TS_EXPR_ ## TYPE ## _NODE: {\
    grn_ts_expr_ ## type ## _node *type ## _node;\
    type ## _node = (grn_ts_expr_ ## type ## _node *)node;\
    return grn_ts_expr_ ## type ## _node_evaluate(ctx, type ## _node,\
                                                  in, n_in, out);\
  }
/* grn_ts_expr_node_evaluate() evaluates a subexpression. */
static grn_rc
grn_ts_expr_node_evaluate(grn_ctx *ctx, grn_ts_expr_node *node,
                          const grn_ts_record *in, size_t n_in,
                          void *out) {
  switch (node->type) {
    GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(ID, id)
    GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(SCORE, score)
    GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(KEY, key)
    GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(VALUE, value)
    GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(CONST, const)
    GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(COLUMN, column)
    GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK(OP, op)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_NODE_EVALUATE_CASE_BLOCK

/* grn_ts_expr_key_node_filter() filters records. */
static grn_rc
grn_ts_expr_key_node_filter(grn_ctx *ctx, grn_ts_expr_key_node *node,
                            grn_ts_record *in, size_t n_in,
                            grn_ts_record *out, size_t *n_out) {
  size_t i, count = 0;
  for (i = 0; i < n_in; i++) {
    const grn_ts_bool *key;
    size_t key_size;
    key = (const grn_ts_bool *)grn_ts_table_get_key(ctx, node->table,
                                                    in[i].id, &key_size);
    if (key && (key_size == sizeof(*key)) && *key) {
      out[count++] = in[i];
    }
  }
  *n_out = count;
  return GRN_SUCCESS;
}

/* grn_ts_expr_value_node_filter() filters records. */
static grn_rc
grn_ts_expr_value_node_filter(grn_ctx *ctx, grn_ts_expr_value_node *node,
                              grn_ts_record *in, size_t n_in,
                              grn_ts_record *out, size_t *n_out) {
  size_t i, count = 0;
  for (i = 0; i < n_in; i++) {
    const grn_ts_bool *value;
    value = (const grn_ts_bool *)grn_ts_table_get_value(ctx, node->table,
                                                        in[i].id);
    if (value && *value) {
      out[count++] = in[i];
    }
  }
  *n_out = count;
  return GRN_SUCCESS;
}

/* grn_ts_expr_const_node_filter() filters records. */
static grn_rc
grn_ts_expr_const_node_filter(grn_ctx *ctx, grn_ts_expr_const_node *node,
                              grn_ts_record *in, size_t n_in,
                              grn_ts_record *out, size_t *n_out) {
  if (node->content.bool_value) {
    if (in != out) {
      size_t i;
      for (i = 0; i < n_in; i++) {
        out[i] = in[i];
      }
    }
    *n_out = n_in;
  } else {
    *n_out = 0;
  }
  return GRN_SUCCESS;
}

/* grn_ts_expr_column_node_filter() filters records. */
static grn_rc
grn_ts_expr_column_node_filter(grn_ctx *ctx, grn_ts_expr_column_node *node,
                               grn_ts_record *in, size_t n_in,
                               grn_ts_record *out, size_t *n_out) {
  size_t i, count = 0;
  grn_ra *ra = (grn_ra *)node->column;
  grn_ra_cache cache;
  GRN_RA_CACHE_INIT(ra, &cache);
  for (i = 0; i < n_in; i++) {
    grn_ts_bool *ptr = NULL;
    if (in[i].id) {
      ptr = grn_ra_ref_cache(ctx, ra, in[i].id, &cache);
    }
    if (ptr && *ptr) {
      out[count++] = in[i];
    }
  }
  GRN_RA_CACHE_FIN(ra, &cache);
  *n_out = count;
  return GRN_SUCCESS;
}

/* grn_ts_expr_op_node_filter() filters records. */
static grn_rc
grn_ts_expr_op_node_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                           grn_ts_record *in, size_t n_in,
                           grn_ts_record *out, size_t *n_out) {
  // TODO
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_NODE_FILTER_CASE_BLOCK(TYPE, type)\
  case GRN_TS_EXPR_ ## TYPE ## _NODE: {\
    grn_ts_expr_ ## type ## _node *type ## _node;\
    type ## _node = (grn_ts_expr_ ## type ## _node *)node;\
    return grn_ts_expr_ ## type ## _node_filter(ctx, type ## _node,\
                                                in, n_in, out, n_out);\
  }
/* grn_ts_expr_node_filter() filters records. */
static grn_rc
grn_ts_expr_node_filter(grn_ctx *ctx, grn_ts_expr_node *node,
                        grn_ts_record *in, size_t n_in,
                        grn_ts_record *out, size_t *n_out) {
  if (node->data_kind != GRN_TS_BOOL) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (node->type) {
    GRN_TS_EXPR_NODE_FILTER_CASE_BLOCK(KEY, key)
    GRN_TS_EXPR_NODE_FILTER_CASE_BLOCK(VALUE, value)
    GRN_TS_EXPR_NODE_FILTER_CASE_BLOCK(CONST, const)
    GRN_TS_EXPR_NODE_FILTER_CASE_BLOCK(COLUMN, column)
    GRN_TS_EXPR_NODE_FILTER_CASE_BLOCK(OP, op)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_NODE_FILTER_CASE_BLOCK

/* grn_ts_expr_score_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_score_node_adjust(grn_ctx *ctx, grn_ts_expr_score_node *node,
                              grn_ts_record *io, size_t n_io) {
  /* Nothing to do. */
  return GRN_SUCCESS;
}

/* grn_ts_expr_key_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_key_node_adjust(grn_ctx *ctx, grn_ts_expr_key_node *node,
                            grn_ts_record *io, size_t n_io) {
  size_t i;
  for (i = 0; i < n_io; i++) {
    const grn_ts_float *key;
    size_t key_size;
    key = (const grn_ts_float *)grn_ts_table_get_key(ctx, node->table,
                                                     io[i].id, &key_size);
    if (key && (key_size == sizeof(*key))) {
      io[i].score = (grn_ts_score)*key;
    }
  }
  return GRN_SUCCESS;
}

/* grn_ts_expr_value_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_value_node_adjust(grn_ctx *ctx, grn_ts_expr_value_node *node,
                              grn_ts_record *io, size_t n_io) {
  size_t i;
  for (i = 0; i < n_io; i++) {
    const grn_ts_float *value;
    value = (const grn_ts_float *)grn_ts_table_get_value(ctx, node->table,
                                                         io[i].id);
    if (value) {
      io[i].score = (grn_ts_score)*value;
    }
  }
  return GRN_SUCCESS;
}

/* grn_ts_expr_const_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_const_node_adjust(grn_ctx *ctx, grn_ts_expr_const_node *node,
                              grn_ts_record *io, size_t n_io) {
  size_t i;
  grn_ts_score score = (grn_ts_score)node->content.float_value;
  for (i = 0; i < n_io; i++) {
    io[i].score = score;
  }
  return GRN_SUCCESS;
}

/* grn_ts_expr_column_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_column_node_adjust(grn_ctx *ctx, grn_ts_expr_column_node *node,
                               grn_ts_record *io, size_t n_io) {
  size_t i;
  grn_ra *ra = (grn_ra *)node->column;
  grn_ra_cache cache;
  GRN_RA_CACHE_INIT(ra, &cache);
  for (i = 0; i < n_io; i++) {
    grn_ts_float *ptr = NULL;
    if (io[i].id) {
      ptr = grn_ra_ref_cache(ctx, ra, io[i].id, &cache);
    }
    if (ptr) {
      io[i].score = (grn_ts_score)*ptr;
    }
  }
  GRN_RA_CACHE_FIN(ra, &cache);
  return GRN_SUCCESS;
}

/* grn_ts_expr_op_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_op_node_adjust(grn_ctx *ctx, grn_ts_expr_op_node *node,
                           grn_ts_record *io, size_t n_io) {
  // TODO
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK(TYPE, type)\
  case GRN_TS_EXPR_ ## TYPE ## _NODE: {\
    grn_ts_expr_ ## type ## _node *type ## _node;\
    type ## _node = (grn_ts_expr_ ## type ## _node *)node;\
    return grn_ts_expr_ ## type ## _node_adjust(ctx, type ## _node, io, n_io);\
  }
/* grn_ts_expr_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_node_adjust(grn_ctx *ctx, grn_ts_expr_node *node,
                        grn_ts_record *io, size_t n_io) {
  if (node->data_kind != GRN_TS_FLOAT) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (node->type) {
    GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK(SCORE, score)
    GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK(KEY, key)
    GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK(VALUE, value)
    GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK(CONST, const)
    GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK(COLUMN, column)
    GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK(OP, op)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_NODE_ADJUST_CASE_BLOCK

/*-------------------------------------------------------------
 * grn_ts_expr.
 */

/* grn_ts_expr_init() initializes an expression. */
static grn_rc
grn_ts_expr_init(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *table) {
  /* Increment a reference count for table and curr_table. */
  grn_rc rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    grn_obj_unlink(ctx, table);
    return rc;
  }
  memset(expr, 0, sizeof(*expr));
  expr->table = table;
  expr->curr_table = table;
  expr->root = NULL;
  expr->nodes = NULL;
  expr->stack = NULL;
  // TODO: Initialize new members.
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_open(grn_ctx *ctx, grn_obj *table, grn_ts_expr **expr) {
  grn_rc rc;
  grn_ts_expr *new_expr;
  if (!ctx || !table || !grn_ts_obj_is_table(ctx, table) || !expr) {
    return GRN_INVALID_ARGUMENT;
  }
  new_expr = GRN_MALLOCN(grn_ts_expr, 1);
  if (!new_expr) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  rc = grn_ts_expr_init(ctx, new_expr, table);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_expr);
    return rc;
  }
  *expr = new_expr;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_parse(grn_ctx *ctx, grn_obj *table,
                  const char *str, size_t str_size, grn_ts_expr **expr) {
  grn_rc rc;
  grn_ts_expr *new_expr;
  if (!ctx || !table || !grn_ts_obj_is_table(ctx, table) ||
      !str || !str_size || !expr) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_open(ctx, table, &new_expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  // TODO: Hmm...
  {
    rc = grn_ts_expr_push(ctx, new_expr, str, str_size);
    if (rc != GRN_SUCCESS) {
      grn_ts_expr_close(ctx, new_expr);
      return rc;
    }
    rc = grn_ts_expr_complete(ctx, new_expr);
    if (rc != GRN_SUCCESS) {
      grn_ts_expr_close(ctx, new_expr);
      return rc;
    }
  }
  *expr = new_expr;
  return GRN_SUCCESS;
}

/* grn_ts_expr_fin() finalizes an expression. */
static grn_rc
grn_ts_expr_fin(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc = GRN_SUCCESS;
  // TODO: Finalize new members.
  if (expr->stack) {
    GRN_FREE(expr->stack);
  }
  if (expr->nodes) {
    size_t i;
    for (i = 0; i < expr->n_nodes; i++) {
      if (expr->nodes[i]) {
        grn_rc rc_new = grn_ts_expr_node_close(ctx, expr->nodes[i]);
        if (rc == GRN_SUCCESS) {
          rc = rc_new;
        }
      }
    }
    GRN_FREE(expr->nodes);
  }
  if (expr->curr_table) {
    grn_obj_unlink(ctx, expr->curr_table);
  }
  if (expr->table) {
    grn_obj_unlink(ctx, expr->table);
  }
  return rc;
}

grn_rc
grn_ts_expr_close(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  if (!ctx || !expr) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_fin(ctx, expr);
  GRN_FREE(expr);
  return rc;
}

grn_obj *
grn_ts_expr_get_table(grn_ctx *ctx, grn_ts_expr *expr) {
  if (!ctx || !expr) {
    return NULL;
  }
  /* The reference counting will never fail in practice. */
  if (grn_ts_obj_increment_ref_count(ctx, expr->table) != GRN_SUCCESS) {
    return NULL;
  }
  return expr->table;
}

grn_ts_expr_type
grn_ts_expr_get_type(grn_ctx *ctx, grn_ts_expr *expr) {
  return (!ctx || !expr) ? GRN_TS_EXPR_BROKEN : expr->type;
}

grn_ts_data_kind
grn_ts_expr_get_data_kind(grn_ctx *ctx, grn_ts_expr *expr) {
  return (!ctx || !expr) ? GRN_TS_VOID : expr->data_kind;
}

grn_ts_data_type
grn_ts_expr_get_data_type(grn_ctx *ctx, grn_ts_expr *expr) {
  return (!ctx || !expr) ? GRN_DB_VOID : expr->data_type;
}

grn_ts_expr_node *
grn_ts_expr_get_root(grn_ctx *ctx, grn_ts_expr *expr) {
  return (!ctx || !expr) ? NULL : expr->root;
}

/* grn_ts_expr_reserve_nodes() extends a node buffer for a new node. */
static grn_rc
grn_ts_expr_reserve_nodes(grn_ctx *ctx, grn_ts_expr *expr) {
  size_t i, n_bytes, new_max_n_nodes;
  grn_ts_expr_node **new_nodes;
  if (expr->n_nodes < expr->max_n_nodes) {
    return GRN_SUCCESS;
  }
  new_max_n_nodes = expr->n_nodes * 2;
  if (!new_max_n_nodes) {
    new_max_n_nodes = 1;
  }
  n_bytes = sizeof(grn_ts_expr_node *) * new_max_n_nodes;
  new_nodes = (grn_ts_expr_node **)GRN_REALLOC(expr->nodes, n_bytes);
  if (!new_nodes) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  for (i = expr->n_nodes; i < new_max_n_nodes; i++) {
    new_nodes[i] = NULL;
  }
  expr->nodes = new_nodes;
  expr->max_n_nodes = new_max_n_nodes;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_open_id_node() opens and registers an ID node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_id_node(grn_ctx *ctx, grn_ts_expr *expr,
                         grn_ts_expr_node **node) {
  grn_ts_expr_node *new_node;
  grn_rc rc = grn_ts_expr_reserve_nodes(ctx, expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_id_node_open(ctx, &new_node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  expr->nodes[expr->n_nodes++] = new_node;
  *node = new_node;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_open_score_node() opens and registers a score node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_score_node(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_expr_node **node) {
  grn_ts_expr_node *new_node;
  grn_rc rc = grn_ts_expr_reserve_nodes(ctx, expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_score_node_open(ctx, &new_node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  expr->nodes[expr->n_nodes++] = new_node;
  *node = new_node;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_open_key_node() opens and registers a key node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_key_node(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_ts_expr_node **node) {
  grn_ts_expr_node *new_node;
  grn_rc rc = grn_ts_expr_reserve_nodes(ctx, expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_key_node_open(ctx, expr->curr_table, &new_node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  expr->nodes[expr->n_nodes++] = new_node;
  *node = new_node;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_open_value_node() opens and registers a value node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_value_node(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_expr_node **node) {
  grn_ts_expr_node *new_node;
  grn_rc rc = grn_ts_expr_reserve_nodes(ctx, expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_value_node_open(ctx, expr->curr_table, &new_node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  expr->nodes[expr->n_nodes++] = new_node;
  *node = new_node;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_open_const_node() opens and registers a const node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_const_node(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_data_kind kind, const void *value,
                            grn_ts_expr_node **node) {
  grn_ts_expr_node *new_node;
  grn_rc rc = grn_ts_expr_reserve_nodes(ctx, expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_const_node_open(ctx, kind, value, &new_node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  expr->nodes[expr->n_nodes++] = new_node;
  *node = new_node;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_open_column_node() opens and registers a column.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_column_node(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_obj *column, grn_ts_expr_node **node) {
  grn_ts_expr_node *new_node;
  grn_rc rc = grn_ts_expr_reserve_nodes(ctx, expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_column_node_open(ctx, column, &new_node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  expr->nodes[expr->n_nodes++] = new_node;
  *node = new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_push_node() pushes a node to stack. */
static grn_rc
grn_ts_expr_push_node(grn_ctx *ctx, grn_ts_expr *expr,
                      grn_ts_expr_node *node) {
  if (expr->stack_depth == expr->stack_size) {
    size_t i, n_bytes, new_size;
    grn_ts_expr_node **new_stack;
    new_size = expr->stack_size * 2;
    if (!new_size) {
      new_size = 1;
    }
    n_bytes = sizeof(grn_ts_expr_node *) * new_size;
    new_stack = GRN_REALLOC(expr->stack, n_bytes);
    if (!new_stack) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    for (i = expr->stack_size; i < new_size; i++) {
      new_stack[i] = NULL;
    }
    expr->stack = new_stack;
    expr->stack_size = new_size;
  }
  expr->stack[expr->stack_depth++] = node;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push(grn_ctx *ctx, grn_ts_expr *expr,
                 const char *str, size_t str_size) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN) ||
      !str || !str_size) {
    return GRN_INVALID_ARGUMENT;
  }
  if ((str_size == GRN_COLUMN_NAME_ID_LEN) &&
      !memcmp(str, GRN_COLUMN_NAME_ID, GRN_COLUMN_NAME_ID_LEN)) {
    return grn_ts_expr_push_id(ctx, expr);
  } else if ((str_size == GRN_COLUMN_NAME_KEY_LEN) &&
             !memcmp(str, GRN_COLUMN_NAME_KEY, GRN_COLUMN_NAME_KEY_LEN)) {
    return grn_ts_expr_push_key(ctx, expr);
  } else if ((str_size == GRN_COLUMN_NAME_VALUE_LEN) &&
             !memcmp(str, GRN_COLUMN_NAME_VALUE, GRN_COLUMN_NAME_VALUE_LEN)) {
    return grn_ts_expr_push_value(ctx, expr);
  } else if ((str_size == GRN_COLUMN_NAME_SCORE_LEN) &&
             !memcmp(str, GRN_COLUMN_NAME_SCORE, GRN_COLUMN_NAME_SCORE_LEN)) {
    return grn_ts_expr_push_score(ctx, expr);
  } else if ((str_size == 4) && !memcmp(str, "true", 4)) {
    return grn_ts_expr_push_bool(ctx, expr, GRN_TRUE);
  } else if ((str_size == 5) && !memcmp(str, "false", 5)) {
    return grn_ts_expr_push_bool(ctx, expr, GRN_FALSE);
  } else if (isdigit((unsigned char)str[0])) {
    char *buf, *end;
    grn_rc rc;
    grn_ts_int int_value;
    buf = GRN_MALLOCN(char, str_size + 1);
    if (!buf) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    grn_memcpy(buf, str, str_size);
    buf[str_size] = '\0';
    int_value = strtol(buf, &end, 0);
    if (*end == '\0') {
      rc = grn_ts_expr_push_int(ctx, expr, int_value);
    } else if (*end == '.') {
      grn_ts_float float_value = strtod(buf, &end);
      if (*end == '\0') {
        rc = grn_ts_expr_push_float(ctx, expr, float_value);
      } else {
        rc = GRN_INVALID_ARGUMENT;
      }
    }
    GRN_FREE(buf);
    return rc;
  } else if (str[0] == '"') {
    char *buf;
    size_t i, len, end;
    grn_rc rc;
    if (str[str_size - 1] != '"') {
      return GRN_INVALID_ARGUMENT;
    }
    if (str_size == 2) {
      return grn_ts_expr_push_text(ctx, expr, grn_ts_text_zero());
    }
    buf = GRN_MALLOCN(char, str_size - 2);
    if (!buf) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    rc = GRN_SUCCESS;
    len = 0;
    end = str_size - 1;
    for (i = 1; (rc == GRN_SUCCESS) && (i < end); i++) {
      switch (str[i]) {
        case '\\': {
          if (i == (end - 1)) {
            rc = GRN_INVALID_ARGUMENT;
            break;
          }
          buf[len++] = str[++i];
          break;
        }
        case '"': {
          rc = GRN_INVALID_ARGUMENT;
          break;
        }
        default: {
          buf[len++] = str[i];
          break;
        }
      }
    }
    if (rc == GRN_SUCCESS) {
      grn_ts_text value = { buf, len };
      rc = grn_ts_expr_push_text(ctx, expr, value);
    }
    GRN_FREE(buf);
    return rc;
  } else {
    grn_rc rc;
    grn_obj *column = grn_obj_column(ctx, expr->curr_table, str, str_size);
    if (!column) {
      return GRN_INVALID_ARGUMENT;
    }
    rc = grn_ts_expr_push_column(ctx, expr, column);
    grn_obj_unlink(ctx, column);
    return rc;
  }
}

#define GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(TYPE, kind)\
  case GRN_DB_ ## TYPE: {\
    return grn_ts_expr_push_ ## kind(ctx, expr, GRN_ ## TYPE ## _VALUE(obj));\
  }
/* grn_ts_expr_push_bulk() pushes a scalar. */
static grn_rc
grn_ts_expr_push_bulk(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj) {
  switch (obj->header.domain) {
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(BOOL, bool)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(INT8, int)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(INT16, int)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(INT32, int)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(INT64, int)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(UINT8, int)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(UINT16, int)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(UINT32, int)
    /* The behavior is undefined if a value is greater than 2^63 - 1. */
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(UINT64, int)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK(TIME, time)
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      grn_ts_text value = { GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj) };
      return grn_ts_expr_push_text(ctx, expr, value);
    }
    case GRN_DB_TOKYO_GEO_POINT: {
      grn_ts_geo_point value;
      GRN_GEO_POINT_VALUE(obj, value.latitude, value.longitude);
      return grn_ts_expr_push_tokyo_geo_point(ctx, expr, value);
    }
    case GRN_DB_WGS84_GEO_POINT: {
      grn_ts_geo_point value;
      GRN_GEO_POINT_VALUE(obj, value.latitude, value.longitude);
      return grn_ts_expr_push_wgs84_geo_point(ctx, expr, value);
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_PUSH_BULK_CASE_BLOCK

#define GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK(TYPE, kind)\
  case GRN_DB_ ## TYPE: {\
    grn_ts_ ## kind ##_vector value = { (grn_ts_ ## kind *)GRN_BULK_HEAD(obj),\
                                        grn_uvector_size(ctx, obj) };\
    return grn_ts_expr_push_ ## kind ## _vector(ctx, expr, value);\
  }
#define GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST(TYPE, kind)\
  case GRN_DB_ ## TYPE: {\
    size_t i;\
    grn_rc rc;\
    grn_ts_ ## kind ## _vector value = { NULL, grn_uvector_size(ctx, obj) };\
    if (!value.size) {\
      return grn_ts_expr_push_ ## kind ## _vector(ctx, expr, value);\
    }\
    grn_ts_ ## kind *buf = GRN_MALLOCN(grn_ts_ ## kind, value.size);\
    if (!buf) {\
      return GRN_NO_MEMORY_AVAILABLE;\
    }\
    for (i = 0; i < value.size; i++) {\
      buf[i] = GRN_ ## TYPE ##_VALUE_AT(obj, i);\
    }\
    value.ptr = buf;\
    rc = grn_ts_expr_push_ ## kind ## _vector(ctx, expr, value);\
    GRN_FREE(buf);\
    return rc;\
  }
/* grn_ts_expr_push_uvector() pushes an array of fixed-size values. */
static grn_rc
grn_ts_expr_push_uvector(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj) {
  switch (obj->header.domain) {
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK(BOOL, bool)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST(INT8, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST(INT16, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST(INT32, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK(INT64, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST(UINT8, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST(UINT16, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST(UINT32, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK(UINT64, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK(TIME, time)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK(TOKYO_GEO_POINT, tokyo_geo_point)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK(WGS84_GEO_POINT, wgs84_geo_point)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK_WITH_TYPECAST
#undef GRN_TS_EXPR_PUSH_UVECTOR_CASE_BLOCK

/* grn_ts_expr_push_uvector() pushes an array of texts. */
static grn_rc
grn_ts_expr_push_vector(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj) {
  switch (obj->header.domain) {
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      size_t i;
      grn_rc rc;
      grn_ts_text_vector value = { NULL, grn_vector_size(ctx, obj) };
      if (!value.size) {
        return grn_ts_expr_push_text_vector(ctx, expr, value);
      }
      grn_ts_text *buf = GRN_MALLOCN(grn_ts_text, value.size);
      if (!buf) {
        return GRN_NO_MEMORY_AVAILABLE;
      }
      for (i = 0; i < value.size; i++) {
        buf[i].size = grn_vector_get_element(ctx, obj, i, &buf[i].ptr,
                                             NULL, NULL);
      }
      value.ptr = buf;
      rc = grn_ts_expr_push_text_vector(ctx, expr, value);
      GRN_FREE(buf);
      return rc;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

static grn_rc
grn_ts_expr_push_accessor(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_accessor *accessor) {
  if (accessor->next) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (accessor->action) {
    case GRN_ACCESSOR_GET_ID: {
      return grn_ts_expr_push_id(ctx, expr);
    }
    case GRN_ACCESSOR_GET_SCORE: {
      return grn_ts_expr_push_score(ctx, expr);
    }
    case GRN_ACCESSOR_GET_KEY: {
      return grn_ts_expr_push_key(ctx, expr);
    }
    case GRN_ACCESSOR_GET_VALUE: {
      return grn_ts_expr_push_value(ctx, expr);
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

grn_rc
grn_ts_expr_push_obj(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN) || !obj) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (obj->header.type) {
    case GRN_BULK: {
      return grn_ts_expr_push_bulk(ctx, expr, obj);
    }
    case GRN_UVECTOR: {
      return grn_ts_expr_push_uvector(ctx, expr, obj);
    }
    case GRN_VECTOR: {
      return grn_ts_expr_push_vector(ctx, expr, obj);
    }
    case GRN_ACCESSOR: {
      return grn_ts_expr_push_accessor(ctx, expr, (grn_accessor *)obj);
    }
    case GRN_COLUMN_FIX_SIZE:
    case GRN_COLUMN_VAR_SIZE: {
      return grn_ts_expr_push_column(ctx, expr, obj);
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

grn_rc
grn_ts_expr_push_id(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_open_id_node(ctx, expr, &node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_push_node(ctx, expr, node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_score(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_open_score_node(ctx, expr, &node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_push_node(ctx, expr, node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_key(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_open_key_node(ctx, expr, &node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_push_node(ctx, expr, node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_value(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_open_value_node(ctx, expr, &node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_push_node(ctx, expr, node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    return grn_ts_expr_push_ ## kind(ctx, expr,\
                                     *(const grn_ts_ ## kind *)value);\
  }
grn_rc
grn_ts_expr_push_const(grn_ctx *ctx, grn_ts_expr *expr,
                       grn_ts_data_kind kind, const void *value) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN) || !value) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (kind) {
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(BOOL, bool)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(INT, int)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(FLOAT, float)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(TIME, time)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(TEXT, text)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(GEO_POINT, geo_point)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(BOOL_VECTOR, bool_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(INT_VECTOR, int_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(FLOAT_VECTOR, float_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(TIME_VECTOR, time_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(TEXT_VECTOR, text_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK(GEO_POINT_VECTOR, geo_point_vector)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_PUSH_CONST_CASE_BLOCK

grn_rc
grn_ts_expr_push_column(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *column) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN) ||
      !column || !grn_ts_obj_is_column(ctx, column) ||
      (DB_OBJ(expr->curr_table)->id != column->header.domain)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_open_column_node(ctx, expr, column, &node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_push_node(ctx, expr, node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_operator(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_ts_op_type op_type) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN)) {
    return GRN_INVALID_ARGUMENT;
  }
  // TODO
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_PUSH_CONST(KIND, kind)\
  grn_rc rc;\
  grn_ts_expr_node *node;\
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_BROKEN) ||\
      !grn_ts_ ## kind ## _is_valid(value)) {\
    return GRN_INVALID_ARGUMENT;\
  }\
  rc = grn_ts_expr_open_const_node(ctx, expr, GRN_TS_ ## KIND, &value, &node);\
  if (rc != GRN_SUCCESS) {\
    return rc;\
  }\
  rc = grn_ts_expr_push_node(ctx, expr, node);\
  if (rc != GRN_SUCCESS) {\
    return rc;\
  }
grn_rc
grn_ts_expr_push_bool(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_bool value) {
  GRN_TS_EXPR_PUSH_CONST(BOOL, bool)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_int(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_int value) {
  GRN_TS_EXPR_PUSH_CONST(INT, int)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_float(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_float value) {
  GRN_TS_EXPR_PUSH_CONST(FLOAT, float)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_time(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_time value) {
  GRN_TS_EXPR_PUSH_CONST(TIME, time)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_text(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_text value) {
  GRN_TS_EXPR_PUSH_CONST(TEXT, text)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                           grn_ts_geo_point value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT, geo_point)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_tokyo_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                                 grn_ts_geo_point value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT, geo_point)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_wgs84_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                                 grn_ts_geo_point value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT, geo_point)
  node->data_type = GRN_DB_WGS84_GEO_POINT;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_bool_vector(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_bool_vector value) {
  GRN_TS_EXPR_PUSH_CONST(BOOL_VECTOR, bool_vector)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_int_vector(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_int_vector value) {
  GRN_TS_EXPR_PUSH_CONST(INT_VECTOR, int_vector)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_float_vector(grn_ctx *ctx, grn_ts_expr *expr,
                              grn_ts_float_vector value) {
  GRN_TS_EXPR_PUSH_CONST(FLOAT_VECTOR, float_vector)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_time_vector(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_time_vector value) {
  GRN_TS_EXPR_PUSH_CONST(TIME_VECTOR, time_vector)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_text_vector(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_text_vector value) {
  GRN_TS_EXPR_PUSH_CONST(TEXT_VECTOR, text_vector)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                  grn_ts_geo_point_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT_VECTOR, geo_point_vector)
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_tokyo_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                        grn_ts_geo_point_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT_VECTOR, geo_point_vector)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_wgs84_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                        grn_ts_geo_point_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT_VECTOR, geo_point_vector)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return GRN_SUCCESS;
}
#undef GRN_TS_EXPR_PUSH_CONST

grn_rc
grn_ts_expr_complete(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_ts_expr_node *root;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (expr->stack_depth != 1) {
    return GRN_INVALID_ARGUMENT;
  }
  root = expr->stack[0];
  switch (root->data_kind) {
    case GRN_TS_REF:
    case GRN_TS_REF_VECTOR: {
      return GRN_INVALID_ARGUMENT;
    }
    default: {
      break;
    }
  }
  switch (root->type) {
    case GRN_TS_EXPR_ID_NODE: {
      expr->type = GRN_TS_EXPR_ID;
      break;
    }
    case GRN_TS_EXPR_SCORE_NODE: {
      expr->type = GRN_TS_EXPR_SCORE;
      break;
    }
    case GRN_TS_EXPR_KEY_NODE:
    case GRN_TS_EXPR_VALUE_NODE: {
      expr->type = GRN_TS_EXPR_VARIABLE;
      break;
    }
    case GRN_TS_EXPR_CONST_NODE: {
      expr->type = GRN_TS_EXPR_CONST;
      break;
    }
    case GRN_TS_EXPR_COLUMN_NODE:
    case GRN_TS_EXPR_OP_NODE: {
      expr->type = GRN_TS_EXPR_VARIABLE;
      break;
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
  expr->data_type = root->data_type;
  expr->data_kind = root->data_kind;
  expr->root = root;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_evaluate(grn_ctx *ctx, grn_ts_expr *expr,
                     const grn_ts_record *in, size_t n_in, void *out) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_INCOMPLETE) ||
      (expr->type == GRN_TS_EXPR_BROKEN) || (!in && n_in) || (n_in && !out)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!n_in) {
    return GRN_SUCCESS;
  }
  return grn_ts_expr_node_evaluate(ctx, expr->root, in, n_in, out);
}

grn_rc
grn_ts_expr_filter(grn_ctx *ctx, grn_ts_expr *expr,
                   grn_ts_record *in, size_t n_in,
                   grn_ts_record *out, size_t *n_out) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_INCOMPLETE) ||
      (expr->type == GRN_TS_EXPR_BROKEN) || (!in && n_in) ||
      !out || !n_out) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!n_in) {
    *n_out = 0;
    return GRN_SUCCESS;
  }
  return grn_ts_expr_node_filter(ctx, expr->root, in, n_in, out, n_out);
}

grn_rc
grn_ts_expr_adjust(grn_ctx *ctx, grn_ts_expr *expr,
                   grn_ts_record *io, size_t n_io) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_INCOMPLETE) ||
      (expr->type == GRN_TS_EXPR_BROKEN) || (!io && n_io)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!n_io) {
    return GRN_SUCCESS;
  }
  return grn_ts_expr_node_adjust(ctx, expr->root, io, n_io);
}

/*-------------------------------------------------------------
 * API.
 */

/* grn_ts_select_filter() applies a filter to all the records of a table. */
static grn_rc
grn_ts_select_filter(grn_ctx *ctx, grn_obj *table,
                     const char *str, size_t str_size,
                     size_t offset, size_t limit,
                     grn_ts_record **out, size_t *n_out, size_t *n_hits) {
  grn_rc rc;
  grn_table_cursor *cursor;
  grn_ts_expr *expr;
  grn_ts_record *buf = NULL;
  size_t buf_size = 0;

  *out = NULL;
  *n_out = 0;
  *n_hits = 0;

  cursor = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1,
                                 GRN_CURSOR_ASCENDING | GRN_CURSOR_BY_ID);
  if (!cursor) {
    return (ctx->rc != GRN_SUCCESS) ? ctx->rc : GRN_UNKNOWN_ERROR;
  }

  rc = grn_ts_expr_parse(ctx, table, str, str_size, &expr);
  if (rc == GRN_SUCCESS) {
    for ( ; ; ) {
      size_t i, batch_size;
      grn_ts_record *batch;

      /* Extend the record buffer. */
      if (buf_size < (*n_out + GRN_TS_BATCH_SIZE)) {
        size_t new_size = buf_size ? (buf_size * 2) : GRN_TS_BATCH_SIZE;
        size_t n_bytes = sizeof(grn_ts_record) * new_size;
        grn_ts_record *new_buf = (grn_ts_record *)GRN_REALLOC(buf, n_bytes);
        if (!new_buf) {
          rc = GRN_NO_MEMORY_AVAILABLE;
          break;
        }
        buf = new_buf;
        buf_size = new_size;
      }

      /* Read records from the cursor. */
      batch = buf + *n_out;
      for (i = 0; i < GRN_TS_BATCH_SIZE; i++) {
        batch[i].id = grn_table_cursor_next(ctx, cursor);
        if (batch[i].id == GRN_ID_NIL) {
          break;
        }
        batch[i].score = 0.0;
      }
      batch_size = i;
      if (!batch_size) {
        break;
      }

      /* Apply the filter. */
      rc = grn_ts_expr_filter(ctx, expr, batch, batch_size,
                              batch, &batch_size);
      if (rc != GRN_SUCCESS) {
        break;
      }
      *n_hits += batch_size;

      /* Apply the offset and the limit. */
      if (offset) {
        if (batch_size <= offset) {
          offset -= batch_size;
          batch_size = 0;
        } else {
          size_t n_bytes = sizeof(grn_ts_record) * (batch_size - offset);
          grn_memcpy(batch, batch + offset, n_bytes);
          batch_size -= offset;
          offset = 0;
        }
      }
      if (batch_size <= limit) {
        limit -= batch_size;
      } else {
        batch_size = limit;
        limit = 0;
      }
      *n_out += batch_size;
    }
    /* Ignore a failure of destruction. */
    grn_ts_expr_close(ctx, expr);
  }
  /* Ignore a failure of  destruction. */
  grn_table_cursor_close(ctx, cursor);

  if (rc != GRN_SUCCESS) {
    if (buf) {
      GRN_FREE(buf);
    }
    *n_out = 0;
    *n_hits = 0;
    return rc;
  }
  *out = buf;
  return GRN_SUCCESS;
}

/* grn_ts_select_output_parse() parses an output_columns option. */
static grn_rc
grn_ts_select_output_parse(grn_ctx *ctx, grn_obj *table,
                           const char *str, size_t str_size,
                           grn_obj *name_buf) {
  const char *rest = str;
  size_t i, rest_size = str_size;
  while (rest_size) {
    const char *name = rest;
    size_t name_size;

    /* Find a delimiter. */
    for (i = 0; i < rest_size; i++) {
      if (rest[i] == ',') {
        break;
      }
    }
    name_size = i;

    rest += name_size;
    rest_size -= name_size;
    if (rest_size) {
      rest++;
      rest_size--;
    }

    /* Trim spaces. */
    for (i = 0; i < name_size; i++) {
      if (!isspace((unsigned char)name[i])) {
        break;
      }
    }
    name += i;
    name_size -= i;
    for (i = name_size; i > 0; i--) {
      if (!isspace((unsigned char)name[i - 1])) {
        break;
      }
    }
    name_size = i;
    if (!name_size) {
      continue;
    }

    /* Add column names to name_buf. */
    if (name[name_size - 1] == '*') {
      /* Expand a wildcard. */
      grn_hash *columns = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                          GRN_OBJ_TABLE_HASH_KEY |
                                          GRN_HASH_TINY);
      if (columns) {
        if (grn_table_columns(ctx, table, "", 0, (grn_obj *)columns)) {
          grn_id *key;
          GRN_HASH_EACH(ctx, columns, id, &key, NULL, NULL, {
            grn_obj *column = grn_ctx_at(ctx, *key);
            if (column) {
              char buf[1024];
              size_t len = grn_column_name(ctx, column, buf, 1024);
              grn_vector_add_element(ctx, name_buf, buf, len, 0, GRN_DB_TEXT);
              grn_obj_unlink(ctx, column);
            }
          });
        }
        grn_hash_close(ctx, columns);
      }
    } else {
      grn_vector_add_element(ctx, name_buf, name, name_size, 0, GRN_DB_TEXT);
    }
  }
  return GRN_SUCCESS;
}

#define GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(TYPE, name)\
  case GRN_DB_ ## TYPE: {\
    GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, name);\
    break;\
  }
/* grn_ts_select_output_columns() outputs column names and data types. */
// FIXME: Errors are ignored.
static grn_rc
grn_ts_select_output_columns(grn_ctx *ctx, const grn_ts_text *names,
                             grn_ts_expr **exprs, size_t n_exprs) {
  GRN_OUTPUT_ARRAY_OPEN("COLUMNS", n_exprs);
  for (size_t i = 0; i < n_exprs; ++i) {
    GRN_OUTPUT_ARRAY_OPEN("COLUMN", 2);
    /* Output a column name. */
    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
    GRN_TEXT_PUT(ctx, ctx->impl->outbuf, names[i].ptr, names[i].size);
    GRN_TEXT_PUT(ctx, ctx->impl->outbuf, "\",\"", 3);
    /* Output a data type. */
    switch (exprs[i]->data_type) {
      case GRN_DB_VOID: {
        if (exprs[i]->data_kind == GRN_TS_GEO_POINT) {
          GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "GeoPoint");
        } else {
          GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Void");
        }
        break;
      }
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(BOOL, "Bool")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(INT8, "Int8")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(INT16, "Int16")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(INT32, "Int32")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(INT64, "Int64")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(UINT8, "UInt8")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(UINT16, "UInt16")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(UINT32, "UInt32")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(UINT64, "UInt64")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(FLOAT, "Float")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(TIME, "Time")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(SHORT_TEXT, "ShortText")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(TEXT, "Text")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(LONG_TEXT, "LongText")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(TOKYO_GEO_POINT, "TokyoGeoPoint")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK(WGS84_GEO_POINT, "WGS84GeoPoint")
      default: {
        grn_obj *obj = grn_ctx_at(ctx, exprs[i]->data_type);
        if (obj && grn_ts_obj_is_table(ctx, obj)) {
          char name[GRN_TABLE_MAX_KEY_SIZE];
          int len = grn_obj_name(ctx, obj, name, sizeof(name));
          GRN_TEXT_PUT(ctx, ctx->impl->outbuf, name, len);
          grn_obj_unlink(ctx, obj);
        } else {
          GRN_TEXT_PUTS(ctx, ctx->impl->outbuf, "Unknown");
        }
        break;
      }
    }
    GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, '"');
    GRN_OUTPUT_ARRAY_CLOSE();
  }
  GRN_OUTPUT_ARRAY_CLOSE(); /* COLUMNS. */
  return GRN_SUCCESS;
}
#undef GRN_TS_SELECT_OUTPUT_COLUMNS_CASE_BLOCK

/* grn_ts_select_output_values() evaluates expressions and output results. */
// FIXME: Errors are ignored.
static grn_rc
grn_ts_select_output_values(grn_ctx *ctx, const grn_ts_record *in, size_t n_in,
                            grn_ts_expr **exprs, size_t n_exprs) {
  grn_rc rc = GRN_SUCCESS;
  size_t i, j, count = 0;
  void **bufs;

  if (!n_in) {
    return GRN_SUCCESS;
  }
  bufs = GRN_MALLOCN(void *, n_exprs);
  if (!bufs) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  /* Allocate memory for results. */
  for (i = 0; i < n_exprs; i++) {
    switch (exprs[i]->data_kind) {
#define GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    bufs[i] = GRN_MALLOCN(grn_ts_ ## kind, GRN_TS_BATCH_SIZE);\
    break;\
  }
#define GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK(KIND, kind)\
  GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(KIND ## _VECTOR, kind ## _vector)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(BOOL, bool)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(INT, int)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(FLOAT, float)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(TIME, time)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(TEXT, text)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK(GEO_POINT, geo_point)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK(BOOL, bool)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK(INT, int)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK(FLOAT, float)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK(TIME, time)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK(TEXT, text)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK(GEO_POINT, geo_point)
#undef GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE_BLOCK
#undef GRN_TS_SELECT_OUTPUT_MALLOC_CASE_BLOCK
      default: {
        bufs[i] = NULL;
        // FIXME: GRN_OBJECT_CORRUPT?
        break;
      }
    }
    if (!bufs[i]) {
      while (i) {
        GRN_FREE(bufs[--i]);
      }
      GRN_FREE(bufs);
      return GRN_NO_MEMORY_AVAILABLE;
    }
  }

  /* Evaluate expressions and output results. */
  while (count < n_in) {
    size_t batch_size = GRN_TS_BATCH_SIZE;
    if (batch_size > (n_in - count)) {
      batch_size = n_in - count;
    }
    for (i = 0; i < n_exprs; ++i) {
      if (!bufs[i]) {
        continue;
      }
      rc = grn_ts_expr_evaluate(ctx, exprs[i], in + count,
                                batch_size, bufs[i]);
      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    for (i = 0; i < batch_size; ++i) {
      GRN_OUTPUT_ARRAY_OPEN("HIT", n_exprs);
      for (j = 0; j < n_exprs; ++j) {
        if (j) {
          GRN_TEXT_PUTC(ctx, ctx->impl->outbuf, ',');
        }
#define GRN_TS_SELECT_OUTPUT_CASE_BLOCK(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_ts_ ## kind *value = (grn_ts_ ## kind *)bufs[j];\
    grn_ts_ ## kind ## _output(ctx, value[i]);\
    break;\
  }
#define GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK(KIND, kind)\
  GRN_TS_SELECT_OUTPUT_CASE_BLOCK(KIND ## _VECTOR, kind ## _vector)
        switch (exprs[j]->data_kind) {
          GRN_TS_SELECT_OUTPUT_CASE_BLOCK(BOOL, bool);
          GRN_TS_SELECT_OUTPUT_CASE_BLOCK(INT, int);
          GRN_TS_SELECT_OUTPUT_CASE_BLOCK(FLOAT, float);
          GRN_TS_SELECT_OUTPUT_CASE_BLOCK(TIME, time);
          GRN_TS_SELECT_OUTPUT_CASE_BLOCK(TEXT, text);
          GRN_TS_SELECT_OUTPUT_CASE_BLOCK(GEO_POINT, geo_point);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK(BOOL, bool);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK(INT, int);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK(FLOAT, float);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK(TIME, time);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK(TEXT, text);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK(GEO_POINT, geo_point);
#undef GRN_TS_SELECT_OUTPUT_VECTOR_CASE_BLOCK
#undef GRN_TS_SELECT_OUTPUT_CASE_BLOCK
          default: {
            break;
          }
        }
      }
      GRN_OUTPUT_ARRAY_CLOSE(); /* HITS. */
    }
    count += batch_size;
  }

  for (i = 0; i < n_exprs; i++) {
    if (bufs[i]) {
      GRN_FREE(bufs[i]);
    }
  }
  GRN_FREE(bufs);
  return rc;
}

/* grn_ts_select_output() outputs the results. */
/* FIXME: Errors are ignored. */
static grn_rc
grn_ts_select_output(grn_ctx *ctx, grn_obj *table,
                     const char *str, size_t str_size,
                     const grn_ts_record *in, size_t n_in, size_t n_hits) {
  grn_rc rc;
  grn_ts_expr **exprs = NULL;
  size_t i, n_exprs = 0;
  grn_obj name_buf;
  grn_ts_text *names = NULL;

  GRN_TEXT_INIT(&name_buf, GRN_OBJ_VECTOR);
  rc = grn_ts_select_output_parse(ctx, table, str, str_size, &name_buf);
  if (rc != GRN_SUCCESS) {
    GRN_OBJ_FIN(ctx, &name_buf);
    return rc;
  }

  /* Create expressions. */
  n_exprs = grn_vector_size(ctx, &name_buf);
  if (n_exprs) {
    size_t count = 0;
    names = GRN_MALLOCN(grn_ts_text, n_exprs);
    exprs = GRN_MALLOCN(grn_ts_expr *, n_exprs);
    if (!names || !exprs) {
      n_exprs = 0;
    }
    for (i = 0; i < n_exprs; i++) {
      exprs[i] = NULL;
    }
    for (i = 0; i < n_exprs; i++) {
      names[i].size = grn_vector_get_element(ctx, &name_buf, i, &names[i].ptr,
                                             NULL, NULL);
      rc = grn_ts_expr_parse(ctx, table, names[i].ptr, names[i].size,
                             &exprs[i]);
      if (rc != GRN_SUCCESS) {
        if ((names[i].size == GRN_COLUMN_NAME_KEY_LEN) &&
            !memcmp(names[i].ptr, GRN_COLUMN_NAME_KEY,
                    GRN_COLUMN_NAME_KEY_LEN)) {
          /*
           * Ignore an error for _key because the default output_columns option
           * contains _key.
           */
          rc = GRN_SUCCESS;
        } else {
          break;
        }
      }
    }
    for (i = 0; i < n_exprs; i++) {
      if (exprs[i]) {
        names[count] = names[i];
        exprs[count] = exprs[i];
        count++;
      }
    }
    n_exprs = count;
  }

  if (rc == GRN_SUCCESS) {
    GRN_OUTPUT_ARRAY_OPEN("RESULT", 1);
    GRN_OUTPUT_ARRAY_OPEN("RESULTSET", 2 + n_in);

    GRN_OUTPUT_ARRAY_OPEN("NHITS", 1);
    rc = grn_text_ulltoa(ctx, ctx->impl->outbuf, n_hits);
    GRN_OUTPUT_ARRAY_CLOSE(); /* NHITS. */

    if (rc == GRN_SUCCESS) {
      rc = grn_ts_select_output_columns(ctx, names, exprs, n_exprs);
      if (rc == GRN_SUCCESS) {
        rc = grn_ts_select_output_values(ctx, in, n_in, exprs, n_exprs);
      }
    }

    GRN_OUTPUT_ARRAY_CLOSE(); /* RESULTSET. */
    GRN_OUTPUT_ARRAY_CLOSE(); /* RESET. */
  }

  /* Finalize. */
  if (exprs) {
    for (i = 0; i < n_exprs; i++) {
      if (exprs[i]) {
        /* Ignore a failure of destruction. */
        grn_ts_expr_close(ctx, exprs[i]);
      }
    }
    GRN_FREE(exprs);
  }
  if (names) {
    GRN_FREE(names);
  }
  GRN_OBJ_FIN(ctx, &name_buf);
  return rc;
}

grn_rc
grn_ts_select(grn_ctx *ctx, grn_obj *table,
              const char *filter, size_t filter_size,
              const char *output_columns, size_t output_columns_size,
              size_t offset, size_t limit) {
  grn_rc rc;
  grn_ts_record *records = NULL;
  size_t n_records, n_hits;
  if (!ctx || !table || !grn_ts_obj_is_table(ctx, table) ||
      (!filter && filter_size) || (!output_columns && output_columns_size)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_select_filter(ctx, table, filter, filter_size, offset, limit,
                            &records, &n_records, &n_hits);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_select_output(ctx, table, output_columns, output_columns_size,
                              records, n_records, n_hits);
  }
  if (records) {
    GRN_FREE(records);
  }
  return rc;
}

#endif /* GRN_WITH_TS */
