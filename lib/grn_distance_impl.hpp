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

#include "grn_distance.hpp"

namespace grn {
  namespace distance {
    template <typename Arch,
              typename ElementType,
              typename BatchFunc,
              typename SequentialFunc>
    void
    each_batch(const ElementType *vector_raw,
               size_t n_elements,
               BatchFunc batch_func,
               SequentialFunc sequential_func)
    {
      using batch = xsimd::batch<ElementType, Arch>;
      auto address = reinterpret_cast<const uintptr_t>(vector_raw);
      size_t i = 0;
      auto unaligned_size = address % Arch::alignment();
      if (unaligned_size != 0) {
        auto adjust_size = Arch::alignment() - unaligned_size;
        if ((adjust_size % sizeof(ElementType)) != 0) {
          // Can't align.
          for (; i < n_elements; i += batch::size) {
            auto vector_batch = batch::load_unaligned(vector_raw + i);
            batch_func(vector_batch);
          }
          for (; i < n_elements; ++i) {
            sequential_func(vector_raw, i);
          }
          return;
        }
        // Adjust start alignment.
        for (; adjust_size > 0; ++i, adjust_size -= sizeof(ElementType)) {
          sequential_func(vector_raw, i);
        }
      }
      // Aligned batches.
      for (; i < n_elements; i += batch::size) {
        auto vector_batch = batch::load_aligned(vector_raw + i);
        batch_func(vector_batch);
      }
      // Rest.
      for (; i < n_elements; ++i) {
        sequential_func(vector_raw, i);
      }
    }

    template <typename Arch,
              typename ElementType,
              typename BatchFunc,
              typename SequentialFunc>
    void
    each_batch(const ElementType *vector_raw1,
               const ElementType *vector_raw2,
               size_t n_elements,
               BatchFunc batch_func,
               SequentialFunc sequential_func)
    {
      using batch = xsimd::batch<ElementType, Arch>;
      auto address1 = reinterpret_cast<const uintptr_t>(vector_raw1);
      auto address2 = reinterpret_cast<const uintptr_t>(vector_raw2);
      size_t i = 0;
      auto unaligned_size1 = address1 % Arch::alignment();
      auto unaligned_size2 = address2 % Arch::alignment();
      bool aligned = true;
      if (unaligned_size1 != unaligned_size2) {
        aligned = false;
      } else if (unaligned_size1 != 0) {
        auto adjust_size = Arch::alignment() - unaligned_size1;
        if ((adjust_size % sizeof(ElementType)) == 0) {
          // Adjust start alignment.
          for (; adjust_size > 0; ++i, adjust_size -= sizeof(ElementType)) {
            sequential_func(vector_raw1, vector_raw2, i);
          }
        } else {
          aligned = false;
        }
      }
      if (!aligned) {
        // Can't align.
        for (; i < n_elements; i += batch::size) {
          auto vector_batch1 = batch::load_unaligned(vector_raw1 + i);
          auto vector_batch2 = batch::load_unaligned(vector_raw2 + i);
          batch_func(vector_batch1, vector_batch2);
        }
        for (; i < n_elements; ++i) {
          sequential_func(vector_raw1, vector_raw2, i);
        }
        return;
      }
      // Aligned batches.
      for (; i < n_elements; i += batch::size) {
        auto vector_batch1 = batch::load_aligned(vector_raw1 + i);
        auto vector_batch2 = batch::load_aligned(vector_raw2 + i);
        batch_func(vector_batch1, vector_batch2);
      }
      // Rest.
      for (; i < n_elements; ++i) {
        sequential_func(vector_raw1, vector_raw2, i);
      }
    }

    template <typename Arch, typename ElementType>
    float
    l2_norm::operator()(Arch, const ElementType *vector_raw, size_t n_elements)
    {
      float square_sum = 0;
      using batch = xsimd::batch<ElementType, Arch>;
      each_batch<Arch, ElementType>(
        vector_raw,
        n_elements,
        [&square_sum](batch &vector_batch) {
          square_sum += xsimd::reduce_add(vector_batch * vector_batch);
        },
        [&square_sum](const ElementType *vector_raw, size_t i) {
          square_sum += vector_raw[i] * vector_raw[i];
        });
      return std::sqrt(square_sum);
    }

    template <typename Arch, typename ElementType>
    float
    difference_l1_norm::operator()(Arch,
                                   const ElementType *vector_raw1,
                                   const ElementType *vector_raw2,
                                   size_t n_elements)
    {
      using batch = xsimd::batch<ElementType, Arch>;
      float absolute_sum = 0;
      if constexpr (std::is_floating_point_v<ElementType>) {
        each_batch<Arch, ElementType>(
          vector_raw1,
          vector_raw2,
          n_elements,
          [&absolute_sum](batch &vector_batch1, batch &vector_batch2) {
            auto difference = vector_batch1 - vector_batch2;
            absolute_sum +=
              xsimd::reduce_add(xsimd::fabs(difference));
          },
          [&absolute_sum](const ElementType *vector_raw1,
                          const ElementType *vector_raw2,
                          size_t i) {
            auto difference = vector_raw1[i] - vector_raw2[i];
            absolute_sum += difference < 0 ? -difference : difference;
          });
      } else {
        each_batch<Arch, ElementType>(
          vector_raw1,
          vector_raw2,
          n_elements,
          [&absolute_sum](batch &vector_batch1, batch &vector_batch2) {
            auto difference = vector_batch1 - vector_batch2;
            absolute_sum +=
              xsimd::reduce_add(xsimd::abs(difference));
          },
          [&absolute_sum](const ElementType *vector_raw1,
                          const ElementType *vector_raw2,
                          size_t i) {
            auto difference = vector_raw1[i] - vector_raw2[i];
            absolute_sum += difference < 0 ? -difference : difference;
          });
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
      using batch = xsimd::batch<ElementType, Arch>;
      float square_sum = 0;
      each_batch<Arch, ElementType>(
        vector_raw1,
        vector_raw2,
        n_elements,
        [&square_sum](batch &vector_batch1, batch &vector_batch2) {
          auto difference = vector_batch1 - vector_batch2;
          square_sum += xsimd::reduce_add(difference * difference);
        },
        [&square_sum](const ElementType *vector_raw1,
                      const ElementType *vector_raw2,
                      size_t i) {
          auto difference = vector_raw1[i] - vector_raw2[i];
          square_sum += difference * difference;
        });
      return square_sum;
    }

    template <typename Arch, typename ElementType>
    float
    inner_product::operator()(Arch,
                              const ElementType *vector_raw1,
                              const ElementType *vector_raw2,
                              size_t n_elements)
    {
      using batch = xsimd::batch<ElementType, Arch>;
      float multiplication_sum = 0;
      each_batch<Arch, ElementType>(
        vector_raw1,
        vector_raw2,
        n_elements,
        [&multiplication_sum](batch &vector_batch1, batch &vector_batch2) {
          multiplication_sum +=
            xsimd::reduce_add(vector_batch1 * vector_batch2);
        },
        [&multiplication_sum](const ElementType *vector_raw1,
                              const ElementType *vector_raw2,
                              size_t i) {
          multiplication_sum += vector_raw1[i] * vector_raw2[i];
        });
      return multiplication_sum;
    }
  } // namespace distance
} // namespace grn
