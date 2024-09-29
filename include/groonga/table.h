/*
  Copyright (C) 2009-2018  Brazil
  Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>

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

#define GRN_TABLE_MAX_KEY_SIZE (0x1000)

/**
 * \brief Define a new table in the DB used by ctx
 *
 * \param ctx The context object
 * \param name Name of the table to create. If `NULL`, it is an unnamed table.
 *             When creating a named table for a persistent db,
 *             GRN_OBJ_PERSISTENT must be specified in `flags`
 * \param name_size Length of `name`
 * \param path File path of the table to create.
 *             Enabled only if GRN_OBJ_PERSISTENT is specified in `flags`.
 *             If `NULL`, the file path is automatically assigned.
 * \param flags
 *    * GRN_OBJ_TABLE_NO_KEY: Array table
 *    * GRN_OBJ_TABLE_HASH_KEY: Hash table
 *    * GRN_OBJ_TABLE_PAT_KEY: Patricia trie
 *    * GRN_OBJ_KEY_WITH_SIS: All suffixes in the key string are automatically
 *      registered
 *    * GRN_OBJ_PERSISTENT: Persistent table
 *    * GRN_OBJ_KEY_NORMALIZE: Normalized string is the key
 * \param key_type Built-in type or table. If table B is created with table A
 *                 as key_type, B will always be a subset of A.
 *                 Disabled if GRN_OBJ_TABLE_NO_KEY is specified.
 * \param value_type Type of the value for key.
 *                   A table can have only one value for key, in addition to
 *                   column
 *
 * \return A newly created table on success, `NULL` on error
 */
GRN_API grn_obj *
grn_table_create(grn_ctx *ctx,
                 const char *name,
                 unsigned int name_size,
                 const char *path,
                 grn_table_flags flags,
                 grn_obj *key_type,
                 grn_obj *value_type);
GRN_API grn_obj *
grn_table_create_similar(grn_ctx *ctx,
                         const char *name,
                         uint32_t name_size,
                         const char *path,
                         grn_obj *base_table);

#define GRN_TABLE_OPEN_OR_CREATE(ctx,                                          \
                                 name,                                         \
                                 name_size,                                    \
                                 path,                                         \
                                 flags,                                        \
                                 key_type,                                     \
                                 value_type,                                   \
                                 table)                                        \
  (((table) = grn_ctx_get((ctx), (name), (name_size))) ||                      \
   ((table) = grn_table_create((ctx),                                          \
                               (name),                                         \
                               (name_size),                                    \
                               (path),                                         \
                               (flags),                                        \
                               (key_type),                                     \
                               (value_type))))
/**
 * \brief Add a new record with `key` to the table and return its ID.
 *        If `key` already exists in the table, returns the ID of the record.
 *        For tables with GRN_OBJ_TABLE_NO_KEY, `key` and `key_size` are ignored
 *
 * \param ctx The context object
 * \param table Target table
 * \param key Search key
 * \param key_size Length of `key`
 * \param added If a non `NULL` value is specified, 1 is set when a new record
 *              is added, and 0 is set when it is an existing record
 *
 * \return Record ID on success, GRN_ID_NIL on error
 *
 * \todo int *added -> bool *added
 */
GRN_API grn_id
grn_table_add(grn_ctx *ctx,
              grn_obj *table,
              const void *key,
              unsigned int key_size,
              int *added);

/**
 * \brief It finds a record that has `key` parameter and returns ID of the found
 *        record. If table parameter is a database, it finds an object (table,
 *        column and so on) that has key parameter and returns ID of the found
 *        object.
 *
 * \param ctx The context object
 * \param table The table or database
 * \param key The record or object key to be found
 * \param key_size Length of `key`
 *
 * \return ID of the found object on success, \ref GRN_ID_NIL on not found or
 *         error
 */
GRN_API grn_id
grn_table_get(grn_ctx *ctx,
              grn_obj *table,
              const void *key,
              unsigned int key_size);
/**
 * \brief Search the table for a record of ID and return the specified ID if it
 *        exists, or \ref GRN_ID_NIL if it does not
 *
 * \attention It is costly to perform. Do not call them frequently
 *
 * \param ctx The context object
 * \param table The table or database
 * \param id The ID to be found
 *
 * \return Specified ID on success, \ref GRN_ID_NIL on not found or error
 */
GRN_API grn_id
grn_table_at(grn_ctx *ctx, grn_obj *table, grn_id id);
/**
 * \brief Execute longest common prefix (LCP) search and return ID found.
 *
 * \details Execute longest common prefix search if the table is
 *          \ref GRN_TABLE_PAT_KEY or \ref GRN_TABLE_DAT_KEY.
 *          If the table is \ref GRN_TABLE_HASH_KEY, search by exact match.
 *
 * \param ctx The context object.
 * \param table The table.
 * \param key Search key.
 * \param key_size Length of `key`.
 *
 * \return ID on success, \ref GRN_ID_NIL on not found or error.
 */
