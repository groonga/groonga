/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#include "grn_expr.h"
#include "grn_output_columns.h"

grn_obj *
grn_output_columns_parse(grn_ctx *ctx,
                         grn_obj *table,
                         const char *raw_output_columns,
                         size_t raw_output_columns_size)
{
  grn_obj *output_columns = NULL;
  grn_obj *variable;

  GRN_API_ENTER;

  GRN_EXPR_CREATE_FOR_QUERY(ctx, table, output_columns, variable);
  if (ctx->rc == GRN_SUCCESS) {
    grn_expr_parse(ctx,
                   output_columns,
                   raw_output_columns,
                   raw_output_columns_size,
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_OUTPUT_COLUMNS);
  }

  GRN_API_RETURN(output_columns);
}
