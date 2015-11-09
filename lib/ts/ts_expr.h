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

#ifndef GRN_TS_EXPR_H
#define GRN_TS_EXPR_H

#include <stdint.h>

#include "../grn_ts.h"

#include "ts_buf.h"

#ifdef __cplusplus
extern "C" {
#endif

/*-------------------------------------------------------------
 * Enumeration types.
 */

/* grn_builtin_type or table ID. */
typedef grn_id grn_ts_data_type;

enum { GRN_TS_VECTOR_FLAG = 1 << 7 };

typedef enum {
  GRN_TS_VOID             = 0, /* GRN_DB_VOID */
  GRN_TS_BOOL             = 1, /* GRN_DB_BOOL */
  GRN_TS_INT              = 2, /* GRN_DB_[U]INT(8/16/32/64) */
  GRN_TS_FLOAT            = 3, /* GRN_DB_FLOAT */
  GRN_TS_TIME             = 4, /* GRN_DB_TIME */
  GRN_TS_TEXT             = 5, /* GRN_DB_[SHORT_/LONG_]TEST */
  GRN_TS_GEO_POINT        = 6, /* GRN_DB_(TOKYO/WGS84)_GEO_POINT */
  GRN_TS_REF              = 7, /* Table reference. */
  GRN_TS_BOOL_VECTOR      = GRN_TS_VECTOR_FLAG | GRN_TS_BOOL,
  GRN_TS_INT_VECTOR       = GRN_TS_VECTOR_FLAG | GRN_TS_INT,
  GRN_TS_FLOAT_VECTOR     = GRN_TS_VECTOR_FLAG | GRN_TS_FLOAT,
  GRN_TS_TIME_VECTOR      = GRN_TS_VECTOR_FLAG | GRN_TS_TIME,
  GRN_TS_TEXT_VECTOR      = GRN_TS_VECTOR_FLAG | GRN_TS_TEXT,
  GRN_TS_GEO_POINT_VECTOR = GRN_TS_VECTOR_FLAG | GRN_TS_GEO_POINT,
  GRN_TS_REF_VECTOR       = GRN_TS_VECTOR_FLAG | GRN_TS_REF
} grn_ts_data_kind;

typedef enum {
  /* Invalid operator. */
  GRN_TS_OP_NOP,

  /* Unary operators. */
  GRN_TS_OP_LOGICAL_NOT,    /* !X */
  GRN_TS_OP_BITWISE_NOT,    /* ~X */
  GRN_TS_OP_POSITIVE,       /* +X */
  GRN_TS_OP_NEGATIVE,       /* -X */

  /* Binary operators. */
  GRN_TS_OP_LOGICAL_AND,            /* X && Y  */
  GRN_TS_OP_LOGICAL_OR,             /* X || Y  */
  GRN_TS_OP_LOGICAL_SUB,            /* X &! Y  */
  GRN_TS_OP_BITWISE_AND,            /* X & Y   */
  GRN_TS_OP_BITWISE_OR,             /* X | Y   */
  GRN_TS_OP_BITWISE_XOR,            /* X ^ Y   */
  GRN_TS_OP_EQUAL,                  /* X == Y  */
  GRN_TS_OP_NOT_EQUAL,              /* X != Y  */
  GRN_TS_OP_LESS,                   /* X < Y   */
  GRN_TS_OP_LESS_EQUAL,             /* X <= Y  */
  GRN_TS_OP_GREATER,                /* X > Y   */
  GRN_TS_OP_GREATER_EQUAL,          /* X >= Y  */
  GRN_TS_OP_SHIFT_ARITHMETIC_LEFT,  /* X << Y  */
  GRN_TS_OP_SHIFT_ARITHMETIC_RIGHT, /* X >> Y  */
  GRN_TS_OP_SHIFT_LOGICAL_LEFT,     /* X <<< Y */
  GRN_TS_OP_SHIFT_LOGICAL_RIGHT,    /* X >>> Y */
  GRN_TS_OP_PLUS,                   /* X + Y   */
  GRN_TS_OP_MINUS,                  /* X - Y   */
  GRN_TS_OP_MULTIPLICATION,         /* X * Y   */
  GRN_TS_OP_DIVISION,               /* X / Y   */
  GRN_TS_OP_MODULUS                 /* X % Y   */
} grn_ts_op_type;

typedef enum {
  GRN_TS_EXPR_INCOMPLETE, /* An incomplete expression. */
  GRN_TS_EXPR_BROKEN,     /* A broken expression. */
                          /* Any operation fails for a broken expression. */
  GRN_TS_EXPR_ID,         /* An expression associated with _id. */
  GRN_TS_EXPR_SCORE,      /* An expression associated with _score. */
  GRN_TS_EXPR_CONST,      /* A const. */
  GRN_TS_EXPR_VARIABLE    /* An expression that contains a variable. */
} grn_ts_expr_type;

typedef enum {
  GRN_TS_EXPR_ID_NODE,     /* Associated with the ID (_id). */
  GRN_TS_EXPR_SCORE_NODE,  /* Asscoaited with the score (_score). */
  GRN_TS_EXPR_KEY_NODE,    /* Asscoaited with the key (_key). */
  GRN_TS_EXPR_VALUE_NODE,  /* Asscoaited with the embedded value (_value). */
  GRN_TS_EXPR_CONST_NODE,  /* Associated with a const. */
  GRN_TS_EXPR_COLUMN_NODE, /* Associated with a column. */
  GRN_TS_EXPR_OP_NODE,     /* Associated with an operator. */
  GRN_TS_EXPR_BRIDGE_NODE  /* Associated with a bridge to a subexpression. */
} grn_ts_expr_node_type;

/*-------------------------------------------------------------
 * Expression components.
 */

#define GRN_TS_EXPR_NODE_COMMON_MEMBERS\
  grn_ts_expr_node_type type; /* Node type. */\
  grn_ts_data_kind data_kind; /* Abstract data type. */\
  grn_ts_data_type data_type; /* Detailed data type. */

typedef struct {
  GRN_TS_EXPR_NODE_COMMON_MEMBERS
} grn_ts_expr_node;

typedef struct {
  grn_obj *src_table;  /* The source table of a bridge (no ref. count). */
  grn_obj *dest_table; /* The destination table of a bridge. */
  size_t stack_depth;  /* The stack depth (position) of a bridge. */
} grn_ts_expr_bridge;

typedef struct {
  grn_obj *table;              /* Associated table. */
  grn_obj *curr_table;         /* Current table (no ref. count). */
  grn_ts_expr_type type;       /* Expression type. */
  grn_ts_data_kind data_kind;  /* Abstract data type. */
  grn_ts_data_type data_type;  /* Detailed data type. */
  grn_ts_expr_node *root;      /* Root node. */
  grn_ts_expr_node **nodes;    /* Buffer for a node stack. */
  size_t n_nodes;              /* Number of nodes. */
  size_t max_n_nodes;          /* Max. number (capacity) of nodes. */
  grn_ts_expr_node **stack;    /* Node stack. */
  size_t stack_depth;          /* Node stack's current depth. */
  size_t stack_size;           /* Node stack's size (capacity). */
  grn_ts_expr_bridge *bridges; /* Bridges to subexpressions. */
  size_t n_bridges;            /* Number of bridges (subexpression depth). */
  size_t max_n_bridges;        /* Max. number (capacity) of bridges. */
} grn_ts_expr;

/* grn_ts_expr_open() creates an empty expression. */
grn_rc grn_ts_expr_open(grn_ctx *ctx, grn_obj *table, grn_ts_expr **expr);

/* grn_ts_expr_parse() creates an expression from a string. */
grn_rc grn_ts_expr_parse(grn_ctx *ctx, grn_obj *table,
                         const char *str_ptr, size_t str_size,
                         grn_ts_expr **expr);

/* grn_ts_expr_close() destroys an expression. */
grn_rc grn_ts_expr_close(grn_ctx *ctx, grn_ts_expr *expr);

/*
 * grn_ts_expr_get_table() returns the associated table.
 * If arguments are invalid, the return value is NULL.
 */
grn_obj *grn_ts_expr_get_table(grn_ctx *ctx, grn_ts_expr *expr);

/*
 * grn_ts_expr_get_type() returns the expression type.
 * If arguments are invalid, the return value is GRN_EXPR_BROKEN.
 */
grn_ts_expr_type grn_ts_expr_get_type(grn_ctx *ctx, grn_ts_expr *expr);

/*
 * grn_ts_expr_get_data_kind() returns the data kind.
 * If arguments are invalid, the return value is GRN_TS_VOID.
 */
grn_ts_data_kind grn_ts_expr_get_data_kind(grn_ctx *ctx, grn_ts_expr *expr);

/*
 * grn_ts_expr_get_data_type() returns the data type.
 * If arguments are invalid, the return value is GRN_DB_VOID.
 */

grn_ts_data_type grn_ts_expr_get_data_type(grn_ctx *ctx, grn_ts_expr *expr);

/*
 * grn_ts_expr_get_root() returns the root node.
 * If arguments are invalid, the return value is NULL.
 */
grn_ts_expr_node *grn_ts_expr_get_root(grn_ctx *ctx, grn_ts_expr *expr);

/* grn_ts_expr_push() parses a string and pushes the result. */
grn_rc grn_ts_expr_push(grn_ctx *ctx, grn_ts_expr *expr,
                        const char *str_ptr, size_t str_size);

/* grn_ts_expr_push_name() pushes a named object. */
grn_rc grn_ts_expr_push_name(grn_ctx *ctx, grn_ts_expr *expr,
                             const char *name_ptr, size_t name_size);

/*
 * grn_ts_expr_push_obj() pushes an object.
 *
 * Acceptable objects are as follows:
 * - Consts
 *  - GRN_BULK: GRN_DB_*.
 *  - GRN_UVECTOR: GRN_DB_* except GRN_DB_[SHORT/LONG_]TEXT.
 *  - GRN_VECTOR: GRN_DB_[SHORT/LONG_]TEXT.
 * - Columns
 *  - GRN_ACCESSOR: _id, _score, _key, and _value.
 *  - GRN_COLUMN_FIX_SIZE: GRN_DB_* except GRN_DB_[SHORT/LONG_]TEXT.
 *  - GRN_COLUMN_VAR_SIZE: GRN_DB_[SHORT/LONG_]TEXT.
 */
grn_rc grn_ts_expr_push_obj(grn_ctx *ctx, grn_ts_expr *expr, grn_obj *obj);

/* grn_ts_expr_push_id() pushes "_id". */
grn_rc grn_ts_expr_push_id(grn_ctx *ctx, grn_ts_expr *expr);
/* grn_ts_expr_push_score() pushes "_score". */
grn_rc grn_ts_expr_push_score(grn_ctx *ctx, grn_ts_expr *expr);
/* grn_ts_expr_push_key() pushes "_key". */
grn_rc grn_ts_expr_push_key(grn_ctx *ctx, grn_ts_expr *expr);
/* grn_ts_expr_push_key() pushes "_value". */
grn_rc grn_ts_expr_push_value(grn_ctx *ctx, grn_ts_expr *expr);
/* grn_ts_expr_push_const() pushes a column. */
grn_rc grn_ts_expr_push_const(grn_ctx *ctx, grn_ts_expr *expr,
                              grn_ts_data_kind kind, const void *value);
/* grn_ts_expr_push_column() pushes a column. */
grn_rc grn_ts_expr_push_column(grn_ctx *ctx, grn_ts_expr *expr,
                               grn_obj *column);
/* grn_ts_expr_push_op() pushes an operator. */
grn_rc grn_ts_expr_push_op(grn_ctx *ctx, grn_ts_expr *expr,
                           grn_ts_op_type op_type);

/* grn_ts_expr_push_bool() pushes a Bool const. */
grn_rc grn_ts_expr_push_bool(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_bool value);
/* grn_ts_expr_push_int() pushes an Int64 const. */
grn_rc grn_ts_expr_push_int(grn_ctx *ctx, grn_ts_expr *expr,
                            grn_ts_int value);
/* grn_ts_expr_push_float() pushes a Float const. */
grn_rc grn_ts_expr_push_float(grn_ctx *ctx, grn_ts_expr *expr,
                              grn_ts_float value);
/* grn_ts_expr_push_time() pushes a Time const. */
grn_rc grn_ts_expr_push_time(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_time value);
/* grn_ts_expr_push_text() pushes a Text const. */
grn_rc grn_ts_expr_push_text(grn_ctx *ctx, grn_ts_expr *expr,
                             grn_ts_text value);
/* grn_ts_expr_push_geo_point() pushes a GeoPoint const. */
grn_rc grn_ts_expr_push_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                                  grn_ts_geo_point value);
/* grn_ts_expr_push_tokyo_geo_point() pushes a TokyoGeoPoint const. */
grn_rc grn_ts_expr_push_tokyo_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                                        grn_ts_geo_point value);
/* grn_ts_expr_push_wgs84_geo_point() pushes a WGS84GeoPoint const. */
grn_rc grn_ts_expr_push_wgs84_geo_point(grn_ctx *ctx, grn_ts_expr *expr,
                                        grn_ts_geo_point value);
