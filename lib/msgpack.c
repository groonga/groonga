/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2018  Brazil
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

#include "grn_ctx.h"
#include "grn_db.h"
#include "grn_msgpack.h"

#ifdef GRN_WITH_MESSAGE_PACK

# include <groonga/msgpack.h>

grn_rc
grn_msgpack_pack_raw_internal(grn_ctx *ctx,
                              msgpack_packer *packer,
                              const char *value,
                              unsigned int value_size,
                              grn_id value_domain)
{
  if (value_size == 0) {
    msgpack_pack_nil(packer);
    return ctx->rc;
  }

  switch (value_domain) {
  case GRN_DB_VOID :
    msgpack_pack_nil(packer);
    break;
  case GRN_DB_BOOL :
    if (*((grn_bool *)value)) {
      msgpack_pack_true(packer);
    } else {
      msgpack_pack_false(packer);
    }
    break;
  case GRN_DB_INT8 :
    msgpack_pack_int8(packer, *((int8_t *)(value)));
    break;
  case GRN_DB_UINT8 :
    msgpack_pack_uint8(packer, *((uint8_t *)(value)));
    break;
  case GRN_DB_INT16 :
    msgpack_pack_int16(packer, *((int16_t *)(value)));
    break;
  case GRN_DB_UINT16 :
    msgpack_pack_uint16(packer, *((uint16_t *)(value)));
    break;
  case GRN_DB_INT32 :
    msgpack_pack_int32(packer, *((int32_t *)(value)));
    break;
  case GRN_DB_UINT32 :
    msgpack_pack_uint32(packer, *((uint32_t *)(value)));
    break;
  case GRN_DB_INT64 :
    msgpack_pack_int64(packer, *((int64_t *)(value)));
    break;
  case GRN_DB_UINT64 :
    msgpack_pack_uint64(packer, *((uint64_t *)(value)));
    break;
  case GRN_DB_FLOAT32 :
    msgpack_pack_float(packer, *((float *)(value)));
    break;
  case GRN_DB_FLOAT :
    msgpack_pack_double(packer, *((double *)(value)));
    break;
  case GRN_DB_TIME :
# if MSGPACK_VERSION_MAJOR < 1
    {
      double time_value = (*((int64_t *)value) / GRN_TIME_USEC_PER_SEC_F);
      msgpack_pack_double(packer, time_value);
    }
# else /* MSGPACK_VERSION_MAJOR < 1 */
    /* TODO: Use timestamp time in spec. */
    msgpack_pack_ext(packer, sizeof(int64_t), GRN_MSGPACK_OBJECT_EXT_TIME);
    msgpack_pack_ext_body(packer, value, sizeof(int64_t));
# endif /* MSGPACK_VERSION_MAJOR < 1 */
    break;
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    msgpack_pack_str(packer, value_size);
    msgpack_pack_str_body(packer, value, value_size);
    break;
  default :
    {
      char domain_name[GRN_TABLE_MAX_KEY_SIZE];
      int domain_name_size;

      domain_name_size = grn_table_get_key(ctx,
                                           grn_ctx_db(ctx),
                                           value_domain,
                                           domain_name,
                                           sizeof(domain_name));
      ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
          "[msgpack][pack] unsupported type: <%.*s>",
          domain_name_size, domain_name);
    }
    break;
  }

  return ctx->rc;
}

grn_rc
grn_msgpack_pack_internal(grn_ctx *ctx,
                          msgpack_packer *packer,
                          grn_obj *value)
{
  if (value) {
    return grn_msgpack_pack_raw_internal(ctx,
                                         packer,
                                         GRN_BULK_HEAD(value),
                                         GRN_BULK_VSIZE(value),
                                         value->header.domain);
  } else {
    return grn_msgpack_pack_raw_internal(ctx,
                                         packer,
                                         NULL,
                                         0,
                                         GRN_DB_VOID);
  }
}

