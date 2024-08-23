/*
  Copyright (C) 2009-2016  Brazil
  Copyright (C) 2020-2024  Sutou Kouhei <kou@clear-code.com>

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

/* Just for backward compatibility.
   Use grn_type_id_is_text_family() instead. */
#define GRN_TYPE_IS_TEXT_FAMILY(type) grn_type_id_is_text_family(NULL, (type))

GRN_API const char *
grn_type_id_to_string_builtin(grn_ctx *ctx, grn_id id);
GRN_API bool
grn_type_id_is_builtin(grn_ctx *ctx, grn_id id);
GRN_API bool
grn_type_id_is_number_family(grn_ctx *ctx, grn_id id);
GRN_API bool
grn_type_id_is_float_family(grn_ctx *ctx, grn_id id);
GRN_API bool
grn_type_id_is_text_family(grn_ctx *ctx, grn_id id);
GRN_API bool
grn_type_id_is_compatible(grn_ctx *ctx, grn_id id1, grn_id id2);
GRN_API size_t
grn_type_id_size(grn_ctx *ctx, grn_id id);

/**
 * \brief Define a new type in DB
 *
 * \param ctx The context object
 * \param name Name of type to create
 * \param name_size Length of the name of type to be created
 * \param flags \ref GRN_OBJ_KEY_VAR_SIZE, \ref GRN_OBJ_KEY_FLOAT,
 *              \ref GRN_OBJ_KEY_INT, \ref GRN_OBJ_KEY_UINT or
 *              \ref GRN_OBJ_KEY_GEO_POINT
 * \param size Maximum length if \ref GRN_OBJ_KEY_VAR_SIZE,
 *             otherwise length (in bytes)
 * \return A newly created type on success, `NULL` on error
 */
GRN_API grn_obj *
grn_type_create(grn_ctx *ctx,
                const char *name,
                unsigned int name_size,
                grn_obj_flags flags,
                unsigned int size);
GRN_API uint32_t
grn_type_size(grn_ctx *ctx, grn_obj *type);

#ifdef __cplusplus
}
#endif