/* grn_ts_expr_push_bool_vector() pushes a Bool vector const. */
grn_rc grn_ts_expr_push_bool_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                    grn_ts_bool_vector value);
/* grn_ts_expr_push_int_vector() pushes an Int64 vector const. */
grn_rc grn_ts_expr_push_int_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                   grn_ts_int_vector value);
/* grn_ts_expr_push_float_vector() pushes a Float vector const. */
grn_rc grn_ts_expr_push_float_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                     grn_ts_float_vector value);
/* grn_ts_expr_push_time_vector() pushes a Time vector const. */
grn_rc grn_ts_expr_push_time_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                    grn_ts_time_vector value);
/* grn_ts_expr_push_text_vector() pushes a Text vector const. */
grn_rc grn_ts_expr_push_text_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                    grn_ts_text_vector value);
/* grn_ts_expr_push_geo_point_vector() pushes a GeoPoint vector const. */
grn_rc grn_ts_expr_push_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                         grn_ts_geo_point_vector value);
/*
 * grn_ts_expr_push_tokyo_geo_point_vector() pushes a TokyoGeoPoint vector
 * const.
 */
grn_rc grn_ts_expr_push_tokyo_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                               grn_ts_geo_point_vector value);
/*
 * grn_ts_expr_push_wgs84_geo_point_vector() pushes a WGS84GeoPoint vector
 * const.
 */
