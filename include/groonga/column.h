/*
  Copyright(C) 2009-2017  Brazil
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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _grn_column_cache grn_column_cache;

/**
 * \brief Name of the pseudo column `_id`.
 *
 * The `_id` pseudo column serves as a unique identifier for each record in a
 * Groonga table. It is automatically defined for all tables upon creation and
 * is immutable as long as the record exists. The `_id` value ranges from 1 to
 * \ref GRN_ID_MAX. And it also typically increments by one for each new record
 * added. Once a record is deleted, its `_id` value becomes available for reuse.
 *
 * \since 3.1.1
 *
 * For usage examples, see \ref grn_obj_column.
 */
#define GRN_COLUMN_NAME_ID "_id"
/**
 * \brief Length of the pseudo `_id` column name.
 *
 * The \ref GRN_COLUMN_NAME_ID_LEN macro returns the byte size of the
 * \ref GRN_COLUMN_NAME_ID, excluding the null terminator.
 *
 * \since 3.1.1
 */
#define GRN_COLUMN_NAME_ID_LEN (sizeof(GRN_COLUMN_NAME_ID) - 1)
/**
 * \brief Name of the pseudo column `_key`.
 *
 * The `_key` pseudo column represents the primary key value of a record in a
 * Groonga table. It is defined only for tables that have a primary key. The
 * primary key value is unique within the table. It is also immutable except
 * \ref GRN_TABLE_DAT_KEY.
 *
 * \since 3.1.1
 *
 * For usage examples, see \ref grn_obj_column.
 */
#define GRN_COLUMN_NAME_KEY "_key"
/**
 * \brief Length of the pseudo `_key` column name.
 *
 * The \ref GRN_COLUMN_NAME_KEY_LEN macro returns the byte size of the
 * \ref GRN_COLUMN_NAME_KEY, excluding the null terminator.
 *
 * \since 3.1.1
 */
#define GRN_COLUMN_NAME_KEY_LEN (sizeof(GRN_COLUMN_NAME_KEY) - 1)
/**
 * \brief Name of the pseudo column `_value`.
 *
 * The `_value` pseudo column represents the value of a record in a
 * Groonga table. It is defined only for tables that have a `value_type`
 * specified. The value is mutable.
 *
 * \since 3.1.1
 *
 * For usage examples, see \ref grn_obj_column.
 */
#define GRN_COLUMN_NAME_VALUE "_value"
/**
 * \brief Length of the pseudo `_value` column name.
 *
 * The \ref GRN_COLUMN_NAME_VALUE_LEN macro returns the byte size of the
 * \ref GRN_COLUMN_NAME_VALUE, excluding the null terminator.
 *
 * \since 3.1.1
 */
#define GRN_COLUMN_NAME_VALUE_LEN (sizeof(GRN_COLUMN_NAME_VALUE) - 1)
/**
 * \brief Name of the pseudo column `_score`.
 *
 * The `_score` pseudo column represents the score value of each record.
 * It is defined only in tables that are generated as search results.
 * The score value is set during the execution of the search process,
 * but it can be freely modified.
 *
 * \since 3.1.1
 *
 * For usage examples, see \ref grn_obj_column.
 */
#define GRN_COLUMN_NAME_SCORE "_score"
/**
 * \brief Length of the pseudo `_score` column name.
 *
 * The \ref GRN_COLUMN_NAME_SCORE_LEN macro returns the byte size of the
 * \ref GRN_COLUMN_NAME_SCORE, excluding the null terminator.
 *
 * \since 3.1.1
 */
#define GRN_COLUMN_NAME_SCORE_LEN (sizeof(GRN_COLUMN_NAME_SCORE) - 1)
/**
 * \brief Name of the pseudo column `_nsubrecs`.
 *
 * The `_nsubrecs` pseudo column represents the number of records that had the
 * same grouping key value before grouping. It is defined only for tables that
 * are generated as search results from grouping (drilldown) operations. When a
 * grouping (drilldown) operation is performed, the number of records that had
 * the same grouping key value in the original table is recorded in the
 * `_nsubrecs` column of the result table.
 *
 * \since 3.1.1
 *
 * For usage examples, see \ref grn_obj_column.
 */
#define GRN_COLUMN_NAME_NSUBRECS "_nsubrecs"
/**
 * \brief Length of the pseudo `_nsubrecs` column name.
 *
 * The \ref GRN_COLUMN_NAME_NSUBRECS_LEN macro returns the byte size of
 * \ref GRN_COLUMN_NAME_NSUBRECS, excluding the null terminator.
 *
 * \since 3.1.1
 */