GRN_API grn_id
grn_table_lcp_search(grn_ctx *ctx,
                     grn_obj *table,
                     const void *key,
                     unsigned int key_size);
/**
 * \brief Get the key assigned to the ID.
 *        If the size of the key found is larger than `buf_size`, it is not
 *        stored in `keybuf`.
 *
 * \param ctx The context object.
 * \param table The table.
 * \param id The ID to be found.
 * \param keybuf Buffer to store the record key.
 * \param buf_size Size of `keybuf` in bytes.
 *
 * \return Key size of the record on success, `0` on not existed.
 */
GRN_API int
grn_table_get_key(
  grn_ctx *ctx, grn_obj *table, grn_id id, void *keybuf, int buf_size);
/**
 * \brief Delete the record matching the key in the table.
 *
 * \param ctx The context object
 * \param table The table
 * \param key Key of record to be deleted
 * \param key_size Size of `key` in bytes
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if the record
 *         does not exist.
 */
GRN_API grn_rc
grn_table_delete(grn_ctx *ctx,
                 grn_obj *table,
                 const void *key,
                 unsigned int key_size);
/**
 * \brief Delete the record matching the ID in the table.
 *
 * \param ctx The context object
 * \param table The table
 * \param id ID of record to be deleted
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if the record
 *         does not exist.
 */
GRN_API grn_rc
grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id);
/**
 * \brief Update the key of the record matching ID in the table.
 *        This operation supports only \ref GRN_TABLE_DAT_KEY.
 *
 * \param ctx The context object.
 * \param table The \ref GRN_TABLE_DAT_KEY table.
 * \param id ID of record to be updated.
 * \param dest_key New key.
 * \param dest_key_size Size of `dest_key` in bytes.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_OPERATION_NOT_PERMITTED is returned if the
 *         table other than \ref GRN_TABLE_DAT_KEY is specified.
 */
GRN_API grn_rc
grn_table_update_by_id(grn_ctx *ctx,
                       grn_obj *table,
                       grn_id id,
                       const void *dest_key,
                       unsigned int dest_key_size);
/**
 * \brief Change the key of the record matching the `src_key` of the table.
 *        Specify the new key and its byte length in `dest_key` and
 *        `dest_key_size`. This operation is allowed only for table of
 *        type GRN_TABLE_DAT_KEY
 *
 * \param ctx The context object
 * \param table Target table
 * \param src_key Key of record to be updated
 * \param src_key_size Length of `src_key` (byte)
 * \param dest_key New key
 * \param dest_key_size Length of `dest_key_size` (byte)
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_OPERATION_NOT_PERMITTED is returned if not
 *         \ref GRN_TABLE_DAT_KEY.
 */
GRN_API grn_rc
grn_table_update(grn_ctx *ctx,
                 grn_obj *table,
                 const void *src_key,
                 unsigned int src_key_size,
                 const void *dest_key,
                 unsigned int dest_key_size);
