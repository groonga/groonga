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

#include "../grn_ctx_impl.h"
#include "../grn_dat.h"
#include "../grn_db.h"
#include "../grn_geo.h"
#include "../grn_hash.h"
#include "../grn_pat.h"
#include "../grn_store.h"

#include "ts_log.h"
#include "ts_str.h"


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
 * Operators.
 */

/* Operator precedence. */
typedef int grn_ts_op_precedence;

/* grn_ts_op_get_n_args() returns the number of arguments. */
static size_t
grn_ts_op_get_n_args(grn_ts_op_type op_type) {
  switch (op_type) {
    case GRN_TS_OP_LOGICAL_NOT: /* !X */
    case GRN_TS_OP_BITWISE_NOT: /* ~X */
    case GRN_TS_OP_POSITIVE:    /* +X */
    case GRN_TS_OP_NEGATIVE: {  /* -X */
      return 1;
    }
    case GRN_TS_OP_LOGICAL_AND:            /* X && Y  */
    case GRN_TS_OP_LOGICAL_OR:             /* X || Y  */
    case GRN_TS_OP_LOGICAL_SUB:            /* X &! Y  */
    case GRN_TS_OP_BITWISE_AND:            /* X & Y   */
    case GRN_TS_OP_BITWISE_OR:             /* X | Y   */
    case GRN_TS_OP_BITWISE_XOR:            /* X ^ Y   */
    case GRN_TS_OP_EQUAL:                  /* X == Y  */
    case GRN_TS_OP_NOT_EQUAL:              /* X != Y  */
    case GRN_TS_OP_LESS:                   /* X < Y   */
    case GRN_TS_OP_LESS_EQUAL:             /* X <= Y  */
    case GRN_TS_OP_GREATER:                /* X > Y   */
    case GRN_TS_OP_GREATER_EQUAL:          /* X >= Y  */
    case GRN_TS_OP_SHIFT_ARITHMETIC_LEFT:  /* X << Y  */
    case GRN_TS_OP_SHIFT_ARITHMETIC_RIGHT: /* X >> Y  */
    case GRN_TS_OP_SHIFT_LOGICAL_LEFT:     /* X <<< Y */
    case GRN_TS_OP_SHIFT_LOGICAL_RIGHT:    /* X >>> Y */
    case GRN_TS_OP_PLUS:                   /* X + Y   */
    case GRN_TS_OP_MINUS:                  /* X - Y   */
    case GRN_TS_OP_MULTIPLICATION:         /* X * Y   */
    case GRN_TS_OP_DIVISION:               /* X / Y   */
    case GRN_TS_OP_MODULUS: {              /* X % Y   */
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
      return 3;
    }
    case GRN_TS_OP_LOGICAL_SUB: {
      return 4;
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
    case GRN_TS_OP_SHIFT_ARITHMETIC_LEFT:
    case GRN_TS_OP_SHIFT_ARITHMETIC_RIGHT:
    case GRN_TS_OP_SHIFT_LOGICAL_LEFT:
    case GRN_TS_OP_SHIFT_LOGICAL_RIGHT: {
      return 11;
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

/*-------------------------------------------------------------
 * Groonga objects.
 */

/* grn_ts_obj_increment_ref_count() increments an object reference count. */
static grn_rc
grn_ts_obj_increment_ref_count(grn_ctx *ctx, grn_obj *obj) {
  grn_id id = grn_obj_id(ctx, obj);
  grn_obj *obj_clone = grn_ctx_at(ctx, id);
  if (!obj_clone) {
    GRN_TS_ERR_RETURN(GRN_UNKNOWN_ERROR, "grn_ctx_at failed: %d", id);
  }
  if (obj_clone != obj) {
    grn_obj_unlink(ctx, obj_clone);
    GRN_TS_ERR_RETURN(GRN_UNKNOWN_ERROR, "wrong object: %p != %p",
                      obj, obj_clone);
  }
  return GRN_SUCCESS;
}

/* grn_ts_obj_is_table() returns whether or not an object is a table. */
static grn_ts_bool
grn_ts_obj_is_table(grn_ctx *ctx, grn_obj *obj) {
  return grn_obj_is_table(ctx, obj);
}

/* grn_ts_obj_is_column() returns whether or not an object is a column. */
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

/*-------------------------------------------------------------
 * grn_ts_expr_parser.
 */

/*
 * FIXME: A grn_ts_expr_parser object is designed to parse one expression
 *        string. grn_ts_expr_parser_parse() should not be called more than
 *        once.
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
  grn_ts_data_kind data_kind; /* The data kind of the const. */
  grn_ts_any content;         /* The const. */
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
  grn_ts_buf str_buf;                    /* Buffer for a source string. */
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
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_MALLOCN failed: %zu x 1",\
                      sizeof(grn_ts_expr_ ## type ## _token));\
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
  grn_ts_buf_init(ctx, &parser->str_buf);
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
  grn_ts_buf_fin(ctx, &parser->str_buf);
}

/* grn_ts_expr_parser_open() creates a parser. */
static grn_rc
grn_ts_expr_parser_open(grn_ctx *ctx, grn_ts_expr *expr,
                        grn_ts_expr_parser **parser) {
  grn_ts_expr_parser *new_parser = GRN_MALLOCN(grn_ts_expr_parser, 1);
  if (!new_parser) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_MALLOCN failed: %zu x 1",
                      sizeof(grn_ts_expr_parser));
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
  *token = (grn_ts_expr_token *)new_token;
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
  *token = (grn_ts_expr_token *)new_token;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_tokenize_number() tokenizes an Int or Float literal. */
static grn_rc
grn_ts_expr_parser_tokenize_number(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                   grn_ts_str str, grn_ts_expr_token **token) {
  char *end;
  grn_rc rc;
  grn_ts_int int_value;
  grn_ts_str token_str;
  grn_ts_expr_const_token *new_token;

  int_value = strtol(str.ptr, &end, 0);
  if ((end != str.ptr) && (*end != '.') && (*end != 'e')) {
    if (grn_ts_byte_is_name_char(*end)) {
      GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT,
                        "unterminated Int literal: \"%.*s\"",
                        (int)str.size, str.ptr);
    }
    token_str.ptr = str.ptr;
    token_str.size = end - str.ptr;
    rc = grn_ts_expr_const_token_open(ctx, token_str, &new_token);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    new_token->data_kind = GRN_TS_INT;
    new_token->content.as_int = int_value;
  } else {
    grn_ts_float float_value = strtod(str.ptr, &end);
    if (end == str.ptr) {
      GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid number literal: \"%.*s\"",
                        (int)str.size, str.ptr);
    }
    if (grn_ts_byte_is_name_char(*end)) {
      GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT,
                        "unterminated Float literal: \"%.*s\"",
                        (int)str.size, str.ptr);
    }
    token_str.ptr = str.ptr;
    token_str.size = end - str.ptr;
    rc = grn_ts_expr_const_token_open(ctx, token_str, &new_token);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    new_token->data_kind = GRN_TS_FLOAT;
    new_token->content.as_float = float_value;
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
    GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "no closing double quote: \"%.*s\"",
                      (int)str.size, str.ptr);
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
  *token = (grn_ts_expr_token *)new_token;
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
  *token = (grn_ts_expr_token *)new_token;
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
      grn_ts_str bracket;
      const grn_ts_expr_bracket_token *bracket_token;
      bracket_token = (const grn_ts_expr_bracket_token *)prev_token;
      bracket = bracket_token->src;
      switch (bracket.ptr[0]) {
        case '(': case '[': {
          n_args = 1;
          break;
        }
        case ')': case ']': {
          n_args = 2;
          break;
        }
        default: {
          GRN_TS_ERR_RETURN(GRN_OBJECT_CORRUPT, "undefined bracket: \"%.*s\"",
                            (int)bracket.size, bracket.ptr);
        }
      }
      break;
    }
    default: {
      GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence: %d",
                        prev_token->type);
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
    case '!': {
      if ((str.size >= 2) && (str.ptr[1] == '=')) {
        token_str.size = 2;
        op_type = GRN_TS_OP_NOT_EQUAL;
      } else {
        token_str.size = 1;
        op_type = GRN_TS_OP_LOGICAL_NOT;
      }
      rc = grn_ts_expr_op_token_open(ctx, token_str, op_type, &new_token);
      break;
    }