#define GRN_COLUMN_NAME_NSUBRECS_LEN (sizeof(GRN_COLUMN_NAME_NSUBRECS) - 1)
#define GRN_COLUMN_NAME_MAX          "_max"
#define GRN_COLUMN_NAME_MAX_LEN      (sizeof(GRN_COLUMN_NAME_MAX) - 1)
#define GRN_COLUMN_NAME_MIN          "_min"
#define GRN_COLUMN_NAME_MIN_LEN      (sizeof(GRN_COLUMN_NAME_MIN) - 1)
#define GRN_COLUMN_NAME_SUM          "_sum"
#define GRN_COLUMN_NAME_SUM_LEN      (sizeof(GRN_COLUMN_NAME_SUM) - 1)
/* Deprecated since 10.0.4. Use GRN_COLUMN_NAME_MEAN instead. */
#define GRN_COLUMN_NAME_AVG      "_avg"
#define GRN_COLUMN_NAME_AVG_LEN  (sizeof(GRN_COLUMN_NAME_AVG) - 1)
#define GRN_COLUMN_NAME_MEAN     "_mean"
#define GRN_COLUMN_NAME_MEAN_LEN (sizeof(GRN_COLUMN_NAME_MEAN) - 1)

/**
 * \brief Create a new column in a table.
 *
 * \param ctx The context object.
 * \param table The target table.
 * \param name The name of the column. The name must be unique within the table.
 *             Duplicate column names are not allowed in the same table.
 * \param name_size The size of the `name` parameter in bytes.
 * \param path The file path where the column will be stored. This parameter is
 *             only effective if the \ref GRN_OBJ_PERSISTENT flag is specified
 *             in `flags`. If you specify \ref GRN_OBJ_PERSISTENT and `NULL` for
 *             `path`, auto generated path is used. In general, you don't need
 *             to specify a path explicitly.
 * \param flags Available values:
 *              * **General flags:**
 *                * \ref GRN_OBJ_PERSISTENT
 *                * \ref GRN_OBJ_COLUMN_INDEX
 *                * \ref GRN_OBJ_COLUMN_SCALAR
 *                * \ref GRN_OBJ_COLUMN_VECTOR
 *                * \ref GRN_OBJ_COMPRESS_ZLIB
 *                * \ref GRN_OBJ_COMPRESS_LZ4
 *                * \ref GRN_OBJ_COMPRESS_ZSTD
 *              * **Index flags** (Only used with \ref GRN_OBJ_COLUMN_INDEX):
 *                * \ref GRN_OBJ_WITH_SECTION
 *                * \ref GRN_OBJ_WITH_WEIGHT
 *                * \ref GRN_OBJ_WITH_POSITION
 * \param type The type of the column values. You can specify a predefined type
 *             or a table.
 *
 * \return A newly created column object on success, `NULL` on error.
 *         See `ctx->rc` for error details.
 */
GRN_API grn_obj *
grn_column_create(grn_ctx *ctx,
                  grn_obj *table,
                  const char *name,
                  unsigned int name_size,
                  const char *path,
                  grn_column_flags flags,
                  grn_obj *type);
GRN_API grn_obj *
grn_column_create_similar(grn_ctx *ctx,
                          grn_obj *table,
                          const char *name,
                          uint32_t name_size,
                          const char *path,
                          grn_obj *base_column);

#define GRN_COLUMN_OPEN_OR_CREATE(ctx,                                         \
                                  table,                                       \
                                  name,                                        \
                                  name_size,                                   \
                                  path,                                        \
                                  flags,                                       \
                                  type,                                        \
                                  column)                                      \
  (((column) = grn_obj_column((ctx), (table), (name), (name_size))) ||         \
   ((column) = grn_column_create((ctx),                                        \
                                 (table),                                      \
                                 (name),                                       \
                                 (name_size),                                  \
                                 (path),                                       \
                                 (flags),                                      \
                                 (type))))

GRN_API grn_rc
grn_column_index_update(grn_ctx *ctx,
                        grn_obj *column,
                        grn_id id,
                        unsigned int section,
                        grn_obj *oldvalue,
                        grn_obj *newvalue);
GRN_API grn_obj *
grn_column_table(grn_ctx *ctx, grn_obj *column);
GRN_API grn_rc
grn_column_truncate(grn_ctx *ctx, grn_obj *column);

GRN_API grn_column_flags
grn_column_get_flags(grn_ctx *ctx, grn_obj *column);
GRN_API grn_column_flags
grn_column_get_missing_mode(grn_ctx *ctx, grn_obj *column);
GRN_API grn_column_flags
grn_column_get_invalid_mode(grn_ctx *ctx, grn_obj *column);

GRN_API grn_column_cache *
grn_column_cache_open(grn_ctx *ctx, grn_obj *column);
GRN_API void
grn_column_cache_close(grn_ctx *ctx, grn_column_cache *cache);
GRN_API void *
grn_column_cache_ref(grn_ctx *ctx,
                     grn_column_cache *cache,
                     grn_id id,
                     size_t *value_size);

GRN_API grn_rc
grn_column_copy(grn_ctx *ctx, grn_obj *from, grn_obj *to);

#ifdef __cplusplus
}
#endif
