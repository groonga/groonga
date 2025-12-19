/*
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

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
 * \brief JSON parser that parses JSON text (\ref GRN_DB_TEXT) and
 * stores parsed JSON as \ref GRN_DB_JSON.
 *
 * \since 15.2.2
 */
typedef struct _grn_json_parser grn_json_parser;

/**
 * \brief Open a new JSON parser.
 *
 * \param ctx The context object.
 * \param input The JSON text.
 * \param output The parsed JSON.
 *
 * \return A newly created JSON parser on success, `NULL` on error.
 *
 * \since 15.2.2
 */
GRN_API grn_json_parser *
grn_json_parser_open(grn_ctx *ctx, grn_obj *input, grn_obj *output);

/**
 * \brief Close a JSON parser.
 *
 * \param ctx The context object.
 * \param parser The parser to close.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 15.2.2
 */
GRN_API grn_rc
grn_json_parser_close(grn_ctx *ctx, grn_json_parser *parser);

/**
 * \brief Parse the given JSON text.
 *
 * \param ctx The context object.
 * \param parser The parser.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 15.2.2
 */
GRN_API grn_rc
grn_json_parser_parse(grn_ctx *ctx, grn_json_parser *parser);

/**
 * \brief JSON reader that readers values from parses JSON (\ref
 * GRN_DB_JSON).
 *
 * \since 15.2.2
 */
typedef struct _grn_json_reader grn_json_reader;

/**
 * \brief Open a new JSON reader.
 *
 * \param ctx The context object.
 * \param json The parsed JSON.
 *
 * \return A newly created JSON reader on success, `NULL` on error.
 *
 * \since 15.2.2
 */
GRN_API grn_json_reader *
grn_json_reader_open(grn_ctx *ctx, grn_obj *json);

/**
 * \brief Close a JSON reader.
 *
 * \param ctx The context object.
 * \param reader The reader to close.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 15.2.2
 */
GRN_API grn_rc
grn_json_reader_close(grn_ctx *ctx, grn_json_reader *reader);

/**
 * \brief JSON value type.
 *
 * \since 15.2.2
 */
typedef enum {
  GRN_JSON_VALUE_UNKNOWN,
  GRN_JSON_VALUE_NULL,
  GRN_JSON_VALUE_FALSE,
  GRN_JSON_VALUE_TRUE,
  GRN_JSON_VALUE_INT64,
  GRN_JSON_VALUE_FLOAT,
  GRN_JSON_VALUE_ARRAY,
  GRN_JSON_VALUE_OBJECT,
} grn_json_value_type;

/**
 * \brief Move to the next value.
 *
 * The initial state doesn't focus on any value. You must call this at
 * least once. This returns \ref GRN_END_OF_DATA when all values are
 * processed. So you can use the following loop:
 *
 * ```c
 * grn_json_reader *reader = grn_json_reader_open(ctx, json);
 * if (!reader) {
 *   // Process error
 * }
 * while (true) {
 *   grn_rc rc = grn_json_reader_next(ctx, reader);
 *   if (rc == GRN_END_OF_DATA) {
 *     break;
 *   }
 *   if (rc != GRN_SUCCESS) {
 *     // Process error.
 *     break;
 *   }
 *   // Process the current value with grn_json_reader_get_type(),
 *   // grn_json_reader_get_value() and grn_json_reader_get_size().
 * }
 * grn_json_reader_close(ctx, reader);
 * ```
 *
 * \param ctx The context object.
 * \param reader The reader.
 *
 * \return \ref GRN_SUCCESS on success, \ref GRN_END_OF_DATA on all
 *         processed, the appropriate \ref grn_rc on error.
 *
 * \since 15.2.2
 */
GRN_API grn_rc
grn_json_reader_next(grn_ctx *ctx, grn_json_reader *reader);

/**
 * \brief Return the current value type.
 *
 * \param ctx The context object.
 * \param reader The reader.
 *
 * \return If the current value is `null`, \ref GRN_JSON_VALUE_NULL.
 *
 *         If the current value is `false`, \ref GRN_JSON_VALUE_FALSE.
 *
 *         If the current value is `true`, \ref GRN_JSON_VALUE_TRUE.
 *
 *         If the current value is an integer, \ref
 *         GRN_JSON_VALUE_INT64.
 *
 *         If the current value is a floating-point number, \ref
 *         GRN_JSON_VALUE_FLOAT.
 *
 *         If the current value is an array, \ref
 *         GRN_JSON_VALUE_ARRAY.
 *
 *         If the current value is an array, \ref
 *         GRN_JSON_VALUE_OBJECT.
 *
 *         Otherwise, \ref GRN_JSON_VALUE_UNKNOWN.
 *
 * \since 15.2.2
 */
GRN_API grn_json_value_type
grn_json_reader_get_type(grn_ctx *ctx, grn_json_reader *reader);

/**
 * \brief Return the current value.
 *
 * This is owned by the reader. Caller must not free it. The returned
 * value is invalid when you call \ref grn_json_reader_next. If you
 * want to use this after the next \ref grn_json_reader_next call, you
 * need to copy this by yourself.
 *
 * \param ctx The context object.
 * \param reader The reader.
 *
 * \return If the current value is `null`, \ref GRN_DB_VOID bulk.
 *
 *         If the current value is `false` or `true`, \ref GRN_DB_BOOL
 *         bulk.
 *
 *         If the current value is an integer, \ref GRN_DB_INT64 bulk.
 *
 *         If the current value is a floating-point number, \ref
 *         GRN_DB_FLOAT bulk.
 *
 *         Otherwise, `NULL`. You can get array elements and object
 *         members by calling \ref grn_json_reader_next.
 *
 * \since 15.2.2
 */
GRN_API grn_obj *
grn_json_reader_get_value(grn_ctx *ctx, grn_json_reader *reader);

/**
 * \brief Return the size of the current value.
 *
 * \param ctx The context object.
 * \param reader The reader.
 *
 * \return If the current value is an array, the number of elements.
 *
 *         If the current value is an object, the number of members.
 *
 *         Otherwise, `0`.
 *
 * \since 15.2.2
 */
GRN_API size_t
grn_json_reader_get_size(grn_ctx *ctx, grn_json_reader *reader);

/**
 * \brief Stringify parsed JSON.
 *
 * \param ctx The context object.
 * \param json The JSON to stringify.
 * \param buffer The output buffer.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on
 *         error.
 *
 * \since 15.2.2
 */
GRN_API grn_rc
grn_json_to_string(grn_ctx *ctx,
                   grn_obj *json,
                   grn_obj *buffer);

#ifdef __cplusplus
}
#endif
