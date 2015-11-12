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

#include "ts_expr.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "../grn_ctx.h"
#include "../grn_dat.h"
#include "../grn_db.h"
#include "../grn_geo.h"
#include "../grn_hash.h"
#include "../grn_pat.h"
#include "../grn_store.h"

#include "ts_log.h"
#include "ts_str.h"
#include "ts_util.h"
#include "ts_expr_parser.h"

/*-------------------------------------------------------------
 * Built-in data kinds.
 */

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
  return isfinite(value);
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

/* grn_ts_geo_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_geo_is_valid(grn_ts_geo value) {
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

/* grn_ts_geo_vector_is_valid() returns whether a value is valid or not. */
inline static grn_ts_bool
grn_ts_geo_vector_is_valid(grn_ts_geo_vector value) {
  GRN_TS_VECTOR_IS_VALID(geo)
}
#undef GRN_TS_VECTOR_IS_VALID

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
  /* Note: bridge->src_table does not increment a reference count. */
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
    for (i = 0; i < expr->stack_depth; i++) {
      if (expr->stack[i]) {
        grn_ts_expr_node_close(ctx, expr->stack[i]);
      }
    }
    GRN_FREE(expr->stack);
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
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!table || !grn_ts_obj_is_table(ctx, table) || !expr) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  new_expr = GRN_MALLOCN(grn_ts_expr, 1);
  if (!new_expr) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_MALLOCN failed: %zu x 1",
                      sizeof(grn_ts_expr));
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
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!table || !grn_ts_obj_is_table(ctx, table) ||
      (!str_ptr && str_size) || !expr) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
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
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_REALLOC failed: %zu",
                      n_bytes);
  }
  for (i = expr->stack_size; i < new_size; i++) {
    new_stack[i] = NULL;
  }
  expr->stack = new_stack;
  expr->stack_size = new_size;
  return GRN_SUCCESS;
}

/* grn_ts_expr_deref() dereferences a node. */
static grn_rc
grn_ts_expr_deref(grn_ctx *ctx, grn_ts_expr *expr,
                  grn_ts_expr_node **node_ptr) {
  grn_ts_expr_node *node = *node_ptr;
  while (node->data_kind == GRN_TS_REF) {
    grn_rc rc;
    grn_ts_expr_node *key_node, *bridge_node;
    grn_id table_id = node->data_type;
    grn_obj *table = grn_ctx_at(ctx, table_id);
    if (!table) {
      return GRN_OBJECT_CORRUPT;
    }
    if (!grn_ts_obj_is_table(ctx, table)) {
      grn_obj_unlink(ctx, table);
      return GRN_OBJECT_CORRUPT;
    }
    rc = grn_ts_expr_key_node_open(ctx, table, &key_node);
    grn_obj_unlink(ctx, table);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    rc = grn_ts_expr_bridge_node_open(ctx, node, key_node, &bridge_node);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    node = bridge_node;
  }
  *node_ptr = node;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push(grn_ctx *ctx, grn_ts_expr *expr,
                 const char *str_ptr, size_t str_size) {
  grn_rc rc;
  grn_ts_expr_parser *parser;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      (!str_ptr && str_size)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
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
  grn_obj *column;
  grn_ts_str name = { name_ptr, name_size };
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      !grn_ts_str_is_name(name)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
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
  /* grn_obj_column() returns a column or accessor. */
  column = grn_obj_column(ctx, expr->curr_table, name.ptr, name.size);
  if (!column) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "object not found: \"%.*s\"",
                      (int)name.size, name.ptr);
  }
  return grn_ts_expr_push_obj(ctx, expr, column);
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
      grn_ts_geo value;
      GRN_GEO_POINT_VALUE(obj, value.latitude, value.longitude);
      return grn_ts_expr_push_tokyo_geo(ctx, expr, value);
    }
    case GRN_DB_WGS84_GEO_POINT: {
      grn_ts_geo value;
      GRN_GEO_POINT_VALUE(obj, value.latitude, value.longitude);
      return grn_ts_expr_push_wgs84_geo(ctx, expr, value);
    }
    default: {
      GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "not bulk");
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
    grn_ts_ ## kind *buf;\
    grn_ts_ ## kind ## _vector value = { NULL, grn_uvector_size(ctx, obj) };\
    if (!value.size) {\
      return grn_ts_expr_push_ ## kind ## _vector(ctx, expr, value);\
    }\
    buf = GRN_MALLOCN(grn_ts_ ## kind, value.size);\
    if (!buf) {\
      GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE,\
                        "GRN_MALLOCN failed: %zu x 1",\
                        sizeof(grn_ts_ ## kind));\
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
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(TOKYO_GEO_POINT, tokyo_geo)
    GRN_TS_EXPR_PUSH_UVECTOR_CASE(WGS84_GEO_POINT, wgs84_geo)
    default: {
      GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid data type: %d",
                        obj->header.domain);
    }
  }
}
#undef GRN_TS_EXPR_PUSH_UVECTOR_CASE_WITH_TYPECAST
#undef GRN_TS_EXPR_PUSH_UVECTOR_CASE

