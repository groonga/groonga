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
#ifndef GRN_UTIL_H
#define GRN_UTIL_H

#ifndef GROONGA_IN_H
#include "groonga_in.h"
#endif /* GROONGA_IN_H */

#ifndef GRN_CTX_H
#include "ctx.h"
#endif /* GRN_CTX_H */

#ifdef __cplusplus
extern "C" {
#endif

grn_rc grn_normalize_offset_and_limit(grn_ctx *ctx, int size, int *offset, int *limit);

grn_obj *grn_inspect(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj);
grn_obj *grn_inspect_name(grn_ctx *ctx, grn_obj *buffer, grn_obj *obj);
void grn_p(grn_ctx *ctx, grn_obj *obj);

#ifdef __cplusplus
}
#endif

#endif /* GRN_UTIL_H */
