/*
  Copyright (C) 2015-2016  Brazil
  Copyright (C) 2021-2022  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_ctx_impl.h"
#include "grn_config.h"

#include <string.h>

grn_rc
grn_config_set(grn_ctx *ctx,
               const char *key, int32_t key_size,
               const char *value, int32_t value_size)
{
  grn_obj *db;
  grn_hash *config;
  grn_id id;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[config][set] DB isn't initialized");
    GRN_API_RETURN(ctx->rc);
  }

  uint32_t real_key_size;
  if (key_size < 0) {
    real_key_size = (uint32_t)strlen(key);
  } else {
    real_key_size = (uint32_t)key_size;
  }
  if (real_key_size > GRN_CONFIG_MAX_KEY_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[config][set] too large key: max=<%u>: <%u>",
        GRN_CONFIG_MAX_KEY_SIZE, real_key_size);
    GRN_API_RETURN(ctx->rc);
  }

  uint32_t real_value_size;
  if (value_size < 0) {
    real_value_size = (uint32_t)strlen(value);
  } else {
    real_value_size = (uint32_t)value_size;
  }
  if (real_value_size > GRN_CONFIG_MAX_VALUE_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[config][set] too large value: max=<%" GRN_FMT_SIZE ">: <%u>",
        GRN_CONFIG_MAX_VALUE_SIZE, real_value_size);
    GRN_API_RETURN(ctx->rc);
  }

  config = ((grn_db *)db)->config;
  {
    grn_rc rc;
    rc = grn_io_lock(ctx, config->io, grn_lock_timeout);
    if (rc != GRN_SUCCESS) {
      if (ctx->rc == GRN_SUCCESS) {
        ERR(rc, "[config][set] failed to lock");
      }
      GRN_API_RETURN(rc);
    }
    id = grn_hash_add(ctx, config, key, real_key_size, NULL, NULL);
    if (id != GRN_ID_NIL) {
      grn_obj packed_value;
      GRN_TEXT_INIT(&packed_value, 0);
      grn_bulk_reserve(ctx, &packed_value, GRN_CONFIG_MAX_VALUE_SIZE);
      GRN_UINT32_PUT(ctx, &packed_value, real_value_size);
      GRN_TEXT_PUT(ctx, &packed_value, value, real_value_size);
      GRN_TEXT_PUTC(ctx, &packed_value, '\0');
      grn_hash_set_value(ctx,
                         config,
                         id,
                         GRN_TEXT_VALUE(&packed_value),
                         GRN_OBJ_SET);
      GRN_OBJ_FIN(ctx, &packed_value);
    }
    grn_io_unlock(ctx, config->io);
  }
  if (id == GRN_ID_NIL || ctx->rc != GRN_SUCCESS) {
    ERR(GRN_INVALID_ARGUMENT,
        "[config][set] failed to set: name=<%.*s>: <%u>",
        (int)real_key_size, key, real_value_size);
  }

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_config_get(grn_ctx *ctx,
               const char *key, int32_t key_size,
               const char **value, uint32_t *value_size)
{
  grn_obj *db;
  grn_hash *config;
  grn_id id;
  void *packed_value;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[config][get] DB isn't initialized");
    GRN_API_RETURN(ctx->rc);
  }

  uint32_t real_key_size;
  if (key_size < 0) {
    real_key_size = (uint32_t)strlen(key);
  } else {
    real_key_size = (uint32_t)key_size;
  }
  if (real_key_size > GRN_CONFIG_MAX_KEY_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[config][get] too large key: max=<%u>: <%u>",
        GRN_CONFIG_MAX_KEY_SIZE, real_key_size);
    GRN_API_RETURN(ctx->rc);
  }

  config = ((grn_db *)db)->config;
  id = grn_hash_get(ctx, config, key, real_key_size, &packed_value);
  if (id == GRN_ID_NIL) {
    *value = NULL;
    *value_size = 0;
    GRN_API_RETURN(GRN_SUCCESS);
  }

  *value = (char *)packed_value + sizeof(uint32_t);
  *value_size = *((uint32_t *)packed_value);
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_config_delete(grn_ctx *ctx,
                  const char *key, int key_size)
{
  grn_obj *db;
  grn_hash *config;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[config][delete] DB isn't initialized");
    GRN_API_RETURN(ctx->rc);
  }

  uint32_t real_key_size;
  if (key_size < 0) {
    real_key_size = (uint32_t)strlen(key);
  } else {
    real_key_size = (uint32_t)key_size;
  }
  if (real_key_size > GRN_CONFIG_MAX_KEY_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[config][delete] too large key: max=<%u>: <%u>",
        GRN_CONFIG_MAX_KEY_SIZE, real_key_size);
    GRN_API_RETURN(ctx->rc);
  }

  config = ((grn_db *)db)->config;
  {
    grn_rc rc;
    rc = grn_io_lock(ctx, config->io, grn_lock_timeout);
    if (rc != GRN_SUCCESS) {
      if (ctx->rc == GRN_SUCCESS) {
        ERR(rc,
            "[config][delete] failed to lock: <%.*s>",
            (int)real_key_size, key);
      }
      GRN_API_RETURN(rc);
    }
    rc = grn_hash_delete(ctx, config, key, real_key_size, NULL);
    grn_io_unlock(ctx, config->io);
    if (rc != GRN_SUCCESS) {
      if (ctx->rc == GRN_SUCCESS) {
        ERR(rc,
            "[config][delete] failed to delete: <%.*s>",
            (int)real_key_size, key);
      }
    }
  }

  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_config_cursor_open(grn_ctx *ctx)
{
  grn_obj *db;
  grn_hash *config;
  grn_config_cursor *cursor;
  grn_id id;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[config][cursor][open] DB isn't initialized");
    GRN_API_RETURN(NULL);
  }
  config = ((grn_db *)db)->config;

  cursor = GRN_CALLOC(sizeof(grn_config_cursor));
  if (!cursor) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[config][cursor][open] failed to allocate memory for config cursor");
    GRN_API_RETURN(NULL);
  }

  GRN_DB_OBJ_SET_TYPE(cursor, GRN_CURSOR_CONFIG);
  cursor->hash_cursor = grn_hash_cursor_open(ctx, config,
                                             NULL, 0,
                                             NULL, 0,
                                             0, -1, 0);
  if (!cursor->hash_cursor) {
    GRN_FREE(cursor);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[config][cursor][open] failed to allocate memory for hash cursor");
    GRN_API_RETURN(NULL);
  }

  id = grn_obj_register(ctx, ctx->impl->db, NULL, 0);
  DB_OBJ(cursor)->header.domain = GRN_ID_NIL;
  DB_OBJ(cursor)->range = GRN_ID_NIL;
  grn_db_obj_init(ctx, ctx->impl->db, id, DB_OBJ(cursor));

  GRN_API_RETURN((grn_obj *)cursor);
}

grn_rc
grn_config_cursor_close(grn_ctx *ctx, grn_config_cursor *cursor)
{
  grn_hash_cursor_close(ctx, cursor->hash_cursor);
  GRN_FREE(cursor);

  return GRN_SUCCESS;
}

bool
grn_config_cursor_next(grn_ctx *ctx, grn_obj *cursor)
{
  bool have_next;
  grn_config_cursor *config_cursor = (grn_config_cursor *)cursor;

  GRN_API_ENTER;

  have_next = grn_hash_cursor_next(ctx, config_cursor->hash_cursor) != GRN_ID_NIL;

  GRN_API_RETURN(have_next);
}

uint32_t
grn_config_cursor_get_key(grn_ctx *ctx, grn_obj *cursor, const char **key)
{
  void *key_raw;
  uint32_t key_size;
  int key_size_raw;
  grn_config_cursor *config_cursor = (grn_config_cursor *)cursor;

  GRN_API_ENTER;

  key_size_raw = grn_hash_cursor_get_key(ctx,
                                         config_cursor->hash_cursor,
                                         &key_raw);
  key_size = (uint32_t)key_size_raw;
  *key = key_raw;

  GRN_API_RETURN(key_size);
}

uint32_t
grn_config_cursor_get_value(grn_ctx *ctx, grn_obj *cursor, const char **value)
{
  void *value_raw;
  uint32_t value_size;
  int value_size_raw;
  grn_config_cursor *config_cursor = (grn_config_cursor *)cursor;

  GRN_API_ENTER;

  value_size_raw = grn_hash_cursor_get_value(ctx,
                                             config_cursor->hash_cursor,
                                             &value_raw);
  *value = (char *)value_raw + sizeof(uint32_t);
  value_size = *((uint32_t *)value_raw);

  GRN_API_RETURN(value_size);
}

/* Deprecated since 5.1.2. Use grn_config_* instead. */
grn_rc
grn_conf_set(grn_ctx *ctx,
             const char *key, int32_t key_size,
             const char *value, int32_t value_size)
{
  return grn_config_set(ctx, key, key_size, value, value_size);
}

grn_rc
grn_conf_get(grn_ctx *ctx,
             const char *key, int32_t key_size,
             const char **value, uint32_t *value_size)
{
  return grn_config_get(ctx, key, key_size, value, value_size);
}
