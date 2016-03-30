/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2016 Brazil

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
#include <mruby/proc.h>
#include <mruby/compile.h>
#include <mruby/opcode.h>

#include "../grn_mrb.h"
#include "mrb_ctx.h"
#include "mrb_eval_context.h"

static mrb_value
eval_context_compile(mrb_state *mrb, mrb_value self)
{
  char *script;
  mrb_int script_length;
  struct mrb_parser_state *parser;
  struct RProc *proc;

  mrb_get_args(mrb, "s", &script, &script_length);
  parser = mrb_parse_nstring(mrb, script, script_length, NULL);
  proc = mrb_generate_code(mrb, parser);
  {
    mrb_code *iseq = proc->body.irep->iseq;
    while (GET_OPCODE(*iseq) != OP_STOP) {
      iseq++;
    }
    *iseq = MKOP_AB(OP_RETURN, 1, OP_R_NORMAL);
  }
  mrb_parser_free(parser);

  return mrb_obj_value(proc);
}

void
grn_mrb_eval_context_init(grn_ctx *ctx)
{
  grn_mrb_data *data = &(ctx->impl->mrb);
  mrb_state *mrb = data->state;
  struct RClass *module = data->module;
  struct RClass *klass;

  klass = mrb_define_class_under(mrb, module, "EvalContext", mrb->object_class);

  mrb_define_method(mrb, klass, "compile", eval_context_compile,
                    MRB_ARGS_REQ(1));
}
#endif
