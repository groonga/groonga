/*
  Copyright(C) 2009-2018 Brazil

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

typedef struct _grn_db_create_optarg grn_db_create_optarg;

struct _grn_db_create_optarg {
  char **builtin_type_names;
  int n_builtin_type_names;
};

/**
 * \brief Create new database.
 *
 * \param ctx The context object.
 * \param path File path of the database. Temporary database if `NULL` is
 *        specified.
 * \param optarg Currently, it is not used. It is just ignored.
 *
 * \return The database on success, `NULL` on error.
 */
GRN_API grn_obj *
grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg);

#define GRN_DB_OPEN_OR_CREATE(ctx, path, optarg, db)                           \
  (((db) = grn_db_open((ctx), (path))) ||                                      \
   (db = grn_db_create((ctx), (path), (optarg))))

/**
 * \brief Open the database.
 *
 * \param ctx The context object.
 * \param path File path of the database.
 *
 * \return The database on success, `NULL` on error.
 */
GRN_API grn_obj *
grn_db_open(grn_ctx *ctx, const char *path);
/**
 * \brief Update the last modified time of db to the current time.
 *        The last modified time is used, for example, to verify whether the
 *        cache is valid or not.
 *
 * \param ctx The context object.
 * \param db The database.
 */
GRN_API void
grn_db_touch(grn_ctx *ctx, grn_obj *db);
/**
 * \brief Check the passed database and recover it if it is broken and it
 *        can be recovered.
 *
 * This API uses lock existence for checking whether the database is
 * broken or not.
 *
 * Here are recoverable cases:
 *
 *   - Index column is broken. The index column must have source column.
 *
 * Here are unrecoverable cases:
 *
 *   - Object name management feature is broken.
 *   - Table is broken.
 *   - Data column is broken.
 *
 * Object name management feature is used for managing table name,
 * column name and so on. If the feature is broken, the database can't
 * be recovered. Please re-create the database from backup.
 *
 * Table and data column can be recovered manually by removing an existence
 * lock and re-add data.
 *
 * \attention This is a dangerous API. You must not use this API when other
 *            thread or process opens the target database. If you use this API
 *            against shared database, the database may be broken.
 *
 * \since 4.0.9
 *
 * \param ctx The context object.
 * \param db The database to be recovered.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_db_recover(grn_ctx *ctx, grn_obj *db);
/**
 * \brief Unmaps all opened tables and columns in the passed database.
 *        Resources used by these opened tables and columns are freed.
 *        Normally, this API is needless. Because resources used by opened
 *        tables and columns are managed by OS automatically.
 *
 * \attention This is a thread unsafe API. You can't touch the database while
 *            this API is running.
 *
 * \since 5.0.7
 *
 * \param ctx The context object.
 * \param db The database to be unmaped.
 *
 * \return \ref GRN_SUCCESS on success, the appropriate \ref grn_rc on error.
 */
GRN_API grn_rc
grn_db_unmap(grn_ctx *ctx, grn_obj *db);
GRN_API uint32_t
grn_db_get_last_modified(grn_ctx *ctx, grn_obj *db);
GRN_API bool
grn_db_is_dirty(grn_ctx *ctx, grn_obj *db);
GRN_API grn_rc
grn_db_set_cache(grn_ctx *ctx, grn_obj *db, grn_cache *cache);
GRN_API grn_cache *
grn_db_get_cache(grn_ctx *ctx, grn_obj *db);

#define GRN_DB_EACH_BEGIN_FLAGS(ctx, cursor, id, flags)                        \
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, grn_ctx_db((ctx)), cursor, id, flags)

#define GRN_DB_EACH_BEGIN_BY_ID(ctx, cursor, id)                               \
  GRN_DB_EACH_BEGIN_FLAGS(ctx,                                                 \
                          cursor,                                              \
                          id,                                                  \
                          GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING)

#define GRN_DB_EACH_BEGIN_BY_KEY(ctx, cursor, id)                              \
  GRN_DB_EACH_BEGIN_FLAGS(ctx,                                                 \
                          cursor,                                              \
                          id,                                                  \
                          GRN_CURSOR_BY_KEY | GRN_CURSOR_ASCENDING)

#define GRN_DB_EACH_END(ctx, cursor) GRN_TABLE_EACH_END(ctx, cursor)

#ifdef __cplusplus
}
#endif
