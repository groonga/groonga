/*
  Copyright (C) 2010-2017  Brazil
  Copyright (C) 2023  Sutou Kouhei <kou@clear-code.com>

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

/**
 * \brief Inspect specified object and produce a textual representation.
 *
 * \since 4.0.8
 *
 * \note If \p obj is a \ref GRN_TABLE_PAT_KEY table, all keys are shown. To
 *       limit output on \ref GRN_TABLE_PAT_KEY tables, use
 *       \ref grn_inspect_limited instead.
 *
 * For example usage:
 * ```c
 *  grn_obj inspected;
 *  GRN_TEXT_INIT(&inspected, 0);
 *  grn_inspect(ctx, &inspected, obj);
 *  printf("%.*s\n", (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
 * ```
 *
 * \param ctx The context object.
 * \param buffer The buffer where the inspected text will be stored.
 * \param obj The target object to inspect.
 *
 * \return The inspected object in text.
 */
GRN_API grn_obj *
grn_inspect(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj);
GRN_API grn_obj *
grn_inspect_indent(grn_ctx *ctx,
                   grn_obj *buffer,
                   grn_obj *content,
                   const char *indent);
GRN_API grn_obj *
grn_inspect_indented(grn_ctx *ctx,
                     grn_obj *buffer,
                     grn_obj *obj,
                     const char *indent);
GRN_API grn_obj *
grn_inspect_limited(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj);
GRN_API grn_obj *
grn_inspect_name(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj);
GRN_API grn_obj *
grn_inspect_encoding(grn_ctx *ctx, grn_obj *buffer, grn_encoding encoding);
GRN_API grn_obj *
grn_inspect_type(grn_ctx *ctx, grn_obj *buffer, unsigned char type);
GRN_API grn_obj *
grn_inspect_query_log_flags(grn_ctx *ctx, grn_obj *buffer, unsigned int flags);
GRN_API grn_obj *
grn_inspect_key(grn_ctx *ctx,
                grn_obj *buffer,
                grn_obj *table,
                const void *key,
                uint32_t key_size);

GRN_API void
grn_p(grn_ctx *ctx, grn_obj *obj);
GRN_API void
grn_p_geo_point(grn_ctx *ctx, grn_geo_point *point);
GRN_API void
grn_p_ii_values(grn_ctx *ctx, grn_obj *obj);

#ifdef __cplusplus
}
#endif
