/*
  Copyright(C) 2009-2016  Brazil
  Copyright(C) 2020  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_db.h"
#include "grn_type.h"

grn_bool
grn_type_id_is_builtin(grn_ctx *ctx, grn_id id)
{
  return id >= GRN_DB_OBJECT && id <= GRN_DB_FLOAT32;
}

grn_bool
grn_type_id_is_number_family(grn_ctx *ctx, grn_id id)
{
  return (GRN_DB_INT8 <= id && id <= GRN_DB_FLOAT) || (id == GRN_DB_FLOAT32);
}

bool
grn_type_id_is_float_family(grn_ctx *ctx, grn_id id)
{
  switch (id) {
  case GRN_DB_FLOAT32 :
  case GRN_DB_FLOAT :
    return true;
  default :
    return false;
  }
}

grn_bool
grn_type_id_is_text_family(grn_ctx *ctx, grn_id id)
{
  return GRN_DB_SHORT_TEXT <= id && id <= GRN_DB_LONG_TEXT;
}

size_t
grn_type_id_size(grn_ctx *ctx, grn_id id)
{
  switch (id) {
  case GRN_DB_BOOL :
    return sizeof(bool);
  case GRN_DB_INT8 :
    return sizeof(int8_t);
  case GRN_DB_UINT8 :
    return sizeof(uint8_t);
  case GRN_DB_INT16 :
    return sizeof(int16_t);
  case GRN_DB_UINT16 :
    return sizeof(uint16_t);
  case GRN_DB_INT32 :
    return sizeof(int32_t);
  case GRN_DB_UINT32 :
    return sizeof(uint32_t);
  case GRN_DB_INT64 :
    return sizeof(int64_t);
  case GRN_DB_UINT64 :
    return sizeof(uint64_t);
  case GRN_DB_FLOAT32 :
    return sizeof(float);
  case GRN_DB_FLOAT :
    return sizeof(double);
  case GRN_DB_TIME :
    return sizeof(int64_t);
  case GRN_DB_SHORT_TEXT :
    return GRN_TYPE_SHORT_TEXT_SIZE;
  case GRN_DB_TEXT :
    return GRN_TYPE_TEXT_SIZE;
  case GRN_DB_LONG_TEXT :
    return GRN_TYPE_LONG_TEXT_SIZE;
  case GRN_DB_TOKYO_GEO_POINT :
  case GRN_DB_WGS84_GEO_POINT :
    return sizeof(grn_geo_point);
  default :
    {
      GRN_API_ENTER;
      size_t size = 0;
      if (grn_type_id_is_builtin(ctx, id)) {
        grn_obj *obj = grn_ctx_at(ctx, id);
        if (grn_obj_is_type(ctx, obj)) {
          size = grn_type_size(ctx, obj);
        }
        grn_obj_unref(ctx, obj);
      } else {
        /*
          We need to check the ID is a ID for table:

            grn_obj *obj = grn_ctx_at(ctx, id);
            if (grn_obj_is_table(ctx, obj)) {
              size = sizeof(grn_id);
            }
            grn_obj_unref(ctx, obj);

          But we want to reduce grn_ctx_at() call for performance.
          So we assume the ID is a ID for table here.
        */
        size = sizeof(grn_id);
      }
      GRN_API_RETURN(size);
    }
  }
}

grn_obj *
grn_type_create(grn_ctx *ctx, const char *name, unsigned int name_size,
                grn_obj_flags flags, unsigned int size)
{
  grn_obj *db;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "[type][create] DB is not initialized");
    return NULL;
  }
  GRN_API_ENTER;
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR("[type][create]", name, name_size);
    GRN_API_RETURN(NULL);
  }
  grn_id id = grn_obj_register(ctx, db, name, name_size);
  if (id == GRN_ID_NIL) {
    ERR(GRN_INVALID_ARGUMENT,
        "[type][create] failed to register type: <%.*s>",
        (int)name_size,
        name);
    GRN_API_RETURN(NULL);
  }
  grn_obj *type = grn_type_create_internal(ctx, id, flags, size);
  GRN_API_RETURN(type);
}

grn_obj *
grn_type_create_internal(grn_ctx *ctx,
                         grn_id id,
                         grn_obj_flags flags,
                         unsigned int size)
{
  GRN_API_ENTER;
  struct _grn_type *type = GRN_MALLOC(sizeof(grn_db_obj));
  if (!type) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[type][init] failed to allocate type: <%u>",
        id);
    GRN_API_RETURN(NULL);
  }
  GRN_DB_OBJ_SET_TYPE(type, GRN_TYPE);
  type->obj.header.flags = flags;
  type->obj.header.domain = GRN_ID_NIL;
  GRN_TYPE_SIZE(&(type->obj)) = size;
  if (grn_db_obj_init(ctx, ctx->impl->db, id, DB_OBJ(type))) {
    GRN_FREE(type);
    GRN_API_RETURN(NULL);
  }
  GRN_API_RETURN((grn_obj *)type);
}

uint32_t
grn_type_size(grn_ctx *ctx, grn_obj *type)
{
  uint32_t size;

  GRN_API_ENTER;
  if (!type) {
    ERR(GRN_INVALID_ARGUMENT, "[type][size] type is NULL");
    GRN_API_RETURN(0);
  }
  size = GRN_TYPE_SIZE(DB_OBJ(type));
  GRN_API_RETURN(size);
}
