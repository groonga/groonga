/*
  Copyright (C) 2018  Brazil
  Copyright (C) 2020-2023  Sutou Kouhei <kou@clear-code.com>

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
#include "grn_ctx.h"
#include "grn_float.h"
#include "grn_io.h"
#include "grn_vector.h"

#include "groonga/bulk.hpp"

#include <cstring>

namespace grn {
  namespace {
    template <typename NUMERIC, typename FLOAT>
    NUMERIC
    cast_value(const char *raw_value)
    {
      return static_cast<NUMERIC>(*reinterpret_cast<const FLOAT *>(raw_value));
    }

    template <>
    bool
    cast_value<bool, float>(const char *raw_value)
    {
      float value = *reinterpret_cast<const float *>(raw_value);
      return grn_float32_is_zero(value);
    }

    template <>
    bool
    cast_value<bool, double>(const char *raw_value)
    {
      double value = *reinterpret_cast<const double *>(raw_value);
      return grn_float_is_zero(value);
    }
  } // namespace

  template <typename NUMERIC>
  NUMERIC
  vector_get_element(grn_ctx *ctx,
                     grn_obj *vector,
                     uint32_t offset,
                     NUMERIC default_value)
  {
    const char *raw_value = NULL;
    grn_id domain;
    uint32_t length;
    NUMERIC value = default_value;

    GRN_API_ENTER;

    length =
      grn_vector_get_element(ctx, vector, offset, &raw_value, NULL, &domain);
    if (length > 0) {
      switch (domain) {
      case GRN_DB_BOOL:
        value = *reinterpret_cast<const grn_bool *>(raw_value);
        break;
      case GRN_DB_INT8:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const int8_t *>(raw_value));
        break;
      case GRN_DB_UINT8:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const uint8_t *>(raw_value));
        break;
      case GRN_DB_INT16:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const int16_t *>(raw_value));
        break;
      case GRN_DB_UINT16:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const uint16_t *>(raw_value));
        break;
      case GRN_DB_INT32:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const int32_t *>(raw_value));
        break;
      case GRN_DB_UINT32:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const uint32_t *>(raw_value));
        break;
      case GRN_DB_INT64:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const int64_t *>(raw_value));
        break;
      case GRN_DB_UINT64:
        value =
          static_cast<NUMERIC>(*reinterpret_cast<const uint64_t *>(raw_value));
        break;
      case GRN_DB_FLOAT32:
        value = cast_value<NUMERIC, float>(raw_value);
        break;
      case GRN_DB_FLOAT:
        value = cast_value<NUMERIC, double>(raw_value);
        break;
      default:
        break;
      }
    }

    GRN_API_RETURN(value);
  }
} // namespace grn

extern "C" {
bool
grn_vector_get_element_bool(grn_ctx *ctx,
                            grn_obj *vector,
                            uint32_t offset,
                            bool default_value)
{
  return grn::vector_get_element<bool>(ctx, vector, offset, default_value);
}

int8_t
grn_vector_get_element_int8(grn_ctx *ctx,
                            grn_obj *vector,
                            uint32_t offset,
                            int8_t default_value)
{
  return grn::vector_get_element<int8_t>(ctx, vector, offset, default_value);
}

uint8_t
grn_vector_get_element_uint8(grn_ctx *ctx,
                             grn_obj *vector,
                             uint32_t offset,
                             uint8_t default_value)
{
  return grn::vector_get_element<uint8_t>(ctx, vector, offset, default_value);
}

int16_t
grn_vector_get_element_int16(grn_ctx *ctx,
                             grn_obj *vector,
                             uint32_t offset,
                             int16_t default_value)
{
  return grn::vector_get_element<int16_t>(ctx, vector, offset, default_value);
}

uint16_t
grn_vector_get_element_uint16(grn_ctx *ctx,
                              grn_obj *vector,
                              uint32_t offset,
                              uint16_t default_value)
{
  return grn::vector_get_element<uint16_t>(ctx, vector, offset, default_value);
}

int32_t
grn_vector_get_element_int32(grn_ctx *ctx,
                             grn_obj *vector,
                             uint32_t offset,
                             int32_t default_value)
{
  return grn::vector_get_element<int32_t>(ctx, vector, offset, default_value);
}

uint32_t
grn_vector_get_element_uint32(grn_ctx *ctx,
                              grn_obj *vector,
                              uint32_t offset,
                              uint32_t default_value)
{
  return grn::vector_get_element<uint32_t>(ctx, vector, offset, default_value);
}

int64_t
grn_vector_get_element_int64(grn_ctx *ctx,
                             grn_obj *vector,
                             uint32_t offset,
                             int64_t default_value)
{
  return grn::vector_get_element<int64_t>(ctx, vector, offset, default_value);
}

uint64_t
grn_vector_get_element_uint64(grn_ctx *ctx,
                              grn_obj *vector,
                              uint32_t offset,
                              uint64_t default_value)
{
  return grn::vector_get_element<uint64_t>(ctx, vector, offset, default_value);
}

float
grn_vector_get_element_float32(grn_ctx *ctx,
                               grn_obj *vector,
                               uint32_t offset,
                               float default_value)
{
  return grn::vector_get_element<float>(ctx, vector, offset, default_value);
}

double
grn_vector_get_element_float64(grn_ctx *ctx,
                               grn_obj *vector,
                               uint32_t offset,
                               double default_value)
{
  return grn::vector_get_element<double>(ctx, vector, offset, default_value);
}

static uint32_t
grn_uvector_element_size_internal(grn_ctx *ctx, grn_obj *uvector)
{
  size_t element_size = grn_type_id_size(ctx, uvector->header.domain);
  if (grn_obj_is_weight_uvector(ctx, uvector)) {
#ifdef GRN_HAVE_BFLOAT16
    if (uvector->header.flags & GRN_OBJ_WEIGHT_BFLOAT16) {
      element_size += sizeof(grn_bfloat16);
    } else {
      element_size += sizeof(float);
    }
#else
    element_size += sizeof(float);
#endif
  }
  return static_cast<uint32_t>(element_size);
}

static uint32_t
grn_uvector_size_internal(grn_ctx *ctx, grn_obj *uvector)
{
  uint32_t element_size = grn_uvector_element_size_internal(ctx, uvector);
  return static_cast<uint32_t>(GRN_BULK_VSIZE(uvector) / element_size);
}

uint32_t
grn_vector_size(grn_ctx *ctx, grn_obj *vector)
{
  uint32_t size;
  if (!vector) {
    ERR(GRN_INVALID_ARGUMENT, "[vector][size] vector is null");
    return 0;
  }
  GRN_API_ENTER;
  switch (vector->header.type) {
  case GRN_BULK:
    size = static_cast<uint32_t>(GRN_BULK_VSIZE(vector));
    break;
  case GRN_UVECTOR:
    size = grn_uvector_size_internal(ctx, vector);
    break;
  case GRN_PVECTOR:
    size = static_cast<uint32_t>(GRN_BULK_VSIZE(vector) / sizeof(grn_obj *));
    break;
  case GRN_VECTOR:
    size = vector->u.v.n_sections;
    break;
  default:
    {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, vector);
      ERR(GRN_INVALID_ARGUMENT,
          "[vector][size] not vector: %.*s",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
    }
    size = 0;
    break;
  }
  GRN_API_RETURN(size);
}

grn_obj *
grn_vector_body(grn_ctx *ctx, grn_obj *v)
{
  if (!v) {
    ERR(GRN_INVALID_ARGUMENT, "invalid argument");
    return NULL;
  }
  switch (v->header.type) {
  case GRN_VECTOR:
    if (!v->u.v.body) {
      v->u.v.body = grn_obj_open(ctx, GRN_BULK, 0, v->header.domain);
    }
    return v->u.v.body;
  case GRN_BULK:
  case GRN_UVECTOR:
    return v;
  default:
    return NULL;
  }
}

uint32_t
grn_vector_get_element(grn_ctx *ctx,
                       grn_obj *vector,
                       uint32_t offset,
                       const char **str,
                       uint32_t *weight,
                       grn_id *domain)
{
  float weight_float;
  uint32_t length = grn_vector_get_element_float(ctx,
                                                 vector,
                                                 offset,
                                                 str,
                                                 &weight_float,
                                                 domain);
  if (weight) {
    *weight = static_cast<uint32_t>(weight_float);
  }
  return length;
}

uint32_t
grn_vector_get_element_float(grn_ctx *ctx,
                             grn_obj *vector,
                             uint32_t offset,
                             const char **str,
                             float *weight,
                             grn_id *domain)
{
  uint32_t length = 0;
  GRN_API_ENTER;
  if (!vector || vector->header.type != GRN_VECTOR) {
    ERR(GRN_INVALID_ARGUMENT, "invalid vector");
    goto exit;
  }
  if (vector->u.v.n_sections <= offset) {
    ERR(GRN_RANGE_ERROR, "offset out of range");
    goto exit;
  }
  {
    grn_section *vp = &vector->u.v.sections[offset];
    grn_obj *body = grn_vector_body(ctx, vector);
    *str = GRN_BULK_HEAD(body) + vp->offset;
    if (weight) {
      *weight = vp->weight;
    }
    if (domain) {
      *domain = vp->domain;
    }
    length = vp->length;
  }
exit:
  GRN_API_RETURN(length);
}

uint32_t
grn_vector_pop_element(grn_ctx *ctx,
                       grn_obj *vector,
                       const char **str,
                       uint32_t *weight,
                       grn_id *domain)
{
  float weight_float;
  uint32_t length =
    grn_vector_pop_element_float(ctx, vector, str, &weight_float, domain);
  if (weight) {
    *weight = static_cast<uint32_t>(weight_float);
  }
  return length;
}

uint32_t
grn_vector_pop_element_float(grn_ctx *ctx,
                             grn_obj *vector,
                             const char **str,
                             float *weight,
                             grn_id *domain)
{
  uint32_t offset, length = 0;
  GRN_API_ENTER;
  if (!vector || vector->header.type != GRN_VECTOR) {
    ERR(GRN_INVALID_ARGUMENT, "invalid vector");
    goto exit;
  }
  if (!vector->u.v.n_sections) {
    ERR(GRN_RANGE_ERROR, "offset out of range");
    goto exit;
  }
  offset = --vector->u.v.n_sections;
  {
    grn_section *vp = &vector->u.v.sections[offset];
    grn_obj *body = grn_vector_body(ctx, vector);
    *str = GRN_BULK_HEAD(body) + vp->offset;
    if (weight) {
      *weight = vp->weight;
    }
    if (domain) {
      *domain = vp->domain;
    }
    length = vp->length;
    grn_bulk_truncate(ctx, body, vp->offset);
  }
exit:
  GRN_API_RETURN(length);
}

#define W_SECTIONS_UNIT 8
#define S_SECTIONS_UNIT (1 << W_SECTIONS_UNIT)
#define M_SECTIONS_UNIT (S_SECTIONS_UNIT - 1)

grn_rc
grn_vector_delimit(grn_ctx *ctx, grn_obj *v, float weight, grn_id domain)
{
  if (v->header.type != GRN_VECTOR) {
    return GRN_INVALID_ARGUMENT;
  }
  if (!(v->u.v.n_sections & M_SECTIONS_UNIT)) {
    size_t size = sizeof(grn_section) * (v->u.v.n_sections + S_SECTIONS_UNIT);
    grn_section *vp =
      static_cast<grn_section *>(GRN_REALLOC(v->u.v.sections, size));
    if (!vp) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    v->u.v.sections = vp;
  }
  {
    grn_obj *body = grn_vector_body(ctx, v);
    grn_section *vp = &v->u.v.sections[v->u.v.n_sections];
    vp->offset = v->u.v.n_sections ? vp[-1].offset + vp[-1].length : 0;
    vp->length = static_cast<uint32_t>(GRN_BULK_VSIZE(body) - vp->offset);
    vp->weight = weight;
    vp->domain = domain;
  }
  v->u.v.n_sections++;
  return GRN_SUCCESS;
}

grn_obj *
grn_vector_pack(grn_ctx *ctx,
                grn_obj *vector,
                uint32_t offset,
                uint32_t n,
                grn_vector_pack_flags flags,
                grn_obj *header,
                grn_obj *footer)
{
  bool need_footer = false;
  grn_text_benc(ctx, header, n);
  for (uint32_t i = 0; i < n; ++i) {
    grn_section *section = &(vector->u.v.sections[i + offset]);
    grn_text_benc(ctx, header, section->length);
    if (section->weight > 0 || section->domain != GRN_ID_NIL) {
      need_footer = true;
    }
  }
  if (need_footer) {
    for (uint32_t i = 0; i < n; ++i) {
      grn_section *section = &(vector->u.v.sections[i + offset]);
      if (flags & GRN_VECTOR_PACK_WEIGHT_FLOAT32) {
        GRN_FLOAT32_PUT(ctx, footer, section->weight);
      } else if (flags & GRN_VECTOR_PACK_WEIGHT_BFLOAT16) {
#ifdef GRN_HAVE_BFLOAT16
        GRN_BFLOAT16_PUT(ctx,
                         footer,
                         grn::numeric::to_bfloat16(section->weight));
#else
        GRN_FLOAT32_PUT(ctx, footer, section->weight);
#endif
      } else {
        grn_text_benc(ctx, footer, static_cast<unsigned int>(section->weight));
      }
      grn_text_benc(ctx, footer, section->domain);
    }
  }
  return grn_vector_body(ctx, vector);
}

grn_rc
grn_vector_unpack(grn_ctx *ctx,
                  grn_obj *vector,
                  const uint8_t *data,
                  uint32_t data_size,
                  grn_vector_pack_flags flags,
                  uint32_t *used_size)
{
  auto start = data;
  auto p = start;
  auto pe = p + data_size;
  uint32_t n0 = vector->u.v.n_sections;
  uint32_t n;
  GRN_B_DEC(n, p);
  if (((n0 + M_SECTIONS_UNIT) >> W_SECTIONS_UNIT) !=
      ((n0 + n + M_SECTIONS_UNIT) >> W_SECTIONS_UNIT)) {
    size_t size =
      sizeof(grn_section) * ((n0 + n + M_SECTIONS_UNIT) & ~M_SECTIONS_UNIT);
    grn_section *sections =
      static_cast<grn_section *>(GRN_REALLOC(vector->u.v.sections, size));
    if (!sections) {
      return GRN_NO_MEMORY_AVAILABLE;
    }
    vector->u.v.sections = sections;
  }
  {
    grn_obj *body = grn_vector_body(ctx, vector);
    uint32_t offset = static_cast<uint32_t>(GRN_BULK_VSIZE(body));
    uint32_t o = 0;
    for (uint32_t i = 0; i < n; ++i) {
      grn_section *section = vector->u.v.sections + n0 + i;
      if (pe <= p) {
        return GRN_INVALID_ARGUMENT;
      }
      uint32_t length;
      GRN_B_DEC(length, p);
      section->length = length;
      section->offset = offset + o;
      section->weight = 0;
      section->domain = 0;
      o += length;
    }
    if (pe < p + o) {
      return GRN_INVALID_ARGUMENT;
    }
    grn_bulk_write(ctx, body, (char *)p, o);
    p += o;
    if (p < pe) {
      for (uint32_t i = 0; i < n; ++i) {
        grn_section *section = vector->u.v.sections + n0 + i;
        if (pe <= p) {
          return GRN_INVALID_ARGUMENT;
        }
        if (flags & GRN_VECTOR_PACK_WEIGHT_FLOAT32) {
          grn_memcpy(&(section->weight), p, sizeof(float));
          p += sizeof(float);
        } else if (flags & GRN_VECTOR_PACK_WEIGHT_BFLOAT16) {
#ifdef GRN_HAVE_BFLOAT16
          grn_bfloat16 weight_bfloat16;
          grn_memcpy(&weight_bfloat16, p, sizeof(grn_bfloat16));
          p += sizeof(grn_bfloat16);
          section->weight = grn_bfloat16_to_float32(weight_bfloat16);
#else
          grn_memcpy(&(section->weight), p, sizeof(float));
          p += sizeof(float);
#endif
        } else {
          uint32_t weight;
          GRN_B_DEC(weight, p);
          section->weight = static_cast<float>(weight);
        }
        GRN_B_DEC(section->domain, p);
      }
    }
  }
  vector->u.v.n_sections += n;
  if (used_size) {
    *used_size = p - start;
  }
  return GRN_SUCCESS;
}

grn_rc
grn_vector_add_element(grn_ctx *ctx,
                       grn_obj *vector,
                       const char *str,
                       uint32_t str_len,
                       uint32_t weight,
                       grn_id domain)
{
  return grn_vector_add_element_float(ctx,
                                      vector,
                                      str,
                                      str_len,
                                      static_cast<float>(weight),
                                      domain);
}

grn_rc
grn_vector_add_element_float(grn_ctx *ctx,
                             grn_obj *vector,
                             const char *str,
                             uint32_t str_len,
                             float weight,
                             grn_id domain)
{
  grn_obj *body;
  GRN_API_ENTER;
  if (!vector) {
    ERR(GRN_INVALID_ARGUMENT, "vector is null");
    goto exit;
  }
  if ((body = grn_vector_body(ctx, vector))) {
    grn_bulk_write(ctx, body, str, str_len);
    grn_vector_delimit(ctx, vector, weight, domain);
  }
exit:
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_vector_copy(grn_ctx *ctx, grn_obj *src, grn_obj *dest)
{
  GRN_API_ENTER;
  uint32_t n_elements = grn_vector_size(ctx, src);
  uint32_t i;
  for (i = 0; i < n_elements; i++) {
    const char *content;
    float weight;
    grn_id domain;
    uint32_t content_size =
      grn_vector_get_element_float(ctx, src, i, &content, &weight, &domain);
    grn_vector_add_element_float(ctx,
                                 dest,
                                 content,
                                 content_size,
                                 weight,
                                 domain);
  }
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_vector_join(grn_ctx *ctx,
                grn_obj *vector,
                const char *separator,
                int separator_length,
                grn_obj *destination)
{
  GRN_API_ENTER;
  if (!vector) {
    ERR(GRN_INVALID_ARGUMENT, "[vector][join] vector is NULL");
    GRN_API_RETURN(NULL);
  }
  if (vector->header.type != GRN_VECTOR) {
    grn_obj type_name;
    GRN_TEXT_INIT(&type_name, 0);
    grn_inspect_type(ctx, &type_name, vector->header.type);
    ERR(GRN_INVALID_ARGUMENT,
        "[vector][join] must be GRN_UVECTOR: %.*s",
        (int)GRN_TEXT_LEN(&type_name),
        GRN_TEXT_VALUE(&type_name));
    GRN_OBJ_FIN(ctx, &type_name);
    GRN_API_RETURN(NULL);
  }
  if (separator_length < 0) {
    separator_length = (int)strlen(separator);
  }
  if (!destination) {
    destination = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  }
  uint32_t n_elements = grn_vector_size(ctx, vector);
  uint32_t i;
  grn_obj element;
  GRN_TEXT_INIT(&element, GRN_OBJ_DO_SHALLOW_COPY);
  for (i = 0; i < n_elements; i++) {
    if (i > 0 && separator_length > 0) {
      GRN_TEXT_PUT(ctx, destination, separator, separator_length);
    }
    const char *content;
    grn_id domain;
    uint32_t content_size =
      grn_vector_get_element_float(ctx, vector, i, &content, NULL, &domain);
    GRN_TEXT_SET(ctx, &element, content, content_size);
    element.header.domain = domain;
    grn_obj_cast(ctx, &element, destination, false);
    element.header.domain = GRN_DB_TEXT;
  }
  GRN_OBJ_FIN(ctx, &element);
  GRN_API_RETURN(destination);
}

uint32_t
grn_uvector_size(grn_ctx *ctx, grn_obj *uvector)
{
  if (!uvector) {
    ERR(GRN_INVALID_ARGUMENT, "uvector must not be NULL");
    return 0;
  }

  if (uvector->header.type != GRN_UVECTOR) {
    grn_obj type_name;
    GRN_TEXT_INIT(&type_name, 0);
    grn_inspect_type(ctx, &type_name, uvector->header.type);
    ERR(GRN_INVALID_ARGUMENT,
        "must be GRN_UVECTOR: %.*s",
        (int)GRN_TEXT_LEN(&type_name),
        GRN_TEXT_VALUE(&type_name));
    GRN_OBJ_FIN(ctx, &type_name);
    return 0;
  }

  GRN_API_ENTER;
  size_t size = grn_uvector_size_internal(ctx, uvector);
  GRN_API_RETURN(static_cast<uint32_t>(size));
}

uint32_t
grn_uvector_element_size(grn_ctx *ctx, grn_obj *uvector)
{
  if (!uvector) {
    ERR(GRN_INVALID_ARGUMENT, "uvector must not be NULL");
    return 0;
  }

  if (uvector->header.type != GRN_UVECTOR) {
    grn_obj type_name;
    GRN_TEXT_INIT(&type_name, 0);
    grn_inspect_type(ctx, &type_name, uvector->header.type);
    ERR(GRN_INVALID_ARGUMENT,
        "must be GRN_UVECTOR: %.*s",
        (int)GRN_TEXT_LEN(&type_name),
        GRN_TEXT_VALUE(&type_name));
    GRN_OBJ_FIN(ctx, &type_name);
    return 0;
  }

  GRN_API_ENTER;
  uint32_t element_size = grn_uvector_element_size_internal(ctx, uvector);
  GRN_API_RETURN(element_size);
}

/* For reference uvector. We can't use this for integer family uvector
   such as Int64 uvector. */
