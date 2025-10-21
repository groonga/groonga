/*
  Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>

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

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct grn_memory_map grn_memory_map;
typedef uint32_t grn_memory_map_flags;

#define GRN_MEMORY_MAP_READ    (1 << 0)
#define GRN_MEMORY_MAP_WRITE   (1 << 1)
#define GRN_MEMORY_MAP_EXCLUDE (1 << 2)

GRN_API grn_memory_map *
grn_memory_map_open(grn_ctx *ctx,
                    const char *path,
                    grn_memory_map_flags flags,
                    uint64_t offset,
                    size_t length);
GRN_API void *
grn_memory_map_get_address(grn_ctx *ctx, grn_memory_map *map);
GRN_API void
grn_memory_map_close(grn_ctx *ctx, grn_memory_map *map);

#ifdef __cplusplus
}
#endif
