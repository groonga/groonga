/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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
#include "grn_hash.h"
#include "grn_db.h"

#include <string.h>

grn_rc
grn_conf_set(grn_ctx *ctx,
             const char *key, int key_size,
             const char *value, int value_size)
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
             const char *key, int key_size,
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
