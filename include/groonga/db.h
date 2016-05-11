/*
  Copyright(C) 2009-2016 Brazil

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

typedef struct _grn_db_create_optarg grn_db_create_optarg;

struct _grn_db_create_optarg {
  char **builtin_type_names;
  int n_builtin_type_names;
};

GRN_API grn_obj *grn_db_create(grn_ctx *ctx, const char *path, grn_db_create_optarg *optarg);

#define GRN_DB_OPEN_OR_CREATE(ctx,path,optarg,db) \
  (((db) = grn_db_open((ctx), (path))) || (db = grn_db_create((ctx), (path), (optarg))))

GRN_API grn_obj *grn_db_open(grn_ctx *ctx, const char *path);
GRN_API void grn_db_touch(grn_ctx *ctx, grn_obj *db);
GRN_API grn_rc grn_db_recover(grn_ctx *ctx, grn_obj *db);
GRN_API grn_rc grn_db_unmap(grn_ctx *ctx, grn_obj *db);

#ifdef __cplusplus
}
#endif
