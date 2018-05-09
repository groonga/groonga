/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2009-2018 Brazil

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

#include "grn_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _grn_token {
  grn_obj data;
  grn_token_status status;
  uint64_t source_offset;
  uint32_t source_length;
};

grn_rc grn_token_init(grn_ctx *ctx, grn_token *token);
grn_rc grn_token_fin(grn_ctx *ctx, grn_token *token);
grn_rc grn_token_copy(grn_ctx *ctx, grn_token *token, grn_token *source);

#ifdef __cplusplus
}
#endif
