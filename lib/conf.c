/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015-2016 Brazil

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

#include "grn_ctx_impl.h"
#include "grn_conf.h"

#include <string.h>

grn_rc
grn_conf_set(grn_ctx *ctx,
             const char *key, int32_t key_size,
             const char *value, int32_t value_size)
{
  grn_obj *db;
  grn_hash *conf;
  void *packed_value;
  grn_id id;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[conf][set] DB isn't initialized");
    GRN_API_RETURN(ctx->rc);
  }

  if (key_size == -1) {
    key_size = strlen(key);
  }
  if (key_size > GRN_CONF_MAX_KEY_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[conf][set] too large key: max=<%d>: <%d>",
        GRN_CONF_MAX_KEY_SIZE, key_size);
    GRN_API_RETURN(ctx->rc);
  }

  if (value_size == -1) {
    value_size = strlen(value);
  }
  if (value_size > GRN_CONF_MAX_VALUE_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[conf][set] too large value: max=<%" GRN_FMT_SIZE ">: <%d>",
        GRN_CONF_MAX_VALUE_SIZE, value_size);
    GRN_API_RETURN(ctx->rc);
  }

  conf = ((grn_db *)db)->conf;
  {
    grn_rc rc;
    rc = grn_io_lock(ctx, conf->io, grn_lock_timeout);
    if (rc != GRN_SUCCESS) {
      if (ctx->rc == GRN_SUCCESS) {
        ERR(rc, "[conf][set] failed to lock");
      }
      GRN_API_RETURN(rc);
    }
    id = grn_hash_add(ctx, conf, key, key_size, &packed_value, NULL);
    grn_io_unlock(conf->io);
  }
  if (id == GRN_ID_NIL && ctx->rc == GRN_SUCCESS) {
    ERR(GRN_INVALID_ARGUMENT,
        "[conf][set] failed to set: name=<%.*s>: <%d>",
        key_size, key, value_size);
  }

  *((uint32_t *)packed_value) = (uint32_t)value_size;
  grn_memcpy((char *)packed_value + sizeof(uint32_t),
             value, value_size);
  ((char *)packed_value)[sizeof(uint32_t) + value_size] = '\0';

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_conf_get(grn_ctx *ctx,
             const char *key, int32_t key_size,
             const char **value, uint32_t *value_size)
{
  grn_obj *db;
  grn_hash *conf;
  grn_id id;
  void *packed_value;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[conf][get] DB isn't initialized");
    GRN_API_RETURN(ctx->rc);
  }

  if (key_size == -1) {
    key_size = strlen(key);
  }
  if (key_size > GRN_CONF_MAX_KEY_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[conf][get] too large key: max=<%d>: <%d>",
        GRN_CONF_MAX_KEY_SIZE, key_size);
    GRN_API_RETURN(ctx->rc);
  }

  conf = ((grn_db *)db)->conf;
  id = grn_hash_get(ctx, conf, key, key_size, &packed_value);
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
grn_conf_delete(grn_ctx *ctx,
                const char *key, int key_size)
{
  grn_obj *db;
  grn_hash *conf;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[conf][delete] DB isn't initialized");
    GRN_API_RETURN(ctx->rc);
  }

  if (key_size == -1) {
    key_size = strlen(key);
  }
  if (key_size > GRN_CONF_MAX_KEY_SIZE) {
    ERR(GRN_INVALID_ARGUMENT,
        "[conf][delete] too large key: max=<%d>: <%d>",
        GRN_CONF_MAX_KEY_SIZE, key_size);
    GRN_API_RETURN(ctx->rc);
  }

  conf = ((grn_db *)db)->conf;
  {
    grn_rc rc;
    rc = grn_io_lock(ctx, conf->io, grn_lock_timeout);
    if (rc != GRN_SUCCESS) {
      if (ctx->rc == GRN_SUCCESS) {
        ERR(rc, "[conf][delete] failed to lock");
      }
      GRN_API_RETURN(rc);
    }
    rc = grn_hash_delete(ctx, conf, key, key_size, NULL);
    grn_io_unlock(conf->io);
    if (rc != GRN_SUCCESS) {
      if (ctx->rc == GRN_SUCCESS) {
        ERR(rc, "[conf][delete] failed to delete");
      }
    }
  }

  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_conf_cursor_open(grn_ctx *ctx)
{
  grn_obj *db;
  grn_hash *conf;
  grn_conf_cursor *cursor;
  grn_id id;

  GRN_API_ENTER;

  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[conf][cursor][open] DB isn't initialized");
    GRN_API_RETURN(NULL);
  }
  conf = ((grn_db *)db)->conf;

  cursor = GRN_MALLOCN(grn_conf_cursor, 1);
  if (!cursor) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[conf][cursor][open] failed to allocate memory for conf cursor");
    GRN_API_RETURN(NULL);
  }

  GRN_DB_OBJ_SET_TYPE(cursor, GRN_CURSOR_CONF);
  cursor->hash_cursor = grn_hash_cursor_open(ctx, conf,
                                             NULL, -1,
                                             NULL, -1,
                                             0, -1, 0);
  if (!cursor->hash_cursor) {
    GRN_FREE(cursor);
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[conf][cursor][open] failed to allocate memory for hash cursor");
    GRN_API_RETURN(NULL);
  }

  id = grn_obj_register(ctx, ctx->impl->db, NULL, 0);
  DB_OBJ(cursor)->header.domain = GRN_ID_NIL;
  DB_OBJ(cursor)->range = GRN_ID_NIL;
  grn_db_obj_init(ctx, ctx->impl->db, id, DB_OBJ(cursor));

  GRN_API_RETURN((grn_obj *)cursor);
}

grn_rc
grn_conf_cursor_close(grn_ctx *ctx, grn_conf_cursor *cursor)
{
  grn_hash_cursor_close(ctx, cursor->hash_cursor);
  GRN_FREE(cursor);

  return GRN_SUCCESS;
}

grn_bool
grn_conf_cursor_next(grn_ctx *ctx, grn_obj *cursor)
{
  grn_bool have_next;
  grn_conf_cursor *conf_cursor = (grn_conf_cursor *)cursor;

  GRN_API_ENTER;

  have_next = grn_hash_cursor_next(ctx, conf_cursor->hash_cursor) != GRN_ID_NIL;

  GRN_API_RETURN(have_next);
}

uint32_t
grn_conf_cursor_get_key(grn_ctx *ctx, grn_obj *cursor, const char **key)
{
  void *key_raw;
  uint32_t key_size;
  grn_conf_cursor *conf_cursor = (grn_conf_cursor *)cursor;

  GRN_API_ENTER;

  key_size = grn_hash_cursor_get_key(ctx, conf_cursor->hash_cursor, &key_raw);
  *key = key_raw;

  GRN_API_RETURN(key_size);
}

uint32_t
grn_conf_cursor_get_value(grn_ctx *ctx, grn_obj *cursor, const char **value)
{
  void *value_raw;
  uint32_t value_size;
  uint32_t value_size_raw;
  grn_conf_cursor *conf_cursor = (grn_conf_cursor *)cursor;

  GRN_API_ENTER;

  value_size_raw = grn_hash_cursor_get_value(ctx,
                                             conf_cursor->hash_cursor,
                                             &value_raw);
  *value = (char *)value_raw + sizeof(uint32_t);
  value_size = *((uint32_t *)value_raw);

  GRN_API_RETURN(value_size);
}