#define GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE(label, TYPE_1, TYPE_2, TYPE_3,\
                                            TYPE_EQUAL)\
  case label: {\
    if ((str.size >= 2) && (str.ptr[1] == '=')) {\
      token_str.size = 2;\
      op_type = GRN_TS_OP_ ## TYPE_EQUAL;\
    } else if ((str.size >= 2) && (str.ptr[1] == label)) {\
      if ((str.size >= 3) && (str.ptr[2] == label)) {\
        token_str.size = 3;\
        op_type = GRN_TS_OP_ ## TYPE_3;\
      } else {\
        token_str.size = 2;\
        op_type = GRN_TS_OP_ ## TYPE_2;\
      }\
    } else {\
      token_str.size = 1;\
      op_type = GRN_TS_OP_ ## TYPE_1;\
    }\
    rc = grn_ts_expr_op_token_open(ctx, token_str, op_type, &new_token);\
    break;\
  }
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('<', LESS, SHIFT_ARITHMETIC_LEFT,
                                        SHIFT_LOGICAL_LEFT, LESS_EQUAL)
    GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE('>', GREATER, SHIFT_ARITHMETIC_RIGHT,
                                        SHIFT_LOGICAL_RIGHT, GREATER_EQUAL)
#undef GRN_TS_EXPR_PARSER_TOKENIZE_OP_CASE
    case '&': {
      if ((str.size >= 2) && (str.ptr[1] == '&')) {
        token_str.size = 2;
        op_type = GRN_TS_OP_LOGICAL_AND;
      } else if ((str.size >= 2) && (str.ptr[1] == '&')) {
        token_str.size = 2;
        op_type = GRN_TS_OP_LOGICAL_SUB;
      } else {
        token_str.size = 1;
        op_type = GRN_TS_OP_BITWISE_AND;
      }
      rc = grn_ts_expr_op_token_open(ctx, token_str, op_type, &new_token);
      break;
    }
    case '|': {
      if ((str.size >= 2) && (str.ptr[1] == '|')) {
        token_str.size = 2;
        op_type = GRN_TS_OP_LOGICAL_OR;
      } else {
        token_str.size = 1;
        op_type = GRN_TS_OP_BITWISE_OR;
      }
      rc = grn_ts_expr_op_token_open(ctx, token_str, op_type, &new_token);
      break;
    }
    case '=': {
      if ((str.size < 2) || (str.ptr[1] != '=')) {
        GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT,
                          "single equal not available: =\"%.*s\"",
                          (int)str.size, str.ptr);
      }
      token_str.size = 2;
      rc = grn_ts_expr_op_token_open(ctx, token_str, GRN_TS_OP_EQUAL,
                                     &new_token);
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
      GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid character: \"%.*s\"",
                        (int)str.size, str.ptr);
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
  if (grn_ts_str_has_number_prefix(rest)) {
    grn_ts_expr_token *prev_token;
    if ((rest.ptr[0] != '+') && (rest.ptr[0] != '-')) {
      return grn_ts_expr_parser_tokenize_number(ctx, parser, rest, token);
    }
    prev_token = parser->tokens[parser->n_tokens - 1];
    switch (prev_token->type) {
      case GRN_TS_EXPR_START_TOKEN:
      case GRN_TS_EXPR_OP_TOKEN: {
        return grn_ts_expr_parser_tokenize_number(ctx, parser, rest, token);
      }
      case GRN_TS_EXPR_BRACKET_TOKEN: {
        if ((prev_token->src.ptr[0] == '(') ||
            (prev_token->src.ptr[0] == '[')) {
          return grn_ts_expr_parser_tokenize_number(ctx, parser, rest, token);
        }
        break;
      }
      default: {
        break;
      }
    }
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
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_REALLOC failed: %zu",
                      n_bytes);
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
  grn_ts_expr_token *token = NULL;
  GRN_TS_DEBUG("str = \"%.*s\"", (int)str.size, str.ptr);
  do {
    grn_rc rc = grn_ts_expr_parser_reserve_tokens(ctx, parser);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    rc = grn_ts_expr_parser_tokenize_next(ctx, parser, rest, &token);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
    if ((token->type != GRN_TS_EXPR_START_TOKEN) &&
        (token->type != GRN_TS_EXPR_END_TOKEN)) {
      GRN_TS_DEBUG("token = \"%.*s\"", (int)token->src.size, token->src.ptr);
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
      GRN_TS_ERR_RETURN(GRN_OBJECT_CORRUPT, "invalid data kind: %d",
                        token->data_kind);
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
  return grn_ts_expr_push_op(ctx, parser->expr, token->op_type);
}

/*
 * grn_ts_expr_parser_apply_one() applies a bridge or prior operator.
 * If there is no target, this function returns GRN_END_OF_DATA.
 */
// FIXME: Support a ternary operator.
static grn_rc
grn_ts_expr_parser_apply_one(grn_ctx *ctx, grn_ts_expr_parser *parser,
                             grn_ts_op_precedence precedence_threshold) {
  grn_rc rc;
  grn_ts_str src;
  grn_ts_expr_token **stack = parser->stack;
  grn_ts_expr_dummy_token *dummy_token;
  size_t n_args, depth = parser->stack_depth;
  if (depth < 2) {
    return GRN_END_OF_DATA;
  }
  if (stack[depth - 1]->type != GRN_TS_EXPR_DUMMY_TOKEN) {
    GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "argument must be dummy token");
  }

  /* Check the number of arguments. */
  switch (stack[depth - 2]->type) {
    case GRN_TS_EXPR_BRIDGE_TOKEN: {
      rc = grn_ts_expr_end_subexpr(ctx, parser->expr);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      n_args = 2;
      break;
    }
    case GRN_TS_EXPR_OP_TOKEN: {
      grn_ts_expr_op_token *op_token;
      grn_ts_op_precedence precedence;
      op_token = (grn_ts_expr_op_token *)stack[depth - 2];
      precedence = grn_ts_op_get_precedence(op_token->op_type);
      if (precedence < precedence_threshold) {
        return GRN_END_OF_DATA;
      }
      rc = grn_ts_expr_parser_push_op(ctx, parser, op_token);
      if (rc != GRN_SUCCESS) {
        return rc;
      }
      n_args = grn_ts_op_get_n_args(op_token->op_type);
      break;
    }
    default: {
      return GRN_END_OF_DATA;
    }
  }

  /* Concatenate the source strings. */
  switch (n_args) {
    case 1: {
      grn_ts_expr_token *arg = stack[depth - 1];
      src.ptr = stack[depth - 2]->src.ptr;
      src.size = (arg->src.ptr + arg->src.size) - src.ptr;
      break;
    }
    case 2: {
      grn_ts_expr_token *args[2] = { stack[depth - 3], stack[depth - 1] };
      src.ptr = args[0]->src.ptr;
      src.size = (args[1]->src.ptr + args[1]->src.size) - src.ptr;
      break;
    }
    default: {
      GRN_TS_ERR_RETURN(GRN_OPERATION_NOT_SUPPORTED,
                        "invalid #arguments: %zu", n_args);
    }
  }

  /* Replace the operator and argument tokens with a dummy token. */
  dummy_token = &parser->dummy_tokens[parser->n_dummy_tokens++];
  GRN_TS_DEBUG("dummy token: \"%.*s\"", (int)src.size, src.ptr);
  grn_ts_expr_dummy_token_init(ctx, dummy_token, src);
  depth -= n_args + 1;
  stack[depth++] = dummy_token;
  parser->stack_depth = depth;
  return GRN_SUCCESS;
}

