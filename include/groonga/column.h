/*
  Copyright(C) 2009-2017 Brazil
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _grn_column_cache grn_column_cache;

GRN_API grn_column_flags grn_column_get_flags(grn_ctx *ctx, grn_obj *column);

GRN_API grn_column_cache *grn_column_cache_open(grn_ctx *ctx, grn_obj *column);
GRN_API void grn_column_cache_close(grn_ctx *ctx, grn_column_cache *cache);
GRN_API void *grn_column_cache_ref(grn_ctx *ctx,
                                   grn_column_cache *cache,
                                   grn_id id,
                                   size_t *value_size);

GRN_API grn_rc
grn_column_copy(grn_ctx *ctx, grn_obj *from, grn_obj *to);

#ifdef __cplusplus
}
#endif
