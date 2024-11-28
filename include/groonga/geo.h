/*
  Copyright(C) 2015-2016  Brazil
  Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int32_t latitude;
  int32_t longitude;
} grn_geo_point;

/**
 * \brief Select records within a specified rectangular area defined by the
 *        `top_left_point` and `bottom_right_point` parameters.
 *
 * \param ctx The context object.
 * \param index The index column for `TokyoGeoPoint` or `WGS84GeoPoint` type.
 * \param top_left_point The top left point of the target rectangle. Its type
 *                       must be one of the followings.
 *                       - \ref GRN_DB_SHORT_TEXT
 *                       - \ref GRN_DB_TEXT
 *                       - \ref GRN_DB_LONG_TEXT
 *                       - \ref GRN_DB_TOKYO_GEO_POINT
 *                       - \ref GRN_DB_WGS84_GEO_POINT
 * \param bottom_right_point The bottom right point of the target rectangle. Its
 *                           type must be one of the followings.
 *                           - \ref GRN_DB_SHORT_TEXT
 *                           - \ref GRN_DB_TEXT
 *                           - \ref GRN_DB_LONG_TEXT
 *                           - \ref GRN_DB_TOKYO_GEO_POINT
 *                           - \ref GRN_DB_WGS84_GEO_POINT
 * \param res The table to store found record IDs. It must be \ref
 *            GRN_TABLE_HASH_KEY type table.
 * \param op The operator for matched records.
 *           - \ref GRN_OP_OR
 *           - \ref GRN_OP_AND
 *           - \ref GRN_OP_AND_NOT
 *           - \ref GRN_OP_ADJUST
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_geo_select_in_rectangle(grn_ctx *ctx,
                            grn_obj *index,
                            grn_obj *top_left_point,
                            grn_obj *bottom_right_point,
                            grn_obj *res,
                            grn_operator op);
/**
 * \brief Estimate the number of records within a specified rectangular area
 *        defined by the `top_left_point` and `bottom_right_point` parameters.
 *
 * \since 4.0.8
 *
 * \param ctx The context object.
 * \param index The index column for `TokyoGeoPoint` or `WGS84GeoPoint` type.
 * \param top_left_point The top left point of the target rectangle. Its type
 *                       must be one of the followings:
 *                       - \ref GRN_DB_SHORT_TEXT
 *                       - \ref GRN_DB_TEXT
 *                       - \ref GRN_DB_LONG_TEXT
 *                       - \ref GRN_DB_TOKYO_GEO_POINT
 *                       - \ref GRN_DB_WGS84_GEO_POINT
 * \param bottom_right_point The bottom right point of the target rectangle. Its
 *                           type must be one of the followings:
 *                           - \ref GRN_DB_SHORT_TEXT
 *                           - \ref GRN_DB_TEXT
 *                           - \ref GRN_DB_LONG_TEXT
 *                           - \ref GRN_DB_TOKYO_GEO_POINT
 *                           - \ref GRN_DB_WGS84_GEO_POINT
 *
 * \return The estimated number of records within the specified rectangle.
 *         Returns 0 if no records are found or if an error occurs.
 *         See `ctx->rc` for error details.
 */
GRN_API unsigned int
grn_geo_estimate_size_in_rectangle(grn_ctx *ctx,
                                   grn_obj *index,
                                   grn_obj *top_left_point,
                                   grn_obj *bottom_right_point);
/**
 * \deprecated Since 4.0.8. Use \ref grn_geo_estimate_size_in_rectangle instead.
 */
GRN_API int
grn_geo_estimate_in_rectangle(grn_ctx *ctx,
                              grn_obj *index,
                              grn_obj *top_left_point,
                              grn_obj *bottom_right_point);
/**
 * \brief Create and return a cursor to retrieve records within a specified
 *        rectangular area defined by the `top_left_point` and
 *        `bottom_right_point` parameters.
 *
 * \param ctx The context object.
 * \param index The index column for `TokyoGeoPoint` or `WGS84GeoPoint` type.
 * \param top_left_point The top-left point of the target rectangle. Its type
 *                       must be one of the following:
 *                       - \ref GRN_DB_SHORT_TEXT
 *                       - \ref GRN_DB_TEXT
 *                       - \ref GRN_DB_LONG_TEXT
 *                       - \ref GRN_DB_TOKYO_GEO_POINT
 *                       - \ref GRN_DB_WGS84_GEO_POINT
 * \param bottom_right_point The bottom-right point of the target rectangle. Its
 *                           type must be one of the following:
 *                           - \ref GRN_DB_SHORT_TEXT
 *                           - \ref GRN_DB_TEXT
 *                           - \ref GRN_DB_LONG_TEXT
 *                           - \ref GRN_DB_TOKYO_GEO_POINT
 *                           - \ref GRN_DB_WGS84_GEO_POINT
 * \param offset The starting position of the records to return (zero-based
 *               index).
 * \param limit The maximum number of records to return. `-1` means no limit.
 *
 * \return A newly opened cursor on success, `NULL` on error.
 *         See `ctx->rc` for error details.
 */
GRN_API grn_obj *
grn_geo_cursor_open_in_rectangle(grn_ctx *ctx,
                                 grn_obj *index,
                                 grn_obj *top_left_point,
                                 grn_obj *bottom_right_point,
                                 int offset,
                                 int limit);
/**
 * \brief Retrieve the next posting from the geo cursor and advance to the next.
 *
 * \param ctx The context object.
 * \param geo_cursor The geo cursor from which to retrieve the next posting.
 *
 * \return \ref grn_posting if the next posting is found, `NULL` otherwise.
 *         You don't need to free the returned \ref grn_posting.
 */
GRN_API grn_posting *
grn_geo_cursor_next(grn_ctx *ctx, grn_obj *cursor);

GRN_API int
grn_geo_table_sort(grn_ctx *ctx,
                   grn_obj *table,
                   int offset,
                   int limit,
                   grn_obj *result,
                   grn_obj *column,
                   grn_obj *geo_point);

#ifdef __cplusplus
}
#endif
