/*
  Copyright(C) 2013-2016 Brazil
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

#include "../grn_ctx.h"

#ifdef __cplusplus
extern "C" {
#endif

void grn_mrb_ctx_init(grn_ctx *ctx);
mrb_value grn_mrb_ctx_to_exception(mrb_state *mrb);
void grn_mrb_ctx_check(mrb_state *mrb);

#ifdef __cplusplus
}
#endif