/**
 * \brief Delete all records in table.
 *
 * \attention Do not use in multi-threading. Because it might access data that
 *            has already been deleted and crash when it does.
 *
 * \attention You need to reopen the target table and its columns in another
 *            process when you use this in multi-processing. This removes
 *            related files and creates new files. If you keep using the target
 *            table and its columns in another process, the target table and
 *            its columns in another process will touch removed data. It will
 *            cause a crash or something wrong.
 *
 * \param ctx The context object
 * \param table The table
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_table_truncate(grn_ctx *ctx, grn_obj *table);

#define GRN_CURSOR_ASCENDING   (0x00 << 0)
#define GRN_CURSOR_DESCENDING  (0x01 << 0)
#define GRN_CURSOR_GE          (0x00 << 1)
#define GRN_CURSOR_GT          (0x01 << 1)
#define GRN_CURSOR_LE          (0x00 << 2)
#define GRN_CURSOR_LT          (0x01 << 2)
#define GRN_CURSOR_BY_KEY      (0x00 << 3)
#define GRN_CURSOR_BY_ID       (0x01 << 3)
#define GRN_CURSOR_PREFIX      (0x01 << 4)
#define GRN_CURSOR_SIZE_BY_BIT (0x01 << 5)
#define GRN_CURSOR_RK          (0x01 << 6)

/**
 * \brief Creates and returns a cursor to retrieve the records registered in
 *        the table in order
 *
 * \param ctx The context object
 * \param table Target table
 * \param min Minimum limit of key (`NULL` means no minimum limit).
 *            See below for GRN_CURSOR_PREFIX
 * \param min_size Size of min. See below for GRN_CURSOR_PREFIX
 * \param max Maximum limit of key (`NULL` means no maximum limit).
 *            See below for GRN_CURSOR_PREFIX
 * \param max_size Size of max. GRN_CURSOR_PREFIX may be ignored
 * \param offset Extracts records from the range of records that meet the
 *               condition, starting with the `offset`-th (offset is
 *               zero-based). When GRN_CURSOR_PREFIX is specified, negative
 *               numbers cannot be specified
 * \param limit Only `limit` records in the range that meet the condition are
 *              extracted. -1 means all.
 *              When GRN_CURSOR_PREFIX is specified, negative numbers cannot
 *              be specified
 * \param flags
 *     * GRN_CURSOR_ASCENDING: Retrieve records in ascending order
 *       * If GRN_CURSOR_PREFIX is specified and a record with a near key is
 *         retrieved, or a common prefix search is used, it will be ignored
 *     * GRN_CURSOR_DESCENDING: Retrieve records in descending order
 *       * If GRN_CURSOR_PREFIX is specified and a record with a near key is
 *         retrieved, or a common prefix search is used, it will be ignored
 *     * GRN_CURSOR_GT: a key that matches `min` is not included in the cursor
 *       range
 *       * If `min` is `NULL`, or if GRN_CURSOR_PREFIX is specified and a record
 *         with a near key is retrieved, or a common prefix search is used,
 *         it will be ignored
 *     * GRN_CURSOR_LT: a key that matches `max` is not included in the cursor
 *       range
 *       * If `max` is `NULL`, or if GRN_CURSOR_PREFIX is specified, it will
 *         be ignored
 *     * GRN_CURSOR_BY_ID: Retrieves records in ID order
 *       * If GRN_CURSOR_PREFIX is specified, it will be ignored
 *     * GRN_CURSOR_BY_KEY: Retrieves records in key order
 *       * It can be used with table that specify GRN_OBJ_TABLE_PAT_KEY
 *       * For table with GRN_OBJ_TABLE_HASH_KEY or GRN_OBJ_TABLE_NO_KEY, it
 *         will be ignored
 *     * GRN_CURSOR_PREFIX: A cursor is created to retrieve the following
 *       records for the table with GRN_OBJ_TABLE_PAT_KEY
 *       * If `max` is `NULL`, retrieve the record for which key is a prefix
 *         match to `min`. `max_size` is ignored
 *       * If `max` and `max_size` are specified and the table key is of type
 *         `ShortText`, then a `max` and common prefix search is executed and
 *         records with a common prefix
 *         greater than or equal to min_size bytes are retrieved. `min` is
 *         ignored
 *       * If `max` and `max_size` are specified and the key of the table is a
 *         fixed-length type, records are retrieved sequentially from nodes that
 *         are near each other on the `max` and PAT tree.
 *         * But, records are not retrieved for nodes in the PAT tree of key for
 *           bits less than `min_size` bytes and corresponding to nodes on a
 *           different branch than `max`.
 *           Being near a position on the PAT tree is not the same as being near
 *           a key value.
 *           In this case, `max` must be as wide as or greater than the key size
 *           of the target table. `min` is ignored
 *     * GRN_CURSOR_BY_ID, GRN_CURSOR_BY_KEY and GRN_CURSOR_PREFIX cannot be
 *       specified at the same time.
 *     * In a table created with GRN_OBJ_TABLE_PAT_KEY, if GRN_CURSOR_PREFIX and
 *       GRN_CURSOR_RK are specified, retrieves records where key is a prefix
 *       matching a string of lower case alphabetic characters converted to
 *       half-width kana characters according to JIS X 4063:2000 (this standard
 *       is abolished).
 *       * Supports only GRN_ENC_UTF8
 *       * GRN_CURSOR_ASCENDING and GRN_CURSOR_DESCENDING are invalid.
 *         Records cannot be retrieved in ascending or descending order of key
 *         value
 *
 * \return  A newly opened table cursor on success, `NULL` on error
 */
GRN_API grn_table_cursor *
grn_table_cursor_open(grn_ctx *ctx,
                      grn_obj *table,
                      const void *min,
                      unsigned int min_size,
                      const void *max,
                      unsigned int max_size,
                      int offset,
                      int limit,
                      int flags);
/**
 * \brief Free the cursor created by grn_table_cursor_open()
 *
 * \param ctx The context object
 * \param tc Target cursor
 *
 * \return \ref GRN_SUCCESS on success, GRN_INVALID_ARGUMENT if `tc` is invalid
 */
