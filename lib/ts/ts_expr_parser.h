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

#ifndef GRN_TS_EXPR_PARSER_H
#define GRN_TS_EXPR_PARSER_H

#include "ts_expr.h"
#include "ts_op.h"
#include "ts_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct grn_ts_expr_parser grn_ts_expr_parser;

/* grn_ts_expr_parser_open() creates a parser. */
grn_rc grn_ts_expr_parser_open(grn_ctx *ctx, grn_ts_expr *expr,
                               grn_ts_expr_parser **parser);

/* grn_ts_expr_parser_close() destroys a parser. */
void grn_ts_expr_parser_close(grn_ctx *ctx, grn_ts_expr_parser *parser);

/*
 * grn_ts_expr_parser_parse() parses a string and pushes nodes into an
 * expression.
 */
grn_rc grn_ts_expr_parser_parse(grn_ctx *ctx, grn_ts_expr_parser *parser,
                                const char *str_ptr, size_t str_size);

#ifdef __cplusplus
}
#endif

#endif /* GRN_TS_EXPR_PARSER_H */
