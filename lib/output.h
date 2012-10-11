/* -*- c-basic-offset: 2 -*- */
/* Copyright(C) 2010-2012 Brazil

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

#ifndef GRN_CTX_IMPL_H
#include "ctx_impl.h"
#endif /* GRN_CTX_IMPL_H */

#ifdef __cplusplus
extern "C" {
#endif

GRN_API void grn_output_array_open(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                           const char *name, int nelements);
GRN_API void grn_output_array_close(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type);
GRN_API void grn_output_map_open(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                                 const char *name, int nelements);
GRN_API void grn_output_map_close(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type);
void grn_output_int32(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                      int32_t value);
GRN_API void grn_output_int64(grn_ctx *ctx, grn_obj *outbuf,
                              grn_content_type output_type,
                              int64_t value);
void grn_output_float(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                      double value);
GRN_API void grn_output_cstr(grn_ctx *ctx, grn_obj *outbuf, grn_content_type output_type,
                             const char *value);
GRN_API void grn_output_str(grn_ctx *ctx, grn_obj *outbuf,
                            grn_content_type output_type,
                            const char *value, size_t value_len);
GRN_API void grn_output_bool(grn_ctx *ctx, grn_obj *outbuf,
                             grn_content_type output_type,
                             grn_bool value);

#define GRN_OUTPUT_ARRAY_OPEN(name,nelements) \
  (grn_output_array_open(ctx, ctx->impl->outbuf, ctx->impl->output_type, name, nelements))
#define GRN_OUTPUT_ARRAY_CLOSE() \
  (grn_output_array_close(ctx, ctx->impl->outbuf, ctx->impl->output_type))
#define GRN_OUTPUT_MAP_OPEN(name,nelements) \
  (grn_output_map_open(ctx, ctx->impl->outbuf, ctx->impl->output_type, name, nelements))
#define GRN_OUTPUT_MAP_CLOSE() \
  (grn_output_map_close(ctx, ctx->impl->outbuf, ctx->impl->output_type))
#define GRN_OUTPUT_INT32(value) \
  (grn_output_int32(ctx, ctx->impl->outbuf, ctx->impl->output_type, value))
#define GRN_OUTPUT_INT64(value) \
  (grn_output_int64(ctx, ctx->impl->outbuf, ctx->impl->output_type, value))
#define GRN_OUTPUT_FLOAT(value) \
  (grn_output_float(ctx, ctx->impl->outbuf, ctx->impl->output_type, value))
#define GRN_OUTPUT_CSTR(value)\
  (grn_output_cstr(ctx, ctx->impl->outbuf, ctx->impl->output_type, value))
#define GRN_OUTPUT_STR(value,value_len)\
  (grn_output_str(ctx, ctx->impl->outbuf, ctx->impl->output_type, value, value_len))
#define GRN_OUTPUT_BOOL(value)\
  (grn_output_bool(ctx, ctx->impl->outbuf, ctx->impl->output_type, value))
#define GRN_OUTPUT_OBJ(obj,format)\
  (grn_output_obj(ctx, ctx->impl->outbuf, ctx->impl->output_type, obj, format))

#ifdef __cplusplus
}
#endif

#endif /* GRN_OUTPUT_H */
