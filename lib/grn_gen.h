/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2016 Brazil

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

#include "grn_db.h"
#include "grn_io.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GRN_GEN_SIZE  (256*8)

struct _grn_gen {
  int id;
  uint8_t table[GRN_GEN_SIZE];
#ifdef _WIN32
  HFILE hFile;
#else
  int fd;
#endif
};
typedef struct _grn_gen grn_gen;

grn_rc grn_gen_init(grn_ctx *ctx, grn_db *db);
grn_rc grn_gen_fin(grn_ctx *ctx, grn_db *db);
int grn_gen_lock(grn_ctx *ctx, grn_io *io);
int grn_gen_unlock(grn_io *io);

#ifdef __cplusplus
}
#endif
