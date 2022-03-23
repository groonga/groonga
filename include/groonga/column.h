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

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _grn_column_cache grn_column_cache;

#define GRN_COLUMN_NAME_ID            "_id"
#define GRN_COLUMN_NAME_ID_LEN        (sizeof(GRN_COLUMN_NAME_ID) - 1)
#define GRN_COLUMN_NAME_KEY           "_key"
#define GRN_COLUMN_NAME_KEY_LEN       (sizeof(GRN_COLUMN_NAME_KEY) - 1)
#define GRN_COLUMN_NAME_VALUE         "_value"
#define GRN_COLUMN_NAME_VALUE_LEN     (sizeof(GRN_COLUMN_NAME_VALUE) - 1)
#define GRN_COLUMN_NAME_SCORE         "_score"
#define GRN_COLUMN_NAME_SCORE_LEN     (sizeof(GRN_COLUMN_NAME_SCORE) - 1)
#define GRN_COLUMN_NAME_NSUBRECS      "_nsubrecs"
#define GRN_COLUMN_NAME_NSUBRECS_LEN  (sizeof(GRN_COLUMN_NAME_NSUBRECS) - 1)
#define GRN_COLUMN_NAME_MAX           "_max"
#define GRN_COLUMN_NAME_MAX_LEN       (sizeof(GRN_COLUMN_NAME_MAX) - 1)
#define GRN_COLUMN_NAME_MIN           "_min"
#define GRN_COLUMN_NAME_MIN_LEN       (sizeof(GRN_COLUMN_NAME_MIN) - 1)
#define GRN_COLUMN_NAME_SUM           "_sum"
#define GRN_COLUMN_NAME_SUM_LEN       (sizeof(GRN_COLUMN_NAME_SUM) - 1)
/* Deprecated since 10.0.4. Use GRN_COLUMN_NAME_MEAN instead. */
#define GRN_COLUMN_NAME_AVG           "_avg"
#define GRN_COLUMN_NAME_AVG_LEN       (sizeof(GRN_COLUMN_NAME_AVG) - 1)
#define GRN_COLUMN_NAME_MEAN          "_mean"
#define GRN_COLUMN_NAME_MEAN_LEN      (sizeof(GRN_COLUMN_NAME_MEAN) - 1)

GRN_API grn_obj *grn_column_create(grn_ctx *ctx, grn_obj *table,
                                   const char *name, unsigned int name_size,
                                   const char *path, grn_column_flags flags, grn_obj *type);
GRN_API grn_obj *
grn_column_create_similar(grn_ctx *ctx,
                          grn_obj *table,
                          const char *name,
                          uint32_t name_size,
                          const char *path,
                          grn_obj *base_column);

#define GRN_COLUMN_OPEN_OR_CREATE(ctx,table,name,name_size,path,flags,type,column) \
  (((column) = grn_obj_column((ctx), (table), (name), (name_size))) ||\
   ((column) = grn_column_create((ctx), (table), (name), (name_size), (path), (flags), (type))))

GRN_API grn_rc grn_column_index_update(grn_ctx *ctx, grn_obj *column,
                                       grn_id id, unsigned int section,
                                       grn_obj *oldvalue, grn_obj *newvalue);
GRN_API grn_obj *grn_column_table(grn_ctx *ctx, grn_obj *column);
GRN_API grn_rc grn_column_truncate(grn_ctx *ctx, grn_obj *column);

GRN_API grn_column_flags grn_column_get_flags(grn_ctx *ctx, grn_obj *column);
GRN_API grn_column_flags
grn_column_get_missing_mode(grn_ctx *ctx, grn_obj *column);
GRN_API grn_column_flags
grn_column_get_invalid_mode(grn_ctx *ctx, grn_obj *column);

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
