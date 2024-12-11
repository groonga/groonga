/*
  Copyright(C) 2015-2018  Brazil
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

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/data.h>
#include <mruby/variable.h>
#include <mruby/string.h>

#include "../grn_mrb.h"
#include "mrb_query_logger.h"

#include "../grn_encoding.h"

static mrb_value
query_logger_need_log_p(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_int flag;

  mrb_get_args(mrb, "i", &flag);

  return mrb_bool_value(grn_query_logger_pass(ctx, (unsigned int)flag));
}

static mrb_value
query_logger_log_raw(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_int flag;
  char *mark;
  char *utf8_message;
  mrb_int utf8_message_size;
  const char *message;
  size_t message_size;

  mrb_get_args(mrb, "izs", &flag, &mark, &utf8_message, &utf8_message_size);
  message = grn_encoding_convert_from_utf8(ctx,
                                           utf8_message,
                                           (ssize_t)utf8_message_size,
                                           &message_size);
  grn_query_logger_put(ctx, (unsigned int)flag, mark,
                       "%.*s",
                       (int)message_size,
                       message);
  grn_encoding_converted_free(ctx, message);

  return self;
}

void
grn_mrb_query_logger_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "QueryLogger", mrb->object_class);

  mrb_define_method(mrb, klass, "need_log?", query_logger_need_log_p,
                    MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "log_raw", query_logger_log_raw,
                    MRB_ARGS_REQ(3));

  grn_mrb_load(ctx, "query_logger/flag.rb");
  grn_mrb_load(ctx, "query_logger.rb");
}
#endif