grn_rc grn_ts_expr_push_wgs84_geo_point_vector(grn_ctx *ctx, grn_ts_expr *expr,
                                               grn_ts_geo_point_vector value);

/* grn_ts_expr_begin_subexpr() begins a subexpression. */
grn_rc grn_ts_expr_begin_subexpr(grn_ctx *ctx, grn_ts_expr *expr);

/* grn_ts_expr_end_subexpr() ends a subexpression. */
grn_rc grn_ts_expr_end_subexpr(grn_ctx *ctx, grn_ts_expr *expr);

/* grn_rc grn_ts_expr_complete() completes an expression. */
grn_rc grn_ts_expr_complete(grn_ctx *ctx, grn_ts_expr *expr);

/* grn_ts_expr_evaluate() evaluates an expression. */
grn_rc grn_ts_expr_evaluate(grn_ctx *ctx, grn_ts_expr *expr,
                            const grn_ts_record *in, size_t n_in, void *out);
/* grn_ts_expr_evaluate_to_buf() evaluates an expression. */
grn_rc grn_ts_expr_evaluate_to_buf(grn_ctx *ctx, grn_ts_expr *expr,
                                   const grn_ts_record *in, size_t n_in,
                                   grn_ts_buf *out);

/* grn_ts_expr_filter() filters records. */
grn_rc grn_ts_expr_filter(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_ts_record *in, size_t n_in,
                          grn_ts_record *out, size_t *n_out);

/* grn_ts_expr_adjust() updates scores. */
grn_rc grn_ts_expr_adjust(grn_ctx *ctx, grn_ts_expr *expr,
                          grn_ts_record *io, size_t n_io);

#ifdef __cplusplus
}
#endif

#endif /* GRN_TS_EXPR_H */
