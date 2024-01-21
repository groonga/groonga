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

GRN_INSTANTIATION_EXTERN template float
grn::distance::l2_norm::operator()<GRN_INSTANTIATION_ARCH, float>(
  GRN_INSTANTIATION_ARCH, const float *vector_raw, size_t n_elements);
GRN_INSTANTIATION_EXTERN template float
grn::distance::l2_norm::operator()<GRN_INSTANTIATION_ARCH, double>(
  GRN_INSTANTIATION_ARCH, const double *vector_raw, size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::inner_product::operator()<GRN_INSTANTIATION_ARCH, float>(
  GRN_INSTANTIATION_ARCH,
  const float *vector_raw1,
  const float *vector_raw2,
  size_t n_elements);
GRN_INSTANTIATION_EXTERN template float
grn::distance::inner_product::operator()<GRN_INSTANTIATION_ARCH, double>(
  GRN_INSTANTIATION_ARCH,
  const double *vector_raw1,
  const double *vector_raw2,
  size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l1_norm::operator()<GRN_INSTANTIATION_ARCH, float>(
  GRN_INSTANTIATION_ARCH,
  const float *vector_raw1,
  const float *vector_raw2,
  size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l1_norm::operator()<GRN_INSTANTIATION_ARCH, double>(
  GRN_INSTANTIATION_ARCH,
  const double *vector_raw1,
  const double *vector_raw2,
  size_t n_elements);

GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l2_norm_squared::operator()<GRN_INSTANTIATION_ARCH,
                                                      float>(
  GRN_INSTANTIATION_ARCH,
  const float *vector_raw1,
  const float *vector_raw2,
  size_t n_elements);
GRN_INSTANTIATION_EXTERN template float
grn::distance::difference_l2_norm_squared::operator()<GRN_INSTANTIATION_ARCH,
                                                      double>(
  GRN_INSTANTIATION_ARCH,
  const double *vector_raw1,
  const double *vector_raw2,
  size_t n_elements);
