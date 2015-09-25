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
 * grn_ts_str.
 */

/* grn_ts_byte_is_decimal() returns whether or not a byte is decimal. */
inline static grn_ts_bool
grn_ts_byte_is_decimal(unsigned char byte) {
  return (byte >= '0') && (byte <= '9');
}

/*
 * grn_ts_byte_is_name_char() returns whether or not a byte is allowed as a
 * part of a name.
 */
inline static grn_ts_bool
grn_ts_byte_is_name_char(unsigned char byte) {
  /*
   * Note: A table name allows '#', '@' and '-'.
   * http://groonga.org/docs/reference/commands/table_create.html#name
   */
  if (((byte >= '0') && (byte <= '9')) || ((byte >= 'A') && (byte <= 'Z')) ||
      ((byte >= 'a') && (byte <= 'z')) || (byte == '_')) {
    return GRN_TRUE;
  }
  return GRN_FALSE;
}

typedef struct {
  const char *ptr; /* The starting address. */
  size_t size;     /* The size in bytes. */
} grn_ts_str;

/* grn_ts_str_trim_left() removes the leading spaces. */
static grn_ts_str
grn_ts_str_trim_left(grn_ts_str str) {
  size_t i;
  for (i = 0; i < str.size; i++) {
    if (!isspace((unsigned char)str.ptr[i])) {
      break;
    }
  }
  str.ptr += i;
  str.size -= i;
  return str;
}

/* grn_ts_str_is_true() returns whether or not a string is true. */
static grn_ts_bool
grn_ts_str_is_true(grn_ts_str str) {
  return (str.size == 4) && !memcmp(str.ptr, "true", 4);
}

/* grn_ts_str_is_false() returns whether or not a string is false. */
static grn_ts_bool
grn_ts_str_is_false(grn_ts_str str) {
  return (str.size == 5) && !memcmp(str.ptr, "false", 5);
}

/* grn_ts_str_is_bool() returns whether or not a string is true or false. */
static grn_ts_bool
grn_ts_str_is_bool(grn_ts_str str) {
  return grn_ts_str_is_true(str) || grn_ts_str_is_false(str);
}

/*
 * grn_ts_str_is_name_prefix() returns whether or not a string is valid as a
 * name prefix.
 */
static grn_ts_bool
grn_ts_str_is_name_prefix(grn_ts_str str) {
  size_t i;
  for (i = 0; i < str.size; i++) {
    if (!grn_ts_byte_is_name_char(str.ptr[i])) {
      return GRN_FALSE;
    }
  }
  return GRN_TRUE;
}

/* grn_ts_str_is_name() returns whether or not a string is valid as a name. */
static grn_ts_bool
grn_ts_str_is_name(grn_ts_str str) {
  if (!str.size) {
    return GRN_FALSE;
  }
  return grn_ts_str_is_name_prefix(str);
}

/* grn_ts_str_is_id_name() returns whether or not a string is "_id". */
static grn_ts_bool
grn_ts_str_is_id_name(grn_ts_str str) {
  return (str.size == GRN_COLUMN_NAME_ID_LEN) &&
         !memcmp(str.ptr, GRN_COLUMN_NAME_ID, GRN_COLUMN_NAME_ID_LEN);
}

/* grn_ts_str_is_score_name() returns whether or not a string is "_score". */
static grn_ts_bool
grn_ts_str_is_score_name(grn_ts_str str) {
  return (str.size == GRN_COLUMN_NAME_SCORE_LEN) &&
         !memcmp(str.ptr, GRN_COLUMN_NAME_SCORE, GRN_COLUMN_NAME_SCORE_LEN);
}

/* grn_ts_str_is_key_name() returns whether or not a string is "_key". */
static grn_ts_bool
grn_ts_str_is_key_name(grn_ts_str str) {
  return (str.size == GRN_COLUMN_NAME_KEY_LEN) &&
         !memcmp(str.ptr, GRN_COLUMN_NAME_KEY, GRN_COLUMN_NAME_KEY_LEN);
}

/* grn_ts_str_is_value_name() returns whether or not a string is "_value". */
static grn_ts_bool
grn_ts_str_is_value_name(grn_ts_str str) {
  return (str.size == GRN_COLUMN_NAME_VALUE_LEN) &&
         !memcmp(str.ptr, GRN_COLUMN_NAME_VALUE, GRN_COLUMN_NAME_VALUE_LEN);
}

/*-------------------------------------------------------------
 * grn_ts_buf.
 */

typedef struct {
  void *ptr;   /* The starting address. */
  size_t size; /* The size in bytes. */
  size_t pos;  /* The current position. */
} grn_ts_buf;

/* grn_ts_buf_init() initializes a buffer. */
static void
grn_ts_buf_init(grn_ctx *ctx, grn_ts_buf *buf) {
  buf->ptr = NULL;
  buf->size = 0;
  buf->pos = 0;
}

/* grn_ts_buf_open() creates a buffer. */
/*
static grn_rc
grn_ts_buf_open(grn_ctx *ctx, grn_ts_buf **buf) {
  grn_ts_buf *new_buf = GRN_MALLOCN(grn_ts_buf, 1);
  if (!new_buf) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_ts_buf_init(ctx, new_buf);
  *buf = new_buf;
  return GRN_SUCCESS;
}
*/

/* grn_ts_buf_fin() finalizes a buffer. */
static void
grn_ts_buf_fin(grn_ctx *ctx, grn_ts_buf *buf) {
  if (buf->ptr) {
    GRN_FREE(buf->ptr);
  }
}

/* grn_ts_buf_close() destroys a buffer. */
/*
static void
grn_ts_buf_close(grn_ctx *ctx, grn_ts_buf *buf) {
  if (buf) {
    grn_ts_buf_fin(ctx, buf);
  }
}
*/

/*
 * grn_ts_buf_reserve() reserves enough memory to store new_size bytes.
 * Note that this function never shrinks a buffer and does nothing if new_size
 * is not greater than the current size.
 */
static grn_rc
grn_ts_buf_reserve(grn_ctx *ctx, grn_ts_buf *buf, size_t new_size) {
  void *new_ptr;
  size_t enough_size;
  if (new_size <= buf->size) {
    return GRN_SUCCESS;
  }
  enough_size = buf->size ? (buf->size << 1) : 1;
  while (enough_size < new_size) {
    enough_size <<= 1;
  }
  new_ptr = GRN_REALLOC(buf->ptr, enough_size);
  if (!new_ptr) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  buf->ptr = new_ptr;
  buf->size = enough_size;
  return GRN_SUCCESS;
}

/* grn_ts_buf_resize() resizes a buffer. */
static grn_rc
grn_ts_buf_resize(grn_ctx *ctx, grn_ts_buf *buf, size_t new_size) {
  void *new_ptr;
  if (new_size == buf->size) {
    return GRN_SUCCESS;
  }
  if (!new_size) {
    if (buf->ptr) {
      GRN_FREE(buf->ptr);
      buf->ptr = NULL;
      buf->size = new_size;
    }
    return GRN_SUCCESS;
  }
  new_ptr = GRN_REALLOC(buf->ptr, new_size);
  if (!new_ptr) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  buf->ptr = new_ptr;
  buf->size = new_size;
  return GRN_SUCCESS;
}

/* grn_ts_buf_write() appends data into a buffer. */
static grn_rc
grn_ts_buf_write(grn_ctx *ctx, grn_ts_buf *buf, const void *ptr, size_t size) {
  size_t new_pos = buf->pos + size;
  if (new_pos > buf->size) {
    grn_rc rc = grn_ts_buf_reserve(ctx, buf, new_pos);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  grn_memcpy((char *)buf->ptr + buf->pos, ptr, size);
  buf->pos += size;
  return GRN_SUCCESS;
}

/*-------------------------------------------------------------
 * Built-in data kinds.
 */

typedef union {
  grn_ts_bool as_bool;
  grn_ts_int as_int;
  grn_ts_float as_float;
  grn_ts_time as_time;
  grn_ts_text as_text;
  grn_ts_geo_point as_geo_point;
  grn_ts_ref as_ref;
  grn_ts_bool_vector as_bool_vector;
  grn_ts_int_vector as_int_vector;
  grn_ts_float_vector as_float_vector;
  grn_ts_time_vector as_time_vector;
  grn_ts_text_vector as_text_vector;
  grn_ts_geo_point_vector as_geo_point_vector;
  grn_ts_ref_vector as_ref_vector;
} grn_ts_any;

/* grn_ts_bool_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_bool_is_valid(grn_ts_bool value) {
  return GRN_TRUE;
}

/* grn_ts_int_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_int_is_valid(grn_ts_int value) {
  return GRN_TRUE;
}

/* grn_ts_float_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_float_is_valid(grn_ts_float value) {
  return !isnan(value);
}

/* grn_ts_time_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_time_is_valid(grn_ts_time value) {
  return GRN_TRUE;
}

/* grn_ts_text_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_text_is_valid(grn_ts_text value) {
  return value.ptr || !value.size;
}

/* grn_ts_geo_point_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
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
inline static grn_ts_bool
grn_ts_bool_vector_is_valid(grn_ts_bool_vector value) {
  GRN_TS_VECTOR_IS_VALID(bool)
}

/* grn_ts_int_vector_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_int_vector_is_valid(grn_ts_int_vector value) {
  GRN_TS_VECTOR_IS_VALID(int)
}

/* grn_ts_float_vector_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_float_vector_is_valid(grn_ts_float_vector value) {
  GRN_TS_VECTOR_IS_VALID(float)
}

/* grn_ts_time_vector_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_time_vector_is_valid(grn_ts_time_vector value) {
  GRN_TS_VECTOR_IS_VALID(time)
}

/* grn_ts_text_vector_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_text_vector_is_valid(grn_ts_text_vector value) {
  GRN_TS_VECTOR_IS_VALID(text)
}

/*
 * grn_ts_geo_point_vector_is_valid() returns whether a value is valid or
 * not.
 */
inline static grn_ts_bool
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
 * Operators.
 */

/* Operator precedence. */
typedef int grn_ts_op_precedence;

/* grn_ts_op_get_n_args() returns the number of arguments. */
static size_t
grn_ts_op_get_n_args(grn_ts_op_type op_type) {
  switch (op_type) {
    case GRN_TS_OP_LOGICAL_NOT:    /* !X */
    case GRN_TS_OP_BITWISE_NOT:    /* ~X */
    case GRN_TS_OP_POSITIVE:       /* +X */
    case GRN_TS_OP_NEGATIVE: {     /* -X */
      return 1;
    }
    case GRN_TS_OP_LOGICAL_AND:    /* X && Y */
    case GRN_TS_OP_LOGICAL_OR:     /* X || Y */
    case GRN_TS_OP_BITWISE_AND:    /* X & Y  */
    case GRN_TS_OP_BITWISE_OR:     /* X | Y  */
    case GRN_TS_OP_BITWISE_XOR:    /* X ^ Y  */
    case GRN_TS_OP_EQUAL:          /* X == Y */
    case GRN_TS_OP_NOT_EQUAL:      /* X != Y */
    case GRN_TS_OP_LESS:           /* X < Y  */
    case GRN_TS_OP_LESS_EQUAL:     /* X <= Y */
    case GRN_TS_OP_GREATER:        /* X > Y  */
    case GRN_TS_OP_GREATER_EQUAL:  /* X >= Y */
    case GRN_TS_OP_PLUS:           /* X + Y  */
    case GRN_TS_OP_MINUS:          /* X - Y  */
    case GRN_TS_OP_MULTIPLICATION: /* X * Y  */
    case GRN_TS_OP_DIVISION:       /* X / Y  */
    case GRN_TS_OP_MODULUS: {      /* X % Y  */
      return 2;
    }
    default: {
      return 0;
    }
  }
}

/*
 * grn_ts_op_get_precedence() returns the precedence.
 * A prior operator has a higher precedence.
 */
static grn_ts_op_precedence
grn_ts_op_get_precedence(grn_ts_op_type op_type) {
  switch (op_type) {
    case GRN_TS_OP_LOGICAL_NOT:
    case GRN_TS_OP_BITWISE_NOT:
    case GRN_TS_OP_POSITIVE:
    case GRN_TS_OP_NEGATIVE: {
      return 14;
    }
    case GRN_TS_OP_LOGICAL_AND: {
      return 5;
    }
    case GRN_TS_OP_LOGICAL_OR: {
      return 4;
    }
    case GRN_TS_OP_EQUAL:
    case GRN_TS_OP_NOT_EQUAL: {
      return 9;
    }
    case GRN_TS_OP_LESS:
    case GRN_TS_OP_LESS_EQUAL:
    case GRN_TS_OP_GREATER:
    case GRN_TS_OP_GREATER_EQUAL: {
      return 10;
    }
    case GRN_TS_OP_BITWISE_AND: {
      return 8;
    }
    case GRN_TS_OP_BITWISE_OR: {
      return 6;
    }
    case GRN_TS_OP_BITWISE_XOR: {
      return 7;
    }
    case GRN_TS_OP_PLUS:
    case GRN_TS_OP_MINUS: {
      return 12;
    }
    case GRN_TS_OP_MULTIPLICATION:
    case GRN_TS_OP_DIVISION:
    case GRN_TS_OP_MODULUS: {
      return 13;
    }
    default: {
      return 0;
    }
  }
}

/* FIXME: The following implementation assumes that NaN values don't appear. */

/* grn_ts_op_equal_bool() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_bool(grn_ts_bool lhs, grn_ts_bool rhs) {
  return lhs == rhs;
}

/* grn_ts_op_equal_int() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs == rhs;
}

/* grn_ts_op_equal_float() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_float(grn_ts_float lhs, grn_ts_float rhs) {
  /* To suppress warnings, "lhs == rhs" is not used. */
  return (lhs <= rhs) && (lhs >= rhs);
}

/* grn_ts_op_equal_time() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_time(grn_ts_time lhs, grn_ts_time rhs) {
  return lhs == rhs;
}

/* grn_ts_op_equal_text() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_text(grn_ts_text lhs, grn_ts_text rhs) {
  return (lhs.size == rhs.size) && !memcmp(lhs.ptr, rhs.ptr, lhs.size);
}

/* grn_ts_op_equal_geo_point() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_geo_point(grn_ts_geo_point lhs, grn_ts_geo_point rhs) {
  return (lhs.latitude == rhs.latitude) && (lhs.longitude == rhs.longitude);
}

/* grn_ts_op_equal_ref() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_ref(grn_ts_ref lhs, grn_ts_ref rhs) {
  /* Ignore scores. */
  return lhs.id == rhs.id;
}

#define GRN_TS_OP_EQUAL_VECTOR(kind)\
  size_t i;\
  if (lhs.size != rhs.size) {\
    return GRN_FALSE;\
  }\
  for (i = 0; i < lhs.size; i++) {\
    if (!grn_ts_op_equal_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
      return GRN_FALSE;\
    }\
  }\
  return GRN_TRUE;
/* grn_ts_op_equal_bool_vector() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_bool_vector(grn_ts_bool_vector lhs, grn_ts_bool_vector rhs) {
  GRN_TS_OP_EQUAL_VECTOR(bool)
}

/* grn_ts_op_equal_int_vector() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_int_vector(grn_ts_int_vector lhs, grn_ts_int_vector rhs) {
  GRN_TS_OP_EQUAL_VECTOR(int)
}

/* grn_ts_op_equal_float_vector() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_float_vector(grn_ts_float_vector lhs,
                             grn_ts_float_vector rhs) {
  GRN_TS_OP_EQUAL_VECTOR(float)
}

/* grn_ts_op_equal_time_vector() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_time_vector(grn_ts_time_vector lhs, grn_ts_time_vector rhs) {
  GRN_TS_OP_EQUAL_VECTOR(time)
}

/* grn_ts_op_equal_text_vector() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_text_vector(grn_ts_text_vector lhs, grn_ts_text_vector rhs) {
  GRN_TS_OP_EQUAL_VECTOR(text)
}

/* grn_ts_op_equal_geo_point_vector() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_geo_point_vector(grn_ts_geo_point_vector lhs,
                                 grn_ts_geo_point_vector rhs) {
  GRN_TS_OP_EQUAL_VECTOR(geo_point)
}

/* grn_ts_op_equal_ref_vector() returns lhs == rhs. */
inline static grn_ts_bool
grn_ts_op_equal_ref_vector(grn_ts_ref_vector lhs, grn_ts_ref_vector rhs) {
  GRN_TS_OP_EQUAL_VECTOR(ref)
}
#undef GRN_TS_OP_EQUAL_VECTOR

/* grn_ts_op_not_equal_bool() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_bool(grn_ts_bool lhs, grn_ts_bool rhs) {
  return lhs != rhs;
}

/* grn_ts_op_not_equal_int() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs != rhs;
}

/* grn_ts_op_not_equal_float() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_float(grn_ts_float lhs, grn_ts_float rhs) {
  /* To suppress warnings, "lhs != rhs" is not used. */
  return (lhs < rhs) || (lhs > rhs);
}

/* grn_ts_op_not_equal_time() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_time(grn_ts_time lhs, grn_ts_time rhs) {
  return lhs != rhs;
}

/* grn_ts_op_not_equal_text() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_text(grn_ts_text lhs, grn_ts_text rhs) {
  return (lhs.size != rhs.size) && !memcmp(lhs.ptr, rhs.ptr, lhs.size);
}

/* grn_ts_op_not_equal_geo_point() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_geo_point(grn_ts_geo_point lhs, grn_ts_geo_point rhs) {
  return (lhs.latitude != rhs.latitude) && (lhs.longitude != rhs.longitude);
}

/* grn_ts_op_not_equal_ref() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_ref(grn_ts_ref lhs, grn_ts_ref rhs) {
  /* Ignore scores. */
  return lhs.id != rhs.id;
}

#define GRN_TS_OP_NOT_EQUAL_VECTOR(kind)\
  size_t i;\
  if (lhs.size != rhs.size) {\
    return GRN_TRUE;\
  }\
  for (i = 0; i < lhs.size; i++) {\
    if (grn_ts_op_not_equal_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
      return GRN_TRUE;\
    }\
  }\
  return GRN_FALSE;
/* grn_ts_op_not_equal_bool_vector() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_bool_vector(grn_ts_bool_vector lhs,
                                grn_ts_bool_vector rhs) {
  GRN_TS_OP_NOT_EQUAL_VECTOR(bool)
}

/* grn_ts_op_not_equal_int_vector() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_int_vector(grn_ts_int_vector lhs, grn_ts_int_vector rhs) {
  GRN_TS_OP_NOT_EQUAL_VECTOR(int)
}

/* grn_ts_op_not_equal_float_vector() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_float_vector(grn_ts_float_vector lhs,
                             grn_ts_float_vector rhs) {
  GRN_TS_OP_NOT_EQUAL_VECTOR(float)
}

/* grn_ts_op_not_equal_time_vector() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_time_vector(grn_ts_time_vector lhs,
                                grn_ts_time_vector rhs) {
  GRN_TS_OP_NOT_EQUAL_VECTOR(time)
}

/* grn_ts_op_not_equal_text_vector() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_text_vector(grn_ts_text_vector lhs,
                                grn_ts_text_vector rhs) {
  GRN_TS_OP_NOT_EQUAL_VECTOR(text)
}

/* grn_ts_op_not_equal_geo_point_vector() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_geo_point_vector(grn_ts_geo_point_vector lhs,
                                 grn_ts_geo_point_vector rhs) {
  GRN_TS_OP_NOT_EQUAL_VECTOR(geo_point)
}

/* grn_ts_op_not_equal_ref_vector() returns lhs != rhs. */
inline static grn_ts_bool
grn_ts_op_not_equal_ref_vector(grn_ts_ref_vector lhs, grn_ts_ref_vector rhs) {
  GRN_TS_OP_NOT_EQUAL_VECTOR(ref)
}
#undef GRN_TS_OP_NOT_EQUAL_VECTOR

/* grn_ts_op_less_int() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs < rhs;
}

/* grn_ts_op_less_float() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs < rhs;
}

/* grn_ts_op_less_time() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_time(grn_ts_time lhs, grn_ts_time rhs) {
  return lhs < rhs;
}

/* grn_ts_op_less_text() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_text(grn_ts_text lhs, grn_ts_text rhs) {
  size_t min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;
  int cmp = memcmp(lhs.ptr, rhs.ptr, min_size);
  return cmp ? (cmp < 0) : (lhs.size < rhs.size);
}

#define GRN_TS_OP_LESS_VECTOR(kind)\
  size_t i, min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;\
  for (i = 0; i < min_size; i++) {\
    if (grn_ts_op_not_equal_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
      if (grn_ts_op_less_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
        return GRN_TRUE;\
      }\
    }\
  }\
  return lhs.size < rhs.size;
/* grn_ts_op_less_int_vector() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_int_vector(grn_ts_int_vector lhs, grn_ts_int_vector rhs) {
  GRN_TS_OP_LESS_VECTOR(int)
}

/* grn_ts_op_less_float_vector() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_float_vector(grn_ts_float_vector lhs, grn_ts_float_vector rhs) {
  GRN_TS_OP_LESS_VECTOR(float)
}

/* grn_ts_op_less_time_vector() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_time_vector(grn_ts_time_vector lhs, grn_ts_time_vector rhs) {
  GRN_TS_OP_LESS_VECTOR(time)
}

/* grn_ts_op_less_text_vector() returns lhs < rhs. */
inline static grn_ts_bool
grn_ts_op_less_text_vector(grn_ts_text_vector lhs, grn_ts_text_vector rhs) {
  GRN_TS_OP_LESS_VECTOR(text)
}
#undef GRN_TS_OP_LESS_VECTOR

/* grn_ts_op_less_equal_int() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs <= rhs;
}

/* grn_ts_op_less_equal_float() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs <= rhs;
}

/* grn_ts_op_less_equal_time() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_time(grn_ts_time lhs, grn_ts_time rhs) {
  return lhs <= rhs;
}

/* grn_ts_op_less_equal_text() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_text(grn_ts_text lhs, grn_ts_text rhs) {
  size_t min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;
  int cmp = memcmp(lhs.ptr, rhs.ptr, min_size);
  return cmp ? (cmp < 0) : (lhs.size <= rhs.size);
}

#define GRN_TS_OP_LESS_EQUAL_VECTOR(kind)\
  size_t i, min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;\
  for (i = 0; i < min_size; i++) {\
    if (grn_ts_op_not_equal_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
      if (grn_ts_op_less_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
        return GRN_TRUE;\
      }\
    }\
  }\
  return lhs.size <= rhs.size;
/* grn_ts_op_less_equal_int_vector() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_int_vector(grn_ts_int_vector lhs, grn_ts_int_vector rhs) {
  GRN_TS_OP_LESS_EQUAL_VECTOR(int)
}

/* grn_ts_op_less_equal_float_vector() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_float_vector(grn_ts_float_vector lhs,
                                  grn_ts_float_vector rhs) {
  GRN_TS_OP_LESS_EQUAL_VECTOR(float)
}

/* grn_ts_op_less_equal_time_vector() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_time_vector(grn_ts_time_vector lhs,
                                 grn_ts_time_vector rhs) {
  GRN_TS_OP_LESS_EQUAL_VECTOR(time)
}

/* grn_ts_op_less_equal_text_vector() returns lhs <= rhs. */
inline static grn_ts_bool
grn_ts_op_less_equal_text_vector(grn_ts_text_vector lhs,
                                 grn_ts_text_vector rhs) {
  GRN_TS_OP_LESS_EQUAL_VECTOR(text)
}
#undef GRN_TS_OP_LESS_EQUAL_VECTOR

