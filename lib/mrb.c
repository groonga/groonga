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
#include "expr.h"
#include "util.h"

#ifdef GRN_WITH_MRUBY
# include <mruby/proc.h>
# include <mruby/compile.h>
# include <mruby/class.h>
# include <mruby/variable.h>
# include <mruby/data.h>
# include <mruby/array.h>
# include <mruby/string.h>
# include <mruby/dump.h>
#endif

#ifdef GRN_WITH_MRUBY
#define MRB_GRN_CONST(name) \
  mrb_define_const(mrb, klass, #name, mrb_fixnum_value(name))
#ifdef mrb_voidp_value
# undef mrb_voidp_value
# define mrb_voidp_value(p) mrb_cptr_value(mrb, (p))
#endif

extern const uint8_t grn_mrb_irepdata_scaninfo[];

static mrb_value
mrb_grn_err(mrb_state *mrb, mrb_value self)
{
  char *msg;
  grn_rc rc;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_get_args(mrb, "iz", &rc, &msg);
  ERR(rc, "%s", msg);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_flags(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  si = DATA_PTR(self);
  return mrb_fixnum_value(grn_scan_info_get_flags(si));
}

static mrb_value
mrb_grn_scan_info_set_flags(mrb_state *mrb, mrb_value self)
{
  int flags;
  scan_info *si;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "i", &flags);
  grn_scan_info_set_flags(si, flags);
  return self;
}

static mrb_value
mrb_grn_scan_info_get_logical_op(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  si = DATA_PTR(self);
  return mrb_fixnum_value(grn_scan_info_get_logical_op(si));
}

static mrb_value
mrb_grn_scan_info_set_logical_op(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  grn_operator logical_op;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "i", &logical_op);
  grn_scan_info_set_logical_op(si, logical_op);
  return self;
}

static mrb_value
mrb_grn_scan_info_set_op(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int op;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "i", &op);
  grn_scan_info_set_op(si, op);
  return self;
}

static mrb_value
mrb_grn_scan_info_set_end(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  int end;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "i", &end);
  grn_scan_info_set_end(si, end);
  return self;
}

static mrb_value
mrb_grn_scan_info_set_query(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_value query;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "o", &query);
  grn_scan_info_set_query(si, DATA_PTR(query));
  return self;
}

static mrb_value
mrb_grn_scan_info_push_arg(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  mrb_value arg;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "o", &arg);
  grn_scan_info_push_arg(si, DATA_PTR(arg));
  return self;
}

static grn_bool
mrb_grn_scan_info_each_arg_callback(grn_ctx *ctx, grn_obj *obj, void *user_data)
{
  mrb_state *mrb = ctx->impl->mrb;
  mrb_value b = *(mrb_value *)user_data;
  mrb_yield(mrb, b, grn_mrb_obj_new(mrb, "Obj", obj));
  return GRN_TRUE;
}

static mrb_value
mrb_grn_scan_info_each_arg(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value b;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "&", &b);
  grn_scan_info_each_arg(ctx, si, mrb_grn_scan_info_each_arg_callback, &b);
  return self;
}

static mrb_value
mrb_grn_scan_info_put_index(mrb_state *mrb, mrb_value self)
{
  int sid;
  int32_t weight;
  scan_info *si;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value index;
  si = DATA_PTR(self);
  mrb_get_args(mrb, "oii", &index, &sid, &weight);
  grn_scan_info_put_index(ctx, si, DATA_PTR(index), sid, weight);
  return self;
}

static mrb_value
mrb_grn_scan_info_free(mrb_state *mrb, mrb_value self)
{
  scan_info *si;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  si = DATA_PTR(self);
  grn_scan_info_free(ctx, si);
  return self;
}

static mrb_value
mrb_grn_scan_info_v_aref(mrb_state *mrb, mrb_value self)
{
  int i;
  scan_info **sis;
  sis = DATA_PTR(self);
  mrb_get_args(mrb, "i", &i);
  return grn_mrb_obj_new(mrb, "Scaninfo", sis[i]);
}

