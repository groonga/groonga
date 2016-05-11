/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2016 Brazil

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

#include "grn_cache.h"
#include "grn_ctx.h"
#include "grn_ctx_impl.h"
#include "grn_hash.h"
#include "grn_db.h"

typedef struct _grn_cache_entry grn_cache_entry;

struct _grn_cache {
  grn_cache_entry *next;
  grn_cache_entry *prev;
  grn_ctx *ctx;
  grn_hash *hash;
  grn_mutex mutex;
  uint32_t max_nentries;
  uint32_t nfetches;
  uint32_t nhits;
};

struct _grn_cache_entry {
  grn_cache_entry *next;
  grn_cache_entry *prev;
  grn_obj *value;
  grn_timeval tv;
  grn_id id;
  uint32_t nref;
};

static grn_ctx grn_cache_ctx;
static grn_cache *grn_cache_current = NULL;
static grn_cache *grn_cache_default = NULL;

grn_cache *
grn_cache_open(grn_ctx *ctx)
{
  grn_cache *cache = NULL;

  GRN_API_ENTER;
  cache = GRN_MALLOC(sizeof(grn_cache));
  if (!cache) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "[cache] failed to allocate grn_cache");
    goto exit;
  }

  cache->next = (grn_cache_entry *)cache;
  cache->prev = (grn_cache_entry *)cache;
  cache->ctx = ctx;
  cache->hash = grn_hash_create(cache->ctx, NULL, GRN_CACHE_MAX_KEY_SIZE,
                                sizeof(grn_cache_entry), GRN_OBJ_KEY_VAR_SIZE);
  if (!cache->hash) {
    ERR(GRN_NO_MEMORY_AVAILABLE, "[cache] failed to create hash table");
    GRN_FREE(cache);
    cache = NULL;
    goto exit;
  }
  MUTEX_INIT(cache->mutex);
  cache->max_nentries = GRN_CACHE_DEFAULT_MAX_N_ENTRIES;
  cache->nfetches = 0;
  cache->nhits = 0;

exit :
  GRN_API_RETURN(cache);
}

