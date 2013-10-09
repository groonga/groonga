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

#include "../ctx_impl.h"

#ifdef GRN_WITH_MRUBY
#include <mruby.h>
#include <mruby/class.h>
#include <mruby/variable.h>
#include <mruby/data.h>

#include "../expr.h"
#include "../util.h"
#include "../mrb.h"
#include "mrb_expr.h"

static struct mrb_data_type mrb_grn_scan_info_type = { "Groonga::ScanInfo", NULL };

static mrb_value
mrb_grn_scan_info_new(mrb_state *mrb, scan_info *ptr)
{
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *klass;
  mrb_value mrb_ptr;

  mrb_ptr = mrb_cptr_value(mrb, ptr);
  klass = mrb_class_ptr(mrb_const_get(mrb, mrb_obj_value(module),
                                      mrb_intern(mrb, "ScanInfo")));
  return mrb_obj_new(mrb, klass, 1, &mrb_ptr);
}

static scan_info **
scan_info_build(grn_ctx *ctx, grn_obj *expr, int *n,
                grn_operator op, uint32_t size)
{
  grn_obj *var;
  scan_stat stat;
  int i, m = 0, o = 0;
  scan_info **sis, *si = NULL;
  grn_expr_code *c, *ce;
  grn_expr *e = (grn_expr *)expr;
  mrb_state *mrb = ctx->impl->mrb.state;
  mrb_value mrb_si;

  if (!(var = grn_expr_get_var_by_offset(ctx, expr, 0))) { return NULL; }
  for (stat = SCAN_START, c = e->codes, ce = &e->codes[e->codes_curr]; c < ce; c++) {
    switch (c->op) {
    case GRN_OP_MATCH :
    case GRN_OP_NEAR :
    case GRN_OP_NEAR2 :
    case GRN_OP_SIMILAR :
    case GRN_OP_PREFIX :
    case GRN_OP_SUFFIX :
    case GRN_OP_EQUAL :
    case GRN_OP_NOT_EQUAL :
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
    case GRN_OP_GEO_WITHINP5 :
    case GRN_OP_GEO_WITHINP6 :
    case GRN_OP_GEO_WITHINP8 :
    case GRN_OP_TERM_EXTRACT :
      if (stat < SCAN_COL1 || SCAN_CONST < stat) { return NULL; }
      stat = SCAN_START;
      m++;
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_AND_NOT :
    case GRN_OP_ADJUST :
      if (stat != SCAN_START) { return NULL; }
      o++;
      if (o >= m) { return NULL; }
      break;
    case GRN_OP_PUSH :
      stat = (c->value == var) ? SCAN_VAR : SCAN_CONST;
      break;
    case GRN_OP_GET_VALUE :
      switch (stat) {
      case SCAN_START :
      case SCAN_CONST :
      case SCAN_VAR :
        stat = SCAN_COL1;
        break;
      case SCAN_COL1 :
        stat = SCAN_COL2;
        break;
      case SCAN_COL2 :
        break;
      default :
        return NULL;
        break;
      }
      break;
    case GRN_OP_CALL :
      if ((c->flags & GRN_EXPR_CODE_RELATIONAL_EXPRESSION) || c + 1 == ce) {
        stat = SCAN_START;
        m++;
      } else {
        stat = SCAN_COL2;
      }
      break;
    default :
      return NULL;
      break;
    }
  }
  if (stat || m != o + 1) { return NULL; }
  if (!(sis = GRN_MALLOCN(scan_info *, m + m + o))) { return NULL; }
  for (i = 0, stat = SCAN_START, c = e->codes, ce = &e->codes[e->codes_curr]; c < ce; c++) {
    switch (c->op) {
    case GRN_OP_MATCH :
    case GRN_OP_NEAR :
    case GRN_OP_NEAR2 :
    case GRN_OP_SIMILAR :
    case GRN_OP_PREFIX :
    case GRN_OP_SUFFIX :
    case GRN_OP_EQUAL :
    case GRN_OP_NOT_EQUAL :
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
    case GRN_OP_GEO_WITHINP5 :
    case GRN_OP_GEO_WITHINP6 :
    case GRN_OP_GEO_WITHINP8 :
    case GRN_OP_TERM_EXTRACT :
      stat = SCAN_START;
      grn_scan_info_set_op(si, c->op);
      grn_scan_info_set_end(si, c - e->codes);
      sis[i++] = si;
      {
        int sid, k;
        grn_obj *index, *arg, **p = &arg;
        for (k = 0; (arg = grn_scan_info_get_arg(ctx, si, k)) ; k++) {
          if ((*p)->header.type == GRN_EXPR) {
            uint32_t j;
            grn_expr_code *ec;
            grn_expr *e = (grn_expr *)(*p);
            for (j = e->codes_curr, ec = e->codes; j--; ec++) {
              if (ec->value) {
                switch (ec->value->header.type) {
                case GRN_ACCESSOR :
                  if (grn_column_index(ctx, ec->value, c->op, &index, 1, &sid)) {
                    int32_t weight = grn_expr_code_get_weight(ctx, ec);
                    grn_scan_info_set_flags(si, grn_scan_info_get_flags(si) | SCAN_ACCESSOR);
                    mrb_si = mrb_grn_scan_info_new(mrb, si);
                    if (((grn_accessor *)ec->value)->next) {
                      mrb_funcall(mrb, mrb_si, "put_index", 3,
                                  mrb_cptr_value(mrb, ec->value),
                                  mrb_fixnum_value(sid),
                                  mrb_fixnum_value(weight));
                    } else {
                      mrb_funcall(mrb, mrb_si, "put_index", 3,
                                  mrb_cptr_value(mrb, index),
                                  mrb_fixnum_value(sid),
                                  mrb_fixnum_value(weight));
                    }
                  }
                  break;
                case GRN_COLUMN_FIX_SIZE :
                case GRN_COLUMN_VAR_SIZE :
                  if (grn_column_index(ctx, ec->value, c->op, &index, 1, &sid)) {
                    mrb_si = mrb_grn_scan_info_new(mrb, si);
                    mrb_funcall(mrb, mrb_si, "put_index", 3,
                                mrb_cptr_value(mrb, index),
                                mrb_fixnum_value(sid),
                                mrb_fixnum_value(grn_expr_code_get_weight(ctx, ec)));
                  }
                  break;
                case GRN_COLUMN_INDEX :
                  sid = 0;
                  index = ec->value;
                  if (j > 2 &&
                      ec[1].value &&
                      ec[1].value->header.domain == GRN_DB_UINT32 &&
                      ec[2].op == GRN_OP_GET_MEMBER) {
                    sid = GRN_UINT32_VALUE(ec[1].value) + 1;
                    j -= 2;
                    ec += 2;
                  }
                  mrb_si = mrb_grn_scan_info_new(mrb, si);
                  mrb_funcall(mrb, mrb_si, "put_index", 3,
                              mrb_cptr_value(mrb, index),
                              mrb_fixnum_value(sid),
                              mrb_fixnum_value(grn_expr_code_get_weight(ctx, ec)));
                  break;
                }
              }
            }
          } else if (GRN_DB_OBJP(*p)) {
            if (grn_column_index(ctx, *p, c->op, &index, 1, &sid)) {
              mrb_si = mrb_grn_scan_info_new(mrb, si);
              mrb_funcall(mrb, mrb_si, "put_index", 3,
                          mrb_cptr_value(mrb, index),
                          mrb_fixnum_value(sid),
                          mrb_fixnum_value(1));
            }
          } else if (GRN_ACCESSORP(*p)) {
            grn_scan_info_set_flags(si, grn_scan_info_get_flags(si) | SCAN_ACCESSOR);
            if (grn_column_index(ctx, *p, c->op, &index, 1, &sid)) {
              if (((grn_accessor *)(*p))->next) {
                mrb_si = mrb_grn_scan_info_new(mrb, si);
                mrb_funcall(mrb, mrb_si, "put_index", 3,
                            mrb_cptr_value(mrb, *p),
                            mrb_fixnum_value(sid),
                            mrb_fixnum_value(1));
              } else {
                mrb_si = mrb_grn_scan_info_new(mrb, si);
                mrb_funcall(mrb, mrb_si, "put_index", 3,
                            mrb_cptr_value(mrb, index),
                            mrb_fixnum_value(sid),
                            mrb_fixnum_value(1));
              }
            }
          } else {
            grn_scan_info_set_query(si, *p);
          }
        }
      }
      si = NULL;
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_AND_NOT :
    case GRN_OP_ADJUST :
      if (!grn_scan_info_put_logical_op(ctx, sis, &i, c->op, c - e->codes)) { return NULL; }
      stat = SCAN_START;
      break;
    case GRN_OP_PUSH :
      if (!si) {
        si = grn_scan_info_open(ctx, c - e->codes);
        if (!si) {
          int j;
          for (j = 0; j < i; j++) { grn_scan_info_close(ctx, sis[j]); }
          GRN_FREE(sis);
          return NULL;
        }
      }
      if (c->value == var) {
        stat = SCAN_VAR;
      } else {
        grn_scan_info_push_arg(si, c->value);
        if (stat == SCAN_START) { grn_scan_info_set_flags(si, grn_scan_info_get_flags(si) | SCAN_PRE_CONST); }
        stat = SCAN_CONST;
      }
      break;
    case GRN_OP_GET_VALUE :
      switch (stat) {
      case SCAN_START :
        if (!si) {
          si = grn_scan_info_open(ctx, c - e->codes);
          if (!si) {
            int j;
            for (j = 0; j < i; j++) { grn_scan_info_close(ctx, sis[j]); }
            GRN_FREE(sis);
            return NULL;
          }
        }
        // fallthru
      case SCAN_CONST :
      case SCAN_VAR :
        stat = SCAN_COL1;
        grn_scan_info_push_arg(si, c->value);
        break;
      case SCAN_COL1 :
        {
          int j;
          grn_obj inspected;
          GRN_TEXT_INIT(&inspected, 0);
          GRN_TEXT_PUTS(ctx, &inspected, "<");
          grn_inspect_name(ctx, &inspected, c->value);
          GRN_TEXT_PUTS(ctx, &inspected, ">: <");
          grn_inspect(ctx, &inspected, expr);
          GRN_TEXT_PUTS(ctx, &inspected, ">");
          ERR(GRN_INVALID_ARGUMENT,
              "invalid expression: can't use column as a value: %.*s",
              (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
          GRN_OBJ_FIN(ctx, &inspected);
          for (j = 0; j < i; j++) { grn_scan_info_close(ctx, sis[j]); }
          GRN_FREE(sis);
          return NULL;
        }
        stat = SCAN_COL2;
        break;
      case SCAN_COL2 :
        break;
      default :
        break;
      }
      break;
    case GRN_OP_CALL :
      if (!si) {
        si = grn_scan_info_open(ctx, c - e->codes);
        if (!si) {
          int j;
          for (j = 0; j < i; j++) { grn_scan_info_close(ctx, sis[j]); }
          GRN_FREE(sis);
          return NULL;
        }
      }
      if ((c->flags & GRN_EXPR_CODE_RELATIONAL_EXPRESSION) || c + 1 == ce) {
        stat = SCAN_START;
        grn_scan_info_set_op(si, c->op);
        grn_scan_info_set_end(si, c - e->codes);
        sis[i++] = si;
        /* better index resolving framework for functions should be implemented */
        {
          int sid, k;
          grn_obj *index, *arg, **p = &arg;
          for (k = 0; (arg = grn_scan_info_get_arg(ctx, si, k)) ; k++) {
            if (GRN_DB_OBJP(*p)) {
              if (grn_column_index(ctx, *p, c->op, &index, 1, &sid)) {
                mrb_si = mrb_grn_scan_info_new(mrb, si);
                mrb_funcall(mrb, mrb_si, "put_index", 3,
                            mrb_cptr_value(mrb, index),
                            mrb_fixnum_value(sid),
                            mrb_fixnum_value(1));
              }
            } else if (GRN_ACCESSORP(*p)) {
              grn_scan_info_set_flags(si, grn_scan_info_get_flags(si) | SCAN_ACCESSOR);
              if (grn_column_index(ctx, *p, c->op, &index, 1, &sid)) {
                mrb_si = mrb_grn_scan_info_new(mrb, si);
                mrb_funcall(mrb, mrb_si, "put_index", 3,
                            mrb_cptr_value(mrb, index),
                            mrb_fixnum_value(sid),
                            mrb_fixnum_value(1));
              }
            } else {
              grn_scan_info_set_query(si, *p);
            }
          }
        }
        si = NULL;
      } else {
        stat = SCAN_COL2;
      }
      break;
    default :
      break;
    }
  }
  if (op == GRN_OP_OR && !size) {
    // for debug
    if (!(grn_scan_info_get_flags(sis[0]) & SCAN_PUSH) || (grn_scan_info_get_logical_op(sis[0]) != op)) {
      int j;
      ERR(GRN_INVALID_ARGUMENT, "invalid expr");
      for (j = 0; j < i; j++) { grn_scan_info_close(ctx, sis[j]); }
      GRN_FREE(sis);
      return NULL;
    } else {
      grn_scan_info_set_flags(sis[0], grn_scan_info_get_flags(sis[0]) & ~SCAN_PUSH);
      grn_scan_info_set_logical_op(sis[0], op);
    }
  } else {
    if (!grn_scan_info_put_logical_op(ctx, sis, &i, op, c - e->codes)) { return NULL; }
  }
  *n = i;
  return sis;
}

static mrb_value
mrb_grn_expr_build(mrb_state *mrb, mrb_value self)
{
  int *n;
  uint32_t size;
  scan_info **sis;
  grn_operator op;
  grn_obj *expr;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  mrb_value mrb_expr, mrb_n;

  mrb_get_args(mrb, "ooii", &mrb_expr, &mrb_n, &op, &size);
  expr = mrb_cptr(mrb_expr);
  n = mrb_cptr(mrb_n);

  sis = scan_info_build(ctx, expr, n, op, size);
  return mrb_cptr_value(mrb, sis);
}

static mrb_value
mrb_grn_scan_info_initialize(mrb_state *mrb, mrb_value self)
{
  mrb_value mrb_ptr;

  mrb_get_args(mrb, "o", &mrb_ptr);
  DATA_TYPE(self) = &mrb_grn_scan_info_type;
  DATA_PTR(self) = mrb_cptr(mrb_ptr);
  return self;
}

static mrb_value
mrb_grn_scan_info_put_index(mrb_state *mrb, mrb_value self)
{
  int sid;
  int32_t weight;
  scan_info *si;
  grn_ctx *ctx = (grn_ctx *)mrb->ud;
  grn_obj *obj;
  mrb_value mrb_obj;

  mrb_get_args(mrb, "oii", &mrb_obj, &sid, &weight);
  si = DATA_PTR(self);
  obj = mrb_cptr(mrb_obj);
  grn_scan_info_put_index(ctx, si, obj, sid, weight);
  return self;
}

void
grn_mrb_expr_init(grn_ctx *ctx)
{
  mrb_state *mrb = ctx->impl->mrb.state;
  struct RClass *module = ctx->impl->mrb.module;
  struct RClass *klass;

  mrb_define_class_method(mrb, module,
                          "build", mrb_grn_expr_build, MRB_ARGS_REQ(4));

  klass = mrb_define_class_under(mrb, module, "ScanInfo", mrb->object_class);
  MRB_SET_INSTANCE_TT(klass, MRB_TT_DATA);
  mrb_define_method(mrb, klass, "initialize", mrb_grn_scan_info_initialize, MRB_ARGS_REQ(1));
  mrb_define_method(mrb, klass, "put_index", mrb_grn_scan_info_put_index, MRB_ARGS_REQ(3));
  grn_mrb_load(ctx, "expression.rb");
}

scan_info **
grn_mrb_scan_info_build(grn_ctx *ctx, grn_obj *expr, int *n,
                        grn_operator op, uint32_t size)
{
  scan_info **sis;
  mrb_state *mrb = ctx->impl->mrb.state;
  mrb_value mrb_sis;

  mrb_sis = mrb_funcall(mrb, mrb_obj_value(ctx->impl->mrb.module), "build", 4,
                        mrb_cptr_value(mrb, expr),
                        mrb_cptr_value(mrb, n),
                        mrb_fixnum_value(op),
                        mrb_fixnum_value(size));
  sis = mrb_cptr(mrb_sis);
  return sis;
}
#endif
