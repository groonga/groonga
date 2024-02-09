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

namespace grn {
  namespace distance {
    template <typename Arch, typename ElementType>
    float
    l2_norm::operator()(Arch, const ElementType *vector_raw, size_t n_elements)
    {
      float square_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        square_sum += vector_raw[i] * vector_raw[i];
      }
      return std::sqrt(square_sum);
    }

    template <typename Arch, typename ElementType>
    float
    difference_l1_norm::operator()(Arch,
                                   const ElementType *vector_raw1,
                                   const ElementType *vector_raw2,
                                   size_t n_elements)
    {
      float absolute_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        auto difference = vector_raw1[i] - vector_raw2[i];
        absolute_sum += difference < 0 ? -difference : difference;
      }
      return absolute_sum;
    }

    template <typename Arch, typename ElementType>
    float
    difference_l2_norm_squared::operator()(Arch,
                                           const ElementType *vector_raw1,
                                           const ElementType *vector_raw2,
                                           size_t n_elements)
    {
      float square_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        auto difference = vector_raw1[i] - vector_raw2[i];
        square_sum += difference * difference;
      }
      return square_sum;
    }

    template <typename Arch, typename ElementType>
    float
    inner_product::operator()(Arch,
                              const ElementType *vector_raw1,
                              const ElementType *vector_raw2,
                              size_t n_elements)
    {
      float multiplication_sum = 0;
      for (size_t i = 0; i < n_elements; ++i) {
        multiplication_sum += vector_raw1[i] * vector_raw2[i];
      }
      return multiplication_sum;
    }

    template <typename Arch, typename ElementType>
    float
    cosine::operator()(Arch,
                       const ElementType *vector_raw1,
                       const ElementType *vector_raw2,
                       size_t n_elements)
    {
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

#define GRN_INSTANTIATION_SIMSIMD_ARCH serial
#define GRN_INSTANTIATION_XSIMD_ARCH   xsimd::generic
#include "grn_distance_instantiation.hpp"
#undef GRN_INSTANTIATION_SIMSIMD_ARCH
#undef GRN_INSTANTIATION_XSIMD_ARCH
