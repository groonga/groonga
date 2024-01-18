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

#include "grn.h"

#ifdef GRN_WITH_SIMD_AVX512
#  include "grn_distance_impl.hpp"

#  define GRN_INSTANTIATION_EXTERN
#  define GRN_INSTANTIATION_ARCH xsimd::avx512f
#  include "grn_distance_instantiation.hpp"
#  undef GRN_INSTANTIATION_ARCH
#  undef GRN_INSTANTIATION_EXTERN
#endif
