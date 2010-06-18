/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef GRN_OUTPUT_H
#define GRN_OUTPUT_H

#ifndef GROONGA_IN_H
#include "groonga_in.h"
#endif /* GROONGA_IN_H */

#ifndef GRN_CTX_H
#include "ctx.h"
#endif /* GRN_CTX_H */

#ifndef GRN_STORE_H
#include "store.h"
#endif /* GRN_STORE_H */

#ifdef __cplusplus
extern "C" {
#endif

void grn_output_array_open(grn_ctx *ctx, const char *name, int nelements);
void grn_output_array_close(grn_ctx *ctx);
void grn_output_map_open(grn_ctx *ctx, const char *name, int nelements);
void grn_output_map_close(grn_ctx *ctx);
void grn_output_int32(grn_ctx *ctx, int value);
void grn_output_int64(grn_ctx *ctx, long long value);
void grn_output_str(grn_ctx *ctx, const char *value);
void grn_output_obj(grn_ctx *ctx, grn_obj *obj, grn_obj_format *format);

#ifdef __cplusplus
}
#endif

#endif /* GRN_OUTPUT_H */
