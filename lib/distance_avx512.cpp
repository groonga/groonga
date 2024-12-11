/*
  Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn_distance_impl.hpp"

#define GRN_INSTANTIATION_SIMSIMD_ARCH avx512
#define GRN_INSTANTIATION_XSIMD_ARCH   xsimd::avx512dq
#include "grn_distance_instantiation.hpp"
#undef GRN_INSTANTIATION_SIMSIMD_ARCH
#undef GRN_INSTANTIATION_XSIMD_ARCH