grn_rc
grn_msgpack_unpack_array_internal(grn_ctx *ctx,
                                  msgpack_object_array *array,
                                  grn_obj *vector)
{
  int32_t i;

  grn_obj_ensure_vector(ctx, vector);

  for (i = 0; i < array->size && ctx->rc == GRN_SUCCESS; i++) {
    msgpack_object *element;

    element = &(array->ptr[i]);
    switch (element->type) {
    case MSGPACK_OBJECT_BOOLEAN :
      {
        grn_bool value = element->via.boolean;
        grn_vector_add_element(ctx,
                               vector,
                               (const char *)&value,
                               sizeof(grn_bool),
                               0,
                               GRN_DB_BOOL);
      }
      break;
    case MSGPACK_OBJECT_POSITIVE_INTEGER :
      grn_vector_add_element(ctx,
                             vector,
                             (const char *)&(element->via.i64),
                             sizeof(int64_t),
                             0,
                             GRN_DB_INT64);
      break;
    case MSGPACK_OBJECT_NEGATIVE_INTEGER :
      grn_vector_add_element(ctx,
                             vector,
                             (const char *)&(element->via.u64),
                             sizeof(uint64_t),
                             0,
                             GRN_DB_UINT64);
      break;
    case MSGPACK_OBJECT_FLOAT32 :
      grn_vector_add_element(ctx,
                             vector,
                             (const char *)&(MSGPACK_OBJECT_FLOAT32_VALUE(element)),
                             sizeof(float),
                             0,
                             GRN_DB_FLOAT32);
      break;
    case MSGPACK_OBJECT_FLOAT64 :
      grn_vector_add_element(ctx,
                             vector,
                             (const char *)&(MSGPACK_OBJECT_FLOAT64_VALUE(element)),
                             sizeof(double),
                             0,
                             GRN_DB_FLOAT);
      break;
    case MSGPACK_OBJECT_STR :
      grn_vector_add_element(ctx,
                             vector,
                             MSGPACK_OBJECT_STR_PTR(element),
                             MSGPACK_OBJECT_STR_SIZE(element),
                             0,
                             GRN_DB_TEXT);
      break;
# if MSGPACK_VERSION_MAJOR >= 1
    case MSGPACK_OBJECT_EXT :
      if (element->via.ext.type == GRN_MSGPACK_OBJECT_EXT_TIME) {
        grn_vector_add_element(ctx,
                               vector,
                               element->via.ext.ptr,
                               sizeof(int64_t),
                               0,
                               GRN_DB_TIME);
      } else {
        ERR(GRN_INVALID_ARGUMENT,
            "[msgpack] unknown extension type: <%u>",
            element->via.ext.type);
      }
      break;
# endif /* MSGPACK_VERSION_MAJOR >= 1 */
    default :
      ERR(GRN_INVALID_ARGUMENT,
          "[msgpack] unexpected element type: <%#x>",
          element->type);
      break;
    }
  }

  return ctx->rc;
}

# if MSGPACK_VERSION_MAJOR >= 1
int64_t
grn_msgpack_unpack_ext_time_internal(grn_ctx *ctx,
                                     msgpack_object_ext *ext)
{
  if (ext->type == GRN_MSGPACK_OBJECT_EXT_TIME) {
    return *(int64_t *)(ext->ptr);
  } else {
    ERR(GRN_INVALID_ARGUMENT,
        "[msgpack] time extension type must be <%u>: <%u>",
        GRN_MSGPACK_OBJECT_EXT_TIME,
        ext->type);
    return 0;
  }
}
# endif /* MSGPACK_VERSION_MAJOR >= 1 */

grn_rc
grn_msgpack_pack_raw(grn_ctx *ctx,
                     msgpack_packer *packer,
                     const char *value,
                     unsigned int value_size,
                     grn_id value_domain)
{
  GRN_API_ENTER;
  grn_msgpack_pack_raw_internal(ctx, packer, value, value_size, value_domain);
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_msgpack_pack(grn_ctx *ctx,
                 msgpack_packer *packer,
                 grn_obj *value)
{
  GRN_API_ENTER;
  grn_msgpack_pack_internal(ctx, packer, value);
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_msgpack_unpack_array(grn_ctx *ctx,
                         msgpack_object_array *array,
                         grn_obj *vector)
{
  GRN_API_ENTER;
  grn_msgpack_unpack_array_internal(ctx, array, vector);
  GRN_API_RETURN(ctx->rc);
}

# if MSGPACK_VERSION_MAJOR >= 1
int64_t
grn_msgpack_unpack_ext_time(grn_ctx *ctx,
                            msgpack_object_ext *ext)
{
  int64_t time;
  GRN_API_ENTER;
  time = grn_msgpack_unpack_ext_time_internal(ctx, ext);
  GRN_API_RETURN(time);
}
# endif /* MSGPACK_VERSION_MAJOR >= 1 */
#endif /* GRN_WITH_MESSAGE_PACK */
