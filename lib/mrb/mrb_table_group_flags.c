/*
  Copyright(C) 2015 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>

#include "mrb_table_group_flags.h"

void
grn_mrb_table_group_flags_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *flags_module;

  flags_module = mrb_define_module_under(mrb, module, "TableGroupFlags");

  mrb_define_const(mrb, flags_module, "CALC_COUNT",
                   mrb_int_value(mrb, GRN_TABLE_GROUP_CALC_COUNT));
  mrb_define_const(mrb, flags_module, "CALC_MAX",
                   mrb_int_value(mrb, GRN_TABLE_GROUP_CALC_MAX));
  mrb_define_const(mrb, flags_module, "CALC_MIN",
                   mrb_int_value(mrb, GRN_TABLE_GROUP_CALC_MIN));
  mrb_define_const(mrb, flags_module, "CALC_SUM",
                   mrb_int_value(mrb, GRN_TABLE_GROUP_CALC_SUM));
  /* Deprecated since 10.0.4. Use CALC_MEAN instead. */
  mrb_define_const(mrb, flags_module, "CALC_AVG",
                   mrb_int_value(mrb, GRN_TABLE_GROUP_CALC_AVG));
  mrb_define_const(mrb, flags_module, "CALC_MEAN",
                   mrb_int_value(mrb, GRN_TABLE_GROUP_CALC_MEAN));
}
#endif
