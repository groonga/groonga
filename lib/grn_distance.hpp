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

#include <groonga/bulk.hpp>

#include <cmath>

#ifdef GRN_WITH_SIMSIMD
#  include <simsimd/simsimd.h>
#endif

#ifdef GRN_WITH_XSIMD
#  include <xsimd/xsimd.hpp>
#endif

namespace grn {
  namespace distance {
    extern bool use_simsimd;
    extern bool use_xsimd;
    constexpr size_t use_simd_threshold = 256;

#ifdef GRN_WITH_SIMSIMD
    namespace simsimd {
      extern simsimd_capability_t capabilities;
    }
#endif

#ifdef GRN_WITH_XSIMD
    struct l2_norm {
      template <typename Arch, typename ElementType>
      float
      operator()(Arch, const ElementType *vector_raw, size_t n_elements);
    };

    struct difference_l1_norm {
      template <typename Arch, typename ElementType>
      float
      operator()(Arch,
                 const ElementType *vector_raw1,
                 const ElementType *vector_raw2,
                 size_t n_elements);
    };

    struct difference_l2_norm_squared {
      template <typename Arch, typename ElementType>
      float
      operator()(Arch,
                 const ElementType *vector_raw1,
                 const ElementType *vector_raw2,
                 size_t n_elements);
    };

    struct inner_product {
      template <typename Arch, typename ElementType>
      float
      operator()(Arch,
                 const ElementType *vector_raw1,
                 const ElementType *vector_raw2,
                 size_t n_elements);
    };

    struct cosine {
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
#  define GRN_INSTANTIATION_EXTERN       extern

#  define GRN_INSTANTIATION_SIMSIMD_ARCH avx512
#  define GRN_INSTANTIATION_XSIMD_ARCH   xsimd::avx512dq
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_SIMSIMD_ARCH
#  undef GRN_INSTANTIATION_XSIMD_ARCH

#  define GRN_INSTANTIATION_XSIMD_ARCH xsimd::avx2
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_XSIMD_ARCH

#  define GRN_INSTANTIATION_XSIMD_ARCH xsimd::avx
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_XSIMD_ARCH

#  define GRN_INSTANTIATION_SIMSIMD_ARCH neon
#  define GRN_INSTANTIATION_XSIMD_ARCH   xsimd::neon64
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_SIMSIMD_ARCH
#  undef GRN_INSTANTIATION_XSIMD_ARCH

#  define GRN_INSTANTIATION_SIMSIMD_ARCH serial
#  define GRN_INSTANTIATION_XSIMD_ARCH   xsimd::generic
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_SIMSIMD_ARCH
#  undef GRN_INSTANTIATION_XSIMD_ARCH

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
#  ifdef GRN_WITH_XSIMD
      if (use_xsimd &&
          (sizeof(ElementType) * n_elements) >= use_simd_threshold) {
        auto dispatched = xsimd::dispatch<xsimd::arch_list<
#    ifdef GRN_WITH_SIMD_AVX512
          xsimd::avx512dq,
#    endif
#    ifdef GRN_WITH_SIMD_AVX2
          xsimd::avx2,
#    endif
#    ifdef GRN_WITH_SIMD_AVX
          xsimd::avx,
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
          xsimd::neon64,
#    endif
          xsimd::generic>>(l2_norm{});
        return dispatched(vector_raw, n_elements);
      }
#  endif
#endif
      float square_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        square_sum += vector_raw[i] * vector_raw[i];
      }
      return std::sqrt(square_sum);
    }

    template <typename ElementType>
    float
    compute_difference_l1_norm(grn_obj *vector1, grn_obj *vector2)
    {
      auto vector_raw1 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector1));
      auto vector_raw2 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector2));
      auto n_elements = GRN_BULK_VSIZE(vector1) / sizeof(ElementType);
#ifdef GRN_WITH_SIMD
#  ifdef GRN_WITH_XSIMD
      if (use_xsimd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
        auto dispatched = xsimd::dispatch<xsimd::arch_list<
#    ifdef GRN_WITH_SIMD_AVX512
          xsimd::avx512dq,
#    endif
#    ifdef GRN_WITH_SIMD_AVX2
          xsimd::avx2,
#    endif
#    ifdef GRN_WITH_SIMD_AVX
          xsimd::avx,
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
          xsimd::neon64,
#    endif
          xsimd::generic>>(difference_l1_norm{});
        return dispatched(vector_raw1, vector_raw2, n_elements);
      }
#  endif
#endif
      float absolute_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        auto difference = vector_raw1[i] - vector_raw2[i];
        absolute_sum += difference * ((difference > 0) - (difference < 0));
      }
      return absolute_sum;
    }

    template <typename ElementType>
    float
    compute_difference_l2_norm_squared(grn_obj *vector1, grn_obj *vector2)
    {
      auto vector_raw1 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector1));
      auto vector_raw2 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector2));
      auto n_elements = GRN_BULK_VSIZE(vector1) / sizeof(ElementType);
#ifdef GRN_WITH_SIMD
#  ifdef GRN_WITH_SIMSIMD
      if (use_simsimd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
#    ifdef GRN_WITH_SIMD_AVX512
        if (simsimd::capabilities & simsimd_cap_x86_avx512_k) {
          return simsimd::compute_distance_l2_norm_squared_avx512(vector_raw1,
                                                                  vector_raw2,
                                                                  n_elements);
        }
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
        if (simsimd::capabilities & simsimd_cap_arm_neon_k) {
          return simsimd::compute_distance_l2_norm_squared_neon(vector_raw1,
                                                                vector_raw2,
                                                                n_elements);
        }
#    endif
        return simsimd::compute_distance_l2_norm_squared_serial(vector_raw1,
                                                                vector_raw2,
                                                                n_elements);
      }