GRN_API grn_rc
grn_table_cursor_close(grn_ctx *ctx, grn_table_cursor *tc);

/**
 * \brief Move the cursur forward to the next and return its record ID.
 *        Return GRN_ID_NIL when the end is reached
 *
 * \param ctx The context object
 * \param tc Target cursor
 *
 * \return Record ID on there is a next, GRN_ID_NIL on no next or on error
 */
GRN_API grn_id
grn_table_cursor_next(grn_ctx *ctx, grn_table_cursor *tc);

/**
 * \brief Sets the key of the current record in cursor to `key` parameter and
 *        returns its length
 *
 * \param ctx The context object
 * \param tc Target cursor
 * \param key A pointer to the key of the current record is set
 *
 * \return Length of `key`
 */
GRN_API int
grn_table_cursor_get_key(grn_ctx *ctx, grn_table_cursor *tc, void **key);

/**
 * \brief Sets the value of the current record in cursor to `value` parameter
 *        and returns its length
 *
 * \param ctx The context object
 * \param tc Target cursor
 * \param value A pointer to the value of the current record is set
 *
 * \return Length of `value`
 */
GRN_API int
grn_table_cursor_get_value(grn_ctx *ctx, grn_table_cursor *tc, void **value);
GRN_API uint32_t
grn_table_cursor_get_key_value(grn_ctx *ctx,
                               grn_table_cursor *tc,
                               void **key,
                               uint32_t *key_size,
                               void **value);
/**
 * \brief Set `value` to the current record according to `flags`
 *
 * \param ctx The context object
 * \param tc Table cursor
 * \param value Value to set
 * \param flags Available values:
 *      * \ref GRN_OBJ_SET
 *      * \ref GRN_OBJ_INCR
 *      * \ref GRN_OBJ_DECR
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if the current
 *         record for cursor does not exist.
 */
GRN_API grn_rc
grn_table_cursor_set_value(grn_ctx *ctx,
                           grn_table_cursor *tc,
                           const void *value,
                           int flags);
/**
 * \brief Delete the current record for cursor
 *
 * \param ctx The context object
 * \param tc Target cursor
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if the current
 *         record for cursor does not exist.
 */
GRN_API grn_rc
grn_table_cursor_delete(grn_ctx *ctx, grn_table_cursor *tc);
GRN_API size_t
grn_table_cursor_get_max_n_records(grn_ctx *ctx, grn_table_cursor *cursor);
/**
 * \brief Return the table of cursor
 *
 * \param ctx The context object
 * \param tc Target cursor
 *
 * \return The table of cursor on success, `NULL` on error
 */
GRN_API grn_obj *
grn_table_cursor_table(grn_ctx *ctx, grn_table_cursor *tc);

GRN_API grn_obj *
grn_index_cursor_open(grn_ctx *ctx,
                      grn_table_cursor *tc,
                      grn_obj *index,
                      grn_id rid_min,
                      grn_id rid_max,
                      int flags);
GRN_API grn_obj *
grn_index_cursor_get_index_column(grn_ctx *ctx, grn_obj *index_cursor);
GRN_API grn_rc
grn_index_cursor_set_term_id(grn_ctx *ctx,
                             grn_obj *index_cursor,
                             grn_id term_id);
GRN_API grn_rc
grn_index_cursor_set_section_id(grn_ctx *ctx,
                                grn_obj *index_cursor,
                                uint32_t section_id);
GRN_API uint32_t
grn_index_cursor_get_section_id(grn_ctx *ctx, grn_obj *index_cursor);
GRN_API grn_rc
grn_index_cursor_set_scale(grn_ctx *ctx, grn_obj *index_cursor, float scale);
GRN_API grn_rc
grn_index_cursor_set_scales(grn_ctx *ctx,
                            grn_obj *index_cursor,
                            float *scales,
                            size_t n_scales);
GRN_API grn_rc
grn_index_cursor_set_start_position(grn_ctx *ctx,
                                    grn_obj *index_cursor,
                                    uint32_t position);
GRN_API grn_rc
grn_index_cursor_reset_start_position(grn_ctx *ctx, grn_obj *index_cursor);
GRN_API uint32_t
grn_index_cursor_get_start_position(grn_ctx *ctx, grn_obj *index_cursor);
GRN_API grn_posting *
grn_index_cursor_next(grn_ctx *ctx, grn_obj *index_cursor, grn_id *term_id);

typedef grn_rc (*grn_table_cursor_foreach_func)(grn_ctx *ctx,
                                                grn_table_cursor *cursor,
                                                grn_id id,
                                                void *user_data);
GRN_API grn_rc
grn_table_cursor_foreach(grn_ctx *ctx,
                         grn_table_cursor *cursor,
                         grn_table_cursor_foreach_func func,
                         void *user_data);

