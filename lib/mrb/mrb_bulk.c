/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2014 Brazil

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

#include "../ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/data.h>
#include <mruby/numeric.h>

#include "../db.h"
#include "mrb_bulk.h"

static struct mrb_data_type mrb_grn_bulk_type = {
  "Groonga::Bulk",
  NULL
};

static mrb_value
mrb_grn_bulk_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_bulk_ptr;

  mrb_get_args(mrb, "o", &mrb_bulk_ptr);
  DATA_TYPE(self) = &mrb_grn_bulk_type;
  DATA_PTR(self) = mrb_cptr(mrb_bulk_ptr);
  return self;
}

static mrb_value
mrb_grn_bulk_get_domain(mrb_state *mrb, mrb_value self)
{
  grn_obj *bulk;

  bulk = DATA_PTR(self);
  return mrb_fixnum_value(bulk->header.domain);
}

static mrb_value
mrb_grn_bulk_get_value(mrb_state *mrb, mrb_value self)
{
  grn_obj *bulk;
  mrb_value mrb_value_;

  bulk = DATA_PTR(self);
  switch (bulk->header.domain) {
  case GRN_DB_UINT32 :
    {
      uint32_t value;
      value = GRN_UINT32_VALUE(bulk);
      if (!FIXABLE(value)) {
        mrb_raisef(mrb, E_ARGUMENT_ERROR,
                   "can't handle large number: <%u>: max: <%" PRIiMRB_INT ">",
                   value, PRIiMRB_INT);
      }
      mrb_value_ = mrb_fixnum_value(value);
    }
    break;
  }

  return mrb_value_;
}

void
grn_mrb_bulk_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Bulk", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_bulk_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "domain",
                    mrb_grn_bulk_get_domain, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "value",
                    mrb_grn_bulk_get_value, MRB_ARGS_NONE());
}
#endif
