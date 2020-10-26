/*
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#pragma once

#ifdef  __cplusplus
extern "C" {
#endif

GRN_API grn_rc
grn_result_set_add_record(grn_ctx *ctx,
                          grn_hash *result_set,
                          grn_posting *posting,
                          grn_operator op);
GRN_API grn_rc
grn_result_set_add_table(grn_ctx *ctx,
                         grn_hash *result_set,
                         grn_obj *table,
                         double score,
                         grn_operator op);
GRN_API grn_rc
grn_result_set_add_table_cursor(grn_ctx *ctx,
                                grn_hash *result_set,
                                grn_table_cursor *cursor,
                                double score,
                                grn_operator op);
GRN_API grn_rc
grn_result_set_add_index_cursor(grn_ctx *ctx,
                                grn_hash *result_set,
                                grn_obj *cursor,
                                double additional_score,
                                double weight,
                                grn_operator op);
GRN_API grn_rc
grn_result_set_add_ii_cursor(grn_ctx *ctx,
                             grn_hash *result_set,
                             grn_ii_cursor *cursor,
                             double additional_score,
                             double weight,
                             grn_operator op);

#ifdef __cplusplus
}
#endif
