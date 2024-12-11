/*
  Copyright(C) 2015  Brazil
  Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>

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
#include <groonga/command.h>

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/value.h>
#include <mruby/string.h>

#include "mrb_ctx.h"
#include "mrb_converter.h"
#include "mrb_command_input.h"

static struct mrb_data_type mrb_grn_command_input_type = {
  "Groonga::CommandInput",
  NULL
};

static mrb_value
mrb_grn_command_input_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_command_input_ptr;

  mrb_get_args(mrb, "o", &mrb_command_input_ptr);
  DATA_TYPE(self) = &mrb_grn_command_input_type;
  DATA_PTR(self) = mrb_cptr(mrb_command_input_ptr);
  return self;
}

static mrb_value
mrb_grn_command_input_array_reference(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_command_input *input;
  mrb_value mrb_key_or_offset;
  grn_obj *argument;

  input = DATA_PTR(self);
  mrb_get_args(mrb, "o", &mrb_key_or_offset);

  switch (mrb_type(mrb_key_or_offset)) {
  case MRB_TT_INTEGER :
    {
      mrb_int offset = mrb_integer(mrb_key_or_offset);
      argument = grn_command_input_at(ctx, input, (unsigned int)offset);
    }
    break;
  case MRB_TT_SYMBOL :
    {
      mrb_sym mrb_key_symbol;
      const char *key;
      mrb_int key_length;

      mrb_key_symbol = mrb_symbol(mrb_key_or_offset);
      key = mrb_sym2name_len(mrb, mrb_key_symbol, &key_length);
      argument = grn_command_input_get(ctx, input, key, (int)key_length);
    }
    break;
  case MRB_TT_STRING :
    {
      mrb_value mrb_key = mrb_key_or_offset;
      argument = grn_command_input_get(ctx, input,
                                       RSTRING_PTR(mrb_key),
                                       RSTRING_LEN(mrb_key));
    }
    break;
  default :
    mrb_raisef(mrb, E_ARGUMENT_ERROR,
               "must be offset (as integer) or key (as symbol or string): %S",
               mrb_key_or_offset);
    break;
  }

  if (!argument) {
    return mrb_nil_value();
  }

  if (GRN_TEXT_LEN(argument) == 0) {
    return mrb_nil_value();
  }

  return mrb_str_new_static(mrb,
                            GRN_TEXT_VALUE(argument),
                            GRN_TEXT_LEN(argument));
}

static mrb_value
mrb_grn_command_input_get_arguments(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_command_input *input;
  grn_obj *arguments;

  input = DATA_PTR(self);
  arguments = grn_command_input_get_arguments(ctx, input);

  return grn_mrb_value_from_grn_obj(mrb, arguments);
}

void
grn_mrb_command_input_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "CommandInput", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_command_input_initialize, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, klass, "[]",
                    mrb_grn_command_input_array_reference, MRB_ARGS_REQ(1));

  mrb_define_method(mrb, klass, "arguments",
                    mrb_grn_command_input_get_arguments, MRB_ARGS_NONE());
}
#endif
