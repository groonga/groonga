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
 * \note If \p obj is a \ref GRN_TABLE_PAT_KEY table, all keys are shown. To
 *       limit output on \ref GRN_TABLE_PAT_KEY tables, use
 *       \ref grn_inspect_limited instead.
 *
 * For example usage:
 * ```c
 * grn_obj inspected;
 * GRN_TEXT_INIT(&inspected, 0);
 * grn_inspect(ctx, &inspected, obj);
 * printf("%.*s\n", (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
 * GRN_OBJ_FIN(ctx, &inspected);
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
/**
 * \brief Inspect specified object and produce an indented textual
 *        representation.
 *
 * \since 4.0.8
 *
 * This function combines \ref grn_inspect and \ref grn_inspect_indent. It first
 * inspects the given \p obj into a temporary buffer, then prepends the given
 * \p indent string to each line of that inspected text and stores the result in
 * \p buffer.
 *
 * \note The \p indent string is only applied when the inspected text contains
 *       one or more newline characters (when the output spans multiple lines).
 *
 * For example usage:
 * ```c
 * grn_obj inspected;
 * GRN_TEXT_INIT(&inspected, 0);
 * grn_inspect_indented(ctx, &inspected, obj, "***");
 * printf("%.*s\n", (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
 * GRN_OBJ_FIN(ctx, &inspected);
 * ```
 *
 * If \p obj is \ref GRN_TABLE_PAT_KEY table, it prints like the following.
 * ```
 * ***#<table:pat Users key:ShortText value:(nil) size:7 columns:[] \
 *      default_tokenizer:(nil) normalizer:(nil) \
 *      keys:["a", "b", "c", "d", "e", "f", "g"] subrec:none nodes:{
 * ***4{0,5,0}
 * ***  L:2{0,6,0}
 * ***    L:1{0,7,0}
 * ***      L:0{0,0,0}
 * ***      R:1{0,7,0}("a")[01100001]
 * ```
 *
 * \param ctx The context object.
 * \param buffer The buffer where the indented inspected text will be stored.
 * \param obj The target object to inspect.
 * \param indent The indentation string to prepend to each line of output.
 *
 * \return The indented inspected object in text.
 */
GRN_API grn_obj *
grn_inspect_indented(grn_ctx *ctx,
                     grn_obj *buffer,
                     grn_obj *obj,
                     const char *indent);
/**
 * \brief Inspect specified object and produce a length-limited textual
 *        representation.
 *
 * \since 7.0.0
 *
 * This function inspects \p obj into \p buffer, but if the full representation
 * exceeds half of \c GRN_CTX_MSGSIZE, the output is truncated to that length,
 * and "...(original length)" is appended to indicate the total size.
 *
 * \note If the inspected text length is greater than \c GRN_CTX_MSGSIZE/2,
 *       it will be truncated otherwise it is copied in full.
 *
 * For example usage:
 * ```c
 * grn_obj inspected;
 * GRN_TEXT_INIT(&inspected, 0);
 * grn_inspect_limited(ctx, &inspected, obj);
 * printf("#=> %.*s\n",
 *        (int)GRN_TEXT_LEN(&inspected),
 *        GRN_TEXT_VALUE(&inspected));
 * GRN_OBJ_FIN(ctx, &buffer);
 * ```
 *
 * If \p obj is \ref GRN_TABLE_PAT_KEY table, it prints truncated result as
 * follows.
 * ```
 * #<table:pat Users key:ShortText value:(nil) size:7 columns:[] de...(502)
 * ```
 *
 * \param ctx The context object.
 * \param buffer The buffer where the truncated inspected text will be stored.
 * \param obj The target object to inspect.
 *
 * \return The truncated inspected object in text.
 */
GRN_API grn_obj *
grn_inspect_limited(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj);
/**
 * \brief Inspect specified object and return the name of the specified object.
 *
 * This function inspects the given \p obj and writes its name into \p buffer.
 * If \p obj has a name, that name is copied into \p buffer.
 * Otherwise:
 * - If \p obj's ID is a \ref GRN_ID_NIL, the `(nil)` is stored in \p buffer.
 * - If \p obj exists but has no name, the `(anonymous:ID)` is stored.
 *
 * For example usage:
 * ```c
 * grn_obj name;
 * GRN_TEXT_INIT(&name, 0);
 * grn_inspect_name(ctx, &name, obj);
 * printf("%.*s\n", (int)GRN_TEXT_LEN(&name),
 *        GRN_TEXT_VALUE(&name));
 * GRN_OBJ_FIN(ctx, &name);
 * ```
 *
 * For example output:
 * Depending on \p obj, it will output one of the following lines:
 * ```
 * Users
 * (nil)
 * (anonymous:42)
 * ```
 *
 * \param ctx The context object.
 * \param buffer The buffer where the inspected name will be stored.
 * \param obj The target object whose name is to be inspected.
 *
 * \return The inspected object's name in text.
 */
GRN_API grn_obj *
grn_inspect_name(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj);
/**
 * \brief Inspect the given encoding and produce its name.
 *
 * \since 4.0.8
 *
 * For example usage:
 * ```c
 * grn_obj encoding;
 * GRN_TEXT_INIT(&encoding, 0);
 * grn_inspect_encoding(ctx, &encoding, GRN_ENC_UTF8);
 * printf("%.*s\n", (int)GRN_TEXT_LEN(&encoding), GRN_TEXT_VALUE(&encoding));
 * GRN_OBJ_FIN(ctx, &encoding);
 * ```
 *
 * For example output:
 * Depending on \p encoding, it will output one of the following lines:
 * ```
 * default(UTF-8)
 * none
 * EUC-JP
 * UTF-8
 * Shift_JIS
 * Latin-1
 * KOI8-R
 * unknown(encoding)
 * ```
 *
 * \param ctx The context object.
 * \param buffer The buffer where the encoding name will be stored.
 * \param encoding The encoding to inspect. Must be one of:
 *                 - \ref GRN_ENC_DEFAULT
 *                 - \ref GRN_ENC_NONE
 *                 - \ref GRN_ENC_EUC_JP
 *                 - \ref GRN_ENC_UTF8
 *                 - \ref GRN_ENC_SJIS
 *                 - \ref GRN_ENC_LATIN1
 *                 - \ref GRN_ENC_KOI8R
 *
 * \return The inspected object's encoding name in text.
 */
GRN_API grn_obj *
grn_inspect_encoding(grn_ctx *ctx, grn_obj *buffer, grn_encoding encoding);
/**
 * \brief Inspect the given type and produce its name.
 *
 * \since 4.0.8
 *
 * For example usage:
 * ```c
 * grn_obj type_name;
 * GRN_TEXT_INIT(&type_name, 0);
 * grn_inspect_type(ctx, &type_name, obj->header.type);
 * printf("%.*s\n",
 *        (int)GRN_TEXT_LEN(&type_name),
 *        GRN_TEXT_VALUE(&type_name));
 * GRN_OBJ_FIN(ctx, &type_name);
 * ```
 *
 * For example output:
 * Depending on \p type, it will output one of the following lines:
 *
 * Table types:
 * ```
 * GRN_TABLE_DAT_KEY
 * GRN_TABLE_HASH_KEY
 * GRN_TABLE_NO_KEY
 * GRN_TABLE_PAT_KEY
 * ```
 *
 * Column types:
 * ```
 * GRN_COLUMN_FIX_SIZE
 * GRN_COLUMN_INDEX
 * GRN_COLUMN_VAR_SIZE
 * ```
 *
 * Data container types:
 * ```
 * GRN_BULK
 * GRN_PTR
 * GRN_PVECTOR
 * GRN_UVECTOR
 * GRN_VECTOR
 * ```
 *
 * Cursor types:
 * ```
 * GRN_CURSOR_COLUMN_GEO_INDEX
 * GRN_CURSOR_COLUMN_INDEX
 * GRN_CURSOR_TABLE_DAT_KEY
 * GRN_CURSOR_TABLE_HASH_KEY
 * GRN_CURSOR_TABLE_NO_KEY
 * GRN_CURSOR_TABLE_PAT_KEY
 * ```
 *
 * Other object types:
 * ```
 * GRN_ACCESSOR
 * GRN_DB
 * GRN_EXPR
 * GRN_MSG
 * GRN_PATSNIP
 * GRN_PROC
 * GRN_QUERY
 * GRN_SNIP
 * GRN_STRING
 * GRN_TYPE
 * GRN_VOID
 * ```
 *
 * Unknown types:
 * ```
 * (unknown: 0x42)
 * ```
 * Where `0x42` is an example of the hexadecimal value for unknown types.
 *
 * \param ctx The context object.
 * \param buffer The buffer where the type name will be stored.
 * \param type The type to inspect. This is typically obtained from an
 *             \ref grn_obj_header::type.

 * \return The inspected object's type name in text.
 */
GRN_API grn_obj *
grn_inspect_type(grn_ctx *ctx, grn_obj *buffer, unsigned char type);
/**
 * \brief Inspect the given query log flags and produce their names.
 *
 * \since 7.0.4
 *
 * For example usage:
 * ```c
 * grn_obj flags;
 * GRN_TEXT_INIT(&flags, 0);
 * unsigned int current_flags = grn_query_logger_get_flags(ctx);
 * grn_inspect_query_log_flags(ctx, &flags, current_flags);
 * printf("%.*s\n",
 *        (int)GRN_TEXT_LEN(&flags),
 *        GRN_TEXT_VALUE(&flags));
 * GRN_OBJ_FIN(ctx, &flags);
 * ```
 *
 * For example output:
 * Depending on \p flags, it will output one of the following:
 * ```
 * NONE
 * CACHE
 * COMMAND
 * DESTINATION
 * RESULT_CODE
 * SCORE
 * SIZE
 * ```
 *
 * When multiple flags are set, they are displayed separated by "|".
 * ```
 * CACHE|COMMAND|DESTINATION|RESULT_CODE|SCORE|SIZE
 * ```
 *
 * Available flags:
 * - \ref GRN_QUERY_LOG_NONE
 * - \ref GRN_QUERY_LOG_CACHE
 * - \ref GRN_QUERY_LOG_COMMAND
 * - \ref GRN_QUERY_LOG_DESTINATION
 * - \ref GRN_QUERY_LOG_RESULT_CODE
 * - \ref GRN_QUERY_LOG_SCORE
 * - \ref GRN_QUERY_LOG_SIZE
 *
 * \param ctx The context object.
 * \param buffer The buffer where the flag names will be stored.
 * \param flags The query log flags to inspect.
 *
 * \return The names of the inspected query log flags as text.
 */
GRN_API grn_obj *
grn_inspect_query_log_flags(grn_ctx *ctx, grn_obj *buffer, unsigned int flags);
GRN_API grn_obj *
grn_inspect_key(grn_ctx *ctx,
                grn_obj *buffer,
                grn_obj *table,
                const void *key,
                uint32_t key_size);

/**
 * \brief Print inspected text of the given object.
 *
 * \since 4.0.8
 *
 * For example usage:
 * ```c
 * grn_p(ctx, obj);
 * ```
 *
 * If \p obj is a ShortText type, it prints output like:
 * ```
 * #<type ShortText size:4096 type:var_size>
 * ```
 *
 * \param ctx The context object.
 * \param obj The object to inspect and print.
 */
GRN_API void
grn_p(grn_ctx *ctx, grn_obj *obj);
/**
 * \brief Print inspected text of the given geo point.
 *
 * \since 4.0.8
 *
 * This function inspects the given \p point and prints the result directly to
 * stdout. It internally creates a WGS84 geo point object from the provided
 * \ref grn_geo_point structure and prints its inspected representation.
 *
 * For example usage:
 * ```c
 * grn_geo_point point;
 * point.latitude = ((40 * 60 * 60) + (42 * 60) + 46) * 1000;
 * point.longitude = ((-74 * 60 * 60) + (0 * 60) + 22) * 1000;
 * grn_p_geo_point(ctx, &point);
 * ```
 *
 * If \p point indicates New York City, it prints output like:
 * ```
 * [(524290,18) ((0, 8, 44, 290),(0, 0, 0, 18)) \
 *   [00000000 00000000 00000000 10000000 00000000 00000000 00000001 00001100]]
 * ```
 *
 * \param ctx The context object.
 * \param point The geo point to inspect and print.
 */
GRN_API void
grn_p_geo_point(grn_ctx *ctx, grn_geo_point *point);
/**
 * \brief Print inspected text of the given index column values.
 *
 * \since 4.0.8
 *
 * For example usage:
 * ```c
 * grn_p_ii_values(ctx, index_column);
 * ```
 *
 * If \p obj is an index column, it prints output like:
 * ```
 * [
 *   #<"!"
 *     elements:[
 *       {status:available, rid:1, sid:1, pos:0, tf:1, weight:0, rest:1},
 *       {status:available, rid:2, sid:1, pos:0, tf:1, weight:0, rest:1}
 *     ]
 *   >,
 *   ...
 # ]
 * ```
 *
 * \param ctx The context object.
 * \param obj The index column to inspect and print.
 */
GRN_API void
grn_p_ii_values(grn_ctx *ctx, grn_obj *obj);

#ifdef __cplusplus
}
#endif