/* grn_ts_expr_parser_apply() applies bridges and prior operators. */
static grn_rc
grn_ts_expr_parser_apply(grn_ctx *ctx, grn_ts_expr_parser *parser,
                         grn_ts_op_precedence precedence_threshold) {
  for ( ; ; ) {
    grn_rc rc = grn_ts_expr_parser_apply_one(ctx, parser,
                                             precedence_threshold);
    if (rc == GRN_END_OF_DATA) {
      return GRN_SUCCESS;
    } else if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
}

/* grn_ts_expr_parser_analyze_op() analyzes a token. */
static grn_rc
grn_ts_expr_parser_analyze_op(grn_ctx *ctx, grn_ts_expr_parser *parser,
                              grn_ts_expr_op_token *token) {
  size_t n_args = grn_ts_op_get_n_args(token->op_type);
  grn_ts_expr_token *ex_token = parser->stack[parser->stack_depth - 1];
  if (n_args == 1) {
    if (ex_token->type == GRN_TS_EXPR_DUMMY_TOKEN) {
      GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence");
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
        GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence");
      }
      parser->stack[parser->stack_depth++] = (grn_ts_expr_token *)token;
      return GRN_SUCCESS;
    }
    case '[': {
      if (ex_token->type != GRN_TS_EXPR_DUMMY_TOKEN) {
        GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence");
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
        GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence");
      }
      ex_ex_token = parser->stack[parser->stack_depth - 2];
      if (ex_ex_token->type != GRN_TS_EXPR_BRACKET_TOKEN) {
        GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence");
      }
      if (token->src.ptr[0] == ')') {
        size_t depth = parser->stack_depth;
        grn_ts_str src;
        grn_ts_expr_dummy_token *dummy_token;
        if (ex_ex_token->src.ptr[0] != '(') {
          GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence");
        }
        src.ptr = ex_ex_token->src.ptr;
        src.size = (token->src.ptr + token->src.size) - src.ptr;
        dummy_token = &parser->dummy_tokens[parser->n_dummy_tokens++];
        GRN_TS_DEBUG("dummy token: \"%.*s\"", (int)src.size, src.ptr);
        grn_ts_expr_dummy_token_init(ctx, dummy_token, src);
        parser->stack[depth - 2] = dummy_token;
        parser->stack_depth--;
        // TODO: Apply a function.
      } else if (token->src.ptr[0] == ']') {
        size_t depth = parser->stack_depth;
        if (ex_ex_token->src.ptr[0] != '[') {
          GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "invalid token sequence");
        }
        parser->stack[depth - 2] = parser->stack[depth - 1];
        parser->stack_depth--;
        // TODO: Push a subscript operator.
      }
      return GRN_SUCCESS;
    }
    default: {
      GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "undefined bracket: \"%.*s\"",
                        (int)token->src.size, token->src.ptr);
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
      GRN_TS_ERR_RETURN(GRN_OBJECT_CORRUPT, "invalid token type: %d",
                        token->type);
    }
  }
}

