/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_distance.hpp"

#include <groonga/bulk.hpp>

#ifdef GRN_WITH_XSIMD
#  include <xsimd/xsimd.hpp>
#endif

namespace grn {
  namespace distance {
#ifdef GRN_WITH_XSIMD
    bool use_simd = true;
#else
    bool use_simd = false;
#endif
  } // namespace distance
} // namespace grn

namespace {
  bool
  validate_vectors(grn_ctx *ctx,
                   grn_obj *vector1,
                   grn_obj *vector2,
                   const char *tag)
  {
    if (!grn_obj_is_number_family_vector(ctx, vector1)) {
      grn::TextBulk inspected(ctx);
      grn_inspect(ctx, vector1, *inspected);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the 1st argument must be number family vector: %.*s",
          tag,
          static_cast<int>(GRN_TEXT_LEN(*inspected)),
          GRN_TEXT_VALUE(*inspected));
      return false;
    }

    if (!grn_obj_is_number_family_vector(ctx, vector2)) {
      grn::TextBulk inspected(ctx);
      grn_inspect(ctx, vector2, *inspected);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the 2nd argument must be number family vector: %.*s",
          tag,
          static_cast<int>(GRN_TEXT_LEN(*inspected)),
          GRN_TEXT_VALUE(*inspected));
      return false;
    }

    if (vector1->header.domain != vector2->header.domain) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s both vectors must be the same type: %s: %s",
          tag,
          grn_type_id_to_string_builtin(ctx, vector1->header.domain),
          grn_type_id_to_string_builtin(ctx, vector2->header.domain));
      return false;
    }

    auto n_elements1 = grn_vector_size(ctx, vector1);
    auto n_elements2 = grn_vector_size(ctx, vector2);
    if (n_elements1 != n_elements2) {
      ERR(GRN_INVALID_ARGUMENT,
          "%s both vectors must be the same size: %d: %d",
          tag,
          n_elements1,
          n_elements2);
      return false;
    }

    return true;
  }

  template <typename ElementType>
  float
  compute_distance_cosine(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
  {
    auto l2_norm1 = grn::distance::compute_l2_norm<ElementType>(vector1);
    if (grn::numeric::is_zero(l2_norm1)) {
      return 0.0;
    }
    auto l2_norm2 = grn::distance::compute_l2_norm<ElementType>(vector2);
    if (grn::numeric::is_zero(l2_norm2)) {
      return 0.0;
    }
    auto inner_product =
      grn::distance::compute_inner_product<ElementType>(vector1, vector2);
    return 1 - (inner_product / (l2_norm1 * l2_norm2));
  }
} // namespace

extern "C" void
grn_distance_init_from_env(void)
{
  {
    char grn_distance_simd_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_DISTANCE_SIMD", grn_distance_simd_env, GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_distance_simd_env, "no") == 0) {
      grn::distance::use_simd = false;
    }
  }
}

extern "C" float
grn_distance_cosine(grn_ctx *ctx, grn_obj *vector1, grn_obj *vector2)
{
  const char *tag = "[distance][cosine]";
  float distance = 0.0;

  if (!validate_vectors(ctx, vector1, vector2, tag)) {
    return distance;
  }

  switch (vector1->header.domain) {
  case GRN_DB_FLOAT32:
    distance = compute_distance_cosine<float>(ctx, vector1, vector2);
    break;
  case GRN_DB_FLOAT:
    distance = compute_distance_cosine<double>(ctx, vector1, vector2);
    break;
  default:
    break;
  }

  return distance;
}