/* grn_ts_expr_push_vector() pushes an array of texts. */
static grn_rc
grn_ts_expr_push_vector(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj) {
  switch (obj->header.domain) {
    case GRN_DB_SHORT_TEXT:
    case GRN_DB_TEXT:
    case GRN_DB_LONG_TEXT: {
      size_t i;
      grn_rc rc;
      grn_ts_text *buf;
      grn_ts_text_vector value = { NULL, grn_vector_size(ctx, obj) };
      if (!value.size) {
        return grn_ts_expr_push_text_vector(ctx, expr, value);
      }
      buf = GRN_MALLOCN(grn_ts_text, value.size);
      if (!buf) {
        GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE,
                          "GRN_MALLOCN failed: %zu x %zu",
                          sizeof(grn_ts_text), value.size);
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
      GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid data type: %d",
                        obj->header.domain);
    }
  }
}

static grn_rc
grn_ts_expr_push_single_accessor(grn_ctx *ctx, grn_ts_expr *expr,
                                 grn_accessor *accessor) {
  switch (accessor->action) {
    case GRN_ACCESSOR_GET_ID: {
      return grn_ts_expr_push_id(ctx, expr);
    }
    case GRN_ACCESSOR_GET_SCORE: {
      return grn_ts_expr_push_score(ctx, expr);
    }
    case GRN_ACCESSOR_GET_KEY: {
      if (accessor->obj != expr->curr_table) {
        GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "table conflict");
      }
      return grn_ts_expr_push_key(ctx, expr);
    }
    case GRN_ACCESSOR_GET_VALUE: {
      if (accessor->obj != expr->curr_table) {
        GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "table conflict");
      }
      return grn_ts_expr_push_value(ctx, expr);
    }
    case GRN_ACCESSOR_GET_COLUMN_VALUE: {
      return grn_ts_expr_push_column(ctx, expr, accessor->obj);
    }
    default: {
      GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid accessor action: %d",
                        accessor->action);
    }
  }
}

static grn_rc
grn_ts_expr_push_accessor(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_accessor *accessor) {
  grn_rc rc = grn_ts_expr_push_single_accessor(ctx, expr, accessor);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  for (accessor = accessor->next; accessor; accessor = accessor->next) {
    rc = grn_ts_expr_begin_subexpr(ctx, expr);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    rc = grn_ts_expr_push_single_accessor(ctx, expr, accessor);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    rc = grn_ts_expr_end_subexpr(ctx, expr);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_push_obj(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj) {
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) || !obj) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
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
      GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid object type: %d",
                        obj->header.type);
    }
  }
}

grn_rc
grn_ts_expr_push_id(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_id_node_open(ctx, &node);
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
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_score_node_open(ctx, &node);
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
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_key_node_open(ctx, expr->curr_table, &node);
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
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_value_node_open(ctx, expr->curr_table, &node);
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
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) || !value) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  switch (kind) {
    GRN_TS_EXPR_PUSH_CONST_CASE(BOOL, bool)
    GRN_TS_EXPR_PUSH_CONST_CASE(INT, int)
    GRN_TS_EXPR_PUSH_CONST_CASE(FLOAT, float)
    GRN_TS_EXPR_PUSH_CONST_CASE(TIME, time)
    GRN_TS_EXPR_PUSH_CONST_CASE(TEXT, text)
    GRN_TS_EXPR_PUSH_CONST_CASE(GEO, geo)
    GRN_TS_EXPR_PUSH_CONST_CASE(BOOL_VECTOR, bool_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(INT_VECTOR, int_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(FLOAT_VECTOR, float_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(TIME_VECTOR, time_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(TEXT_VECTOR, text_vector)
    GRN_TS_EXPR_PUSH_CONST_CASE(GEO_VECTOR, geo_vector)
    default: {
      GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid data kind: %d", kind);
    }
  }
}
#undef GRN_TS_EXPR_PUSH_CONST_CASE

grn_rc
grn_ts_expr_push_column(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *column) {
  grn_rc rc;
  grn_ts_expr_node *node;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||
      !column || !grn_ts_obj_is_column(ctx, column) ||
      (DB_OBJ(expr->curr_table)->id != column->header.domain)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  rc = grn_ts_expr_reserve_stack(ctx, expr);
  if (rc == GRN_SUCCESS) {
    rc = grn_ts_expr_column_node_open(ctx, column, &node);
    if (rc == GRN_SUCCESS) {
      expr->stack[expr->stack_depth++] = node;
    }
  }
  return rc;
}

grn_rc
grn_ts_expr_push_op(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_op_type op_type) {
  grn_rc rc;
  grn_ts_expr_node **args, *node;
  size_t i, n_args, max_n_args;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  n_args = grn_ts_op_get_n_args(op_type);
  if (!n_args) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid #arguments: %zu", n_args);
  }
  max_n_args = expr->stack_depth;
  if (expr->n_bridges) {
    max_n_args -= expr->bridges[expr->n_bridges - 1].stack_depth;
  }
  if (n_args > max_n_args) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid #arguments: %zu, %zu",
                      n_args, expr->stack_depth);
  }
  /* Arguments are the top n_args nodes in the stack. */
  args = &expr->stack[expr->stack_depth - n_args];
  for (i = 0; i < n_args; i++) {
    /*
     * FIXME: Operators "==" and "!=" should compare arguments as references
     *        if possible.
     */
    rc = grn_ts_expr_deref(ctx, expr, &args[i]);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  expr->stack_depth -= n_args;
  rc = grn_ts_expr_op_node_open(ctx, op_type, args, n_args, &node);
  if (rc == GRN_SUCCESS) {
    expr->stack[expr->stack_depth++] = node;
  }
  return rc;
}

#define GRN_TS_EXPR_PUSH_CONST(KIND, kind)\
  grn_rc rc;\
  grn_ts_expr_node *node;\
  if (!ctx) {\
    return GRN_INVALID_ARGUMENT;\
  }\
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) ||\
      !grn_ts_ ## kind ## _is_valid(value)) {\
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");\
  }\
  rc = grn_ts_expr_reserve_stack(ctx, expr);\
  if (rc == GRN_SUCCESS) {\
    rc = grn_ts_expr_const_node_open(ctx, GRN_TS_ ## KIND, &value, &node);\
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
grn_ts_expr_push_geo(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_geo value) {
  GRN_TS_EXPR_PUSH_CONST(GEO, geo)
  return rc;
}

