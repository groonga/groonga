/*
  Copyright(C) 2009-2017  Brazil
  Copyright(C) 2021  Sutou Kouhei <kou@clear-code.com>

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

#include "grn_io.h"
#include "grn_db.h"

#ifdef __cplusplus
extern "C" {
#endif

grn_io *grn_obj_get_io(grn_ctx *ctx, grn_obj *obj);

void
grn_obj_set_error(grn_ctx *ctx,
                  grn_obj *obj,
                  grn_rc rc,
                  grn_id id,
                  const char *tag,
                  const char *format,
                  ...);

void
grn_obj_log(grn_ctx *ctx,
            grn_obj *obj,
            grn_log_level level,
            grn_id id,
            const char *tag,
            const char *format,
            ...);


#ifdef __cplusplus
}
#endif