/* grn_ts_op_greater_int() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs > rhs;
}

/* grn_ts_op_greater_float() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs > rhs;
}

/* grn_ts_op_greater_time() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_time(grn_ts_time lhs, grn_ts_time rhs) {
  return lhs > rhs;
}

/* grn_ts_op_greater_text() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_text(grn_ts_text lhs, grn_ts_text rhs) {
  size_t min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;
  int cmp = memcmp(lhs.ptr, rhs.ptr, min_size);
  return cmp ? (cmp > 0) : (lhs.size > rhs.size);
}

#define GRN_TS_OP_GREATER_VECTOR(kind)\
  size_t i, min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;\
  for (i = 0; i < min_size; i++) {\
    if (grn_ts_op_not_equal_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
      if (grn_ts_op_greater_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
        return GRN_TRUE;\
      }\
    }\
  }\
  return lhs.size > rhs.size;
/* grn_ts_op_greater_int_vector() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_int_vector(grn_ts_int_vector lhs, grn_ts_int_vector rhs) {
  GRN_TS_OP_GREATER_VECTOR(int)
}

/* grn_ts_op_greater_float_vector() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_float_vector(grn_ts_float_vector lhs,
                               grn_ts_float_vector rhs) {
  GRN_TS_OP_GREATER_VECTOR(float)
}

/* grn_ts_op_greater_time_vector() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_time_vector(grn_ts_time_vector lhs, grn_ts_time_vector rhs) {
  GRN_TS_OP_GREATER_VECTOR(time)
}

/* grn_ts_op_greater_text_vector() returns lhs > rhs. */
inline static grn_ts_bool
grn_ts_op_greater_text_vector(grn_ts_text_vector lhs, grn_ts_text_vector rhs) {
  GRN_TS_OP_GREATER_VECTOR(text)
}
#undef GRN_TS_OP_GREATER_VECTOR

/* grn_ts_op_greater_equal_int() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs >= rhs;
}

/* grn_ts_op_greater_equal_float() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs >= rhs;
}

/* grn_ts_op_greater_equal_time() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_time(grn_ts_time lhs, grn_ts_time rhs) {
  return lhs >= rhs;
}

/* grn_ts_op_greater_equal_text() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_text(grn_ts_text lhs, grn_ts_text rhs) {
  size_t min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;
  int cmp = memcmp(lhs.ptr, rhs.ptr, min_size);
  return cmp ? (cmp > 0) : (lhs.size >= rhs.size);
}

#define GRN_TS_OP_GREATER_EQUAL_VECTOR(kind)\
  size_t i, min_size = (lhs.size < rhs.size) ? lhs.size : rhs.size;\
  for (i = 0; i < min_size; i++) {\
    if (grn_ts_op_not_equal_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
      if (grn_ts_op_greater_ ## kind(lhs.ptr[i], rhs.ptr[i])) {\
        return GRN_TRUE;\
      }\
    }\
  }\
  return lhs.size >= rhs.size;
/* grn_ts_op_greater_equal_int_vector() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_int_vector(grn_ts_int_vector lhs,
                                   grn_ts_int_vector rhs) {
  GRN_TS_OP_GREATER_EQUAL_VECTOR(int)
}

/* grn_ts_op_greater_equal_float_vector() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_float_vector(grn_ts_float_vector lhs,
                                     grn_ts_float_vector rhs) {
  GRN_TS_OP_GREATER_EQUAL_VECTOR(float)
}

/* grn_ts_op_greater_equal_time_vector() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_time_vector(grn_ts_time_vector lhs,
                                    grn_ts_time_vector rhs) {
  GRN_TS_OP_GREATER_EQUAL_VECTOR(time)
}

/* grn_ts_op_greater_equal_text_vector() returns lhs >= rhs. */
inline static grn_ts_bool
grn_ts_op_greater_equal_text_vector(grn_ts_text_vector lhs,
                                    grn_ts_text_vector rhs) {
  GRN_TS_OP_GREATER_EQUAL_VECTOR(text)
}
#undef GRN_TS_OP_GREATER_EQUAL_VECTOR

/* grn_ts_op_plus_int() returns lhs + rhs. */
inline static grn_ts_int
grn_ts_op_plus_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs + rhs;
}

/* grn_ts_op_plus_float() returns lhs + rhs. */
inline static grn_ts_float
grn_ts_op_plus_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs + rhs;
}

/* grn_ts_op_plus_time_int() returns lhs + rhs (Time + Int = Time). */
inline static grn_ts_time
grn_ts_op_plus_time_int(grn_ts_time lhs, grn_ts_int rhs) {
  return lhs + (rhs * 1000000);
}

/* grn_ts_op_plus_time_float() returns lhs + rhs (Time + Float = Time). */
inline static grn_ts_time
grn_ts_op_plus_time_float(grn_ts_time lhs, grn_ts_float rhs) {
  return lhs + (grn_ts_int)(rhs * 1000000.0);
}

/* grn_ts_op_minus_int() returns lhs - rhs. */
inline static grn_ts_int
grn_ts_op_minus_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs - rhs;
}

/* grn_ts_op_minus_float() returns lhs - rhs. */
inline static grn_ts_float
grn_ts_op_minus_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs - rhs;
}

/* grn_ts_op_minus_time_time() returns lhs - rhs (Time - Time = Float). */
inline static grn_ts_float
grn_ts_op_minus_time_time(grn_ts_time lhs, grn_ts_time rhs) {
  return (lhs - rhs) * 0.000001;
}

/* grn_ts_op_minus_time_int() returns lhs - rhs (Time - Int = Time). */
inline static grn_ts_time
grn_ts_op_minus_time_int(grn_ts_time lhs, grn_ts_int rhs) {
  return lhs - (rhs * 1000000);
}

/* grn_ts_op_minus_time_float() returns lhs - rhs (Time - Float = Time). */
inline static grn_ts_time
grn_ts_op_minus_time_float(grn_ts_time lhs, grn_ts_float rhs) {
  return lhs - (grn_ts_int)(rhs * 1000000.0);
}

/* grn_ts_op_multiplication_int() returns lhs * rhs. */
inline static grn_ts_int
grn_ts_op_multiplication_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs * rhs;
}

/* grn_ts_op_multiplication_float() returns lhs * rhs. */
inline static grn_ts_float
grn_ts_op_multiplication_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs * rhs;
}

/*
 * grn_ts_op_division_int() returns lhs / rhs.
 *
 * This function causes a critical error in the following cases:
 * - rhs == 0
 * - (lhs == INT64_MIN) && (rhs == -1)
 */
inline static grn_ts_int
grn_ts_op_division_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs / rhs;
}

/* grn_ts_op_division_float() returns lhs / rhs. */
inline static grn_ts_float
grn_ts_op_division_float(grn_ts_float lhs, grn_ts_float rhs) {
  return lhs / rhs;
}

/*
 * grn_ts_op_modulus_int() returns lhs % rhs.
 *
 * This function causes a critical error in the following cases:
 * - rhs == 0
 * - (lhs == INT64_MIN) && (rhs == -1)
 */
inline static grn_ts_int
grn_ts_op_modulus_int(grn_ts_int lhs, grn_ts_int rhs) {
  return lhs % rhs;
}