grn_rc
grn_ts_expr_push_tokyo_geo(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_geo value) {
  GRN_TS_EXPR_PUSH_CONST(GEO, geo)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return rc;
}

grn_rc
grn_ts_expr_push_wgs84_geo(grn_ctx *ctx, grn_ts_expr *expr, grn_ts_geo value) {
  GRN_TS_EXPR_PUSH_CONST(GEO, geo)
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
grn_ts_expr_push_geo_vector(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_geo_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_VECTOR, geo_vector)
  return rc;
}

grn_rc
grn_ts_expr_push_tokyo_geo_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                  grn_ts_geo_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_VECTOR, geo_vector)
  node->data_type = GRN_DB_TOKYO_GEO_POINT;
  return rc;
}

grn_rc
grn_ts_expr_push_wgs84_geo_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                  grn_ts_geo_vector value) {
  GRN_TS_EXPR_PUSH_CONST(GEO_VECTOR, geo_vector)
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
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_REALLOC failed: %zu",
                      n_bytes);
  }
  expr->bridges = new_bridges;
  expr->max_n_bridges = new_max_n_bridges;
  return GRN_SUCCESS;
}

grn_rc
grn_ts_expr_begin_subexpr(grn_ctx *ctx, grn_ts_expr *expr) {
  grn_rc rc;
  size_t max_n_args;
  grn_obj *obj;
  grn_ts_expr_node *node;
  grn_ts_expr_bridge *bridge;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }
  max_n_args = expr->stack_depth;
  if (expr->n_bridges) {
    max_n_args -= expr->bridges[expr->n_bridges - 1].stack_depth;
  }
  if (!max_n_args) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid argument");
  }

  /* Check whehter or not the latest node refers to a table. */
  node = expr->stack[expr->stack_depth - 1];
  if ((node->data_kind & ~GRN_TS_VECTOR_FLAG) != GRN_TS_REF) {
    GRN_TS_ERR_RETURN(GRN_INVALID_ARGUMENT, "invalid data kind: %d",
                      node->data_kind);
  }
  obj = grn_ctx_at(ctx, node->data_type);
  if (!obj) {
    GRN_TS_ERR_RETURN(GRN_UNKNOWN_ERROR, "grn_ctx_at failed: %d",
                      node->data_type);
  }
  if (!grn_ts_obj_is_table(ctx, obj)) {
    grn_obj_unlink(ctx, obj);
    GRN_TS_ERR_RETURN(GRN_UNKNOWN_ERROR, "not table: %d", node->data_type);
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
  rc = grn_ts_expr_bridge_node_open(ctx, args[0], args[1], &node);
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
  grn_rc rc;
  grn_ts_expr_node *root;
  if (!ctx || !expr || (expr->type != GRN_TS_EXPR_INCOMPLETE)) {
    return GRN_INVALID_ARGUMENT;
  }
  if (expr->stack_depth != 1) {
    return GRN_INVALID_ARGUMENT;
  }
  rc = grn_ts_expr_deref(ctx, expr, &expr->stack[0]);
  if (rc != GRN_SUCCESS) {
    return rc;
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
grn_ts_expr_evaluate_to_buf(grn_ctx *ctx, grn_ts_expr *expr,
                            const grn_ts_record *in, size_t n_in,
                            grn_ts_buf *out) {
  if (!ctx || !expr || (expr->type == GRN_TS_EXPR_INCOMPLETE) ||
      (expr->type == GRN_TS_EXPR_BROKEN) || (!in && n_in) || !out) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!n_in) {
    return GRN_SUCCESS;
  }
  return grn_ts_expr_node_evaluate_to_buf(ctx, expr->root, in, n_in, out);
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