grn_rc
grn_uvector_add_element(grn_ctx *ctx,
                        grn_obj *uvector,
                        grn_id id,
                        uint32_t weight)
{
  return grn_uvector_add_element_record(ctx,
                                        uvector,
                                        id,
                                        static_cast<float>(weight));
}

grn_rc
grn_uvector_add_element_record(grn_ctx *ctx,
                               grn_obj *uvector,
                               grn_id id,
                               float weight)
{
  GRN_API_ENTER;
  if (!uvector) {
    ERR(GRN_INVALID_ARGUMENT, "[uvector][add-element][record] uvector is null");
    goto exit;
  }
  GRN_RECORD_PUT(ctx, uvector, id);
  if (grn_obj_is_weight_uvector(ctx, uvector)) {
    if (uvector->header.flags & GRN_OBJ_WEIGHT_BFLOAT16) {
#ifdef GRN_HAVE_BFLOAT16
      GRN_BFLOAT16_PUT(ctx, uvector, grn::numeric::to_bfloat16(weight));
#else
      GRN_FLOAT32_PUT(ctx, uvector, weight);
#endif
    } else {
      GRN_FLOAT32_PUT(ctx, uvector, weight);
    }
  }
exit:
  GRN_API_RETURN(ctx->rc);
}

/* For reference uvector. We can't use this for integer family uvector
   such as Int64 uvector. */
