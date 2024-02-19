/*
  Copyright (C) 2021-2024  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn_ctx.hpp"
#include "grn_ctx_impl.h"
#include "grn_dat.h"
#include "grn_db.h"
#include "grn_ii.h"
#include "grn_obj.h"
#include "grn_pat.h"
#include "grn_store.h"
#include "grn_table.h"
#include "grn_task_executor.hpp"
#include "grn_wal.h"

#include <utility>
#include <vector>

static const char *grn_db_wal_recover_tag = "[db][wal][recover]";

static void
grn_db_wal_recover_ensure_remove_by_id(grn_ctx *ctx, grn_db *db, grn_id id)
{
  if (grn_table_at(ctx, db->keys, id) != id) {
    return;
  }

  grn_obj_delete_by_id(ctx, (grn_obj *)db, id, true);

  char path[PATH_MAX];
  grn_obj_path_by_id(ctx, (grn_obj *)db, id, path);
  grn_io_remove_if_exist(ctx, path);
  grn_wal_remove(ctx, path, grn_db_wal_recover_tag);
  grn_strcat(path, PATH_MAX, ".c");
  grn_io_remove_if_exist(ctx, path);
}

static void
grn_db_wal_recover_remove_object(grn_ctx *ctx,
                                 grn_db *db,
                                 grn_obj *object,
                                 grn_id id)
{
  if (object) {
    if (id == GRN_ID_NIL) {
      id = DB_OBJ(object)->id;
    }
    grn_obj_clear_lock(ctx, object);
    grn_obj_remove(ctx, object);
  }
  grn_db_wal_recover_ensure_remove_by_id(ctx, db, id);
}

static void
grn_db_wal_recover_keys(grn_ctx *ctx, grn_db *db)
{
  const char *tag = grn_db_wal_recover_tag;
  const char *placeholder_prefix = "_PLACEHOLDER_";

  GRN_LOG(ctx, GRN_LOG_NOTICE, "%s rebuild broken DB keys", tag);

  bool success = false;

  grn_obj path;
  GRN_TEXT_INIT(&path, 0);
  GRN_TEXT_SETS(ctx, &path, grn_io_path(grn_obj_get_io(ctx, db->keys)));
  GRN_TEXT_PUTC(ctx, &path, '\0');
  grn_obj recovering_path;
  GRN_TEXT_INIT(&recovering_path, 0);
  GRN_TEXT_PUTS(ctx, &recovering_path, GRN_TEXT_VALUE(&path));
  GRN_TEXT_PUTS(ctx, &recovering_path, ".recovering");
  GRN_TEXT_PUTC(ctx, &recovering_path, '\0');
  grn_obj broken_path;
  GRN_TEXT_INIT(&broken_path, 0);
  GRN_TEXT_PUTS(ctx, &broken_path, GRN_TEXT_VALUE(&path));
  GRN_TEXT_PUTS(ctx, &broken_path, ".broken");
  GRN_TEXT_PUTC(ctx, &broken_path, '\0');

  grn_io_remove_if_exist(ctx, GRN_TEXT_VALUE(&recovering_path));
  bool use_pat = (db->keys->header.type == GRN_TABLE_PAT_KEY);
  grn_obj *new_keys = NULL;
  if (use_pat) {
    new_keys = (grn_obj *)grn_pat_create(ctx,
                                         GRN_TEXT_VALUE(&recovering_path),
                                         GRN_TABLE_MAX_KEY_SIZE,
                                         0,
                                         GRN_OBJ_KEY_VAR_SIZE);
  } else {
    new_keys = (grn_obj *)grn_dat_create(ctx,
                                         GRN_TEXT_VALUE(&recovering_path),
                                         GRN_TABLE_MAX_KEY_SIZE,
                                         0,
                                         GRN_OBJ_KEY_VAR_SIZE);
  }
  if (!new_keys) {
    goto exit;
  }

  {
    grn_obj placeholder_key;
    GRN_TEXT_INIT(&placeholder_key, 0);
    grn_id previous_id = GRN_ID_NIL;
    GRN_TABLE_EACH_BEGIN_FLAGS(ctx,
                               db->keys,
                               cursor,
                               id,
                               GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING)
    {
      for (; previous_id < id - 1; previous_id++) {
        GRN_BULK_REWIND(&placeholder_key);
        grn_text_printf(ctx,
                        &placeholder_key,
                        "%s%u",
                        placeholder_prefix,
                        previous_id + 1);
        grn_id placeholder_id;
        if (use_pat) {
          placeholder_id = grn_pat_add(ctx,
                                       (grn_pat *)new_keys,
                                       GRN_TEXT_VALUE(&placeholder_key),
                                       GRN_TEXT_LEN(&placeholder_key),
                                       NULL,
                                       NULL);
        } else {
          placeholder_id = grn_dat_add(ctx,
                                       (grn_dat *)new_keys,
                                       GRN_TEXT_VALUE(&placeholder_key),
                                       GRN_TEXT_LEN(&placeholder_key),
                                       NULL,
                                       NULL);
        }
        if (placeholder_id != previous_id + 1) {
          ERR(GRN_UNKNOWN_ERROR,
              "%s failed to assign ID for deleted object: "
              "assigned:%u "
              "expected:%u",
              tag,
              placeholder_id,
              previous_id + 1);
          break;
        }
      }
      if (ctx->rc != GRN_SUCCESS) {
        break;
      }
      void *key;
      int key_size = grn_table_cursor_get_key(ctx, cursor, &key);
      grn_id added_id;
      if (new_keys->header.type == GRN_TABLE_PAT_KEY) {
        added_id =
          grn_pat_add(ctx, (grn_pat *)new_keys, key, key_size, NULL, NULL);
      } else {
        added_id =
          grn_dat_add(ctx, (grn_dat *)new_keys, key, key_size, NULL, NULL);
      }
      if (added_id != id) {
        ERR(GRN_UNKNOWN_ERROR,
            "%s failed to assign expected ID: "
            "name:<%.*s> "
            "assigned:%u "
            "expected:%u",
            tag,
            key_size,
            (const char *)key,
            added_id,
            id);
        break;
      }
      previous_id = id;
    }
    GRN_TABLE_EACH_END(ctx, cursor);
    GRN_OBJ_FIN(ctx, &placeholder_key);
  }
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  GRN_TABLE_EACH_BEGIN_MIN(ctx,
                           new_keys,
                           cursor,
                           id,
                           placeholder_prefix,
                           strlen(placeholder_prefix),
                           GRN_CURSOR_PREFIX)
  {
    grn_table_cursor_delete(ctx, cursor);
  }
  GRN_TABLE_EACH_END(ctx, cursor);

  grn_obj_close(ctx, new_keys);
  new_keys = NULL;
  grn_obj_close(ctx, db->keys);

  grn_io_remove_if_exist(ctx, GRN_TEXT_VALUE(&broken_path));
  grn_io_rename(ctx, GRN_TEXT_VALUE(&path), GRN_TEXT_VALUE(&broken_path));
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }
  grn_io_rename(ctx, GRN_TEXT_VALUE(&recovering_path), GRN_TEXT_VALUE(&path));
  if (ctx->rc != GRN_SUCCESS) {
    ERRCLR(ctx);
    grn_io_remove_if_exist(ctx, GRN_TEXT_VALUE(&path));
    grn_io_rename(ctx, GRN_TEXT_VALUE(&broken_path), GRN_TEXT_VALUE(&path));
    ERR(GRN_UNKNOWN_ERROR,
        "%s failed to rename files of rebuilt DB keys: <%s>",
        tag,
        GRN_TEXT_VALUE(&recovering_path));
    goto exit;
  }
  if (use_pat) {
    db->keys = (grn_obj *)grn_pat_open(ctx, GRN_TEXT_VALUE(&path));
  } else {
    db->keys = (grn_obj *)grn_dat_open(ctx, GRN_TEXT_VALUE(&path));
  }
  if (!db->keys) {
    ERRCLR(ctx);
    grn_io_remove_if_exist(ctx, GRN_TEXT_VALUE(&path));
    grn_io_rename(ctx, GRN_TEXT_VALUE(&broken_path), GRN_TEXT_VALUE(&path));
    ERR(GRN_UNKNOWN_ERROR,
        "%s failed to open rebuilt DB keys: <%s>",
        tag,
        GRN_TEXT_VALUE(&path));
    goto exit;
  }
  grn_io_remove(ctx, GRN_TEXT_VALUE(&broken_path));
  success = true;
  GRN_LOG(ctx, GRN_LOG_NOTICE, "%s succeeded to rebuild broken DB keys", tag);

exit:
  if (!success) {
    GRN_LOG(ctx, GRN_LOG_NOTICE, "%s failed to rebuild broken DB keys", tag);
  }
  if (new_keys) {
    grn_obj_close(ctx, new_keys);
  }
  GRN_OBJ_FIN(ctx, &path);
  GRN_OBJ_FIN(ctx, &recovering_path);
  GRN_OBJ_FIN(ctx, &broken_path);
}

static grn_hash *
grn_broken_ids_open(grn_ctx *ctx)
{
  return grn_hash_create(ctx,
                         NULL,
                         sizeof(grn_id),
                         0,
                         GRN_OBJ_TABLE_HASH_KEY | GRN_HASH_TINY);
}

static void
grn_broken_ids_close(grn_ctx *ctx, grn_hash *broken_ids)
{
  grn_hash_close(ctx, broken_ids);
}

static void
grn_broken_ids_add(grn_ctx *ctx, grn_hash *broken_ids, grn_id id)
{
  grn_hash_add(ctx, broken_ids, &id, sizeof(grn_id), NULL, NULL);
}

static bool
grn_broken_ids_exist(grn_ctx *ctx, grn_hash *broken_ids, grn_id id)
{
  return grn_hash_get(ctx, broken_ids, &id, sizeof(grn_id), NULL) != GRN_ID_NIL;
}

static grn_id
grn_broken_ids_cursor_get(grn_ctx *ctx, grn_hash_cursor *cursor)
{
  void *key;
  grn_hash_cursor_get_key(ctx, cursor, &key);
  return *((grn_id *)key);
}

static void
grn_db_wal_recover_collect_depended_object_ids(grn_ctx *ctx,
                                               grn_db *db,
                                               grn_hash *broken_table_ids,
                                               grn_hash *broken_column_ids)
{
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, db->keys, cursor, id, GRN_CURSOR_BY_ID)
  {
    if (grn_id_is_builtin(ctx, id)) {
      continue;
    }
    if (grn_broken_ids_exist(ctx, broken_table_ids, id)) {
      continue;
    }
    if (grn_broken_ids_exist(ctx, broken_column_ids, id)) {
      continue;
    }

    grn_ctx_push_temporary_open_space(ctx);
    grn_obj *object = grn_ctx_at(ctx, id);
    if (object) {
      switch (object->header.type) {
      case GRN_TABLE_HASH_KEY:
      case GRN_TABLE_PAT_KEY:
      case GRN_TABLE_DAT_KEY:
        if (grn_broken_ids_exist(ctx,
                                 broken_table_ids,
                                 object->header.domain)) {
          grn_broken_ids_add(ctx, broken_table_ids, id);
        }
        break;
      case GRN_COLUMN_FIX_SIZE:
        if (grn_broken_ids_exist(ctx,
                                 broken_table_ids,
                                 DB_OBJ(object)->range)) {
          grn_broken_ids_add(ctx, broken_column_ids, id);
        }
        break;
      case GRN_COLUMN_VAR_SIZE:
        if (grn_broken_ids_exist(ctx,
                                 broken_table_ids,
                                 DB_OBJ(object)->range)) {
          grn_broken_ids_add(ctx, broken_column_ids, id);
        }
        {
          grn_obj source_ids;
          GRN_RECORD_INIT(&source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
          grn_obj_get_info(ctx, object, GRN_INFO_SOURCE, &source_ids);
          size_t n = GRN_RECORD_VECTOR_SIZE(&source_ids);
          size_t i;
          for (i = 0; i < n; i++) {
            grn_id source_id = GRN_RECORD_VALUE_AT(&source_ids, i);
            if (grn_broken_ids_exist(ctx, broken_column_ids, source_id)) {
              grn_broken_ids_add(ctx, broken_column_ids, id);
            }
          }
          GRN_OBJ_FIN(ctx, &source_ids);
        }
        break;
      case GRN_COLUMN_INDEX:
        if (grn_broken_ids_exist(ctx,
                                 broken_table_ids,
                                 DB_OBJ(object)->range)) {
          grn_broken_ids_add(ctx, broken_column_ids, id);
        }
        {
          grn_obj source_ids;
          GRN_RECORD_INIT(&source_ids, GRN_OBJ_VECTOR, GRN_ID_NIL);
          grn_obj_get_info(ctx, object, GRN_INFO_SOURCE, &source_ids);
          size_t n = GRN_RECORD_VECTOR_SIZE(&source_ids);
          size_t i;
          for (i = 0; i < n; i++) {
            grn_id source_id = GRN_RECORD_VALUE_AT(&source_ids, i);
            if (grn_broken_ids_exist(ctx, broken_table_ids, source_id) ||
                grn_broken_ids_exist(ctx, broken_column_ids, source_id)) {
              grn_broken_ids_add(ctx, broken_column_ids, id);
            }
          }
          GRN_OBJ_FIN(ctx, &source_ids);
        }
        break;
      default:
        break;
      }
      GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_obj_unref(ctx, object); }
      GRN_DB_WAL_DISABLE_WAL_END(ctx);
    }
    ERRCLR(ctx);
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
}

static void
grn_db_wal_recover_remove_duplicated_broken_column_ids(
  grn_ctx *ctx,
  grn_db *db,
  grn_hash *broken_table_ids,
  grn_hash *broken_column_ids)
{
  GRN_HASH_EACH_BEGIN(ctx, broken_column_ids, cursor, column_entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_column_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_obj *broken_column = grn_ctx_at(ctx, broken_column_id);
    if (broken_column && grn_broken_ids_exist(ctx,
                                              broken_table_ids,
                                              broken_column->header.domain)) {
      grn_hash_cursor_delete(ctx, cursor, NULL);
    }
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
  }
  GRN_HASH_EACH_END(ctx, cursor);
}

static void
grn_db_wal_recover_collect_related_broken_object_ids(
  grn_ctx *ctx,
  grn_db *db,
  grn_hash *broken_table_ids,
  grn_hash *broken_column_ids)
{
  GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx)
  {
    grn_db_wal_recover_collect_depended_object_ids(ctx,
                                                   db,
                                                   broken_table_ids,
                                                   broken_column_ids);
    grn_db_wal_recover_remove_duplicated_broken_column_ids(ctx,
                                                           db,
                                                           broken_table_ids,
                                                           broken_column_ids);
  }
  GRN_DB_WAL_DISABLE_WAL_END(ctx);
}

static const char grn_db_wal_recovering_name_prefix[] = "#recovering#";
static const int grn_db_wal_recovering_name_prefix_length =
  sizeof(grn_db_wal_recovering_name_prefix) - 1;
static const char grn_db_wal_broken_name_prefix[] = "#broken#";
static const int grn_db_wal_broken_name_prefix_length =
  sizeof(grn_db_wal_broken_name_prefix) - 1;

static bool
grn_db_wal_recover_is_prefixed_name(grn_ctx *ctx,
                                    const char *name,
                                    int name_length,
                                    const char *prefix,
                                    int prefix_length)
{
  if (name_length < prefix_length) {
    return false;
  }
  if (strncmp(name, prefix, prefix_length) == 0) {
    return true;
  }
  const char *column_name = NULL;
  int column_name_length = 0;
  int i;
  for (i = 0; i < name_length; i++) {
    if (name[i] == '.') {
      column_name = name + i + 1;
      column_name_length = name_length - i - 1;
      break;
    }
  }
  if (column_name_length < prefix_length) {
    return false;
  }
  return strncmp(column_name, prefix, prefix_length) == 0;
}

static bool
grn_db_wal_recover_is_recovering_object_name(grn_ctx *ctx,
                                             const char *name,
                                             int name_length)
{
  return grn_db_wal_recover_is_prefixed_name(
    ctx,
    name,
    name_length,
    grn_db_wal_recovering_name_prefix,
    grn_db_wal_recovering_name_prefix_length);
}

static void
grn_db_wal_recover_remove_recovering_object(
  grn_ctx *ctx, grn_db *db, grn_id id, const char *name, int name_length)
{
  const char *tag = grn_db_wal_recover_tag;

  grn_obj *object = grn_ctx_at(ctx, id);
  if (!object) {
    if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "%s remove unopenable recovering object: <%.*s>(%u): %s",
              tag,
              name_length,
              name,
              id,
              ctx->errbuf);
    }
    ERRCLR(ctx);
    grn_db_wal_recover_ensure_remove_by_id(ctx, db, id);
    return;
  }

  if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "%s remove recovering object: <%.*s>(%u)",
            tag,
            name_length,
            name,
            id);
  }
  grn_db_wal_recover_remove_object(ctx, db, object, id);

  const char *original_name = name - grn_db_wal_recovering_name_prefix_length;
  int original_name_length =
    name_length - grn_db_wal_recovering_name_prefix_length;
  grn_obj broken_name;
  GRN_TEXT_INIT(&broken_name, 0);
  GRN_TEXT_PUT(ctx,
               &broken_name,
               grn_db_wal_broken_name_prefix,
               grn_db_wal_broken_name_prefix_length);
  GRN_TEXT_PUT(ctx, &broken_name, original_name, original_name_length);
  grn_obj *original_object =
    grn_ctx_get(ctx, original_name, original_name_length);
  grn_obj *broken_object =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(&broken_name), GRN_TEXT_LEN(&broken_name));
  if (broken_object) {
    grn_id broken_object_id = DB_OBJ(broken_object)->id;
    if (original_object) {
      if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "%s remove broken object: <%.*s>(%u)",
                tag,
                (int)GRN_TEXT_LEN(&broken_name),
                GRN_TEXT_VALUE(&broken_name),
                broken_object_id);
      }
      grn_db_wal_recover_remove_object(ctx,
                                       db,
                                       broken_object,
                                       broken_object_id);
    } else {
      if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "%s reuse broken object: <%.*s>(%u)",
                tag,
                (int)GRN_TEXT_LEN(&broken_name),
                GRN_TEXT_VALUE(&broken_name),
                broken_object_id);
      }
      grn_obj_rename(ctx, broken_object, original_name, original_name_length);
    }
  }
  GRN_OBJ_FIN(ctx, &broken_name);
}

static void
grn_db_wal_recover_remove_recovering_objects(grn_ctx *ctx, grn_db *db)
{
  GRN_TABLE_EACH_BEGIN(ctx, db->keys, cursor, id)
  {
    void *key;
    int key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    auto name = static_cast<char *>(key);
    if (!grn_db_wal_recover_is_recovering_object_name(ctx, name, key_size)) {
      continue;
    }
    grn_ctx_push_temporary_open_space(ctx);
    grn_db_wal_recover_remove_recovering_object(ctx, db, id, name, key_size);
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
}

static bool
grn_db_wal_recover_is_broken_object_name(grn_ctx *ctx,
                                         const char *name,
                                         int name_length)
{
  return grn_db_wal_recover_is_prefixed_name(
    ctx,
    name,
    name_length,
    grn_db_wal_broken_name_prefix,
    grn_db_wal_broken_name_prefix_length);
}

static void
grn_db_wal_recover_remove_broken_object(
  grn_ctx *ctx, grn_db *db, grn_id id, const char *name, int name_length)
{
  const char *tag = grn_db_wal_recover_tag;

  grn_obj *object = grn_ctx_at(ctx, id);
  if (!object) {
    if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
      GRN_LOG(ctx,
              GRN_LOG_DEBUG,
              "%s remove unopenable broken object: <%.*s>(%u): %s",
              tag,
              name_length,
              name,
              id,
              ctx->errbuf);
    }
    ERRCLR(ctx);
    grn_db_wal_recover_ensure_remove_by_id(ctx, db, id);
    return;
  }

  if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
    GRN_LOG(ctx,
            GRN_LOG_DEBUG,
            "%s remove broken object: <%.*s>(%u)",
            tag,
            name_length,
            name,
            id);
  }
  grn_db_wal_recover_remove_object(ctx, db, object, id);
}

static void
grn_db_wal_recover_remove_broken_objects(grn_ctx *ctx, grn_db *db)
{
  GRN_TABLE_EACH_BEGIN(ctx, db->keys, cursor, id)
  {
    void *key;
    int key_size = grn_table_cursor_get_key(ctx, cursor, &key);
    auto name = static_cast<char *>(key);
    if (!grn_db_wal_recover_is_broken_object_name(ctx, name, key_size)) {
      continue;
    }
    grn_ctx_push_temporary_open_space(ctx);
    grn_db_wal_recover_remove_broken_object(ctx, db, id, name, key_size);
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
  }
  GRN_TABLE_EACH_END(ctx, cursor);
}

static void
grn_db_wal_recover_copy_table(grn_ctx *ctx, grn_obj *table, grn_id_map *id_map)
{
  GRN_DEFINE_NAME(table);
  GRN_LOG(ctx,
          GRN_LOG_NOTICE,
          "%s rebuild broken table: <%.*s>(%u)",
          grn_db_wal_recover_tag,
          name_size,
          name,
          DB_OBJ(table)->id);
  grn_obj new_name;
  GRN_TEXT_INIT(&new_name, 0);
  GRN_TEXT_PUT(ctx,
               &new_name,
               grn_db_wal_recovering_name_prefix,
               grn_db_wal_recovering_name_prefix_length);
  GRN_TEXT_PUT(ctx, &new_name, name, name_size);
  grn_obj *new_table =
    grn_table_create_similar_id_map(ctx,
                                    GRN_TEXT_VALUE(&new_name),
                                    GRN_TEXT_LEN(&new_name),
                                    NULL,
                                    table,
                                    id_map);
  GRN_OBJ_FIN(ctx, &new_name);
  if (new_table) {
    grn_id_map_add(ctx, id_map, DB_OBJ(table)->id, DB_OBJ(new_table)->id);
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_table_copy(ctx, table, new_table); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    grn_obj_unref(ctx, new_table);
  }
}

static bool
grn_db_wal_recover_is_data_column(grn_ctx *ctx, grn_obj *column)
{
  if (grn_obj_have_source(ctx, column)) {
    return false;
  }
  if (grn_obj_is_index_column(ctx, column)) {
    return false;
  }
  return true;
}

static void
grn_db_wal_recover_copy_data_column(grn_ctx *ctx,
                                    grn_obj *column,
                                    grn_obj *new_table,
                                    grn_id_map *id_map)
{
  if (!grn_db_wal_recover_is_data_column(ctx, column)) {
    return;
  }
  if (grn_logger_pass(ctx, GRN_LOG_NOTICE)) {
    GRN_DEFINE_NAME(column);
    GRN_LOG(ctx,
            GRN_LOG_NOTICE,
            "%s rebuild broken data column: <%.*s>(%u)",
            grn_db_wal_recover_tag,
            name_size,
            name,
            DB_OBJ(column)->id);
  }
  char name[GRN_TABLE_MAX_KEY_SIZE];
  int name_size = grn_column_name(ctx, column, name, sizeof(name));
  grn_obj new_name;
  bool need_new_table_unref = false;
  if (new_table) {
    GRN_TEXT_INIT(&new_name, GRN_OBJ_DO_SHALLOW_COPY);
    GRN_TEXT_SET(ctx, &new_name, name, name_size);
  } else {
    GRN_TEXT_INIT(&new_name, 0);
    GRN_TEXT_PUT(ctx,
                 &new_name,
                 grn_db_wal_recovering_name_prefix,
                 grn_db_wal_recovering_name_prefix_length);
    GRN_TEXT_PUT(ctx, &new_name, name, name_size);
    new_table = grn_ctx_at(ctx, column->header.domain);
    need_new_table_unref = true;
  }
  grn_obj *new_column =
    grn_column_create_similar_id_map(ctx,
                                     new_table,
                                     GRN_TEXT_VALUE(&new_name),
                                     GRN_TEXT_LEN(&new_name),
                                     NULL,
                                     column,
                                     id_map);
  GRN_OBJ_FIN(ctx, &new_name);
  if (new_column) {
    grn_id_map_add(ctx, id_map, DB_OBJ(column)->id, DB_OBJ(new_column)->id);
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx)
    {
      grn_column_copy(ctx, column, new_column);
    }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    grn_obj_unref(ctx, new_column);
  }
  if (need_new_table_unref) {
    grn_obj_unref(ctx, new_table);
  }
}

static void
grn_db_wal_recover_copy_non_reference_data_column(grn_ctx *ctx,
                                                  grn_obj *column,
                                                  grn_obj *new_table,
                                                  grn_id_map *id_map)
{
  if (!grn_id_is_builtin(ctx, DB_OBJ(column)->range)) {
    return;
  }
  grn_db_wal_recover_copy_data_column(ctx, column, new_table, id_map);
}

static void
grn_db_wal_recover_copy_non_reference_data_columns(grn_ctx *ctx,
                                                   grn_obj *table,
                                                   grn_id_map *id_map)
{
  grn_hash *columns = grn_table_all_columns(ctx, table);
  if (grn_hash_size(ctx, columns) == 0) {
    return;
  }

  GRN_DEFINE_NAME(table);
  grn_obj new_name;
  GRN_TEXT_INIT(&new_name, 0);
  GRN_TEXT_PUT(ctx,
               &new_name,
               grn_db_wal_recovering_name_prefix,
               grn_db_wal_recovering_name_prefix_length);
  GRN_TEXT_PUT(ctx, &new_name, name, name_size);
  grn_obj *new_table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(&new_name), GRN_TEXT_LEN(&new_name));
  GRN_OBJ_FIN(ctx, &new_name);

  GRN_HASH_EACH_BEGIN(ctx, columns, cursor, column_entry_id)
  {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    grn_id column_id = *((grn_id *)key);
    grn_obj *column = grn_ctx_at(ctx, column_id);
    if (!column) {
      continue;
    }
    grn_db_wal_recover_copy_non_reference_data_column(ctx,
                                                      column,
                                                      new_table,
                                                      id_map);
    grn_obj_unref(ctx, column);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);

  grn_obj_unref(ctx, new_table);
}

static void
grn_db_wal_recover_copy_reference_data_column(grn_ctx *ctx,
                                              grn_obj *column,
                                              grn_obj *new_table,
                                              grn_id_map *id_map)
{
  if (grn_id_is_builtin(ctx, DB_OBJ(column)->range)) {
    return;
  }
  grn_db_wal_recover_copy_data_column(ctx, column, new_table, id_map);
}

static void
grn_db_wal_recover_copy_reference_data_columns(grn_ctx *ctx,
                                               grn_obj *table,
                                               grn_id_map *id_map)
{
  grn_hash *columns = grn_table_all_columns(ctx, table);
  if (grn_hash_size(ctx, columns) == 0) {
    return;
  }

  GRN_DEFINE_NAME(table);
  grn_obj new_name;
  GRN_TEXT_INIT(&new_name, 0);
  GRN_TEXT_PUT(ctx,
               &new_name,
               grn_db_wal_recovering_name_prefix,
               grn_db_wal_recovering_name_prefix_length);
  GRN_TEXT_PUT(ctx, &new_name, name, name_size);
  grn_obj *new_table =
    grn_ctx_get(ctx, GRN_TEXT_VALUE(&new_name), GRN_TEXT_LEN(&new_name));
  GRN_OBJ_FIN(ctx, &new_name);

  GRN_HASH_EACH_BEGIN(ctx, columns, cursor, column_entry_id)
  {
    void *key;
    grn_hash_cursor_get_key(ctx, cursor, &key);
    grn_id column_id = *((grn_id *)key);
    grn_obj *column = grn_ctx_at(ctx, column_id);
    if (!column) {
      continue;
    }
    grn_db_wal_recover_copy_reference_data_column(ctx,
                                                  column,
                                                  new_table,
                                                  id_map);
    grn_obj_unref(ctx, column);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);

  grn_obj_unref(ctx, new_table);
}

static void
grn_db_wal_recover_rename_column(grn_ctx *ctx,
                                 grn_db *db,
                                 grn_id broken_column_id,
                                 grn_id_map *id_map)
{
  grn_id recovering_column_id =
    grn_id_map_resolve(ctx, id_map, broken_column_id);
  grn_obj *recovering_column = grn_ctx_at(ctx, recovering_column_id);
  char recovering_column_name[GRN_TABLE_MAX_KEY_SIZE];
  int recovering_column_name_length =
    grn_column_name(ctx,
                    recovering_column,
                    recovering_column_name,
                    sizeof(recovering_column_name));
  char *column_name =
    recovering_column_name + grn_db_wal_recovering_name_prefix_length;
  int column_name_length =
    recovering_column_name_length - grn_db_wal_recovering_name_prefix_length;

  grn_obj *broken_column = grn_ctx_at(ctx, broken_column_id);
  if (broken_column) {
    grn_obj broken_column_name;
    GRN_TEXT_INIT(&broken_column_name, 0);
    GRN_TEXT_PUT(ctx,
                 &broken_column_name,
                 grn_db_wal_broken_name_prefix,
                 grn_db_wal_broken_name_prefix_length);
    GRN_TEXT_PUT(ctx, &broken_column_name, column_name, column_name_length);
    grn_column_rename(ctx,
                      broken_column,
                      GRN_TEXT_VALUE(&broken_column_name),
                      GRN_TEXT_LEN(&broken_column_name));
    GRN_OBJ_FIN(ctx, &broken_column_name);
  }

  grn_column_rename(ctx, recovering_column, column_name, column_name_length);
  if (ctx->rc == GRN_SUCCESS) {
    if (broken_column) {
      grn_db_wal_recover_remove_object(ctx, db, broken_column, GRN_ID_NIL);
    }
  }
  if (grn_logger_pass(ctx, GRN_LOG_NOTICE)) {
    GRN_DEFINE_NAME(recovering_column);
    GRN_LOG(ctx,
            GRN_LOG_NOTICE,
            "%s succeeded to rebuild broken column: <%.*s>(%u)",
            grn_db_wal_recover_tag,
            name_size,
            name,
            DB_OBJ(recovering_column)->id);
  }
  grn_obj_unref(ctx, recovering_column);
}

static void
grn_db_wal_recover_rename_table(grn_ctx *ctx,
                                grn_db *db,
                                grn_id broken_table_id,
                                grn_id_map *id_map)
{
  grn_id new_table_id = grn_id_map_resolve(ctx, id_map, broken_table_id);
  grn_obj *new_table = grn_ctx_at(ctx, new_table_id);
  GRN_DEFINE_NAME_CUSTOM(new_table, new_table_name);
  const char *table_name =
    new_table_name + grn_db_wal_recovering_name_prefix_length;
  int table_name_length =
    new_table_name_size - grn_db_wal_recovering_name_prefix_length;

  grn_obj *broken_table = grn_ctx_at(ctx, broken_table_id);
  if (broken_table) {
    grn_obj broken_table_name;
    GRN_TEXT_INIT(&broken_table_name, 0);
    GRN_TEXT_PUT(ctx,
                 &broken_table_name,
                 grn_db_wal_broken_name_prefix,
                 grn_db_wal_broken_name_prefix_length);
    GRN_TEXT_PUT(ctx, &broken_table_name, table_name, table_name_length);
    grn_table_rename(ctx,
                     broken_table,
                     GRN_TEXT_VALUE(&broken_table_name),
                     GRN_TEXT_LEN(&broken_table_name));
    GRN_OBJ_FIN(ctx, &broken_table_name);
  }

  grn_table_rename(ctx, new_table, table_name, table_name_length);
  if (ctx->rc == GRN_SUCCESS) {
    if (broken_table) {
      grn_db_wal_recover_remove_object(ctx, db, broken_table, GRN_ID_NIL);
    }
  }
  GRN_LOG(ctx,
          GRN_LOG_NOTICE,
          "%s succeeded to rebuild broken table: <%.*s>(%u)",
          grn_db_wal_recover_tag,
          table_name_length,
          table_name,
          DB_OBJ(new_table)->id);
  grn_obj_unref(ctx, new_table);
}

namespace {
  class DBWALCopyRecoverer {
  private:
    grn_ctx *ctx_;
    grn_hash *broken_table_ids_;
    grn_hash *broken_column_ids_;
    grn_id_map *id_map_;
    grn::TaskExecutor *task_executor_;

  public:
    explicit DBWALCopyRecoverer(grn_ctx *ctx,
                                grn_hash *broken_table_ids,
                                grn_hash *broken_column_ids,
                                grn_id_map *id_map)
      : ctx_(ctx),
        broken_table_ids_(broken_table_ids),
        broken_column_ids_(broken_column_ids),
        id_map_(id_map),
        task_executor_(grn_ctx_get_task_executor(ctx))
    {
    }

    void
    rebuild_auto_generated_columns()
    {
      std::vector<std::pair<grn_id, grn_id>> targets;
      GRN_HASH_EACH_BEGIN(ctx_, broken_table_ids_, cursor, entry_id)
      {
        grn_ctx_push_temporary_open_space(ctx_);
        grn_id broken_table_id = grn_broken_ids_cursor_get(ctx_, cursor);
        grn_obj *broken_table = grn_ctx_at(ctx_, broken_table_id);
        if (broken_table) {
          collect_broken_auto_generated_columns(broken_table, targets);
          grn_obj_unref(ctx_, broken_table);
        }
        GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx_)
        {
          grn_ctx_pop_temporary_open_space(ctx_);
        }
        GRN_DB_WAL_DISABLE_WAL_END(ctx_);
        if (ctx_->rc != GRN_SUCCESS) {
          return;
        }
      }
      GRN_HASH_EACH_END(ctx_, cursor);

      GRN_HASH_EACH_BEGIN(ctx_, broken_column_ids_, cursor, entry_id)
      {
        grn_ctx_push_temporary_open_space(ctx_);
        grn_id broken_column_id = grn_broken_ids_cursor_get(ctx_, cursor);
        grn_obj *broken_column = grn_ctx_at(ctx_, broken_column_id);
        if (broken_column) {
          if (is_auto_generated_column(ctx_, broken_column)) {
            targets.emplace_back(GRN_ID_NIL, broken_column_id);
          }
          grn_obj_unref(ctx_, broken_column);
        }
        GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx_)
        {
          grn_ctx_pop_temporary_open_space(ctx_);
        }
        GRN_DB_WAL_DISABLE_WAL_END(ctx_);
        if (ctx_->rc != GRN_SUCCESS) {
          return;
        }
      }
      GRN_HASH_EACH_END(ctx_, cursor);

      auto n_columns = targets.size();
      if (n_columns == 0) {
        return;
      }

      auto n_workers = task_executor_->get_n_workers();
      if (n_workers == 0) {
        n_workers = 1;
      }
      n_workers = std::min(n_workers, static_cast<uint32_t>(n_columns));
      auto n_columns_per_worker = (n_columns / n_workers) + 1;
      // We can't use temporary open space with parallel mode because
      // it may close objects that are still used by other thread. But
      // we can use temporary open space with reference count
      // mode. Because it doesn't close used objects.
      const bool use_temporary_open_space =
        (!task_executor_->is_parallel() || grn_is_reference_count_enable());
      for (size_t i = 0; i < n_workers; i++) {
        auto execute = [&, i]() {
          size_t start = n_columns_per_worker * i;
          size_t end = std::min(n_columns_per_worker * (i + 1), n_columns);
          grn_ctx *task_ctx = ctx_;
          grn_ctx *child_ctx = nullptr;
          if (task_executor_->is_parallel()) {
            task_ctx = child_ctx = grn_ctx_pull_child(ctx_);
          }
          grn::ChildCtxReleaser releaser(ctx_, child_ctx);
          for (size_t j = start; j < end; ++j) {
            if (use_temporary_open_space) {
              grn_ctx_push_temporary_open_space(task_ctx);
            }
            auto [new_table_id, broken_column_id] = targets[j];
            grn_obj *new_table = nullptr;
            if (new_table_id != GRN_ID_NIL) {
              new_table = grn_ctx_at(task_ctx, new_table_id);
            }
            auto broken_column = grn_ctx_at(task_ctx, broken_column_id);
            if (broken_column) {
              rebuild_auto_generated_column(task_ctx, broken_column, new_table);
              grn_obj_unref(task_ctx, broken_column);
            }
            if (new_table) {
              grn_obj_unref(task_ctx, new_table);
            }
            if (use_temporary_open_space) {
              GRN_DB_WAL_DISABLE_WAL_BEGIN(task_ctx)
              {
                grn_ctx_pop_temporary_open_space(task_ctx);
              }
              GRN_DB_WAL_DISABLE_WAL_END(task_ctx);
            }
            if (task_ctx->rc != GRN_SUCCESS) {
              break;
            }
          }
          return task_ctx->rc == GRN_SUCCESS;
        };
        task_executor_->execute(i, execute, grn_db_wal_recover_tag);
      }
      task_executor_->wait_all();
    }

  private:
    bool
    is_auto_generated_column(grn_ctx *ctx, grn_obj *column)
    {
      return !grn_db_wal_recover_is_data_column(ctx, column);
    }

    void
    collect_broken_auto_generated_columns(
      grn_obj *table, std::vector<std::pair<grn_id, grn_id>> &targets)
    {
      auto columns = grn_table_all_columns(ctx_, table);
      if (grn_hash_size(ctx_, columns) == 0) {
        return;
      }

      auto ctx = ctx_;
      GRN_DEFINE_NAME(table);
      grn_obj new_name;
      GRN_TEXT_INIT(&new_name, 0);
      GRN_TEXT_PUT(ctx_,
                   &new_name,
                   grn_db_wal_recovering_name_prefix,
                   grn_db_wal_recovering_name_prefix_length);
      GRN_TEXT_PUT(ctx, &new_name, name, name_size);
      grn_obj *new_table =
        grn_ctx_get(ctx_, GRN_TEXT_VALUE(&new_name), GRN_TEXT_LEN(&new_name));
      GRN_OBJ_FIN(ctx, &new_name);

      auto new_table_id = DB_OBJ(new_table)->id;
      GRN_HASH_EACH_BEGIN(ctx, columns, cursor, column_entry_id)
      {
        void *key;
        grn_hash_cursor_get_key(ctx, cursor, &key);
        grn_id column_id = *((grn_id *)key);
        grn_obj *column = grn_ctx_at(ctx, column_id);
        if (!column) {
          continue;
        }
        if (is_auto_generated_column(ctx, column)) {
          targets.emplace_back(new_table_id, column_id);
        }
        grn_obj_unref(ctx, column);
        if (ctx->rc != GRN_SUCCESS) {
          break;
        }
      }
      GRN_HASH_EACH_END(ctx, cursor);

      grn_obj_unref(ctx, new_table);
    }

    void
    rebuild_auto_generated_column(grn_ctx *ctx,
                                  grn_obj *column,
                                  grn_obj *new_table)
    {
      if (grn_logger_pass(ctx, GRN_LOG_NOTICE)) {
        GRN_DEFINE_NAME(column);
        GRN_LOG(ctx,
                GRN_LOG_NOTICE,
                "%s rebuild broken auto generated column: <%.*s>(%u)",
                grn_db_wal_recover_tag,
                name_size,
                name,
                DB_OBJ(column)->id);
      }
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size = grn_column_name(ctx, column, name, sizeof(name));
      grn_obj new_name;
      bool need_new_table_unref = false;
      if (new_table) {
        GRN_TEXT_INIT(&new_name, GRN_OBJ_DO_SHALLOW_COPY);
        GRN_TEXT_SET(ctx, &new_name, name, name_size);
      } else {
        GRN_TEXT_INIT(&new_name, 0);
        GRN_TEXT_PUT(ctx,
                     &new_name,
                     grn_db_wal_recovering_name_prefix,
                     grn_db_wal_recovering_name_prefix_length);
        GRN_TEXT_PUT(ctx, &new_name, name, name_size);
        new_table = grn_ctx_at(ctx, column->header.domain);
        need_new_table_unref = true;
      }
      auto new_column =
        grn_column_create_similar_id_map(ctx,
                                         new_table,
                                         GRN_TEXT_VALUE(&new_name),
                                         GRN_TEXT_LEN(&new_name),
                                         nullptr,
                                         column,
                                         id_map_);
      GRN_OBJ_FIN(ctx, &new_name);
      if (new_column) {
        grn_id_map_add(ctx,
                       id_map_,
                       DB_OBJ(column)->id,
                       DB_OBJ(new_column)->id);
        grn_obj_unref(ctx, new_column);
      }
      if (need_new_table_unref) {
        grn_obj_unref(ctx, new_table);
      }
    }
  };
}; // namespace

static void
grn_db_wal_recover_copy_rename(grn_ctx *ctx,
                               grn_db *db,
                               grn_hash *broken_table_ids,
                               grn_hash *broken_column_ids)
{
  grn_id_map *id_map = grn_id_map_open(ctx);

  GRN_HASH_EACH_BEGIN(ctx, broken_table_ids, cursor, entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_table_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_obj *broken_table = grn_ctx_at(ctx, broken_table_id);
    if (broken_table) {
      grn_db_wal_recover_copy_table(ctx, broken_table, id_map);
      grn_obj_unref(ctx, broken_table);
    }
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  GRN_HASH_EACH_BEGIN(ctx, broken_table_ids, cursor, entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_table_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_obj *broken_table = grn_ctx_at(ctx, broken_table_id);
    if (broken_table) {
      grn_db_wal_recover_copy_non_reference_data_columns(ctx,
                                                         broken_table,
                                                         id_map);
      grn_obj_unref(ctx, broken_table);
    }
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  GRN_HASH_EACH_BEGIN(ctx, broken_column_ids, cursor, entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_column_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_obj *broken_column = grn_ctx_at(ctx, broken_column_id);
    if (broken_column) {
      grn_db_wal_recover_copy_non_reference_data_column(ctx,
                                                        broken_column,
                                                        NULL,
                                                        id_map);
      grn_obj_unref(ctx, broken_column);
    }
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  GRN_HASH_EACH_BEGIN(ctx, broken_table_ids, cursor, entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_table_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_obj *broken_table = grn_ctx_at(ctx, broken_table_id);
    if (broken_table) {
      grn_db_wal_recover_copy_reference_data_columns(ctx, broken_table, id_map);
      grn_obj_unref(ctx, broken_table);
    }
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  GRN_HASH_EACH_BEGIN(ctx, broken_column_ids, cursor, entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_column_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_obj *broken_column = grn_ctx_at(ctx, broken_column_id);
    if (broken_column) {
      grn_db_wal_recover_copy_reference_data_column(ctx,
                                                    broken_column,
                                                    NULL,
                                                    id_map);
      grn_obj_unref(ctx, broken_column);
    }
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  {
    DBWALCopyRecoverer recoverer(ctx,
                                 broken_table_ids,
                                 broken_column_ids,
                                 id_map);
    recoverer.rebuild_auto_generated_columns();
  }
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  GRN_HASH_EACH_BEGIN(ctx, broken_column_ids, cursor, entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_column_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_db_wal_recover_rename_column(ctx, db, broken_column_id, id_map);
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

  GRN_HASH_EACH_BEGIN(ctx, broken_table_ids, cursor, entry_id)
  {
    grn_ctx_push_temporary_open_space(ctx);
    grn_id broken_table_id = grn_broken_ids_cursor_get(ctx, cursor);
    grn_db_wal_recover_rename_table(ctx, db, broken_table_id, id_map);
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx) { grn_ctx_pop_temporary_open_space(ctx); }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_HASH_EACH_END(ctx, cursor);
  if (ctx->rc != GRN_SUCCESS) {
    goto exit;
  }

exit:
  grn_id_map_close(ctx, id_map);
}

static bool
grn_db_wal_recover_is_target_object(grn_ctx *ctx, grn_obj *object)
{
  if (!object) {
    return false;
  }

  if (grn_obj_is_table(ctx, object)) {
    return true;
  }
  if (grn_obj_is_column(ctx, object)) {
    return true;
  }
  return false;
}

void
grn_db_wal_recover(grn_ctx *ctx, grn_db *db)
{
  const char *tag = grn_db_wal_recover_tag;

  if (GRN_CTX_GET_WAL_ROLE(ctx) != GRN_WAL_ROLE_PRIMARY) {
    return;
  }

  GRN_LOG(ctx, GRN_LOG_DEBUG, "%s recover DB keys", tag);
  switch (db->keys->header.type) {
  case GRN_TABLE_PAT_KEY:
    grn_pat_wal_recover(ctx, (grn_pat *)(db->keys));
    break;
  case GRN_TABLE_DAT_KEY:
    grn_dat_wal_recover(ctx, (grn_dat *)(db->keys));
    break;
  default:
    break;
  }
  if (ctx->rc == GRN_SUCCESS) {
    GRN_LOG(ctx, GRN_LOG_DEBUG, "%s succeeded to recover DB keys", tag);
  } else {
    ERRCLR(ctx);
    grn_db_wal_recover_keys(ctx, db);
    if (ctx->rc != GRN_SUCCESS) {
      GRN_LOG(ctx,
              GRN_LOG_ERROR,
              "%s failed to recover DB keys: %s",
              tag,
              ctx->errbuf);
      return;
    }
  }

  GRN_LOG(ctx, GRN_LOG_DEBUG, "%s recover DB specs", tag);
  grn_ja_wal_recover(ctx, db->specs);
  if (ctx->rc == GRN_SUCCESS) {
    GRN_LOG(ctx, GRN_LOG_DEBUG, "%s succeeded to recover DB specs", tag);
  } else {
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s failed to recover DB specs: %s",
            tag,
            ctx->errbuf);
    return;
  }

  GRN_LOG(ctx, GRN_LOG_DEBUG, "%s recover DB config", tag);
  grn_hash_wal_recover(ctx, db->config);
  if (ctx->rc == GRN_SUCCESS) {
    GRN_LOG(ctx, GRN_LOG_DEBUG, "%s succeeded to recover DB config", tag);
  } else {
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s failed to recover DB config: %s",
            tag,
            ctx->errbuf);
    return;
  }

  GRN_LOG(ctx, GRN_LOG_DEBUG, "%s recover DB options", tag);
  grn_options_wal_recover(ctx, db->options);
  if (ctx->rc == GRN_SUCCESS) {
    GRN_LOG(ctx, GRN_LOG_DEBUG, "%s succeeded to recover DB options", tag);
  } else {
    GRN_LOG(ctx,
            GRN_LOG_ERROR,
            "%s failed to recover DB options: %s",
            tag,
            ctx->errbuf);
    return;
  }

  grn_db_wal_recover_remove_recovering_objects(ctx, db);
  grn_db_wal_recover_remove_broken_objects(ctx, db);

  grn_hash *broken_table_ids = grn_broken_ids_open(ctx);
  grn_hash *broken_column_ids = grn_broken_ids_open(ctx);
  GRN_TABLE_EACH_BEGIN_FLAGS(ctx, db->keys, cursor, id, GRN_CURSOR_BY_ID)
  {
    if (grn_id_is_builtin(ctx, id)) {
      continue;
    }

    grn_ctx_push_temporary_open_space(ctx);
    grn_obj *object = grn_ctx_at(ctx, id);
    if (grn_db_wal_recover_is_target_object(ctx, object)) {
      bool is_table = false;
      bool is_column = false;
      if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
        GRN_DEFINE_NAME(object);
        GRN_LOG(ctx,
                GRN_LOG_DEBUG,
                "%s recover %s: <%.*s>(%u)",
                tag,
                grn_obj_type_to_string(object->header.type),
                name_size,
                name,
                DB_OBJ(object)->id);
      }
      switch (object->header.type) {
      case GRN_TABLE_HASH_KEY:
        is_table = true;
        grn_hash_wal_recover(ctx, (grn_hash *)object);
        break;
      case GRN_TABLE_PAT_KEY:
        is_table = true;
        grn_pat_wal_recover(ctx, (grn_pat *)object);
        break;
      case GRN_TABLE_DAT_KEY:
        is_table = true;
        grn_dat_wal_recover(ctx, (grn_dat *)object);
        break;
      case GRN_TABLE_NO_KEY:
        is_table = true;
        grn_array_wal_recover(ctx, (grn_array *)object);
        break;
      case GRN_COLUMN_FIX_SIZE:
        is_column = true;
        grn_ra_wal_recover(ctx, (grn_ra *)object);
        break;
      case GRN_COLUMN_VAR_SIZE:
        is_column = true;
        grn_ja_wal_recover(ctx, (grn_ja *)object);
        break;
      case GRN_COLUMN_INDEX:
        is_column = true;
        grn_ii_wal_recover(ctx, (grn_ii *)object);
        break;
      default:
        break;
      }
      if (ctx->rc == GRN_SUCCESS) {
        if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
          GRN_DEFINE_NAME(object);
          GRN_LOG(ctx,
                  GRN_LOG_DEBUG,
                  "%s succeeded to recover %s: <%.*s>(%u)",
                  tag,
                  grn_obj_type_to_string(object->header.type),
                  name_size,
                  name,
                  DB_OBJ(object)->id);
        }
      } else {
        if (grn_logger_pass(ctx, GRN_LOG_DEBUG)) {
          GRN_DEFINE_NAME(object);
          GRN_LOG(ctx,
                  GRN_LOG_DEBUG,
                  "%s failed to recover %s: <%.*s>(%u)",
                  tag,
                  grn_obj_type_to_string(object->header.type),
                  name_size,
                  name,
                  DB_OBJ(object)->id);
        }
        if (is_table) {
          grn_broken_ids_add(ctx, broken_table_ids, id);
        } else if (is_column) {
          grn_broken_ids_add(ctx, broken_column_ids, id);
        }
        ERRCLR(ctx);
      }
    }
    GRN_DB_WAL_DISABLE_WAL_BEGIN(ctx)
    {
      if (object) {
        grn_obj_unref(ctx, object);
      }
      grn_ctx_pop_temporary_open_space(ctx);
    }
    GRN_DB_WAL_DISABLE_WAL_END(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      break;
    }
  }
  GRN_TABLE_EACH_END(ctx, cursor);

  grn_db_wal_recover_collect_related_broken_object_ids(ctx,
                                                       db,
                                                       broken_table_ids,
                                                       broken_column_ids);

  grn_db_wal_recover_copy_rename(ctx, db, broken_table_ids, broken_column_ids);

  grn_broken_ids_close(ctx, broken_table_ids);
  grn_broken_ids_close(ctx, broken_column_ids);

  grn_db_wal_recover_remove_recovering_objects(ctx, db);
  grn_db_wal_recover_remove_broken_objects(ctx, db);

  if (ctx->rc == GRN_SUCCESS) {
    GRN_LOG(ctx, GRN_LOG_DEBUG, "%s succeeded to recover", tag);
    grn_obj_flush(ctx, (grn_obj *)db);
  } else {
    GRN_LOG(ctx, GRN_LOG_ERROR, "%s failed to recover: %s", tag, ctx->errbuf);
  }
}
