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

#pragma once

#include "grn_ctx.h"

#include <cmath>

#ifdef GRN_WITH_XSIMD
#  include <xsimd/xsimd.hpp>
#endif

namespace grn {
  namespace distance {
    extern bool use_simd;
    constexpr size_t use_simd_threshold = 256;

#ifdef GRN_WITH_XSIMD
    struct l2_norm {
      template <typename Arch, typename ElementType>
      float
      operator()(Arch, const ElementType *vector_raw, size_t n_elements);
    };

    struct inner_product {
      template <typename Arch, typename ElementType>
      float
      operator()(Arch,
                 const ElementType *vector_raw1,
                 const ElementType *vector_raw2,
                 size_t n_elements);
    };
#endif
  } // namespace distance
} // namespace grn

#ifdef GRN_WITH_XSIMD
#  define GRN_INSTANTIATION_EXTERN extern

#  define GRN_INSTANTIATION_ARCH   xsimd::avx512dq
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_ARCH

#  define GRN_INSTANTIATION_ARCH xsimd::avx2
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_ARCH

#  define GRN_INSTANTIATION_ARCH xsimd::avx
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_ARCH

#  define GRN_INSTANTIATION_ARCH xsimd::neon64
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_ARCH

#  define GRN_INSTANTIATION_ARCH xsimd::generic
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_ARCH

#  undef GRN_INSTANTIATION_EXTERN
#endif

namespace grn {
  namespace distance {
    template <typename ElementType>
    float
    compute_l2_norm(grn_obj *vector)
    {
      auto vector_raw =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector));
      auto n_elements = GRN_BULK_VSIZE(vector) / sizeof(ElementType);
#ifdef GRN_WITH_SIMD
      if (use_simd &&
          (sizeof(ElementType) * n_elements) >= use_simd_threshold) {
        auto dispatched = xsimd::dispatch<xsimd::arch_list<
#  ifdef GRN_WITH_SIMD_AVX512
          xsimd::avx512dq,
#  endif
#  ifdef GRN_WITH_SIMD_AVX2
          xsimd::avx2,
#  endif
#  ifdef GRN_WITH_SIMD_AVX
          xsimd::avx,
#  endif
#  ifdef GRN_WITH_SIMD_NEON64
          xsimd::neon64,
#  endif
          xsimd::generic>>(l2_norm{});
        return dispatched(vector_raw, n_elements);
      }
#endif
      float square_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        square_sum += vector_raw[i] * vector_raw[i];
      }
      return std::sqrt(square_sum);
    }

    template <typename ElementType>
    float
    compute_inner_product(grn_obj *vector1, grn_obj *vector2)
    {
      auto vector_raw1 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector1));
      auto vector_raw2 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector2));
      auto n_elements = GRN_BULK_VSIZE(vector1) / sizeof(ElementType);
#ifdef GRN_WITH_SIMD
      if (use_simd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
        auto dispatched = xsimd::dispatch<xsimd::arch_list<
#  ifdef GRN_WITH_SIMD_AVX512
          xsimd::avx512dq,
#  endif
#  ifdef GRN_WITH_SIMD_AVX2
          xsimd::avx2,
#  endif
#  ifdef GRN_WITH_SIMD_AVX
          xsimd::avx,
#  endif
#  ifdef GRN_WITH_SIMD_NEON64
          xsimd::neon64,
#  endif
          xsimd::generic>>(inner_product{});
        return dispatched(vector_raw1, vector_raw2, n_elements);
      }
#endif
      float multiplication_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        multiplication_sum += vector_raw1[i] * vector_raw2[i];
      }
      return multiplication_sum;
    }
  } // namespace distance
} // namespace grn