/* grn_ts_expr_parser_analyze() analyzes tokens. */
static grn_rc
grn_ts_expr_parser_analyze(grn_ctx *ctx, grn_ts_expr_parser *parser) {
  size_t i;

  /* Reserve temporary work spaces. */
  parser->dummy_tokens = GRN_MALLOCN(grn_ts_expr_dummy_token,
                                     parser->n_tokens);
  if (!parser->dummy_tokens) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_MALLOCN failed: %zu x %zu",
                      sizeof(grn_ts_expr_dummy_token), parser->n_tokens);
  }
  parser->stack = GRN_MALLOCN(grn_ts_expr_token *, parser->n_tokens);
  if (!parser->stack) {
    GRN_TS_ERR_RETURN(GRN_NO_MEMORY_AVAILABLE, "GRN_MALLOCN failed: %zu x %zu",
                      sizeof(grn_ts_expr_token *), parser->n_tokens);
  }

  /* Analyze tokens. */
  for (i = 0; i < parser->n_tokens; i++) {
    grn_rc rc;
    rc = grn_ts_expr_parser_analyze_token(ctx, parser, parser->tokens[i]);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  if (parser->stack_depth != 2) {
    GRN_TS_ERR_RETURN(GRN_INVALID_FORMAT, "tokens left in stack: %zu",
                      parser->stack_depth);
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
  grn_ts_str str;
  rc = grn_ts_buf_reserve(ctx, &parser->str_buf, str_size + 1);
  if (rc != GRN_SUCCESS) {
    return rc;
  }
  grn_memcpy(parser->str_buf.ptr, str_ptr, str_size);
  ((char *)parser->str_buf.ptr)[str_size] = '\0';
  str.ptr = (const char *)parser->str_buf.ptr;
  str.size = str_size;
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
  size_t i, n_args;
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
  if (n_args > expr->stack_depth) {
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
  grn_obj *obj;
  grn_ts_expr_node *node;
  grn_ts_expr_bridge *bridge;
  if (!ctx) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!expr || (expr->type != GRN_TS_EXPR_INCOMPLETE) || !expr->stack_depth) {
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
