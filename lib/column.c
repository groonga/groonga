/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2017 Brazil

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

#include "grn.h"
#include "grn_column.h"
#include "grn_ii.h"

grn_column_flags
grn_column_get_flags(grn_ctx *ctx, grn_obj *column)
{
  grn_column_flags flags = 0;

  GRN_API_ENTER;

  if (!column) {
    GRN_API_RETURN(0);
  }

  switch (column->header.type) {
  case GRN_COLUMN_FIX_SIZE :
    flags = column->header.flags;
    break;
  case GRN_COLUMN_VAR_SIZE :
    flags = grn_ja_get_flags(ctx, (grn_ja *)column);
    break;
  case GRN_COLUMN_INDEX :
    flags = grn_ii_get_flags(ctx, (grn_ii *)column);
    break;
  default :
    break;
  }

  GRN_API_RETURN(flags);
}

grn_column_cache *
grn_column_cache_open(grn_ctx *ctx, grn_obj *column)
{
  grn_column_cache *cache;

  GRN_API_ENTER;

  if (!column) {
    GRN_API_RETURN(NULL);
  }

  if (column->header.type != GRN_COLUMN_FIX_SIZE) {
    GRN_API_RETURN(NULL);
  }

  cache = GRN_MALLOC(sizeof(grn_column_cache));
  if (!cache) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[column-cache][open] failed to allocate memory");
    GRN_API_RETURN(NULL);
  }

  cache->ra = (grn_ra *)column;
  GRN_RA_CACHE_INIT(column, &(cache->ra_cache));

  GRN_API_RETURN(cache);
}

void
grn_column_cache_close(grn_ctx *ctx, grn_column_cache *cache)
{
  GRN_API_ENTER;

  if (!cache) {
    GRN_API_RETURN();
  }

  GRN_RA_CACHE_FIN(cache->ra, &(cache->ra_cache));
  GRN_FREE(cache);

  GRN_API_RETURN();
}

void *
grn_column_cache_ref(grn_ctx *ctx,
                     grn_column_cache *cache,
                     grn_id id,
                     size_t *value_size)
{
  void *value;

  GRN_API_ENTER;

  if (!cache) {
    GRN_API_RETURN(NULL);
  }

  value = grn_ra_ref_cache(ctx, cache->ra, id, &(cache->ra_cache));
  if (value_size) {
    if (value) {
      *value_size = cache->ra->header->element_size;
    } else {
      *value_size = 0;
    }
  }

  GRN_API_RETURN(value);
}
