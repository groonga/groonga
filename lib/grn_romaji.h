/*
  Copyright(C) 2018 Kouhei Sutou <kou@clear-code.com>

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

#include "grn.h"

#ifdef __cplusplus
extern "C" {
#endif

const unsigned char *
grn_romaji_hepburn_convert(grn_ctx *ctx,
                           const unsigned char *current,
                           const unsigned char *end,
                           size_t *n_used_bytes,
                           size_t *n_used_characters,
                           unsigned char *unified_buffer,
                           size_t *n_unified_bytes,
                           size_t *n_unified_characters);

#ifdef __cplusplus
}
#endif
