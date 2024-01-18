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
  } // namespace distance
} // namespace grn

#define GRN_INSTANTIATION_EXTERN
#define GRN_INSTANTIATION_ARCH xsimd::generic
#include "grn_distance_instantiation.hpp"
#undef GRN_INSTANTIATION_ARCH
#undef GRN_INSTANTIATION_EXTERN
