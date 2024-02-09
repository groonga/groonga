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

#if defined(GRN_WITH_SIMSIMD) && defined(GRN_INSTANTIATION_SIMSIMD_ARCH)
namespace grn {
  namespace distance {
    namespace simsimd {
#  ifdef GRN_INSTANTIATION_EXTERN
#    define FUNC_EXPANDED(simsimd_backend,                                     \
                          c_type,                                              \
                          simsimd_type,                                        \
                          grn_metric,                                          \
                          simsimd_metric)                                      \
      float compute_distance_##grn_metric##_##simsimd_backend(                 \
        const c_type *vector_raw1,                                             \
        const c_type *vector_raw2,                                             \
        size_t n_elements);
#  else
#    define FUNC_EXPANDED(simsimd_backend,                                     \
                          c_type,                                              \
                          simsimd_type,                                        \
                          grn_metric,                                          \
                          simsimd_metric)                                      \
      float compute_distance_##grn_metric##_##simsimd_backend(                 \
        const c_type *vector_raw1,                                             \
        const c_type *vector_raw2,                                             \
        size_t n_elements)                                                     \
      {                                                                        \
        return simsimd_##simsimd_backend##_##simsimd_type##_##simsimd_metric(  \
          vector_raw1,                                                         \
          vector_raw2,                                                         \
          n_elements);                                                         \
      }
#  endif
#  define FUNC(simsimd_backend,                                                \
               c_type,                                                         \
               simsimd_type,                                                   \
               grn_metric,                                                     \
               simsimd_metric)                                                 \
    FUNC_EXPANDED(simsimd_backend,                                             \
                  c_type,                                                      \
                  simsimd_type,                                                \
                  grn_metric,                                                  \
                  simsimd_metric)

      FUNC(GRN_INSTANTIATION_SIMSIMD_ARCH, float, f32, cosine, cos)
      FUNC(GRN_INSTANTIATION_SIMSIMD_ARCH, double, f64, cosine, cos)

      FUNC(GRN_INSTANTIATION_SIMSIMD_ARCH, float, f32, inner_product, ip)
      FUNC(GRN_INSTANTIATION_SIMSIMD_ARCH, double, f64, inner_product, ip)

      FUNC(GRN_INSTANTIATION_SIMSIMD_ARCH, float, f32, l2_norm_squared, l2sq)
      FUNC(GRN_INSTANTIATION_SIMSIMD_ARCH, double, f64, l2_norm_squared, l2sq)

#  undef FUNC
#  undef FUNC_EXPANDED
    } // namespace simsimd
  }   // namespace distance
} // namespace grn
#endif

#ifdef GRN_WITH_XSIMD

#  ifndef GRN_INSTANTIATION_EXTERN
#    define GRN_INSTANTIATION_EXTERN
#  endif

GRN_INSTANTIATION_EXTERN template float
grn::distance::l2_norm::operator()<GRN_INSTANTIATION_XSIMD_ARCH, float>(
  GRN_INSTANTIATION_XSIMD_ARCH, const float *vector_raw, size_t n_elements);
GRN_INSTANTIATION_EXTERN template float
grn::distance::l2_norm::operator()<GRN_INSTANTIATION_XSIMD_ARCH, double>(
  GRN_INSTANTIATION_XSIMD_ARCH, const double *vector_raw, size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::inner_product::operator()<GRN_INSTANTIATION_XSIMD_ARCH, float>(
  GRN_INSTANTIATION_XSIMD_ARCH,
  const float *vector_raw1,
  const float *vector_raw2,
  size_t n_elements);
GRN_INSTANTIATION_EXTERN template float
grn::distance::inner_product::operator()<GRN_INSTANTIATION_XSIMD_ARCH, double>(
  GRN_INSTANTIATION_XSIMD_ARCH,
  const double *vector_raw1,
  const double *vector_raw2,
  size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l1_norm::operator()<GRN_INSTANTIATION_XSIMD_ARCH,
                                              float>(
  GRN_INSTANTIATION_XSIMD_ARCH,
  const float *vector_raw1,
  const float *vector_raw2,
  size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l1_norm::operator()<GRN_INSTANTIATION_XSIMD_ARCH,
                                              double>(
  GRN_INSTANTIATION_XSIMD_ARCH,
  const double *vector_raw1,
  const double *vector_raw2,
  size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l2_norm_squared::
operator()<GRN_INSTANTIATION_XSIMD_ARCH, float>(GRN_INSTANTIATION_XSIMD_ARCH,
                                                const float *vector_raw1,
                                                const float *vector_raw2,
                                                size_t n_elements);
GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l2_norm_squared::
operator()<GRN_INSTANTIATION_XSIMD_ARCH, double>(GRN_INSTANTIATION_XSIMD_ARCH,
                                                 const double *vector_raw1,
                                                 const double *vector_raw2,
                                                 size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::cosine::operator()<GRN_INSTANTIATION_XSIMD_ARCH, float>(
  GRN_INSTANTIATION_XSIMD_ARCH,
  const float *vector_raw1,
  const float *vector_raw2,
  size_t n_elements);
GRN_INSTANTIATION_EXTERN template float
grn::distance::cosine::operator()<GRN_INSTANTIATION_XSIMD_ARCH, double>(
  GRN_INSTANTIATION_XSIMD_ARCH,
  const double *vector_raw1,
  const double *vector_raw2,
  size_t n_elements);
#endif