grn_id
grn_uvector_get_element(grn_ctx *ctx,
                        grn_obj *uvector,
                        uint32_t offset,
                        uint32_t *weight)
{
  float weight_float;
  grn_id id =
    grn_uvector_get_element_record(ctx, uvector, offset, &weight_float);
  if (weight) {
    *weight = static_cast<uint32_t>(weight_float);
  }
  return id;
}

grn_id
grn_uvector_get_element_record(grn_ctx *ctx,
                               grn_obj *uvector,
                               uint32_t offset,
                               float *weight)
{
  grn_id id = GRN_ID_NIL;

  GRN_API_ENTER;
  if (!uvector) {
    ERR(GRN_INVALID_ARGUMENT, "[uvector][get-element][record] uvector is null");
    goto exit;
  }
  if (uvector->header.type != GRN_UVECTOR) {
    ERR(GRN_INVALID_ARGUMENT, "[uvector][get-element][record] invalid uvector");
    goto exit;
  }

  {
    size_t element_value_size = sizeof(grn_id);
    size_t element_size = element_value_size;
    if (grn_obj_is_weight_uvector(ctx, uvector)) {
      if (uvector->header.flags & GRN_OBJ_WEIGHT_BFLOAT16) {
#ifdef GRN_HAVE_BFLOAT16
        element_size += sizeof(grn_bfloat16);
#else
        element_size += sizeof(float);
#endif
      } else {
        element_size += sizeof(float);
      }
    }
    const char *elements_start = GRN_BULK_HEAD(uvector);
    const char *elements_end = GRN_BULK_CURR(uvector);
    if (offset > elements_end - elements_start) {
      ERR(GRN_RANGE_ERROR,
          "[uvector][get-element][record] "
          "offset out of range: <%u>/<%" GRN_FMT_SIZE ">",
          offset,
          elements_end - elements_start);
      goto exit;
    }

    id = *((grn_id *)(elements_start + (element_size * offset)));
    if (weight) {
      if (grn_obj_is_weight_uvector(ctx, uvector)) {
        if (uvector->header.flags & GRN_OBJ_WEIGHT_BFLOAT16) {
#ifdef GRN_HAVE_BFLOAT16
          grn_bfloat16 weight_bfloat16 =
            *reinterpret_cast<const grn_bfloat16 *>(
              elements_start + (element_size * offset) + element_value_size);
          *weight = grn_bfloat16_to_float32(weight_bfloat16);
#else
          *weight = *reinterpret_cast<const float *>(
            elements_start + (element_size * offset) + element_value_size);
#endif
        } else {
          *weight = *reinterpret_cast<const float *>(
            elements_start + (element_size * offset) + element_value_size);
        }
      } else {
        *weight = 0.0;
      }
    }
  }
exit:
  GRN_API_RETURN(id);
}

