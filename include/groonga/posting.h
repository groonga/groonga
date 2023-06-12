/*
  Copyright(C) 2009-2018  Brazil
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

typedef struct {
  grn_id rid;
  uint32_t sid;
  uint32_t pos;
  uint32_t tf;
  uint32_t weight;
  uint32_t rest;
} grn_posting;

bool grn_posting_equal(grn_ctx *ctx,
                       grn_posting *posting1, grn_posting *posting2);

GRN_API grn_posting *grn_posting_open(grn_ctx *ctx);
GRN_API void grn_posting_close(grn_ctx *ctx, grn_posting *posting);

GRN_API grn_id grn_posting_get_record_id(grn_ctx *ctx, grn_posting *posting);
GRN_API uint32_t grn_posting_get_section_id(grn_ctx *ctx, grn_posting *posting);
GRN_API uint32_t grn_posting_get_position(grn_ctx *ctx, grn_posting *posting);
GRN_API uint32_t grn_posting_get_tf(grn_ctx *ctx, grn_posting *posting);
GRN_API uint32_t grn_posting_get_weight(grn_ctx *ctx, grn_posting *posting);
GRN_API float grn_posting_get_weight_float(grn_ctx *ctx, grn_posting *posting);
GRN_API uint32_t grn_posting_get_rest(grn_ctx *ctx, grn_posting *posting);

GRN_API void
grn_posting_set_weight(grn_ctx *ctx,
                       grn_posting *posting,
                       uint32_t weight);
GRN_API void
grn_posting_set_weight_float(grn_ctx *ctx,
                             grn_posting *posting,
                             float weight);

#ifdef __cplusplus
}
#endif
