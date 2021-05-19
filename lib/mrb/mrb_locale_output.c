/*
  Copyright(C) 2018 Brazil

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
#include <mruby/variable.h>

#include "../grn_mrb.h"
#include "../grn_encoding.h"

static mrb_value
mrb_grn_locale_output_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_output;

  mrb_get_args(mrb, "o", &mrb_output);
  mrb_iv_set(mrb, self, mrb_intern_lit(mrb, "@output"), mrb_output);

  return self;
}

static mrb_value
mrb_grn_locale_output_write(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value mrb_output;
  char *utf8_message;
  mrb_int utf8_message_size;
  const char *locale_message;
  size_t locale_message_size;
  mrb_value mrb_locale_message;
  mrb_value mrb_written;

  mrb_get_args(mrb, "s", &utf8_message, &utf8_message_size);
  locale_message =
    grn_encoding_convert_to_locale_from_utf8(ctx,
                                             utf8_message,
                                             utf8_message_size,
                                             &locale_message_size);
  mrb_locale_message = mrb_str_new(mrb, locale_message, locale_message_size);
  grn_encoding_converted_free(ctx, locale_message);

  mrb_output = mrb_iv_get(mrb, self, mrb_intern_lit(mrb, "@output"));
  mrb_written = mrb_funcall(mrb, mrb_output, "write", 1, mrb_locale_message);

  return mrb_written;
}

void
grn_mrb_locale_output_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "LocaleOutput", mrb->object_class);

  mrb_define_method(mrb, klass, "initialize",
                    mrb_grn_locale_output_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "write",
                    mrb_grn_locale_output_write, MRB_ARGS_REQ(1));
}
#endif
