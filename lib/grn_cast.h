/*
  Copyright(C) 2019-2021  Sutou Kouhei <kou@clear-code.com>

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

grn_rc
grn_obj_cast_text_to_uvector(grn_ctx *ctx,
                             grn_obj *src,
                             grn_obj *dest,
                             bool add_record_if_not_exist);
grn_rc
grn_obj_cast_text_to_text_vector(grn_ctx *ctx,
                                 grn_obj *src,
                                 grn_obj *dest);

#ifdef __cplusplus
}
#endif
