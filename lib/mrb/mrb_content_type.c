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

#include "mrb_ctx.h"
#include "mrb_content_type.h"

void
grn_mrb_content_type_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module;

  module = mrb_define_module_under(mrb, data->module, "ContentType");

  mrb_define_const(mrb, module, "NONE",
                   mrb_int_value(mrb, GRN_CONTENT_NONE));
  mrb_define_const(mrb, module, "TSV",
                   mrb_int_value(mrb, GRN_CONTENT_TSV));
  mrb_define_const(mrb, module, "JSON",
                   mrb_int_value(mrb, GRN_CONTENT_JSON));
  mrb_define_const(mrb, module, "XML",
                   mrb_int_value(mrb, GRN_CONTENT_XML));
  mrb_define_const(mrb, module, "MSGPACK",
                   mrb_int_value(mrb, GRN_CONTENT_MSGPACK));
  mrb_define_const(mrb, module, "GROONGA_COMMAND_LIST",
                   mrb_int_value(mrb, GRN_CONTENT_GROONGA_COMMAND_LIST));
  mrb_define_const(mrb, module, "APACHE_ARROW",
                   mrb_int_value(mrb, GRN_CONTENT_APACHE_ARROW));
}
#endif
