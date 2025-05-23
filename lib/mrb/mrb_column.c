/*
  Copyright(C) 2013-2018  Brazil
  Copyright(C) 2020-2022  Sutou Kouhei <kou@clear-code.com>

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
#include "../grn_proc.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>

#include "mrb_bulk.h"
#include "mrb_column.h"
#include "mrb_converter.h"
#include "mrb_ctx.h"
#include "mrb_uvector.h"
#include "mrb_vector.h"

static mrb_value
mrb_grn_column_class_parse_flags(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  char *error_message_tag;
  char *flags_text;
  mrb_int flags_text_size;
  grn_column_flags flags;

  mrb_get_args(mrb, "zs", &error_message_tag, &flags_text, &flags_text_size);

  flags = grn_proc_column_parse_flags(ctx,
                                      error_message_tag,
                                      flags_text,
                                      flags_text + flags_text_size);
  return mrb_int_value(mrb, flags);
}

static mrb_value
mrb_grn_column_array_reference(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *column;
  mrb_int record_id;
  grn_obj column_value;
  mrb_value rb_column_value;
  mrb_value rb_inspected_unsupported_value = mrb_nil_value();

  column = DATA_PTR(self);
  mrb_get_args(mrb, "i", &record_id);

  GRN_VOID_INIT(&column_value);
  grn_obj_get_value(ctx, column, (grn_id)record_id, &column_value);
  if (grn_obj_is_bulk(ctx, &column_value)) {
    rb_column_value = grn_mrb_value_from_bulk(mrb, &column_value);
  } else if (grn_obj_is_vector(ctx, &column_value)) {
    rb_column_value = grn_mrb_value_from_vector(mrb, &column_value);
  } else if (grn_obj_is_uvector(ctx, &column_value)) {
    rb_column_value = grn_mrb_value_from_uvector(mrb, &column_value);
  } else {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, &column_value);
    rb_inspected_unsupported_value = grn_mrb_value_from_bulk(mrb, &inspected);
    GRN_OBJ_FIN(ctx, &inspected);
  }
  GRN_OBJ_FIN(ctx, &column_value);
  if (!mrb_nil_p(rb_inspected_unsupported_value)) {
    mrb_raisef(mrb, E_NOTIMP_ERROR,
               "unsupported object to convert to mrb_value: %S",
               rb_inspected_unsupported_value);
  }
  return rb_column_value;
}

static mrb_value
mrb_grn_column_is_scalar(mrb_state *mrb, mrb_value self)
{
  grn_obj *column;
  grn_obj_flags column_type;

  column = DATA_PTR(self);
  column_type = (column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK);

  return mrb_bool_value(column_type == GRN_OBJ_COLUMN_SCALAR);
}

static mrb_value
mrb_grn_column_is_vector(mrb_state *mrb, mrb_value self)
{
  grn_obj *column;
  grn_obj_flags column_type;

  column = DATA_PTR(self);
  column_type = (column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK);

  return mrb_bool_value(column_type == GRN_OBJ_COLUMN_VECTOR);
}

static mrb_value
mrb_grn_column_is_index(mrb_state *mrb, mrb_value self)
{
  grn_obj *column;
  grn_obj_flags column_type;

  column = DATA_PTR(self);
  column_type = (column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK);

  return mrb_bool_value(column_type == GRN_OBJ_COLUMN_INDEX);
}

static mrb_value
mrb_grn_column_is_locked(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  unsigned int is_locked;

  is_locked = grn_obj_is_locked(ctx, DATA_PTR(self));
  grn_mrb_ctx_check(mrb);

  return mrb_bool_value(is_locked != 0);
}

static mrb_value
mrb_grn_column_clear_lock(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;

  grn_obj_clear_lock(ctx, DATA_PTR(self));
  grn_mrb_ctx_check(mrb);
  return mrb_nil_value();
}

static mrb_value
mrb_grn_column_get_table(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *table;

  table = grn_column_table(ctx, DATA_PTR(self));
  if (!table) {
    return mrb_nil_value();
  }

  return grn_mrb_value_from_grn_obj(mrb, table);
}

static mrb_value
mrb_grn_column_truncate(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *column;

  column = DATA_PTR(self);
  grn_column_truncate(ctx, column);
  grn_mrb_ctx_check(mrb);
  return mrb_nil_value();
}

void
grn_mrb_column_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *object_class = data->object_class;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "Column", object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_class_method(mrb, klass, "parse_flags",
                          mrb_grn_column_class_parse_flags, MRB_ARGS_REQ(2));

  mrb_define_method(mrb, klass, "[]",
                    mrb_grn_column_array_reference, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, klass, "scalar?",
                    mrb_grn_column_is_scalar, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "vector?",
                    mrb_grn_column_is_vector, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "index?",
                    mrb_grn_column_is_index, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "locked?",
                    mrb_grn_column_is_locked, MRB_ARGS_NONE());
  mrb_define_method(mrb, klass, "clear_lock",
                    mrb_grn_column_clear_lock, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "table",
                    mrb_grn_column_get_table, MRB_ARGS_NONE());

  mrb_define_method(mrb, klass, "truncate",
                    mrb_grn_column_truncate, MRB_ARGS_NONE());
}
#endif
