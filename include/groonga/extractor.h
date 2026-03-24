/*
  Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>

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

#include <groonga/tokenizer.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \brief This is an opaque data to pass data to an extractor from
 *        Groonga.
 *
 * This is used for \ref grn_extractor_extract_func.
 *
 * \see grn_extractor_set_extract_func
 *
 * \since 16.0.1
 */
typedef struct grn_extract_data grn_extract_data;

/**
 * \brief Return the extract target value.
 *
 * This is owned by \ref grn_extract_data. You must not free it.
 *
 * \param ctx The context object.
 * \param data The data.
 *
 * \return The extract target value.
 *
 * \since 16.0.1
 */
GRN_PLUGIN_EXPORT grn_obj *
grn_extract_data_get_value(grn_ctx *ctx, grn_extract_data *data);

/**
 * \brief Return the table that extracts target value belongs to.
 *
 * \param ctx The context object.
 * \param data The data.
 *
 * \return The table that extract target value belongs to.
 *
 * \since 16.0.1
 */
GRN_PLUGIN_EXPORT grn_obj *
grn_extract_data_get_table(grn_ctx *ctx, grn_extract_data *data);

/**
 * \brief Return the index of the target extractor in the table.
 *
 * \param ctx The context object.
 * \param data The data.
 *
 * \return The index of the target extractor in the table.
 *
 * \since 16.0.1
 */
GRN_PLUGIN_EXPORT uint32_t
grn_extract_data_get_index(grn_ctx *ctx, grn_extract_data *data);

/**
 * \brief A function that implements extraction feature for an
 *        extractor.
 *
 * If the `extractor` can extract a content from the `data`, the
 * `extractor` must extract a content from the `data` and return it as
 * a \ref grn_obj that is created by \ref grn_obj_open.
 *
 * If the `extractor` can't extract a content from the `data`, the
 * `extractor` must return `NULL`.
 *
 * \param ctx The context object.
 * \param extractor The extractor.
 * \param data The data to be extracted.
 *
 * \return The extracted value as a \ref grn_obj, `NULL` if the extractor
 *         doesn't extract anything or has any error.
 *
 *         You must free the returned value by \ref GRN_OBJ_FIN when
 *         it's no longer needed.
 *
 * \since 16.0.1
 */
typedef grn_obj *
grn_extractor_extract_func(grn_ctx *ctx,
                           grn_obj *extractor,
                           grn_extract_data *data);

/**
 * \brief Create an extractor.
 *
 * You must set at least an extract function to the created extractor
 * by \ref grn_extractor_set_extract_func.
 *
 * \param ctx The context object.
 * \param name The name of the new extractor.
 * \param name_length The byte size of `name`.You can use `-1` if
 *                    `name` is a `\0`-terminated string.
 *
 * \return The next extractor.
 *
 * \since 16.0.1
 */
GRN_PLUGIN_EXPORT grn_obj *
grn_extractor_create(grn_ctx *ctx, const char *name, int name_length);

/**
 * \brief Set an extract function to an extractor.
 *
 * \param ctx The context object.
 * \param extractor The extractor.
 * \param extract The function to extract a content.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 16.0.1
 */
GRN_PLUGIN_EXPORT grn_rc
grn_extractor_set_extract_func(grn_ctx *ctx,
                               grn_obj *extractor,
                               grn_extractor_extract_func *extract);

#ifdef __cplusplus
}
#endif
