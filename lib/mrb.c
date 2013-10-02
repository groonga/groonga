/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2013 Brazil

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

#include "mrb.h"
#include "ctx_impl.h"

#ifdef GRN_WITH_MRUBY
# include <mruby/proc.h>
# include <mruby/compile.h>
# include <mruby/variable.h>
# include <mruby/data.h>
# include <mruby/string.h>
#endif

#ifdef GRN_WITH_MRUBY
void
grn_ctx_impl_mrb_init(grn_ctx *ctx)
{
  const char *grn_mruby_enabled;
  grn_mruby_enabled = getenv("GRN_MRUBY_ENABLED");
  if (grn_mruby_enabled && strcmp(grn_mruby_enabled, "no") == 0) {
    ctx->impl->mrb = NULL;
  } else {
    ctx->impl->mrb = mrb_open();
  }
}

void
grn_ctx_impl_mrb_fin(grn_ctx *ctx)
{
  if (ctx->impl->mrb) {
    mrb_close(ctx->impl->mrb);
    ctx->impl->mrb = NULL;
  }
}

mrb_value
grn_mrb_eval(grn_ctx *ctx, const char *script, int script_length)
{
  mrb_state *mrb = ctx->impl->mrb;
  int n;
  mrb_value result;
  struct mrb_parser_state *parser;

  if (!mrb) {
    return mrb_nil_value();
  }

  if (script_length < 0) {
    script_length = strlen(script);
  }
  parser = mrb_parse_nstring(mrb, script, script_length, NULL);
  n = mrb_generate_code(mrb, parser);
  result = mrb_run(mrb,
                   mrb_proc_new(mrb, mrb->irep[n]),
                   mrb_top_self(mrb));
  mrb_parser_free(parser);

  return result;
}

mrb_value
grn_mrb_from_grn(grn_ctx *ctx, grn_obj **argv)
{
  grn_obj *obj = (*argv)++;
  mrb_state *mrb = ctx->impl->mrb;
  switch (obj->header.type) {
  case GRN_EXPR:
    return grn_mrb_obj_new(mrb, "Expr", obj);
  case GRN_PTR:
    {
      const char *cname = GRN_TEXT_VALUE(*argv);
      (*argv)++;
      return grn_mrb_obj_new(mrb, cname, GRN_PTR_VALUE(obj));
    }
  default:
    switch (obj->header.domain) {
    case GRN_DB_INT32:
      return mrb_fixnum_value(GRN_INT32_VALUE(obj));
    }
  }
  return grn_mrb_obj_new(mrb, "Obj", obj);
}

grn_rc
grn_mrb_send(grn_ctx *ctx, grn_obj *grn_recv, const char *name, int argc,
             grn_obj *grn_argv, grn_obj *grn_object)
{
  int i, offset, ai;
  grn_rc stat;
  mrb_state *mrb = ctx->impl->mrb;
  mrb_value ret, recv, *argv;
  ai = mrb_gc_arena_save(mrb);
  argv = GRN_MALLOCN(mrb_value, argc);
  recv = grn_mrb_from_grn(ctx, &grn_recv);
  for (i = offset = 0; i < argc; i++) {
    argv[i] = grn_mrb_from_grn(ctx, &grn_argv);
  }
  ret = mrb_funcall_argv(mrb, recv, mrb_intern(mrb, name), argc, argv);
  GRN_FREE(argv);
  if (mrb->exc) {
    mrb_value msg = mrb_inspect(mrb, mrb_obj_value(mrb->exc));
    ERR(GRN_UNKNOWN_ERROR, "mruby error - %s", RSTRING_PTR(msg));
    stat = GRN_UNKNOWN_ERROR;
    mrb->exc = NULL;
  } else {
    GRN_VOID_INIT(grn_object);
    stat = grn_mrb_to_grn(ctx, ret, grn_object);
    mrb_gc_arena_restore(mrb, ai);
  }
  return stat;
}

grn_rc
grn_mrb_to_grn(grn_ctx *ctx, mrb_value mrb_object, grn_obj *grn_object)
{
  grn_rc rc = GRN_SUCCESS;

  switch (mrb_type(mrb_object)) {
  case MRB_TT_FIXNUM :
    grn_obj_reinit(ctx, grn_object, GRN_DB_INT32, 0);
    GRN_INT32_SET(ctx, grn_object, mrb_fixnum(mrb_object));
    break;
  case MRB_TT_FALSE :
    grn_obj_reinit(ctx, grn_object, GRN_DB_VOID, 0);
  default :
    rc = GRN_INVALID_ARGUMENT;
    break;
  }

  return rc;
}

mrb_value
grn_mrb_obj_new(mrb_state *mrb, const char *cname, void *ptr)
{
  mrb_value obj, type;
  struct RClass *klass;
  if (!ptr) { return mrb_nil_value(); }
  klass = mrb_class_get(mrb, cname);
  type  = mrb_iv_get(mrb, mrb_obj_value(klass), mrb_intern(mrb, "type"));
  obj = mrb_obj_value(Data_Wrap_Struct(mrb, klass, mrb_voidp(type), ptr));
  return obj;
}

#else
void
grn_ctx_impl_mrb_init(grn_ctx *ctx)
{
}

void
grn_ctx_impl_mrb_fin(grn_ctx *ctx)
{
}
#endif