#define GRN_TABLE_EACH(ctx,                                                    \
                       table,                                                  \
                       head,                                                   \
                       tail,                                                   \
                       id,                                                     \
                       key,                                                    \
                       key_size,                                               \
                       value,                                                  \
                       block)                                                  \
  do {                                                                         \
    (ctx)->errlvl = GRN_LOG_NOTICE;                                            \
    (ctx)->rc = GRN_SUCCESS;                                                   \
    if ((ctx)->seqno & 1) {                                                    \
      (ctx)->subno++;                                                          \
    } else {                                                                   \
      (ctx)->seqno++;                                                          \
    }                                                                          \
    if ((table) && grn_table_size(ctx, (table)) > 0) {                         \
      switch ((table)->header.type) {                                          \
      case GRN_TABLE_PAT_KEY:                                                  \
        GRN_PAT_EACH((ctx),                                                    \
                     (grn_pat *)(table),                                       \
                     (id),                                                     \
                     (key),                                                    \
                     (key_size),                                               \
                     (value),                                                  \
                     block);                                                   \
        break;                                                                 \
      case GRN_TABLE_DAT_KEY:                                                  \
        GRN_DAT_EACH((ctx),                                                    \
                     (grn_dat *)(table),                                       \
                     (id),                                                     \
                     (key),                                                    \
                     (key_size),                                               \
                     block);                                                   \
        break;                                                                 \
      case GRN_TABLE_HASH_KEY:                                                 \
        GRN_HASH_EACH((ctx),                                                   \
                      (grn_hash *)(table),                                     \
                      (id),                                                    \
                      (key),                                                   \
                      (key_size),                                              \
                      (value),                                                 \
                      block);                                                  \
        break;                                                                 \
      case GRN_TABLE_NO_KEY:                                                   \
        GRN_ARRAY_EACH((ctx),                                                  \
                       (grn_array *)(table),                                   \
                       (head),                                                 \
                       (tail),                                                 \
                       (id),                                                   \
                       (value),                                                \
                       block);                                                 \
        break;                                                                 \
      }                                                                        \
    }                                                                          \
    if ((ctx)->subno) {                                                        \
      (ctx)->subno--;                                                          \
    } else {                                                                   \
      (ctx)->seqno++;                                                          \
    }                                                                          \
  } while (0)

