/*
  Copyright(C) 2013-2018 Brazil
  Copyright(C) 2019 Kouhei Sutou <kou@clear-code.com>

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

void grn_mrb_object_init(grn_ctx *ctx);

mrb_value grn_mrb_object_get_domain_id(mrb_state *mrb, mrb_value self);
mrb_value grn_mrb_object_inspect(mrb_state *mrb, mrb_value self);
mrb_value grn_mrb_object_close(mrb_state *mrb, mrb_value self);
mrb_value grn_mrb_object_is_closed(mrb_state *mrb, mrb_value self);
mrb_value grn_mrb_object_is_true(mrb_state *mrb, mrb_value self);

#ifdef __cplusplus
}
#endif