grn_rc
grn_uvector_copy(grn_ctx *ctx, grn_obj *src, grn_obj *dest)
{
  GRN_API_ENTER;
  if (src->header.domain != dest->header.domain) {
    ERR(GRN_INVALID_ARGUMENT,
        "[uvector][copy] different domain: "
        "source=<%u> "
        "destination=<%u>",
        src->header.domain,
        dest->header.domain);
    GRN_API_RETURN(ctx->rc);
  }
  bool src_have_weight = grn_obj_is_weight_uvector(ctx, src);
  bool dest_have_weight = grn_obj_is_weight_uvector(ctx, dest);
  if (src_have_weight != dest_have_weight) {
    ERR(GRN_INVALID_ARGUMENT,
        "[uvector][copy] different weight availability: "
        "source=<%s> "
        "destination=<%s>",
        src_have_weight ? "true" : "false",
        dest_have_weight ? "true" : "false");
    GRN_API_RETURN(ctx->rc);
  }
  grn_bulk_write(ctx, dest, GRN_BULK_HEAD(src), GRN_BULK_VSIZE(src));
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_uvector_join(grn_ctx *ctx,
                 grn_obj *uvector,
                 const char *separator,
                 int separator_length,
                 grn_obj *destination)
{
  GRN_API_ENTER;
  if (!uvector) {
    ERR(GRN_INVALID_ARGUMENT, "[uvector][join] uvector is NULL");
    GRN_API_RETURN(NULL);
  }
  if (uvector->header.type != GRN_UVECTOR) {
    grn_obj type_name;
    GRN_TEXT_INIT(&type_name, 0);
    grn_inspect_type(ctx, &type_name, uvector->header.type);
    ERR(GRN_INVALID_ARGUMENT,
        "[uvector][join] must be GRN_UVECTOR: %.*s",
        (int)GRN_TEXT_LEN(&type_name),
        GRN_TEXT_VALUE(&type_name));
    GRN_OBJ_FIN(ctx, &type_name);
    GRN_API_RETURN(NULL);
  }
  if (separator_length < 0) {
    separator_length = (int)strlen(separator);
  }
  if (!destination) {
    destination = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  }
  uint32_t element_size = grn_uvector_element_size_internal(ctx, uvector);
  uint32_t element_content_size = element_size;
  if (grn_obj_is_weight_uvector(ctx, uvector)) {
    if (uvector->header.flags & GRN_OBJ_WEIGHT_BFLOAT16) {
#ifdef GRN_HAVE_BFLOAT16
      element_content_size -= static_cast<uint32_t>(sizeof(grn_bfloat16));
#else
      element_content_size -= static_cast<uint32_t>(sizeof(float));
#endif
    } else {
      element_content_size -= static_cast<uint32_t>(sizeof(float));
    }
  }
  uint32_t n_elements = grn_uvector_size_internal(ctx, uvector);
  uint32_t i;
  grn_obj element;
  GRN_VALUE_FIX_SIZE_INIT(&element,
                          GRN_OBJ_DO_SHALLOW_COPY,
                          uvector->header.domain);
  for (i = 0; i < n_elements; i++) {
    if (i > 0 && separator_length > 0) {
      GRN_TEXT_PUT(ctx, destination, separator, separator_length);
    }
    GRN_TEXT_SET(ctx,
                 &element,
                 GRN_BULK_HEAD(uvector) + (element_size * i),
                 element_content_size);
    grn_obj_cast(ctx, &element, destination, false);
  }
  GRN_OBJ_FIN(ctx, &element);
  GRN_API_RETURN(destination);
}

grn_obj *
grn_pvector_join(grn_ctx *ctx,
                 grn_obj *pvector,
                 const char *separator,
                 int separator_length,
                 grn_obj *destination)
{
  GRN_API_ENTER;
  if (!pvector) {
    ERR(GRN_INVALID_ARGUMENT, "[pvector][join] pvector is NULL");
    GRN_API_RETURN(NULL);
  }
  if (pvector->header.type != GRN_PVECTOR) {
    grn_obj type_name;
    GRN_TEXT_INIT(&type_name, 0);
    grn_inspect_type(ctx, &type_name, pvector->header.type);
    ERR(GRN_INVALID_ARGUMENT,
        "[pvector][join] must be GRN_PVECTOR: %.*s",
        (int)GRN_TEXT_LEN(&type_name),
        GRN_TEXT_VALUE(&type_name));
    GRN_OBJ_FIN(ctx, &type_name);
    GRN_API_RETURN(NULL);
  }
  if (separator_length < 0) {
    separator_length = (int)strlen(separator);
  }
  if (!destination) {
    destination = grn_obj_open(ctx, GRN_BULK, 0, GRN_DB_TEXT);
  }
  size_t n_elements = GRN_PTR_VECTOR_SIZE(pvector);
  size_t i;
  for (i = 0; i < n_elements; i++) {
    if (i > 0 && separator_length > 0) {
      GRN_TEXT_PUT(ctx, destination, separator, separator_length);
    }
    grn_obj *element = GRN_PTR_VALUE_AT(pvector, i);
    grn_obj_cast(ctx, element, destination, false);
  }
  GRN_API_RETURN(destination);
}
}
