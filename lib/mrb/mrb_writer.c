/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2015 Brazil

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

#include "../grn_ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/string.h>

#include "../grn_mrb.h"
#include "../grn_output.h"
#include "mrb_writer.h"

static mrb_value
writer_write(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value target;

  mrb_get_args(mrb, "o", &target);

  switch (mrb_type(target)) {
  case MRB_TT_FALSE :
    GRN_OUTPUT_BOOL(GRN_FALSE);
    break;
  case MRB_TT_TRUE :
    GRN_OUTPUT_BOOL(GRN_TRUE);
    break;
  case MRB_TT_FIXNUM :
    GRN_OUTPUT_INT32(mrb_fixnum(target));
    break;
  case MRB_TT_FLOAT :
    GRN_OUTPUT_FLOAT(mrb_float(target));
    break;
  case MRB_TT_STRING :
    GRN_OUTPUT_STR(RSTRING_PTR(target), RSTRING_LEN(target));
    break;
  default :
    mrb_raisef(mrb, E_ARGUMENT_ERROR,
               "must be true, false, number, float or string: %S", target);
    break;
  }

  return mrb_nil_value();
}

void
grn_mrb_writer_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Writer", mrb->object_class);

  mrb_define_method(mrb, klass, "write", writer_write, MRB_ARGS_REQ(1));
}
#endif
