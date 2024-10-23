/*
  Copyright(C) 2015-2018  Brazil
  Copyright(C) 2018-2022  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga/option.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  grn_obj *src;
  grn_obj *dest;
  uint32_t flags;
  grn_obj *target;
} grn_caster;

GRN_API grn_rc
grn_caster_cast(grn_ctx *ctx, grn_caster *caster);
/**
 * \brief Cast and append a value from the source bulk/vector to the destination
 *        bulk/vector.
 *
 * If `dest` is a reference type bulk/vector, `add_record_if_not_exist` is used.
 * Reference type bulk/vector means that type of `dest` is a table.
 * If `src` value doesn't exist in the table that is a type of `dest`. The `src`
 * value is added to the table.
 *
 * \param ctx The context object.
 * \param src The bulk object containing the value to be casted.
 * \param dest The bulk/vector object specifying the target type and where the
 *             casted value will be stored.
 * \param add_record_if_not_exist If `dest` is a reference type bulk/vector and
 *                                the `src` value doesn't exist in the target
 *                                table, setting this to true will add the `src`
 *                                value to the table.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_obj_cast(grn_ctx *ctx,
             grn_obj *src,
             grn_obj *dest,
             grn_bool add_record_if_not_exist);

#ifdef __cplusplus
}
#endif
