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

#include "grn.h"
#include "grn_ctx.h"
#include "grn_float.h"
#include "grn_vector.h"

#include <cstring>

namespace grn {
  namespace {
    template <typename NUMERIC, typename FLOAT>
    NUMERIC
    cast_value(const char *raw_value) {
      return *reinterpret_cast<const FLOAT *>(raw_value);
    }

    template <>
    bool
    cast_value<bool, float>(const char *raw_value) {
      float value = *reinterpret_cast<const float *>(raw_value);
      return grn_float32_is_zero(value);
    }

    template <>
    bool
    cast_value<bool, double>(const char *raw_value) {
      double value = *reinterpret_cast<const double *>(raw_value);
      return grn_float_is_zero(value);
    }
  }

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

    length = grn_vector_get_element(ctx,
                                    vector,
                                    offset,
                                    &raw_value,
                                    NULL,
                                    &domain);
    if (length > 0) {
      switch (domain) {
      case GRN_DB_BOOL :
        value = *reinterpret_cast<const grn_bool *>(raw_value);
        break;
      case GRN_DB_INT8 :
        value = *reinterpret_cast<const int8_t *>(raw_value);
        break;
      case GRN_DB_UINT8 :
        value = *reinterpret_cast<const uint8_t *>(raw_value);
        break;
      case GRN_DB_INT16 :
        value = *reinterpret_cast<const int16_t *>(raw_value);
        break;
      case GRN_DB_UINT16 :
        value = *reinterpret_cast<const uint16_t *>(raw_value);
        break;
      case GRN_DB_INT32 :
        value = *reinterpret_cast<const int32_t *>(raw_value);
        break;
      case GRN_DB_UINT32 :
        value = *reinterpret_cast<const uint32_t *>(raw_value);
        break;
      case GRN_DB_INT64 :
        value = *reinterpret_cast<const int64_t *>(raw_value);
        break;
      case GRN_DB_UINT64 :
        value = *reinterpret_cast<const uint64_t *>(raw_value);
        break;
      case GRN_DB_FLOAT32 :
        value = cast_value<NUMERIC, float>(raw_value);
        break;
      case GRN_DB_FLOAT :
        value = cast_value<NUMERIC, double>(raw_value);
        break;
      default :
        break;
      }
    }