static mrb_value
mrb_grn_scan_info_v_aset(mrb_state *mrb, mrb_value self)
{
  int i;
  scan_info **sis;
  mrb_value si;
  sis = DATA_PTR(self);
  mrb_get_args(mrb, "io", &i, &si);
  sis[i] = DATA_PTR(si);
  return self;
}

static mrb_value
mrb_grn_scan_info_v_free(mrb_state *mrb, mrb_value self)
{
  scan_info **sis;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  sis = DATA_PTR(self);
  GRN_FREE(sis);
  return self;
}

static mrb_value
mrb_grn_exprcode_op(mrb_state *mrb, mrb_value self)
{
  grn_expr_code *c;
  c = DATA_PTR(self);
  return mrb_fixnum_value(c->op);
}

static mrb_value
mrb_grn_exprcode_value(mrb_state *mrb, mrb_value self)
{
  grn_expr_code *c;
  c = DATA_PTR(self);
  return grn_mrb_obj_new(mrb, "Obj", c->value);
}

static mrb_value
mrb_grn_exprcode_flags(mrb_state *mrb, mrb_value self)
{
  unsigned int flags;
  grn_expr_code *c;
  c = DATA_PTR(self);
  if (mrb_get_args(mrb, "|i", &flags) == 1) {
    return c->flags & flags ? mrb_true_value() : mrb_false_value();
  }
  return mrb_fixnum_value(c->flags);
}

static mrb_value
mrb_grn_exprcode_weight(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_expr_code *c;
  c = DATA_PTR(self);
  return mrb_fixnum_value(grn_expr_code_get_weight(ctx, c));
}

static mrb_value
mrb_grn_obj_type(mrb_state *mrb, mrb_value self)
{
  grn_obj *obj;
  obj = DATA_PTR(self);
  return mrb_fixnum_value(obj->header.type);
}

static mrb_value
mrb_grn_obj_domain(mrb_state *mrb, mrb_value self)
{
  grn_obj *obj;
  obj = DATA_PTR(self);
  return mrb_fixnum_value(obj->header.domain);
}