grn_rc
grn_cache_close(grn_ctx *ctx_not_used, grn_cache *cache)
{
  grn_ctx *ctx = cache->ctx;
  grn_cache_entry *vp;

  GRN_API_ENTER;

  GRN_HASH_EACH(ctx, cache->hash, id, NULL, NULL, &vp, {
    grn_obj_close(ctx, vp->value);
  });
  grn_hash_close(ctx, cache->hash);
  MUTEX_FIN(cache->mutex);
  GRN_FREE(cache);

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_cache_current_set(grn_ctx *ctx, grn_cache *cache)
{
  grn_cache_current = cache;
  return GRN_SUCCESS;
}

grn_cache *
grn_cache_current_get(grn_ctx *ctx)
{
  return grn_cache_current;
}

void
grn_cache_init(void)
{
  grn_ctx *ctx = &grn_cache_ctx;

  grn_ctx_init(ctx, 0);

  grn_cache_default = grn_cache_open(ctx);
  grn_cache_current_set(ctx, grn_cache_default);
}

grn_rc
grn_cache_set_max_n_entries(grn_ctx *ctx, grn_cache *cache, unsigned int n)
{
  uint32_t current_max_n_entries;

  if (!cache) {
    return GRN_INVALID_ARGUMENT;
  }

  current_max_n_entries = cache->max_nentries;
  cache->max_nentries = n;
  if (n < current_max_n_entries) {
    grn_cache_expire(cache, current_max_n_entries - n);
  }

  return GRN_SUCCESS;
}

uint32_t
grn_cache_get_max_n_entries(grn_ctx *ctx, grn_cache *cache)
{
  if (!cache) {
    return 0;
  }
  return cache->max_nentries;
}

void
grn_cache_get_statistics(grn_ctx *ctx, grn_cache *cache,
                         grn_cache_statistics *statistics)
{
  MUTEX_LOCK(cache->mutex);
  statistics->nentries = GRN_HASH_SIZE(cache->hash);
  statistics->max_nentries = cache->max_nentries;
  statistics->nfetches = cache->nfetches;
  statistics->nhits = cache->nhits;
  MUTEX_UNLOCK(cache->mutex);
}

static void
grn_cache_expire_entry(grn_cache *cache, grn_cache_entry *ce)
{
  if (!ce->nref) {
    ce->prev->next = ce->next;
    ce->next->prev = ce->prev;
    grn_obj_close(cache->ctx, ce->value);
    grn_hash_delete_by_id(cache->ctx, cache->hash, ce->id, NULL);
  }
}

grn_obj *
grn_cache_fetch(grn_ctx *ctx, grn_cache *cache,
                const char *str, uint32_t str_len)
{
  grn_cache_entry *ce;
  grn_obj *obj = NULL;
  if (!ctx->impl || !ctx->impl->db) { return obj; }
  MUTEX_LOCK(cache->mutex);
  cache->nfetches++;
  if (grn_hash_get(cache->ctx, cache->hash, str, str_len, (void **)&ce)) {
    if (ce->tv.tv_sec <= grn_db_get_last_modified(ctx, ctx->impl->db)) {
      grn_cache_expire_entry(cache, ce);
      goto exit;
    }
    ce->nref++;
    obj = ce->value;
    ce->prev->next = ce->next;
    ce->next->prev = ce->prev;
    {
      grn_cache_entry *ce0 = (grn_cache_entry *)cache;
      ce->next = ce0->next;
      ce->prev = ce0;
      ce0->next->prev = ce;
      ce0->next = ce;
    }
    cache->nhits++;
  }
exit :
  MUTEX_UNLOCK(cache->mutex);
  return obj;
}

void
grn_cache_unref(grn_ctx *ctx, grn_cache *cache,
                const char *str, uint32_t str_len)
{
  grn_cache_entry *ce;
  MUTEX_LOCK(cache->mutex);
  if (grn_hash_get(cache->ctx, cache->hash, str, str_len, (void **)&ce)) {
    if (ce->nref) { ce->nref--; }
  }
  MUTEX_UNLOCK(cache->mutex);
}

void
grn_cache_update(grn_ctx *ctx, grn_cache *cache,
                 const char *str, uint32_t str_len, grn_obj *value)
{
  grn_id id;
  int added = 0;
  grn_cache_entry *ce;
  grn_rc rc = GRN_SUCCESS;
  grn_obj *old = NULL;
  grn_obj *obj = NULL;

  if (!ctx->impl || !cache->max_nentries) { return; }

  MUTEX_LOCK(cache->mutex);
  obj = grn_obj_open(cache->ctx, GRN_BULK, 0, GRN_DB_TEXT);
  if (!obj) {
    goto exit;
  }
  GRN_TEXT_PUT(cache->ctx, obj, GRN_TEXT_VALUE(value), GRN_TEXT_LEN(value));
  id = grn_hash_add(cache->ctx, cache->hash, str, str_len, (void **)&ce, &added);
  if (id) {
    if (!added) {
      if (ce->nref) {
        rc = GRN_RESOURCE_BUSY;
        goto exit;
      }
      old = ce->value;
      ce->prev->next = ce->next;
      ce->next->prev = ce->prev;
    }
    ce->id = id;
    ce->value = obj;
    ce->tv = ctx->impl->tv;
    ce->nref = 0;
    {
      grn_cache_entry *ce0 = (grn_cache_entry *)cache;
      ce->next = ce0->next;
      ce->prev = ce0;
      ce0->next->prev = ce;
      ce0->next = ce;
    }
    if (GRN_HASH_SIZE(cache->hash) > cache->max_nentries) {
      grn_cache_expire_entry(cache, cache->prev);
    }
  } else {
    rc = GRN_NO_MEMORY_AVAILABLE;
  }
exit :
  if (rc) { grn_obj_close(cache->ctx, obj); }
  if (old) { grn_obj_close(cache->ctx, old); }
  MUTEX_UNLOCK(cache->mutex);
}

void
grn_cache_expire(grn_cache *cache, int32_t size)
{
  grn_cache_entry *ce0 = (grn_cache_entry *)cache;
  MUTEX_LOCK(cache->mutex);
  while (ce0 != ce0->prev && size--) {
    grn_cache_expire_entry(cache, ce0->prev);
  }
  MUTEX_UNLOCK(cache->mutex);
}

void
grn_cache_fin(void)
{
  grn_ctx *ctx = &grn_cache_ctx;

  grn_cache_current_set(ctx, NULL);

  grn_cache_close(ctx, grn_cache_default);
  grn_cache_default = NULL;

  grn_ctx_fin(ctx);
}