    GRN_API_RETURN(value);
  }
}

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

  typedef struct {
    grn_id id;
    uint32_t weight;
  } weight_uvector_entry;

  static unsigned int
  grn_uvector_element_size_internal(grn_ctx *ctx, grn_obj *uvector)
  {
    unsigned int element_size;

    if (grn_obj_is_weight_uvector(ctx, uvector)) {
      element_size = sizeof(weight_uvector_entry);
    } else {
      switch (uvector->header.domain) {
      case GRN_DB_BOOL :
        element_size = sizeof(grn_bool);
        break;
      case GRN_DB_INT8 :
        element_size = sizeof(int8_t);
        break;
      case GRN_DB_UINT8 :
        element_size = sizeof(uint8_t);
        break;
      case GRN_DB_INT16 :
        element_size = sizeof(int16_t);
        break;
      case GRN_DB_UINT16 :
        element_size = sizeof(uint16_t);
        break;
      case GRN_DB_INT32 :
        element_size = sizeof(int32_t);
        break;
      case GRN_DB_UINT32 :
        element_size = sizeof(uint32_t);
        break;
      case GRN_DB_INT64 :
        element_size = sizeof(int64_t);
        break;
      case GRN_DB_UINT64 :
        element_size = sizeof(uint64_t);
        break;
      case GRN_DB_FLOAT32 :
        element_size = sizeof(float);
        break;
      case GRN_DB_FLOAT :
        element_size = sizeof(double);
        break;
      case GRN_DB_TIME :
        element_size = sizeof(int64_t);
        break;
      case GRN_DB_TOKYO_GEO_POINT :
      case GRN_DB_WGS84_GEO_POINT :
        element_size = sizeof(grn_geo_point);
        break;
      default :
        element_size = sizeof(grn_id);
        break;
      }
    }

    return element_size;
  }

  static unsigned int
  grn_uvector_size_internal(grn_ctx *ctx, grn_obj *uvector)
  {
    unsigned int element_size;

    element_size = grn_uvector_element_size_internal(ctx, uvector);
    return GRN_BULK_VSIZE(uvector) / element_size;
  }

  unsigned int
  grn_vector_size(grn_ctx *ctx, grn_obj *vector)
  {
    unsigned int size;
    if (!vector) {
      ERR(GRN_INVALID_ARGUMENT, "vector is null");
      return 0;
    }
    GRN_API_ENTER;
    switch (vector->header.type) {
    case GRN_BULK :
      size = GRN_BULK_VSIZE(vector);
      break;
    case GRN_UVECTOR :
      size = grn_uvector_size_internal(ctx, vector);
      break;
    case GRN_PVECTOR :
      size = GRN_BULK_VSIZE(vector) / sizeof(grn_obj *);
      break;
    case GRN_VECTOR :
      size = vector->u.v.n_sections;
      break;
    default :
      ERR(GRN_INVALID_ARGUMENT, "not vector");
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
    case GRN_VECTOR :
      if (!v->u.v.body) {
        v->u.v.body = grn_obj_open(ctx, GRN_BULK, 0, v->header.domain);
      }
      return v->u.v.body;
    case GRN_BULK :
    case GRN_UVECTOR :
      return v;
    default :
      return NULL;
    }
  }

  unsigned int
  grn_vector_get_element(grn_ctx *ctx,
                         grn_obj *vector,
                         uint32_t offset,
                         const char **str,
                         uint32_t *weight,
                         grn_id *domain)
  {
    unsigned int length = 0;
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
      if (weight) { *weight = vp->weight; }
      if (domain) { *domain = vp->domain; }
      length = vp->length;
    }
  exit :
    GRN_API_RETURN(length);
  }

  unsigned int
  grn_vector_pop_element(grn_ctx *ctx, grn_obj *vector,
                         const char **str, uint32_t *weight, grn_id *domain)
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
      if (weight) { *weight = vp->weight; }
      if (domain) { *domain = vp->domain; }
      length = vp->length;
      grn_bulk_truncate(ctx, body, vp->offset);
    }
  exit :
    GRN_API_RETURN(length);
  }

  #define W_SECTIONS_UNIT 8
  #define S_SECTIONS_UNIT (1 << W_SECTIONS_UNIT)
  #define M_SECTIONS_UNIT (S_SECTIONS_UNIT - 1)

  grn_rc
  grn_vector_delimit(grn_ctx *ctx, grn_obj *v, uint32_t weight, grn_id domain)
  {
    if (v->header.type != GRN_VECTOR) { return GRN_INVALID_ARGUMENT; }
    if (!(v->u.v.n_sections & M_SECTIONS_UNIT)) {
      size_t size = sizeof(grn_section) * (v->u.v.n_sections + S_SECTIONS_UNIT);
      grn_section *vp =
        static_cast<grn_section *>(GRN_REALLOC(v->u.v.sections, size));
      if (!vp) { return GRN_NO_MEMORY_AVAILABLE; }
      v->u.v.sections = vp;
    }
    {
      grn_obj *body = grn_vector_body(ctx, v);
      grn_section *vp = &v->u.v.sections[v->u.v.n_sections];
      vp->offset = v->u.v.n_sections ? vp[-1].offset + vp[-1].length : 0;
      vp->length = GRN_BULK_VSIZE(body) - vp->offset;
      vp->weight = weight;
      vp->domain = domain;
    }
    v->u.v.n_sections++;
    return GRN_SUCCESS;
  }

  grn_rc
  grn_vector_decode(grn_ctx *ctx, grn_obj *v, const char *data, uint32_t data_size)
  {
    uint8_t *p = (uint8_t *)data;
    uint8_t *pe = p + data_size;
    uint32_t n, n0 = v->u.v.n_sections;
    GRN_B_DEC(n, p);
    if (((n0 + M_SECTIONS_UNIT) >> W_SECTIONS_UNIT) !=
        ((n0 + n + M_SECTIONS_UNIT) >> W_SECTIONS_UNIT)) {
      size_t size = sizeof(grn_section) *
        ((n0 + n + M_SECTIONS_UNIT) & ~M_SECTIONS_UNIT);
      grn_section *vp =
        static_cast<grn_section *>(GRN_REALLOC(v->u.v.sections, size));
      if (!vp) { return GRN_NO_MEMORY_AVAILABLE; }
      v->u.v.sections = vp;
    }
    {
      grn_section *vp;
      grn_obj *body = grn_vector_body(ctx, v);
      uint32_t offset = GRN_BULK_VSIZE(body);
      uint32_t o = 0, l, i;
      for (i = n, vp = v->u.v.sections + n0; i; i--, vp++) {
        if (pe <= p) { return GRN_INVALID_ARGUMENT; }
        GRN_B_DEC(l, p);
        vp->length = l;
        vp->offset = offset + o;
        vp->weight = 0;
        vp->domain = 0;
        o += l;
      }
      if (pe < p + o) { return GRN_INVALID_ARGUMENT; }
      grn_bulk_write(ctx, body, (char *)p, o);
      p += o;
      if (p < pe) {
        for (i = n, vp = v->u.v.sections + n0; i; i--, vp++) {
          if (pe <= p) { return GRN_INVALID_ARGUMENT; }
          GRN_B_DEC(vp->weight, p);
          GRN_B_DEC(vp->domain, p);
        }
      }
    }
    v->u.v.n_sections += n;
    return GRN_SUCCESS;
  }

  grn_rc
  grn_vector_add_element(grn_ctx *ctx, grn_obj *vector,
                         const char *str, unsigned int str_len,
                         uint32_t weight, grn_id domain)
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
  exit :
    GRN_API_RETURN(ctx->rc);
  }

  unsigned int
  grn_uvector_size(grn_ctx *ctx, grn_obj *uvector)
  {
    unsigned int size;

    if (!uvector) {
      ERR(GRN_INVALID_ARGUMENT, "uvector must not be NULL");
      return 0;
    }

    if (uvector->header.type != GRN_UVECTOR) {
      grn_obj type_name;
      GRN_TEXT_INIT(&type_name, 0);
      grn_inspect_type(ctx, &type_name, uvector->header.type);
      ERR(GRN_INVALID_ARGUMENT, "must be GRN_UVECTOR: %.*s",
          (int)GRN_TEXT_LEN(&type_name), GRN_TEXT_VALUE(&type_name));
      GRN_OBJ_FIN(ctx, &type_name);
      return 0;
    }

    GRN_API_ENTER;
    size = grn_uvector_size_internal(ctx, uvector);
    GRN_API_RETURN(size);
  }

  unsigned int
  grn_uvector_element_size(grn_ctx *ctx, grn_obj *uvector)
  {
    unsigned int element_size;

    if (!uvector) {
      ERR(GRN_INVALID_ARGUMENT, "uvector must not be NULL");
      return 0;
    }

    if (uvector->header.type != GRN_UVECTOR) {
      grn_obj type_name;
      GRN_TEXT_INIT(&type_name, 0);
      grn_inspect_type(ctx, &type_name, uvector->header.type);
      ERR(GRN_INVALID_ARGUMENT, "must be GRN_UVECTOR: %.*s",
          (int)GRN_TEXT_LEN(&type_name), GRN_TEXT_VALUE(&type_name));
      GRN_OBJ_FIN(ctx, &type_name);
      return 0;
    }

    GRN_API_ENTER;
    element_size = grn_uvector_element_size_internal(ctx, uvector);
    GRN_API_RETURN(element_size);
  }

  /* For reference uvector. We can't use this for integer family uvector
     such as Int64 uvector. */
  grn_rc
  grn_uvector_add_element(grn_ctx *ctx, grn_obj *uvector,
                          grn_id id, uint32_t weight)
  {
    GRN_API_ENTER;
    if (!uvector) {
      ERR(GRN_INVALID_ARGUMENT, "uvector is null");
      goto exit;
    }
    if (grn_obj_is_weight_uvector(ctx, uvector)) {
      weight_uvector_entry entry;
      entry.id = id;
      entry.weight = weight;
      grn_bulk_write(ctx, uvector,
                     (const char *)&entry, sizeof(weight_uvector_entry));
    } else {
      grn_bulk_write(ctx, uvector,
                     (const char *)&id, sizeof(grn_id));
    }
  exit :
    GRN_API_RETURN(ctx->rc);
  }

  /* For reference uvector. We can't use this for integer family uvector
     such as Int64 uvector. */
  grn_id
  grn_uvector_get_element(grn_ctx *ctx, grn_obj *uvector,
                          uint32_t offset, uint32_t *weight)
  {
    grn_id id = GRN_ID_NIL;

    GRN_API_ENTER;
    if (!uvector || uvector->header.type != GRN_UVECTOR) {
      ERR(GRN_INVALID_ARGUMENT, "invalid uvector");
      goto exit;
    }

    if (grn_obj_is_weight_uvector(ctx, uvector)) {
      const weight_uvector_entry *entry;
      const weight_uvector_entry *entries_start;
      const weight_uvector_entry *entries_end;

      entries_start = (const weight_uvector_entry *)GRN_BULK_HEAD(uvector);
      entries_end = (const weight_uvector_entry *)GRN_BULK_CURR(uvector);
      if (offset > entries_end - entries_start) {
        ERR(GRN_RANGE_ERROR, "offset out of range");
        goto exit;
      }

      entry = entries_start + offset;
      id = entry->id;
      if (weight) { *weight = entry->weight; }
    } else {
      const grn_id *ids_start;
      const grn_id *ids_end;

      ids_start = (const grn_id *)GRN_BULK_HEAD(uvector);
      ids_end = (const grn_id *)GRN_BULK_CURR(uvector);
      if (offset > ids_end - ids_start) {
        ERR(GRN_RANGE_ERROR, "offset out of range");
        goto exit;
      }
      id = ids_start[offset];
      if (weight) { *weight = 0; }
    }
  exit :
    GRN_API_RETURN(id);
  }
}
