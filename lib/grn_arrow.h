/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2019 Sutou Kouhei <kou@clear-code.com>

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
#include "grn_load.h"

#ifdef  __cplusplus
extern "C" {
#endif

typedef struct _grn_arrow_stream_loader grn_arrow_stream_loader;

grn_arrow_stream_loader *
grn_arrow_stream_loader_open(grn_ctx *ctx,
                             grn_loader *loader);
grn_rc
grn_arrow_stream_loader_close(grn_ctx *ctx,
                              grn_arrow_stream_loader *loader);
grn_rc
grn_arrow_stream_loader_feed(grn_ctx *ctx,
                             grn_arrow_stream_loader *loader,
                             const char *data,
                             size_t data_size);

#ifdef __cplusplus
}
#endif
