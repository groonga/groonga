/*
  Copyright(C) 2015-2016 Brazil

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
  int latitude;
  int longitude;
} grn_geo_point;

GRN_API grn_rc grn_geo_select_in_rectangle(grn_ctx *ctx,
                                           grn_obj *index,
                                           grn_obj *top_left_point,
                                           grn_obj *bottom_right_point,
                                           grn_obj *res,
                                           grn_operator op);
GRN_API unsigned int grn_geo_estimate_size_in_rectangle(grn_ctx *ctx,
                                                        grn_obj *index,
                                                        grn_obj *top_left_point,
                                                        grn_obj *bottom_right_point);
/* Deprecated since 4.0.8. Use grn_geo_estimate_size_in_rectangle() instead. */
GRN_API int grn_geo_estimate_in_rectangle(grn_ctx *ctx,
                                          grn_obj *index,
                                          grn_obj *top_left_point,
                                          grn_obj *bottom_right_point);
GRN_API grn_obj *grn_geo_cursor_open_in_rectangle(grn_ctx *ctx,
                                                  grn_obj *index,
                                                  grn_obj *top_left_point,
                                                  grn_obj *bottom_right_point,
                                                  int offset,
                                                  int limit);
GRN_API grn_posting *grn_geo_cursor_next(grn_ctx *ctx, grn_obj *cursor);


GRN_API int grn_geo_table_sort(grn_ctx *ctx,
                               grn_obj *table,
                               int offset,
                               int limit,
                               grn_obj *result,
                               grn_obj *column,
                               grn_obj *geo_point);

#ifdef __cplusplus
}
#endif