/* grn_ts_op_modulus_float() returns lhs % rhs. */
inline static grn_ts_float
grn_ts_op_modulus_float(grn_ts_float lhs, grn_ts_float rhs) {
  return fmod(lhs, rhs);
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
static grn_ts_bool
grn_ts_obj_is_table(grn_ctx *ctx, grn_obj *obj) {
  return grn_obj_is_table(ctx, obj);
}

/* grn_ts_obj_is_column() returns whether an object is a column or not */
static grn_ts_bool
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
grn_ts_ja_get_value(grn_ctx *ctx, grn_ja *ja, grn_ts_id id,
                    grn_ts_buf *buf, size_t *value_size) {
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
  rc = grn_ts_buf_write(ctx, buf, ptr, size);
  tmp_rc = grn_ja_unref(ctx, &iw);
  if (rc == GRN_SUCCESS) {
    if (tmp_rc == GRN_SUCCESS) {
      if (value_size) {
        *value_size = size;
      }
    } else {
      /* Discard the value read. */
      buf->pos = buf->size - size;
      rc = tmp_rc;
    }
  }
  return rc;
}

#define GRN_TS_TABLE_GET_KEY(type)\
  uint32_t key_size;\
  const void *key_ptr = _grn_ ## type ## _key(ctx, type, id, &key_size);\
  if (!key_ptr) {\
    return GRN_INVALID_ARGUMENT;\
  }\
/* grn_ts_hash_get_bool_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_bool_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                         grn_ts_bool *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const grn_ts_bool *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_int8_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_int8_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                         grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const int8_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_int16_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_int16_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                          grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const int16_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_int32_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_int32_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                          grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const int32_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_int64_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_int64_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                          grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const int64_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_uint8_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_uint8_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                          grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const uint8_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_uint16_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_uint16_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                           grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const uint16_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_uint32_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_uint32_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                           grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const uint32_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_uint64_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_uint64_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                           grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = (grn_ts_int)*(const uint64_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_float_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_float_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                          grn_ts_float *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const grn_ts_float *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_geo_point_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_geo_point_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                              grn_ts_geo_point *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  *key = *(const grn_ts_geo_point *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_text_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_text_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                         grn_ts_text *key) {
  GRN_TS_TABLE_GET_KEY(hash)
  key->ptr = key_ptr;
  key->size = key_size;
  return GRN_SUCCESS;
}

/* grn_ts_hash_get_ref_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_hash_get_ref_key(grn_ctx *ctx, grn_hash *hash, grn_ts_id id,
                        grn_ts_ref *key) {
  uint32_t key_size;
  const char *key_ptr = _grn_hash_key(ctx, hash, id, &key_size);
  if (!key_ptr || (key_size != sizeof(grn_ts_id))) {
    return GRN_INVALID_ARGUMENT;
  }
  key->id = *(const grn_ts_id *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_bool_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_bool_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                        grn_ts_bool *key) {
  GRN_TS_TABLE_GET_KEY(pat)
  *key = *(const grn_ts_bool *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_int8_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_int8_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                        grn_ts_int *key) {
  int8_t tmp;
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntohi(&tmp, key_ptr, sizeof(tmp));
  *key = tmp;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_int16_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_int16_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                         grn_ts_int *key) {
  int16_t tmp;
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntohi(&tmp, key_ptr, sizeof(tmp));
  *key = tmp;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_int32_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_int32_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                         grn_ts_int *key) {
  int32_t tmp;
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntohi(&tmp, key_ptr, sizeof(tmp));
  *key = tmp;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_int64_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_int64_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                         grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntohi(key, key_ptr, sizeof(grn_ts_int));
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_uint8_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_uint8_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                         grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(pat)
  *key = *(const uint8_t *)key_ptr;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_uint16_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_uint16_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                          grn_ts_int *key) {
  uint16_t tmp;
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntoh(&tmp, key_ptr, sizeof(tmp));
  *key = tmp;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_uint32_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_uint32_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                          grn_ts_int *key) {
  uint32_t tmp;
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntoh(&tmp, key_ptr, sizeof(tmp));
  *key = tmp;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_uint64_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_uint64_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                          grn_ts_int *key) {
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntoh(key, key_ptr, sizeof(grn_ts_int));
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_float_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_float_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                         grn_ts_float *key) {
  int64_t tmp;
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntoh(&tmp, key_ptr, sizeof(tmp));
  tmp ^= (((tmp ^ ((int64_t)1 << 63)) >> 63) | ((int64_t)1 << 63));
  *(int64_t *)key = tmp;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_geo_point_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_geo_point_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                             grn_ts_geo_point *key) {
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntog(key, key_ptr, sizeof(grn_ts_geo_point));
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_text_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_text_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                        grn_ts_text *key) {
  GRN_TS_TABLE_GET_KEY(pat)
  key->ptr = key_ptr;
  key->size = key_size;
  return GRN_SUCCESS;
}

/* grn_ts_pat_get_ref_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_pat_get_ref_key(grn_ctx *ctx, grn_pat *pat, grn_ts_id id,
                       grn_ts_ref *key) {
  GRN_TS_TABLE_GET_KEY(pat)
  grn_ntoh(&key->id, key_ptr, sizeof(key->id));
  return GRN_SUCCESS;
}

/* grn_ts_dat_get_text_key() gets a reference to a key (_key). */
static grn_rc
grn_ts_dat_get_text_key(grn_ctx *ctx, grn_dat *dat, grn_ts_id id,
                        grn_ts_text *key) {
  GRN_TS_TABLE_GET_KEY(dat)
  key->ptr = key_ptr;
  key->size = key_size;
  return GRN_SUCCESS;
}
#undef GRN_TS_TABLE_GET_KEY

/* grn_ts_table_has_key() returns whether a table has _key or not. */
static grn_ts_bool
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
static grn_ts_bool
grn_ts_table_has_value(grn_ctx *ctx, grn_obj *table) {
  return DB_OBJ(table)->range != GRN_DB_VOID;
}

/* grn_ts_table_get_value() gets a reference to a value (_value). */
static const void *
grn_ts_table_get_value(grn_ctx *ctx, grn_obj *table, grn_ts_id id) {
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
 * grn_ts_expr_id_node.
 */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
} grn_ts_expr_id_node;

/* grn_ts_expr_id_node_init() initializes a node. */
static void
grn_ts_expr_id_node_init(grn_ctx *ctx, grn_ts_expr_id_node *node) {
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_ID_NODE;
  node->data_kind = GRN_TS_INT;
  node->data_type = GRN_DB_UINT32;
}

/* grn_ts_expr_id_node_fin() finalizes a node. */
static void
grn_ts_expr_id_node_fin(grn_ctx *ctx, grn_ts_expr_id_node *node) {
  /* Nothing to do. */
}

/* grn_ts_expr_id_node_open() creates a node associated with ID (_id). */
static grn_rc
grn_ts_expr_id_node_open(grn_ctx *ctx, grn_ts_expr_node **node) {
  grn_ts_expr_id_node *new_node = GRN_MALLOCN(grn_ts_expr_id_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_ts_expr_id_node_init(ctx, new_node);
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_id_node_close() destroys a node. */
static void
grn_ts_expr_id_node_close(grn_ctx *ctx, grn_ts_expr_id_node *node) {
  grn_ts_expr_id_node_fin(ctx, node);
  GRN_FREE(node);
}

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

/*-------------------------------------------------------------
 * grn_ts_expr_score_node.
 */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
} grn_ts_expr_score_node;

/* grn_ts_expr_score_node_init() initializes a node. */
static void
grn_ts_expr_score_node_init(grn_ctx *ctx, grn_ts_expr_score_node *node) {
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_SCORE_NODE;
  node->data_kind = GRN_TS_FLOAT;
  node->data_type = GRN_DB_FLOAT;
}

/* grn_ts_expr_score_node_fin() finalizes a node. */
static void
grn_ts_expr_score_node_fin(grn_ctx *ctx, grn_ts_expr_score_node *node) {
  /* Nothing to do. */
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
  grn_ts_expr_score_node_init(ctx, new_node);
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_score_node_close() destroys a node. */
static void
grn_ts_expr_score_node_close(grn_ctx *ctx, grn_ts_expr_score_node *node) {
  grn_ts_expr_score_node_fin(ctx, node);
  GRN_FREE(node);
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

/* grn_ts_expr_score_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_score_node_adjust(grn_ctx *ctx, grn_ts_expr_score_node *node,
                              grn_ts_record *io, size_t n_io) {
  /* Nothing to do. */
  return GRN_SUCCESS;
}

/*-------------------------------------------------------------
 * grn_ts_expr_key_node.
 */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_obj *table;
  grn_ts_buf buf;
} grn_ts_expr_key_node;

/* grn_ts_expr_key_node_init() initializes a node. */
static void
grn_ts_expr_key_node_init(grn_ctx *ctx, grn_ts_expr_key_node *node) {
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_KEY_NODE;
  node->table = NULL;
  grn_ts_buf_init(ctx, &node->buf);
}

/* grn_ts_expr_key_node_fin() finalizes a node. */
static void
grn_ts_expr_key_node_fin(grn_ctx *ctx, grn_ts_expr_key_node *node) {
  grn_ts_buf_fin(ctx, &node->buf);
  if (node->table) {
    grn_obj_unlink(ctx, node->table);
  }
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
  grn_ts_expr_key_node_init(ctx, new_node);
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    grn_ts_expr_key_node_fin(ctx, new_node);
    GRN_FREE(new_node);
    return rc;
  }
  new_node->data_kind = grn_ts_data_type_to_kind(table->header.domain);
  new_node->data_type = table->header.domain;
  new_node->table = table;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_key_node_close() destroys a node. */
static void
grn_ts_expr_key_node_close(grn_ctx *ctx, grn_ts_expr_key_node *node) {
  grn_ts_expr_key_node_fin(ctx, node);
  GRN_FREE(node);
}

#define GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE(table, KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_ts_ ## kind *out_ptr = (grn_ts_ ## kind *)out;\
    for (i = 0; i < n_in; i++) {\
      rc = grn_ts_ ## table ## _get_ ## kind ## _key(ctx, table, in[i].id,\
                                                     &out_ptr[i]);\
      if (rc != GRN_SUCCESS) {\
        return rc;\
      }\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(table, TYPE, type)\
  case GRN_DB_ ## TYPE: {\
    grn_ts_int *out_ptr = (grn_ts_int *)out;\
    for (i = 0; i < n_in; i++) {\
      rc = grn_ts_ ## table ## _get_ ## type ## _key(ctx, table, in[i].id,\
                                                     &out_ptr[i]);\
      if (rc != GRN_SUCCESS) {\
        return rc;\
      }\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_KEY_NODE_EVALUATE_TEXT_CASE(table)\
  case GRN_TS_TEXT: {\
    char *buf_ptr;\
    grn_ts_text *out_ptr = (grn_ts_text *)out;\
    node->buf.pos = 0;\
    for (i = 0; i < n_in; i++) {\
      grn_ts_text key;\
      rc = grn_ts_ ## table ## _get_text_key(ctx, table, in[i].id, &key);\
      if (rc != GRN_SUCCESS) {\
        return rc;\
      }\
      rc = grn_ts_buf_write(ctx, &node->buf, key.ptr, key.size);\
      if (rc != GRN_SUCCESS) {\
        return rc;\
      }\
      out_ptr[i].size = key.size;\
    }\
    buf_ptr = (char *)node->buf.ptr;\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i].ptr = buf_ptr;\
      buf_ptr += out_ptr[i].size;\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_KEY_NODE_EVALUATE_REF_CASE(table)\
  case GRN_TS_REF: {\
    grn_ts_ref *out_ptr = (grn_ts_ref *)out;\
    for (i = 0; i < n_in; i++) {\
      rc = grn_ts_ ## table ## _get_ref_key(ctx, table, in[i].id,\
                                            &out_ptr[i]);\
      if (rc != GRN_SUCCESS) {\
        return rc;\
      }\
      out_ptr[i].score = in[i].score;\
    }\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_key_node_evaluate() outputs keys. */
static grn_rc
grn_ts_expr_key_node_evaluate(grn_ctx *ctx, grn_ts_expr_key_node *node,
                              const grn_ts_record *in, size_t n_in,
                              void *out) {
  size_t i;
  grn_rc rc;
  switch (node->table->header.type) {
    case GRN_TABLE_HASH_KEY: {
      grn_hash *hash = (grn_hash *)node->table;
      switch (node->data_kind) {
        GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE(hash, BOOL, bool)
        case GRN_TS_INT: {
          switch (node->data_type) {
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, INT8, int8)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, INT16, int16)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, INT32, int32)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, INT64, int64)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, UINT8, uint8)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, UINT16, uint16)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, UINT32, uint32)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(hash, UINT64, uint64)
          }
        }
        GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE(hash, FLOAT, float)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_TEXT_CASE(hash)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE(hash, GEO_POINT, geo_point)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_REF_CASE(hash)
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    case GRN_TABLE_PAT_KEY: {
      grn_pat *pat = (grn_pat *)node->table;
      switch (node->data_kind) {
        GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE(pat, BOOL, bool)
        case GRN_TS_INT: {
          switch (node->data_type) {
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, INT8, int8)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, INT16, int16)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, INT32, int32)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, INT64, int64)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, UINT8, uint8)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, UINT16, uint16)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, UINT32, uint32)
            GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE(pat, UINT64, uint64)
          }
        }
        GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE(pat, FLOAT, float)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_TEXT_CASE(pat)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE(pat, GEO_POINT, geo_point)
        GRN_TS_EXPR_KEY_NODE_EVALUATE_REF_CASE(pat)
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    case GRN_TABLE_DAT_KEY: {
      grn_dat *dat = (grn_dat *)node->table;
      switch (node->data_kind) {
        GRN_TS_EXPR_KEY_NODE_EVALUATE_TEXT_CASE(dat)
        /* GRN_TABLE_DAT_KEY supports only Text. */
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    /* GRN_TABLE_NO_KEY doesn't support _key. */
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_KEY_NODE_EVALUATE_REF_CASE
#undef GRN_TS_EXPR_KEY_NODE_EVALUATE_TEXT_CASE
#undef GRN_TS_EXPR_KEY_NODE_EVALUATE_INT_CASE
#undef GRN_TS_EXPR_KEY_NODE_EVALUATE_CASE

/* grn_ts_expr_key_node_filter() filters records. */
static grn_rc
grn_ts_expr_key_node_filter(grn_ctx *ctx, grn_ts_expr_key_node *node,
                            grn_ts_record *in, size_t n_in,
                            grn_ts_record *out, size_t *n_out) {
  size_t i, count;
  grn_ts_bool key;
  switch (node->table->header.type) {
    case GRN_TABLE_HASH_KEY: {
      grn_hash *hash = (grn_hash *)node->table;
      for (i = 0, count = 0; i < n_in; i++) {
        grn_rc rc = grn_ts_hash_get_bool_key(ctx, hash, in[i].id, &key);
        if (rc != GRN_SUCCESS) {
          return GRN_INVALID_ARGUMENT;
        }
        if (key) {
          out[count++] = in[i];
        }
      }
      *n_out = count;
      return GRN_SUCCESS;
    }
    case GRN_TABLE_PAT_KEY: {
      grn_pat *pat = (grn_pat *)node->table;
      for (i = 0, count = 0; i < n_in; i++) {
        grn_rc rc = grn_ts_pat_get_bool_key(ctx, pat, in[i].id, &key);
        if (rc != GRN_SUCCESS) {
          return GRN_INVALID_ARGUMENT;
        }
        if (key) {
          out[count++] = in[i];
        }
      }
      *n_out = count;
      return GRN_SUCCESS;
    }
    /* GRN_TABLE_DAT_KEY and GRN_TABLE_NO_KEY don't support a Bool key. */
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_expr_key_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_key_node_adjust(grn_ctx *ctx, grn_ts_expr_key_node *node,
                            grn_ts_record *io, size_t n_io) {
  size_t i;
  grn_ts_float key;
  switch (node->table->header.type) {
    case GRN_TABLE_HASH_KEY: {
      grn_hash *hash = (grn_hash *)node->table;
      for (i = 0; i < n_io; i++) {
        grn_rc rc = grn_ts_hash_get_float_key(ctx, hash, io[i].id, &key);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        io[i].score = (grn_ts_score)key;
      }
      return GRN_SUCCESS;
    }
    case GRN_TABLE_PAT_KEY: {
      grn_pat *pat = (grn_pat *)node->table;
      for (i = 0; i < n_io; i++) {
        grn_rc rc = grn_ts_pat_get_float_key(ctx, pat, io[i].id, &key);
        if (rc != GRN_SUCCESS) {
          return rc;
        }
        io[i].score = (grn_ts_score)key;
      }
      return GRN_SUCCESS;
    }
    /* GRN_TABLE_DAT_KEY and GRN_TABLE_NO_KEY don't support a Float key. */
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/*-------------------------------------------------------------
 * grn_ts_expr_value_node.
 */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_obj *table;
} grn_ts_expr_value_node;

/* grn_ts_expr_value_node_init() initializes a node. */
static void
grn_ts_expr_value_node_init(grn_ctx *ctx, grn_ts_expr_value_node *node) {
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_VALUE_NODE;
  node->table = NULL;
}

/* grn_ts_expr_value_node_fin() finalizes a node. */
static void
grn_ts_expr_value_node_fin(grn_ctx *ctx, grn_ts_expr_value_node *node) {
  if (node->table) {
    grn_obj_unlink(ctx, node->table);
  }
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
  grn_ts_expr_value_node_init(ctx, new_node);
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_node);
    return rc;
  }
  new_node->data_kind = grn_ts_data_type_to_kind(DB_OBJ(table)->range);
  new_node->data_type = DB_OBJ(table)->range;
  new_node->table = table;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_value_node_close() destroys a node. */
static void
grn_ts_expr_value_node_close(grn_ctx *ctx, grn_ts_expr_value_node *node) {
  grn_ts_expr_value_node_fin(ctx, node);
  GRN_FREE(node);
}

#define GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE(KIND, kind)\
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
#define GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(TYPE, type)\
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
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE(BOOL, bool)
    case GRN_TS_INT: {
      switch (node->data_type) {
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(INT8, int8)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(INT16, int16)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(INT32, int32)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(INT64, int64)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(UINT8, uint8)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(UINT16, uint16)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(UINT32, uint32)
        GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE(UINT64, uint64)
        default: {
          return GRN_OBJECT_CORRUPT;
        }
      }
    }
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE(FLOAT, float)
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE(TIME, time)
    GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE(GEO_POINT, geo_point)
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
#undef GRN_TS_EXPR_VALUE_NODE_EVALUATE_INT_CASE
#undef GRN_TS_EXPR_VALUE_NODE_EVALUATE_CASE

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

/*-------------------------------------------------------------
 * grn_ts_expr_const_node.
 */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_ts_any content;
  grn_ts_buf text_buf;
  grn_ts_buf vector_buf;
} grn_ts_expr_const_node;

/* grn_ts_expr_const_node_init() initializes a node. */
static void
grn_ts_expr_const_node_init(grn_ctx *ctx, grn_ts_expr_const_node *node) {
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_CONST_NODE;
  grn_ts_buf_init(ctx, &node->text_buf);
  grn_ts_buf_init(ctx, &node->vector_buf);
}

/* grn_ts_expr_const_node_fin() finalizes a node. */
static void
grn_ts_expr_const_node_fin(grn_ctx *ctx, grn_ts_expr_const_node *node) {
  grn_ts_buf_fin(ctx, &node->vector_buf);
  grn_ts_buf_fin(ctx, &node->text_buf);
}

#define GRN_TS_EXPR_CONST_NODE_SET_SCALAR_CASE(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    node->content.as_ ## kind = *(const grn_ts_ ## kind *)value;\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_const_node_set_scalar() sets a scalar value. */
static grn_rc
grn_ts_expr_const_node_set_scalar(grn_ctx *ctx, grn_ts_expr_const_node *node,
                                  const void *value) {
  switch (node->data_kind) {
    GRN_TS_EXPR_CONST_NODE_SET_SCALAR_CASE(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_SET_SCALAR_CASE(INT, int)
    GRN_TS_EXPR_CONST_NODE_SET_SCALAR_CASE(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_SET_SCALAR_CASE(TIME, time)
    case GRN_TS_TEXT: {
      grn_ts_text text_value = *(const grn_ts_text *)value;
      grn_rc rc = grn_ts_buf_write(ctx, &node->text_buf,
                                   text_value.ptr, text_value.size);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      node->content.as_text.ptr = (const char *)node->text_buf.ptr;
      node->content.as_text.size = text_value.size;
      return GRN_SUCCESS;
    }
    GRN_TS_EXPR_CONST_NODE_SET_SCALAR_CASE(GEO_POINT, geo_point)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_CONST_NODE_SET_SCALAR_CASE

#define GRN_TS_EXPR_CONST_NODE_SET_VECTOR_CASE(KIND, kind)\
  case GRN_TS_ ## KIND ## _VECTOR: {\
    grn_rc rc;\
    const grn_ts_ ## kind *buf_ptr;\
    grn_ts_ ## kind ## _vector vector;\
    vector = *(const grn_ts_ ## kind ## _vector *)value;\
    rc = grn_ts_buf_write(ctx, &node->vector_buf, vector.ptr,\
                          sizeof(grn_ts_ ## kind) * vector.size);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
    buf_ptr = (const grn_ts_ ## kind *)node->vector_buf.ptr;\
    node->content.as_ ## kind ## _vector.ptr = buf_ptr;\
    node->content.as_ ## kind ## _vector.size = vector.size;\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_const_node_set_vector() sets a vector value. */
static grn_rc
grn_ts_expr_const_node_set_vector(grn_ctx *ctx, grn_ts_expr_const_node *node,
                                  const void *value) {
  switch (node->data_kind) {
    GRN_TS_EXPR_CONST_NODE_SET_VECTOR_CASE(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_SET_VECTOR_CASE(INT, int)
    GRN_TS_EXPR_CONST_NODE_SET_VECTOR_CASE(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_SET_VECTOR_CASE(TIME, time)
    case GRN_TS_TEXT_VECTOR: {
      grn_rc rc;
      size_t i, offset, total_size;
      grn_ts_text_vector vector = *(const grn_ts_text_vector *)value;
      grn_ts_text *vector_buf;
      char *text_buf;
      rc = grn_ts_buf_resize(ctx, &node->vector_buf,
                             sizeof(grn_ts_text) * vector.size);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      vector_buf = (grn_ts_text *)node->vector_buf.ptr;
      total_size = 0;
      for (i = 0; i < vector.size; i++) {
        total_size += vector.ptr[i].size;
      }
      rc = grn_ts_buf_resize(ctx, &node->text_buf, total_size);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      text_buf = (char *)node->text_buf.ptr;
      offset = 0;
      for (i = 0; i < vector.size; i++) {
        grn_memcpy(text_buf + offset, vector.ptr[i].ptr, vector.ptr[i].size);
        vector_buf[i].ptr = text_buf + offset;
        vector_buf[i].size = vector.ptr[i].size;
        offset += vector.ptr[i].size;
      }
      node->content.as_text_vector.ptr = vector_buf;
      node->content.as_text_vector.size = vector.size;
      return GRN_SUCCESS;
    }
    GRN_TS_EXPR_CONST_NODE_SET_VECTOR_CASE(GEO_POINT, geo_point)
    default: {
      return GRN_UNKNOWN_ERROR;
    }
  }
}
#undef GRN_TS_EXPR_CONST_NODE_SET_VECTOR_CASE

/* grn_ts_expr_const_node_open() creates a node associated with a const. */
static grn_rc
grn_ts_expr_const_node_open(grn_ctx *ctx, grn_ts_data_kind kind,
                            const void *value, grn_ts_expr_node **node) {
  grn_rc rc;
  grn_ts_expr_const_node *new_node = GRN_MALLOCN(grn_ts_expr_const_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_ts_expr_const_node_init(ctx, new_node);
  new_node->data_kind = kind;
  new_node->data_type = grn_ts_data_kind_to_type(kind);
  if (kind & GRN_TS_VECTOR_FLAG) {
    rc = grn_ts_expr_const_node_set_vector(ctx, new_node, value);
  } else {
    rc = grn_ts_expr_const_node_set_scalar(ctx, new_node, value);
  }
  if (rc != GRN_SUCCESS) {
    grn_ts_expr_const_node_fin(ctx, new_node);
    GRN_FREE(new_node);
    return rc;
  }
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_const_node_close() destroys a node. */
static void
grn_ts_expr_const_node_close(grn_ctx *ctx, grn_ts_expr_const_node *node) {
  grn_ts_expr_const_node_fin(ctx, node);
  GRN_FREE(node);
}

#define GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    size_t i;\
    grn_ts_ ## kind *out_ptr = (grn_ts_ ## kind *)out;\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i] = node->content.as_ ## kind;\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE(KIND, kind)\
  GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(KIND ## _VECTOR, kind ## _vector)
/* grn_ts_expr_const_node_evaluate() outputs consts. */
static grn_rc
grn_ts_expr_const_node_evaluate(grn_ctx *ctx, grn_ts_expr_const_node *node,
                                const grn_ts_record *in, size_t n_in,
                                void *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(INT, int)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(TIME, time)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(TEXT, text)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE(GEO_POINT, geo_point)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE(BOOL, bool)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE(INT, int)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE(FLOAT, float)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE(TIME, time)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE(TEXT, text)
    GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE(GEO_POINT, geo_point)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_CONST_NODE_EVALUATE_VECTOR_CASE
#undef GRN_TS_EXPR_CONST_NODE_EVALUATE_CASE

/* grn_ts_expr_const_node_filter() filters records. */
static grn_rc
grn_ts_expr_const_node_filter(grn_ctx *ctx, grn_ts_expr_const_node *node,
                              grn_ts_record *in, size_t n_in,
                              grn_ts_record *out, size_t *n_out) {
  if (node->content.as_bool) {
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

/* grn_ts_expr_const_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_const_node_adjust(grn_ctx *ctx, grn_ts_expr_const_node *node,
                              grn_ts_record *io, size_t n_io) {
  size_t i;
  grn_ts_score score = (grn_ts_score)node->content.as_float;
  for (i = 0; i < n_io; i++) {
    io[i].score = score;
  }
  return GRN_SUCCESS;
}

/*-------------------------------------------------------------
 * grn_ts_expr_column_node.
 */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_obj *column;
  grn_ts_buf buf;
  grn_ts_buf body_buf;
} grn_ts_expr_column_node;

/* grn_ts_expr_column_node_init() initializes a node. */
static void
grn_ts_expr_column_node_init(grn_ctx *ctx, grn_ts_expr_column_node *node) {
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_COLUMN_NODE;
  node->column = NULL;
  grn_ts_buf_init(ctx, &node->buf);
  grn_ts_buf_init(ctx, &node->body_buf);
}

/* grn_ts_expr_column_node_fin() finalizes a node. */
static void
grn_ts_expr_column_node_fin(grn_ctx *ctx, grn_ts_expr_column_node *node) {
  grn_ts_buf_fin(ctx, &node->body_buf);
  grn_ts_buf_fin(ctx, &node->buf);
  if (node->column) {
    grn_obj_unlink(ctx, node->column);
  }
}

#define GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE(TYPE)\
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
  grn_ts_expr_column_node_init(ctx, new_node);
  new_node->data_kind = grn_ts_data_type_to_kind(DB_OBJ(column)->range);
  if (column->header.type == GRN_COLUMN_VAR_SIZE) {
    grn_obj_flags type = column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK;
    if (type == GRN_OBJ_COLUMN_VECTOR) {
      new_node->data_kind |= GRN_TS_VECTOR_FLAG;
    }
  }
  new_node->data_type = DB_OBJ(column)->range;
  rc = grn_ts_obj_increment_ref_count(ctx, column);
  if (rc != GRN_SUCCESS) {
    grn_ts_expr_column_node_fin(ctx, new_node);
    GRN_FREE(new_node);
    return rc;
  }
  new_node->column = column;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}
#undef GRN_TS_EXPR_COLUMN_NODE_OPEN_CASE

/* grn_ts_expr_column_node_close() destroys a node. */
static void
grn_ts_expr_column_node_close(grn_ctx *ctx, grn_ts_expr_column_node *node) {
  grn_ts_expr_column_node_fin(ctx, node);
  GRN_FREE(node);
}

#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE(KIND, kind)\
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
#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(TYPE, type)\
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
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE(BOOL, bool)
    case GRN_TS_INT: {
      switch (node->data_type) {
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(INT8, int8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(INT16, int16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(INT32, int32)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(INT64, int64)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(UINT8, uint8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(UINT16, uint16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(UINT32, uint32)
        /* The behavior is undefined if a value is greater than 2^63 - 1. */
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE(UINT64, uint64)
        default: {
          return GRN_OBJECT_CORRUPT;
        }
      }
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE(FLOAT, float)
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE(TIME, time)
    case GRN_TS_TEXT: {
      size_t i, size;
      char *buf_ptr;
      grn_ja *ja = (grn_ja *)node->column;
      grn_ts_text *out_ptr = (grn_ts_text *)out;
      /* Read column values into node->buf and save the size of each value. */
      node->buf.pos = 0;
      for (i = 0; i < n_in; i++) {
        grn_rc rc = grn_ts_ja_get_value(ctx, ja, in[i].id, &node->buf, &size);
        out_ptr[i].size = (rc == GRN_SUCCESS) ? size : 0;
      }
      buf_ptr = (char *)node->buf.ptr;
      for (i = 0; i < n_in; i++) {
        out_ptr[i].ptr = buf_ptr;
        buf_ptr += out_ptr[i].size;
      }
      return GRN_SUCCESS;
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE(GEO_POINT, geo_point)
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
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_INT_CASE
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_SCALAR_CASE

/*
 * grn_ts_expr_column_node_evaluate_text_vector() outputs text vector column
 * values.
 */
static grn_rc
grn_ts_expr_column_node_evaluate_text_vector(grn_ctx *ctx,
                                             grn_ts_expr_column_node *node,
                                             const grn_ts_record *in,
                                             size_t n_in, void *out) {
  grn_rc rc;
  char *buf_ptr;
  size_t i, j, n_bytes, n_values, total_n_bytes = 0, total_n_values = 0;
  grn_ts_text *text_ptr;
  grn_ts_text_vector *out_ptr = (grn_ts_text_vector *)out;
  /* Read encoded values into node->body_buf and get the size of each value. */
  node->body_buf.pos = 0;
  for (i = 0; i < n_in; i++) {
    char *ptr;
    rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,
                             &node->body_buf, &n_bytes);
    if (rc == GRN_SUCCESS) {
      ptr = (char *)node->body_buf.ptr + total_n_bytes;
      GRN_B_DEC(n_values, ptr);
    } else {
      n_bytes = 0;
      n_values = 0;
    }
    grn_memcpy(&out_ptr[i].ptr, &n_bytes, sizeof(n_bytes));
    out_ptr[i].size = n_values;
    total_n_bytes += n_bytes;
    total_n_values += n_values;
  }
  /* Resize node->buf. */
  n_bytes = sizeof(grn_ts_text) * total_n_values;
  rc = grn_ts_buf_reserve(ctx, &node->buf, n_bytes);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  /* Decode values and compose the result. */
  buf_ptr = (char *)node->body_buf.ptr;
  text_ptr = (grn_ts_text *)node->buf.ptr;
  for (i = 0; i < n_in; i++) {
    char *ptr = buf_ptr;
    grn_memcpy(&n_bytes, &out_ptr[i].ptr, sizeof(n_bytes));
    buf_ptr += n_bytes;
    GRN_B_DEC(n_values, ptr);
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
  grn_rc rc;
  size_t i, j, n_bytes, offset = 0;
  grn_ts_id *buf_ptr;
  grn_ts_ref *ref_ptr;
  grn_ts_ref_vector *out_ptr = (grn_ts_ref_vector *)out;
  /* Read column values into node->body_buf and get the size of each value. */
  node->body_buf.pos = 0;
  for (i = 0; i < n_in; i++) {
    size_t size;
    rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,
                             &node->body_buf, &size);
    if (rc == GRN_SUCCESS) {
      out_ptr[i].size = size / sizeof(grn_ts_id);
      offset += out_ptr[i].size;
    } else {
      out_ptr[i].size = 0;
    }
  }
  /* Resize node->buf. */
  n_bytes = sizeof(grn_ts_ref) * offset;
  rc = grn_ts_buf_reserve(ctx, &node->buf, n_bytes);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  /* Compose the result. */
  buf_ptr = (grn_ts_id *)node->body_buf.ptr;
  ref_ptr = (grn_ts_ref *)node->buf.ptr;
  for (i = 0; i < n_in; i++) {
    out_ptr[i].ptr = ref_ptr;
    for (j = 0; j < out_ptr[i].size; j++, buf_ptr++, ref_ptr++) {
      ref_ptr->id = *buf_ptr;
      ref_ptr->score = in[i].score;
    }
  }
  return GRN_SUCCESS;
}

#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE(KIND, kind)\
  case GRN_TS_ ## KIND ## _VECTOR: {\
    size_t i;\
    grn_ts_ ## kind *buf_ptr;\
    grn_ts_ ## kind ## _vector *out_ptr = (grn_ts_ ## kind ## _vector *)out;\
    /* Read column values into node->buf and save the size of each value. */\
    node->buf.pos = 0;\
    for (i = 0; i < n_in; i++) {\
      size_t n_bytes;\
      grn_rc rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,\
                                      &node->buf, &n_bytes);\
      if (rc == GRN_SUCCESS) {\
        out_ptr[i].size = n_bytes / sizeof(grn_ts_ ## kind);\
      } else {\
        out_ptr[i].size = 0;\
      }\
    }\
    buf_ptr = (grn_ts_ ## kind *)node->buf.ptr;\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i].ptr = buf_ptr;\
      buf_ptr += out_ptr[i].size;\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(TYPE, type)\
  case GRN_DB_ ## TYPE: {\
    size_t i, j;\
    grn_ts_int *buf_ptr;\
    grn_ts_int_vector *out_ptr = (grn_ts_int_vector *)out;\
    /*
     * Read column values into body_buf and typecast the values to grn_ts_int.
     * Then, store the grn_ts_int values into node->buf and save the size of
     * each value.
     */\
    node->buf.pos = 0;\
    for (i = 0; i < n_in; i++) {\
      grn_rc rc;\
      size_t n_bytes, new_n_bytes;\
      node->body_buf.pos = 0;\
      rc = grn_ts_ja_get_value(ctx, (grn_ja *)node->column, in[i].id,\
                               &node->body_buf, &n_bytes);\
      if (rc == GRN_SUCCESS) {\
        out_ptr[i].size = n_bytes / sizeof(type ## _t);\
      } else {\
        out_ptr[i].size = 0;\
      }\
      new_n_bytes = node->buf.pos + (sizeof(grn_ts_int) * out_ptr[i].size);\
      rc = grn_ts_buf_reserve(ctx, &node->buf, new_n_bytes);\
      if (rc == GRN_SUCCESS) {\
        type ## _t *src_ptr = (type ## _t *)node->body_buf.ptr;\
        grn_ts_int *dest_ptr;\
        dest_ptr = (grn_ts_int *)((char *)node->buf.ptr + node->buf.pos);\
        for (j = 0; j < out_ptr[i].size; j++) {\
          dest_ptr[j] = (grn_ts_int)src_ptr[j];\
        }\
        node->buf.pos = new_n_bytes;\
      } else {\
        out_ptr[i].size = 0;\
      }\
    }\
    buf_ptr = (grn_ts_int *)node->buf.ptr;\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i].ptr = buf_ptr;\
      buf_ptr += out_ptr[i].size;\
    }\
    return GRN_SUCCESS;\
  }
/* grn_ts_expr_column_node_evaluate_vector() outputs vector column values. */
static grn_rc
grn_ts_expr_column_node_evaluate_vector(grn_ctx *ctx,
                                        grn_ts_expr_column_node *node,
                                        const grn_ts_record *in, size_t n_in,
                                        void *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE(BOOL, bool)
    case GRN_TS_INT_VECTOR: {
      switch (node->data_type) {
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(INT8, int8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(INT16, int16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(INT32, int32)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(INT64, int64)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(UINT8, uint8)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(UINT16, uint16)
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(UINT32, uint32)
        /* The behavior is undefined if a value is greater than 2^63 - 1. */
        GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE(UINT64, uint64)
        default: {
          return GRN_OBJECT_CORRUPT;
        }
      }
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE(FLOAT, float)
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE(TIME, time)
    case GRN_TS_TEXT_VECTOR: {
      return grn_ts_expr_column_node_evaluate_text_vector(ctx, node, in, n_in,
                                                          out);
    }
    GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE(GEO_POINT, geo_point)
    case GRN_TS_REF_VECTOR: {
      return grn_ts_expr_column_node_evaluate_ref_vector(ctx, node, in, n_in,
                                                         out);
    }
    default: {
      return GRN_OBJECT_CORRUPT;
    }
  }
}
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_INT_CASE
#undef GRN_TS_EXPR_COLUMN_NODE_EVALUATE_VECTOR_CASE

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

/*-------------------------------------------------------------
 * grn_ts_expr_op_node.
 */

enum {
  GRN_TS_EXPR_OP_NODE_MAX_N_ARGS = 3,
  GRN_TS_EXPR_OP_NODE_N_BUFS = 3
};

/* Forward declarations. */
static grn_rc grn_ts_expr_node_evaluate(grn_ctx *ctx, grn_ts_expr_node *node,
                                        const grn_ts_record *in, size_t n_in,
                                        void *out);
static grn_rc grn_ts_expr_node_evaluate_to_buf(grn_ctx *ctx,
                                               grn_ts_expr_node *node,
                                               const grn_ts_record *in,
                                               size_t n_in,
                                               grn_ts_buf *out);
static grn_rc grn_ts_expr_node_filter(grn_ctx *ctx, grn_ts_expr_node *node,
                                      grn_ts_record *in, size_t n_in,
                                      grn_ts_record *out, size_t *n_out);
static grn_rc grn_ts_expr_node_adjust(grn_ctx *ctx, grn_ts_expr_node *node,
                                      grn_ts_record *io, size_t n_io);

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_ts_op_type op_type;
  grn_ts_expr_node *args[GRN_TS_EXPR_OP_NODE_MAX_N_ARGS];
  size_t n_args;
  grn_ts_buf bufs[GRN_TS_EXPR_OP_NODE_N_BUFS];
} grn_ts_expr_op_node;

/* grn_ts_expr_op_node_init() initializes a node. */
static void
grn_ts_expr_op_node_init(grn_ctx *ctx, grn_ts_expr_op_node *node) {
  size_t i;
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_OP_NODE;
  for (i = 0; i < GRN_TS_EXPR_OP_NODE_MAX_N_ARGS; i++) {
    node->args[i] = NULL;
  }
  for (i = 0; i < GRN_TS_EXPR_OP_NODE_N_BUFS; i++) {
    grn_ts_buf_init(ctx, &node->bufs[i]);
  }
}

/* grn_ts_expr_op_node_fin() finalizes a node. */
static void
grn_ts_expr_op_node_fin(grn_ctx *ctx, grn_ts_expr_op_node *node) {
  size_t i;
  for (i = 0; i < GRN_TS_EXPR_OP_NODE_N_BUFS; i++) {
    grn_ts_buf_fin(ctx, &node->bufs[i]);
  }
}

/* grn_ts_op_plus_check_args() checks arguments. */
static grn_rc
grn_ts_op_plus_check_args(grn_ctx *ctx, grn_ts_expr_op_node *node) {
  switch (node->args[0]->data_kind) {
    case GRN_TS_INT: {
      switch (node->args[1]->data_kind) {
        case GRN_TS_INT: {
          /* Int + Int = Int. */
          node->data_kind = GRN_TS_INT;
          node->data_type = GRN_DB_INT64;
          return GRN_SUCCESS;
        }
        case GRN_TS_TIME: {
          /* Int + Time = Time + Int = Time. */
          grn_ts_expr_node *tmp = node->args[0];
          node->args[0] = node->args[1];
          node->args[1] = tmp;
          node->data_kind = GRN_TS_TIME;
          node->data_type = GRN_DB_TIME;
          return GRN_SUCCESS;
        }
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    case GRN_TS_FLOAT: {
      switch (node->args[1]->data_kind) {
        case GRN_TS_FLOAT: {
          /* Float + Float = Float. */
          node->data_kind = GRN_TS_FLOAT;
          node->data_type = GRN_DB_FLOAT;
          return GRN_SUCCESS;
        }
        case GRN_TS_TIME: {
          /* Float + Time = Time + Float = Time. */
          grn_ts_expr_node *tmp = node->args[0];
          node->args[0] = node->args[1];
          node->args[1] = tmp;
          node->data_kind = GRN_TS_TIME;
          node->data_type = GRN_DB_TIME;
          return GRN_SUCCESS;
        }
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    case GRN_TS_TIME: {
      switch (node->args[1]->data_kind) {
        case GRN_TS_INT:
        case GRN_TS_FLOAT: {
          /* Time + Int or Float = Time. */
          node->data_kind = GRN_TS_TIME;
          node->data_type = GRN_DB_TIME;
          return GRN_SUCCESS;
        }
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_op_minus_check_args() checks arguments. */
static grn_rc
grn_ts_op_minus_check_args(grn_ctx *ctx, grn_ts_expr_op_node *node) {
  switch (node->args[0]->data_kind) {
    case GRN_TS_INT: {
      if (node->args[1]->data_kind == GRN_TS_INT) {
        /* Int - Int = Int. */
        node->data_kind = GRN_TS_INT;
        node->data_type = GRN_DB_INT64;
        return GRN_SUCCESS;
      }
      return GRN_INVALID_ARGUMENT;
    }
    case GRN_TS_FLOAT: {
      if (node->args[1]->data_kind == GRN_TS_FLOAT) {
        /* Float - Float = Float. */
        node->data_kind = GRN_TS_FLOAT;
        node->data_type = GRN_DB_FLOAT;
        return GRN_SUCCESS;
      }
      return GRN_INVALID_ARGUMENT;
    }
    case GRN_TS_TIME: {
      switch (node->args[1]->data_kind) {
        case GRN_TS_INT:
        case GRN_TS_FLOAT: {
          /* Time - Int or Float = Time. */
          node->data_kind = GRN_TS_TIME;
          node->data_type = GRN_DB_TIME;
          return GRN_SUCCESS;
        }
        case GRN_TS_TIME: {
          /* Time - Time = Float. */
          node->data_kind = GRN_TS_FLOAT;
          node->data_type = GRN_DB_FLOAT;
          return GRN_SUCCESS;
        }
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_expr_op_node_check_args() checks arguments. */
static grn_rc
grn_ts_expr_op_node_check_args(grn_ctx *ctx, grn_ts_expr_op_node *node) {
  switch (node->op_type) {
    case GRN_TS_OP_LOGICAL_AND:
    case GRN_TS_OP_LOGICAL_OR: {
      if (node->args[1]->data_kind != GRN_TS_BOOL) {
        return GRN_INVALID_ARGUMENT;
      }
      /* Fall through. */
    }
    case GRN_TS_OP_LOGICAL_NOT: {
      if (node->args[0]->data_kind != GRN_TS_BOOL) {
        return GRN_INVALID_ARGUMENT;
      }
      node->data_kind = GRN_TS_BOOL;
      node->data_type = GRN_DB_BOOL;
      return GRN_SUCCESS;
    }
    case GRN_TS_OP_EQUAL:
    case GRN_TS_OP_NOT_EQUAL: {
      grn_ts_data_kind scalar_data_kind;
      if (node->args[0]->data_kind != node->args[1]->data_kind) {
        return GRN_INVALID_ARGUMENT;
      }
      scalar_data_kind = node->args[0]->data_kind & ~GRN_TS_VECTOR_FLAG;
      if (((scalar_data_kind == GRN_TS_REF) ||
           (scalar_data_kind == GRN_TS_GEO_POINT)) &&
          (node->args[0]->data_type != node->args[1]->data_type)) {
        return GRN_INVALID_ARGUMENT;
      }
      node->data_kind = GRN_TS_BOOL;
      node->data_type = GRN_DB_BOOL;
      return GRN_SUCCESS;
    }
    case GRN_TS_OP_LESS:
    case GRN_TS_OP_LESS_EQUAL:
    case GRN_TS_OP_GREATER:
    case GRN_TS_OP_GREATER_EQUAL: {
      if (node->args[0]->data_kind != node->args[1]->data_kind) {
        return GRN_INVALID_ARGUMENT;
      }
      switch (node->args[0]->data_kind) {
        case GRN_TS_INT:
        case GRN_TS_FLOAT:
        case GRN_TS_TIME:
        case GRN_TS_TEXT:
        case GRN_TS_INT_VECTOR:
        case GRN_TS_FLOAT_VECTOR:
        case GRN_TS_TIME_VECTOR:
        case GRN_TS_TEXT_VECTOR: {
          node->data_kind = GRN_TS_BOOL;
          node->data_type = GRN_DB_BOOL;
          return GRN_SUCCESS;
        }
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
      case GRN_TS_OP_PLUS: {
        return grn_ts_op_plus_check_args(ctx, node);
      }
      case GRN_TS_OP_MINUS: {
        return grn_ts_op_minus_check_args(ctx, node);
      }
      case GRN_TS_OP_MULTIPLICATION:
      case GRN_TS_OP_DIVISION:
      case GRN_TS_OP_MODULUS: {
        if (node->args[0]->data_kind != node->args[1]->data_kind) {
          return GRN_INVALID_ARGUMENT;
        }
        switch (node->args[0]->data_kind) {
          case GRN_TS_INT:
          case GRN_TS_FLOAT: {
            node->data_kind = node->args[0]->data_kind;
            node->data_type = grn_ts_data_kind_to_type(node->data_kind);
            return GRN_SUCCESS;
          }
          default: {
            return GRN_INVALID_ARGUMENT;
          }
        }
      }
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_expr_op_node_open() creates a node associated with an operator. */
static grn_rc
grn_ts_expr_op_node_open(grn_ctx *ctx, grn_ts_op_type op_type,
                         grn_ts_expr_node **args, size_t n_args,
                         grn_ts_expr_node **node) {
  size_t i;
  grn_rc rc;
  grn_ts_expr_op_node *new_node = GRN_MALLOCN(grn_ts_expr_op_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_ts_expr_op_node_init(ctx, new_node);
  new_node->op_type = op_type;
  for (i = 0; i < n_args; i++) {
    new_node->args[i] = args[i];
  }
  new_node->n_args = n_args;

  /* Check arguments. */
  rc = grn_ts_expr_op_node_check_args(ctx, new_node);
  if ((rc == GRN_SUCCESS) &&
      ((new_node->data_kind == GRN_TS_VOID) ||
       (new_node->data_type == GRN_DB_VOID))) {
    rc = GRN_UNKNOWN_ERROR;
  }
  if (rc != GRN_SUCCESS) {
    grn_ts_expr_op_node_fin(ctx, new_node);
    GRN_FREE(new_node);
    return rc;
  }
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_op_node_close() destroys a node. */
static void
grn_ts_expr_op_node_close(grn_ctx *ctx, grn_ts_expr_op_node *node) {
  grn_ts_expr_op_node_fin(ctx, node);
  GRN_FREE(node);
}

/* grn_ts_op_logical_not_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_logical_not_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                               const grn_ts_record *in, size_t n_in,
                               void *out) {
  size_t i;
  grn_ts_bool *out_ptr = (grn_ts_bool *)out;
  grn_rc rc = grn_ts_expr_node_evaluate(ctx, node->args[0], in, n_in, out);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  for (i = 0; i < n_in; i++) {
    out_ptr[i] = !out_ptr[i];
  }
  return GRN_SUCCESS;
}

/* grn_ts_op_logical_and_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_logical_and_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                               const grn_ts_record *in, size_t n_in,
                               void *out) {
  size_t i, j, count;
  grn_rc rc;
  grn_ts_bool *buf_ptrs[2], *out_ptr = (grn_ts_bool *)out;
  grn_ts_buf *tmp_in_buf = &node->bufs[2];
  grn_ts_record *tmp_in;

  /* Evaluate the 1st argument. */
  rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[0], in, n_in,
                                        &node->bufs[0]);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  buf_ptrs[0] = (grn_ts_bool *)node->bufs[0].ptr;

  /* Create a list of true records. */
  rc = grn_ts_buf_reserve(ctx, tmp_in_buf, sizeof(grn_ts_record) * n_in);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  tmp_in = (grn_ts_record *)tmp_in_buf->ptr;
  count = 0;
  for (i = 0; i < n_in; i++) {
    if (buf_ptrs[0][i]) {
      tmp_in[count++] = in[i];
    }
  }

  /* Evaluate the 2nd argument. */
  rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1], tmp_in, count,
                                        &node->bufs[1]);
  buf_ptrs[1] = (grn_ts_bool *)node->bufs[1].ptr;

  /* Merge the results. */
  count = 0;
  for (i = 0, j = 0; i < n_in; i++) {
    out_ptr[count++] = buf_ptrs[0][i] && buf_ptrs[1][j++];
  }
  return GRN_SUCCESS;
}

/* grn_ts_op_logical_or_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_logical_or_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                               const grn_ts_record *in, size_t n_in,
                               void *out) {
  size_t i, j, count;
  grn_rc rc;
  grn_ts_bool *buf_ptrs[2], *out_ptr = (grn_ts_bool *)out;
  grn_ts_buf *tmp_in_buf = &node->bufs[2];
  grn_ts_record *tmp_in;

  /* Evaluate the 1st argument. */
  rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[0], in, n_in,
                                        &node->bufs[0]);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  buf_ptrs[0] = (grn_ts_bool *)node->bufs[0].ptr;

  /* Create a list of false records. */
  rc = grn_ts_buf_reserve(ctx, tmp_in_buf, sizeof(grn_ts_record) * n_in);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  tmp_in = (grn_ts_record *)tmp_in_buf->ptr;
  count = 0;
  for (i = 0; i < n_in; i++) {
    if (!buf_ptrs[0][i]) {
      tmp_in[count++] = in[i];
    }
  }

  /* Evaluate the 2nd argument. */
  rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1], tmp_in, count,
                                        &node->bufs[1]);
  buf_ptrs[1] = (grn_ts_bool *)node->bufs[1].ptr;

  /* Merge the results. */
  count = 0;
  for (i = 0, j = 0; i < n_in; i++) {
    out_ptr[count++] = buf_ptrs[0][i] || buf_ptrs[1][j++];
  }
  return GRN_SUCCESS;
}

#define GRN_TS_OP_CHK_EVALUATE_CASE(type, KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_ts_ ## kind *buf_ptrs[] = {\
      (grn_ts_ ## kind *)node->bufs[0].ptr,\
      (grn_ts_ ## kind *)node->bufs[1].ptr\
    };\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i] = grn_ts_op_ ## type ## _ ## kind(buf_ptrs[0][i],\
                                                   buf_ptrs[1][i]);\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, KIND, kind)\
  GRN_TS_OP_CHK_EVALUATE_CASE(type, KIND ## _VECTOR, kind ## _vector)
#define GRN_TS_OP_CHK_EVALUATE(type)\
  size_t i;\
  grn_rc rc;\
  grn_ts_bool *out_ptr = (grn_ts_bool *)out;\
  if (node->args[0]->data_kind == GRN_TS_BOOL) {\
    /*
     * Use the output buffer to put evaluation results of the 1st argument,
     * because the data kind is same.
     */\
    rc = grn_ts_expr_node_evaluate(ctx, node->args[0], in, n_in, out);\
    if (rc == GRN_SUCCESS) {\
      grn_ts_buf *buf = &node->bufs[0];\
      rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1],\
                                            in, n_in, buf);\
      if (rc == GRN_SUCCESS) {\
        grn_ts_bool *buf_ptr = (grn_ts_bool *)buf->ptr;\
        for (i = 0; i < n_in; i++) {\
          out_ptr[i] = grn_ts_op_ ## type ## _bool(out_ptr[i], buf_ptr[i]);\
        }\
      }\
    }\
    return rc;\
  }\
  for (i = 0; i < 2; i++) {\
    rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[i], in, n_in,\
                                          &node->bufs[i]);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
  }\
  switch (node->args[0]->data_kind) {\
    GRN_TS_OP_CHK_EVALUATE_CASE(type, INT, int)\
    GRN_TS_OP_CHK_EVALUATE_CASE(type, FLOAT, float)\
    GRN_TS_OP_CHK_EVALUATE_CASE(type, TIME, time)\
    GRN_TS_OP_CHK_EVALUATE_CASE(type, TEXT, text)\
    GRN_TS_OP_CHK_EVALUATE_CASE(type, GEO_POINT, geo_point)\
    GRN_TS_OP_CHK_EVALUATE_CASE(type, REF, ref)\
    GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, BOOL, bool)\
    GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, INT, int)\
    GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, FLOAT, float)\
    GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, TIME, time)\
    GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, TEXT, text)\
    GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, GEO_POINT, geo_point)\
    GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE(type, REF, ref)\
    default: {\
      return GRN_INVALID_ARGUMENT;\
    }\
  }
/* grn_ts_op_equal_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_equal_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                         const grn_ts_record *in, size_t n_in, void *out) {
  GRN_TS_OP_CHK_EVALUATE(equal)
}

/* grn_ts_op_not_equal_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_not_equal_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                             const grn_ts_record *in, size_t n_in, void *out) {
  GRN_TS_OP_CHK_EVALUATE(not_equal)
}
#undef GRN_TS_OP_CHK_EVALUATE
#undef GRN_TS_OP_CHK_EVALUATE_VECTOR_CASE
#undef GRN_TS_OP_CHK_EVALUATE_CASE

#define GRN_TS_OP_CMP_EVALUATE_CASE(type, KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_ts_ ## kind *buf_ptrs[] = {\
      (grn_ts_ ## kind *)node->bufs[0].ptr,\
      (grn_ts_ ## kind *)node->bufs[1].ptr\
    };\
    for (i = 0; i < n_in; i++) {\
      out_ptr[i] = grn_ts_op_ ## type ## _ ## kind(buf_ptrs[0][i],\
                                                   buf_ptrs[1][i]);\
    }\
    return GRN_SUCCESS;\
  }
#define GRN_TS_OP_CMP_EVALUATE_VECTOR_CASE(type, KIND, kind)\
  GRN_TS_OP_CMP_EVALUATE_CASE(type, KIND ## _VECTOR, kind ## _vector)
#define GRN_TS_OP_CMP_EVALUATE(type)\
  size_t i;\
  grn_rc rc;\
  grn_ts_bool *out_ptr = (grn_ts_bool *)out;\
  for (i = 0; i < 2; i++) {\
    rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[i], in, n_in,\
                                          &node->bufs[i]);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
  }\
  switch (node->args[0]->data_kind) {\
    GRN_TS_OP_CMP_EVALUATE_CASE(type, INT, int)\
    GRN_TS_OP_CMP_EVALUATE_CASE(type, FLOAT, float)\
    GRN_TS_OP_CMP_EVALUATE_CASE(type, TIME, time)\
    GRN_TS_OP_CMP_EVALUATE_CASE(type, TEXT, text)\
    GRN_TS_OP_CMP_EVALUATE_VECTOR_CASE(type, INT, int)\
    GRN_TS_OP_CMP_EVALUATE_VECTOR_CASE(type, FLOAT, float)\
    GRN_TS_OP_CMP_EVALUATE_VECTOR_CASE(type, TIME, time)\
    GRN_TS_OP_CMP_EVALUATE_VECTOR_CASE(type, TEXT, text)\
    default: {\
      return GRN_INVALID_ARGUMENT;\
    }\
  }
/* grn_ts_op_less_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_less_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                        const grn_ts_record *in, size_t n_in, void *out) {
  GRN_TS_OP_CMP_EVALUATE(less)
}

/* grn_ts_op_less_equal_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_less_equal_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                              const grn_ts_record *in, size_t n_in,
                              void *out) {
  GRN_TS_OP_CMP_EVALUATE(less_equal)
}

/* grn_ts_op_greater_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_greater_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                           const grn_ts_record *in, size_t n_in, void *out) {
  GRN_TS_OP_CMP_EVALUATE(greater)
}

/* grn_ts_op_greater_equal_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_greater_equal_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                                 const grn_ts_record *in, size_t n_in,
                                 void *out) {
  GRN_TS_OP_CMP_EVALUATE(greater_equal)
}
#undef GRN_TS_OP_CMP_EVALUATE
#undef GRN_TS_OP_CMP_EVALUATE_VECTOR_CASE
#undef GRN_TS_OP_CMP_EVALUATE_CASE

#define GRN_TS_OP_ARITH_EVALUATE_CASE(type, KIND, kind)\
  case GRN_TS_ ## KIND: {\
    /*
     * Use the output buffer to put evaluation results of the 1st argument,
     * because the data kind is same.
     */\
    size_t i;\
    grn_rc rc;\
    grn_ts_ ## kind *out_ptr = (grn_ts_ ## kind *)out;\
    rc = grn_ts_expr_node_evaluate(ctx, node->args[0], in, n_in, out);\
    if (rc == GRN_SUCCESS) {\
      rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1],\
                                            in, n_in, &node->bufs[0]);\
      if (rc == GRN_SUCCESS) {\
        grn_ts_ ## kind *buf_ptr = (grn_ts_ ## kind *)node->bufs[0].ptr;\
        for (i = 0; i < n_in; i++) {\
          out_ptr[i] = grn_ts_op_ ## type ## _ ## kind(out_ptr[i],\
                                                       buf_ptr[i]);\
        }\
      }\
    }\
    return rc;\
  }
#define GRN_TS_OP_ARITH_EVALUATE_TIME_CASE(type, KIND, lhs, rhs)\
  case GRN_TS_ ## KIND: {\
    /*
     * Use the output buffer to put evaluation results of the 1st argument,
     * because the data kind is same.
     */\
    size_t i;\
    grn_rc rc;\
    grn_ts_ ## lhs *out_ptr = (grn_ts_ ## lhs *)out;\
    rc = grn_ts_expr_node_evaluate(ctx, node->args[0], in, n_in, out);\
    if (rc == GRN_SUCCESS) {\
      rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1],\
                                            in, n_in, &node->bufs[0]);\
      if (rc == GRN_SUCCESS) {\
        grn_ts_ ## rhs *buf_ptr = (grn_ts_ ## rhs *)node->bufs[0].ptr;\
        for (i = 0; i < n_in; i++) {\
          out_ptr[i] = grn_ts_op_ ## type ## _ ## lhs ## _ ## rhs(out_ptr[i],\
                                                                  buf_ptr[i]);\
        }\
      }\
    }\
    return rc;\
  }
/* grn_ts_op_plus_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_plus_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                        const grn_ts_record *in, size_t n_in, void *out) {
  switch (node->args[0]->data_kind) {
    GRN_TS_OP_ARITH_EVALUATE_CASE(plus, INT, int)
    GRN_TS_OP_ARITH_EVALUATE_CASE(plus, FLOAT, float)
    case GRN_TS_TIME: {
      switch (node->args[1]->data_kind) {
        GRN_TS_OP_ARITH_EVALUATE_TIME_CASE(plus, INT, time, int)
        GRN_TS_OP_ARITH_EVALUATE_TIME_CASE(plus, FLOAT, time, float)
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_op_minus_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_minus_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                         const grn_ts_record *in, size_t n_in, void *out) {
  switch (node->args[0]->data_kind) {
    GRN_TS_OP_ARITH_EVALUATE_CASE(minus, INT, int)
    GRN_TS_OP_ARITH_EVALUATE_CASE(minus, FLOAT, float)
    case GRN_TS_TIME: {
      switch (node->args[1]->data_kind) {
        GRN_TS_OP_ARITH_EVALUATE_TIME_CASE(minus, INT, time, int)
        GRN_TS_OP_ARITH_EVALUATE_TIME_CASE(minus, FLOAT, time, float)
        case GRN_TS_TIME: {
          size_t i;
          grn_rc rc;
          grn_ts_float *out_ptr = (grn_ts_float *)out;
          grn_ts_time *buf_ptrs[2];
          rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[0], in, n_in,
                                                &node->bufs[0]);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1], in, n_in,
                                                &node->bufs[1]);
          if (rc != GRN_SUCCESS) {
            return rc;
          }
          buf_ptrs[0] = (grn_ts_time *)node->bufs[0].ptr;
          buf_ptrs[1] = (grn_ts_time *)node->bufs[1].ptr;
          for (i = 0; i < n_in; i++) {
            out_ptr[i] = grn_ts_op_minus_time_time(buf_ptrs[0][i],
                                                   buf_ptrs[1][i]);
          }
          return GRN_SUCCESS;
        }
        default: {
          return GRN_INVALID_ARGUMENT;
        }
      }
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_OP_ARITH_EVALUATE_TIME_CASE

/* grn_ts_op_multiplication_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_multiplication_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                                  const grn_ts_record *in, size_t n_in,
                                  void *out) {
  switch (node->data_kind) {
    GRN_TS_OP_ARITH_EVALUATE_CASE(multiplication, INT, int)
    GRN_TS_OP_ARITH_EVALUATE_CASE(multiplication, FLOAT, float)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_op_division_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_division_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                            const grn_ts_record *in, size_t n_in, void *out) {
  switch (node->data_kind) {
    case GRN_TS_INT: {
      /* Specialized to detect a critical error. */
      size_t i;
      grn_rc rc;
      grn_ts_int *out_ptr = (grn_ts_int *)out;
      rc = grn_ts_expr_node_evaluate(ctx, node->args[0], in, n_in, out);
      if (rc == GRN_SUCCESS) {
        rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1],
                                              in, n_in, &node->bufs[0]);
        if (rc == GRN_SUCCESS) {
          grn_ts_int *buf_ptr = (grn_ts_int *)node->bufs[0].ptr;
          for (i = 0; i < n_in; i++) {
            if (!buf_ptr[i] ||
                ((out_ptr[i] == INT64_MIN) && (buf_ptr[i] == -1))) {
              rc = GRN_INVALID_ARGUMENT;
              break;
            }
            out_ptr[i] = grn_ts_op_division_int(out_ptr[i], buf_ptr[i]);
          }
        }
      }
      return rc;
    }
    GRN_TS_OP_ARITH_EVALUATE_CASE(division, FLOAT, float)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_op_modulus_evaluate() evaluates an operator. */
static grn_rc
grn_ts_op_modulus_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                           const grn_ts_record *in, size_t n_in, void *out) {
  switch (node->data_kind) {
    case GRN_TS_INT: {
      /* Specialized to detect a critical error. */
      size_t i;
      grn_rc rc;
      grn_ts_int *out_ptr = (grn_ts_int *)out;
      rc = grn_ts_expr_node_evaluate(ctx, node->args[0], in, n_in, out);
      if (rc == GRN_SUCCESS) {
        rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1],
                                              in, n_in, &node->bufs[0]);
        if (rc == GRN_SUCCESS) {
          grn_ts_int *buf_ptr = (grn_ts_int *)node->bufs[0].ptr;
          for (i = 0; i < n_in; i++) {
            if (!buf_ptr[i] ||
                ((out_ptr[i] == INT64_MIN) && (buf_ptr[i] == -1))) {
              rc = GRN_INVALID_ARGUMENT;
              break;
            }
            out_ptr[i] = grn_ts_op_modulus_int(out_ptr[i], buf_ptr[i]);
          }
        }
      }
      return rc;
    }
    GRN_TS_OP_ARITH_EVALUATE_CASE(modulus, FLOAT, float)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_OP_ARITH_EVALUATE_CASE

/* grn_ts_expr_op_node_evaluate() evaluates an operator. */
static grn_rc
grn_ts_expr_op_node_evaluate(grn_ctx *ctx, grn_ts_expr_op_node *node,
                             const grn_ts_record *in, size_t n_in,
                             void *out) {
  switch (node->op_type) {
    case GRN_TS_OP_LOGICAL_NOT: {
      return grn_ts_op_logical_not_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_LOGICAL_AND: {
      return grn_ts_op_logical_and_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_LOGICAL_OR: {
      return grn_ts_op_logical_or_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_EQUAL: {
      return grn_ts_op_equal_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_NOT_EQUAL: {
      return grn_ts_op_not_equal_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_LESS: {
      return grn_ts_op_less_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_LESS_EQUAL: {
      return grn_ts_op_less_equal_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_GREATER: {
      return grn_ts_op_greater_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_GREATER_EQUAL: {
      return grn_ts_op_greater_equal_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_PLUS: {
      return grn_ts_op_plus_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_MINUS: {
      return grn_ts_op_minus_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_MULTIPLICATION: {
      return grn_ts_op_multiplication_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_DIVISION: {
      return grn_ts_op_division_evaluate(ctx, node, in, n_in, out);
    }
    case GRN_TS_OP_MODULUS: {
      return grn_ts_op_modulus_evaluate(ctx, node, in, n_in, out);
    }
    // TODO: Add operators.
    default: {
      return GRN_OPERATION_NOT_SUPPORTED;
    }
  }
}

/* grn_ts_op_logical_not_filter() filters records. */
static grn_rc
grn_ts_op_logical_not_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                             grn_ts_record *in, size_t n_in,
                             grn_ts_record *out, size_t *n_out) {
  size_t i, count;
  grn_rc rc;
  grn_ts_bool *buf_ptr;
  rc = grn_ts_buf_reserve(ctx, &node->bufs[0], sizeof(grn_ts_bool) * n_in);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  buf_ptr = (grn_ts_bool *)node->bufs[0].ptr;
  rc = grn_ts_expr_node_evaluate(ctx, node->args[0], in, n_in, buf_ptr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  for (i = 0, count = 0; i < n_in; i++) {
    if (!buf_ptr[i]) {
      out[count++] = in[i];
    }
  }
  *n_out = count;
  return GRN_SUCCESS;
}

/* grn_ts_op_logical_and_filter() filters records. */
static grn_rc
grn_ts_op_logical_and_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                             grn_ts_record *in, size_t n_in,
                             grn_ts_record *out, size_t *n_out) {
  grn_rc rc = grn_ts_expr_node_filter(ctx, node->args[0], in, n_in,
                                      out, n_out);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return grn_ts_expr_node_filter(ctx, node->args[1], out, *n_out, out, n_out);
}

/* grn_ts_op_logical_or_filter() filters records. */
static grn_rc
grn_ts_op_logical_or_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                            grn_ts_record *in, size_t n_in,
                            grn_ts_record *out, size_t *n_out) {
  size_t i, j, count;
  grn_rc rc;
  grn_ts_bool *buf_ptrs[2];
  grn_ts_buf *tmp_in_buf = &node->bufs[2];
  grn_ts_record *tmp_in;

  /* Evaluate the 1st argument. */
  rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[0], in, n_in,
                                        &node->bufs[0]);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  buf_ptrs[0] = (grn_ts_bool *)node->bufs[0].ptr;

  /* Create a list of false records. */
  rc = grn_ts_buf_reserve(ctx, tmp_in_buf, sizeof(grn_ts_record) * n_in);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  tmp_in = (grn_ts_record *)tmp_in_buf->ptr;
  count = 0;
  for (i = 0; i < n_in; i++) {
    if (!buf_ptrs[0][i]) {
      tmp_in[count++] = in[i];
    }
  }

  /* Evaluate the 2nd argument. */
  rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[1], tmp_in, count,
                                        &node->bufs[1]);
  buf_ptrs[1] = (grn_ts_bool *)node->bufs[1].ptr;

  /* Merge the results. */
  count = 0;
  for (i = 0, j = 0; i < n_in; i++) {
    if (buf_ptrs[0][i] || buf_ptrs[1][j++]) {
      out[count++] = in[i];
    }
  }
  *n_out = count;
  return GRN_SUCCESS;
}

#define GRN_TS_OP_CHK_FILTER_CASE(type, KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_ts_ ## kind *buf_ptrs[] = {\
      (grn_ts_ ## kind *)node->bufs[0].ptr,\
      (grn_ts_ ## kind *)node->bufs[1].ptr\
    };\
    for (i = 0; i < n_in; i++) {\
      if (grn_ts_op_ ## type ## _ ## kind(buf_ptrs[0][i], buf_ptrs[1][i])) {\
        out[count++] = in[i];\
      }\
    }\
    *n_out = count;\
    return GRN_SUCCESS;\
  }
#define GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, KIND, kind)\
  GRN_TS_OP_CHK_FILTER_CASE(type, KIND ## _VECTOR, kind ## _vector)
#define GRN_TS_OP_CHK_FILTER(type)\
  size_t i, count = 0;\
  for (i = 0; i < 2; i++) {\
    grn_rc rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[i], in, n_in,\
                                                 &node->bufs[i]);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
  }\
  switch (node->args[0]->data_kind) {\
    GRN_TS_OP_CHK_FILTER_CASE(type, BOOL, int)\
    GRN_TS_OP_CHK_FILTER_CASE(type, INT, int)\
    GRN_TS_OP_CHK_FILTER_CASE(type, FLOAT, float)\
    GRN_TS_OP_CHK_FILTER_CASE(type, TIME, time)\
    GRN_TS_OP_CHK_FILTER_CASE(type, TEXT, text)\
    GRN_TS_OP_CHK_FILTER_CASE(type, GEO_POINT, geo_point)\
    GRN_TS_OP_CHK_FILTER_CASE(type, REF, ref)\
    GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, BOOL, bool)\
    GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, INT, int)\
    GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, FLOAT, float)\
    GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, TIME, time)\
    GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, TEXT, text)\
    GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, GEO_POINT, geo_point)\
    GRN_TS_OP_CHK_FILTER_VECTOR_CASE(type, REF, ref)\
    default: {\
      return GRN_INVALID_ARGUMENT;\
    }\
  }
/* grn_ts_op_equal_filter() filters records. */
static grn_rc
grn_ts_op_equal_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                         const grn_ts_record *in, size_t n_in,
                         grn_ts_record *out, size_t *n_out) {
  GRN_TS_OP_CHK_FILTER(equal)
}

/* grn_ts_op_not_equal_filter() filters records. */
static grn_rc
grn_ts_op_not_equal_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                           const grn_ts_record *in, size_t n_in,
                           grn_ts_record *out, size_t *n_out) {
  GRN_TS_OP_CHK_FILTER(not_equal)
}
#undef GRN_TS_OP_CHK_FILTER
#undef GRN_TS_OP_CHK_FILTER_VECTOR_CASE
#undef GRN_TS_OP_CHK_FILTER_CASE

#define GRN_TS_OP_CMP_FILTER_CASE(type, KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_ts_ ## kind *buf_ptrs[] = {\
      (grn_ts_ ## kind *)node->bufs[0].ptr,\
      (grn_ts_ ## kind *)node->bufs[1].ptr\
    };\
    for (i = 0; i < n_in; i++) {\
      if (grn_ts_op_ ## type ## _ ## kind(buf_ptrs[0][i], buf_ptrs[1][i])) {\
        out[count++] = in[i];\
      }\
    }\
    *n_out = count;\
    return GRN_SUCCESS;\
  }
#define GRN_TS_OP_CMP_FILTER_VECTOR_CASE(type, KIND, kind)\
  GRN_TS_OP_CMP_FILTER_CASE(type, KIND ## _VECTOR, kind ## _vector)
#define GRN_TS_OP_CMP_FILTER(type)\
  size_t i, count = 0;\
  for (i = 0; i < 2; i++) {\
    grn_rc rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[i], in, n_in,\
                                                 &node->bufs[i]);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
  }\
  switch (node->args[0]->data_kind) {\
    GRN_TS_OP_CMP_FILTER_CASE(type, INT, int)\
    GRN_TS_OP_CMP_FILTER_CASE(type, FLOAT, float)\
    GRN_TS_OP_CMP_FILTER_CASE(type, TIME, time)\
    GRN_TS_OP_CMP_FILTER_CASE(type, TEXT, text)\
    GRN_TS_OP_CMP_FILTER_VECTOR_CASE(type, INT, int)\
    GRN_TS_OP_CMP_FILTER_VECTOR_CASE(type, FLOAT, float)\
    GRN_TS_OP_CMP_FILTER_VECTOR_CASE(type, TIME, time)\
    GRN_TS_OP_CMP_FILTER_VECTOR_CASE(type, TEXT, text)\
    default: {\
      return GRN_INVALID_ARGUMENT;\
    }\
  }
/* grn_ts_op_less_filter() filters records. */
static grn_rc
grn_ts_op_less_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                      const grn_ts_record *in, size_t n_in,
                      grn_ts_record *out, size_t *n_out) {
  GRN_TS_OP_CMP_FILTER(less)
}

/* grn_ts_op_less_equal_filter() filters records. */
static grn_rc
grn_ts_op_less_equal_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                            const grn_ts_record *in, size_t n_in,
                            grn_ts_record *out, size_t *n_out) {
  GRN_TS_OP_CMP_FILTER(less_equal)
}

/* grn_ts_op_greater_filter() filters records. */
static grn_rc
grn_ts_op_greater_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                         const grn_ts_record *in, size_t n_in,
                         grn_ts_record *out, size_t *n_out) {
  GRN_TS_OP_CMP_FILTER(greater)
}

/* grn_ts_op_greater_equal_filter() filters records. */
static grn_rc
grn_ts_op_greater_equal_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                               const grn_ts_record *in, size_t n_in,
                               grn_ts_record *out, size_t *n_out) {
  GRN_TS_OP_CMP_FILTER(greater_equal)
}
#undef GRN_TS_OP_CMP_FILTER
#undef GRN_TS_OP_CMP_FILTER_VECTOR_CASE
#undef GRN_TS_OP_CMP_FILTER_CASE

/* grn_ts_expr_op_node_filter() filters records. */
static grn_rc
grn_ts_expr_op_node_filter(grn_ctx *ctx, grn_ts_expr_op_node *node,
                           grn_ts_record *in, size_t n_in,
                           grn_ts_record *out, size_t *n_out) {
  switch (node->op_type) {
    case GRN_TS_OP_LOGICAL_NOT: {
      return grn_ts_op_logical_not_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_LOGICAL_AND: {
      return grn_ts_op_logical_and_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_LOGICAL_OR: {
      return grn_ts_op_logical_or_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_EQUAL: {
      return grn_ts_op_equal_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_NOT_EQUAL: {
      return grn_ts_op_not_equal_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_LESS: {
      return grn_ts_op_less_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_LESS_EQUAL: {
      return grn_ts_op_less_equal_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_GREATER: {
      return grn_ts_op_greater_filter(ctx, node, in, n_in, out, n_out);
    }
    case GRN_TS_OP_GREATER_EQUAL: {
      return grn_ts_op_greater_equal_filter(ctx, node, in, n_in, out, n_out);
    }
    // TODO: Add operators.
    default: {
      return GRN_OPERATION_NOT_SUPPORTED;
    }
  }
}

#define GRN_TS_OP_ARITH_ADJUST(type)\
  size_t i, count = 0;\
  for (i = 0; i < 2; i++) {\
    grn_rc rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->args[i], io, n_io,\
                                                 &node->bufs[i]);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
  }\
  grn_ts_float *buf_ptrs[] = {\
    (grn_ts_float *)node->bufs[0].ptr,\
    (grn_ts_float *)node->bufs[1].ptr\
  };\
  for (i = 0; i < n_io; i++) {\
    grn_ts_float result = grn_ts_op_ ## type ## _float(buf_ptrs[0][i],\
                                                       buf_ptrs[1][i]);\
    io[count++].score = (grn_ts_score)result;\
  }\
  return GRN_SUCCESS;
/* grn_ts_op_plus_adjust() updates scores. */
static grn_rc
grn_ts_op_plus_adjust(grn_ctx *ctx, grn_ts_expr_op_node *node,
                      grn_ts_record *io, size_t n_io) {
  GRN_TS_OP_ARITH_ADJUST(plus)
}

/* grn_ts_op_minus_adjust() updates scores. */
static grn_rc
grn_ts_op_minus_adjust(grn_ctx *ctx, grn_ts_expr_op_node *node,
                       grn_ts_record *io, size_t n_io) {
  GRN_TS_OP_ARITH_ADJUST(minus)
}

/* grn_ts_op_multiplication_adjust() updates scores. */
static grn_rc
grn_ts_op_multiplication_adjust(grn_ctx *ctx, grn_ts_expr_op_node *node,
                                grn_ts_record *io, size_t n_io) {
  GRN_TS_OP_ARITH_ADJUST(multiplication)
}

/* grn_ts_op_division_adjust() updates scores. */
static grn_rc
grn_ts_op_division_adjust(grn_ctx *ctx, grn_ts_expr_op_node *node,
                          grn_ts_record *io, size_t n_io) {
  GRN_TS_OP_ARITH_ADJUST(division)
}

/* grn_ts_op_modulus_adjust() updates scores. */
static grn_rc
grn_ts_op_modulus_adjust(grn_ctx *ctx, grn_ts_expr_op_node *node,
                         grn_ts_record *io, size_t n_io) {
  GRN_TS_OP_ARITH_ADJUST(modulus)
}
#undef GRN_TS_OP_ARITH_ADJUST

/* grn_ts_expr_op_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_op_node_adjust(grn_ctx *ctx, grn_ts_expr_op_node *node,
                           grn_ts_record *io, size_t n_io) {
  switch (node->op_type) {
    case GRN_TS_OP_PLUS: {
      return grn_ts_op_plus_adjust(ctx, node, io, n_io);
    }
    case GRN_TS_OP_MINUS: {
      return grn_ts_op_minus_adjust(ctx, node, io, n_io);
    }
    case GRN_TS_OP_MULTIPLICATION: {
      return grn_ts_op_multiplication_adjust(ctx, node, io, n_io);
    }
    case GRN_TS_OP_DIVISION: {
      return grn_ts_op_division_adjust(ctx, node, io, n_io);
    }
    case GRN_TS_OP_MODULUS: {
      return grn_ts_op_modulus_adjust(ctx, node, io, n_io);
    }
    // TODO: Add operators.
    default: {
      return GRN_OPERATION_NOT_SUPPORTED;
    }
  }
}

/*-------------------------------------------------------------
 * grn_ts_expr_bridge_node.
 */

enum { GRN_TS_EXPR_BRIDGE_NODE_N_BUFS = 2 };

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
  grn_ts_expr_node *src;
  grn_ts_expr_node *dest;
  grn_ts_buf bufs[GRN_TS_EXPR_BRIDGE_NODE_N_BUFS];
} grn_ts_expr_bridge_node;

/* grn_ts_expr_bridge_node_init() initializes a node. */
static void
grn_ts_expr_bridge_node_init(grn_ctx *ctx, grn_ts_expr_bridge_node *node) {
  size_t i;
  memset(node, 0, sizeof(*node));
  node->type = GRN_TS_EXPR_BRIDGE_NODE;
  node->src = NULL;
  node->dest = NULL;
  for (i = 0; i < GRN_TS_EXPR_BRIDGE_NODE_N_BUFS; i++) {
    grn_ts_buf_init(ctx, &node->bufs[i]);
  }
}

/* grn_ts_expr_bridge_node_fin() finalizes a node. */
static void
grn_ts_expr_bridge_node_fin(grn_ctx *ctx, grn_ts_expr_bridge_node *node) {
  size_t i;
  for (i = 0; i < GRN_TS_EXPR_BRIDGE_NODE_N_BUFS; i++) {
    grn_ts_buf_fin(ctx, &node->bufs[i]);
  }
}

/* grn_ts_expr_bridge_node_open() creates a node. */
static grn_rc
grn_ts_expr_bridge_node_open(grn_ctx *ctx, grn_ts_expr_node *src,
                             grn_ts_expr_node *dest, grn_ts_expr_node **node) {
  grn_ts_expr_bridge_node *new_node = GRN_MALLOCN(grn_ts_expr_bridge_node, 1);
  if (!new_node) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_ts_expr_bridge_node_init(ctx, new_node);
  new_node->data_kind = dest->data_kind;
  new_node->data_type = dest->data_type;
  new_node->src = src;
  new_node->dest = dest;
  *node = (grn_ts_expr_node *)new_node;
  return GRN_SUCCESS;
}

/* grn_ts_expr_bridge_node_close() destroys a node. */
static void
grn_ts_expr_bridge_node_close(grn_ctx *ctx, grn_ts_expr_bridge_node *node) {
  grn_ts_expr_bridge_node_fin(ctx, node);
  GRN_FREE(node);
}

/* grn_ts_expr_bridge_node_evaluate() evaluates a bridge. */
static grn_rc
grn_ts_expr_bridge_node_evaluate(grn_ctx *ctx, grn_ts_expr_bridge_node *node,
                                 const grn_ts_record *in, size_t n_in,
                                 void *out) {
  grn_ts_record *tmp;
  grn_rc rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->src, in, n_in,
                                               &node->bufs[0]);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  tmp = (grn_ts_record *)node->bufs[0].ptr;
  return grn_ts_expr_node_evaluate(ctx, node->dest, tmp, n_in, out);
}

/* grn_ts_expr_bridge_node_filter() filters records. */
static grn_rc
grn_ts_expr_bridge_node_filter(grn_ctx *ctx, grn_ts_expr_bridge_node *node,
                               grn_ts_record *in, size_t n_in,
                               grn_ts_record *out, size_t *n_out) {
  size_t i, count;
  grn_ts_bool *values;
  grn_ts_record *tmp;
  grn_rc rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->src, in, n_in,
                                               &node->bufs[0]);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  tmp = (grn_ts_record *)node->bufs[0].ptr;
  rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->dest, in, n_in,
                                        &node->bufs[1]);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  values = (grn_ts_bool *)&node->bufs[1].ptr;
  for (i = 0, count = 0; i < n_in; i++) {
    if (values[i]) {
      out[count++] = in[i];
    }
  }
  *n_out = count;
  return GRN_SUCCESS;
}

/* grn_ts_expr_bridge_node_adjust() updates scores. */
static grn_rc
grn_ts_expr_bridge_node_adjust(grn_ctx *ctx, grn_ts_expr_bridge_node *node,
                               grn_ts_record *io, size_t n_io) {
  size_t i;
  grn_ts_record *tmp;
  grn_rc rc = grn_ts_expr_node_evaluate_to_buf(ctx, node->src, io, n_io,
                                               &node->bufs[0]);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  tmp = (grn_ts_record *)node->bufs[0].ptr;
  rc = grn_ts_expr_node_adjust(ctx, node->dest, tmp, n_io);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  for (i = 0; i < n_io; i++) {
    io[i].score = tmp[i].score;
  }
  return GRN_SUCCESS;
}

/*-------------------------------------------------------------
 * grn_ts_expr_node.
 */

#define GRN_TS_EXPR_NODE_CLOSE_CASE(TYPE, type)\
  case GRN_TS_EXPR_ ## TYPE ## _NODE: {\
    grn_ts_expr_ ## type ## _node *type ## _node;\
    type ## _node = (grn_ts_expr_ ## type ## _node *)node;\
    grn_ts_expr_ ## type ## _node_close(ctx, type ## _node);\
    return;\
  }
/* grn_ts_expr_node_close() destroys a node. */
static void
grn_ts_expr_node_close(grn_ctx *ctx, grn_ts_expr_node *node) {
  switch (node->type) {
    GRN_TS_EXPR_NODE_CLOSE_CASE(ID, id)
    GRN_TS_EXPR_NODE_CLOSE_CASE(SCORE, score)
    GRN_TS_EXPR_NODE_CLOSE_CASE(KEY, key)
    GRN_TS_EXPR_NODE_CLOSE_CASE(VALUE, value)
    GRN_TS_EXPR_NODE_CLOSE_CASE(CONST, const)
    GRN_TS_EXPR_NODE_CLOSE_CASE(COLUMN, column)
    GRN_TS_EXPR_NODE_CLOSE_CASE(OP, op)
    GRN_TS_EXPR_NODE_CLOSE_CASE(BRIDGE, bridge)
  }
}
#undef GRN_TS_EXPR_NODE_CLOSE_CASE

#define GRN_TS_EXPR_NODE_EVALUATE_CASE(TYPE, type)\
  case GRN_TS_EXPR_ ## TYPE ## _NODE: {\
    grn_ts_expr_ ## type ## _node *type ## _node;\
    type ## _node = (grn_ts_expr_ ## type ## _node *)node;\
    return grn_ts_expr_ ## type ## _node_evaluate(ctx, type ## _node,\
                                                  in, n_in, out);\
  }
/* grn_ts_expr_node_evaluate() evaluates a subexpression. */
static grn_rc
grn_ts_expr_node_evaluate(grn_ctx *ctx, grn_ts_expr_node *node,
                          const grn_ts_record *in, size_t n_in, void *out) {
  switch (node->type) {
    GRN_TS_EXPR_NODE_EVALUATE_CASE(ID, id)
    GRN_TS_EXPR_NODE_EVALUATE_CASE(SCORE, score)
    GRN_TS_EXPR_NODE_EVALUATE_CASE(KEY, key)
    GRN_TS_EXPR_NODE_EVALUATE_CASE(VALUE, value)
    GRN_TS_EXPR_NODE_EVALUATE_CASE(CONST, const)
    GRN_TS_EXPR_NODE_EVALUATE_CASE(COLUMN, column)
    GRN_TS_EXPR_NODE_EVALUATE_CASE(OP, op)
    GRN_TS_EXPR_NODE_EVALUATE_CASE(BRIDGE, bridge)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_NODE_EVALUATE_CASE

#define GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_rc rc = grn_ts_buf_reserve(ctx, out, sizeof(grn_ts_ ## kind) * n_in);\
    if (rc != GRN_SUCCESS) {\
      return rc;\
    }\
    return grn_ts_expr_node_evaluate(ctx, node, in, n_in, out->ptr);\
  }
#define GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(KIND, kind)\
  GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(KIND ## _VECTOR, kind ## _vector)
/* grn_ts_expr_node_evaluate_to_buf() evaluates a subexpression. */
static grn_rc
grn_ts_expr_node_evaluate_to_buf(grn_ctx *ctx, grn_ts_expr_node *node,
                                 const grn_ts_record *in, size_t n_in,
                                 grn_ts_buf *out) {
  switch (node->data_kind) {
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(BOOL, bool)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(INT, int)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(FLOAT, float)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(TIME, time)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(TEXT, text)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(GEO_POINT, geo_point)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE(REF, ref)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(BOOL, bool)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(INT, int)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(FLOAT, float)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(TIME, time)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(TEXT, text)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(GEO_POINT, geo_point)
    GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE(REF, ref)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_VECTOR_CASE
#undef GRN_TS_EXPR_NODE_EVALUATE_TO_BUF_CASE

#define GRN_TS_EXPR_NODE_FILTER_CASE(TYPE, type)\
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
    GRN_TS_EXPR_NODE_FILTER_CASE(KEY, key)
    GRN_TS_EXPR_NODE_FILTER_CASE(VALUE, value)
    GRN_TS_EXPR_NODE_FILTER_CASE(CONST, const)
    GRN_TS_EXPR_NODE_FILTER_CASE(COLUMN, column)
    GRN_TS_EXPR_NODE_FILTER_CASE(OP, op)
    GRN_TS_EXPR_NODE_FILTER_CASE(BRIDGE, bridge)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_NODE_FILTER_CASE

#define GRN_TS_EXPR_NODE_ADJUST_CASE(TYPE, type)\
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
    GRN_TS_EXPR_NODE_ADJUST_CASE(SCORE, score)
    GRN_TS_EXPR_NODE_ADJUST_CASE(KEY, key)
    GRN_TS_EXPR_NODE_ADJUST_CASE(VALUE, value)
    GRN_TS_EXPR_NODE_ADJUST_CASE(CONST, const)
    GRN_TS_EXPR_NODE_ADJUST_CASE(COLUMN, column)
    GRN_TS_EXPR_NODE_ADJUST_CASE(OP, op)
    GRN_TS_EXPR_NODE_ADJUST_CASE(BRIDGE, bridge)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_NODE_ADJUST_CASE

/*-------------------------------------------------------------
 * grn_ts_expr_parser.
 */

typedef enum {
  GRN_TS_EXPR_DUMMY_TOKEN,  /* No extra data. */
  GRN_TS_EXPR_START_TOKEN,  /* No extra data. */
  GRN_TS_EXPR_END_TOKEN,    /* No extra data. */
  GRN_TS_EXPR_CONST_TOKEN,  /* +data_kind, content and buf. */
  GRN_TS_EXPR_NAME_TOKEN,   /* +name. */
  GRN_TS_EXPR_OP_TOKEN,     /* +op_type. */
  GRN_TS_EXPR_BRIDGE_TOKEN, /* No extra data. */
  GRN_TS_EXPR_BRACKET_TOKEN /* No extra data. */
} grn_ts_expr_token_type;

#define GRN_TS_EXPR_TOKEN_COMMON_MEMBERS\
  grn_ts_str src;              /* Source string. */\
  grn_ts_expr_token_type type; /* Token type. */

typedef struct {
  GRN_TS_EXPR_TOKEN_COMMON_MEMBERS
} grn_ts_expr_token;

typedef grn_ts_expr_token grn_ts_expr_dummy_token;
typedef grn_ts_expr_token grn_ts_expr_start_token;
typedef grn_ts_expr_token grn_ts_expr_end_token;

typedef struct {
  GRN_TS_EXPR_TOKEN_COMMON_MEMBERS
  grn_ts_data_kind data_kind;
  grn_ts_any content;
  grn_ts_buf buf;             /* Buffer for content.as_text. */
} grn_ts_expr_const_token;

typedef grn_ts_expr_token grn_ts_expr_name_token;

typedef struct {
  GRN_TS_EXPR_TOKEN_COMMON_MEMBERS
  grn_ts_op_type op_type;     /* Operator type. */
} grn_ts_expr_op_token;

typedef grn_ts_expr_token grn_ts_expr_bridge_token;
typedef grn_ts_expr_token grn_ts_expr_bracket_token;

typedef struct {
  grn_ts_expr *expr;                     /* Associated expression. */
  grn_ts_expr_token **tokens;            /* Tokens. */
  size_t n_tokens;                       /* Number of tokens. */
  size_t max_n_tokens;                   /* Max. number of tokens. */
  grn_ts_expr_dummy_token *dummy_tokens; /* Dummy tokens. */
  size_t n_dummy_tokens;                 /* Number of dummy tokens. */
  grn_ts_expr_token **stack;             /* Token stack. */
  size_t stack_depth;                    /* Token stack's current depth. */
} grn_ts_expr_parser;

#define GRN_TS_EXPR_TOKEN_INIT(TYPE)\
  memset(token, 0, sizeof(*token));\
  token->type = GRN_TS_EXPR_ ## TYPE ## _TOKEN;\
  token->src = src;
/* grn_ts_expr_dummy_token_init() initializes a token. */
static void
grn_ts_expr_dummy_token_init(grn_ctx *ctx, grn_ts_expr_dummy_token *token,
                             grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(DUMMY)
}

/* grn_ts_expr_start_token_init() initializes a token. */
static void
grn_ts_expr_start_token_init(grn_ctx *ctx, grn_ts_expr_start_token *token,
                             grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(START)
}

/* grn_ts_expr_end_token_init() initializes a token. */
static void
grn_ts_expr_end_token_init(grn_ctx *ctx, grn_ts_expr_end_token *token,
                           grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(END)
}

/* grn_ts_expr_const_token_init() initializes a token. */
static void
grn_ts_expr_const_token_init(grn_ctx *ctx, grn_ts_expr_const_token *token,
                             grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(CONST);
  grn_ts_buf_init(ctx, &token->buf);
}

/* grn_ts_expr_name_token_init() initializes a token. */
static void
grn_ts_expr_name_token_init(grn_ctx *ctx, grn_ts_expr_name_token *token,
                            grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(NAME);
}

/* grn_ts_expr_op_token_init() initializes a token. */
static void
grn_ts_expr_op_token_init(grn_ctx *ctx, grn_ts_expr_op_token *token,
                          grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(OP);
}

/* grn_ts_expr_bridge_token_init() initializes a token. */
static void
grn_ts_expr_bridge_token_init(grn_ctx *ctx, grn_ts_expr_bridge_token *token,
                              grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(BRIDGE)
}

/* grn_ts_expr_bracket_token_init() initializes a token. */
static void
grn_ts_expr_bracket_token_init(grn_ctx *ctx, grn_ts_expr_bracket_token *token,
                               grn_ts_str src) {
  GRN_TS_EXPR_TOKEN_INIT(BRACKET)
}
#undef GRN_TS_EXPR_TOKEN_INIT

/* grn_ts_expr_dummy_token_fin() finalizes a token. */
static void
grn_ts_expr_dummy_token_fin(grn_ctx *ctx, grn_ts_expr_dummy_token *token) {
  /* Nothing to do. */
}

/* grn_ts_expr_start_token_fin() finalizes a token. */
static void
grn_ts_expr_start_token_fin(grn_ctx *ctx, grn_ts_expr_start_token *token) {
  /* Nothing to do. */
}

/* grn_ts_expr_end_token_fin() finalizes a token. */
static void
grn_ts_expr_end_token_fin(grn_ctx *ctx, grn_ts_expr_end_token *token) {
  /* Nothing to do. */
}

/* grn_ts_expr_const_token_fin() finalizes a token. */
static void
grn_ts_expr_const_token_fin(grn_ctx *ctx, grn_ts_expr_const_token *token) {
  grn_ts_buf_fin(ctx, &token->buf);
}

/* grn_ts_expr_name_token_fin() finalizes a token. */
static void
grn_ts_expr_name_token_fin(grn_ctx *ctx, grn_ts_expr_name_token *token) {
  /* Nothing to do. */
}

/* grn_ts_expr_op_token_fin() finalizes a token. */
static void
grn_ts_expr_op_token_fin(grn_ctx *ctx, grn_ts_expr_op_token *token) {
  /* Nothing to do. */
}

/* grn_ts_expr_bridge_token_fin() finalizes a token. */
static void
grn_ts_expr_bridge_token_fin(grn_ctx *ctx, grn_ts_expr_bridge_token *token) {
  /* Nothing to do. */
}

/* grn_ts_expr_bracket_token_fin() finalizes a token. */
static void
grn_ts_expr_bracket_token_fin(grn_ctx *ctx, grn_ts_expr_bracket_token *token) {
  /* Nothing to do. */
}

#define GRN_TS_EXPR_TOKEN_OPEN(TYPE, type)\
  grn_ts_expr_ ## type ## _token *new_token;\
  new_token = GRN_MALLOCN(grn_ts_expr_ ## type ## _token, 1);\
  if (!new_token) {\
    return GRN_NO_MEMORY_AVAILABLE;\
  }\
  grn_ts_expr_ ## type ## _token_init(ctx, new_token, src);\
  *token = new_token;
/* grn_ts_expr_dummy_token_open() creates a token. */
/*
static grn_rc
grn_ts_expr_dummy_token_open(grn_ctx *ctx, grn_ts_str src,
                             grn_ts_expr_dummy_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(DUMMY, dummy)
  return GRN_SUCCESS;
}
*/

/* grn_ts_expr_start_token_open() creates a token. */
static grn_rc
grn_ts_expr_start_token_open(grn_ctx *ctx, grn_ts_str src,
                             grn_ts_expr_start_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(START, start)
  return GRN_SUCCESS;
}

/* grn_ts_expr_end_token_open() creates a token. */
static grn_rc
grn_ts_expr_end_token_open(grn_ctx *ctx, grn_ts_str src,
                           grn_ts_expr_end_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(END, end)
  return GRN_SUCCESS;
}

/* grn_ts_expr_const_token_open() creates a token. */
static grn_rc
grn_ts_expr_const_token_open(grn_ctx *ctx, grn_ts_str src,
                             grn_ts_expr_const_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(CONST, const)
  return GRN_SUCCESS;
}

/* grn_ts_expr_name_token_open() creates a token. */
static grn_rc
grn_ts_expr_name_token_open(grn_ctx *ctx, grn_ts_str src,
                            grn_ts_expr_name_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(NAME, name)
  return GRN_SUCCESS;
}

/* grn_ts_expr_op_token_open() creates a token. */
static grn_rc
grn_ts_expr_op_token_open(grn_ctx *ctx, grn_ts_str src, grn_ts_op_type op_type,
                          grn_ts_expr_op_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(OP, op)
  new_token->op_type = op_type;
  return GRN_SUCCESS;
}

/* grn_ts_expr_bridge_token_open() creates a token. */
static grn_rc
grn_ts_expr_bridge_token_open(grn_ctx *ctx, grn_ts_str src,
                              grn_ts_expr_bridge_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(BRIDGE, bridge)
  return GRN_SUCCESS;
}

/* grn_ts_expr_bracket_token_open() creates a token. */
static grn_rc
grn_ts_expr_bracket_token_open(grn_ctx *ctx, grn_ts_str src,
                               grn_ts_expr_bracket_token **token) {
  GRN_TS_EXPR_TOKEN_OPEN(BRACKET, bracket)
  return GRN_SUCCESS;
}
#undef GRN_TS_EXPR_TOKEN_OPEN

#define GRN_TS_EXPR_TOKEN_CLOSE_CASE(TYPE, type)\
  case GRN_TS_EXPR_ ## TYPE ## _TOKEN: {\
    grn_ts_expr_ ## type ## _token *type ## _token;\
    type ## _token = (grn_ts_expr_ ## type ## _token *)token;\
    grn_ts_expr_ ## type ## _token_fin(ctx, type ## _token);\
    break;\
  }
/* grn_ts_expr_token_close() destroys a token. */
static void
grn_ts_expr_token_close(grn_ctx *ctx, grn_ts_expr_token *token) {
  switch (token->type) {
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(DUMMY, dummy)
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(START, start)
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(END, end)
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(CONST, const)
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(NAME, name)
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(OP, op)
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(BRACKET, bracket)
    GRN_TS_EXPR_TOKEN_CLOSE_CASE(BRIDGE, bridge)
  }
  GRN_FREE(token);
}
#undef GRN_TS_EXPR_TOKEN_CLOSE_CASE

/* grn_ts_expr_parser_init() initializes a parser. */
static void
grn_ts_expr_parser_init(grn_ctx *ctx, grn_ts_expr *expr,
                        grn_ts_expr_parser *parser) {
  memset(parser, 0, sizeof(*parser));
  parser->expr = expr;
  parser->tokens = NULL;
  parser->dummy_tokens = NULL;
  parser->stack = NULL;
}

/* grn_ts_expr_parser_fin() finalizes a parser. */
static void
grn_ts_expr_parser_fin(grn_ctx *ctx, grn_ts_expr_parser *parser) {
  if (parser->stack) {
    GRN_FREE(parser->stack);
  }
  if (parser->dummy_tokens) {
    size_t i;
    for (i = 0; i < parser->n_dummy_tokens; i++) {
      grn_ts_expr_dummy_token_fin(ctx, &parser->dummy_tokens[i]);
    }
    GRN_FREE(parser->dummy_tokens);
  }
  if (parser->tokens) {
    size_t i;
    for (i = 0; i < parser->n_tokens; i++) {
      grn_ts_expr_token_close(ctx, parser->tokens[i]);
    }
    GRN_FREE(parser->tokens);
  }
}

/* grn_ts_expr_parser_open() creates a parser. */
static grn_rc
grn_ts_expr_parser_open(grn_ctx *ctx, grn_ts_expr *expr,
                        grn_ts_expr_parser **parser) {
  grn_ts_expr_parser *new_parser = GRN_MALLOCN(grn_ts_expr_parser, 1);
  if (!new_parser) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_ts_expr_parser_init(ctx, expr, new_parser);
  *parser = new_parser;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_close() destroys a parser. */
static void
grn_ts_expr_parser_close(grn_ctx *ctx, grn_ts_expr_parser *parser) {
  grn_ts_expr_parser_fin(ctx, parser);
  GRN_FREE(parser);
}

/* grn_ts_expr_parser_tokenize_start() creates the start token. */
static grn_rc
grn_ts_expr_parser_tokenize_start(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                  grn_ts_str str, grn_ts_expr_token **token) {
  grn_ts_str token_str = { str.ptr, 0 };
  grn_ts_expr_start_token *new_token;
  grn_rc rc = grn_ts_expr_start_token_open(ctx, token_str, &new_token);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  *token = new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_end() creates the end token. */
static grn_rc
grn_ts_expr_parser_tokenize_end(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                grn_ts_str str, grn_ts_expr_token **token) {
  grn_ts_str token_str = { str.ptr, 0 };
  grn_ts_expr_end_token *new_token;
  grn_rc rc = grn_ts_expr_end_token_open(ctx, token_str, &new_token);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  *token = new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_number() tokenizes an Int or Float literal. */
static grn_rc
grn_ts_expr_parser_tokenize_number(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                   grn_ts_str str, grn_ts_expr_token **token) {
  // TODO: Improve this not to make a copy of str.
  char *buf, *end;
  grn_rc rc;
  grn_ts_int int_value;
  grn_ts_str token_str;
  grn_ts_expr_const_token *new_token;

  buf = GRN_MALLOCN(char, str.size + 1);
  if (!buf) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  grn_memcpy(buf, str.ptr, str.size);
  buf[str.size] = '\0';

  int_value = strtol(buf, &end, 0);
  if (*end != '.') {
    token_str.ptr = str.ptr;
    token_str.size = end - buf;
    rc = grn_ts_expr_const_token_open(ctx, token_str, &new_token);
    if (rc == GRN_SUCCESS) {
      new_token->data_kind = GRN_TS_INT;
      new_token->content.as_int = int_value;
    }
  } else {
    grn_ts_float float_value = strtod(buf, &end);
    token_str.ptr = str.ptr;
    token_str.size = end - buf;
    rc = grn_ts_expr_const_token_open(ctx, token_str, &new_token);
    if (rc == GRN_SUCCESS) {
      new_token->data_kind = GRN_TS_FLOAT;
      new_token->content.as_float = float_value;
    }
  }
  GRN_FREE(buf);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  *token = (grn_ts_expr_token *)new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_text() tokenizes a Text literal. */
static grn_rc
grn_ts_expr_parser_tokenize_text(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                 grn_ts_str str, grn_ts_expr_token **token) {
  size_t i, n_escapes = 0;
  grn_rc rc;
  grn_ts_str token_str;
  grn_ts_expr_const_token *new_token;
  for (i = 1; i < str.size; i++) {
    if (str.ptr[i] == '\\') {
      i++;
      n_escapes++;
    } else if (str.ptr[i] == '"') {
      break;
    }
  }
  if (i >= str.size) {
    /* No closing double-quote. */
    return GRN_INVALID_ARGUMENT;
  }
  token_str.ptr = str.ptr;
  token_str.size = i + 1;
  rc = grn_ts_expr_const_token_open(ctx, token_str, &new_token);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  new_token->data_kind = GRN_TS_TEXT;
  if (n_escapes) {
    char *buf_ptr;
    const char *str_ptr = str.ptr + 1;
    size_t size = token_str.size - 2 - n_escapes;
    rc = grn_ts_buf_resize(ctx, &new_token->buf, size);
    if (rc != GRN_SUCCESS) {
      grn_ts_expr_token_close(ctx, (grn_ts_expr_token *)new_token);
      return rc;
    }
    buf_ptr = (char *)new_token->buf.ptr;
    for (i = 0; i < size; i++) {
      if (str_ptr[i] == '\\') {
        str_ptr++;
      }
      buf_ptr[i] = str_ptr[i];
    }
    new_token->content.as_text.ptr = buf_ptr;
    new_token->content.as_text.size = size;
  } else {
    new_token->content.as_text.ptr = token_str.ptr + 1;
    new_token->content.as_text.size = token_str.size - 2;
  }
  *token = (grn_ts_expr_token *)new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_name() tokenizes a Bool literal or a name. */
static grn_rc
grn_ts_expr_parser_tokenize_name(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                 grn_ts_str str, grn_ts_expr_token **token) {
  size_t i;
  grn_ts_str token_str;
  for (i = 1; i < str.size; i++) {
    if (!grn_ts_byte_is_name_char(str.ptr[i])) {
      break;
    }
  }
  token_str.ptr = str.ptr;
  token_str.size = i;

  if (grn_ts_str_is_bool(token_str)) {
    grn_ts_expr_const_token *new_token;
    grn_rc rc = grn_ts_expr_const_token_open(ctx, token_str, &new_token);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    new_token->data_kind = GRN_TS_BOOL;
    if (token_str.ptr[0] == 't') {
      new_token->content.as_bool = GRN_TRUE;
    } else {
      new_token->content.as_bool = GRN_FALSE;
    }
    *token = (grn_ts_expr_token *)new_token;
    return GRN_SUCCESS;
  }
  return grn_ts_expr_name_token_open(ctx, token_str, token);
}

/* grn_ts_expr_parser_tokenize_bridge() tokenizes a bridge. */
static grn_rc
grn_ts_expr_parser_tokenize_bridge(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                   grn_ts_str str, grn_ts_expr_token **token) {
  grn_ts_str token_str = { str.ptr, 1 };
  grn_ts_expr_bridge_token *new_token;
  grn_rc rc = grn_ts_expr_bridge_token_open(ctx, token_str, &new_token);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  *token = new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_bracket() tokenizes a bracket. */
static grn_rc
grn_ts_expr_parser_tokenize_bracket(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                    grn_ts_str str,
                                    grn_ts_expr_token **token) {
  grn_ts_str token_str = { str.ptr, 1 };
  grn_ts_expr_bracket_token *new_token;
  grn_rc rc = grn_ts_expr_bracket_token_open(ctx, token_str, &new_token);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  *token = new_token;
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_parsre_tokenize_sign() tokenizes an operator '+' or '-'.
 * Note that '+' and '-' have two roles each.
 * '+' is GRN_TS_OP_POSITIVE or GRN_TS_OP_PLUS.
 * '-' is GRN_TS_OP_NEGATIVE or GRN_TS_OP_MINUS.
 */
static grn_rc
grn_ts_expr_parser_tokenize_sign(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                 grn_ts_str str, grn_ts_expr_token **token) {
  size_t n_args;
  grn_rc rc;
  grn_ts_op_type op_type;
  grn_ts_str token_str = { str.ptr, 1 };
  grn_ts_expr_token *prev_token = parser->tokens[parser->n_tokens - 1];
  grn_ts_expr_op_token *new_token;
  switch (prev_token->type) {
    case GRN_TS_EXPR_START_TOKEN:
    case GRN_TS_EXPR_OP_TOKEN: {
      n_args = 1;
      break;
    }
    case GRN_TS_EXPR_CONST_TOKEN:
    case GRN_TS_EXPR_NAME_TOKEN: {
      n_args = 2;
      break;
    }
    case GRN_TS_EXPR_BRACKET_TOKEN: {
      const grn_ts_expr_bracket_token *bracket_token;
      bracket_token = (const grn_ts_expr_bracket_token *)prev_token;
      switch (bracket_token->src.ptr[0]) {
        case '(': case '[': {
          n_args = 1;
          break;
        }
        case ')': case ']': {
          n_args = 2;
          break;
        }
        default: {
          return GRN_UNKNOWN_ERROR;
        }
      }
      break;
    }
    default: {
      return GRN_INVALID_FORMAT;
    }
  }
  if (token_str.ptr[0] == '+') {
    op_type = (n_args == 1) ? GRN_TS_OP_POSITIVE : GRN_TS_OP_PLUS;
  } else {
    op_type = (n_args == 1) ? GRN_TS_OP_NEGATIVE : GRN_TS_OP_MINUS;
  }
  rc = grn_ts_expr_op_token_open(ctx, token_str, op_type, &new_token);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  *token = (grn_ts_expr_token *)new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_op() tokenizes an operator. */
static grn_rc
grn_ts_expr_parser_tokenize_op(grn_ctx *ctx, grn_ts_expr_parser *parser,
                               grn_ts_str str, grn_ts_expr_token **token) {
  grn_rc rc = GRN_SUCCESS;
  grn_ts_str token_str = str;
  grn_ts_op_type op_type;
  grn_ts_expr_op_token *new_token;
  switch (str.ptr[0]) {
    case '+': case '-': {
      return grn_ts_expr_parser_tokenize_sign(ctx, parser, str, token);
    }
#define GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE(label, TYPE, EQUAL_TYPE)\
  case label: {\
    if ((str.size >= 2) && (str.ptr[1] == '=')) {\
      token_str.size = 2;\
      op_type = GRN_TS_OP_ ## EQUAL_TYPE;\
    } else {\
      token_str.size = 1;\
      op_type = GRN_TS_OP_ ## TYPE;\
    }\
    rc = grn_ts_expr_op_token_open(ctx, token_str, op_type, &new_token);\
    break;\
  }
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('!', BITWISE_NOT, NOT_EQUAL)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('<', LESS, LESS_EQUAL)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('>', GREATER, GREATER_EQUAL)
#undef GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE
#define GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE(label, TYPE, DOUBLE_TYPE)\
  case label: {\
    if ((str.size >= 2) && (str.ptr[1] == str.ptr[0])) {\
      token_str.size = 2;\
      op_type = GRN_TS_OP_ ## DOUBLE_TYPE;\
    } else {\
      token_str.size = 1;\
      op_type = GRN_TS_OP_ ## TYPE;\
    }\
    rc = grn_ts_expr_op_token_open(ctx, token_str, op_type, &new_token);\
    break;\
  }
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('&', BITWISE_AND, LOGICAL_AND)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('|', BITWISE_OR, LOGICAL_OR)
#undef GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE
    case '=': {
      if ((str.size >= 2) && (str.ptr[1] == '=')) {
        token_str.size = 2;
        rc = grn_ts_expr_op_token_open(ctx, token_str, GRN_TS_OP_EQUAL,
                                       &new_token);
      } else {
        rc = GRN_INVALID_ARGUMENT;
      }
      break;
    }
#define GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE(label, TYPE)\
  case label: {\
    token_str.size = 1;\
    rc = grn_ts_expr_op_token_open(ctx, token_str, GRN_TS_OP_ ## TYPE,\
                                   &new_token);\
    break;\
  }
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('~', BITWISE_NOT)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('^', BITWISE_XOR)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('*', MULTIPLICATION)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('/', DIVISION)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('%', MODULUS)
#undef GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE
    default: {
      rc = GRN_INVALID_ARGUMENT;
      break;
    }
  }
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  *token = (grn_ts_expr_token *)new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_next() extracts the next token. */
static grn_rc
grn_ts_expr_parser_tokenize_next(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                 grn_ts_str str, grn_ts_expr_token **token) {
  grn_ts_str rest;
  if (!parser->n_tokens) {
    return grn_ts_expr_parser_tokenize_start(ctx, parser, str, token);
  }
  rest = grn_ts_str_trim_left(str);
  if (!rest.size) {
    return grn_ts_expr_parser_tokenize_end(ctx, parser, rest, token);
  }
  if (grn_ts_byte_is_decimal(rest.ptr[0]) ||
      ((rest.size >= 2) && grn_ts_byte_is_decimal(rest.ptr[1]))) {
    return grn_ts_expr_parser_tokenize_number(ctx, parser, rest, token);
  }
  if (rest.ptr[0] == '"') {
    return grn_ts_expr_parser_tokenize_text(ctx, parser, rest, token);
  }
  if (grn_ts_byte_is_name_char(rest.ptr[0])) {
    return grn_ts_expr_parser_tokenize_name(ctx, parser, rest, token);
  }
  switch (rest.ptr[0]) {
    case '(': case ')': case '[': case ']': {
      return grn_ts_expr_parser_tokenize_bracket(ctx, parser, rest, token);
    }
    case '.': {
      return grn_ts_expr_parser_tokenize_bridge(ctx, parser, rest, token);
    }
    default: {
      return grn_ts_expr_parser_tokenize_op(ctx, parser, rest, token);
    }
  }
}

/*
 * grn_ts_expr_parser_reserve_tokens() extends a token buffer for a new token.
 */
static grn_rc
grn_ts_expr_parser_reserve_tokens(grn_ctx *ctx, grn_ts_expr_parser *parser) {
  size_t i, n_bytes, new_max_n_tokens;
  grn_ts_expr_token **new_tokens;
  if (parser->n_tokens < parser->max_n_tokens) {
    return GRN_SUCCESS;
  }
  new_max_n_tokens = parser->n_tokens * 2;
  if (!new_max_n_tokens) {
    new_max_n_tokens = 1;
  }
  n_bytes = sizeof(grn_ts_expr_token *) * new_max_n_tokens;
  new_tokens = (grn_ts_expr_token **)GRN_REALLOC(parser->tokens, n_bytes);
  if (!new_tokens) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  for (i = parser->n_tokens; i < new_max_n_tokens; i++) {
    new_tokens[i] = NULL;
  }
  parser->tokens = new_tokens;
  parser->max_n_tokens = new_max_n_tokens;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize() tokenizes a string. */
static grn_rc
grn_ts_expr_parser_tokenize(grn_ctx *ctx, grn_ts_expr_parser *parser,
                            grn_ts_str str) {
  grn_ts_str rest = str;
  const char *end = str.ptr + str.size;
  grn_ts_expr_token *token;
  do {
    grn_rc rc = grn_ts_expr_parser_reserve_tokens(ctx, parser);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    rc = grn_ts_expr_parser_tokenize_next(ctx, parser, rest, &token);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    parser->tokens[parser->n_tokens++] = token;
    rest.ptr = token->src.ptr + token->src.size;
    rest.size = end - rest.ptr;
  } while (token->type != GRN_TS_EXPR_END_TOKEN);
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_push_const() pushes a token to an expression. */
static grn_rc
grn_ts_expr_parser_push_const(grn_ctx *ctx, grn_ts_expr_parser *parser,
                              grn_ts_expr_const_token *token) {
  switch (token->data_kind) {
    case GRN_TS_BOOL: {
      return grn_ts_expr_push_bool(ctx, parser->expr, token->content.as_bool);
    }
    case GRN_TS_INT: {
      return grn_ts_expr_push_int(ctx, parser->expr, token->content.as_int);
    }
    case GRN_TS_FLOAT: {
      return grn_ts_expr_push_float(ctx, parser->expr,
                                    token->content.as_float);
    }
    case GRN_TS_TEXT: {
      return grn_ts_expr_push_text(ctx, parser->expr, token->content.as_text);
    }
    default: {
      return GRN_OBJECT_CORRUPT;
    }
  }
}

/* grn_ts_expr_parser_push_name() pushes a token to an expression. */
static grn_rc
grn_ts_expr_parser_push_name(grn_ctx *ctx, grn_ts_expr_parser *parser,
                             grn_ts_expr_name_token *token) {
  return grn_ts_expr_push_name(ctx, parser->expr,
                               token->src.ptr, token->src.size);
}

/* grn_ts_expr_parser_push_op() pushes a token to an expression. */
static grn_rc
grn_ts_expr_parser_push_op(grn_ctx *ctx, grn_ts_expr_parser *parser,
                           grn_ts_expr_op_token *token) {
  return grn_ts_expr_push_operator(ctx, parser->expr, token->op_type);
}

/* grn_ts_expr_parser_apply() applies bridges and prior operators. */
static grn_rc
grn_ts_expr_parser_apply(grn_ctx *ctx, grn_ts_expr_parser *parser,
                         grn_ts_op_precedence precedence_threshold) {
  grn_rc rc = GRN_SUCCESS;
  grn_ts_expr_token **stack = parser->stack;
  size_t depth = parser->stack_depth;
  while (depth >= 2) {
    size_t n_args;
    grn_ts_str src;
    grn_ts_expr_dummy_token *dummy_token;
    if (stack[depth - 1]->type != GRN_TS_EXPR_DUMMY_TOKEN) {
      rc = GRN_INVALID_ARGUMENT;
      break;
    }

    /* Check the number of arguments. */
    if (stack[depth - 2]->type == GRN_TS_EXPR_BRIDGE_TOKEN) {
      n_args = 2;
      rc = grn_ts_expr_end_subexpr(ctx, parser->expr);
      if (rc != GRN_SUCCESS) {
        break;
      }
    } else if (stack[depth - 2]->type == GRN_TS_EXPR_OP_TOKEN) {
      grn_ts_expr_op_token *op_token;
      grn_ts_op_precedence precedence;
      op_token = (grn_ts_expr_op_token *)stack[depth - 2];
      precedence = grn_ts_op_get_precedence(op_token->op_type);
      if (precedence < precedence_threshold) {
        break;
      }
      rc = grn_ts_expr_parser_push_op(ctx, parser, op_token);
      if (rc != GRN_SUCCESS) {
        break;
      }
      n_args = grn_ts_op_get_n_args(op_token->op_type);
    } else {
      break;
    }

    /* Concatenate the source strings. */
    if (n_args == 1) {
      grn_ts_expr_token *arg = stack[depth - 1];
      src.ptr = stack[depth - 2]->src.ptr;
      src.size = (arg->src.ptr + arg->src.size) - src.ptr;
    } else if (n_args == 2) {
      grn_ts_expr_token *args[2] = { stack[depth - 3], stack[depth - 1] };
      src.ptr = args[0]->src.ptr;
      src.size = (args[1]->src.ptr + args[1]->src.size) - src.ptr;
    }

    /* Replace the operator and argument tokens with a dummy token. */
    dummy_token = &parser->dummy_tokens[parser->n_dummy_tokens++];
    grn_ts_expr_dummy_token_init(ctx, dummy_token, src);
    depth -= n_args + 1;
    stack[depth++] = dummy_token;
  }
  parser->stack = stack;
  parser->stack_depth = depth;
  return rc;
}

/* grn_ts_expr_parser_analyze_op() analyzes a token. */
static grn_rc
grn_ts_expr_parser_analyze_op(grn_ctx *ctx, grn_ts_expr_parser *parser,
                              grn_ts_expr_op_token *token) {
  size_t n_args = grn_ts_op_get_n_args(token->op_type);
  grn_ts_expr_token *ex_token = parser->stack[parser->stack_depth - 1];
  if (n_args == 1) {
    if (ex_token->type == GRN_TS_EXPR_DUMMY_TOKEN) {
      return GRN_INVALID_FORMAT;
    }
  } else if (n_args == 2) {
    grn_ts_op_precedence precedence = grn_ts_op_get_precedence(token->op_type);
    grn_rc rc = grn_ts_expr_parser_apply(ctx, parser, precedence);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  parser->stack[parser->stack_depth++] = (grn_ts_expr_token *)token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_analyze_bridge() analyzes a token. */
static grn_rc
grn_ts_expr_parser_analyze_bridge(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                  grn_ts_expr_bridge_token *token) {
  grn_rc rc = grn_ts_expr_begin_subexpr(ctx, parser->expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  parser->stack[parser->stack_depth++] = (grn_ts_expr_token *)token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_analyze_bracket() analyzes a token. */
static grn_rc
grn_ts_expr_parser_analyze_bracket(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                   grn_ts_expr_bracket_token *token) {
  grn_ts_expr_token *ex_token = parser->stack[parser->stack_depth - 1];
  switch (token->src.ptr[0]) {
    case '(': {
      if (ex_token->type == GRN_TS_EXPR_DUMMY_TOKEN) {
        return GRN_INVALID_FORMAT;
      }
      parser->stack[parser->stack_depth++] = (grn_ts_expr_token *)token;
      return GRN_SUCCESS;
    }
    case '[': {
      if (ex_token->type != GRN_TS_EXPR_DUMMY_TOKEN) {
        return GRN_INVALID_FORMAT;
      }
      parser->stack[parser->stack_depth++] = (grn_ts_expr_token *)token;
      return GRN_SUCCESS;
    }
    case ')': case ']': {
      grn_ts_expr_token *ex_ex_token;
      grn_rc rc = grn_ts_expr_parser_apply(ctx, parser, 0);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      if (parser->stack_depth < 2) {
        return GRN_INVALID_FORMAT;
      }
      ex_ex_token = parser->stack[parser->stack_depth - 2];
      if (ex_ex_token->type != GRN_TS_EXPR_BRACKET_TOKEN) {
        return GRN_INVALID_FORMAT;
      }
      if (token->src.ptr[0] == ')') {
        size_t depth = parser->stack_depth;
        grn_ts_str src;
        grn_ts_expr_dummy_token *dummy_token;
        if (ex_ex_token->src.ptr[0] != '(') {
          return GRN_INVALID_FORMAT;
        }
        src.ptr = ex_ex_token->src.ptr;
        src.size = (token->src.ptr + token->src.size) - src.ptr;
        dummy_token = &parser->dummy_tokens[parser->n_dummy_tokens++];
        grn_ts_expr_dummy_token_init(ctx, dummy_token, src);
        parser->stack[depth - 2] = dummy_token;
        parser->stack_depth--;
        // TODO: Apply a function.
      } else if (token->src.ptr[0] == ']') {
        size_t depth = parser->stack_depth;
        if (ex_ex_token->src.ptr[0] != '[') {
          return GRN_INVALID_FORMAT;
        }
        parser->stack[depth - 2] = parser->stack[depth - 1];
        parser->stack_depth--;
        // TODO: Push a subscript operator.
      }
      return GRN_SUCCESS;
    }
    default: {
      return GRN_INVALID_FORMAT;
    }
  }
}

/* grn_ts_expr_parser_analyze_token() analyzes a token. */
static grn_rc
grn_ts_expr_parser_analyze_token(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                 grn_ts_expr_token *token) {
  switch (token->type) {
    case GRN_TS_EXPR_START_TOKEN: {
      parser->stack[parser->stack_depth++] = token;
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_END_TOKEN: {
      return grn_ts_expr_parser_apply(ctx, parser, 0);
    }
    case GRN_TS_EXPR_CONST_TOKEN: {
      grn_ts_expr_const_token *const_token = (grn_ts_expr_const_token *)token;
      grn_ts_expr_dummy_token *dummy_token;
      grn_rc rc = grn_ts_expr_parser_push_const(ctx, parser, const_token);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      dummy_token = &parser->dummy_tokens[parser->n_dummy_tokens++];
      grn_ts_expr_dummy_token_init(ctx, dummy_token, token->src);
      parser->stack[parser->stack_depth++] = dummy_token;
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_NAME_TOKEN: {
      grn_ts_expr_name_token *name_token = (grn_ts_expr_name_token *)token;
      grn_ts_expr_dummy_token *dummy_token;
      grn_rc rc = grn_ts_expr_parser_push_name(ctx, parser, name_token);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      dummy_token = &parser->dummy_tokens[parser->n_dummy_tokens++];
      grn_ts_expr_dummy_token_init(ctx, dummy_token, token->src);
      parser->stack[parser->stack_depth++] = dummy_token;
      return GRN_SUCCESS;
    }
    case GRN_TS_EXPR_OP_TOKEN: {
      grn_ts_expr_op_token *op_token = (grn_ts_expr_op_token *)token;
      return grn_ts_expr_parser_analyze_op(ctx, parser, op_token);
    }
    case GRN_TS_EXPR_BRIDGE_TOKEN: {
      grn_ts_expr_bridge_token *bridge_token;
      bridge_token = (grn_ts_expr_bridge_token *)token;
      return grn_ts_expr_parser_analyze_bridge(ctx, parser, bridge_token);
    }
    case GRN_TS_EXPR_BRACKET_TOKEN: {
      grn_ts_expr_bracket_token *bracket_token;
      bracket_token = (grn_ts_expr_bracket_token *)token;
      return grn_ts_expr_parser_analyze_bracket(ctx, parser, bracket_token);
    }
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}

/* grn_ts_expr_parser_analyze() analyzes tokens. */
static grn_rc
grn_ts_expr_parser_analyze(grn_ctx *ctx, grn_ts_expr_parser *parser) {
  /* Reserve temporary work spaces. */
  parser->dummy_tokens = GRN_MALLOCN(grn_ts_expr_dummy_token,
                                     parser->n_tokens);
  if (!parser->dummy_tokens) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  parser->stack = GRN_MALLOCN(grn_ts_expr_token *, parser->n_tokens);
  if (!parser->stack) {
    return GRN_NO_MEMORY_AVAILABLE;
  }

  size_t i;
  for (i = 0; i < parser->n_tokens; i++) {
    grn_rc rc;
    rc = grn_ts_expr_parser_analyze_token(ctx, parser, parser->tokens[i]);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  if (parser->stack_depth != 2) {
    return GRN_INVALID_FORMAT;
  }
  return GRN_SUCCESS;
}

/*
 * grn_ts_expr_parser_parse() parses a string and pushes nodes into an
 * expression.
 */
static grn_rc
grn_ts_expr_parser_parse(grn_ctx *ctx, grn_ts_expr_parser *parser,
                         const char *str_ptr, size_t str_size) {
  grn_rc rc;
  grn_ts_str str = { str_ptr, str_size };
  rc = grn_ts_expr_parser_tokenize(ctx, parser, str);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_parser_analyze(ctx, parser);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  return GRN_SUCCESS;
}

/*-------------------------------------------------------------
 * grn_ts_expr_bridge.
 */

/* grn_ts_expr_bridge_init() initializes a bridge. */
static void
grn_ts_expr_bridge_init(grn_ctx *ctx, grn_ts_expr_bridge *bridge) {
  memset(bridge, 0, sizeof(*bridge));
  bridge->src_table = NULL;
  bridge->dest_table = NULL;
}

/* grn_ts_expr_bridge_fin() finalizes a bridge. */
static void
grn_ts_expr_bridge_fin(grn_ctx *ctx, grn_ts_expr_bridge *bridge) {
  if (bridge->dest_table) {
    grn_obj_unlink(ctx, bridge->dest_table);
  }
  /* Note: bridge->src_count does not increment a reference count. */
}

/*-------------------------------------------------------------
 * grn_ts_expr.
 */

/* grn_ts_expr_init() initializes an expression. */
static void
grn_ts_expr_init(grn_ctx *ctx, grn_ts_expr *expr) {
  memset(expr, 0, sizeof(*expr));
  expr->table = NULL;
  expr->curr_table = NULL;
  expr->root = NULL;
  expr->nodes = NULL;
  expr->stack = NULL;
  expr->bridges = NULL;
}

/* grn_ts_expr_fin() finalizes an expression. */
static void
grn_ts_expr_fin(grn_ctx *ctx, grn_ts_expr *expr) {
  size_t i;
  if (expr->bridges) {
    for (i = 0; i < expr->n_bridges; i++) {
      grn_ts_expr_bridge_fin(ctx, &expr->bridges[i]);
    }
    GRN_FREE(expr->bridges);
  }
  if (expr->stack) {
    GRN_FREE(expr->stack);
  }
  if (expr->nodes) {
    for (i = 0; i < expr->n_nodes; i++) {
      if (expr->nodes[i]) {
        grn_ts_expr_node_close(ctx, expr->nodes[i]);
      }
    }
    GRN_FREE(expr->nodes);
  }
  /* Note: expr->curr_table does not increment a reference count. */
  if (expr->table) {
    grn_obj_unlink(ctx, expr->table);
  }
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
  rc = grn_ts_obj_increment_ref_count(ctx, table);
  if (rc != GRN_SUCCESS) {
    GRN_FREE(new_expr);
    return rc;
  }
  grn_ts_expr_init(ctx, new_expr);
  new_expr->table = table;
  new_expr->curr_table = table;
  *expr = new_expr;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_parse(grn_ctx *ctx, grn_obj *table,
                  const char *str_ptr, size_t str_size, grn_ts_expr **expr) {
  grn_rc rc;
  grn_ts_expr *new_expr;
  if (!ctx || !table || !grn_ts_obj_is_table(ctx, table) ||
      (!str_ptr && str_size) || !expr) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_open(ctx, table, &new_expr);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_push(ctx, new_expr, str_ptr, str_size);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_complete(ctx, new_expr);
  }
  if (rc != GRN_SUCCESS) {
    grn_ts_expr_close(ctx, new_expr);
    return rc;
  }
  *expr = new_expr;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_close(grn_ctx *ctx, grn_ts_expr *expr) {
  if (!ctx || !expr) {
    return GRN_INVALID_ARGUMENT;
  }
  grn_ts_expr_fin(ctx, expr);
  GRN_FREE(expr);
  return GRN_SUCCESS;
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

#define GRN_TS_EXPR_OPEN_NODE(call)\
  grn_rc rc = grn_ts_expr_reserve_nodes(ctx, expr);\
  if (rc != GRN_SUCCESS) {\
    return rc;\
  }\
  rc = call;\
  if (rc != GRN_SUCCESS) {\
    return rc;\
  }\
  expr->nodes[expr->n_nodes++] = *node;\
  return GRN_SUCCESS;
/*
 * grn_ts_expr_open_id_node() opens and registers an ID node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_id_node(grn_ctx *ctx, grn_ts_expr *expr,
                         grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_id_node_open(ctx, node))
}

/*
 * grn_ts_expr_open_score_node() opens and registers a score node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_score_node(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_score_node_open(ctx, node))
}

/*
 * grn_ts_expr_open_key_node() opens and registers a key node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_key_node(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_key_node_open(ctx, expr->curr_table, node))
}

/*
 * grn_ts_expr_open_value_node() opens and registers a value node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_value_node(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_value_node_open(ctx, expr->curr_table,
                                                    node))
}

/*
 * grn_ts_expr_open_const_node() opens and registers a const node.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_const_node(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_data_kind kind, const void *value,
                            grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_const_node_open(ctx, kind, value, node))
}

/*
 * grn_ts_expr_open_column_node() opens and registers a column.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_column_node(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_obj *column, grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_column_node_open(ctx, column, node))
}

/*
 * grn_ts_expr_open_op_node() opens and registers an operator.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_op_node(grn_ctx *ctx, grn_ts_expr *expr,
                         grn_ts_op_type op_type, grn_ts_expr_node **args,
                         size_t n_args, grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_op_node_open(ctx, op_type, args, n_args,
                                                 node))
}

/*
 * grn_ts_expr_open_bridge_node() opens and registers a bridge.
 * Registered nodes will be closed in grn_ts_expr_fin().
 */
static grn_rc
grn_ts_expr_open_bridge_node(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_expr_node *src, grn_ts_expr_node *dest,
                             grn_ts_expr_node **node) {
  GRN_TS_EXPR_OPEN_NODE(grn_ts_expr_bridge_node_open(ctx, src, dest, node))
}
#undef GRN_TS_EXPR_OPEN_NODE

/* grn_ts_expr_reserve_stack() extends a stack. */
static grn_rc
grn_ts_expr_reserve_stack(grn_ctx *ctx, grn_ts_expr *expr) {
  size_t i, n_bytes, new_size;
  grn_ts_expr_node **new_stack;
  if (expr->stack_depth < expr->stack_size) {
    return GRN_SUCCESS;
  }
  new_size = expr->stack_size ? (expr->stack_size * 2) : 1;
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
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push(grn_ctx *ctx, grn_ts_expr *expr,
                 const char *str_ptr, size_t str_size) {
  grn_rc rc;
  grn_ts_expr_parser *parser;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      (!str_ptr && str_size)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_parser_open(ctx, expr, &parser);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  rc = grn_ts_expr_parser_parse(ctx, parser, str_ptr, str_size);
  grn_ts_expr_parser_close(ctx, parser);
  return rc;
}

grn_rc
grn_ts_expr_push_name(grn_ctx *ctx, grn_ts_expr *expr,
                     const char *name_ptr, size_t name_size) {
  grn_rc rc;
  grn_obj *column;
  grn_ts_str name = { name_ptr, name_size };
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      !grn_ts_str_is_name(name)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (grn_ts_str_is_id_name(name)) {
    return grn_ts_expr_push_id(ctx, expr);
  }
  if (grn_ts_str_is_score_name(name)) {
    return grn_ts_expr_push_score(ctx, expr);
  }
  if (grn_ts_str_is_key_name(name)) {
    return grn_ts_expr_push_key(ctx, expr);
  }
  if (grn_ts_str_is_value_name(name)) {
    return grn_ts_expr_push_value(ctx, expr);
  }
  /* TODO: Resolve references. */
  column = grn_obj_column(ctx, expr->curr_table, name.ptr, name.size);
  if (!column) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_push_column(ctx, expr, column);
  grn_obj_unlink(ctx, column);
  return rc;
}

#define GRN_TS_EXPR_PUSH_BULK_CASE(TYPE, kind)\
  case GRN_DB_ ## TYPE: {\
    return grn_ts_expr_push_ ## kind(ctx, expr, GRN_ ## TYPE ## _VALUE(obj));\
  }
/* grn_ts_expr_push_bulk() pushes a scalar. */
static grn_rc
grn_ts_expr_push_bulk(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj) {
  switch (obj->header.domain) {
    GRN_TS_EXPR_PUSH_BULK_CASE(BOOL, bool)
    GRN_TS_EXPR_PUSH_BULK_CASE(INT8, int)
    GRN_TS_EXPR_PUSH_BULK_CASE(INT16, int)
    GRN_TS_EXPR_PUSH_BULK_CASE(INT32, int)
    GRN_TS_EXPR_PUSH_BULK_CASE(INT64, int)
    GRN_TS_EXPR_PUSH_BULK_CASE(UINT8, int)
    GRN_TS_EXPR_PUSH_BULK_CASE(UINT16, int)
    GRN_TS_EXPR_PUSH_BULK_CASE(UINT32, int)
    /* The behavior is undefined if a value is greater than 2^63 - 1. */
    GRN_TS_EXPR_PUSH_BULK_CASE(UINT64, int)
    GRN_TS_EXPR_PUSH_BULK_CASE(FLOAT, float)
    GRN_TS_EXPR_PUSH_BULK_CASE(TIME, time)
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
#undef GRN_TS_EXPR_PUSH_BULK_CASE

#define GRN_TS_EXPR_PUSH_UVECTOR_CASE(TYPE, kind)\
  case GRN_DB_ ## TYPE: {\
    grn_ts_ ## kind ##_vector value = { (grn_ts_ ## kind *)GRN_BULK_HEAD(obj),\
                                        grn_uvector_size(ctx, obj) };\
    return grn_ts_expr_push_ ## kind ## _vector(ctx, expr, value);\
  }
#define GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST(TYPE, kind)\
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
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(BOOL, bool)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST(INT8, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST(INT16, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST(INT32, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(INT64, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST(UINT8, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST(UINT16, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST(UINT32, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(UINT64, int)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(TIME, time)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(TOKYO_GEO_POINT, tokyo_geo_point)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(WGS84_GEO_POINT, wgs84_geo_point)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST
#undef GRN_TS_EXPR_PUSH_UVECTOR_CASE

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
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) || !obj) {
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
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_open_id_node(ctx, expr, &node);
    if (rc == GRN_SUCCESS) {
      expr->stack[expr->stack_depth++] = node;
    }
  }
  return rc;
}

grn_rc
grn_ts_expr_push_score(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_open_score_node(ctx, expr, &node);
    if (rc == GRN_SUCCESS) {
      expr->stack[expr->stack_depth++] = node;
    }
  }
  return rc;
}

grn_rc
grn_ts_expr_push_key(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_open_key_node(ctx, expr, &node);
    if (rc == GRN_SUCCESS) {
      expr->stack[expr->stack_depth++] = node;
    }
  }
  return rc;
}

grn_rc
grn_ts_expr_push_value(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_open_value_node(ctx, expr, &node);
    if (rc == GRN_SUCCESS) {
      expr->stack[expr->stack_depth++] = node;
    }
  }
  return rc;
}

#define GRN_TS_EXPR_PUSH_CONST_CASE(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    return grn_ts_expr_push_ ## kind(ctx, expr,\
                                     *(const grn_ts_ ## kind *)value);\
  }
grn_rc
grn_ts_expr_push_const(grn_ctx *ctx, grn_ts_expr *expr,
                       grn_ts_data_kind kind, const void *value) {
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) || !value) {
    return GRN_INVALID_ARGUMENT;
  }
  switch (kind) {
    GRN_TS_EXPR_PUSH_CONST_CASE(BOOL, bool)
    GRN_TS_EXPR_PUSH_CONST_CASE(INT, int)
    GRN_TS_EXPR_PUSH_CONST_CASE(FLOAT, float)
    GRN_TS_EXPR_PUSH_CONST_CASE(TIME, time)
    GRN_TS_EXPR_PUSH_CONST_CASE(TEXT, text)
    GRN_TS_EXPR_PUSH_CONST_CASE(GEO_POINT, geo_point)
    GRN_TS_EXPR_PUSH_CONST_CASE(BOOL_VECTOR, bool_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(INT_VECTOR, int_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(FLOAT_VECTOR, float_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(TIME_VECTOR, time_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(TEXT_VECTOR, text_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(GEO_POINT_VECTOR, geo_point_vector)
    default: {
      return GRN_INVALID_ARGUMENT;
    }
  }
}
#undef GRN_TS_EXPR_PUSH_CONST_CASE

grn_rc
grn_ts_expr_push_column(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *column) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      !column || !grn_ts_obj_is_column(ctx, column) ||
      (DB_OBJ(expr->curr_table)->id != column->header.domain)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_open_column_node(ctx, expr, column, &node);
    if (rc == GRN_SUCCESS) {
      expr->stack[expr->stack_depth++] = node;
    }
  }
  return rc;
}

grn_rc
grn_ts_expr_push_operator(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_ts_op_type op_type) {
  grn_rc rc;
  grn_ts_expr_node **args, *node;
  size_t n_args;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    return GRN_INVALID_ARGUMENT;
  }
  n_args = grn_ts_op_get_n_args(op_type);
  if (!n_args) {
    return GRN_INVALID_ARGUMENT;
  }
  if (n_args > expr->stack_depth) {
    return GRN_INVALID_ARGUMENT;
  }
  /* Arguments are the top n_args nodes in the stack. */
  args = &expr->stack[expr->stack_depth - n_args];
  rc = grn_ts_expr_open_op_node(ctx, expr, op_type, args, n_args, &node);
  if (rc == GRN_SUCCESS) {
    expr->stack_depth -= n_args;
    expr->stack[expr->stack_depth++] = node;
  }
  return rc;
}

#define GRN_TS_EXPR_PUSH_CONST(KIND, kind)\
  grn_rc rc;\
  grn_ts_expr_node *node;\
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||\
      !grn_ts_ ## kind ## _is_valid(value)) {\
    return GRN_INVALID_ARGUMENT;\
  }\
  rc = grn_ts_expr_reserve_stack(ctx, expr);\
  if (rc == GRN_SUCCESS) {\
    rc = grn_ts_expr_open_const_node(ctx, expr, GRN_TS_ ## KIND,\
                                     &value, &node);\
    if (rc == GRN_SUCCESS) {\
      expr->stack[expr->stack_depth++] = node;\
    }\
  }
grn_rc
grn_ts_expr_push_bool(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_bool value) {
  GRN_TS_EXPR_PUSH_CONST(BOOL, bool)
  return rc;
}

grn_rc
grn_ts_expr_push_int(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_int value) {
  GRN_TS_EXPR_PUSH_CONST(INT, int)
  return rc;
}

grn_rc
grn_ts_expr_push_float(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_float value) {
  GRN_TS_EXPR_PUSH_CONST(FLOAT, float)
  return rc;
}

grn_rc
grn_ts_expr_push_time(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_time value) {
  GRN_TS_EXPR_PUSH_CONST(TIME, time)
  return rc;
}

grn_rc
grn_ts_expr_push_text(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_text value) {
  GRN_TS_EXPR_PUSH_CONST(TEXT, text)
  return rc;
}

grn_rc
grn_ts_expr_push_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                           grn_ts_geo_point value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT, geo_point)
  return rc;
}

grn_rc
grn_ts_expr_push_tokyo_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                                 grn_ts_geo_point value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT, geo_point)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return rc;
}

grn_rc
grn_ts_expr_push_wgs84_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                                 grn_ts_geo_point value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT, geo_point)
  node->data_type = GRN_DB_WGS84_GEO_POINT;
  return rc;
}

grn_rc
grn_ts_expr_push_bool_vector(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_bool_vector value) {
  GRN_TS_EXPR_PUSH_CONST(BOOL_VECTOR, bool_vector)
  return rc;
}

grn_rc
grn_ts_expr_push_int_vector(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_int_vector value) {
  GRN_TS_EXPR_PUSH_CONST(INT_VECTOR, int_vector)
  return rc;
}

grn_rc
grn_ts_expr_push_float_vector(grn_ctx *ctx, grn_ts_expr *expr,
                              grn_ts_float_vector value) {
  GRN_TS_EXPR_PUSH_CONST(FLOAT_VECTOR, float_vector)
  return rc;
}

grn_rc
grn_ts_expr_push_time_vector(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_time_vector value) {
  GRN_TS_EXPR_PUSH_CONST(TIME_VECTOR, time_vector)
  return rc;
}

grn_rc
grn_ts_expr_push_text_vector(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_text_vector value) {
  GRN_TS_EXPR_PUSH_CONST(TEXT_VECTOR, text_vector)
  return rc;
}

grn_rc
grn_ts_expr_push_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                  grn_ts_geo_point_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT_VECTOR, geo_point_vector)
  return rc;
}

grn_rc
grn_ts_expr_push_tokyo_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                        grn_ts_geo_point_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT_VECTOR, geo_point_vector)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return rc;
}

grn_rc
grn_ts_expr_push_wgs84_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                        grn_ts_geo_point_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_POINT_VECTOR, geo_point_vector)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return rc;
}
#undef GRN_TS_EXPR_PUSH_CONST

/* grn_ts_expr_reserve_bridges() extends a bridge buffer for a new bridge. */
static grn_rc
grn_ts_expr_reserve_bridges(grn_ctx *ctx, grn_ts_expr *expr) {
  size_t n_bytes, new_max_n_bridges;
  grn_ts_expr_bridge *new_bridges;
  if (expr->n_bridges < expr->max_n_bridges) {
    return GRN_SUCCESS;
  }
  new_max_n_bridges = expr->n_bridges * 2;
  if (!new_max_n_bridges) {
    new_max_n_bridges = 1;
  }
  n_bytes = sizeof(grn_ts_expr_bridge) * new_max_n_bridges;
  new_bridges = (grn_ts_expr_bridge *)GRN_REALLOC(expr->bridges, n_bytes);
  if (!new_bridges) {
    return GRN_NO_MEMORY_AVAILABLE;
  }
  expr->bridges = new_bridges;
  expr->max_n_bridges = new_max_n_bridges;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_begin_subexpr(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_obj *obj;
  grn_ts_expr_node *node;
  grn_ts_expr_bridge *bridge;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      !expr->stack_depth) {
    return GRN_INVALID_ARGUMENT;
  }

  /* Check whehter or not the latest node refers to a table. */
  node = expr->stack[expr->stack_depth - 1];
  if ((node->data_kind & ~GRN_TS_VECTOR_FLAG) != GRN_TS_REF) {
    return GRN_INVALID_ARGUMENT;
  }
  obj = grn_ctx_at(ctx, node->data_type);
  if (!obj) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!grn_ts_obj_is_table(ctx, obj)) {
    grn_obj_unlink(ctx, obj);
    return GRN_INVALID_ARGUMENT;
  }

  /* Creates a bridge to a subexpression. */
  rc = grn_ts_expr_reserve_bridges(ctx, expr);
  if (rc != GRN_SUCCESS) {
    grn_obj_unlink(ctx, obj);
    return rc;
  }
  bridge = &expr->bridges[expr->n_bridges++];
  grn_ts_expr_bridge_init(ctx, bridge);
  bridge->src_table = expr->curr_table;
  bridge->dest_table = obj;
  bridge->stack_depth = expr->stack_depth;
  expr->curr_table = bridge->dest_table;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_end_subexpr(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node **args, *node;
  grn_ts_expr_bridge *bridge;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      (expr->stack_depth < 2) || !expr->n_bridges) {
    return GRN_INVALID_ARGUMENT;
  }
  /* Check whehter or not the subexpression is complete.*/
  bridge = &expr->bridges[expr->n_bridges - 1];
  if (expr->stack_depth != (bridge->stack_depth + 1)) {
    return GRN_INVALID_ARGUMENT;
  }
  /* Creates a bridge node. */
  args = &expr->stack[expr->stack_depth - 2];
  rc = grn_ts_expr_open_bridge_node(ctx, expr, args[0], args[1], &node);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  /* Note: grn_ts_expr_reserve_stack() is not required. */
  expr->stack_depth -= 2;
  expr->stack[expr->stack_depth++] = node;
  expr->curr_table = bridge->src_table;
  grn_ts_expr_bridge_fin(ctx, bridge);
  expr->n_bridges--;
  return GRN_SUCCESS;
}

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
    case GRN_TS_EXPR_OP_NODE:
    case GRN_TS_EXPR_BRIDGE_NODE: {
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
grn_ts_select_filter(grn_ctx *ctx, grn_obj *table, grn_ts_str str,
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

  rc = grn_ts_expr_parse(ctx, table, str.ptr, str.size, &expr);
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

/*
 * grn_ts_tokenize_output_columns() gets the first token.
 * If the input is empty, this function returns GRN_END_OF_DATA.
 */
static grn_rc
grn_ts_tokenize_output_columns(grn_ctx *ctx, grn_ts_str in,
                               grn_ts_str *token, grn_ts_str *rest) {
  size_t i;
  char stack_top;
  grn_rc rc = GRN_SUCCESS;
  grn_ts_str str = in;
  grn_ts_buf stack;

  /* Find a non-empty token. */
  grn_ts_buf_init(ctx, &stack);
  for ( ; ; ) {
    str = grn_ts_str_trim_left(str);
    if (!str.size) {
      rc = GRN_END_OF_DATA;
      break;
    }
    for (i = 0; i < str.size; i++) {
      if (stack.pos) {
        if (str.ptr[i] == stack_top) {
          if (--stack.pos) {
            stack_top = ((char *)stack.ptr)[stack.pos - 1];
          }
          continue;
        }
        if (stack_top == '"') {
          /* Skip the next byte of an escape character. */
          if ((str.ptr[i] == '\\') && (i < (str.size - 1))) {
            i++;
          }
          continue;
        }
      } else if (str.ptr[i] == ',') {
        break;
      }
      switch (str.ptr[i]) {
        case '(': {
          stack_top = ')';
          rc = grn_ts_buf_write(ctx, &stack, &stack_top, 1);
          break;
        }
        case '[': {
          stack_top = ']';
          rc = grn_ts_buf_write(ctx, &stack, &stack_top, 1);
          break;
        }
        case '{': {
          stack_top = '}';
          rc = grn_ts_buf_write(ctx, &stack, &stack_top, 1);
          break;
        }
        case '"': {
          stack_top = '"';
          rc = grn_ts_buf_write(ctx, &stack, &stack_top, 1);
          break;
        }
      }
      if (rc != GRN_SUCCESS) {
        break;
      }
    }
    if (rc != GRN_SUCCESS) {
      break;
    }
    if (i) {
      /* Output the result. */
      token->ptr = str.ptr;
      token->size = i;
      if (token->size == str.size) {
        rest->ptr = str.ptr + str.size;
        rest->size = 0;
      } else {
        rest->ptr = str.ptr + token->size + 1;
        rest->size = str.size - token->size - 1;
      }
      break;
    }
    str.ptr++;
    str.size--;
  }
  grn_ts_buf_fin(ctx, &stack);
  return rc;
}

/* grn_ts_select_output_parse() parses an output_columns option. */
static grn_rc
grn_ts_select_output_parse(grn_ctx *ctx, grn_obj *table,
                           grn_ts_str str, grn_obj *name_buf) {
  grn_ts_str rest = str;
  for ( ; ; ) {
    grn_rc rc;
    grn_ts_str token;
    rc = grn_ts_tokenize_output_columns(ctx, rest, &token, &rest);
    if (rc != GRN_SUCCESS) {
      return (rc == GRN_END_OF_DATA) ? GRN_SUCCESS : rc;
    }

    /* Add column names to name_buf. */
    if ((token.ptr[token.size - 1] == '*') &&
        grn_ts_str_is_name_prefix((grn_ts_str){ token.ptr, token.size - 1 })) {
      /* Expand a wildcard. */
      grn_hash *columns = grn_hash_create(ctx, NULL, sizeof(grn_ts_id), 0,
                                          GRN_OBJ_TABLE_HASH_KEY |
                                          GRN_HASH_TINY);
      if (columns) {
        if (grn_table_columns(ctx, table, token.ptr, token.size - 1,
                              (grn_obj *)columns)) {
          grn_ts_id *key;
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
      grn_vector_add_element(ctx, name_buf, token.ptr, token.size, 0,
                             GRN_DB_TEXT);
    }
  }
  return GRN_SUCCESS;
}

#define GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(TYPE, name)\
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
    grn_text_esc(ctx, ctx->impl->outbuf, names[i].ptr, names[i].size);
    GRN_TEXT_PUT(ctx, ctx->impl->outbuf, ",\"", 2);
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
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(BOOL, "Bool")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(INT8, "Int8")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(INT16, "Int16")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(INT32, "Int32")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(INT64, "Int64")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(UINT8, "UInt8")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(UINT16, "UInt16")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(UINT32, "UInt32")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(UINT64, "UInt64")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(FLOAT, "Float")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(TIME, "Time")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(SHORT_TEXT, "ShortText")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(TEXT, "Text")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(LONG_TEXT, "LongText")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(TOKYO_GEO_POINT, "TokyoGeoPoint")
      GRN_TS_SELECT_OUTPUT_COLUMNS_CASE(WGS84_GEO_POINT, "WGS84GeoPoint")
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
#undef GRN_TS_SELECT_OUTPUT_COLUMNS_CASE

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
#define GRN_TS_SELECT_OUTPUT_MALLOC_CASE(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    bufs[i] = GRN_MALLOCN(grn_ts_ ## kind, GRN_TS_BATCH_SIZE);\
    break;\
  }
#define GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE(KIND, kind)\
  GRN_TS_SELECT_OUTPUT_MALLOC_CASE(KIND ## _VECTOR, kind ## _vector)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE(BOOL, bool)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE(INT, int)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE(FLOAT, float)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE(TIME, time)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE(TEXT, text)
      GRN_TS_SELECT_OUTPUT_MALLOC_CASE(GEO_POINT, geo_point)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE(BOOL, bool)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE(INT, int)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE(FLOAT, float)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE(TIME, time)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE(TEXT, text)
      GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE(GEO_POINT, geo_point)
#undef GRN_TS_SELECT_OUTPUT_MALLOC_VECTOR_CASE
#undef GRN_TS_SELECT_OUTPUT_MALLOC_CASE
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
#define GRN_TS_SELECT_OUTPUT_CASE(KIND, kind)\
  case GRN_TS_ ## KIND: {\
    grn_ts_ ## kind *value = (grn_ts_ ## kind *)bufs[j];\
    grn_ts_ ## kind ## _output(ctx, value[i]);\
    break;\
  }
#define GRN_TS_SELECT_OUTPUT_VECTOR_CASE(KIND, kind)\
  GRN_TS_SELECT_OUTPUT_CASE(KIND ## _VECTOR, kind ## _vector)
        switch (exprs[j]->data_kind) {
          GRN_TS_SELECT_OUTPUT_CASE(BOOL, bool);
          GRN_TS_SELECT_OUTPUT_CASE(INT, int);
          GRN_TS_SELECT_OUTPUT_CASE(FLOAT, float);
          GRN_TS_SELECT_OUTPUT_CASE(TIME, time);
          GRN_TS_SELECT_OUTPUT_CASE(TEXT, text);
          GRN_TS_SELECT_OUTPUT_CASE(GEO_POINT, geo_point);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE(BOOL, bool);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE(INT, int);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE(FLOAT, float);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE(TIME, time);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE(TEXT, text);
          GRN_TS_SELECT_OUTPUT_VECTOR_CASE(GEO_POINT, geo_point);
#undef GRN_TS_SELECT_OUTPUT_VECTOR_CASE
#undef GRN_TS_SELECT_OUTPUT_CASE
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
grn_ts_select_output(grn_ctx *ctx, grn_obj *table, grn_ts_str str,
                     const grn_ts_record *in, size_t n_in, size_t n_hits) {
  grn_rc rc;
  grn_ts_expr **exprs = NULL;
  size_t i, n_exprs = 0;
  grn_obj name_buf;
  grn_ts_text *names = NULL;

  GRN_TEXT_INIT(&name_buf, GRN_OBJ_VECTOR);
  rc = grn_ts_select_output_parse(ctx, table, str, &name_buf);
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
              const char *filter_ptr, size_t filter_size,
              const char *output_columns_ptr, size_t output_columns_size,
              size_t offset, size_t limit) {
  grn_rc rc;
  grn_ts_str filter = { filter_ptr, filter_size };
  grn_ts_str output_columns = { output_columns_ptr, output_columns_size };
  grn_ts_record *records = NULL;
  size_t n_records, n_hits;
  if (!ctx || !table || !grn_ts_obj_is_table(ctx, table) ||
      (!filter_ptr && filter_size) ||
      (!output_columns_ptr && output_columns_size)) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_select_filter(ctx, table, filter, offset, limit,
                            &records, &n_records, &n_hits);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_select_output(ctx, table, output_columns,
                              records, n_records, n_hits);
  }
  if (records) {
    GRN_FREE(records);
  }
  return rc;
}

#endif /* GRN_WITH_TS */
