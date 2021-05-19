/*
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

#include "../grn_ctx_impl.h"
#include <string.h>

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/hash.h>
#include <mruby/array.h>
#include <mruby/string.h>

#include "mrb_converter.h"
#include "mrb_ctx.h"
#include "mrb_output_columns.h"

static struct mrb_data_type mrb_grn_output_columns_type = {
  "Groonga::OutputColumns",
  NULL
};

static mrb_value
mrb_grn_output_columns_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_output_columns_ptr;

  mrb_get_args(mrb, "o", &mrb_output_columns_ptr);
  DATA_TYPE(self) = &mrb_grn_output_columns_type;
  DATA_PTR(self) = mrb_cptr(mrb_output_columns_ptr);
  return self;
}

static mrb_value
mrb_grn_output_columns_apply(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value *mrb_columns;
  mrb_int n_mrb_columns;
  grn_obj *output_columns;
  grn_obj columns;
  mrb_value mrb_exception;

  mrb_get_args(mrb, "a", &mrb_columns, &n_mrb_columns);

  output_columns = DATA_PTR(self);
  GRN_PTR_INIT(&columns, GRN_OBJ_VECTOR, GRN_ID_NIL);
  {
    mrb_int i;
    for (i = 0; i < n_mrb_columns; i++) {
      grn_obj *column = GRN_MRB_DATA_PTR(mrb_columns[i]);
      GRN_PTR_PUT(ctx, &columns, column);
    }
  }
  grn_output_columns_apply(ctx, output_columns, &columns);
  mrb_exception = grn_mrb_ctx_to_exception(mrb);
  GRN_OBJ_FIN(ctx, &columns);
  if (!mrb_nil_p(mrb_exception)) {
    mrb_exc_raise(mrb, mrb_exception);
  }

  return mrb_nil_value();
}

void
grn_mrb_output_columns_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *object_class = data->object_class;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "OutputColumns", object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_output_columns_initialize,
                    MRB_ARGS_REQ(1));

  mrb_define_method(mrb, klass, "apply",
                    mrb_grn_output_columns_apply,
                    MRB_ARGS_REQ(1));
}
#endif