static mrb_value
mrb_grn_obj_db_p(mrb_state *mrb, mrb_value self)
{
  grn_obj *obj;
  obj = DATA_PTR(self);
  return GRN_DB_OBJP(obj) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_grn_obj_accessor_p(mrb_state *mrb, mrb_value self)
{
  grn_obj *obj;
  obj = DATA_PTR(self);
  return GRN_ACCESSORP(obj) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_grn_accessor_next(mrb_state *mrb, mrb_value self)
{
  grn_accessor *accessor = DATA_PTR(self);
  return grn_mrb_obj_new(mrb, "Obj", accessor->next);
}

static mrb_value
mrb_grn_obj_to_accessor(mrb_state *mrb, mrb_value self)
{
  grn_obj *obj = DATA_PTR(self);
  return grn_mrb_obj_new(mrb, "Accessor", obj);
}

static mrb_value
mrb_grn_obj_to_expr(mrb_state *mrb, mrb_value self)
{
  grn_obj *obj = DATA_PTR(self);
  return grn_mrb_obj_new(mrb, "Expr", obj);
}

static mrb_value
mrb_grn_obj_equal(mrb_state *mrb, mrb_value self)
{
  mrb_value obj;
  mrb_get_args(mrb, "o", &obj);
  return DATA_PTR(self) == DATA_PTR(obj) ? mrb_true_value() : mrb_false_value();
}

static mrb_value
mrb_grn_obj_name(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj inspected;
  mrb_value str;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect_name(ctx, &inspected, DATA_PTR(self));
  str = mrb_str_new(mrb, GRN_TEXT_VALUE(&inspected), (int)GRN_TEXT_LEN(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
  return str;
}

static mrb_value
mrb_grn_obj_inspect(mrb_state *mrb, mrb_value self)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj inspected;
  mrb_value str;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect(ctx, &inspected, DATA_PTR(self));
  str = mrb_str_new(mrb, GRN_TEXT_VALUE(&inspected), (int)GRN_TEXT_LEN(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
  return str;
}

static mrb_value
mrb_grn_obj_column_index(mrb_state *mrb, mrb_value self)
{
  int ret, sid, op;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *obj, *index;
  mrb_value ary;
  obj = DATA_PTR(self);
  mrb_get_args(mrb, "i", &op);
  ret = grn_column_index(ctx, obj, op, &index, 1, &sid);
  if (!ret) { return mrb_nil_value(); }
  ary = mrb_ary_new_capa(mrb, 2);
  mrb_ary_push(mrb, ary, grn_mrb_obj_new(mrb, "Obj", index));
  mrb_ary_push(mrb, ary, mrb_fixnum_value(sid));
  return ary;
}

static mrb_value
mrb_grn_expr_aref(mrb_state *mrb, mrb_value self)
{
  int i;
  grn_expr *e;
  mrb_get_args(mrb, "i", &i);
  e = DATA_PTR(self);
  if (i >= e->codes_curr) { return mrb_nil_value(); }
  return grn_mrb_obj_new(mrb, "ExprCode", &e->codes[i]);
}

static mrb_value
mrb_grn_expr_codes_curr(mrb_state *mrb, mrb_value self)
{
  grn_expr *e;
  e = DATA_PTR(self);
  return mrb_fixnum_value(e->codes_curr);
}

static mrb_value
mrb_grn_expr_index(mrb_state *mrb, mrb_value self)
{
  grn_expr *e;
  grn_expr_code *c;
  mrb_value mrb_c;
  e = DATA_PTR(self);
  mrb_get_args(mrb, "o", &mrb_c);
  c = (grn_expr_code *)DATA_PTR(mrb_c);
  return mrb_fixnum_value(c - e->codes);
}

static mrb_value
mrb_grn_expr_each(mrb_state *mrb, mrb_value self)
{
  grn_expr *e;
  grn_expr_code *c, *ce;
  mrb_value b;
  e = DATA_PTR(self);
  mrb_get_args(mrb, "&", &b);
  for (c = e->codes, ce = &e->codes[e->codes_curr]; c < ce; c++) {
    if (!mrb_test(mrb_yield(mrb, b, grn_mrb_obj_new(mrb, "ExprCode", c)))) {
      break;
    }
  }
  return self;
}

static mrb_value
mrb_grn_expr_alloc_si(mrb_state *mrb, mrb_value self)
{
   uint32_t start;
  scan_info *si;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_get_args(mrb, "i", &start);
  si = grn_scan_info_alloc(ctx, start);
  if (!si) { return mrb_nil_value(); }
  return grn_mrb_obj_new(mrb, "Scaninfo", si);
}

static struct mrb_data_type mrb_scaninfo_type = { "Scaninfo", NULL };
static struct mrb_data_type mrb_scaninfov_type = { "ScaninfoVector", NULL };
static struct mrb_data_type mrb_exprcode_type = { "ExprCode", NULL };
static struct mrb_data_type mrb_obj_type = { "Obj", NULL };
static struct mrb_data_type mrb_accessor_type = { "Accessor", NULL };
static struct mrb_data_type mrb_expr_type = { "Expr", NULL };

static inline struct RClass *
mrb_grn_class(mrb_state *mrb, const char *name, struct RClass *parent,
              struct mrb_data_type *type)
{
  struct RClass *klass;
  klass = mrb_define_class(mrb, name, parent);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_iv_set(mrb, mrb_obj_value(klass), mrb_intern(mrb, "type"),
             mrb_voidp_value(type));
  return klass;
}

void grn_mrb_init_expr(grn_ctx *ctx)
{
  struct RClass *klass, *obj_class;
  mrb_state *mrb = ctx->impl->mrb;

  obj_class = klass = mrb_define_class(mrb, "Obj", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_iv_set(mrb, mrb_obj_value(klass), mrb_intern(mrb, "type"),
             mrb_voidp_value(&mrb_obj_type));
  mrb_define_method(mrb, klass, "type", mrb_grn_obj_type, ARGS_NONE());
  mrb_define_method(mrb, klass, "domain", mrb_grn_obj_domain, ARGS_NONE());
  mrb_define_method(mrb, klass, "db?", mrb_grn_obj_db_p, ARGS_NONE());
  mrb_define_method(mrb, klass, "accessor?", mrb_grn_obj_accessor_p,
                    ARGS_NONE());
  mrb_define_method(mrb, klass, "to_accessor", mrb_grn_obj_to_accessor,
                    ARGS_NONE());
  mrb_define_method(mrb, klass, "to_expr", mrb_grn_obj_to_expr, ARGS_NONE());
  mrb_define_method(mrb, klass, "==", mrb_grn_obj_equal, ARGS_REQ(1));
  mrb_define_method(mrb, klass, "name", mrb_grn_obj_name, ARGS_NONE());
  mrb_define_method(mrb, klass, "inspect", mrb_grn_obj_inspect, ARGS_NONE());
  mrb_define_method(mrb, klass, "column_index", mrb_grn_obj_column_index,
                    ARGS_REQ(1));
  klass = mrb_grn_class(mrb, "Accessor", obj_class, &mrb_accessor_type);
  mrb_define_method(mrb, klass, "next", mrb_grn_accessor_next,
                    ARGS_NONE());
  klass = mrb_grn_class(mrb, "Expr", obj_class, &mrb_expr_type);
  MRB_GRN_CONST(GRN_OP_OR);
  MRB_GRN_CONST(GRN_OP_MATCH);
  MRB_GRN_CONST(GRN_OP_NEAR);
  MRB_GRN_CONST(GRN_OP_NEAR2);
  MRB_GRN_CONST(GRN_OP_SIMILAR);
  MRB_GRN_CONST(GRN_OP_PREFIX);
  MRB_GRN_CONST(GRN_OP_SUFFIX);
  MRB_GRN_CONST(GRN_OP_EQUAL);
  MRB_GRN_CONST(GRN_OP_NOT_EQUAL);
  MRB_GRN_CONST(GRN_OP_LESS);
  MRB_GRN_CONST(GRN_OP_GREATER);
  MRB_GRN_CONST(GRN_OP_LESS_EQUAL);
  MRB_GRN_CONST(GRN_OP_GREATER_EQUAL);
  MRB_GRN_CONST(GRN_OP_GEO_WITHINP5);
  MRB_GRN_CONST(GRN_OP_GEO_WITHINP6);
  MRB_GRN_CONST(GRN_OP_GEO_WITHINP8);
  MRB_GRN_CONST(GRN_OP_TERM_EXTRACT);
  MRB_GRN_CONST(GRN_OP_AND);
  MRB_GRN_CONST(GRN_OP_OR);
  MRB_GRN_CONST(GRN_OP_AND_NOT);
  MRB_GRN_CONST(GRN_OP_ADJUST);
  MRB_GRN_CONST(GRN_OP_PUSH);
  MRB_GRN_CONST(GRN_OP_GET_VALUE);
  MRB_GRN_CONST(GRN_OP_CALL);
  MRB_GRN_CONST(GRN_OP_GET_MEMBER);
  MRB_GRN_CONST(GRN_INVALID_ARGUMENT);
  MRB_GRN_CONST(GRN_EXPR);
  MRB_GRN_CONST(GRN_ACCESSOR);
  MRB_GRN_CONST(GRN_COLUMN_FIX_SIZE);
  MRB_GRN_CONST(GRN_COLUMN_VAR_SIZE);
  MRB_GRN_CONST(GRN_COLUMN_INDEX);
  MRB_GRN_CONST(GRN_DB_UINT32);
  MRB_GRN_CONST(GRN_EXPR_CODE_RELATIONAL_EXPRESSION);
  MRB_GRN_CONST(SCAN_PUSH);
  MRB_GRN_CONST(SCAN_POP);
  MRB_GRN_CONST(SCAN_START);
  MRB_GRN_CONST(SCAN_VAR);
  MRB_GRN_CONST(SCAN_PRE_CONST);
  MRB_GRN_CONST(SCAN_CONST);
  MRB_GRN_CONST(SCAN_COL1);
  MRB_GRN_CONST(SCAN_COL2);
  MRB_GRN_CONST(SCAN_ACCESSOR);
  mrb_define_method(mrb, klass, "err", mrb_grn_err, ARGS_REQ(2));
  mrb_define_method(mrb, klass, "[]", mrb_grn_expr_aref, ARGS_REQ(4));
  mrb_define_method(mrb, klass, "codes_curr", mrb_grn_expr_codes_curr,
                    ARGS_NONE());
  mrb_define_method(mrb, klass, "index", mrb_grn_expr_index, ARGS_REQ(1));
  mrb_define_method(mrb, klass, "each", mrb_grn_expr_each, ARGS_BLOCK());

  mrb_define_method(mrb, klass, "alloc_si",
                    mrb_grn_expr_alloc_si, ARGS_REQ(1));
  klass = mrb_grn_class(mrb, "Scaninfo", mrb->object_class, &mrb_scaninfo_type);
  mrb_define_method(mrb, klass, "put_index", mrb_grn_scan_info_put_index,
                    ARGS_REQ(3));
  mrb_define_method(mrb, klass, "flags", mrb_grn_scan_info_get_flags, ARGS_NONE());
  mrb_define_method(mrb, klass, "flags=", mrb_grn_scan_info_set_flags,
                    ARGS_REQ(1));
  mrb_define_method(mrb, klass, "logical_op", mrb_grn_scan_info_get_logical_op,
                    ARGS_NONE());
  mrb_define_method(mrb, klass, "logical_op=", mrb_grn_scan_info_set_logical_op,
                    ARGS_REQ(1));
  mrb_define_method(mrb, klass, "op=", mrb_grn_scan_info_set_op, ARGS_REQ(1));
  mrb_define_method(mrb, klass, "fin=", mrb_grn_scan_info_set_end, ARGS_REQ(1));
  mrb_define_method(mrb, klass, "query=", mrb_grn_scan_info_set_query, ARGS_REQ(1));
  mrb_define_method(mrb, klass, "push_arg", mrb_grn_scan_info_push_arg,
                    ARGS_REQ(1));
  mrb_define_method(mrb, klass, "each_arg", mrb_grn_scan_info_each_arg,
                    ARGS_BLOCK());
  mrb_define_method(mrb, klass, "free", mrb_grn_scan_info_free, ARGS_NONE());
  klass = mrb_grn_class(mrb, "ScaninfoVector", mrb->object_class,
                        &mrb_scaninfov_type);
  mrb_define_method(mrb, klass, "[]", mrb_grn_scan_info_v_aref, ARGS_REQ(1));
  mrb_define_method(mrb, klass, "[]=", mrb_grn_scan_info_v_aset, ARGS_REQ(2));
  mrb_define_method(mrb, klass, "free", mrb_grn_scan_info_v_free, ARGS_NONE());
  klass = mrb_grn_class(mrb, "ExprCode", mrb->object_class, &mrb_exprcode_type);
  mrb_define_method(mrb, klass, "op", mrb_grn_exprcode_op, ARGS_NONE());
  mrb_define_method(mrb, klass, "value", mrb_grn_exprcode_value, ARGS_NONE());
  mrb_define_method(mrb, klass, "flags", mrb_grn_exprcode_flags, ARGS_OPT(1));
  mrb_define_method(mrb, klass, "weight", mrb_grn_exprcode_weight, ARGS_NONE());
}

void
grn_ctx_impl_mrb_init(grn_ctx *ctx)
{
  const char *grn_mruby_enabled;
  grn_mruby_enabled = getenv("GRN_MRUBY_ENABLED");
  if (grn_mruby_enabled && strcmp(grn_mruby_enabled, "no") == 0) {
    ctx->impl->mrb = NULL;
  } else {
    int32_t n;
    ctx->impl->mrb = mrb_open();
    ctx->impl->mrb->ud = ctx;
    grn_mrb_init_expr(ctx);
    n = mrb_read_irep(ctx->impl->mrb, grn_mrb_irepdata_scaninfo);
    mrb_run(ctx->impl->mrb,
            mrb_proc_new(ctx->impl->mrb, ctx->impl->mrb->irep[n]),
            mrb_top_self(ctx->impl->mrb));
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