#  endif
#  ifdef GRN_WITH_XSIMD
      if (use_xsimd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
        auto dispatched = xsimd::dispatch<xsimd::arch_list<
#    ifdef GRN_WITH_SIMD_AVX512
          xsimd::avx512dq,
#    endif
#    ifdef GRN_WITH_SIMD_AVX2
          xsimd::avx2,
#    endif
#    ifdef GRN_WITH_SIMD_AVX
          xsimd::avx,
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
          xsimd::neon64,
#    endif
          xsimd::generic>>(difference_l2_norm_squared{});
        return dispatched(vector_raw1, vector_raw2, n_elements);
      }
#  endif
#endif
      float square_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        auto difference = vector_raw1[i] - vector_raw2[i];
        square_sum += difference * difference;
      }
      return square_sum;
    }

    template <typename ElementType>
    float
    compute_distance_inner_product(grn_obj *vector1, grn_obj *vector2)
    {
      auto vector_raw1 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector1));
      auto vector_raw2 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector2));
      auto n_elements = GRN_BULK_VSIZE(vector1) / sizeof(ElementType);
#ifdef GRN_WITH_SIMD
#  ifdef GRN_WITH_SIMSIMD
      if (use_simsimd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
#    ifdef GRN_WITH_SIMD_AVX512
        if (simsimd::capabilities & simsimd_cap_x86_avx512_k) {
          return simsimd::compute_distance_inner_product_avx512(vector_raw1,
                                                                vector_raw2,
                                                                n_elements);
        }
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
        if (simsimd::capabilities & simsimd_cap_arm_neon_k) {
          return simsimd::compute_distance_inner_product_neon(vector_raw1,
                                                              vector_raw2,
                                                              n_elements);
        }
#    endif
        return simsimd::compute_distance_inner_product_serial(vector_raw1,
                                                              vector_raw2,
                                                              n_elements);
      }
#  endif
#  ifdef GRN_WITH_XSIMD
      if (use_xsimd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
        auto dispatched = xsimd::dispatch<xsimd::arch_list<
#    ifdef GRN_WITH_SIMD_AVX512
          xsimd::avx512dq,
#    endif
#    ifdef GRN_WITH_SIMD_AVX2
          xsimd::avx2,
#    endif
#    ifdef GRN_WITH_SIMD_AVX
          xsimd::avx,
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
          xsimd::neon64,
#    endif
          xsimd::generic>>(inner_product{});
        return dispatched(vector_raw1, vector_raw2, n_elements);
      }
#  endif
#endif
      float multiplication_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        multiplication_sum += vector_raw1[i] * vector_raw2[i];
      }
      return 1 - multiplication_sum;
    }

    template <typename ElementType>
    float
    compute_distance_cosine(grn_obj *vector1, grn_obj *vector2)
    {
      auto vector_raw1 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector1));
      auto vector_raw2 =
        reinterpret_cast<const ElementType *>(GRN_BULK_HEAD(vector2));
      auto n_elements = GRN_BULK_VSIZE(vector1) / sizeof(ElementType);
#ifdef GRN_WITH_SIMD
#  ifdef GRN_WITH_SIMSIMD
      if (use_simsimd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
#    ifdef GRN_WITH_SIMD_AVX512
        if (simsimd::capabilities & simsimd_cap_x86_avx512_k) {
          return simsimd::compute_distance_cosine_avx512(vector_raw1,
                                                         vector_raw2,
                                                         n_elements);
        }
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
        if (simsimd::capabilities & simsimd_cap_arm_neon_k) {
          return simsimd::compute_distance_cosine_neon(vector_raw1,
                                                       vector_raw2,
                                                       n_elements);
        }
#    endif
        return simsimd::compute_distance_cosine_serial(vector_raw1,
                                                       vector_raw2,
                                                       n_elements);
      }
#  endif
#  ifdef GRN_WITH_XSIMD
      if (use_xsimd &&
          (sizeof(ElementType) * n_elements * 2) >= use_simd_threshold) {
        auto dispatched = xsimd::dispatch<xsimd::arch_list<
#    ifdef GRN_WITH_SIMD_AVX512
          xsimd::avx512dq,
#    endif
#    ifdef GRN_WITH_SIMD_AVX2
          xsimd::avx2,
#    endif
#    ifdef GRN_WITH_SIMD_AVX
          xsimd::avx,
#    endif
#    ifdef GRN_WITH_SIMD_NEON64
          xsimd::neon64,
#    endif
          xsimd::generic>>(cosine{});
        return dispatched(vector_raw1, vector_raw2, n_elements);
      }
#  endif
#endif
      ElementType inner_product = 0;
      ElementType square_sum1 = 0;
      ElementType square_sum2 = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        ElementType value1 = vector_raw1[i];
        ElementType value2 = vector_raw2[i];
        inner_product += value1 * value2;
        square_sum1 += value1 * value1;
        square_sum2 += value2 * value2;
      }
      if (numeric::is_zero(inner_product)) {
        return 1;
      } else {
        return 1 - (inner_product /
                    (std::sqrt(square_sum1) * std::sqrt(square_sum2)));
      }
    }
  } // namespace distance
} // namespace grn