#define GRN_TABLE_EACH_BEGIN(ctx, table, cursor, id)                           \
  do {                                                                         \
    if ((table) && grn_table_size(ctx, (table)) > 0) {                         \
      grn_table_cursor *cursor;                                                \
      cursor = grn_table_cursor_open((ctx),                                    \
                                     (table),                                  \
                                     NULL,                                     \
                                     0,                                        \
                                     NULL,                                     \
                                     0,                                        \
                                     0,                                        \
                                     -1,                                       \
                                     GRN_CURSOR_ASCENDING);                    \
      if (cursor) {                                                            \
        grn_id id;                                                             \
        while ((id = grn_table_cursor_next((ctx), cursor))) {

#define GRN_TABLE_EACH_BEGIN_FLAGS(ctx, table, cursor, id, flags)              \
  do {                                                                         \
    if ((table) && grn_table_size(ctx, (table)) > 0) {                         \
      grn_table_cursor *cursor;                                                \
      cursor = grn_table_cursor_open((ctx),                                    \
                                     (table),                                  \
                                     NULL,                                     \
                                     0,                                        \
                                     NULL,                                     \
                                     0,                                        \
                                     0,                                        \
                                     -1,                                       \
                                     (flags));                                 \
      if (cursor) {                                                            \
        grn_id id;                                                             \
        while ((id = grn_table_cursor_next((ctx), cursor))) {

#define GRN_TABLE_EACH_BEGIN_MIN(ctx, table, cursor, id, min, min_size, flags) \
  do {                                                                         \
    if ((table) && grn_table_size(ctx, (table)) > 0) {                         \
      grn_table_cursor *cursor;                                                \
      cursor = grn_table_cursor_open((ctx),                                    \
                                     (table),                                  \
                                     (min),                                    \
                                     (min_size),                               \
                                     NULL,                                     \
                                     0,                                        \
                                     0,                                        \
                                     -1,                                       \
                                     (flags));                                 \
      if (cursor) {                                                            \
        grn_id id;                                                             \
        while ((id = grn_table_cursor_next((ctx), cursor))) {

#define GRN_TABLE_EACH_END(ctx, cursor)                                        \
  }                                                                            \
  grn_table_cursor_close((ctx), cursor);                                       \
  }                                                                            \
  }                                                                            \
  }                                                                            \
  while (0)

typedef struct _grn_table_sort_key grn_table_sort_key;
typedef unsigned char grn_table_sort_flags;

/**
 * \brief Sort in ascending order
 */
#define GRN_TABLE_SORT_ASC (0x00 << 0)
/**
 * \brief Sort in descending order
 */
#define GRN_TABLE_SORT_DESC (0x01 << 0)

struct _grn_table_sort_key {
  grn_obj *key;
  grn_table_sort_flags flags;
  int offset;
};

/**
 * \brief Sort the records in table and store the results in `result`
 *
 * \param ctx The context object
 * \param table Target table
 * \param offset Starting offset of the sorted record.
 *               The records are stored in order from the offset (zero-based).
 * \param limit Maximum number of records to be stored in `result`
 * \param result Table to store results
 * \param keys Array of sort keys
 *             * keys.key: You can specify either column of table, accessor of
 *               table, or proc
 *             * keys.flags: You can specify the next
 *               * \ref GRN_TABLE_SORT_ASC
 *               * \ref GRN_TABLE_SORT_DESC
 *             * keys.offset: Member for internal use
 * \param n_keys Number of elements in `keys` array
 *
 * \return Number of sorted records
 */
GRN_API int
grn_table_sort(grn_ctx *ctx,
               grn_obj *table,
               int offset,
               int limit,
               grn_obj *result,
               grn_table_sort_key *keys,
               int n_keys);

typedef struct _grn_table_group_result grn_table_group_result;
typedef uint32_t grn_table_group_flags;

#define GRN_TABLE_GROUP_CALC_COUNT (0x01 << 3)
#define GRN_TABLE_GROUP_CALC_MAX   (0x01 << 4)
#define GRN_TABLE_GROUP_CALC_MIN   (0x01 << 5)
#define GRN_TABLE_GROUP_CALC_SUM   (0x01 << 6)
/* Deprecated since 10.0.4. Use GRN_TABLE_GROUP_CALC_MEAN instead. */
#define GRN_TABLE_GROUP_CALC_AVG                       GRN_TABLE_GROUP_CALC_MEAN
#define GRN_TABLE_GROUP_CALC_MEAN                      (0x01 << 7)
#define GRN_TABLE_GROUP_CALC_AGGREGATOR                (0x01 << 8)
#define GRN_TABLE_GROUP_LIMIT                          (0x01 << 9)
#define GRN_TABLE_GROUP_KEY_VECTOR_EXPANSION_POWER_SET (0x01 << 10)

typedef struct _grn_table_group_aggregator grn_table_group_aggregator;

GRN_API grn_table_group_aggregator *
grn_table_group_aggregator_open(grn_ctx *ctx);
GRN_API grn_rc
grn_table_group_aggregator_close(grn_ctx *ctx,
                                 grn_table_group_aggregator *aggregator);
GRN_API grn_rc
grn_table_group_aggregator_set_output_column_name(
  grn_ctx *ctx,
  grn_table_group_aggregator *aggregator,
  const char *name,
  int32_t name_len);
GRN_API const char *
grn_table_group_aggregator_get_output_column_name(
  grn_ctx *ctx, grn_table_group_aggregator *aggregator, uint32_t *len);
GRN_API grn_rc
grn_table_group_aggregator_set_output_column_type(
  grn_ctx *ctx, grn_table_group_aggregator *aggregator, grn_obj *type);
GRN_API grn_obj *
grn_table_group_aggregator_get_output_column_type(
  grn_ctx *ctx, grn_table_group_aggregator *aggregator);
GRN_API grn_rc
grn_table_group_aggregator_set_output_column_flags(
  grn_ctx *ctx, grn_table_group_aggregator *aggregator, grn_column_flags flags);
GRN_API grn_column_flags
grn_table_group_aggregator_get_output_column_flags(
  grn_ctx *ctx, grn_table_group_aggregator *aggregator);
GRN_API grn_rc
grn_table_group_aggregator_set_expression(
  grn_ctx *ctx,
  grn_table_group_aggregator *aggregator,
  const char *expression,
  int32_t expression_len);
GRN_API const char *
grn_table_group_aggregator_get_expression(
  grn_ctx *ctx,
  grn_table_group_aggregator *aggregator,
  uint32_t *expression_len);

struct _grn_table_group_result {
  grn_obj *table;
  uint8_t key_begin;
  uint8_t key_end;
  int limit;
  grn_table_group_flags flags;
  grn_operator op;
  uint32_t max_n_subrecs;
  grn_obj *calc_target;
  grn_table_group_aggregator **aggregators;
  uint32_t n_aggregators;
};

/**
 * \brief Group the records in the table.
 *
 * \param ctx The context object.
 * \param table The table.
 * \param keys Array of keys for grouping.
 * \param n_keys Number of elements in `keys` array.
 * \param results Array to store the results of grouping.
 * \param n_results Number of elements in `results` array.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if `table` is
 *         `NULL`.
 */
GRN_API grn_rc
grn_table_group(grn_ctx *ctx,
                grn_obj *table,
                grn_table_sort_key *keys,
                int n_keys,
                grn_table_group_result *results,
                int n_results);
GRN_API grn_table_sort_key *
grn_table_group_keys_parse(grn_ctx *ctx,
                           grn_obj *table,
                           const char *raw_sort_keys,
                           int32_t raw_sort_keys_size,
                           uint32_t *n_keys);
/**
 * \brief Execute the set operation of `table1` and `table2` according to
 *        the specification of `op` and store the result into `res`.
 *
 * \attention The table specified in `res` will be destructed.
 *
 * \param ctx The context object.
 * \param table1 The table.
 * \param table2 The table.
 * \param res Specify `table1` or `table2`.
 * \param op Type of operation.
 *           - \ref GRN_OP_OR
 *           - \ref GRN_OP_AND
 *           - \ref GRN_OP_AND_NOT
 *           - \ref GRN_OP_ADJUST
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 *         For example, \ref GRN_INVALID_ARGUMENT is returned if `table1` is
 *         `NULL`.
 */
GRN_API grn_rc
grn_table_setoperation(grn_ctx *ctx,
                       grn_obj *table1,
                       grn_obj *table2,
                       grn_obj *res,
                       grn_operator op);
/**
 * \brief Delete duplicated records in `table1` and `table2`.
 *
 * \param ctx The context object.
 * \param table1 The table.
 * \param table2 The table.
 * \param res1 Specify `table1`.
 * \param res2 Specify `table2`.
 *
 * \return \ref GRN_SUCCESS on success, \ref GRN_INVALID_ARGUMENT on
 *         `table1 != res1 || table2 != res2`.
 */
GRN_API grn_rc
grn_table_difference(
  grn_ctx *ctx, grn_obj *table1, grn_obj *table2, grn_obj *res1, grn_obj *res2);
/**
 * \brief Store the column IDs of the table starting with `name` in `res`.
 *
 * \param ctx The context object.
 * \param table The table.
 * \param name The column name prefix.
 * \param name_size Size of `name` in bytes. If `0`, you get all column IDs.
 * \param res The \ref GRN_TABLE_HASH_KEY table to store results.
 *
 * \return The number of IDs.
 */
GRN_API int
grn_table_columns(grn_ctx *ctx,
                  grn_obj *table,
                  const char *name,
                  unsigned int name_size,
                  grn_obj *res);

/**
 * \brief Return the number of records registered in the table
 *
 * \param ctx The context object
 * \param table The table or database
 *
 * \return The number of records
 */
GRN_API unsigned int
grn_table_size(grn_ctx *ctx, grn_obj *table);

/**
 *
 * \brief Rename table to `name`. All columns of the table are renamed at the
 *        same time. The table must be a persistent object.
 *
 * \param ctx The context object
 * \param table Target table
 * \param name New name
 * \param name_size Size of `name` in bytes
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error
 */
GRN_API grn_rc
grn_table_rename(grn_ctx *ctx,
                 grn_obj *table,
                 const char *name,
                 unsigned int name_size);

GRN_API grn_obj *
grn_table_select(grn_ctx *ctx,
                 grn_obj *table,
                 grn_obj *expr,
                 grn_obj *result_set,
                 grn_operator op);

GRN_API grn_table_sort_key *
grn_table_sort_key_from_str(grn_ctx *ctx,
                            const char *str,
                            unsigned int str_size,
                            grn_obj *table,
                            uint32_t *nkeys);
GRN_API grn_table_sort_key *
grn_table_sort_keys_parse(grn_ctx *ctx,
                          grn_obj *table,
                          const char *raw_sort_keys,
                          int32_t raw_sort_keys_size,
                          uint32_t *n_keys);
GRN_API grn_rc
grn_table_sort_key_close(grn_ctx *ctx,
                         grn_table_sort_key *keys,
                         uint32_t nkeys);

GRN_API bool
grn_table_is_grouped(grn_ctx *ctx, grn_obj *table);

GRN_API unsigned int
grn_table_max_n_subrecs(grn_ctx *ctx, grn_obj *table);

GRN_API grn_obj *
grn_table_create_for_group(grn_ctx *ctx,
                           const char *name,
                           unsigned int name_size,
                           const char *path,
                           grn_obj *group_key,
                           grn_obj *value_type,
                           uint32_t max_n_subrecs);

GRN_API unsigned int
grn_table_get_subrecs(grn_ctx *ctx,
                      grn_obj *table,
                      grn_id id,
                      grn_id *subrecbuf,
                      int *scorebuf,
                      int buf_size);

GRN_API grn_obj *
grn_table_tokenize(grn_ctx *ctx,
                   grn_obj *table,
                   const char *str,
                   unsigned int str_len,
                   grn_obj *buf,
                   bool addp);

GRN_API grn_rc
grn_table_apply_expr(grn_ctx *ctx,
                     grn_obj *table,
                     grn_obj *output_column,
                     grn_obj *expr);

GRN_API grn_id
grn_table_find_reference_object(grn_ctx *ctx, grn_obj *table);

GRN_API grn_rc
grn_table_copy(grn_ctx *ctx, grn_obj *from, grn_obj *to);

GRN_API grn_rc
grn_table_get_duplicated_keys(grn_ctx *ctx,
                              grn_obj *table,
                              grn_obj **duplicated_keys);
GRN_API bool
grn_table_have_duplicated_keys(grn_ctx *ctx, grn_obj *table);

GRN_API bool
grn_table_have_tokenizer(grn_ctx *ctx, grn_obj *table);

typedef struct _grn_table_selector grn_table_selector;

GRN_API grn_table_selector *
grn_table_selector_open(grn_ctx *ctx,
                        grn_obj *table,
                        grn_obj *expr,
                        grn_operator op);
GRN_API grn_rc
grn_table_selector_close(grn_ctx *ctx, grn_table_selector *table_selector);
GRN_API grn_id
grn_table_selector_get_min_id(grn_ctx *ctx, grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_min_id(grn_ctx *ctx,
                              grn_table_selector *table_selector,
                              grn_id min_id);
GRN_API bool
grn_table_selector_get_use_sequential_scan(grn_ctx *ctx,
                                           grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_use_sequential_scan(grn_ctx *ctx,
                                           grn_table_selector *table_selector,
                                           bool use);
GRN_API float
grn_table_selector_get_weight_factor(grn_ctx *ctx,
                                     grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_weight_factor(grn_ctx *ctx,
                                     grn_table_selector *table_selector,
                                     float factor);
GRN_API double
grn_table_selector_get_enough_filtered_ratio(
  grn_ctx *ctx, grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_enough_filtered_ratio(grn_ctx *ctx,
                                             grn_table_selector *table_selector,
                                             double ratio);
GRN_API int64_t
grn_table_selector_get_max_n_enough_filtered_records(
  grn_ctx *ctx, grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_max_n_enough_filtered_records(
  grn_ctx *ctx, grn_table_selector *table_selector, int64_t n);
GRN_API bool
grn_table_selector_get_ensure_using_select_result(
  grn_ctx *ctx, grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_ensure_using_select_result(
  grn_ctx *ctx, grn_table_selector *table_selector, bool use);
GRN_API uint32_t
grn_table_selector_get_fuzzy_max_distance(grn_ctx *ctx,
                                          grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_fuzzy_max_distance(grn_ctx *ctx,
                                          grn_table_selector *table_selector,
                                          uint32_t distance);
GRN_API uint32_t
grn_table_selector_get_fuzzy_max_expansions(grn_ctx *ctx,
                                            grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_fuzzy_max_expansions(grn_ctx *ctx,
                                            grn_table_selector *table_selector,
                                            uint32_t expansions);
GRN_API uint32_t
grn_table_selector_get_fuzzy_prefix_length(grn_ctx *ctx,
                                           grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_fuzzy_prefix_length(grn_ctx *ctx,
                                           grn_table_selector *table_selector,
                                           uint32_t length);
GRN_API float
grn_table_selector_get_fuzzy_max_distance_ratio(
  grn_ctx *ctx, grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_fuzzy_max_distance_ratio(
  grn_ctx *ctx, grn_table_selector *table_selector, float ratio);
GRN_API bool
grn_table_selector_get_fuzzy_with_transposition(
  grn_ctx *ctx, grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_fuzzy_with_transposition(
  grn_ctx *ctx, grn_table_selector *table_selector, bool with);
GRN_API bool
grn_table_selector_get_fuzzy_tokenize(grn_ctx *ctx,
                                      grn_table_selector *table_selector);
GRN_API grn_rc
grn_table_selector_set_fuzzy_tokenize(grn_ctx *ctx,
                                      grn_table_selector *table_selector,
                                      bool tokenize);
GRN_API grn_obj *
grn_table_selector_select(grn_ctx *ctx,
                          grn_table_selector *table_selector,
                          grn_obj *result_set);

#ifdef __cplusplus
}
#endif
