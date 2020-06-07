/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2010-2018 Brazil
  Copyright(C) 2018-2020 Sutou Kouhei <kou@clear-code.com>

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

#include "grn.h"
#include "grn_tokenizer.h"

#include "grn_db.h"
#include "grn_ctx_impl.h"
#include "grn_ctx_impl_mrb.h"
#include <string.h>
#include "grn_ii.h"
#include "grn_geo.h"
#include "grn_expr.h"
#include "grn_expr_code.h"
#include "grn_expr_executor.h"
#include "grn_scanner.h"
#include "grn_util.h"
#include "grn_report.h"
#include "grn_token_cursor.h"
#include "grn_posting.h"
#include "grn_selector.h"
#include "grn_mrb.h"
#include "mrb/mrb_expr.h"

static double grn_table_select_enough_filtered_ratio = 0.01;
static int grn_table_select_max_n_enough_filtered_records = 1000;
static grn_bool grn_table_select_and_min_skip_enable = GRN_TRUE;
static grn_bool grn_scan_info_regexp_dot_asterisk_enable = GRN_TRUE;
static grn_bool grn_query_log_show_condition = GRN_TRUE;

void
grn_expr_init_from_env(void)
{
  {
    char grn_table_select_enough_filtered_ratio_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_SELECT_ENOUGH_FILTERED_RATIO",
               grn_table_select_enough_filtered_ratio_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_table_select_enough_filtered_ratio_env[0]) {
      grn_table_select_enough_filtered_ratio =
        atof(grn_table_select_enough_filtered_ratio_env);
    }
  }

  {
    char grn_table_select_max_n_enough_filtered_records_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_SELECT_MAX_N_ENOUGH_FILTERED_RECORDS",
               grn_table_select_max_n_enough_filtered_records_env,
               GRN_ENV_BUFFER_SIZE);
    if (grn_table_select_max_n_enough_filtered_records_env[0]) {
      grn_table_select_max_n_enough_filtered_records =
        atoi(grn_table_select_max_n_enough_filtered_records_env);
    }
  }

  {
    char grn_table_select_and_min_skip_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_TABLE_SELECT_AND_MIN_SKIP_ENABLE",
               grn_table_select_and_min_skip_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_table_select_and_min_skip_enable_env, "no") == 0) {
      grn_table_select_and_min_skip_enable = GRN_FALSE;
    } else {
      grn_table_select_and_min_skip_enable = GRN_TRUE;
    }
  }

  {
    char grn_scan_info_regexp_dot_asterisk_enable_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_SCAN_INFO_REGEXP_DOT_ASTERISK_ENABLE",
               grn_scan_info_regexp_dot_asterisk_enable_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_scan_info_regexp_dot_asterisk_enable_env, "no") == 0) {
      grn_scan_info_regexp_dot_asterisk_enable = GRN_FALSE;
    } else {
      grn_scan_info_regexp_dot_asterisk_enable = GRN_TRUE;
    }
  }

  {
    char grn_query_log_show_condition_env[GRN_ENV_BUFFER_SIZE];
    grn_getenv("GRN_QUERY_LOG_SHOW_CONDITION",
               grn_query_log_show_condition_env,
               GRN_ENV_BUFFER_SIZE);
    if (strcmp(grn_query_log_show_condition_env, "no") == 0) {
      grn_query_log_show_condition = GRN_FALSE;
    } else {
      grn_query_log_show_condition = GRN_TRUE;
    }
  }
}

grn_obj *
grn_expr_alloc(grn_ctx *ctx, grn_obj *expr, grn_id domain, unsigned char flags)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  if (e) {
    if (e->values_curr >= e->values_size) {
      // todo : expand values.
      ERR(GRN_NO_MEMORY_AVAILABLE, "no more e->values");
      return NULL;
    }
    res = &e->values[e->values_curr++];
    if (e->values_curr > e->values_tail) { e->values_tail = e->values_curr; }
    grn_obj_reinit(ctx, res, domain, flags);
  }
  return res;
}

grn_hash *
grn_expr_get_vars(grn_ctx *ctx, grn_obj *expr, unsigned int *nvars)
{
  grn_hash *vars = NULL;
  if (expr->header.type == GRN_PROC || expr->header.type == GRN_EXPR) {
    grn_id id = DB_OBJ(expr)->id;
    grn_expr *e = (grn_expr *)expr;
    int added = 0;
    grn_hash **vp;
    if (grn_hash_add(ctx, ctx->impl->expr_vars, &id, sizeof(grn_id), (void **)&vp, &added)) {
      if (!*vp) {
        *vp = grn_hash_create(ctx, NULL, GRN_TABLE_MAX_KEY_SIZE, sizeof(grn_obj),
                              GRN_OBJ_KEY_VAR_SIZE|GRN_OBJ_TEMPORARY|GRN_HASH_TINY);
        if (*vp) {
          uint32_t i;
          grn_obj *value;
          grn_expr_var *v;
          for (v = e->vars, i = e->nvars; i; v++, i--) {
            grn_hash_add(ctx, *vp, v->name, v->name_size, (void **)&value, &added);
            GRN_OBJ_INIT(value, v->value.header.type, 0, v->value.header.domain);
            GRN_TEXT_PUT(ctx, value, GRN_TEXT_VALUE(&v->value), GRN_TEXT_LEN(&v->value));
          }
        }
      }
      vars = *vp;
    }
  }
  *nvars = vars ? GRN_HASH_SIZE(vars) : 0;
  return vars;
}

grn_rc
grn_expr_clear_vars(grn_ctx *ctx, grn_obj *expr)
{
  if (expr->header.type == GRN_PROC || expr->header.type == GRN_EXPR) {
    grn_hash **vp;
    grn_id eid, id = DB_OBJ(expr)->id;
    if ((eid = grn_hash_get(ctx, ctx->impl->expr_vars, &id, sizeof(grn_id), (void **)&vp))) {
      if (*vp) {
        grn_obj *value;
        GRN_HASH_EACH(ctx, *vp, i, NULL, NULL, (void **)&value, {
          GRN_OBJ_FIN(ctx, value);
        });
        grn_hash_close(ctx, *vp);
      }
      grn_hash_delete_by_id(ctx, ctx->impl->expr_vars, eid, NULL);
    }
  }
  return ctx->rc;
}

grn_obj *
grn_proc_get_info(grn_ctx *ctx, grn_user_data *user_data,
                  grn_expr_var **vars, unsigned int *nvars, grn_obj **caller)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  if (caller) { *caller = pctx->caller; }
  if (pctx->proc) {
    if (vars) {
      *vars = pctx->proc->vars;
   // *vars = grn_expr_get_vars(ctx, (grn_obj *)pctx->proc, nvars);
    }
    if (nvars) { *nvars = pctx->proc->nvars; }
  } else {
    if (vars) { *vars = NULL; }
    if (nvars) { *nvars = 0; }
  }
  return (grn_obj *)pctx->proc;
}

grn_obj *
grn_proc_get_vars(grn_ctx *ctx, grn_user_data *user_data)
{
  uint32_t n;
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  if (pctx->proc) {
    return (grn_obj *)grn_expr_get_vars(ctx, (grn_obj *)pctx->proc, &n);
  } else {
    return NULL;
  }
}

grn_obj *
grn_proc_get_var(grn_ctx *ctx, grn_user_data *user_data, const char *name, unsigned int name_size)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  return pctx->proc ? grn_expr_get_var(ctx, (grn_obj *)pctx->proc, name, name_size) : NULL;
}

grn_obj *
grn_proc_get_var_by_offset(grn_ctx *ctx, grn_user_data *user_data, unsigned int offset)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  return pctx->proc ? grn_expr_get_var_by_offset(ctx, (grn_obj *)pctx->proc, offset) : NULL;
}

grn_obj *
grn_proc_get_or_add_var(grn_ctx *ctx, grn_user_data *user_data,
                        const char *name, unsigned int name_size)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  return pctx->proc ? grn_expr_get_or_add_var(ctx, (grn_obj *)pctx->proc, name, name_size) : NULL;
}

grn_obj *
grn_proc_alloc(grn_ctx *ctx, grn_user_data *user_data, grn_id domain, unsigned char flags)
{
  grn_proc_ctx *pctx = (grn_proc_ctx *)user_data;
  return pctx->caller ? grn_expr_alloc(ctx, (grn_obj *)pctx->caller, domain, flags) : NULL;
}

grn_proc_type
grn_proc_get_type(grn_ctx *ctx, grn_obj *proc)
{
  grn_proc *proc_ = (grn_proc *)proc;
  return proc_ ? proc_->type : GRN_PROC_INVALID;
}

grn_rc
grn_proc_set_selector(grn_ctx *ctx, grn_obj *proc, grn_selector_func selector)
{
  grn_proc *proc_ = (grn_proc *)proc;
  if (!grn_obj_is_function_proc(ctx, proc)) {
    return GRN_INVALID_ARGUMENT;
  }
  proc_->callbacks.function.selector = selector;
  return GRN_SUCCESS;
}

grn_rc
grn_proc_set_selector_operator(grn_ctx *ctx, grn_obj *proc, grn_operator op)
{
  grn_proc *proc_ = (grn_proc *)proc;
  if (!grn_obj_is_function_proc(ctx, proc)) {
    return GRN_INVALID_ARGUMENT;
  }
  proc_->callbacks.function.selector_op = op;
  return GRN_SUCCESS;
}

grn_operator
grn_proc_get_selector_operator(grn_ctx *ctx, grn_obj *proc)
{
  grn_proc *proc_ = (grn_proc *)proc;
  if (!grn_obj_is_function_proc(ctx, proc)) {
    return GRN_OP_NOP;
  }
  return proc_->callbacks.function.selector_op;
}

grn_rc
grn_proc_set_is_stable(grn_ctx *ctx, grn_obj *proc, grn_bool is_stable)
{
  grn_proc *proc_ = (grn_proc *)proc;
  if (!grn_obj_is_function_proc(ctx, proc)) {
    return GRN_INVALID_ARGUMENT;
  }
  proc_->callbacks.function.is_stable = is_stable;
  return GRN_SUCCESS;
}

grn_bool
grn_proc_is_stable(grn_ctx *ctx, grn_obj *proc)
{
  grn_proc *proc_ = (grn_proc *)proc;
  if (!grn_obj_is_function_proc(ctx, proc)) {
    return GRN_FALSE;
  }
  return proc_->callbacks.function.is_stable;
}

/* grn_expr */

grn_obj *
grn_ctx_pop(grn_ctx *ctx)
{
  if (ctx && ctx->impl && ctx->impl->stack_curr) {
    return ctx->impl->stack[--ctx->impl->stack_curr];
  }
  return NULL;
}

grn_rc
grn_ctx_expand_stack(grn_ctx *ctx)
{
  uint32_t stack_size = ctx->impl->stack_size * 2;
  grn_obj **stack = (grn_obj **)GRN_REALLOC(ctx->impl->stack,
                                            sizeof(grn_obj *) * stack_size);
  if (!stack) {
    return ctx->rc;
  }
  ctx->impl->stack = stack;
  ctx->impl->stack_size = stack_size;
  return GRN_SUCCESS;
}

grn_rc
grn_ctx_push(grn_ctx *ctx, grn_obj *obj)
{
  if (!ctx || !ctx->impl) {
    return GRN_INVALID_ARGUMENT;
  }
  if (ctx->impl->stack_curr >= ctx->impl->stack_size) {
    grn_rc rc = grn_ctx_expand_stack(ctx);
    if (rc != GRN_SUCCESS) {
      return rc;
    }
  }
  ctx->impl->stack[ctx->impl->stack_curr++] = obj;
  return GRN_SUCCESS;
}

grn_obj *
grn_expr_alloc_const(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  uint32_t id = e->nconsts % GRN_EXPR_CONST_BLK_SIZE;
  uint32_t blk_id = e->nconsts / GRN_EXPR_CONST_BLK_SIZE;

  if (id == 0) {
    uint32_t nblks = blk_id + 1;
    grn_obj **blks = (grn_obj **)GRN_REALLOC(e->const_blks,
                                             sizeof(grn_obj *) * nblks);
    if (!blks) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "realloc failed");
      return NULL;
    }
    e->const_blks = blks;
    blks[blk_id] = GRN_MALLOCN(grn_obj, GRN_EXPR_CONST_BLK_SIZE);
    if (!blks[blk_id]) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "malloc failed");
      return NULL;
    }
  }
  e->nconsts++;
  return &e->const_blks[blk_id][id];
}

void
grn_obj_pack(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  grn_text_benc(ctx, buf, obj->header.type);
  if (GRN_DB_OBJP(obj)) {
    grn_text_benc(ctx, buf, DB_OBJ(obj)->id);
  } else {
    // todo : support vector, query, accessor, snip..
    uint32_t vs = GRN_BULK_VSIZE(obj);
    grn_text_benc(ctx, buf, obj->header.domain);
    grn_text_benc(ctx, buf, vs);
    if (vs) { GRN_TEXT_PUT(ctx, buf, GRN_BULK_HEAD(obj), vs); }
  }
}

const uint8_t *
grn_obj_unpack(grn_ctx *ctx, const uint8_t *p, const uint8_t *pe, uint8_t type, uint8_t flags, grn_obj *obj)
{
  grn_id domain;
  uint32_t vs;
  GRN_B_DEC(domain, p);
  GRN_OBJ_INIT(obj, type, flags, domain);
  GRN_B_DEC(vs, p);
  if (pe < p + vs) {
    ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
    return p;
  }
  grn_bulk_write(ctx, obj, (const char *)p, vs);
  return p + vs;
}

typedef enum {
  GRN_EXPR_PACK_TYPE_NULL     = 0,
  GRN_EXPR_PACK_TYPE_VARIABLE = 1,
  GRN_EXPR_PACK_TYPE_OTHERS   = 2
} grn_expr_pack_type;

void
grn_expr_pack(grn_ctx *ctx, grn_obj *buf, grn_obj *expr)
{
  grn_expr_code *c;
  grn_expr_var *v;
  grn_expr *e = (grn_expr *)expr;
  uint32_t i, j;
  grn_text_benc(ctx, buf, e->nvars);
  for (i = e->nvars, v = e->vars; i; i--, v++) {
    grn_text_benc(ctx, buf, v->name_size);
    if (v->name_size) { GRN_TEXT_PUT(ctx, buf, v->name, v->name_size); }
    grn_obj_pack(ctx, buf, &v->value);
  }
  i = e->codes_curr;
  grn_text_benc(ctx, buf, i);
  for (c = e->codes; i; i--, c++) {
    grn_text_benc(ctx, buf, c->op);
    grn_text_benc(ctx, buf, c->nargs);
    if (!c->value) {
      grn_text_benc(ctx, buf, GRN_EXPR_PACK_TYPE_NULL);
    } else {
      for (j = 0, v = e->vars; j < e->nvars; j++, v++) {
        if (&v->value == c->value) {
          grn_text_benc(ctx, buf, GRN_EXPR_PACK_TYPE_VARIABLE);
          grn_text_benc(ctx, buf, j);
          break;
        }
      }
      if (j == e->nvars) {
        grn_text_benc(ctx, buf, GRN_EXPR_PACK_TYPE_OTHERS);
        grn_obj_pack(ctx, buf, c->value);
      }
    }
  }
}

const uint8_t *
grn_expr_unpack(grn_ctx *ctx, const uint8_t *p, const uint8_t *pe, grn_obj *expr)
{
  grn_obj *v;
  grn_expr_pack_type type;
  uint32_t i, n, ns;
  grn_expr_code *code;
  grn_expr *e = (grn_expr *)expr;
  GRN_B_DEC(n, p);
  for (i = 0; i < n; i++) {
    uint32_t object_type;
    GRN_B_DEC(ns, p);
    v = grn_expr_add_var(ctx, expr, ns ? (const char *)p : NULL, ns);
    p += ns;
    GRN_B_DEC(object_type, p);
    if (GRN_TYPE <= object_type && object_type <= GRN_COLUMN_INDEX) { /* error */ }
    p = grn_obj_unpack(ctx, p, pe, object_type, 0, v);
    if (pe < p) {
      ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
      return p;
    }
  }
  GRN_B_DEC(n, p);
  /* confirm e->codes_size >= n */
  e->codes_curr = n;
  for (i = 0, code = e->codes; i < n; i++, code++) {
    GRN_B_DEC(code->op, p);
    GRN_B_DEC(code->nargs, p);
    GRN_B_DEC(type, p);
    switch (type) {
    case GRN_EXPR_PACK_TYPE_NULL :
      code->value = NULL;
      break;
    case GRN_EXPR_PACK_TYPE_VARIABLE :
      {
        uint32_t offset;
        GRN_B_DEC(offset, p);
        code->value = &e->vars[i].value;
      }
      break;
    case GRN_EXPR_PACK_TYPE_OTHERS :
      {
        uint32_t object_type;
        GRN_B_DEC(object_type, p);
        if (GRN_TYPE <= object_type && object_type <= GRN_COLUMN_INDEX) {
          grn_id id;
          GRN_B_DEC(id, p);
          code->value = grn_ctx_at(ctx, id);
        } else {
          if (!(v = grn_expr_alloc_const(ctx, expr))) { return NULL; }
          p = grn_obj_unpack(ctx, p, pe, object_type, GRN_OBJ_EXPRCONST, v);
          code->value = v;
        }
      }
      break;
    }
    if (pe < p) {
      ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
      return p;
    }
  }
  return p;
}

grn_obj *
grn_expr_open(grn_ctx *ctx, grn_obj_spec *spec, const uint8_t *p, const uint8_t *pe)
{
  grn_expr *expr = NULL;
  if ((expr = GRN_MALLOCN(grn_expr, 1))) {
    int size = GRN_STACK_SIZE;
    expr->const_blks = NULL;
    expr->nconsts = 0;
    GRN_TEXT_INIT(&expr->name_buf, 0);
    GRN_TEXT_INIT(&expr->dfi, 0);
    GRN_PTR_INIT(&expr->objs, GRN_OBJ_VECTOR, GRN_ID_NIL);
    GRN_TEXT_INIT(&expr->query_log_tag_prefix, 0);
    GRN_TEXT_PUTC(ctx, &expr->query_log_tag_prefix, '\0');
    expr->parent = NULL;
    expr->vars = NULL;
    expr->nvars = 0;
    GRN_DB_OBJ_SET_TYPE(expr, GRN_EXPR);
    if ((expr->values = GRN_MALLOCN(grn_obj, size))) {
      int i;
      for (i = 0; i < size; i++) {
        GRN_OBJ_INIT(&expr->values[i], GRN_BULK, GRN_OBJ_EXPRVALUE, GRN_ID_NIL);
      }
      expr->values_curr = 0;
      expr->values_tail = 0;
      expr->values_size = size;
      if ((expr->codes = GRN_MALLOCN(grn_expr_code, size))) {
        expr->codes_curr = 0;
        expr->codes_size = size;
        expr->obj.header = spec->header;
        expr->cache.codes = NULL;
        expr->cache.codes_curr = 0;
        if (grn_expr_unpack(ctx, p, pe, (grn_obj *)expr) == pe) {
          goto exit;
        } else {
          ERR(GRN_INVALID_FORMAT, "benced image is corrupt");
        }
        GRN_FREE(expr->codes);
      }
      GRN_FREE(expr->values);
    }
    GRN_OBJ_FIN(ctx, &expr->name_buf);
    GRN_OBJ_FIN(ctx, &expr->dfi);
    GRN_OBJ_FIN(ctx, &expr->objs);
    GRN_OBJ_FIN(ctx, &expr->query_log_tag_prefix);
    GRN_FREE(expr);
    expr = NULL;
  }
exit :
  return (grn_obj *)expr;
}

static grn_inline bool
grn_expr_is_takable_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj)
{
  if (grn_enable_reference_count) {
    return true;
  }

  if (grn_obj_is_table(ctx, obj)) {
    /* efsi->object_literal */
    return grn_obj_id(ctx, obj) == GRN_ID_NIL;
  }

  if (grn_obj_is_column(ctx, obj)) {
    return false;
  }

  return true;
}

/* Pass ownership of `obj` to `expr`. */
void
grn_expr_take_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj)
{
  grn_log_reference_count("expr_take_obj: %p\n", obj);
  if (!grn_expr_is_takable_obj(ctx, expr, obj)) {
    return;
  }

  grn_expr *e = (grn_expr *)expr;
  GRN_PTR_PUT(ctx, &(e->objs), obj);
}

/* data flow info */
typedef struct {
  grn_expr_code *code;
  grn_id domain;
  unsigned char type;
} grn_expr_dfi;

static grn_expr_dfi *
grn_expr_dfi_pop(grn_expr *expr)
{
  if (GRN_BULK_VSIZE(&expr->dfi) >= sizeof(grn_expr_dfi)) {
    grn_expr_dfi *dfi;
    GRN_BULK_INCR_LEN(&expr->dfi, -((ssize_t)(sizeof(grn_expr_dfi))));
    dfi = (grn_expr_dfi *)GRN_BULK_CURR(&expr->dfi);
    expr->code0 = dfi->code;
    return dfi;
  } else {
    expr->code0 = NULL;
    return NULL;
  }
}

static void
grn_expr_dfi_put(grn_ctx *ctx, grn_expr *expr, uint8_t type, grn_id domain,
                 grn_expr_code *code)
{
  grn_expr_dfi dfi;
  dfi.type = type;
  dfi.domain = domain;
  dfi.code = code;
  if (expr->code0) {
    expr->code0->modify = code ? (code - expr->code0) : 0;
  }
  grn_bulk_write(ctx, &expr->dfi, (char *)&dfi, sizeof(grn_expr_dfi));
  expr->code0 = NULL;
}

grn_obj *
grn_expr_create(grn_ctx *ctx, const char *name, unsigned int name_size)
{
  grn_id id;
  grn_obj *db;
  grn_expr *expr = NULL;
  if (!ctx || !ctx->impl || !(db = ctx->impl->db)) {
    ERR(GRN_INVALID_ARGUMENT, "db not initialized");
    return NULL;
  }
  if (name_size) {
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "[expr][create] named expression isn't implemented yet");
    return NULL;
  }
  GRN_API_ENTER;
  if (grn_db_check_name(ctx, name, name_size)) {
    GRN_DB_CHECK_NAME_ERR("[expr][create]", name, name_size);
    GRN_API_RETURN(NULL);
  }
  if (!GRN_DB_P(db)) {
    ERR(GRN_INVALID_ARGUMENT, "named expr is not supported");
    GRN_API_RETURN(NULL);
  }
  id = grn_obj_register(ctx, db, name, name_size);
  if (id && (expr = GRN_MALLOCN(grn_expr, 1))) {
    int size = GRN_STACK_SIZE;
    expr->const_blks = NULL;
    expr->nconsts = 0;
    GRN_TEXT_INIT(&expr->name_buf, 0);
    GRN_TEXT_INIT(&expr->dfi, 0);
    GRN_PTR_INIT(&expr->objs, GRN_OBJ_VECTOR, GRN_ID_NIL);
    GRN_TEXT_INIT(&expr->query_log_tag_prefix, 0);
    GRN_TEXT_PUTC(ctx, &expr->query_log_tag_prefix, '\0');
    expr->parent = NULL;
    expr->code0 = NULL;
    expr->vars = NULL;
    expr->nvars = 0;
    expr->cacheable = 1;
    expr->taintable = 0;
    expr->values_curr = 0;
    expr->values_tail = 0;
    expr->values_size = size;
    expr->codes_curr = 0;
    expr->codes_size = size;
    expr->cache.codes = NULL;
    expr->cache.codes_curr = 0;
    GRN_DB_OBJ_SET_TYPE(expr, GRN_EXPR);
    expr->obj.header.domain = GRN_ID_NIL;
    expr->obj.range = GRN_ID_NIL;
    if (!grn_db_obj_init(ctx, db, id, DB_OBJ(expr))) {
      if ((expr->values = GRN_MALLOCN(grn_obj, size))) {
        int i;
        for (i = 0; i < size; i++) {
          GRN_OBJ_INIT(&expr->values[i], GRN_BULK, GRN_OBJ_EXPRVALUE, GRN_ID_NIL);
        }
        if ((expr->codes = GRN_MALLOCN(grn_expr_code, size))) {
          goto exit;
        }
        GRN_FREE(expr->values);
      }
    }
    GRN_OBJ_FIN(ctx, &expr->name_buf);
    GRN_OBJ_FIN(ctx, &expr->dfi);
    GRN_OBJ_FIN(ctx, &expr->objs);
    GRN_OBJ_FIN(ctx, &expr->query_log_tag_prefix);
    GRN_FREE(expr);
    expr = NULL;
  }
exit :
  GRN_API_RETURN((grn_obj *)expr);
}

grn_rc
grn_expr_close(grn_ctx *ctx, grn_obj *expr)
{
  uint32_t i, j;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  /*
  if (e->obj.header.domain) {
    grn_hash_delete(ctx, ctx->impl->qe, &e->obj.header.domain, sizeof(grn_id), NULL);
  }
  */
  GRN_OBJ_FIN(ctx, &(e->query_log_tag_prefix));
  if (e->cache.codes) {
    grn_expr_executor_fin(ctx, &(e->cache.executor));
  }
  grn_expr_clear_vars(ctx, expr);
  if (e->const_blks) {
    uint32_t nblks = e->nconsts + GRN_EXPR_CONST_BLK_SIZE - 1;
    nblks /= GRN_EXPR_CONST_BLK_SIZE;
    for (i = 0; i < nblks; i++) {
      uint32_t end;
      if (i < nblks - 1) {
        end = GRN_EXPR_CONST_BLK_SIZE;
      } else {
        end = ((e->nconsts - 1) % GRN_EXPR_CONST_BLK_SIZE) + 1;
      }
      for (j = 0; j < end; j++) {
        grn_obj *const_obj = &e->const_blks[i][j];
        grn_obj_close(ctx, const_obj);
      }
      GRN_FREE(e->const_blks[i]);
    }
    GRN_FREE(e->const_blks);
  }
  GRN_OBJ_FIN(ctx, &e->name_buf);
  GRN_OBJ_FIN(ctx, &e->dfi);
  for (;;) {
    grn_obj *obj;
    GRN_PTR_POP(&e->objs, obj);
    if (!obj) {
      break;
    }

    grn_log_reference_count("expr_close: obj: %p\n", obj);

#ifdef USE_MEMORY_DEBUG
    grn_obj_unlink(ctx, obj);
#else
    if (obj->header.type == GRN_VOID) {
      GRN_LOG(ctx, GRN_LOG_WARNING, "GRN_VOID object is tried to be unlinked");
      continue;
    }

    if (obj->header.type == GRN_TABLE_HASH_KEY &&
        grn_obj_is_temporary(ctx, obj) &&
        ((grn_hash *)obj)->value_size == sizeof(grn_obj)) {
      GRN_HASH_EACH_BEGIN(ctx, (grn_hash *)obj, cursor, id) {
        void *value;
        grn_hash_cursor_get_value(ctx, cursor, &value);
        grn_obj *v = value;
        GRN_OBJ_FIN(ctx, v);
      } GRN_HASH_EACH_END(ctx, cursor);
    }
    grn_obj_unlink(ctx, obj);
#endif
  }
  GRN_OBJ_FIN(ctx, &e->objs);
  for (i = 0; i < e->nvars; i++) {
    grn_obj_close(ctx, &e->vars[i].value);
  }
  if (e->vars) { GRN_FREE(e->vars); }
  for (i = 0; i < e->values_tail; i++) {
    grn_obj_close(ctx, &e->values[i]);
  }
  GRN_FREE(e->values);
  GRN_FREE(e->codes);
  GRN_FREE(e);
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_expr_add_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned int name_size)
{
  uint32_t i;
  char *p;
  grn_expr_var *v;
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (DB_OBJ(expr)->id & GRN_OBJ_TMP_OBJECT) {
    res = grn_expr_get_or_add_var(ctx, expr, name, name_size);
  } else {
    if (!e->vars) {
      if (!(e->vars = GRN_MALLOCN(grn_expr_var, GRN_STACK_SIZE))) {
        ERR(GRN_NO_MEMORY_AVAILABLE,
            "[expr][variable][add] failed to allocate: <%d>",
            GRN_STACK_SIZE);
      }
    }
    if (e->vars && e->nvars < GRN_STACK_SIZE) {
      v = e->vars + e->nvars++;
      if (name_size) {
        GRN_TEXT_PUT(ctx, &e->name_buf, name, name_size);
      } else {
        uint32_t ol = GRN_TEXT_LEN(&e->name_buf);
        GRN_TEXT_PUTC(ctx, &e->name_buf, '$');
        grn_text_itoa(ctx, &e->name_buf, e->nvars);
        name_size = GRN_TEXT_LEN(&e->name_buf) - ol;
      }
      v->name_size = name_size;
      res = &v->value;
      GRN_VOID_INIT(res);
      for (i = e->nvars, p = GRN_TEXT_VALUE(&e->name_buf), v = e->vars; i; i--, v++) {
        v->name = p;
        p += v->name_size;
      }
    }
  }
  GRN_API_RETURN(res);
}

grn_obj *
grn_expr_get_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned int name_size)
{
  uint32_t n;
  grn_obj *res = NULL;
  grn_hash *vars = grn_expr_get_vars(ctx, expr, &n);
  if (vars) { grn_hash_get(ctx, vars, name, name_size, (void **)&res); }
  return res;
}

grn_obj *
grn_expr_get_or_add_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned int name_size)
{
  uint32_t n;
  grn_obj *res = NULL;
  grn_hash *vars = grn_expr_get_vars(ctx, expr, &n);
  if (vars) {
    int added = 0;
    char name_buf[16];
    if (!name_size) {
      char *rest;
      name_buf[0] = '$';
      grn_itoa((int)GRN_HASH_SIZE(vars) + 1, name_buf + 1, name_buf + 16, &rest);
      name_size = rest - name_buf;
      name = name_buf;
    }
    grn_hash_add(ctx, vars, name, name_size, (void **)&res, &added);
    if (added) { GRN_TEXT_INIT(res, 0); }
  }
  return res;
}

grn_obj *
grn_expr_get_var_by_offset(grn_ctx *ctx, grn_obj *expr, unsigned int offset)
{
  uint32_t n;
  grn_obj *res = NULL;
  grn_hash *vars = grn_expr_get_vars(ctx, expr, &n);
  if (vars) { res = (grn_obj *)grn_hash_get_value_(ctx, vars, offset + 1, NULL); }
  return res;
}

#define CONSTP(obj) ((obj) && ((obj)->header.impl_flags & GRN_OBJ_EXPRCONST))

#define PUSH_CODE(e,o,v,n,c) do {\
  (c) = &(e)->codes[e->codes_curr++];\
  (c)->value = (v);\
  (c)->nargs = (n);\
  (c)->op = (o);\
  (c)->flags = 0;\
  (c)->modify = 0;\
} while (0)

#define APPEND_UNARY_MINUS_OP(e) do {                           \
  grn_expr_code *code_;                                         \
  grn_id domain;                                                \
  unsigned char type;                                           \
  grn_obj *x;                                                   \
  dfi = grn_expr_dfi_pop(e);                                    \
  code_ = dfi->code;                                            \
  domain = dfi->domain;                                         \
  type = dfi->type;                                             \
  x = code_->value;                                             \
  if (CONSTP(x)) {                                              \
    switch (domain) {                                           \
    case GRN_DB_INT32:                                          \
      {                                                         \
        int value;                                              \
        value = GRN_INT32_VALUE(x);                             \
        if (value == (int)0x80000000) {                         \
          domain = GRN_DB_INT64;                                \
          x->header.domain = domain;                            \
          GRN_INT64_SET(ctx, x, -((long long int)value));       \
        } else {                                                \
          GRN_INT32_SET(ctx, x, -value);                        \
        }                                                       \
      }                                                         \
      break;                                                    \
    case GRN_DB_UINT32:                                         \
      {                                                         \
        unsigned int value;                                     \
        value = GRN_UINT32_VALUE(x);                            \
        if (value > (unsigned int)0x80000000) {                 \
          domain = GRN_DB_INT64;                                \
          x->header.domain = domain;                            \
          GRN_INT64_SET(ctx, x, -((long long int)value));       \
        } else {                                                \
          domain = GRN_DB_INT32;                                \
          x->header.domain = domain;                            \
          GRN_INT32_SET(ctx, x, -((int)value));                 \
        }                                                       \
      }                                                         \
      break;                                                    \
    case GRN_DB_INT64:                                          \
      GRN_INT64_SET(ctx, x, -GRN_INT64_VALUE(x));               \
      break;                                                    \
    case GRN_DB_FLOAT32:                                        \
      GRN_FLOAT32_SET(ctx, x, -GRN_FLOAT32_VALUE(x));           \
      break;                                                    \
    case GRN_DB_FLOAT:                                          \
      GRN_FLOAT_SET(ctx, x, -GRN_FLOAT_VALUE(x));               \
      break;                                                    \
    default:                                                    \
      PUSH_CODE(e, op, obj, nargs, code);                       \
      break;                                                    \
    }                                                           \
  } else {                                                      \
    PUSH_CODE(e, op, obj, nargs, code);                         \
  }                                                             \
  grn_expr_dfi_put(ctx, e, type, domain, code_);                \
} while (0)

#define PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code) do { \
  PUSH_CODE(e, op, obj, nargs, code);                           \
  {                                                             \
    int i = nargs;                                              \
    while (i--) {                                               \
      dfi = grn_expr_dfi_pop(e);                                \
    }                                                           \
  }                                                             \
  grn_expr_dfi_put(ctx, e, type, domain, code);                 \
} while (0)

static void
grn_expr_append_obj_resolve_const(grn_ctx *ctx,
                                  grn_obj *obj,
                                  grn_id to_domain)
{
  grn_obj dest;

  GRN_OBJ_INIT(&dest, GRN_BULK, 0, to_domain);
  if (!grn_obj_cast(ctx, obj, &dest, GRN_FALSE)) {
    grn_obj_reinit(ctx, obj, to_domain, 0);
    grn_bulk_write(ctx, obj, GRN_BULK_HEAD(&dest), GRN_BULK_VSIZE(&dest));
  }
  GRN_OBJ_FIN(ctx, &dest);
}

grn_obj *
grn_expr_append_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj, grn_operator op, int nargs)
{
  uint8_t type = GRN_VOID;
  grn_id domain = GRN_ID_NIL;
  grn_expr_dfi *dfi;
  grn_expr_code *code;
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (e->codes_curr >= e->codes_size) {
    grn_expr_dfi *dfis = (grn_expr_dfi *)GRN_BULK_HEAD(&e->dfi);
    size_t i, n_dfis = GRN_BULK_VSIZE(&e->dfi) / sizeof(grn_expr_dfi);
    uint32_t new_codes_size = e->codes_size * 2;
    size_t n_bytes = sizeof(grn_expr_code) * new_codes_size;
    grn_expr_code *new_codes = (grn_expr_code *)GRN_MALLOC(n_bytes);
    if (!new_codes) {
      ERR(GRN_NO_MEMORY_AVAILABLE, "stack is full");
      goto exit;
    }
    grn_memcpy(new_codes, e->codes, sizeof(grn_expr_code) * e->codes_size);
    if (e->code0 >= e->codes && e->code0 < e->codes + e->codes_size) {
      e->code0 = new_codes + (e->code0 - e->codes);
    }
    for (i = 0; i < n_dfis; i++) {
      if (dfis[i].code >= e->codes && dfis[i].code < e->codes + e->codes_size) {
        dfis[i].code = new_codes + (dfis[i].code - e->codes);
      }
    }
    GRN_FREE(e->codes);
    e->codes = new_codes;
    e->codes_size = new_codes_size;
  }
  {
    switch (op) {
    case GRN_OP_PUSH :
      if (obj) {
        PUSH_CODE(e, op, obj, nargs, code);
        grn_expr_dfi_put(ctx, e, obj->header.type, GRN_OBJ_GET_DOMAIN(obj),
                         code);
      } else {
        ERR(GRN_INVALID_ARGUMENT, "obj not assigned for GRN_OP_PUSH");
        goto exit;
      }
      break;
    case GRN_OP_NOP :
      /* nop */
      break;
    case GRN_OP_POP :
      if (obj) {
        ERR(GRN_INVALID_ARGUMENT, "obj assigned for GRN_OP_POP");
        goto exit;
      } else {
        PUSH_CODE(e, op, obj, nargs, code);
        dfi = grn_expr_dfi_pop(e);
      }
      break;
    case GRN_OP_CALL :
      {
        grn_obj *proc = NULL;
        /*
         * This is for keeping backward compatibility. We want to
         * handle all "nargs" means that "N items on stack are used (N
         * items are popped)" but "nargs" for OP_CALL is used as "N
         * arguments" not "N items on stack are used" historically. It
         * means that called function isn't included in "nargs".
         *
         * We adjust "nargs" here to handle "code->nargs" more easily.
         * If we don't adjust "nargs" here, we need to care
         * "code->nargs" at all locations that use "code->nargs". We
         * need to use "code->nargs + 1" for OP_CALL and "code->nargs"
         * for not OP_CALL to compute N items should be popped. It's
         * wired. So we adjust "nargs" here.
         */
        nargs++;
        if (e->codes_curr - (nargs - 1) > 0) {
          int i;
          grn_expr_code *code;
          code = &(e->codes[e->codes_curr - 1]);
          for (i = 0; i < nargs - 1; i++) {
            int rest_n_codes = 1;
            while (rest_n_codes > 0) {
              rest_n_codes += code->nargs;
              if (code->value) {
                rest_n_codes--;
              }
              rest_n_codes--;
              code--;
            }
          }
          proc = code->value;
        }
        if (!proc) {
          ERR(GRN_INVALID_ARGUMENT, "invalid function call expression");
          goto exit;
        }
        if (!(grn_obj_is_function_proc(ctx, proc) ||
              grn_obj_is_scorer_proc(ctx, proc) ||
              grn_obj_is_window_function_proc(ctx, proc) ||
              grn_obj_is_tokenizer_proc(ctx, proc) ||
              grn_obj_is_normalizer_proc(ctx, proc) ||
              grn_obj_is_token_filter_proc(ctx, proc))) {
          grn_obj buffer;

          GRN_TEXT_INIT(&buffer, 0);
          switch (proc->header.type) {
          case GRN_TABLE_HASH_KEY:
          case GRN_TABLE_PAT_KEY:
          case GRN_TABLE_NO_KEY:
          case GRN_COLUMN_FIX_SIZE:
          case GRN_COLUMN_VAR_SIZE:
          case GRN_COLUMN_INDEX:
            grn_inspect_name(ctx, &buffer, proc);
            break;
          default:
            grn_inspect(ctx, &buffer, proc);
            break;
          }
          ERR(GRN_INVALID_ARGUMENT, "invalid function: <%.*s>",
              (int)GRN_TEXT_LEN(&buffer), GRN_TEXT_VALUE(&buffer));
          GRN_OBJ_FIN(ctx, &buffer);
          goto exit;
        }

        PUSH_CODE(e, op, obj, nargs, code);
        {
          int i = nargs - 1;
          while (i--) { dfi = grn_expr_dfi_pop(e); }
        }
        if (!obj) { dfi = grn_expr_dfi_pop(e); }
        // todo : increment e->values_tail.
        /* cannot identify type of return value */
        grn_expr_dfi_put(ctx, e, type, domain, code);
        if (!grn_proc_is_stable(ctx, proc)) {
          e->cacheable = 0;
        }
      }
      break;
    case GRN_OP_INTERN :
      if (obj && CONSTP(obj)) {
        grn_obj *value;
        value = grn_expr_get_var(ctx, expr, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj));
        if (!value) { value = grn_ctx_get(ctx, GRN_TEXT_VALUE(obj), GRN_TEXT_LEN(obj)); }
        if (value) {
          obj = value;
          op = GRN_OP_PUSH;
          type = obj->header.type;
          domain = GRN_OBJ_GET_DOMAIN(obj);
        }
      }
      PUSH_CODE(e, op, obj, nargs, code);
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_EQUAL :
      PUSH_CODE(e, op, obj, nargs, code);
      if (nargs) {
        grn_id xd, yd = GRN_ID_NIL;
        grn_obj *x, *y = NULL;
        int i = nargs - 1;
        if (obj) {
          xd = GRN_OBJ_GET_DOMAIN(obj);
          x = obj;
        } else {
          dfi = grn_expr_dfi_pop(e);
          x = dfi->code->value;
          xd = dfi->domain;
        }
        while (i--) {
          dfi = grn_expr_dfi_pop(e);
          y = dfi->code->value;
          yd = dfi->domain;
        }
        if (CONSTP(x)) {
          if (CONSTP(y)) {
            /* todo */
          } else {
            if (xd != yd &&
                grn_obj_is_bulk(ctx, x) &&
                !(grn_type_id_is_number_family(ctx, xd) &&
                  grn_type_id_is_number_family(ctx, yd))) {
              grn_expr_append_obj_resolve_const(ctx, x, yd);
            }
          }
        } else {
          if (CONSTP(y)) {
            if (xd != yd &&
                grn_obj_is_bulk(ctx, y) &&
                !(grn_type_id_is_number_family(ctx, xd) &&
                  grn_type_id_is_number_family(ctx, yd))) {
              grn_expr_append_obj_resolve_const(ctx, y, xd);
            }
          }
        }
      }
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_TABLE_CREATE :
    case GRN_OP_EXPR_GET_VAR :
    case GRN_OP_MATCH :
    case GRN_OP_NEAR :
    case GRN_OP_NEAR2 :
    case GRN_OP_NEAR_PHRASE :
    case GRN_OP_SIMILAR :
    case GRN_OP_PREFIX :
    case GRN_OP_SUFFIX :
    case GRN_OP_NOT_EQUAL :
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
    case GRN_OP_GEO_DISTANCE1 :
    case GRN_OP_GEO_DISTANCE2 :
    case GRN_OP_GEO_DISTANCE3 :
    case GRN_OP_GEO_DISTANCE4 :
    case GRN_OP_GEO_WITHINP5 :
    case GRN_OP_GEO_WITHINP6 :
    case GRN_OP_GEO_WITHINP8 :
    case GRN_OP_OBJ_SEARCH :
    case GRN_OP_TABLE_SELECT :
    case GRN_OP_TABLE_SORT :
    case GRN_OP_TABLE_GROUP :
    case GRN_OP_JSON_PUT :
    case GRN_OP_GET_REF :
    case GRN_OP_ADJUST :
    case GRN_OP_TERM_EXTRACT :
    case GRN_OP_REGEXP :
    case GRN_OP_QUORUM :
      PUSH_CODE(e, op, obj, nargs, code);
      if (nargs) {
        int i = nargs - 1;
        if (!obj) { dfi = grn_expr_dfi_pop(e); }
        while (i--) { dfi = grn_expr_dfi_pop(e); }
      }
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_AND_NOT :
      PUSH_CODE(e, op, obj, nargs, code);
      if (nargs != 2) {
        GRN_LOG(ctx, GRN_LOG_WARNING, "nargs(%d) != 2 in relative op", nargs);
      }
      if (obj) {
        GRN_LOG(ctx, GRN_LOG_WARNING, "obj assigned to relative op");
      }
      {
        int i = nargs;
        while (i--) {
          dfi = grn_expr_dfi_pop(e);
          if (dfi) {
            dfi->code->flags |= GRN_EXPR_CODE_RELATIONAL_EXPRESSION;
          } else {
            ERR(GRN_SYNTAX_ERROR, "stack under flow in relative op");
          }
        }
      }
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_NOT :
      if (nargs == 1) {
        PUSH_CODE(e, op, obj, nargs, code);
      }
      break;
    case GRN_OP_PLUS :
      if (nargs > 1) {
        PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code);
      }
      break;
    case GRN_OP_MINUS :
      if (nargs == 1) {
        APPEND_UNARY_MINUS_OP(e);
      } else {
        PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code);
      }
      break;
    case GRN_OP_BITWISE_NOT :
      dfi = grn_expr_dfi_pop(e);
      if (dfi) {
        type = dfi->type;
        domain = dfi->domain;
        switch (domain) {
        case GRN_DB_UINT8 :
          domain = GRN_DB_INT16;
          break;
        case GRN_DB_UINT16 :
          domain = GRN_DB_INT32;
          break;
        case GRN_DB_UINT32 :
        case GRN_DB_UINT64 :
          domain = GRN_DB_INT64;
          break;
        }
      }
      PUSH_CODE(e, op, obj, nargs, code);
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_STAR :
    case GRN_OP_SLASH :
    case GRN_OP_MOD :
    case GRN_OP_SHIFTL :
    case GRN_OP_SHIFTR :
    case GRN_OP_SHIFTRR :
    case GRN_OP_BITWISE_OR :
    case GRN_OP_BITWISE_XOR :
    case GRN_OP_BITWISE_AND :
      PUSH_N_ARGS_ARITHMETIC_OP(e, op, obj, nargs, code);
      break;
    case GRN_OP_INCR :
    case GRN_OP_DECR :
    case GRN_OP_INCR_POST :
    case GRN_OP_DECR_POST :
      {
        dfi = grn_expr_dfi_pop(e);
        if (dfi) {
          type = dfi->type;
          domain = dfi->domain;
          if (dfi->code) {
            if (dfi->code->op == GRN_OP_GET_VALUE) {
              dfi->code->op = GRN_OP_GET_REF;
            }
            if (dfi->code->value && grn_obj_is_persistent(ctx, dfi->code->value)) {
              e->cacheable = 0;
              e->taintable = 1;
            }
          }
        }
        PUSH_CODE(e, op, obj, nargs, code);
      }
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_GET_VALUE :
      {
        grn_id vdomain = GRN_ID_NIL;
        if (obj) {
          if (nargs == 1) {
            grn_obj *v = grn_expr_get_var_by_offset(ctx, expr, 0);
            if (v) { vdomain = GRN_OBJ_GET_DOMAIN(v); }
          } else {
            dfi = grn_expr_dfi_pop(e);
            vdomain = dfi->domain;
          }
          if (vdomain && CONSTP(obj) && obj->header.type == GRN_BULK) {
            grn_obj *table = grn_ctx_at(ctx, vdomain);
            grn_obj *col = grn_obj_column(ctx, table, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
            if (col) {
              obj = col;
              type = col->header.type;
              domain = grn_obj_get_range(ctx, col);
              grn_expr_take_obj(ctx, (grn_obj *)e, col);
            }
          } else {
            domain = grn_obj_get_range(ctx, obj);
          }
          PUSH_CODE(e, op, obj, nargs, code);
        } else {
          grn_expr_dfi *dfi0;
          dfi0 = grn_expr_dfi_pop(e);
          if (nargs == 1) {
            grn_obj *v = grn_expr_get_var_by_offset(ctx, expr, 0);
            if (v) { vdomain = GRN_OBJ_GET_DOMAIN(v); }
          } else {
            dfi = grn_expr_dfi_pop(e);
            vdomain = dfi->domain;
          }
          if (dfi0->code->op == GRN_OP_PUSH) {
            dfi0->code->op = op;
            dfi0->code->nargs = nargs;
            obj = dfi0->code->value;
            if (vdomain && obj && CONSTP(obj) && obj->header.type == GRN_BULK) {
              grn_obj *table = grn_ctx_at(ctx, vdomain);
              grn_obj *col = grn_obj_column(ctx, table, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
              if (col) {
                dfi0->code->value = col;
                type = col->header.type;
                domain = grn_obj_get_range(ctx, col);
                grn_expr_take_obj(ctx, expr, col);
              }
            } else {
              domain = grn_obj_get_range(ctx, obj);
            }
            code = dfi0->code;
          } else {
            PUSH_CODE(e, op, obj, nargs, code);
          }
        }
      }
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_ASSIGN :
    case GRN_OP_STAR_ASSIGN :
    case GRN_OP_SLASH_ASSIGN :
    case GRN_OP_MOD_ASSIGN :
    case GRN_OP_PLUS_ASSIGN :
    case GRN_OP_MINUS_ASSIGN :
    case GRN_OP_SHIFTL_ASSIGN :
    case GRN_OP_SHIFTR_ASSIGN :
    case GRN_OP_SHIFTRR_ASSIGN :
    case GRN_OP_AND_ASSIGN :
    case GRN_OP_OR_ASSIGN :
    case GRN_OP_XOR_ASSIGN :
      {
        if (obj) {
          type = obj->header.type;
          domain = GRN_OBJ_GET_DOMAIN(obj);
        } else {
          dfi = grn_expr_dfi_pop(e);
          if (dfi) {
            type = dfi->type;
            domain = dfi->domain;
          }
        }
        dfi = grn_expr_dfi_pop(e);
        if (dfi && (dfi->code)) {
          if (dfi->code->op == GRN_OP_GET_VALUE) {
            dfi->code->op = GRN_OP_GET_REF;
          }
          if (dfi->code->value && grn_obj_is_persistent(ctx, dfi->code->value)) {
            e->cacheable = 0;
            e->taintable = 1;
          }
        }
        PUSH_CODE(e, op, obj, nargs, code);
      }
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    case GRN_OP_JUMP :
      dfi = grn_expr_dfi_pop(e);
      PUSH_CODE(e, op, obj, nargs, code);
      break;
    case GRN_OP_CJUMP :
      dfi = grn_expr_dfi_pop(e);
      PUSH_CODE(e, op, obj, nargs, code);
      break;
    case GRN_OP_COMMA :
      PUSH_CODE(e, op, obj, nargs, code);
      break;
    case GRN_OP_GET_MEMBER :
      dfi = grn_expr_dfi_pop(e);
      dfi = grn_expr_dfi_pop(e);
      if (dfi) {
        type = dfi->type;
        domain = dfi->domain;
        if (dfi->code) {
          if (dfi->code->op == GRN_OP_GET_VALUE) {
            dfi->code->op = GRN_OP_GET_REF;
          }
        }
      }
      PUSH_CODE(e, op, obj, nargs, code);
      grn_expr_dfi_put(ctx, e, type, domain, code);
      break;
    default :
      break;
    }
  }
exit :
  if (!ctx->rc) { res = obj; }
  GRN_API_RETURN(res);
}
#undef PUSH_N_ARGS_ARITHMETIC_OP
#undef APPEND_UNARY_MINUS_OP

grn_obj *
grn_expr_append_const(grn_ctx *ctx, grn_obj *expr, grn_obj *obj,
                      grn_operator op, int nargs)
{
  grn_obj *res = NULL;
  GRN_API_ENTER;
  if (!obj) {
    ERR(GRN_SYNTAX_ERROR, "constant is null");
    goto exit;
  }
  if (GRN_DB_OBJP(obj) || GRN_ACCESSORP(obj)) {
    res = obj;
  } else {
    if ((res = grn_expr_alloc_const(ctx, expr))) {
      switch (obj->header.type) {
      case GRN_VOID :
      case GRN_BULK :
      case GRN_UVECTOR :
        GRN_OBJ_INIT(res, obj->header.type, 0, obj->header.domain);
        grn_bulk_write(ctx, res, GRN_BULK_HEAD(obj), GRN_BULK_VSIZE(obj));
        break;
      case GRN_VECTOR :
        GRN_OBJ_INIT(res, obj->header.type, 0, obj->header.domain);
        unsigned int n = grn_vector_size(ctx, obj);
        for (unsigned int i = 0; i < n; i++) {
          const char *content;
          unsigned int content_length;
          uint32_t weight;
          grn_id domain;
          content_length = grn_vector_get_element(ctx,
                                                  obj,
                                                  i,
                                                  &content,
                                                  &weight,
                                                  &domain);
          grn_vector_add_element(ctx,
                                 res,
                                 content,
                                 content_length,
                                 weight,
                                 domain);
        }
        break;
      default :
        res = NULL;
        ERR(GRN_FUNCTION_NOT_IMPLEMENTED, "unsupported type");
        goto exit;
      }
      res->header.impl_flags |= GRN_OBJ_EXPRCONST;
    }
  }
  grn_expr_append_obj(ctx, expr, res, op, nargs); /* constant */
exit :
  GRN_API_RETURN(res);
}

static grn_obj *
grn_expr_add_str(grn_ctx *ctx, grn_obj *expr, const char *str, unsigned int str_size)
{
  grn_obj *res = NULL;
  if ((res = grn_expr_alloc_const(ctx, expr))) {
    GRN_TEXT_INIT(res, 0);
    grn_bulk_write(ctx, res, str, str_size);
    res->header.impl_flags |= GRN_OBJ_EXPRCONST;
  }
  return res;
}

grn_obj *
grn_expr_append_const_str(grn_ctx *ctx, grn_obj *expr, const char *str, unsigned int str_size,
                          grn_operator op, int nargs)
{
  grn_obj *res;
  GRN_API_ENTER;
  res = grn_expr_add_str(ctx, expr, str, str_size);
  grn_expr_append_obj(ctx, expr, res, op, nargs); /* constant */
  GRN_API_RETURN(res);
}

grn_obj *
grn_expr_append_const_int(grn_ctx *ctx, grn_obj *expr, int i,
                          grn_operator op, int nargs)
{
  grn_obj *res = NULL;
  GRN_API_ENTER;
  if ((res = grn_expr_alloc_const(ctx, expr))) {
    GRN_INT32_INIT(res, 0);
    GRN_INT32_SET(ctx, res, i);
    res->header.impl_flags |= GRN_OBJ_EXPRCONST;
  }
  grn_expr_append_obj(ctx, expr, res, op, nargs); /* constant */
  GRN_API_RETURN(res);
}

grn_obj *
grn_expr_append_const_bool(grn_ctx *ctx, grn_obj *expr, grn_bool value,
                           grn_operator op, int nargs)
{
  grn_obj *res = NULL;
  GRN_API_ENTER;
  if ((res = grn_expr_alloc_const(ctx, expr))) {
    GRN_BOOL_INIT(res, 0);
    GRN_BOOL_SET(ctx, res, value);
    res->header.impl_flags |= GRN_OBJ_EXPRCONST;
  }
  grn_expr_append_obj(ctx, expr, res, op, nargs); /* constant */
  GRN_API_RETURN(res);
}

grn_rc
grn_expr_append_op(grn_ctx *ctx, grn_obj *expr, grn_operator op, int nargs)
{
  grn_expr_append_obj(ctx, expr, NULL, op, nargs);
  return ctx->rc;
}

grn_rc
grn_expr_compile(grn_ctx *ctx, grn_obj *expr)
{
  grn_obj_spec_save(ctx, DB_OBJ(expr));
  return ctx->rc;
}

grn_obj *
grn_expr_rewrite(grn_ctx *ctx, grn_obj *expr)
{
  grn_obj *rewritten = NULL;

  GRN_API_ENTER;

#ifdef GRN_WITH_MRUBY
  grn_ctx_impl_mrb_ensure_init(ctx);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_API_RETURN(NULL);
  }
  if (ctx->impl->mrb.state) {
    rewritten = grn_mrb_expr_rewrite(ctx, expr);
  }
#endif

  GRN_API_RETURN(rewritten);
}

grn_rc
grn_proc_call(grn_ctx *ctx, grn_obj *proc, int nargs, grn_obj *caller)
{
  grn_proc_ctx pctx;
  grn_obj *obj = NULL, **args;
  grn_proc *p = (grn_proc *)proc;
  if (nargs > ctx->impl->stack_curr) { return GRN_INVALID_ARGUMENT; }
  GRN_API_ENTER;
  if (grn_obj_is_selector_only_proc(ctx, proc)) {
    char name[GRN_TABLE_MAX_KEY_SIZE];
    int name_size;
    name_size = grn_obj_name(ctx, proc, name, GRN_TABLE_MAX_KEY_SIZE);
    ERR(GRN_FUNCTION_NOT_IMPLEMENTED,
        "selector only proc can't be called: <%.*s>",
        name_size, name);
    GRN_API_RETURN(ctx->rc);
  }
  args = ctx->impl->stack + ctx->impl->stack_curr - nargs;
  pctx.proc = p;
  pctx.caller = caller;
  pctx.user_data.ptr = NULL;
  if (p->funcs[PROC_INIT]) {
    grn_obj *sub_obj;
    sub_obj = p->funcs[PROC_INIT](ctx, nargs, args, &pctx.user_data);
    if (sub_obj) {
      obj = sub_obj;
    }
  }
  pctx.phase = PROC_NEXT;
  if (p->funcs[PROC_NEXT]) {
    grn_obj *sub_obj;
    sub_obj = p->funcs[PROC_NEXT](ctx, nargs, args, &pctx.user_data);
    if (sub_obj) {
      obj = sub_obj;
    }
  }
  pctx.phase = PROC_FIN;
  if (p->funcs[PROC_FIN]) {
    grn_obj *sub_obj;
    sub_obj = p->funcs[PROC_FIN](ctx, nargs, args, &pctx.user_data);
    if (sub_obj) {
      obj = sub_obj;
    }
  }
  ctx->impl->stack_curr -= nargs;
  grn_ctx_push(ctx, obj);
  GRN_API_RETURN(ctx->rc);
}

grn_obj *
grn_expr_exec(grn_ctx *ctx, grn_obj *expr, int nargs)
{
  grn_obj *value = NULL;
  GRN_API_ENTER;
  if (expr->header.type == GRN_PROC) {
    grn_proc *proc = (grn_proc *)expr;
    if (proc->type == GRN_PROC_COMMAND) {
      grn_command_input *input;
      input = grn_command_input_open(ctx, expr);
      grn_command_run(ctx, expr, input);
      grn_command_input_close(ctx, input);
      value = NULL;
    } else {
      uint32_t stack_curr = ctx->impl->stack_curr;
      grn_proc_call(ctx, expr, nargs, expr);
      if (ctx->impl->stack_curr + nargs > stack_curr) {
        value = grn_ctx_pop(ctx);
      }
      if (ctx->impl->stack_curr + nargs > stack_curr) {
        /*
          GRN_LOG(ctx, GRN_LOG_WARNING, "nargs=%d stack balance=%d",
          nargs, stack_curr - ctx->impl->stack_curr);
        */
        ctx->impl->stack_curr = stack_curr - nargs;
      }
    }
  } else {
    grn_expr *e = (grn_expr *)expr;
    grn_expr_executor *executor = &(e->cache.executor);
    grn_id id = GRN_ID_NIL;

    if (e->codes != e->cache.codes ||
        e->codes_curr != e->cache.codes_curr) {
      if (e->cache.codes) {
        grn_expr_executor_fin(ctx, executor);
      }
      grn_expr_executor_init(ctx, executor, expr);
      if (ctx->rc != GRN_SUCCESS) {
        e->cache.codes = NULL;
        e->cache.codes_curr = 0;
        goto exit;
      }
      e->cache.codes = e->codes;
      e->cache.codes_curr = e->codes_curr;
    }

    if (executor->variable) {
      id = GRN_RECORD_VALUE(executor->variable);
    }
    value = grn_expr_executor_exec(ctx, executor, id);
  }
exit :
  GRN_API_RETURN(value);
}

grn_obj *
grn_expr_get_value(grn_ctx *ctx, grn_obj *expr, int offset)
{
  grn_obj *res = NULL;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_ENTER;
  if (0 <= offset && offset < e->values_size) {
    res = &e->values[offset];
  }
  GRN_API_RETURN(res);
}

#define DEFAULT_WEIGHT 5
#define DEFAULT_DECAYSTEP 2
#define DEFAULT_MAX_INTERVAL 10
#define DEFAULT_SIMILARITY_THRESHOLD 0
#define DEFAULT_QUORUM_THRESHOLD 1
#define DEFAULT_TERM_EXTRACT_POLICY 0
#define DEFAULT_WEIGHT_VECTOR_SIZE 4096

#define GRN_SCAN_INFO_INITIAL_MAX_N_ARGS 16

struct _grn_scan_info {
  uint32_t start;
  uint32_t end;
  int32_t nargs;
  int flags;
  grn_operator op;
  grn_operator logical_op;
  grn_obj wv;
  grn_obj index;
  grn_obj *query;
  grn_obj **args;
  grn_obj *initial_args[GRN_SCAN_INFO_INITIAL_MAX_N_ARGS];
  int max_interval;
  int similarity_threshold;
  int quorum_threshold;
  grn_obj scorers;
  grn_obj scorer_args_exprs;
  grn_obj scorer_args_expr_offsets;
  struct {
    grn_bool specified;
    int start;
  } position;
  int32_t max_nargs;
};

static grn_inline void
grn_scan_info_free(grn_ctx *ctx,
                   scan_info *si)
{
  GRN_OBJ_FIN(ctx, &(si->wv));
  if (grn_enable_reference_count) {
    size_t i;
    size_t n_indexes = GRN_PTR_VECTOR_SIZE(&(si->index));
    for (i = 0; i < n_indexes; i++) {
      grn_obj_unlink(ctx, GRN_PTR_VALUE_AT(&(si->index), i));
    }
  }
  GRN_OBJ_FIN(ctx, &(si->index));
  GRN_OBJ_FIN(ctx, &(si->scorers));
  GRN_OBJ_FIN(ctx, &(si->scorer_args_exprs));
  GRN_OBJ_FIN(ctx, &(si->scorer_args_expr_offsets));
  if (si->args != si->initial_args) {
    GRN_FREE(si->args);
  }
  GRN_FREE(si);
}

#define SI_FREE(si) grn_scan_info_free((ctx), (si))

#define SI_ALLOC_RAW(si, st) do {\
  if (((si) = GRN_MALLOCN(scan_info, 1))) {\
    GRN_INT32_INIT(&(si)->wv, GRN_OBJ_VECTOR);\
    GRN_PTR_INIT(&(si)->index, GRN_OBJ_VECTOR, GRN_ID_NIL);\
    (si)->op = GRN_OP_NOP;\
    (si)->logical_op = GRN_OP_OR;\
    (si)->flags = SCAN_PUSH;\
    (si)->nargs = 0;\
    (si)->max_nargs = GRN_SCAN_INFO_INITIAL_MAX_N_ARGS;\
    (si)->args = (si)->initial_args;\
    (si)->max_interval = DEFAULT_MAX_INTERVAL;\
    (si)->similarity_threshold = DEFAULT_SIMILARITY_THRESHOLD;\
    (si)->quorum_threshold = DEFAULT_QUORUM_THRESHOLD;\
    (si)->start = (st);\
    (si)->end = (st);\
    (si)->query = NULL;\
    GRN_PTR_INIT(&(si)->scorers, GRN_OBJ_VECTOR, GRN_ID_NIL);\
    GRN_PTR_INIT(&(si)->scorer_args_exprs, GRN_OBJ_VECTOR, GRN_ID_NIL);\
    GRN_UINT32_INIT(&(si)->scorer_args_expr_offsets, GRN_OBJ_VECTOR);\
    (si)->position.specified = GRN_FALSE;\
    (si)->position.start = 0;\
  }\
} while (0)

#define SI_ALLOC(si, i, st) do {\
  SI_ALLOC_RAW(si, st);\
  if (!(si)) {\
    int j;\
    for (j = 0; j < i; j++) { SI_FREE(sis[j]); }\
    GRN_FREE(sis);\
    return NULL;\
  }\
} while (0)

static scan_info **
put_logical_op(grn_ctx *ctx, scan_info **sis, int *ip, grn_operator op, int start)
{
  int nparens = 1, ndifops = 0, i = *ip, j = i, r = 0;
  while (j--) {
    scan_info *s_ = sis[j];
    if (s_->flags & SCAN_POP) {
      ndifops++;
      nparens++;
    } else {
      if (s_->flags & SCAN_PUSH) {
        if (!(--nparens)) {
          if (!r) {
            if (ndifops) {
              if (j && op != GRN_OP_AND_NOT) {
                nparens = 1;
                ndifops = 0;
                r = j;
              } else {
                SI_ALLOC(s_, i, start);
                s_->flags = SCAN_POP;
                s_->logical_op = op;
                sis[i++] = s_;
                *ip = i;
                break;
              }
            } else {
              s_->flags &= ~SCAN_PUSH;
              s_->logical_op = op;
              break;
            }
          } else {
            if (ndifops) {
              SI_ALLOC(s_, i, start);
              s_->flags = SCAN_POP;
              s_->logical_op = op;
              sis[i++] = s_;
              *ip = i;
            } else {
              s_->flags &= ~SCAN_PUSH;
              s_->logical_op = op;
              grn_memcpy(&sis[i], &sis[j], sizeof(scan_info *) * (r - j));
              grn_memmove(&sis[j], &sis[r], sizeof(scan_info *) * (i - r));
              grn_memcpy(&sis[i + j - r], &sis[i], sizeof(scan_info *) * (r - j));
            }
            break;
          }
        }
      } else {
        if ((op == GRN_OP_AND_NOT) || (op != s_->logical_op)) {
          ndifops++;
        }
      }
    }
  }
  if (j < 0) {
    ERR(GRN_INVALID_ARGUMENT, "unmatched nesting level");
    for (j = 0; j < i; j++) { SI_FREE(sis[j]); }
    GRN_FREE(sis);
    return NULL;
  }
  return sis;
}

/* TODO: Remove me if nobody doesn't want to reuse the implementation again. */
#if 0
static const char *opstrs[] = {
  "PUSH",
  "POP",
  "NOP",
  "CALL",
  "INTERN",
  "GET_REF",
  "GET_VALUE",
  "AND",
  "AND_NOT",
  "OR",
  "ASSIGN",
  "STAR_ASSIGN",
  "SLASH_ASSIGN",
  "MOD_ASSIGN",
  "PLUS_ASSIGN",
  "MINUS_ASSIGN",
  "SHIFTL_ASSIGN",
  "SHIFTR_ASSIGN",
  "SHIFTRR_ASSIGN",
  "AND_ASSIGN",
  "XOR_ASSIGN",
  "OR_ASSIGN",
  "JUMP",
  "CJUMP",
  "COMMA",
  "BITWISE_OR",
  "BITWISE_XOR",
  "BITWISE_AND",
  "BITWISE_NOT",
  "EQUAL",
  "NOT_EQUAL",
  "LESS",
  "GREATER",
  "LESS_EQUAL",
  "GREATER_EQUAL",
  "IN",
  "MATCH",
  "NEAR",
  "NEAR2",
  "SIMILAR",
  "TERM_EXTRACT",
  "SHIFTL",
  "SHIFTR",
  "SHIFTRR",
  "PLUS",
  "MINUS",
  "STAR",
  "SLASH",
  "MOD",
  "DELETE",
  "INCR",
  "DECR",
  "INCR_POST",
  "DECR_POST",
  "NOT",
  "ADJUST",
  "EXACT",
  "LCP",
  "PARTIAL",
  "UNSPLIT",
  "PREFIX",
  "SUFFIX",
  "GEO_DISTANCE1",
  "GEO_DISTANCE2",
  "GEO_DISTANCE3",
  "GEO_DISTANCE4",
  "GEO_WITHINP5",
  "GEO_WITHINP6",
  "GEO_WITHINP8",
  "OBJ_SEARCH",
  "EXPR_GET_VAR",
  "TABLE_CREATE",
  "TABLE_SELECT",
  "TABLE_SORT",
  "TABLE_GROUP",
  "JSON_PUT",
  "GET_MEMBER",
  "REGEXP",
  "FUZZY",
  "QUORUM",
  "NEAR_PHRASE",
};

static void
put_value(grn_ctx *ctx, grn_obj *buf, grn_obj *obj)
{
  int len;
  char namebuf[GRN_TABLE_MAX_KEY_SIZE];
  if ((len = grn_column_name(ctx, obj, namebuf, GRN_TABLE_MAX_KEY_SIZE))) {
    GRN_TEXT_PUT(ctx, buf, namebuf, len);
  } else {
    grn_text_otoj(ctx, buf, obj, NULL);
  }
}

static grn_rc
grn_expr_inspect_internal(grn_ctx *ctx, grn_obj *buf, grn_obj *expr)
{
  uint32_t i, j;
  grn_expr_var *var;
  grn_expr_code *code;
  grn_expr *e = (grn_expr *)expr;
  grn_hash *vars = grn_expr_get_vars(ctx, expr, &i);
  GRN_TEXT_PUTS(ctx, buf, "noname");
  GRN_TEXT_PUTC(ctx, buf, '(');
  {
    int i = 0;
    grn_obj *value;
    const char *name;
    uint32_t name_len;
    GRN_HASH_EACH(ctx, vars, id, &name, &name_len, &value, {
      if (i++) { GRN_TEXT_PUTC(ctx, buf, ','); }
      GRN_TEXT_PUT(ctx, buf, name, name_len);
      GRN_TEXT_PUTC(ctx, buf, ':');
      put_value(ctx, buf, value);
    });
  }
  GRN_TEXT_PUTC(ctx, buf, ')');
  GRN_TEXT_PUTC(ctx, buf, '{');
  for (j = 0, code = e->codes; j < e->codes_curr; j++, code++) {
    if (j) { GRN_TEXT_PUTC(ctx, buf, ','); }
    grn_text_itoa(ctx, buf, code->modify);
    if (code->op == GRN_OP_PUSH) {
      for (i = 0, var = e->vars; i < e->nvars; i++, var++) {
        if (&var->value == code->value) {
          GRN_TEXT_PUTC(ctx, buf, '?');
          if (var->name_size) {
            GRN_TEXT_PUT(ctx, buf, var->name, var->name_size);
          } else {
            grn_text_itoa(ctx, buf, (int)i);
          }
          break;
        }
      }
      if (i == e->nvars) {
        put_value(ctx, buf, code->value);
      }
    } else {
      if (code->value) {
        put_value(ctx, buf, code->value);
        GRN_TEXT_PUTC(ctx, buf, ' ');
      }
      GRN_TEXT_PUTS(ctx, buf, opstrs[code->op]);
    }
  }
  GRN_TEXT_PUTC(ctx, buf, '}');
  return GRN_SUCCESS;
}

#define EXPRLOG(name,expr) do {\
  grn_obj strbuf;\
  GRN_TEXT_INIT(&strbuf, 0);\
  grn_expr_inspect_internal(ctx, &strbuf, (expr));\
  GRN_TEXT_PUTC(ctx, &strbuf, '\0');\
  GRN_LOG(ctx, GRN_LOG_NOTICE, "%s=(%s)", (name), GRN_TEXT_VALUE(&strbuf));\
  GRN_OBJ_FIN(ctx, &strbuf);\
} while (0)
#endif


static void
scan_info_put_index(grn_ctx *ctx, scan_info *si,
                    grn_obj *index, uint32_t sid, int32_t weight,
                    grn_obj *scorer,
                    grn_obj *scorer_args_expr,
                    uint32_t scorer_args_expr_offset)
{
  GRN_PTR_PUT(ctx, &si->index, index);
  GRN_UINT32_PUT(ctx, &si->wv, sid);
  GRN_INT32_PUT(ctx, &si->wv, weight);
  GRN_PTR_PUT(ctx, &si->scorers, scorer);
  GRN_PTR_PUT(ctx, &si->scorer_args_exprs, scorer_args_expr);
  GRN_UINT32_PUT(ctx, &si->scorer_args_expr_offsets, scorer_args_expr_offset);
  {
    int i, ni = (GRN_BULK_VSIZE(&si->index) / sizeof(grn_obj *)) - 1;
    grn_obj **pi = &GRN_PTR_VALUE_AT(&si->index, ni);
    for (i = 0; i < ni; i++, pi--) {
      if (index == pi[-1]) {
        if (i) {
          int32_t *pw = &GRN_INT32_VALUE_AT(&si->wv, (ni - i) * 2);
          grn_memmove(pw + 2, pw, sizeof(int32_t) * 2 * i);
          pw[0] = (int32_t) sid;
          pw[1] = weight;
          grn_memmove(pi + 1, pi, sizeof(grn_obj *) * i);
          pi[0] = index;
        }
        return;
      }
    }
  }
}

static int32_t
get_weight(grn_ctx *ctx, grn_expr_code *ec, uint32_t *offset)
{
  if (ec->modify == 2 && ec[2].op == GRN_OP_STAR &&
      ec[1].value && ec[1].value->header.type == GRN_BULK) {
    if (offset) {
      *offset = 2;
    }
    if (ec[1].value->header.domain == GRN_DB_INT32 ||
        ec[1].value->header.domain == GRN_DB_UINT32) {
      return GRN_INT32_VALUE(ec[1].value);
    } else {
      int32_t weight = 1;
      grn_obj weight_buffer;
      GRN_INT32_INIT(&weight_buffer, 0);
      if (!grn_obj_cast(ctx, ec[1].value, &weight_buffer, GRN_FALSE)) {
        weight = GRN_INT32_VALUE(&weight_buffer);
      }
      grn_obj_unlink(ctx, &weight_buffer);
      return weight;
    }
  } else {
    if (offset) {
      *offset = 0;
    }
    return 1;
  }
}

scan_info *
grn_scan_info_open(grn_ctx *ctx, int start)
{
  scan_info *si = GRN_MALLOCN(scan_info, 1);

  if (!si) {
    return NULL;
  }

  GRN_INT32_INIT(&si->wv, GRN_OBJ_VECTOR);
  GRN_PTR_INIT(&si->index, GRN_OBJ_VECTOR, GRN_ID_NIL);
  si->query = NULL;
  si->logical_op = GRN_OP_OR;
  si->flags = SCAN_PUSH;
  si->nargs = 0;
  si->max_nargs = GRN_SCAN_INFO_INITIAL_MAX_N_ARGS;
  si->args = si->initial_args;
  si->max_interval = DEFAULT_MAX_INTERVAL;
  si->similarity_threshold = DEFAULT_SIMILARITY_THRESHOLD;
  si->quorum_threshold = DEFAULT_QUORUM_THRESHOLD;
  si->start = start;
  GRN_PTR_INIT(&si->scorers, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_PTR_INIT(&si->scorer_args_exprs, GRN_OBJ_VECTOR, GRN_ID_NIL);
  GRN_UINT32_INIT(&si->scorer_args_expr_offsets, GRN_OBJ_VECTOR);
  si->position.specified = GRN_FALSE;
  si->position.start = 0;

  return si;
}

void
grn_scan_info_close(grn_ctx *ctx, scan_info *si)
{
  SI_FREE(si);
}

void
grn_scan_info_put_index(grn_ctx *ctx, scan_info *si,
                        grn_obj *index, uint32_t sid, int32_t weight,
                        grn_obj *scorer,
                        grn_obj *scorer_args_expr,
                        uint32_t scorer_args_expr_offset)
{
  scan_info_put_index(ctx, si, index, sid, weight,
                      scorer,
                      scorer_args_expr,
                      scorer_args_expr_offset);
}

scan_info **
grn_scan_info_put_logical_op(grn_ctx *ctx, scan_info **sis, int *ip,
                             grn_operator op, int start)
{
  return put_logical_op(ctx, sis, ip, op, start);
}

int32_t
grn_expr_code_get_weight(grn_ctx *ctx, grn_expr_code *ec, uint32_t *offset)
{
  return get_weight(ctx, ec, offset);
}

int
grn_scan_info_get_flags(scan_info *si)
{
  return si->flags;
}

void
grn_scan_info_set_flags(scan_info *si, int flags)
{
  si->flags = flags;
}

grn_operator
grn_scan_info_get_logical_op(scan_info *si)
{
  return si->logical_op;
}

void
grn_scan_info_set_logical_op(scan_info *si, grn_operator logical_op)
{
  si->logical_op = logical_op;
}

grn_operator
grn_scan_info_get_op(scan_info *si)
{
  return si->op;
}

void
grn_scan_info_set_op(scan_info *si, grn_operator op)
{
  si->op = op;
}

void
grn_scan_info_set_end(scan_info *si, uint32_t end)
{
  si->end = end;
}

void
grn_scan_info_set_query(scan_info *si, grn_obj *query)
{
  si->query = query;
}

int
grn_scan_info_get_max_interval(scan_info *si)
{
  return si->max_interval;
}

void
grn_scan_info_set_max_interval(scan_info *si, int max_interval)
{
  si->max_interval = max_interval;
}

int
grn_scan_info_get_similarity_threshold(scan_info *si)
{
  return si->similarity_threshold;
}

void
grn_scan_info_set_similarity_threshold(scan_info *si, int similarity_threshold)
{
  si->similarity_threshold = similarity_threshold;
}

int
grn_scan_info_get_quorum_threshold(scan_info *si)
{
  return si->quorum_threshold;
}

void
grn_scan_info_set_quorum_threshold(scan_info *si, int quorum_threshold)
{
  si->quorum_threshold = quorum_threshold;
}

grn_bool
grn_scan_info_push_arg(grn_ctx *ctx, scan_info *si, grn_obj *arg)
{
  if (si->nargs >= si->max_nargs) {
    grn_obj **args;
    int32_t max_nargs = si->max_nargs * 2;
    if (si->args == si->initial_args) {
      args = GRN_MALLOCN(grn_obj *, max_nargs);
      if (args == NULL) {
        return GRN_FALSE;
      }
      grn_memcpy(args, si->args, sizeof(grn_obj *) * si->nargs);
    } else {
      args = (grn_obj **)GRN_REALLOC(si->args, sizeof(grn_obj *) * max_nargs);
      if (args == NULL) {
        return GRN_FALSE;
      }
    }
    si->args = args;
    si->max_nargs = max_nargs;
  }

  si->args[si->nargs++] = arg;
  return GRN_TRUE;
}

grn_obj *
grn_scan_info_get_arg(grn_ctx *ctx, scan_info *si, int i)
{
  if (i >= si->nargs) {
    return NULL;
  }
  return si->args[i];
}

int
grn_scan_info_get_start_position(scan_info *si)
{
  return si->position.start;
}

void
grn_scan_info_set_start_position(scan_info *si, int start)
{
  si->position.specified = GRN_TRUE;
  si->position.start = start;
}

void
grn_scan_info_reset_position(scan_info *si)
{
  si->position.specified = GRN_FALSE;
}

static uint32_t
scan_info_build_match_expr_codes_find_index(grn_ctx *ctx, scan_info *si,
                                            grn_expr *expr, uint32_t i,
                                            grn_obj **index,
                                            int *sid)
{
  grn_expr_code *ec;
  uint32_t offset = 1;
  grn_index_datum index_datum;
  unsigned int n_index_data = 0;

  ec = &(expr->codes[i]);
  switch (ec->value->header.type) {
  case GRN_ACCESSOR :
    n_index_data = grn_column_find_index_data(ctx, ec->value, si->op,
                                              &index_datum, 1);
    if (n_index_data > 0) {
      grn_accessor *a = (grn_accessor *)(ec->value);
      *sid = index_datum.section;
      if (a->next && a->obj != index_datum.index) {
        *index = ec->value;
      } else {
        *index = index_datum.index;
      }
    }
    break;
  case GRN_COLUMN_FIX_SIZE :
  case GRN_COLUMN_VAR_SIZE :
    n_index_data = grn_column_find_index_data(ctx, ec->value, si->op,
                                              &index_datum, 1);
    if (n_index_data > 0) {
      *index = index_datum.index;
      *sid = index_datum.section;
    }
    break;
  case GRN_COLUMN_INDEX :
    {
      uint32_t n_rest_codes;

      *index = ec->value;
      if (grn_enable_reference_count) {
        *index = grn_ctx_at(ctx, grn_obj_id(ctx, *index));
      }

      n_rest_codes = expr->codes_curr - i;
      if (n_rest_codes >= 2 &&
          ec[1].value &&
          (ec[1].value->header.domain == GRN_DB_INT32 ||
           ec[1].value->header.domain == GRN_DB_UINT32) &&
          ec[2].op == GRN_OP_GET_MEMBER) {
        if (ec[1].value->header.domain == GRN_DB_INT32) {
          *sid = GRN_INT32_VALUE(ec[1].value) + 1;
        } else {
          *sid = GRN_UINT32_VALUE(ec[1].value) + 1;
        }
        offset += 2;
      }
    }
    break;
  default :
    break;
  }

  return offset;
}

static uint32_t
scan_info_build_match_expr_codes(grn_ctx *ctx,
                                 scan_info *si,
                                 grn_expr *expr,
                                 uint32_t i,
                                 int32_t weight)
{
  grn_expr_code *ec;
  grn_obj *index = NULL;
  int sid = 0;
  uint32_t offset = 0;

  ec = &(expr->codes[i]);
  if (!ec->value) {
    return i + 1;
  }

  switch (ec->value->header.type) {
  case GRN_ACCESSOR :
  case GRN_COLUMN_FIX_SIZE :
  case GRN_COLUMN_VAR_SIZE :
  case GRN_COLUMN_INDEX :
    offset = scan_info_build_match_expr_codes_find_index(ctx, si, expr, i,
                                                         &index, &sid);
    i += offset - 1;
    if (index) {
      if (ec->value->header.type == GRN_ACCESSOR) {
        si->flags |= SCAN_ACCESSOR;
      }
      scan_info_put_index(ctx, si, index, sid,
                          get_weight(ctx, &(expr->codes[i]), &offset) + weight,
                          NULL, NULL, 0);
      i += offset;
    }
    break;
  case GRN_PROC :
    if (!grn_obj_is_scorer_proc(ctx, ec->value)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, ec->value);
      ERR(GRN_INVALID_ARGUMENT,
          "procedure must be scorer: <%.*s>",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      return expr->codes_curr;
    }
    i++;
    offset = scan_info_build_match_expr_codes_find_index(ctx, si, expr, i,
                                                         &index, &sid);
    i += offset;
    if (index) {
      uint32_t scorer_args_expr_offset = 0;
      if (expr->codes[i].op != GRN_OP_CALL) {
        scorer_args_expr_offset = i;
      }
      while (i < expr->codes_curr && expr->codes[i].op != GRN_OP_CALL) {
        i++;
      }
      scan_info_put_index(ctx, si, index, sid,
                          get_weight(ctx, &(expr->codes[i]), &offset) + weight,
                          ec->value,
                          (grn_obj *)expr,
                          scorer_args_expr_offset);
      i += offset;
    }
    break;
  default :
    {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      name_size = grn_obj_name(ctx, ec->value, name, GRN_TABLE_MAX_KEY_SIZE);
      ERR(GRN_INVALID_ARGUMENT,
          "invalid match target: <%.*s>",
          name_size, name);
      return expr->codes_curr;
    }
    break;
  }

  return i + 1;
}

static void
scan_info_build_match_expr(grn_ctx *ctx,
                           scan_info *si,
                           grn_expr *expr,
                           int32_t weight)
{
  uint32_t i;
  i = 0;
  while (i < expr->codes_curr) {
    i = scan_info_build_match_expr_codes(ctx, si, expr, i, weight);
  }
}

static grn_bool
is_index_searchable_regexp(grn_ctx *ctx, grn_obj *regexp)
{
  const char *all_off_options = "?-mix:";
  size_t all_off_options_length = strlen(all_off_options);
  const char *regexp_raw;
  const char *regexp_raw_end;
  grn_bool escaping = GRN_FALSE;
  grn_bool in_paren = GRN_FALSE;
  grn_bool dot = GRN_FALSE;

  if (!(regexp->header.domain == GRN_DB_SHORT_TEXT ||
        regexp->header.domain == GRN_DB_TEXT ||
        regexp->header.domain == GRN_DB_LONG_TEXT)) {
    return GRN_FALSE;
  }

  regexp_raw = GRN_TEXT_VALUE(regexp);
  regexp_raw_end = regexp_raw + GRN_TEXT_LEN(regexp);

  while (regexp_raw < regexp_raw_end) {
    unsigned int char_len;

    char_len = grn_charlen(ctx, regexp_raw, regexp_raw_end);
    if (char_len == 0) {
      return GRN_FALSE;
    }

    if (char_len == 1) {
      if (escaping) {
        escaping = GRN_FALSE;
        switch (regexp_raw[0]) {
        case 'Z' :
        case 'b' :
        case 'B' :
        case 'd' :
        case 'D' :
        case 'h' :
        case 'H' :
        case 'p' :
        case 's' :
        case 'S' :
        case 'w' :
        case 'W' :
        case 'X' :
        case 'k' :
        case 'g' :
        case '1' :
        case '2' :
        case '3' :
        case '4' :
        case '5' :
        case '6' :
        case '7' :
        case '8' :
        case '9' :
          return GRN_FALSE;
        default :
          break;
        }
      } else {
        switch (regexp_raw[0]) {
        case '(' :
          if (in_paren) {
            return GRN_FALSE;
          } else {
            const char *options = regexp_raw + 1;
            if (regexp_raw_end - options >= all_off_options_length &&
                memcmp(options, all_off_options, all_off_options_length) == 0) {
              in_paren = GRN_TRUE;
              regexp_raw += all_off_options_length;
              continue;
            } else {
              return GRN_FALSE;
            }
          }
          break;
        case ')' :
          if (in_paren) {
            in_paren = GRN_FALSE;
          } else {
            return GRN_FALSE;
          }
          break;
        case '.' :
          if (dot) {
            return GRN_FALSE;
          }
          dot = GRN_TRUE;
          break;
        case '*' :
          if (!dot) {
            return GRN_FALSE;
          }
          if (!grn_scan_info_regexp_dot_asterisk_enable)  {
            return GRN_FALSE;
          }
          dot = GRN_FALSE;
          break;
        case '[' :
        case ']' :
        case '|' :
        case '?' :
        case '+' :
        case '{' :
        case '}' :
        case '^' :
        case '$' :
          return GRN_FALSE;
        case '\\' :
          if (dot) {
            return GRN_FALSE;
          }
          escaping = GRN_TRUE;
          break;
        default :
          if (dot) {
            return GRN_FALSE;
          }
          break;
        }
      }
    } else {
      escaping = GRN_FALSE;
    }

    regexp_raw += char_len;
  }

  if (dot) {
    return GRN_FALSE;
  }
  if (in_paren) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static void
scan_info_build_match(grn_ctx *ctx, scan_info *si, int32_t weight)
{
  grn_obj **p, **pe;

  if (si->op == GRN_OP_REGEXP) {
    p = si->args;
    pe = si->args + si->nargs;
    for (; p < pe; p++) {
      if ((*p)->header.type == GRN_BULK &&
          !is_index_searchable_regexp(ctx, *p)) {
        return;
      }
    }
  }

  p = si->args;
  pe = si->args + si->nargs;
  for (; p < pe; p++) {
    if ((*p)->header.type == GRN_EXPR) {
      scan_info_build_match_expr(ctx, si, (grn_expr *)(*p), weight);
    } else if ((*p)->header.type == GRN_COLUMN_INDEX) {
      grn_obj *index = grn_ctx_at(ctx, grn_obj_id(ctx, (*p)));
      scan_info_put_index(ctx, si, index, 0, 1 + weight, NULL, NULL, 0);
    } else if (grn_obj_is_proc(ctx, *p)) {
      break;
    } else if (GRN_DB_OBJP(*p)) {
      grn_index_datum index_datum;
      unsigned int n_index_data;
      n_index_data = grn_column_find_index_data(ctx, *p, si->op,
                                                &index_datum, 1);
      if (n_index_data > 0) {
        scan_info_put_index(ctx, si,
                            index_datum.index, index_datum.section, 1 + weight,
                            NULL, NULL, 0);
      }
    } else if (GRN_ACCESSORP(*p)) {
      grn_index_datum index_datum;
      unsigned int n_index_data;
      si->flags |= SCAN_ACCESSOR;
      n_index_data = grn_column_find_index_data(ctx, *p, si->op,
                                                &index_datum, 1);
      if (n_index_data > 0) {
        grn_obj *index;
        if (((grn_accessor *)(*p))->next) {
          index = *p;
          if (index != index_datum.index) {
            grn_obj_unref(ctx, index_datum.index);
          }
        } else {
          index = index_datum.index;
        }
        scan_info_put_index(ctx, si,
                            index, index_datum.section, 1 + weight,
                            NULL, NULL, 0);
      }
    } else {
      switch (si->op) {
      case GRN_OP_NEAR :
      case GRN_OP_NEAR2 :
      case GRN_OP_NEAR_PHRASE :
        if (si->nargs == 3 &&
            *p == si->args[2] &&
            (*p)->header.domain == GRN_DB_INT32) {
          si->max_interval = GRN_INT32_VALUE(*p);
        } else {
          si->query = *p;
        }
        break;
      case GRN_OP_SIMILAR :
        if (si->nargs == 3 &&
            *p == si->args[2] &&
            (*p)->header.domain == GRN_DB_INT32) {
          si->similarity_threshold = GRN_INT32_VALUE(*p);
        } else {
          si->query = *p;
        }
        break;
      case GRN_OP_QUORUM :
        if (si->nargs == 3 &&
            *p == si->args[2] &&
            (*p)->header.domain == GRN_DB_INT32) {
          si->quorum_threshold = GRN_INT32_VALUE(*p);
        } else {
          si->query = *p;
        }
        break;
      default :
        si->query = *p;
        break;
      }
    }
  }
}

static grn_bool
grn_scan_info_build_full_not(grn_ctx *ctx,
                             scan_info **sis,
                             int *i,
                             grn_expr_code *codes,
                             grn_expr_code *code,
                             grn_expr_code *code_end,
                             grn_operator *next_code_op)
{
  scan_info *last_si;

  if (*i == 0) {
    return GRN_TRUE;
  }

  last_si = sis[*i - 1];
  switch (last_si->op) {
  case GRN_OP_LESS :
    last_si->op = GRN_OP_GREATER_EQUAL;
    last_si->end++;
    break;
  case GRN_OP_LESS_EQUAL :
    last_si->op = GRN_OP_GREATER;
    last_si->end++;
    break;
  case GRN_OP_GREATER :
    last_si->op = GRN_OP_LESS_EQUAL;
    last_si->end++;
    break;
  case GRN_OP_GREATER_EQUAL :
    last_si->op = GRN_OP_LESS;
    last_si->end++;
    break;
  case GRN_OP_NOT_EQUAL :
    last_si->op = GRN_OP_EQUAL;
    last_si->end++;
    break;
  default :
    if (*i == 1) {
      if (GRN_BULK_VSIZE(&(last_si->index)) > 0) {
        scan_info *all_records_si = NULL;
        SI_ALLOC_RAW(all_records_si, 0);
        if (!all_records_si) {
          return GRN_FALSE;
        }
        all_records_si->op = GRN_OP_CALL;
        grn_scan_info_push_arg(ctx, all_records_si,
                               grn_ctx_get(ctx, "all_records", -1));
        last_si->logical_op = GRN_OP_AND_NOT;
        last_si->flags &= ~SCAN_PUSH;
        sis[*i] = sis[*i - 1];
        sis[*i - 1] = all_records_si;
        (*i)++;
      } else {
        if (last_si->op == GRN_OP_EQUAL) {
          last_si->op = GRN_OP_NOT_EQUAL;
          last_si->end++;
        } else {
          return GRN_FALSE;
        }
      }
    } else {
      grn_expr_code *next_code = code + 1;

      if (next_code >= code_end) {
        return GRN_FALSE;
      }

      switch (next_code->op) {
      case GRN_OP_AND :
        *next_code_op = GRN_OP_AND_NOT;
        break;
      case GRN_OP_AND_NOT :
        *next_code_op = GRN_OP_AND;
        break;
      case GRN_OP_OR :
        {
          scan_info *all_records_si = NULL;
          SI_ALLOC_RAW(all_records_si, 0);
          if (!all_records_si) {
            return GRN_FALSE;
          }
          all_records_si->op = GRN_OP_CALL;
          grn_scan_info_push_arg(ctx, all_records_si,
                                 grn_ctx_get(ctx, "all_records", -1));
          sis[*i] = sis[*i - 1];
          sis[*i - 1] = all_records_si;
          (*i)++;
          put_logical_op(ctx, sis, i, GRN_OP_AND_NOT, code - codes);
        }
        break;
      default :
        return GRN_FALSE;
        break;
      }
    }
  }

  return GRN_TRUE;
}

static scan_info **
grn_scan_info_build_full(grn_ctx *ctx, grn_obj *expr, int *n,
                         grn_operator op, grn_bool record_exist)
{
  grn_obj *var;
  scan_stat stat;
  int i, m = 0, o = 0;
  int n_nots = 0;
  scan_info **sis, *si = NULL;
  grn_expr_code *c, *ce;
  grn_expr *e = (grn_expr *)expr;
  grn_operator next_code_op;

  if (!(var = grn_expr_get_var_by_offset(ctx, expr, 0))) { return NULL; }
  for (stat = SCAN_START, c = e->codes, ce = &e->codes[e->codes_curr]; c < ce; c++) {
    switch (c->op) {
    case GRN_OP_MATCH :
    case GRN_OP_NEAR :
    case GRN_OP_NEAR2 :
    case GRN_OP_NEAR_PHRASE :
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
    case GRN_OP_REGEXP :
    case GRN_OP_QUORUM :
      if (stat < SCAN_COL1 || SCAN_CONST < stat) { return NULL; }
      stat = SCAN_START;
      m++;
      break;
    case GRN_OP_BITWISE_OR :
    case GRN_OP_BITWISE_XOR :
    case GRN_OP_BITWISE_AND :
    case GRN_OP_BITWISE_NOT :
    case GRN_OP_SHIFTL :
    case GRN_OP_SHIFTR :
    case GRN_OP_SHIFTRR :
    case GRN_OP_PLUS :
    case GRN_OP_MINUS :
    case GRN_OP_STAR :
    case GRN_OP_MOD :
      if (stat < SCAN_COL1 || SCAN_CONST < stat) { return NULL; }
      stat = SCAN_START;
      if (m != o + 1) { return NULL; }
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_AND_NOT :
    case GRN_OP_ADJUST :
      switch (stat) {
      case SCAN_START :
        o++;
        if (o >= m) { return NULL; }
        break;
      case SCAN_CONST :
        o++;
        m++;
        if (o >= m) { return NULL; }
        stat = SCAN_START;
        break;
      default :
        return NULL;
        break;
      }
      break;
    case GRN_OP_PUSH :
      {
        grn_bool is_completed_term = GRN_FALSE;
        if (c->modify > 0) {
          switch ((c + c->modify)->op) {
          case GRN_OP_AND :
          case GRN_OP_OR :
          case GRN_OP_AND_NOT :
          case GRN_OP_ADJUST :
            is_completed_term = GRN_TRUE;
            break;
          default :
            is_completed_term = GRN_FALSE;
            break;
          }
        }
        if (is_completed_term) {
          m++;
          stat = SCAN_START;
        } else {
          stat = (c->value == var) ? SCAN_VAR : SCAN_CONST;
        }
      }
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
    case GRN_OP_GET_REF :
      switch (stat) {
      case SCAN_START :
        stat = SCAN_COL1;
        break;
      default :
        return NULL;
        break;
      }
      break;
    case GRN_OP_GET_MEMBER :
      switch (stat) {
      case SCAN_CONST :
        {
          grn_expr_code *prev_c = c - 1;
          if (prev_c->value->header.domain < GRN_DB_INT8 ||
              prev_c->value->header.domain > GRN_DB_UINT64) {
            return NULL;
          }
        }
        stat = SCAN_COL1;
        break;
      default :
        return NULL;
        break;
      }
      break;
    case GRN_OP_NOT :
      n_nots++;
      break;
    default :
      return NULL;
      break;
    }
  }
  if (stat || m != o + 1) { return NULL; }
  if (!(sis = GRN_MALLOCN(scan_info *, m + m + o + n_nots))) { return NULL; }

  next_code_op = (grn_operator)-1;
  for (i = 0, stat = SCAN_START, c = e->codes, ce = &e->codes[e->codes_curr]; c < ce; c++) {
    grn_operator code_op;
    if (next_code_op == (grn_operator)-1) {
      code_op = c->op;
    } else {
      code_op = next_code_op;
      next_code_op = -1;
    }
    switch (code_op) {
    case GRN_OP_MATCH :
    case GRN_OP_NEAR :
    case GRN_OP_NEAR2 :
    case GRN_OP_NEAR_PHRASE :
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
    case GRN_OP_REGEXP :
    case GRN_OP_QUORUM :
      stat = SCAN_START;
      si->op = code_op;
      si->end = c - e->codes;
      sis[i++] = si;
      {
        int32_t weight = 0;
        if (c->value && c->value->header.domain == GRN_DB_INT32) {
          weight = GRN_INT32_VALUE(c->value);
        }
        scan_info_build_match(ctx, si, weight);
      }
      if (ctx->rc != GRN_SUCCESS) {
        int j;
        for (j = 0; j < i; j++) { SI_FREE(sis[j]); }
        GRN_FREE(sis);
        return NULL;
      }
      si = NULL;
      break;
    case GRN_OP_AND :
    case GRN_OP_OR :
    case GRN_OP_AND_NOT :
    case GRN_OP_ADJUST :
      if (stat == SCAN_CONST) {
        si->op = GRN_OP_PUSH;
        si->end = si->start;
        sis[i++] = si;
        si = NULL;
      }
      if (!put_logical_op(ctx, sis, &i, code_op, c - e->codes)) { return NULL; }
      stat = SCAN_START;
      break;
    case GRN_OP_PUSH :
      if (!si) { SI_ALLOC(si, i, c - e->codes); }
      if (c->value == var) {
        stat = SCAN_VAR;
      } else {
        grn_scan_info_push_arg(ctx, si, c->value);
        if (stat == SCAN_START) { si->flags |= SCAN_PRE_CONST; }
        stat = SCAN_CONST;
      }
      if (c->modify > 0) {
        grn_bool is_completed_term = GRN_FALSE;
        switch ((c + c->modify)->op) {
        case GRN_OP_AND :
        case GRN_OP_OR :
        case GRN_OP_AND_NOT :
        case GRN_OP_ADJUST :
          is_completed_term = GRN_TRUE;
          break;
        default :
          is_completed_term = GRN_FALSE;
          break;
        }
        if (is_completed_term) {
          si->op = GRN_OP_PUSH;
          si->end = si->start;
          sis[i++] = si;
          si = NULL;
          stat = SCAN_START;
        }
      }
      break;
    case GRN_OP_GET_VALUE :
      switch (stat) {
      case SCAN_START :
        if (!si) { SI_ALLOC(si, i, c - e->codes); }
        // fallthru
      case SCAN_CONST :
      case SCAN_VAR :
        stat = SCAN_COL1;
        grn_scan_info_push_arg(ctx, si, c->value);
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
          SI_FREE(si);
          for (j = 0; j < i; j++) { SI_FREE(sis[j]); }
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
      if (!si) { SI_ALLOC(si, i, c - e->codes); }
      if ((c->flags & GRN_EXPR_CODE_RELATIONAL_EXPRESSION) || c + 1 == ce) {
        stat = SCAN_START;
        si->op = code_op;
        si->end = c - e->codes;
        sis[i++] = si;
        /* better index resolving framework for functions should be implemented */
        if (grn_obj_is_selector_proc(ctx, si->args[0])) {
          grn_obj *selector;
          grn_obj **p;
          grn_obj **pe;
          grn_operator selector_op;

          selector = si->args[0];
          p = si->args + 1;
          pe = si->args + si->nargs;
          selector_op = grn_proc_get_selector_operator(ctx, selector);
          for (; p < pe; p++) {
            if (GRN_DB_OBJP(*p)) {
              grn_index_datum index_datum;
              unsigned int n_index_data;
              n_index_data = grn_column_find_index_data(ctx, *p, selector_op,
                                                        &index_datum, 1);
              if (n_index_data > 0) {
                scan_info_put_index(ctx, si,
                                    index_datum.index, index_datum.section, 1,
                                    NULL, NULL, 0);
              }
            } else if (GRN_ACCESSORP(*p)) {
              grn_index_datum index_datum;
              unsigned int n_index_data;
              si->flags |= SCAN_ACCESSOR;
              n_index_data = grn_column_find_index_data(ctx, *p, selector_op,
                                                        &index_datum, 1);
              if (n_index_data > 0) {
                scan_info_put_index(ctx, si,
                                    index_datum.index, index_datum.section, 1,
                                    NULL, NULL, 0);
              }
            } else {
              si->query = *p;
            }
          }
        }
        si = NULL;
      } else {
        stat = SCAN_COL2;
      }
      break;
    case GRN_OP_GET_REF :
      switch (stat) {
      case SCAN_START :
        if (!si) { SI_ALLOC(si, i, c - e->codes); }
        stat = SCAN_COL1;
        grn_scan_info_push_arg(ctx, si, c->value);
        break;
      default :
        break;
      }
      break;
    case GRN_OP_GET_MEMBER :
      {
        grn_obj *start_position;
        grn_obj buffer;
        start_position = si->args[--si->nargs];
        GRN_INT32_INIT(&buffer, 0);
        grn_obj_cast(ctx, start_position, &buffer, GRN_FALSE);
        grn_scan_info_set_start_position(si, GRN_INT32_VALUE(&buffer));
        GRN_OBJ_FIN(ctx, &buffer);
      }
      stat = SCAN_COL1;
      break;
    case GRN_OP_NOT :
      {
        grn_bool valid;
        valid = grn_scan_info_build_full_not(ctx,
                                             sis,
                                             &i,
                                             e->codes,
                                             c,
                                             ce,
                                             &next_code_op);
        if (!valid) {
          int j;
          for (j = 0; j < i; j++) {
            SI_FREE(sis[j]);
          }
          GRN_FREE(sis);
          return NULL;
        }
      }
      break;
    default :
      break;
    }
  }
  if (op == GRN_OP_OR && !record_exist) {
    // for debug
    if (!(sis[0]->flags & SCAN_PUSH) || (sis[0]->logical_op != op)) {
      int j;
      ERR(GRN_INVALID_ARGUMENT, "invalid expr");
      for (j = 0; j < i; j++) { SI_FREE(sis[j]); }
      GRN_FREE(sis);
      return NULL;
    } else {
      sis[0]->flags &= ~SCAN_PUSH;
      sis[0]->logical_op = op;
    }
  } else {
    if (!put_logical_op(ctx, sis, &i, op, c - e->codes)) { return NULL; }
  }
  *n = i;
  return sis;
}

static scan_info **
grn_scan_info_build_simple_open(grn_ctx *ctx, int *n, grn_operator logical_op)
{
  scan_info **sis;
  scan_info *si;

  sis = GRN_MALLOCN(scan_info *, 1);
  if (!sis) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[scan_info][build] failed to allocate memory for scan_info **");
    return NULL;
  }

  si = grn_scan_info_open(ctx, 0);
  if (!si) {
    ERR(GRN_NO_MEMORY_AVAILABLE,
        "[scan_info][build] failed to allocate memory for scan_info *");
    GRN_FREE(sis);
    return NULL;
  }

  si->flags &= ~SCAN_PUSH;
  si->logical_op = logical_op;

  sis[0] = si;
  *n = 1;

  return sis;
}

static scan_info **
grn_scan_info_build_simple_value(grn_ctx *ctx,
                                 grn_obj *expr,
                                 int *n,
                                 grn_operator logical_op,
                                 grn_bool record_exist)
{
  grn_expr *e = (grn_expr *)expr;
  scan_info **sis;
  scan_info *si;
  grn_expr_code *target = e->codes;

  switch (target->op) {
  case GRN_OP_PUSH :
  case GRN_OP_GET_VALUE :
    break;
  default :
    return NULL;
    break;
  }

  sis = grn_scan_info_build_simple_open(ctx, n, logical_op);
  if (!sis) {
    return NULL;
  }

  si = sis[0];
  si->end = 0;
  si->op = target->op;
  return sis;
}

static scan_info **
grn_scan_info_build_simple_operation(grn_ctx *ctx,
                                     grn_obj *expr,
                                     int *n,
                                     grn_operator logical_op,
                                     grn_bool record_exist)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *target;
  grn_expr_code *constant;
  grn_expr_code *operator;
  scan_info **sis;
  scan_info *si;

  target   = e->codes + 0;
  constant = e->codes + 1;
  operator = e->codes + 2;

  if (target->op != GRN_OP_GET_VALUE) {
    return NULL;
  }
  if (target->nargs != 1) {
    return NULL;
  }
  if (!target->value) {
    return NULL;
  }

  if (constant->op != GRN_OP_PUSH) {
    return NULL;
  }
  if (constant->nargs != 1) {
    return NULL;
  }
  if (!constant->value) {
    return NULL;
  }

  if (operator->nargs != 2) {
    return NULL;
  }
  switch (operator->op) {
  case GRN_OP_MATCH :
  case GRN_OP_NEAR :
  case GRN_OP_NEAR_PHRASE :
  case GRN_OP_SIMILAR :
  case GRN_OP_PREFIX :
  case GRN_OP_SUFFIX :
  case GRN_OP_EQUAL :
  case GRN_OP_NOT_EQUAL :
  case GRN_OP_LESS :
  case GRN_OP_GREATER :
  case GRN_OP_LESS_EQUAL :
  case GRN_OP_GREATER_EQUAL :
  case GRN_OP_TERM_EXTRACT :
  case GRN_OP_REGEXP :
  case GRN_OP_QUORUM :
    break;
  default :
    return NULL;
    break;
  }

  sis = grn_scan_info_build_simple_open(ctx, n, logical_op);
  if (!sis) {
    return NULL;
  }

  si = sis[0];
  si->end = 2;
  si->op = operator->op;
  grn_scan_info_push_arg(ctx, si, target->value);
  grn_scan_info_push_arg(ctx, si, constant->value);
  {
    int32_t weight = 0;
    if (operator->value && operator->value->header.domain == GRN_DB_INT32) {
      weight = GRN_INT32_VALUE(operator->value);
    }
    scan_info_build_match(ctx, si, weight);
    if (ctx->rc != GRN_SUCCESS) {
      SI_FREE(sis[0]);
      GRN_FREE(sis);
      return NULL;
    }
  }
  return sis;
}

static scan_info **
grn_scan_info_build_simple_and_operations(grn_ctx *ctx,
                                          grn_obj *expr,
                                          int *n,
                                          grn_operator logical_op,
                                          grn_bool record_exist)
{
  grn_expr *e = (grn_expr *)expr;
  scan_info **sis = NULL;
  int n_sis = 0;
  int i;
  int nth_sis;

  for (i = 0, nth_sis = 0; i < e->codes_curr; i += 3, nth_sis++) {
    grn_expr_code *target = e->codes + i;
    grn_expr_code *constant = e->codes + i + 1;
    grn_expr_code *operator = e->codes + i + 2;

    if (target->op != GRN_OP_GET_VALUE) {
      return NULL;
    }
    if (target->nargs != 1) {
      return NULL;
    }
    if (!target->value) {
      return NULL;
    }

    if (constant->op != GRN_OP_PUSH) {
      return NULL;
    }
    if (constant->nargs != 1) {
      return NULL;
    }
    if (!constant->value) {
      return NULL;
    }

    if (operator->nargs != 2) {
      return NULL;
    }
    switch (operator->op) {
    case GRN_OP_MATCH :
    case GRN_OP_NEAR :
    case GRN_OP_NEAR_PHRASE :
    case GRN_OP_SIMILAR :
    case GRN_OP_PREFIX :
    case GRN_OP_SUFFIX :
    case GRN_OP_EQUAL :
    case GRN_OP_NOT_EQUAL :
    case GRN_OP_LESS :
    case GRN_OP_GREATER :
    case GRN_OP_LESS_EQUAL :
    case GRN_OP_GREATER_EQUAL :
    case GRN_OP_TERM_EXTRACT :
    case GRN_OP_REGEXP :
    case GRN_OP_QUORUM :
      break;
    default :
      return NULL;
      break;
    }

    if (nth_sis > 0) {
      grn_expr_code *logical_operator = e->codes + i + 3;

      if (logical_operator->op != GRN_OP_AND) {
        return NULL;
      }
      if (logical_operator->nargs != 2) {
        return NULL;
      }

      i++;
    }
  }
  n_sis = nth_sis;

  sis = GRN_CALLOC(sizeof(scan_info *) * n_sis);
  if (!sis) {
    return NULL;
  }

  for (i = 0, nth_sis = 0; i < e->codes_curr; i += 3, nth_sis++) {
    grn_expr_code *target = e->codes + i;
    grn_expr_code *constant = e->codes + i + 1;
    grn_expr_code *operator = e->codes + i + 2;
    scan_info *si;

    sis[nth_sis] = si = grn_scan_info_open(ctx, i);
    if (!si) {
      goto exit;
    }
    grn_scan_info_push_arg(ctx, si, target->value);
    grn_scan_info_push_arg(ctx, si, constant->value);
    si->op = operator->op;
    si->end = i + 2;
    si->flags &= ~SCAN_PUSH;
    if (nth_sis == 0) {
      si->logical_op = logical_op;
    } else {
      si->logical_op = GRN_OP_AND;
    }
    {
      int32_t weight = 0;
      if (operator->value && operator->value->header.domain == GRN_DB_INT32) {
        weight = GRN_INT32_VALUE(operator->value);
      }
      scan_info_build_match(ctx, si, weight);
    }

    if (nth_sis > 0) {
      i++;
    }
  }

  *n = n_sis;
  return sis;

exit :
  if (n_sis > 0) {
    for (i = 0; i < n_sis; i++) {
      scan_info *si = sis[i];
      if (si) {
        grn_scan_info_close(ctx, si);
      }
    }
    GRN_FREE(sis);
  }

  return NULL;
}

static scan_info **
grn_scan_info_build_simple(grn_ctx *ctx, grn_obj *expr, int *n,
                           grn_operator logical_op, grn_bool record_exist)
{
  grn_expr *e = (grn_expr *)expr;

  if (e->codes_curr == 1) {
    return grn_scan_info_build_simple_value(ctx,
                                            expr,
                                            n,
                                            logical_op,
                                            record_exist);
  } else if (e->codes_curr == 3) {
    return grn_scan_info_build_simple_operation(ctx,
                                                expr,
                                                n,
                                                logical_op,
                                                record_exist);
  } else if (e->codes_curr % 4 == 3) {
    return grn_scan_info_build_simple_and_operations(ctx,
                                                     expr,
                                                     n,
                                                     logical_op,
                                                     record_exist);
  }

  return NULL;
}

scan_info **
grn_scan_info_build(grn_ctx *ctx, grn_obj *expr, int *n,
                    grn_operator op, grn_bool record_exist)
{
  scan_info **sis;

  sis = grn_scan_info_build_simple(ctx, expr, n, op, record_exist);
#ifdef GRN_WITH_MRUBY
  if (!sis) {
    grn_ctx_impl_mrb_ensure_init(ctx);
    if (ctx->rc != GRN_SUCCESS) {
      return NULL;
    }
    if (ctx->impl->mrb.state) {
      return grn_mrb_scan_info_build(ctx, expr, n, op, record_exist);
    }
  }
#endif
  if (!sis) {
    sis = grn_scan_info_build_full(ctx, expr, n, op, record_exist);
  }
  return sis;
}

void
grn_inspect_scan_info_list(grn_ctx *ctx, grn_obj *buffer, scan_info **sis, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    scan_info *si = sis[i];

    grn_text_printf(ctx, buffer, "[%d]\n", i);
    grn_text_printf(ctx, buffer,
                    "  op:         <%s>\n",
                    grn_operator_to_string(si->op));
    grn_text_printf(ctx, buffer,
                    "  logical_op: <%s>\n",
                    grn_operator_to_string(si->logical_op));

    if (si->op == GRN_OP_CALL) {
      int i;
      for (i = 0; i < si->nargs; i++) {
        grn_text_printf(ctx, buffer, "  args[%d]:    <", i);
        grn_inspect(ctx, buffer, si->args[i]);
        GRN_TEXT_PUTS(ctx, buffer, ">\n");
      }
    } else {
      GRN_TEXT_PUTS(ctx, buffer, "  index:      <");
      grn_inspect(ctx, buffer, &(si->index));
      GRN_TEXT_PUTS(ctx, buffer, ">\n");

      GRN_TEXT_PUTS(ctx, buffer, "  query:      <");
      grn_inspect(ctx, buffer, si->query);
      GRN_TEXT_PUTS(ctx, buffer, ">\n");
    }

    grn_text_printf(ctx, buffer,
                    "  expr:       <%d..%d>\n", si->start, si->end);
  }
}

void
grn_p_scan_info_list(grn_ctx *ctx, scan_info **sis, int n)
{
  grn_obj inspected;
  GRN_TEXT_INIT(&inspected, 0);
  grn_inspect_scan_info_list(ctx, &inspected, sis, n);
  printf("%.*s\n",
         (int)GRN_TEXT_LEN(&inspected),
         GRN_TEXT_VALUE(&inspected));
  GRN_OBJ_FIN(ctx, &inspected);
}

grn_inline static int32_t
exec_result_to_score(grn_ctx *ctx, grn_obj *result, grn_obj *score_buffer)
{
  if (!result) {
    return 0;
  }

  switch (result->header.type) {
  case GRN_VOID :
    return 0;
  case GRN_BULK :
    switch (result->header.domain) {
    case GRN_DB_BOOL :
      return GRN_BOOL_VALUE(result) ? 1 : 0;
    case GRN_DB_INT32 :
      return GRN_INT32_VALUE(result);
    default :
      GRN_BULK_REWIND(score_buffer);
      if (grn_obj_cast(ctx, result, score_buffer, GRN_FALSE) != GRN_SUCCESS) {
        return 1;
      }
      return GRN_INT32_VALUE(score_buffer);
    }
  case GRN_UVECTOR :
  case GRN_PVECTOR :
  case GRN_VECTOR :
    return 1;
  default :
    return 1; /* TODO: 1 is reasonable? */
  }
}

typedef struct {
  grn_obj *expr;
  grn_obj *variable;
  grn_scanner *scanner;
  int nth_scan_info;
  scan_info *scan_info;
  grn_obj *res;
  grn_id min_id;
  grn_bool is_first_unskipped_scan_info;
  struct {
    grn_bool is_skipped;
    grn_search_optarg search_options;
  } current;
} grn_table_select_data;

grn_rc
grn_table_select_sequential(grn_ctx *ctx,
                            grn_obj *table,
                            grn_obj *expr,
                            grn_obj *res,
                            grn_operator op)
{
  grn_obj *result;
  grn_obj score_buffer;
  int32_t score;
  grn_id id, *idp;
  grn_table_cursor *tc;
  grn_hash_cursor *hc;
  grn_hash *s = (grn_hash *)res;
  grn_expr_executor executor;

  grn_expr_executor_init(ctx, &executor, expr);
  if (ctx->rc != GRN_SUCCESS) {
    return ctx->rc;
  }
  GRN_INT32_INIT(&score_buffer, 0);
  switch (op) {
  case GRN_OP_OR :
    if ((tc = grn_table_cursor_open(ctx, table, NULL, 0, NULL, 0, 0, -1, 0))) {
      while ((id = grn_table_cursor_next(ctx, tc))) {
        result = grn_expr_executor_exec(ctx, &executor, id);
        if (ctx->rc) {
          break;
        }
        score = exec_result_to_score(ctx, result, &score_buffer);
        if (score > 0) {
          grn_rset_recinfo *ri;
          if (grn_hash_add(ctx, s, &id, s->key_size, (void **)&ri, NULL)) {
            grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)&id, 1);
          }
        }
      }
      grn_table_cursor_close(ctx, tc);
    }
    break;
  case GRN_OP_AND :
    if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0, -1, 0))) {
      while (grn_hash_cursor_next(ctx, hc)) {
        grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
        result = grn_expr_executor_exec(ctx, &executor, *idp);
        if (ctx->rc) {
          break;
        }
        score = exec_result_to_score(ctx, result, &score_buffer);
        if (score > 0) {
          grn_rset_recinfo *ri;
          grn_hash_cursor_get_value(ctx, hc, (void **) &ri);
          grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)idp, 1);
        } else {
          grn_hash_cursor_delete(ctx, hc, NULL);
        }
      }
      grn_hash_cursor_close(ctx, hc);
    }
    break;
  case GRN_OP_AND_NOT :
    if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0, -1, 0))) {
      while (grn_hash_cursor_next(ctx, hc)) {
        grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
        result = grn_expr_executor_exec(ctx, &executor, *idp);
        if (ctx->rc) {
          break;
        }
        score = exec_result_to_score(ctx, result, &score_buffer);
        if (score > 0) {
          grn_hash_cursor_delete(ctx, hc, NULL);
        }
      }
      grn_hash_cursor_close(ctx, hc);
    }
    break;
  case GRN_OP_ADJUST :
    if ((hc = grn_hash_cursor_open(ctx, s, NULL, 0, NULL, 0, 0, -1, 0))) {
      while (grn_hash_cursor_next(ctx, hc)) {
        grn_hash_cursor_get_key(ctx, hc, (void **) &idp);
        result = grn_expr_executor_exec(ctx, &executor, *idp);
        if (ctx->rc) {
          break;
        }
        score = exec_result_to_score(ctx, result, &score_buffer);
        if (score > 0) {
          grn_rset_recinfo *ri;
          grn_hash_cursor_get_value(ctx, hc, (void **) &ri);
          grn_table_add_subrec(res, ri, score, (grn_rset_posinfo *)idp, 1);
        }
      }
      grn_hash_cursor_close(ctx, hc);
    }
    break;
  default :
    break;
  }
  GRN_OBJ_FIN(ctx, &score_buffer);
  grn_expr_executor_fin(ctx, &executor);

  return ctx->rc;
}

static grn_inline void
grn_table_select_index_report(grn_ctx *ctx, const char *tag, grn_obj *index)
{
  grn_report_index(ctx, "[table][select]", tag, index);
}

static grn_inline void
grn_table_select_index_not_used_report(grn_ctx *ctx,
                                       const char *tag,
                                       grn_obj *index,
                                       const char *reason)
{
  grn_report_index_not_used(ctx, "[table][select]", tag, index, reason);
}

static grn_inline bool
grn_table_select_index_use_sequential_search(grn_ctx *ctx,
                                             grn_obj *res,
                                             grn_operator logical_op,
                                             const char *tag,
                                             grn_obj *index)
{
  if (logical_op != GRN_OP_AND) {
    return false;
  }

  grn_obj *table = grn_ctx_at(ctx, res->header.domain);
  int n_records = grn_table_size(ctx, table);
  int n_filtered_records = grn_table_size(ctx, res);
  grn_obj_unref(ctx, table);
  double filtered_ratio;
  if (n_records == 0) {
    filtered_ratio = 1.0;
  } else {
    filtered_ratio = (double)n_filtered_records / (double)n_records;
  }

  if (filtered_ratio >= grn_table_select_enough_filtered_ratio) {
    return false;
  }

  if (n_filtered_records > grn_table_select_max_n_enough_filtered_records) {
    return false;
  }

  grn_obj reason;
  GRN_TEXT_INIT(&reason, 0);
  grn_text_printf(ctx, &reason,
                  "enough filtered: %.2f%%(%d/%d) < %.2f%% && %d <= %d",
                  filtered_ratio * 100,
                  n_filtered_records,
                  n_records,
                  grn_table_select_enough_filtered_ratio * 100,
                  n_filtered_records,
                  grn_table_select_max_n_enough_filtered_records);
  GRN_TEXT_PUTC(ctx, &reason, '\0');
  grn_table_select_index_not_used_report(ctx,
                                         tag,
                                         index,
                                         GRN_TEXT_VALUE(&reason));
  GRN_OBJ_FIN(ctx, &reason);
  return true;
}

static grn_inline grn_id
grn_table_select_index_resolve_key(grn_ctx *ctx,
                                   grn_obj *domain,
                                   grn_obj *key)
{
  if (GRN_OBJ_GET_DOMAIN(key) == DB_OBJ(domain)->id) {
    return GRN_RECORD_VALUE(key);
  } else if (GRN_OBJ_GET_DOMAIN(key) == domain->header.domain) {
    return grn_table_get(ctx,
                         domain,
                         GRN_BULK_HEAD(key),
                         GRN_BULK_VSIZE(key));
  } else {
    grn_id id = GRN_ID_NIL;
    grn_obj casted_key;
    GRN_OBJ_INIT(&casted_key, GRN_BULK, 0, domain->header.domain);
    if (grn_obj_cast(ctx, key, &casted_key, GRN_FALSE) == GRN_SUCCESS) {
      id = grn_table_get(ctx,
                         domain,
                         GRN_BULK_HEAD(&casted_key),
                         GRN_BULK_VSIZE(&casted_key));
    }
    GRN_OBJ_FIN(ctx, &casted_key);
    return id;
  }
}

static grn_inline grn_rc
grn_table_select_index_equal(grn_ctx *ctx,
                             grn_obj *index,
                             grn_operator op,
                             grn_obj *res,
                             grn_operator logical_op,
                             grn_table_select_data *data)
{
  scan_info *si = data->scan_info;

  if (si->query->header.type != GRN_BULK) {
    /* We can't use index for non bulk value such as vector. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  if (GRN_BULK_VSIZE(si->query) == 0) {
    /* We can't use index for empty value. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  if (grn_obj_is_accessor(ctx, index)) {
    grn_search_optarg optarg;
    memset(&optarg, 0, sizeof(grn_search_optarg));
    optarg.mode = GRN_OP_EQUAL;
    optarg.vector_size = 1;
    rc = grn_obj_search(ctx,
                        index,
                        si->query,
                        res,
                        logical_op,
                        &optarg);
  } else {
    const char *tag = "[equal]";

    grn_obj *domain = grn_ctx_at(ctx, index->header.domain);
    if (domain) {
      bool optimizable = false;

      if (domain->header.domain == GRN_DB_SHORT_TEXT) {
        grn_obj *normalizer = NULL;
        grn_table_get_info(ctx, domain, NULL, NULL, NULL, &normalizer, NULL);
        if (normalizer == grn_ctx_get(ctx, "NormalizerAuto", -1)) {
          optimizable = true;
        }
      } else {
        optimizable = true;
      }
      if (optimizable &&
          grn_table_select_index_use_sequential_search(ctx,
                                                       res,
                                                       logical_op,
                                                       tag,
                                                       index)) {
        if (grn_enable_reference_count) {
          grn_obj_unlink(ctx, domain);
        }
        domain = NULL;
      }
    }

    if (domain) {
      grn_id tid;

      grn_table_select_index_report(ctx, tag, index);

      tid = grn_table_select_index_resolve_key(ctx, domain, si->query);
      if (tid != GRN_ID_NIL) {
        uint32_t sid;
        int32_t weight;
        grn_ii *ii = (grn_ii *)index;
        grn_ii_cursor *ii_cursor;

        sid = GRN_UINT32_VALUE_AT(&(si->wv), 0);
        weight = GRN_INT32_VALUE_AT(&(si->wv), 1);
        ii_cursor = grn_ii_cursor_open(ctx, ii, tid,
                                       GRN_ID_NIL, GRN_ID_MAX,
                                       ii->n_elements, 0);
        if (ii_cursor) {
          grn_posting *posting;
          while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
            grn_posting_internal new_posting;

            if (!(sid == 0 || posting->sid == sid)) {
              continue;
            }

            if (si->position.specified) {
              while ((posting = grn_ii_cursor_next_pos(ctx, ii_cursor))) {
                if (posting->pos == si->position.start) {
                  break;
                }
              }
              if (!posting) {
                continue;
              }
            }

            new_posting = *((grn_posting_internal *)posting);
            new_posting.weight_float += 1;
            new_posting.weight_float *= weight;
            grn_ii_posting_add_float(ctx,
                                     (grn_posting *)(&new_posting),
                                     (grn_hash *)res,
                                     logical_op);
          }
          grn_ii_cursor_close(ctx, ii_cursor);
        }
      }
      rc = GRN_SUCCESS;

      if (grn_enable_reference_count) {
        grn_obj_unlink(ctx, domain);
      }
    }
  }

  if (rc == GRN_SUCCESS) {
    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, logical_op);
  }

  return rc;
}

static grn_inline grn_rc
grn_table_select_index_not_equal(grn_ctx *ctx,
                                 grn_obj *index,
                                 grn_operator op,
                                 grn_obj *res,
                                 grn_operator logical_op,
                                 grn_table_select_data *data)
{
  scan_info *si = data->scan_info;

  if (GRN_BULK_VSIZE(si->query) == 0) {
    /* We can't use index for empty value. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  if (logical_op != GRN_OP_AND) {
    /* We can't use index for OR and AND_NOT. */
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  if (grn_obj_is_accessor(ctx, index)) {
    grn_obj dest;
    grn_accessor *a = (grn_accessor *)index;
    grn_id id;
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID :
      grn_table_select_index_report(ctx,
                                    "[not-equal][accessor][id]",
                                    a->obj);
      GRN_UINT32_INIT(&dest, 0);
      if (!grn_obj_cast(ctx, si->query, &dest, GRN_FALSE)) {
        id = GRN_UINT32_VALUE(&dest);
        if (id != GRN_ID_NIL) {
          if (id == grn_table_at(ctx, a->obj, id)) {
            grn_hash_delete(ctx, (grn_hash *)res, &id, sizeof(grn_id), NULL);
          }
        }
        rc = GRN_SUCCESS;
      }
      GRN_OBJ_FIN(ctx, &dest);
      break;
    case GRN_ACCESSOR_GET_KEY :
      grn_table_select_index_report(ctx,
                                    "[not-equal][accessor][key]",
                                    a->obj);
      GRN_OBJ_INIT(&dest, GRN_BULK, 0, a->obj->header.domain);
      if (!grn_obj_cast(ctx, si->query, &dest, GRN_FALSE)) {
        id = grn_table_get(ctx,
                           a->obj,
                           GRN_BULK_HEAD(&dest),
                           GRN_BULK_VSIZE(&dest));
        if (id != GRN_ID_NIL) {
          grn_hash_delete(ctx, (grn_hash *)res, &id, sizeof(grn_id), NULL);
        }
        rc = GRN_SUCCESS;
      }
      GRN_OBJ_FIN(ctx, &dest);
      break;
    }
  } else {
    grn_obj *domain = grn_ctx_at(ctx, index->header.domain);
    if (domain) {
      grn_id tid;

      grn_table_select_index_report(ctx, "[not-equal]", index);

      tid = grn_table_select_index_resolve_key(ctx, domain, si->query);
      if (tid == GRN_ID_NIL) {
        rc = GRN_SUCCESS;
      } else {
        uint32_t sid;
        int32_t weight;
        grn_ii *ii = (grn_ii *)index;
        grn_ii_cursor *ii_cursor;

        sid = GRN_UINT32_VALUE_AT(&(si->wv), 0);
        weight = GRN_INT32_VALUE_AT(&(si->wv), 1);
        ii_cursor = grn_ii_cursor_open(ctx, ii, tid,
                                       GRN_ID_NIL, GRN_ID_MAX,
                                       ii->n_elements, 0);
        if (ii_cursor) {
          grn_posting *posting;
          while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
            if (!(sid == 0 || posting->sid == sid)) {
              continue;
            }

            if (si->position.specified) {
              while ((posting = grn_ii_cursor_next_pos(ctx, ii_cursor))) {
                if (posting->pos == si->position.start) {
                  break;
                }
              }
              if (!posting) {
                continue;
              }
            }

            grn_hash_delete(ctx, (grn_hash *)res,
                            &(posting->rid), sizeof(grn_id),
                            NULL);
          }
          grn_ii_cursor_close(ctx, ii_cursor);
          rc = GRN_SUCCESS;
        }
      }
    }
  }

  return rc;
}

static grn_inline grn_rc
grn_table_select_index_fix(grn_ctx *ctx,
                           grn_obj *index,
                           grn_operator op,
                           grn_obj *res,
                           grn_operator logical_op,
                           grn_table_select_data *data)
{
  scan_info *si = data->scan_info;
  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;

  if (grn_obj_is_accessor(ctx, index)) {
    grn_accessor *a = (grn_accessor *)index;
    switch (a->action) {
    case GRN_ACCESSOR_GET_ID :
      /* todo */
      break;
    case GRN_ACCESSOR_GET_KEY :
      index = a->obj;
      break;
    }
  }

  if (grn_obj_is_table(ctx, index)) {
    grn_obj *table = index;
    if (op == GRN_OP_SUFFIX) {
      grn_table_select_index_report(ctx, "[suffix][accessor][key]", table);
    } else {
      grn_table_select_index_report(ctx, "[prefix][accessor][key]", table);
    }
    grn_obj dest;
    GRN_OBJ_INIT(&dest, GRN_BULK, 0, table->header.domain);
    if (!grn_obj_cast(ctx, si->query, &dest, GRN_FALSE)) {
      grn_hash *pres;
      if ((pres = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY))) {
        grn_table_search(ctx,
                         index,
                         GRN_BULK_HEAD(&dest),
                         GRN_BULK_VSIZE(&dest),
                         op,
                         (grn_obj *)pres,
                         GRN_OP_OR);
        grn_id *key;
        grn_posting_internal posting = {0};
        posting.sid = 1;
        posting.pos = 0;
        posting.weight_float = 1;
        GRN_HASH_EACH(ctx, pres, id, &key, NULL, NULL, {
          posting.rid = *key;
          grn_ii_posting_add_float(ctx,
                                   (grn_posting *)(&posting),
                                   (grn_hash *)res,
                                   logical_op);
        });
        grn_hash_close(ctx, pres);
      }
      rc = GRN_SUCCESS;
      grn_ii_resolve_sel_and(ctx, (grn_hash *)res, logical_op);
    }
    GRN_OBJ_FIN(ctx, &dest);
  } else {
    grn_obj *lexicon = grn_ctx_at(ctx, index->header.domain);
    if (lexicon) {
      grn_hash *keys;
      if ((keys = grn_hash_create(ctx, NULL, sizeof(grn_id), 0,
                                  GRN_OBJ_TABLE_HASH_KEY))) {
        grn_id *key;
        if (op == GRN_OP_SUFFIX) {
          grn_table_select_index_report(ctx, "[suffix]", index);
        } else {
          grn_table_select_index_report(ctx, "[prefix]", index);
        }
        grn_table_search(ctx, lexicon,
                         GRN_BULK_HEAD(si->query),
                         GRN_BULK_VSIZE(si->query),
                         op, (grn_obj *)keys, GRN_OP_OR);
        GRN_HASH_EACH(ctx, keys, id, &key, NULL, NULL, {
          grn_ii_at(ctx, (grn_ii *)index, *key, (grn_hash *)res, logical_op);
        });
        grn_hash_close(ctx, keys);
      }
      grn_obj_unlink(ctx, lexicon);
    }
    rc = GRN_SUCCESS;
  }
  return rc;
}

static grn_inline grn_rc
grn_table_select_index_match(grn_ctx *ctx,
                             grn_obj *index,
                             grn_operator op,
                             grn_obj *res,
                             grn_operator logical_op,
                             grn_table_select_data *data)
{
  scan_info *si = data->scan_info;
  grn_search_optarg *options = &(data->current.search_options);
  return grn_obj_search(ctx, index, si->query, res, logical_op, options);
}

static grn_inline grn_rc
grn_table_select_index_extract(grn_ctx *ctx,
                               grn_obj *index,
                               grn_operator op,
                               grn_obj *res,
                               grn_operator logical_op,
                               grn_table_select_data *data)
{
  if (!grn_obj_is_accessor(ctx, index)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = data->scan_info;
  grn_accessor *a = (grn_accessor *)index;
  switch (a->action) {
  case GRN_ACCESSOR_GET_KEY :
    grn_table_select_index_report(ctx,
                                  "[term-extract][accessor][key]",
                                  a->obj);
    grn_table_search(ctx,
                     a->obj,
                     GRN_TEXT_VALUE(si->query),
                     GRN_TEXT_LEN(si->query),
                     GRN_OP_TERM_EXTRACT,
                     res,
                     logical_op);
    rc = GRN_SUCCESS;
    break;
  }

  return rc;
}

static grn_inline grn_rc
grn_table_select_index_call(grn_ctx *ctx,
                            grn_obj *index,
                            grn_operator op,
                            grn_obj *res,
                            grn_operator logical_op,
                            grn_table_select_data *data)
{
  scan_info *si = data->scan_info;
  grn_obj *selector = si->args[0];

  if (!grn_obj_is_selector_proc(ctx, selector)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;

  grn_operator selector_op = grn_proc_get_selector_operator(ctx, selector);

  grn_index_datum index_datum;
  unsigned int n_index_datum;
  if (grn_obj_is_index_column(ctx, index)) {
    index_datum.index = index;
    n_index_datum = 1;
  } else if (grn_obj_is_accessor(ctx, index)) {
    n_index_datum = grn_column_find_index_data(ctx,
                                               ((grn_accessor *)index)->obj,
                                               selector_op,
                                               &index_datum,
                                               1);
  } else {
    n_index_datum = grn_column_find_index_data(ctx,
                                               index,
                                               selector_op,
                                               &index_datum,
                                               1);
  }
  if (n_index_datum > 0) {
    if (grn_logger_pass(ctx, GRN_REPORT_INDEX_LOG_LEVEL)) {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      char tag[GRN_TABLE_MAX_KEY_SIZE];
      name_size = grn_obj_name(ctx, selector, name, GRN_TABLE_MAX_KEY_SIZE);
      grn_snprintf(tag, GRN_TABLE_MAX_KEY_SIZE, GRN_TABLE_MAX_KEY_SIZE,
                   "[selector][%.*s]",
                   name_size, name);
      grn_table_select_index_report(ctx, tag, index_datum.index);
    }

    grn_id range = grn_obj_get_range(ctx, index_datum.index);
    grn_obj *table = grn_ctx_at(ctx, range);
    rc = grn_selector_run(ctx,
                          selector,
                          data->expr,
                          table,
                          index_datum.index,
                          si->nargs,
                          si->args,
                          res,
                          logical_op);
    grn_obj_unref(ctx, table);
  }

  return rc;
}

static grn_inline grn_rc
grn_table_select_index_range_id(grn_ctx *ctx,
                                grn_obj *table,
                                grn_operator op,
                                grn_obj *res,
                                grn_operator logical_op,
                                grn_table_select_data *data)
{
  const char *tag = "[range][id]";

  if (grn_table_select_index_use_sequential_search(ctx,
                                                   res,
                                                   logical_op,
                                                   tag,
                                                   table)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = data->scan_info;
  grn_obj id;
  GRN_UINT32_INIT(&id, 0);
  if (grn_obj_cast(ctx, si->query, &id, GRN_FALSE) == GRN_SUCCESS) {
    grn_table_cursor *cursor;
    grn_id min = GRN_ID_NIL;
    grn_id max = GRN_ID_MAX;
    int offset = 0;
    int limit = -1;
    int flags = GRN_CURSOR_BY_ID | GRN_CURSOR_ASCENDING;

    grn_table_select_index_report(ctx, tag, table);

    switch (op) {
    case GRN_OP_LESS :
      max = GRN_UINT32_VALUE(&id);
      if (max > GRN_ID_NIL) {
        max--;
      }
      break;
    case GRN_OP_GREATER :
      min = GRN_UINT32_VALUE(&id) + 1;
      break;
    case GRN_OP_LESS_EQUAL :
      max = GRN_UINT32_VALUE(&id);
      break;
    case GRN_OP_GREATER_EQUAL :
      min = GRN_UINT32_VALUE(&id);
      break;
    default :
      break;
    }
    cursor = grn_table_cursor_open(ctx, table,
                                   NULL, 0,
                                   NULL, 0,
                                   offset, limit, flags);
    if (cursor) {
      uint32_t sid;
      int32_t weight;

      sid = GRN_UINT32_VALUE_AT(&(si->wv), 0);
      weight = GRN_INT32_VALUE_AT(&(si->wv), 1);

      if (sid == 0) {
        grn_posting_internal posting = {0};

        posting.weight_float = weight;
        while ((posting.rid = grn_table_cursor_next(ctx, cursor))) {
          if (posting.rid < min) {
            continue;
          }
          if (posting.rid > max) {
            break;
          }
          grn_ii_posting_add_float(ctx,
                                   (grn_posting *)(&posting),
                                   (grn_hash *)res,
                                   logical_op);
        }
      }
      rc = GRN_SUCCESS;
      grn_table_cursor_close(ctx, cursor);
    }

    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, logical_op);
  }
  GRN_OBJ_FIN(ctx, &id);

  return rc;
}

static grn_inline grn_rc
grn_table_select_index_range_key(grn_ctx *ctx,
                                 grn_obj *table,
                                 grn_operator op,
                                 grn_obj *res,
                                 grn_operator logical_op,
                                 grn_table_select_data *data)
{
  const char *tag = "[range][key]";

  if (grn_table_select_index_use_sequential_search(ctx,
                                                   res,
                                                   logical_op,
                                                   tag,
                                                   table)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = data->scan_info;
  grn_obj key;
  GRN_OBJ_INIT(&key, GRN_BULK, 0, table->header.domain);
  if (grn_obj_cast(ctx, si->query, &key, GRN_FALSE) == GRN_SUCCESS) {
    grn_table_cursor *cursor;
    const void *min = NULL, *max = NULL;
    unsigned int min_size = 0, max_size = 0;
    int offset = 0;
    int limit = -1;
    int flags = GRN_CURSOR_ASCENDING;

    grn_table_select_index_report(ctx, tag, table);

    switch (op) {
    case GRN_OP_LESS :
      flags |= GRN_CURSOR_LT;
      max = GRN_BULK_HEAD(&key);
      max_size = GRN_BULK_VSIZE(&key);
      break;
    case GRN_OP_GREATER :
      flags |= GRN_CURSOR_GT;
      min = GRN_BULK_HEAD(&key);
      min_size = GRN_BULK_VSIZE(&key);
      break;
    case GRN_OP_LESS_EQUAL :
      flags |= GRN_CURSOR_LE;
      max = GRN_BULK_HEAD(&key);
      max_size = GRN_BULK_VSIZE(&key);
      break;
    case GRN_OP_GREATER_EQUAL :
      flags |= GRN_CURSOR_GE;
      min = GRN_BULK_HEAD(&key);
      min_size = GRN_BULK_VSIZE(&key);
      break;
    default :
      break;
    }
    cursor = grn_table_cursor_open(ctx, table,
                                   min, min_size, max, max_size,
                                   offset, limit, flags);
    if (cursor) {
      uint32_t sid;
      int32_t weight;

      sid = GRN_UINT32_VALUE_AT(&(si->wv), 0);
      weight = GRN_INT32_VALUE_AT(&(si->wv), 1);

      if (sid == 0) {
        grn_posting_internal posting = {0};

        posting.weight_float = weight;
        while ((posting.rid = grn_table_cursor_next(ctx, cursor))) {
          grn_ii_posting_add_float(ctx,
                                   (grn_posting *)(&posting),
                                   (grn_hash *)res,
                                   logical_op);
        }
      }
      rc = GRN_SUCCESS;
      grn_table_cursor_close(ctx, cursor);
    }

    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, logical_op);
  }
  GRN_OBJ_FIN(ctx, &key);

  return rc;
}

static grn_inline grn_rc
grn_table_select_index_range_column(grn_ctx *ctx,
                                    grn_obj *index,
                                    grn_operator op,
                                    grn_obj *res,
                                    grn_operator logical_op,
                                    grn_table_select_data *data)
{
  const char *tag = "[range]";

  grn_obj *lexicon = grn_ctx_at(ctx, index->header.domain);
  if (!lexicon) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  if (grn_table_select_index_use_sequential_search(ctx,
                                                   res,
                                                   logical_op,
                                                   tag,
                                                   lexicon)) {
    return GRN_FUNCTION_NOT_IMPLEMENTED;
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  scan_info *si = data->scan_info;
  grn_obj range;
  GRN_OBJ_INIT(&range, GRN_BULK, 0, lexicon->header.domain);
  if (grn_obj_cast(ctx, si->query, &range, GRN_FALSE) == GRN_SUCCESS) {
    grn_table_cursor *cursor;
    const void *min = NULL, *max = NULL;
    unsigned int min_size = 0, max_size = 0;
    int offset = 0;
    int limit = -1;
    int flags = GRN_CURSOR_ASCENDING;

    grn_table_select_index_report(ctx, "[range]", index);

    switch (op) {
    case GRN_OP_LESS :
      flags |= GRN_CURSOR_LT;
      max = GRN_BULK_HEAD(&range);
      max_size = GRN_BULK_VSIZE(&range);
      break;
    case GRN_OP_GREATER :
      flags |= GRN_CURSOR_GT;
      min = GRN_BULK_HEAD(&range);
      min_size = GRN_BULK_VSIZE(&range);
      break;
    case GRN_OP_LESS_EQUAL :
      flags |= GRN_CURSOR_LE;
      max = GRN_BULK_HEAD(&range);
      max_size = GRN_BULK_VSIZE(&range);
      break;
    case GRN_OP_GREATER_EQUAL :
      flags |= GRN_CURSOR_GE;
      min = GRN_BULK_HEAD(&range);
      min_size = GRN_BULK_VSIZE(&range);
      break;
    default :
      break;
    }
    cursor = grn_table_cursor_open(ctx, lexicon,
                                   min, min_size, max, max_size,
                                   offset, limit, flags);
    if (cursor) {
      grn_id tid;
      uint32_t sid;
      int32_t weight;
      grn_ii *ii = (grn_ii *)index;

      sid = GRN_UINT32_VALUE_AT(&(si->wv), 0);
      weight = GRN_INT32_VALUE_AT(&(si->wv), 1);
      while ((tid = grn_table_cursor_next(ctx, cursor)) != GRN_ID_NIL) {
        grn_ii_cursor *ii_cursor;

        ii_cursor = grn_ii_cursor_open(ctx, ii, tid,
                                       GRN_ID_NIL, GRN_ID_MAX,
                                       ii->n_elements, 0);
        if (ii_cursor) {
          grn_posting *posting;
          while ((posting = grn_ii_cursor_next(ctx, ii_cursor))) {
            grn_posting_internal new_posting;

            if (!(sid == 0 || posting->sid == sid)) {
              continue;
            }

            if (si->position.specified) {
              while ((posting = grn_ii_cursor_next_pos(ctx, ii_cursor))) {
                if (posting->pos == si->position.start) {
                  break;
                }
              }
              if (!posting) {
                continue;
              }
            }

            new_posting = *((grn_posting_internal *)posting);
            new_posting.weight_float += 1;
            new_posting.weight_float *= weight;
            grn_ii_posting_add_float(ctx,
                                     (grn_posting *)(&new_posting),
                                     (grn_hash *)res,
                                     logical_op);
          }
        }
        grn_ii_cursor_close(ctx, ii_cursor);
      }
      rc = GRN_SUCCESS;
      grn_table_cursor_close(ctx, cursor);
    }

    grn_ii_resolve_sel_and(ctx, (grn_hash *)res, logical_op);
  }
  GRN_OBJ_FIN(ctx, &range);

  return rc;
}

static grn_inline grn_rc
grn_table_select_index_range(grn_ctx *ctx,
                             grn_obj *index,
                             grn_operator op,
                             grn_obj *res,
                             grn_operator logical_op,
                             grn_table_select_data *data)
{
  grn_obj *target = index;

  if (grn_obj_is_accessor(ctx, index)) {
    target = ((grn_accessor *)index)->obj;
    if (grn_obj_is_id_accessor(ctx, index)) {
      return grn_table_select_index_range_id(ctx,
                                             target,
                                             op,
                                             res,
                                             logical_op,
                                             data);
    }
  }

  switch (target->header.type) {
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_DAT_KEY :
    /* target == table */
    return grn_table_select_index_range_key(ctx,
                                            target,
                                            op,
                                            res,
                                            logical_op,
                                            data);
  default :
    return grn_table_select_index_range_column(ctx,
                                               target,
                                               op,
                                               res,
                                               logical_op,
                                               data);
  }
}

static grn_rc
grn_table_select_index_dispatch(grn_ctx *ctx,
                                grn_obj *index,
                                grn_operator op,
                                grn_obj *res,
                                grn_operator logical_op,
                                void *user_data)
{
  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  grn_table_select_data *data = user_data;

  switch (op) {
  case GRN_OP_EQUAL :
    rc = grn_table_select_index_equal(ctx,
                                      index,
                                      op,
                                      res,
                                      logical_op,
                                      data);
    break;
  case GRN_OP_NOT_EQUAL :
    rc = grn_table_select_index_not_equal(ctx,
                                          index,
                                          op,
                                          res,
                                          logical_op,
                                          data);
    break;
  case GRN_OP_MATCH :
  case GRN_OP_NEAR :
  case GRN_OP_NEAR2 :
  case GRN_OP_NEAR_PHRASE :
  case GRN_OP_SIMILAR :
  case GRN_OP_REGEXP :
  case GRN_OP_QUORUM :
    rc = grn_table_select_index_match(ctx,
                                      index,
                                      op,
                                      res,
                                      logical_op,
                                      data);
    break;
  case GRN_OP_PREFIX :
  case GRN_OP_SUFFIX :
    rc = grn_table_select_index_fix(ctx,
                                    index,
                                    op,
                                    res,
                                    logical_op,
                                    data);
    break;
  case GRN_OP_TERM_EXTRACT :
    rc = grn_table_select_index_extract(ctx,
                                        index,
                                        op,
                                        res,
                                        logical_op,
                                        data);
    break;
  case GRN_OP_CALL :
    rc = grn_table_select_index_call(ctx,
                                     index,
                                     op,
                                     res,
                                     logical_op,
                                     data);
    break;
  case GRN_OP_LESS :
  case GRN_OP_GREATER :
  case GRN_OP_LESS_EQUAL :
  case GRN_OP_GREATER_EQUAL :
    rc = grn_table_select_index_range(ctx,
                                      index,
                                      op,
                                      res,
                                      logical_op,
                                      data);
    break;
  default :
    /* todo : implement */
    /* todo : handle SCAN_PRE_CONST */
    break;
  }

  return rc;
}

static grn_inline bool
grn_table_select_index(grn_ctx *ctx,
                       grn_obj *table,
                       grn_table_select_data *data)
{
  scan_info *si = data->scan_info;

  if (!si->query) {
    if (si->op != GRN_OP_CALL || !grn_obj_is_selector_proc(ctx, si->args[0])) {
      return false;
    }
  }

  grn_rc rc = GRN_FUNCTION_NOT_IMPLEMENTED;
  size_t n_indexes = GRN_PTR_VECTOR_SIZE(&(si->index));
  if (n_indexes > 0) {
    unsigned int previous_n_hits = grn_table_size(ctx, data->res);

    grn_id minimum_min_id = GRN_ID_NIL;
    bool minimum_min_id_is_set = false;

    grn_obj weights;
    GRN_INT32_INIT(&weights, GRN_OBJ_VECTOR);

    int original_flags = ctx->flags;
    ctx->flags |= GRN_CTX_TEMPORARY_DISABLE_II_RESOLVE_SEL_AND;
    size_t i;
    for (i = 0; i < n_indexes; i++) {
      grn_obj *index = GRN_PTR_VALUE_AT(&(si->index), i);

      grn_search_optarg *options = &(data->current.search_options);
      memset(options, 0, sizeof(*options));
      if (si->op == GRN_OP_MATCH) {
        options->mode = GRN_OP_EXACT;
      } else {
        options->mode = si->op;
      }
      options->match_info.flags = GRN_MATCH_INFO_GET_MIN_RECORD_ID;
      options->max_interval = si->max_interval;
      options->similarity_threshold = si->similarity_threshold;
      options->quorum_threshold = si->quorum_threshold;

      uint32_t section = GRN_UINT32_VALUE_AT(&(si->wv), i * 2);
      int32_t weight = GRN_INT32_VALUE_AT(&(si->wv), (i * 2) + 1);
      if (section > 0) {
        int weight_index = section - 1;
        int current_vector_size;
        current_vector_size = GRN_INT32_VECTOR_SIZE(&weights);
        if (weight_index < current_vector_size) {
          ((int32_t *)(GRN_BULK_HEAD(&weights)))[weight_index] = weight;
        } else {
          GRN_INT32_SET_AT(ctx, &weights, weight_index, weight);
        }
        options->weight_vector = &GRN_INT32_VALUE(&weights);
        options->vector_size = GRN_INT32_VECTOR_SIZE(&weights);
      } else {
        options->weight_vector = NULL;
        options->vector_size = weight;
      }
      options->scorer = GRN_PTR_VALUE_AT(&(si->scorers), i);
      options->scorer_args_expr =
        GRN_PTR_VALUE_AT(&(si->scorer_args_exprs), i);
      options->scorer_args_expr_offset =
        GRN_UINT32_VALUE_AT(&(si->scorer_args_expr_offsets), i);
      grn_bool use_and_min_skip_enable;
      use_and_min_skip_enable =
        (grn_table_select_and_min_skip_enable && !GRN_ACCESSORP(index));
      if (use_and_min_skip_enable) {
        options->match_info.min = data->min_id;
      } else {
        options->match_info.min = GRN_ID_NIL;
      }
      if (n_indexes == 1 && grn_obj_is_accessor(ctx, index)) {
        rc = grn_accessor_execute(ctx,
                                  index,
                                  grn_table_select_index_dispatch,
                                  data,
                                  si->op,
                                  data->res,
                                  si->logical_op);
      } else {
        if (i < n_indexes - 1) {
          if (section > 0 &&
              index == GRN_PTR_VALUE_AT(&(si->index), i + 1) &&
              !options->scorer &&
              !GRN_PTR_VALUE_AT(&(si->scorers), i + 1)) {
            continue;
          }
        }
        rc = grn_table_select_index_dispatch(ctx,
                                             index,
                                             si->op,
                                             data->res,
                                             si->logical_op,
                                             data);
      }
      if (rc != GRN_SUCCESS) {
        break;
      }
      if (options->weight_vector) {
        int i;
        for (i = 0; i < options->vector_size; i++) {
          options->weight_vector[i] = 0;
        }
      }
      GRN_BULK_REWIND(&weights);
      if (use_and_min_skip_enable &&
          (!minimum_min_id_is_set ||
           options->match_info.min < minimum_min_id)) {
        minimum_min_id_is_set = true;
        minimum_min_id = options->match_info.min;
      }
      if (options->match_info.flags & GRN_MATCH_INFO_ONLY_SKIP_TOKEN) {
        data->current.is_skipped = true;
      }
    }
    ctx->flags = original_flags;

    GRN_OBJ_FIN(ctx, &weights);

    if (rc == GRN_SUCCESS) {
      bool need_resolve_sel_and = !data->current.is_skipped;
      if (si->op == GRN_OP_NOT_EQUAL) {
        need_resolve_sel_and = false;
      }
      if (need_resolve_sel_and) {
        grn_ii_resolve_sel_and(ctx, (grn_hash *)(data->res), si->logical_op);
      }
      if ((si->logical_op == GRN_OP_AND) ||
          (si->logical_op == GRN_OP_OR && previous_n_hits == 0)) {
        data->min_id = minimum_min_id;
      } else {
        data->min_id = GRN_ID_NIL;
      }
    }
  } else {
    if (si->op == GRN_OP_CALL && grn_obj_is_selector_proc(ctx, si->args[0])) {
      grn_obj *selector = si->args[0];
      grn_proc *proc = (grn_proc *)(selector);
      grn_operator selector_op = grn_proc_get_selector_operator(ctx, selector);
      bool callable = false;
      if (selector_op == GRN_OP_NOP) {
        callable = true;
      } else if (si->nargs > 1) {
        grn_obj *first_arg = si->args[1];
        if (grn_obj_is_index_column(ctx, first_arg)) {
          callable = grn_index_column_is_usable(ctx, first_arg, selector_op);
        }
      }
      if (callable) {
        if (grn_logger_pass(ctx, GRN_REPORT_INDEX_LOG_LEVEL)) {
          char proc_name[GRN_TABLE_MAX_KEY_SIZE];
          int proc_name_size;
          char tag[GRN_TABLE_MAX_KEY_SIZE];
          proc_name_size = grn_obj_name(ctx, (grn_obj *)proc,
                                        proc_name, GRN_TABLE_MAX_KEY_SIZE);
          proc_name[proc_name_size] = '\0';
          grn_snprintf(tag, GRN_TABLE_MAX_KEY_SIZE, GRN_TABLE_MAX_KEY_SIZE,
                       "[selector][no-index][%s]", proc_name);
          grn_table_select_index_report(ctx, tag, table);
        }
        rc = grn_selector_run(ctx,
                              selector,
                              data->expr,
                              table,
                              NULL,
                              si->nargs,
                              si->args,
                              data->res,
                              si->logical_op);
      }
    }
  }

  switch (rc) {
  case GRN_SUCCESS :
    return true;
    break;
  case GRN_FUNCTION_NOT_IMPLEMENTED :
    ERRCLR(ctx);
    return false;
  default :
    /* TODO: report error */
    return false;
  }
}

static void
grn_table_select_inspect_condition_argument(grn_ctx *ctx,
                                            grn_obj *buffer,
                                            grn_obj *argument)
{
  grn_obj *domain;

  switch (argument->header.type) {
  case GRN_BULK :
    domain = grn_ctx_at(ctx, argument->header.domain);
    if (grn_obj_is_table(ctx, domain)) {
      grn_record_inspect_without_columns(ctx, buffer, argument);
    } else {
      grn_inspect(ctx, buffer, argument);
    }
    break;
  case GRN_UVECTOR :
    domain = grn_ctx_at(ctx, argument->header.domain);
    if (grn_obj_is_table(ctx, domain)) {
      grn_uvector_record_inspect_without_columns(ctx, buffer, argument);
    } else {
      grn_inspect(ctx, buffer, argument);
    }
    break;
  case GRN_TABLE_HASH_KEY :
  case GRN_TABLE_PAT_KEY :
  case GRN_TABLE_NO_KEY :
  case GRN_COLUMN_FIX_SIZE :
  case GRN_COLUMN_VAR_SIZE :
  case GRN_COLUMN_INDEX :
    grn_inspect_name(ctx, buffer, argument);
    break;
  default :
    grn_inspect(ctx, buffer, argument);
    break;
  }
}

static void
grn_table_select_inspect_condition_call(grn_ctx *ctx,
                                        grn_obj *buffer,
                                        grn_expr_code *codes,
                                        uint32_t start,
                                        uint32_t end)
{
  uint32_t i;

  for (i = start; i <= end; i++) {
    grn_expr_code *code = codes + i;
    if (i == start) {
      if (grn_obj_is_proc(ctx, code->value)) {
        grn_inspect_name(ctx, buffer, code->value);
      } else {
        grn_table_select_inspect_condition_argument(ctx, buffer, code->value);
      }
      GRN_TEXT_PUTC(ctx, buffer, '(');
    } else if (code->value) {
      if (i > start + 1) {
        GRN_TEXT_PUTS(ctx, buffer, ", ");
      }
      grn_table_select_inspect_condition_argument(ctx, buffer, code->value);
    }
  }
  GRN_TEXT_PUTC(ctx, buffer, ')');
}

static const char *
grn_table_select_inspect_condition(grn_ctx *ctx,
                                   grn_obj *buffer,
                                   scan_info *si,
                                   grn_expr *expr)
{
  uint32_t i;
  uint32_t n_codes;
  grn_operator last_operator;

  if (!grn_query_log_show_condition) {
    return "";
  }

  GRN_BULK_REWIND(buffer);

  GRN_TEXT_PUTS(ctx, buffer, ": ");

  if (si->flags & SCAN_POP) {
    GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(si->logical_op));
    GRN_TEXT_PUTC(ctx, buffer, '\0');
    return GRN_TEXT_VALUE(buffer);
  }

  n_codes = si->end - si->start + 1;
  last_operator = expr->codes[si->end].op;

  switch (last_operator) {
  case GRN_OP_CALL :
    grn_table_select_inspect_condition_call(ctx,
                                            buffer,
                                            expr->codes,
                                            si->start,
                                            si->end);
    break;
  case GRN_OP_EQUAL :
  case GRN_OP_NOT_EQUAL :
  case GRN_OP_LESS :
  case GRN_OP_GREATER :
  case GRN_OP_LESS_EQUAL :
  case GRN_OP_GREATER_EQUAL :
  case GRN_OP_MATCH :
    if (n_codes == 3) {
      grn_expr_code *arg1 = expr->codes + si->start;
      grn_expr_code *arg2 = expr->codes + si->start + 1;

      if (arg1->value->header.type == GRN_EXPR) {
        GRN_TEXT_PUTS(ctx, buffer, "(match columns)");
      } else {
        grn_table_select_inspect_condition_argument(ctx, buffer, arg1->value);
      }
      GRN_TEXT_PUTC(ctx, buffer, ' ');
      GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(last_operator));
      GRN_TEXT_PUTC(ctx, buffer, ' ');
      grn_table_select_inspect_condition_argument(ctx, buffer, arg2->value);
    } else {
      GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(last_operator));
      GRN_TEXT_PUTC(ctx, buffer, '(');
      for (i = si->start; i < si->end; i++) {
        grn_expr_code *code = expr->codes + i;
        if (i != si->start) {
          GRN_TEXT_PUTS(ctx, buffer, ", ");
        }
        if (code->modify > 0 && code[code->modify].op == GRN_OP_CALL) {
          grn_table_select_inspect_condition_call(ctx,
                                                  buffer,
                                                  code,
                                                  0,
                                                  code->modify);
          if (i + code->modify < si->end &&
              code[code->modify + 1].op == GRN_OP_PUSH) {
            i += code->modify;
          } else {
            i += code->modify - 1;
          }
        } else {
          if (code->value) {
            grn_table_select_inspect_condition_argument(ctx,
                                                        buffer,
                                                        code->value);
          } else {
            GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(code->op));
          }
        }
      }
      GRN_TEXT_PUTC(ctx, buffer, ')');
    }
    break;
  default :
    for (i = si->start; i <= si->end; i++) {
      grn_expr_code *code = expr->codes + i;
      if (i > si->start) {
        GRN_TEXT_PUTC(ctx, buffer, ' ');
      }
      if (code->value) {
        grn_table_select_inspect_condition_argument(ctx, buffer, code->value);
      } else {
        GRN_TEXT_PUTS(ctx, buffer, grn_operator_to_string(code->op));
      }
    }
    break;
  }

  GRN_TEXT_PUTC(ctx, buffer, '\0');
  return GRN_TEXT_VALUE(buffer);
}

grn_obj *
grn_table_select(grn_ctx *ctx, grn_obj *table, grn_obj *expr,
                 grn_obj *res, grn_operator op)
{
  grn_obj *v;
  unsigned int res_size;
  grn_bool res_created = GRN_FALSE;
  if (res) {
    if (res->header.type != GRN_TABLE_HASH_KEY ||
        (res->header.domain != DB_OBJ(table)->id)) {
      ERR(GRN_INVALID_ARGUMENT, "hash table required");
      return NULL;
    }
  } else {
    if (!(res = grn_table_create(ctx, NULL, 0, NULL,
                                 GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                 table, NULL))) {
      return NULL;
    }
    res_created = GRN_TRUE;
  }
  if (!(v = grn_expr_get_var_by_offset(ctx, expr, 0))) {
    ERR(GRN_INVALID_ARGUMENT, "at least one variable must be defined");
    return NULL;
  }
  GRN_API_ENTER;
  res_size = GRN_HASH_SIZE((grn_hash *)res);
  if (op == GRN_OP_OR || res_size) {
    grn_scanner *scanner;
    scanner = grn_scanner_open(ctx, expr, op, res_size > 0);
    if (scanner) {
      int i;
      grn_obj res_stack;
      grn_expr *e = (grn_expr *)scanner->expr;
      grn_expr_code *codes = e->codes;
      uint32_t codes_curr = e->codes_curr;
      grn_obj condition_inspect_buffer;
      grn_obj *base_res = NULL;
      grn_table_select_data data;

      GRN_PTR_INIT(&res_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
      GRN_TEXT_INIT(&condition_inspect_buffer, 0);

      data.expr = scanner->expr;
      data.variable = grn_expr_get_var_by_offset(ctx, (grn_obj *)e, 0);
      data.scanner = scanner;
      data.res = res;
      data.min_id = GRN_ID_NIL;
      data.current.is_skipped = false;
      data.is_first_unskipped_scan_info = GRN_TRUE;
      if (res_size > 0 && op == GRN_OP_AND) {
        grn_bool have_push = GRN_FALSE;
        for (i = 0; i < scanner->n_sis; i++) {
          scan_info *si = scanner->sis[i];
          if (si->flags & SCAN_PUSH) {
            have_push = GRN_TRUE;
            break;
          }
        }
        if (have_push) {
          base_res = grn_table_create(ctx,
                                      NULL, 0,
                                      NULL,
                                      GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                      table,
                                      NULL);
          if (base_res) {
            void *key = NULL, *value = NULL, *base_value = NULL;
            uint32_t key_size = 0;

            GRN_TABLE_EACH(ctx, data.res, 0, 0, id, &key, &key_size, &value, {
              if (grn_table_add_v(ctx, base_res, key, key_size, &base_value, NULL)) {
                grn_rset_recinfo *ri = value;
                grn_rset_recinfo *base_ri = base_value;
                grn_memcpy(base_ri, ri, ((grn_hash *)base_res)->value_size);
                base_ri->score = 0;
              }
            });
          }
        }
      }

      for (i = 0; i < scanner->n_sis; i++) {
        scan_info *si = scanner->sis[i];
        data.nth_scan_info = i;
        data.scan_info = si;
        if (i > 0 && data.is_first_unskipped_scan_info) {
          if (data.current.is_skipped) {
            if (si->logical_op == GRN_OP_AND) {
              si->logical_op = GRN_OP_OR;
            }
          } else {
            data.is_first_unskipped_scan_info = GRN_FALSE;
          }
        }
        data.current.is_skipped = GRN_FALSE;
        if (si->flags & SCAN_POP) {
          grn_obj *res_;
          GRN_PTR_POP(&res_stack, res_);
          grn_table_setoperation(ctx, res_, data.res, res_, si->logical_op);
          grn_obj_close(ctx, data.res);
          data.res = res_;
          data.min_id = GRN_ID_NIL;
        } else {
          grn_bool processed = GRN_FALSE;
          if (si->flags & SCAN_PUSH) {
            grn_obj *res_ = NULL;
            res_ = grn_table_create(ctx, NULL, 0, NULL,
                                    GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                                    table, NULL);
            if (!res_) {
              break;
            }
            if (base_res && si->logical_op == GRN_OP_OR) {
              GRN_LOG(ctx, GRN_REPORT_INDEX_LOG_LEVEL,
                      "[table][select][push][initial] <%u>",
                      grn_table_size(ctx, base_res));
              grn_table_setoperation(ctx, res_, base_res, res_, GRN_OP_OR);
              si->logical_op = GRN_OP_AND;
            }
            GRN_PTR_PUT(ctx, &res_stack, data.res);
            data.res = res_;
            data.min_id = GRN_ID_NIL;
          }
          if (si->logical_op != GRN_OP_AND) {
            data.min_id = GRN_ID_NIL;
          }
          processed = grn_table_select_index(ctx, table, &data);
          if (!processed) {
            if (ctx->rc) { break; }
            e->codes = codes + si->start;
            e->codes_curr = si->end - si->start + 1;
            grn_table_select_sequential(ctx,
                                        table,
                                        (grn_obj *)e,
                                        data.res,
                                        si->logical_op);
            e->codes = codes;
            e->codes_curr = codes_curr;
            data.min_id = GRN_ID_NIL;
          }
        }
        GRN_QUERY_LOG(ctx, GRN_QUERY_LOG_SIZE,
                      ":", "%sfilter(%d)%s",
                      grn_expr_get_query_log_tag_prefix(ctx, expr),
                      grn_table_size(ctx, data.res),
                      grn_table_select_inspect_condition(
                        ctx,
                        &condition_inspect_buffer,
                        si,
                        e));
        if (ctx->rc) {
          if (res_created) {
            grn_obj_close(ctx, data.res);
          }
          data.res = NULL;
          break;
        }
      }
      res = data.res;

      GRN_OBJ_FIN(ctx, &condition_inspect_buffer);
      if (base_res) {
        grn_obj_close(ctx, base_res);
      }

      i = 0;
      if (!res_created) { i++; }
      for (; i < GRN_BULK_VSIZE(&res_stack) / sizeof(grn_obj *); i++) {
        grn_obj *stacked_res;
        stacked_res = *((grn_obj **)GRN_BULK_HEAD(&res_stack) + i);
        grn_obj_close(ctx, stacked_res);
      }
      GRN_OBJ_FIN(ctx, &res_stack);
      e->codes = codes;
      e->codes_curr = codes_curr;

      grn_scanner_close(ctx, scanner);
    } else {
      if (!ctx->rc) {
        grn_table_select_sequential(ctx, table, expr, res, op);
        if (ctx->rc) {
          if (res_created) {
            grn_obj_close(ctx, res);
          }
          res = NULL;
        }
      }
    }
  }
  GRN_API_RETURN(res);
}

/* grn_expr_parse */

grn_obj *
grn_ptr_value_at(grn_obj *obj, int offset)
{
  int size = GRN_BULK_VSIZE(obj) / sizeof(grn_obj *);
  if (offset < 0) { offset = size + offset; }
  return (0 <= offset && offset < size)
    ? (((grn_obj **)GRN_BULK_HEAD(obj))[offset])
    : NULL;
}

int32_t
grn_int32_value_at(grn_obj *obj, int offset)
{
  int size = GRN_BULK_VSIZE(obj) / sizeof(int32_t);
  if (offset < 0) { offset = size + offset; }
  return (0 <= offset && offset < size)
    ? (((int32_t *)GRN_BULK_HEAD(obj))[offset])
    : 0;
}

/* grn_expr_create_from_str */

#include "grn_snip.h"

typedef struct {
  grn_ctx *ctx;
  grn_obj *e;
  grn_obj *v;
  const char *str;
  const char *cur;
  const char *str_end;
  grn_obj *table;
  grn_obj *default_column;
  grn_obj buf;
  grn_obj token_stack;
  grn_obj column_stack;
  grn_obj op_stack;
  grn_obj mode_stack;
  grn_obj max_interval_stack;
  grn_obj similarity_threshold_stack;
  grn_obj quorum_threshold_stack;
  grn_obj weight_stack;
  grn_operator default_op;
  grn_select_optarg opt;
  grn_operator default_mode;
  grn_expr_flags flags;
  grn_expr_flags default_flags;
  int escalation_threshold;
  int escalation_decaystep;
  int weight_offset;
  grn_hash *weight_set;
  snip_cond *snip_conds;
  grn_hash *object_literal;
  grn_obj *array_literal;
  int paren_depth;
  struct {
    const char *string;
    size_t string_length;
    int token;
    int weight;
  } pending_token;
} efs_info;

typedef struct {
  grn_operator op;
  int weight;
} efs_op;

grn_inline static void
skip_space(grn_ctx *ctx, efs_info *q)
{
  unsigned int len;
  while (q->cur < q->str_end && grn_isspace(q->cur, ctx->encoding)) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, q->cur, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    q->cur += len;
  }
}

static grn_bool
parse_query_op(efs_info *q, efs_op *op, grn_operator *mode, int *option)
{
  grn_bool found = GRN_TRUE;
  const char *start, *end = q->cur;
  switch (*end) {
  case 'S' :
    *mode = GRN_OP_SIMILAR;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_SIMILARITY_THRESHOLD; }
    q->cur = end;
    break;
  case 'N' :
    *mode = GRN_OP_NEAR;
    start = ++end;
    if (start < q->str_end && start[0] == 'P') {
      *mode = GRN_OP_NEAR_PHRASE;
      start++;
    }
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_MAX_INTERVAL; }
    q->cur = end;
    break;
  case 'n' :
    *mode = GRN_OP_NEAR2;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_MAX_INTERVAL; }
    q->cur = end;
    break;
  case 'T' :
    *mode = GRN_OP_TERM_EXTRACT;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_TERM_EXTRACT_POLICY; }
    q->cur = end;
    break;
  case 'X' : /* force exact mode */
    op->op = GRN_OP_AND;
    *mode = GRN_OP_EXACT;
    *option = 0;
    start = ++end;
    q->cur = end;
    break;
  case 'Q' :
    *mode = GRN_OP_QUORUM;
    start = ++end;
    *option = grn_atoi(start, q->str_end, (const char **)&end);
    if (start == end) { *option = DEFAULT_QUORUM_THRESHOLD; }
    q->cur = end;
    break;
  default :
    found = GRN_FALSE;
    break;
  }
  return found;
}

#define DISABLE_UNUSED_CODE 1
#ifndef DISABLE_UNUSED_CODE
static const char *
get_weight_vector(grn_ctx *ctx, efs_info *query, const char *source)
{
  const char *p;

  if (!query->opt.weight_vector &&
      !query->weight_set &&
      !(query->opt.weight_vector = GRN_CALLOC(sizeof(int) * DEFAULT_WEIGHT_VECTOR_SIZE))) {
    GRN_LOG(ctx, GRN_LOG_ALERT, "get_weight_vector malloc fail");
    return source;
  }
  for (p = source; p < query->str_end; ) {
    unsigned int key;
    int value;

    /* key, key is not zero */
    key = grn_atoui(p, query->str_end, &p);
    if (!key || key > GRN_ID_MAX) { break; }

    /* value */
    if (*p == ':') {
      p++;
      value = grn_atoi(p, query->str_end, &p);
    } else {
      value = 1;
    }

    if (query->weight_set) {
      int *pval;
      if (grn_hash_add(ctx, query->weight_set, &key, sizeof(unsigned int), (void **)&pval, NULL)) {
        *pval = value;
      }
    } else if (key < DEFAULT_WEIGHT_VECTOR_SIZE) {
      query->opt.weight_vector[key - 1] = value;
    } else {
      GRN_FREE(query->opt.weight_vector);
      query->opt.weight_vector = NULL;
      if (!(query->weight_set = grn_hash_create(ctx, NULL, sizeof(unsigned int), sizeof(int),
                                                0))) {
        return source;
      }
      p = source;           /* reparse */
      continue;
    }
    if (*p != ',') { break; }
    p++;
  }
  return p;
}

static void
get_pragma(grn_ctx *ctx, efs_info *q)
{
  const char *start, *end = q->cur;
  while (end < q->str_end && *end == GRN_QUERY_PREFIX) {
    if (++end >= q->str_end) { break; }
    switch (*end) {
    case 'E' :
      start = ++end;
      q->escalation_threshold = grn_atoi(start, q->str_end, (const char **)&end);
      while (end < q->str_end && (('0' <= *end && *end <= '9') || *end == '-')) { end++; }
      if (*end == ',') {
        start = ++end;
        q->escalation_decaystep = grn_atoi(start, q->str_end, (const char **)&end);
      }
      q->cur = end;
      break;
    case 'D' :
      start = ++end;
      while (end < q->str_end && *end != GRN_QUERY_PREFIX && !grn_isspace(end, ctx->encoding)) {
        end++;
      }
      if (end > start) {
        switch (*start) {
        case 'O' :
          q->default_op = GRN_OP_OR;
          break;
        case GRN_QUERY_AND :
          q->default_op = GRN_OP_AND;
          break;
        case GRN_QUERY_AND_NOT :
          q->default_op = GRN_OP_AND_NOT;
          break;
        case GRN_QUERY_ADJ_INC :
          q->default_op = GRN_OP_ADJUST;
          break;
        }
      }
      q->cur = end;
      break;
    case 'W' :
      start = ++end;
      end = (char *)get_weight_vector(ctx, q, start);
      q->cur = end;
      break;
    }
  }
}

static int
section_weight_cb(grn_ctx *ctx, grn_hash *r, const void *rid, int sid, void *arg)
{
  int *w;
  grn_hash *s = (grn_hash *)arg;
  if (s && grn_hash_get(ctx, s, &sid, sizeof(grn_id), (void **)&w)) {
    return *w;
  } else {
    return 0;
  }
}
#endif

#include "grn_ecmascript.h"
#include "grn_ecmascript.c"

static grn_rc
grn_expr_parser_open(grn_ctx *ctx)
{
  if (!ctx->impl->parser) {
    ctx->impl->parser = grn_expr_parserAlloc(malloc);
  }
  return ctx->rc;
}

#define PARSE(token) grn_expr_parser(ctx->impl->parser, (token), 0, q)

static void
parse_query_accept_string(grn_ctx *ctx, efs_info *efsi,
                          const char *str, unsigned int str_size)
{
  grn_obj *column, *token;
  grn_operator mode;
  int32_t weight;

  GRN_PTR_PUT(ctx, &efsi->token_stack,
              grn_expr_add_str(ctx, efsi->e, str, str_size));
  {
    efs_info *q = efsi;
    PARSE(GRN_EXPR_TOKEN_QSTRING);
  }

  GRN_PTR_POP(&efsi->token_stack, token);
  column = grn_ptr_value_at(&efsi->column_stack, -1);
  grn_expr_append_const(efsi->ctx, efsi->e, column, GRN_OP_GET_VALUE, 1);
  grn_expr_append_obj(efsi->ctx, efsi->e, token, GRN_OP_PUSH, 1);

  mode = grn_int32_value_at(&efsi->mode_stack, -1);
  weight = grn_int32_value_at(&efsi->weight_stack, -1);
  switch (mode) {
  case GRN_OP_ASSIGN :
    grn_expr_append_op(efsi->ctx, efsi->e, mode, 2);
    break;
  case GRN_OP_NEAR :
  case GRN_OP_NEAR2 :
  case GRN_OP_NEAR_PHRASE :
    {
      int max_interval;
      max_interval = grn_int32_value_at(&efsi->max_interval_stack, -1);
      grn_expr_append_const_int(efsi->ctx, efsi->e, max_interval,
                                GRN_OP_PUSH, 1);
      if (weight == 0) {
        grn_expr_append_op(efsi->ctx, efsi->e, mode, 3);
      } else {
        grn_expr_append_const_int(efsi->ctx, efsi->e, weight, mode, 3);
      }
    }
    break;
  case GRN_OP_SIMILAR :
    {
      int similarity_threshold;
      similarity_threshold =
        grn_int32_value_at(&efsi->similarity_threshold_stack, -1);
      grn_expr_append_const_int(efsi->ctx, efsi->e, similarity_threshold,
                                GRN_OP_PUSH, 1);
      if (weight == 0) {
        grn_expr_append_op(efsi->ctx, efsi->e, mode, 3);
      } else {
        grn_expr_append_const_int(efsi->ctx, efsi->e, weight, mode, 3);
      }
    }
    break;
  case GRN_OP_QUORUM :
    {
      int quorum_threshold;
      quorum_threshold =
        grn_int32_value_at(&efsi->quorum_threshold_stack, -1);
      grn_expr_append_const_int(efsi->ctx, efsi->e, quorum_threshold,
                                GRN_OP_PUSH, 1);
      if (weight == 0) {
        grn_expr_append_op(efsi->ctx, efsi->e, mode, 3);
      } else {
        grn_expr_append_const_int(efsi->ctx, efsi->e, weight, mode, 3);
      }
    }
    break;
  default :
    if (weight == 0) {
      grn_expr_append_op(efsi->ctx, efsi->e, mode, 2);
    } else {
      grn_expr_append_const_int(efsi->ctx, efsi->e, weight, mode, 2);
    }
    break;
  }
}

static void
parse_query_flush_pending_token(grn_ctx *ctx, efs_info *q)
{
  const char *cur_keep;

  if (!(q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR)) {
    return;
  }

  if (q->pending_token.string_length == 0) {
    return;
  }

  cur_keep = q->cur;
  q->cur = q->pending_token.string;
  if (q->pending_token.token == GRN_EXPR_TOKEN_ADJUST ||
      q->pending_token.token == GRN_EXPR_TOKEN_NEGATIVE) {
    GRN_INT32_PUT(ctx, &q->weight_stack, q->pending_token.weight);
  }
  PARSE(q->pending_token.token);
  q->cur = cur_keep;

  q->pending_token.string = NULL;
  q->pending_token.string_length = 0;
  q->pending_token.token = 0;
  q->pending_token.weight = 0;
}

static void
parse_query_accept_logical_op(grn_ctx *ctx,
                              efs_info *q,
                              const char *string,
                              unsigned int string_length,
                              int token)
{
  if (!(q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR)) {
    PARSE(token);
    return;
  }

  if (q->pending_token.string_length > 0) {
    parse_query_accept_string(ctx,
                              q,
                              q->pending_token.string,
                              q->pending_token.string_length);
  }

  q->pending_token.string = string;
  q->pending_token.string_length = string_length;
  q->pending_token.token = token;
}

static void
parse_query_accept_adjust(grn_ctx *ctx,
                          efs_info *q,
                          const char *string,
                          unsigned int string_length,
                          int token,
                          int weight)
{
  if (!(q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR)) {
    GRN_INT32_PUT(ctx, &q->weight_stack, weight);
    PARSE(token);
    return;
  }

  if (q->pending_token.string_length > 0) {
    parse_query_accept_string(ctx,
                              q,
                              q->pending_token.string,
                              q->pending_token.string_length);
  }

  q->pending_token.string = string;
  q->pending_token.string_length = string_length;
  q->pending_token.token = token;
  q->pending_token.weight = weight;
}

static grn_rc
parse_query_word(grn_ctx *ctx, efs_info *q)
{
  const char *end;
  unsigned int len;
  GRN_BULK_REWIND(&q->buf);
  for (end = q->cur;; ) {
    /* null check and length check */
    if (!(len = grn_charlen(ctx, end, q->str_end))) {
      q->cur = q->str_end;
      break;
    }
    if (grn_isspace(end, ctx->encoding) ||
        *end == GRN_QUERY_PARENL || *end == GRN_QUERY_PARENR) {
      q->cur = end;
      break;
    }
    if (q->flags & GRN_EXPR_ALLOW_COLUMN && *end == GRN_QUERY_COLUMN) {
      grn_operator mode;
      grn_obj *c = grn_obj_column(ctx, q->table,
                                  GRN_TEXT_VALUE(&q->buf),
                                  GRN_TEXT_LEN(&q->buf));
      if (c && end + 1 < q->str_end) {
        switch (end[1]) {
        case '!' :
          mode = GRN_OP_NOT_EQUAL;
          q->cur = end + 2;
          break;
        case '=' :
          if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
            mode = GRN_OP_ASSIGN;
            q->cur = end + 2;
          } else {
            mode = GRN_OP_EQUAL;
            q->cur = end + 1;
          }
          break;
        case '<' :
          if (end + 2 < q->str_end && end[2] == '=') {
            mode = GRN_OP_LESS_EQUAL;
            q->cur = end + 3;
          } else {
            mode = GRN_OP_LESS;
            q->cur = end + 2;
          }
          break;
        case '>' :
          if (end + 2 < q->str_end && end[2] == '=') {
            mode = GRN_OP_GREATER_EQUAL;
            q->cur = end + 3;
          } else {
            mode = GRN_OP_GREATER;
            q->cur = end + 2;
          }
          break;
        case '@' :
          mode = GRN_OP_MATCH;
          q->cur = end + 2;
          break;
        case '^' :
          mode = GRN_OP_PREFIX;
          q->cur = end + 2;
          break;
        case '$' :
          mode = GRN_OP_SUFFIX;
          q->cur = end + 2;
          break;
        case '~' :
          mode = GRN_OP_REGEXP;
          q->cur = end + 2;
          break;
        default :
          mode = GRN_OP_EQUAL;
          q->cur = end + 1;
          break;
        }
      } else if (q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR) {
        GRN_TEXT_PUT(ctx, &q->buf, end, len);
        end += len;
        continue;
      } else {
        ERR(GRN_INVALID_ARGUMENT, "column lookup failed");
        q->cur = q->str_end;
        return ctx->rc;
      }
      parse_query_flush_pending_token(ctx, q);
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      PARSE(GRN_EXPR_TOKEN_RELATIVE_OP);

      grn_expr_take_obj(ctx, q->e, c);
      GRN_PTR_PUT(ctx, &q->column_stack, c);
      GRN_INT32_PUT(ctx, &q->mode_stack, mode);

      return GRN_SUCCESS;
    } else if (GRN_TEXT_LEN(&q->buf) > 0 && *end == GRN_QUERY_PREFIX) {
      q->cur = end + 1;
      GRN_INT32_PUT(ctx, &q->mode_stack, GRN_OP_PREFIX);
      break;
    } else if (*end == GRN_QUERY_ESCAPE) {
      end += len;
      if (!(len = grn_charlen(ctx, end, q->str_end))) {
        q->cur = q->str_end;
        break;
      }
    }
    GRN_TEXT_PUT(ctx, &q->buf, end, len);
    end += len;
  }
  parse_query_flush_pending_token(ctx, q);
  parse_query_accept_string(ctx,
                            q,
                            GRN_TEXT_VALUE(&q->buf),
                            GRN_TEXT_LEN(&q->buf));

  return GRN_SUCCESS;
}

static grn_rc
parse_query(grn_ctx *ctx, efs_info *q)
{
  int option = 0;
  grn_operator mode;
  efs_op op_, *op = &op_;
  grn_bool first_token = GRN_TRUE;
  grn_bool only_first_and = GRN_FALSE;
  grn_bool block_started = GRN_FALSE;

  op->op = q->default_op;
  op->weight = DEFAULT_WEIGHT;
  while (!ctx->rc) {
    skip_space(ctx, q);

    if (q->cur >= q->str_end) { goto exit; }
    if (*q->cur == '\0') { goto exit; }

    switch (*q->cur) {
    case GRN_QUERY_PARENR :
      if (q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR) {
        if (q->paren_depth == 0) {
          const char parenr = GRN_QUERY_PARENR;
          parse_query_flush_pending_token(ctx, q);
          parse_query_accept_string(ctx, q, &parenr, 1);
          q->cur++;
          break;
        }
        if (first_token) {
          const char parenl = GRN_QUERY_PARENL;
          const char parenr = GRN_QUERY_PARENR;
          parse_query_flush_pending_token(ctx, q);
          parse_query_accept_string(ctx, q, &parenl, 1);
          parse_query_accept_string(ctx, q, &parenr, 1);
        }
        if (only_first_and) {
          const char query_and[] = {GRN_QUERY_AND};
          parse_query_flush_pending_token(ctx, q);
          parse_query_accept_string(ctx,
                                    q,
                                    query_and,
                                    1);
        }
      }
      parse_query_flush_pending_token(ctx, q);
      PARSE(GRN_EXPR_TOKEN_PARENR);
      q->paren_depth--;
      q->cur++;
      break;
    case GRN_QUERY_QUOTEL :
      q->cur++;

      {
        grn_bool closed = GRN_FALSE;
        const char *start, *s;
        start = s = q->cur;
        GRN_BULK_REWIND(&q->buf);
        while (1) {
          unsigned int len;
          if (s >= q->str_end) {
            q->cur = s;
            break;
          }
          len = grn_charlen(ctx, s, q->str_end);
          if (len == 0) {
            /* invalid string containing malformed multibyte char */
            goto exit;
          } else if (len == 1) {
            if (*s == GRN_QUERY_QUOTER) {
              q->cur = s + 1;
              closed = GRN_TRUE;
              break;
            } else if (*s == GRN_QUERY_ESCAPE && s + 1 < q->str_end) {
              s++;
              len = grn_charlen(ctx, s, q->str_end);
            }
          }
          GRN_TEXT_PUT(ctx, &q->buf, s, len);
          s += len;
        }
        if (!closed && (q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR)) {
          q->cur = start - 1;
          parse_query_word(ctx, q);
        } else {
          parse_query_flush_pending_token(ctx, q);
          parse_query_accept_string(ctx,
                                    q,
                                    GRN_TEXT_VALUE(&q->buf),
                                    GRN_TEXT_LEN(&q->buf));
        }
      }

      break;
    case GRN_QUERY_PREFIX :
      q->cur++;
      if (parse_query_op(q, op, &mode, &option)) {
        switch (mode) {
        case GRN_OP_NEAR :
        case GRN_OP_NEAR2 :
        case GRN_OP_NEAR_PHRASE :
          GRN_INT32_PUT(ctx, &q->max_interval_stack, option);
          break;
        case GRN_OP_SIMILAR :
          GRN_INT32_PUT(ctx, &q->similarity_threshold_stack, option);
          break;
        case GRN_OP_QUORUM :
          GRN_INT32_PUT(ctx, &q->quorum_threshold_stack, option);
          break;
        default :
          break;
        }
        GRN_INT32_PUT(ctx, &q->mode_stack, mode);
        parse_query_flush_pending_token(ctx, q);
        PARSE(GRN_EXPR_TOKEN_RELATIVE_OP);
      } else {
        q->cur--;
        parse_query_word(ctx, q);
      }
      break;
    case GRN_QUERY_AND :
      if (first_token) {
        only_first_and = GRN_TRUE;
      } else {
        op->op = GRN_OP_AND;
        parse_query_accept_logical_op(ctx,
                                      q,
                                      q->cur, 1,
                                      GRN_EXPR_TOKEN_LOGICAL_AND);
      }
      q->cur++;
      break;
    case GRN_QUERY_AND_NOT :
      if (first_token) {
        if (q->flags & GRN_EXPR_ALLOW_LEADING_NOT) {
          grn_obj *all_records = grn_ctx_get(ctx, "all_records", 11);
          if (all_records) {
            /* dummy token */
            PARSE(GRN_EXPR_TOKEN_QSTRING);
            grn_expr_append_obj(ctx, q->e, all_records, GRN_OP_PUSH, 1);
            grn_expr_append_op(ctx, q->e, GRN_OP_CALL, 0);
          }
        } else if (q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR) {
          parse_query_flush_pending_token(ctx, q);
          parse_query_accept_string(ctx, q, q->cur, 1);
          q->cur++;
          break;
        }
      }
      op->op = GRN_OP_AND_NOT;
      parse_query_accept_logical_op(ctx,
                                    q,
                                    q->cur, 1,
                                    GRN_EXPR_TOKEN_LOGICAL_AND_NOT);
      q->cur++;
      break;
    case GRN_QUERY_ADJ_INC :
      if (op->weight < 127) { op->weight++; }
      op->op = GRN_OP_ADJUST;
      parse_query_accept_adjust(ctx,
                                q,
                                q->cur, 1,
                                GRN_EXPR_TOKEN_ADJUST,
                                op->weight);
      q->cur++;
      break;
    case GRN_QUERY_ADJ_DEC :
      if (op->weight > -128) { op->weight--; }
      op->op = GRN_OP_ADJUST;
      parse_query_accept_adjust(ctx,
                                q,
                                q->cur, 1,
                                GRN_EXPR_TOKEN_ADJUST,
                                op->weight);
      q->cur++;
      break;
    case GRN_QUERY_ADJ_NEG :
      if (first_token) {
        parse_query_flush_pending_token(ctx, q);
        parse_query_accept_string(ctx, q, q->cur, 1);
      } else {
        op->op = GRN_OP_ADJUST;
        parse_query_accept_adjust(ctx,
                                  q,
                                  q->cur, 1,
                                  GRN_EXPR_TOKEN_NEGATIVE,
                                  -DEFAULT_WEIGHT);
      }
      q->cur++;
      break;
    case GRN_QUERY_PARENL :
      parse_query_flush_pending_token(ctx, q);
      PARSE(GRN_EXPR_TOKEN_PARENL);
      q->cur++;
      q->paren_depth++;
      block_started = GRN_TRUE;
      break;
    case 'O' :
      if (q->cur + 2 < q->str_end && q->cur[1] == 'R' && q->cur[2] == ' ') {
        if (first_token && (q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR)) {
          parse_query_flush_pending_token(ctx, q);
          parse_query_accept_string(ctx, q, q->cur, 2);
        } else {
          parse_query_accept_logical_op(ctx,
                                        q,
                                        q->cur, 2,
                                        GRN_EXPR_TOKEN_LOGICAL_OR);
        }
        q->cur += 2;
        break;
      }
      /* fallthru */
    default :
      parse_query_word(ctx, q);
      break;
    }
    if (!first_token) {
      only_first_and = GRN_FALSE;
    }
    first_token = block_started;
    block_started = GRN_FALSE;
  }
exit :
  if (q->flags & GRN_EXPR_QUERY_NO_SYNTAX_ERROR) {
    if (q->pending_token.string_length > 0) {
      parse_query_accept_string(ctx,
                                q,
                                q->pending_token.string,
                                q->pending_token.string_length);
    } else if (only_first_and) {
      const char query_and[] = {GRN_QUERY_AND};
      parse_query_accept_string(ctx,
                                q,
                                query_and,
                                1);
    }
    if (q->paren_depth > 0) {
      int paren_depth = q->paren_depth;
      while (paren_depth > 0) {
        const char parenl = GRN_QUERY_PARENL;
        parse_query_accept_string(ctx, q, &parenl, 1);
        PARSE(GRN_EXPR_TOKEN_PARENR);
        paren_depth--;
      }
    }
  }
  PARSE(0);
  return GRN_SUCCESS;
}

static grn_rc
get_string(grn_ctx *ctx, efs_info *q, char quote)
{
  const char *s;
  unsigned int len;
  grn_rc rc = GRN_END_OF_DATA;
  GRN_BULK_REWIND(&q->buf);
  for (s = q->cur + 1; s < q->str_end; s += len) {
    if (!(len = grn_charlen(ctx, s, q->str_end))) { break; }
    if (len == 1) {
      if (*s == quote) {
        s++;
        rc = GRN_SUCCESS;
        break;
      }
      if (*s == GRN_QUERY_ESCAPE && s + 1 < q->str_end) {
        s++;
        if (!(len = grn_charlen(ctx, s, q->str_end))) { break; }
      }
    }
    GRN_TEXT_PUT(ctx, &q->buf, s, len);
  }
  q->cur = s;
  return rc;
}

static grn_obj *
resolve_top_level_name(grn_ctx *ctx, const char *name, unsigned int name_size)
{
  unsigned int i;
  unsigned int first_delimiter_position = 0;
  unsigned int n_delimiters = 0;
  grn_obj *top_level_object;
  grn_obj *object;

  for (i = 0; i < name_size; i++) {
    if (name[i] != GRN_DB_DELIMITER) {
      continue;
    }

    if (n_delimiters == 0) {
      first_delimiter_position = i;
    }
    n_delimiters++;
  }

  if (n_delimiters < 2) {
    return grn_ctx_get(ctx, name, name_size);
  }

  top_level_object = grn_ctx_get(ctx, name, first_delimiter_position);
  if (!top_level_object) {
    return NULL;
  }
  object = grn_obj_column(ctx, top_level_object,
                          name + first_delimiter_position + 1,
                          name_size - first_delimiter_position - 1);
  grn_obj_unlink(ctx, top_level_object);
  return object;
}

static grn_rc
get_identifier(grn_ctx *ctx, efs_info *q, grn_obj *name_resolve_context)
{
  const char *s;
  unsigned int len;
  grn_rc rc = GRN_SUCCESS;
  for (s = q->cur; s < q->str_end; s += len) {
    if (!(len = grn_charlen(ctx, s, q->str_end))) {
      rc = GRN_END_OF_DATA;
      goto exit;
    }
    if (grn_isspace(s, ctx->encoding)) { goto done; }
    if (len == 1) {
      switch (*s) {
      case '\0' : case '(' : case ')' : case '{' : case '}' :
      case '[' : case ']' : case ',' : case ':' : case '@' :
      case '?' : case '"' : case '*' : case '+' : case '-' :
      case '|' : case '/' : case '%' : case '!' : case '^' :
      case '&' : case '>' : case '<' : case '=' : case '~' :
        /* case '.' : */
        goto done;
        break;
      }
    }
  }
done :
  len = s - q->cur;
  switch (*q->cur) {
  case 'd' :
    if (len == 6 && !memcmp(q->cur, "delete", 6)) {
      PARSE(GRN_EXPR_TOKEN_DELETE);
      goto exit;
    }
    break;
  case 'f' :
    if (len == 5 && !memcmp(q->cur, "false", 5)) {
      grn_obj buf;
      PARSE(GRN_EXPR_TOKEN_BOOLEAN);
      GRN_BOOL_INIT(&buf, 0);
      GRN_BOOL_SET(ctx, &buf, 0);
      grn_expr_append_const(ctx, q->e, &buf, GRN_OP_PUSH, 1);
      GRN_OBJ_FIN(ctx, &buf);
      goto exit;
    }
    break;
  case 'i' :
    if (len == 2 && !memcmp(q->cur, "in", 2)) {
      PARSE(GRN_EXPR_TOKEN_IN);
      goto exit;
    }
    break;
  case 'n' :
    if (len == 4 && !memcmp(q->cur, "null", 4)) {
      grn_obj buf;
      PARSE(GRN_EXPR_TOKEN_NULL);
      GRN_VOID_INIT(&buf);
      grn_expr_append_const(ctx, q->e, &buf, GRN_OP_PUSH, 1);
      GRN_OBJ_FIN(ctx, &buf);
      goto exit;
    }
    break;
  case 't' :
    if (len == 4 && !memcmp(q->cur, "true", 4)) {
      grn_obj buf;
      PARSE(GRN_EXPR_TOKEN_BOOLEAN);
      GRN_BOOL_INIT(&buf, 0);
      GRN_BOOL_SET(ctx, &buf, 1);
      grn_expr_append_const(ctx, q->e, &buf, GRN_OP_PUSH, 1);
      GRN_OBJ_FIN(ctx, &buf);
      goto exit;
    }
    break;
  }
  {
    grn_obj *obj;
    const char *name = q->cur;
    unsigned int name_size = s - q->cur;
    if (name_resolve_context) {
      if ((obj = grn_obj_column(ctx, name_resolve_context, name, name_size))) {
        grn_expr_take_obj(ctx, q->e, obj);
        PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
        grn_expr_append_obj(ctx, q->e, obj, GRN_OP_GET_VALUE, 2);
        goto exit;
      }
    }
    if ((obj = grn_expr_get_var(ctx, q->e, name, name_size))) {
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      grn_expr_append_obj(ctx, q->e, obj, GRN_OP_PUSH, 1);
      goto exit;
    }
    if ((obj = grn_obj_column(ctx, q->table, name, name_size))) {
      grn_expr_take_obj(ctx, q->e, obj);
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      grn_expr_append_obj(ctx, q->e, obj, GRN_OP_GET_VALUE, 1);
      goto exit;
    }
    if ((obj = resolve_top_level_name(ctx, name, name_size))) {
      grn_expr_take_obj(ctx, q->e, obj);
      PARSE(GRN_EXPR_TOKEN_IDENTIFIER);
      grn_expr_append_obj(ctx, q->e, obj, GRN_OP_PUSH, 1);
      goto exit;
    }
    if (q->flags & GRN_EXPR_SYNTAX_OUTPUT_COLUMNS) {
      PARSE(GRN_EXPR_TOKEN_NONEXISTENT_COLUMN);
    } else {
      rc = GRN_SYNTAX_ERROR;
      ERR(rc,
          "[expr][parse] unknown identifier: <%.*s>",
          (int)name_size,
          name);
    }
  }
exit :
  q->cur = s;
  return rc;
}

static void
set_tos_minor_to_curr(grn_ctx *ctx, efs_info *q)
{
  yyParser *parser = ctx->impl->parser;
  yyStackEntry *yytos = parser->yytos;
  yytos->minor.yy0 = ((grn_expr *)(q->e))->codes_curr;
}

static grn_obj *
parse_script_extract_name_resolve_context(grn_ctx *ctx, efs_info *q)
{
  grn_expr *expr = (grn_expr *)(q->e);
  grn_expr_code *code_start;
  grn_expr_code *code_last;

  if (expr->codes_curr == 0) {
    return NULL;
  }

  code_start = expr->codes;
  code_last = code_start + (expr->codes_curr - 1);
  switch (code_last->op) {
  case GRN_OP_GET_MEMBER :
    {
      unsigned int n_used_codes_for_key;
      grn_expr_code *code_key;
      grn_expr_code *code_receiver;

      code_key = code_last - 1;
      if (code_key < code_start) {
        return NULL;
      }

      n_used_codes_for_key = grn_expr_code_n_used_codes(ctx,
                                                        code_start,
                                                        code_key);
      if (n_used_codes_for_key == 0) {
        return NULL;
      }
      code_receiver = code_key - n_used_codes_for_key;
      if (code_receiver < code_start) {
        return NULL;
      }
      return code_receiver->value;
    }
    break;
  default :
    /* TODO: Support other operators. */
    return NULL;
    break;
  }
}

static grn_rc
parse_script(grn_ctx *ctx, efs_info *q)
{
  grn_rc rc = GRN_SUCCESS;
  grn_obj *name_resolve_context = NULL;
  for (;;) {
    grn_obj *current_name_resolve_context = name_resolve_context;
    name_resolve_context = NULL;
    skip_space(ctx, q);
    if (q->cur >= q->str_end) { rc = GRN_END_OF_DATA; goto exit; }
    switch (*q->cur) {
    case '\0' :
      rc = GRN_END_OF_DATA;
      goto exit;
      break;
    case '(' :
      PARSE(GRN_EXPR_TOKEN_PARENL);
      q->cur++;
      break;
    case ')' :
      PARSE(GRN_EXPR_TOKEN_PARENR);
      q->cur++;
      break;
    case '{' :
      PARSE(GRN_EXPR_TOKEN_BRACEL);
      q->cur++;
      break;
    case '}' :
      PARSE(GRN_EXPR_TOKEN_BRACER);
      q->cur++;
      break;
    case '[' :
      PARSE(GRN_EXPR_TOKEN_BRACKETL);
      q->cur++;
      break;
    case ']' :
      PARSE(GRN_EXPR_TOKEN_BRACKETR);
      q->cur++;
      break;
    case ',' :
      PARSE(GRN_EXPR_TOKEN_COMMA);
      q->cur++;
      break;
    case '.' :
      PARSE(GRN_EXPR_TOKEN_DOT);
      name_resolve_context = parse_script_extract_name_resolve_context(ctx, q);
      q->cur++;
      break;
    case ':' :
      PARSE(GRN_EXPR_TOKEN_COLON);
      q->cur++;
      set_tos_minor_to_curr(ctx, q);
      grn_expr_append_op(ctx, q->e, GRN_OP_JUMP, 0);
      break;
    case '@' :
      switch (q->cur[1]) {
      case '^' :
        PARSE(GRN_EXPR_TOKEN_PREFIX);
        q->cur += 2;
        break;
      case '$' :
        PARSE(GRN_EXPR_TOKEN_SUFFIX);
        q->cur += 2;
        break;
      case '~' :
        PARSE(GRN_EXPR_TOKEN_REGEXP);
        q->cur += 2;
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_MATCH);
        q->cur++;
        break;
      }
      break;
    case '~' :
      PARSE(GRN_EXPR_TOKEN_BITWISE_NOT);
      q->cur++;
      break;
    case '?' :
      PARSE(GRN_EXPR_TOKEN_QUESTION);
      q->cur++;
      set_tos_minor_to_curr(ctx, q);
      grn_expr_append_op(ctx, q->e, GRN_OP_CJUMP, 0);
      break;
    case '"' :
      if ((rc = get_string(ctx, q, '"'))) { goto exit; }
      PARSE(GRN_EXPR_TOKEN_STRING);
      grn_expr_append_const(ctx, q->e, &q->buf, GRN_OP_PUSH, 1);
      break;
    case '\'' :
      if ((rc = get_string(ctx, q, '\''))) { goto exit; }
      PARSE(GRN_EXPR_TOKEN_STRING);
      grn_expr_append_const(ctx, q->e, &q->buf, GRN_OP_PUSH, 1);
      break;
    case '*' :
      switch (q->cur[1]) {
      case 'N' :
        {
          const char *next_start = q->cur + 2;
          const char *end;
          int max_interval;
          int token = GRN_EXPR_TOKEN_NEAR;
          if (next_start < q->str_end && next_start[0] == 'P') {
            token = GRN_EXPR_TOKEN_NEAR_PHRASE;
            next_start++;
          }
          max_interval = grn_atoi(next_start, q->str_end, &end);
          if (end == next_start) {
            max_interval = DEFAULT_MAX_INTERVAL;
          } else {
            next_start = end;
          }
          GRN_INT32_PUT(ctx, &q->max_interval_stack, max_interval);
          PARSE(token);
          q->cur = next_start;
        }
        break;
      case 'S' :
        {
          const char *next_start = q->cur + 2;
          const char *end;
          int similarity_threshold;
          similarity_threshold = grn_atoi(next_start, q->str_end, &end);
          if (end == next_start) {
            similarity_threshold = DEFAULT_SIMILARITY_THRESHOLD;
          } else {
            next_start = end;
          }
          GRN_INT32_PUT(ctx,
                        &q->similarity_threshold_stack,
                        similarity_threshold);
          PARSE(GRN_EXPR_TOKEN_SIMILAR);
          q->cur = next_start;
        }
        break;
      case 'T' :
        PARSE(GRN_EXPR_TOKEN_TERM_EXTRACT);
        q->cur += 2;
        break;
      case 'Q' :
        {
          const char *next_start = q->cur + 2;
          const char *end;
          int quorum_threshold;
          quorum_threshold = grn_atoi(next_start, q->str_end, &end);
          if (end == next_start) {
            quorum_threshold = DEFAULT_QUORUM_THRESHOLD;
          } else {
            next_start = end;
          }
          GRN_INT32_PUT(ctx, &q->quorum_threshold_stack, quorum_threshold);
          PARSE(GRN_EXPR_TOKEN_QUORUM);
          q->cur = next_start;
        }
        break;
      case '>' :
        PARSE(GRN_EXPR_TOKEN_ADJUST);
        q->cur += 2;
        break;
      case '<' :
        PARSE(GRN_EXPR_TOKEN_ADJUST);
        q->cur += 2;
        break;
      case '~' :
        PARSE(GRN_EXPR_TOKEN_ADJUST);
        q->cur += 2;
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_STAR_ASSIGN);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'*=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_STAR);
        q->cur++;
        break;
      }
      break;
    case '+' :
      switch (q->cur[1]) {
      case '+' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_INCR);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'++' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_PLUS_ASSIGN);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'+=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_PLUS);
        q->cur++;
        break;
      }
      break;
    case '-' :
      switch (q->cur[1]) {
      case '-' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_DECR);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'--' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_MINUS_ASSIGN);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'-=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_MINUS);
        q->cur++;
        break;
      }
      break;
    case '|' :
      switch (q->cur[1]) {
      case '|' :
        PARSE(GRN_EXPR_TOKEN_LOGICAL_OR);
        q->cur += 2;
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_OR_ASSIGN);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'|=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_BITWISE_OR);
        q->cur++;
        break;
      }
      break;
    case '/' :
      switch (q->cur[1]) {
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_SLASH_ASSIGN);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'/=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_SLASH);
        q->cur++;
        break;
      }
      break;
    case '%' :
      switch (q->cur[1]) {
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_MOD_ASSIGN);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'%%=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_MOD);
        q->cur++;
        break;
      }
      break;
    case '!' :
      switch (q->cur[1]) {
      case '=' :
        PARSE(GRN_EXPR_TOKEN_NOT_EQUAL);
        q->cur += 2;
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_NOT);
        q->cur++;
        break;
      }
      break;
    case '^' :
      switch (q->cur[1]) {
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          q->cur += 2;
          PARSE(GRN_EXPR_TOKEN_XOR_ASSIGN);
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'^=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_BITWISE_XOR);
        q->cur++;
        break;
      }
      break;
    case '&' :
      switch (q->cur[1]) {
      case '&' :
        PARSE(GRN_EXPR_TOKEN_LOGICAL_AND);
        q->cur += 2;
        break;
      case '=' :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_AND_ASSIGN);
          q->cur += 2;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'&=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      case '!' :
        PARSE(GRN_EXPR_TOKEN_LOGICAL_AND_NOT);
        q->cur += 2;
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_BITWISE_AND);
        q->cur++;
        break;
      }
      break;
    case '>' :
      switch (q->cur[1]) {
      case '>' :
        switch (q->cur[2]) {
        case '>' :
          switch (q->cur[3]) {
          case '=' :
            if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
              PARSE(GRN_EXPR_TOKEN_SHIFTRR_ASSIGN);
              q->cur += 4;
            } else {
              ERR(GRN_UPDATE_NOT_ALLOWED,
                  "'>>>=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
            }
            break;
          default :
            PARSE(GRN_EXPR_TOKEN_SHIFTRR);
            q->cur += 3;
            break;
          }
          break;
        case '=' :
          if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
            PARSE(GRN_EXPR_TOKEN_SHIFTR_ASSIGN);
            q->cur += 3;
          } else {
            ERR(GRN_UPDATE_NOT_ALLOWED,
                "'>>=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
          }
          break;
        default :
          PARSE(GRN_EXPR_TOKEN_SHIFTR);
          q->cur += 2;
          break;
        }
        break;
      case '=' :
        PARSE(GRN_EXPR_TOKEN_GREATER_EQUAL);
        q->cur += 2;
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_GREATER);
        q->cur++;
        break;
      }
      break;
    case '<' :
      switch (q->cur[1]) {
      case '<' :
        switch (q->cur[2]) {
        case '=' :
          if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
            PARSE(GRN_EXPR_TOKEN_SHIFTL_ASSIGN);
            q->cur += 3;
          } else {
            ERR(GRN_UPDATE_NOT_ALLOWED,
                "'<<=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
          }
          break;
        default :
          PARSE(GRN_EXPR_TOKEN_SHIFTL);
          q->cur += 2;
          break;
        }
        break;
      case '=' :
        PARSE(GRN_EXPR_TOKEN_LESS_EQUAL);
        q->cur += 2;
        break;
      default :
        PARSE(GRN_EXPR_TOKEN_LESS);
        q->cur++;
        break;
      }
      break;
    case '=' :
      switch (q->cur[1]) {
      case '=' :
        PARSE(GRN_EXPR_TOKEN_EQUAL);
        q->cur += 2;
        break;
      default :
        if (q->flags & GRN_EXPR_ALLOW_UPDATE) {
          PARSE(GRN_EXPR_TOKEN_ASSIGN);
          q->cur++;
        } else {
          ERR(GRN_UPDATE_NOT_ALLOWED,
              "'=' is not allowed: <%.*s>", (int)(q->str_end - q->str), q->str);
        }
        break;
      }
      break;
    case '0' : case '1' : case '2' : case '3' : case '4' :
    case '5' : case '6' : case '7' : case '8' : case '9' :
      {
        const char *rest;
        int64_t int64 = grn_atoll(q->cur, q->str_end, &rest);
        // checks to see grn_atoll was appropriate
        // (NOTE: *q->cur begins with a digit. Thus, grn_atoll parses at least
        //        one char.)
        if (q->str_end != rest &&
            (*rest == '.' || *rest == 'e' || *rest == 'E' ||
             (*rest >= '0' && *rest <= '9'))) {
          grn_obj buffer;
          char *rest_float;
          double d;
          grn_obj floatbuf;
          GRN_TEXT_INIT(&buffer, 0);
          GRN_TEXT_SET(ctx, &buffer, q->cur, q->str_end - q->cur);
          GRN_TEXT_PUTC(ctx, &buffer, '\0');
          errno = 0;
          d = strtod(GRN_TEXT_VALUE(&buffer), &rest_float);
          rest = q->cur + (rest_float - GRN_TEXT_VALUE(&buffer));
          GRN_OBJ_FIN(ctx, &buffer);
          GRN_FLOAT_INIT(&floatbuf, 0);
          GRN_FLOAT_SET(ctx, &floatbuf, d);
          grn_expr_append_const(ctx, q->e, &floatbuf, GRN_OP_PUSH, 1);
        } else {
          const char *rest64 = rest;
          grn_atoui(q->cur, q->str_end, &rest);
          // checks to see grn_atoi failed (see above NOTE)
          if ((int64 > UINT32_MAX) ||
              (q->str_end != rest && *rest >= '0' && *rest <= '9')) {
            grn_obj int64buf;
            GRN_INT64_INIT(&int64buf, 0);
            GRN_INT64_SET(ctx, &int64buf, int64);
            grn_expr_append_const(ctx, q->e, &int64buf, GRN_OP_PUSH, 1);
            rest = rest64;
          } else if (int64 > INT32_MAX || int64 < INT32_MIN) {
            grn_obj int64buf;
            GRN_INT64_INIT(&int64buf, 0);
            GRN_INT64_SET(ctx, &int64buf, int64);
            grn_expr_append_const(ctx, q->e, &int64buf, GRN_OP_PUSH, 1);
          } else {
            grn_obj int32buf;
            GRN_INT32_INIT(&int32buf, 0);
            GRN_INT32_SET(ctx, &int32buf, (int32_t)int64);
            grn_expr_append_const(ctx, q->e, &int32buf, GRN_OP_PUSH, 1);
          }
        }
        PARSE(GRN_EXPR_TOKEN_DECIMAL);
        q->cur = rest;
      }
      break;
    default :
      if ((rc = get_identifier(ctx, q, current_name_resolve_context))) {
        goto exit;
      }
      break;
    }
    if (ctx->rc) { rc = ctx->rc; break; }
  }
exit :
  PARSE(0);
  return rc;
}

grn_rc
grn_expr_parse(grn_ctx *ctx, grn_obj *expr,
               const char *str, unsigned int str_size,
               grn_obj *default_column, grn_operator default_mode,
               grn_operator default_op, grn_expr_flags flags)
{
  efs_info efsi;
  if (grn_expr_parser_open(ctx)) { return ctx->rc; }
  GRN_API_ENTER;
  efsi.ctx = ctx;
  efsi.str = str;
  if ((efsi.v = grn_expr_get_var_by_offset(ctx, expr, 0)) &&
      (efsi.table = grn_ctx_at(ctx, efsi.v->header.domain))) {
    GRN_TEXT_INIT(&efsi.buf, 0);
    GRN_INT32_INIT(&efsi.op_stack, GRN_OBJ_VECTOR);
    GRN_INT32_INIT(&efsi.mode_stack, GRN_OBJ_VECTOR);
    GRN_INT32_INIT(&efsi.max_interval_stack, GRN_OBJ_VECTOR);
    GRN_INT32_INIT(&efsi.similarity_threshold_stack, GRN_OBJ_VECTOR);
    GRN_INT32_INIT(&efsi.quorum_threshold_stack, GRN_OBJ_VECTOR);
    GRN_INT32_INIT(&efsi.weight_stack, GRN_OBJ_VECTOR);
    GRN_PTR_INIT(&efsi.column_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
    GRN_PTR_INIT(&efsi.token_stack, GRN_OBJ_VECTOR, GRN_ID_NIL);
    efsi.e = expr;
    efsi.str = str;
    efsi.cur = str;
    efsi.str_end = str + str_size;
    efsi.default_column = default_column;
    GRN_PTR_PUT(ctx, &efsi.column_stack, default_column);
    GRN_INT32_PUT(ctx, &efsi.op_stack, default_op);
    GRN_INT32_PUT(ctx, &efsi.mode_stack, default_mode);
    GRN_INT32_PUT(ctx, &efsi.weight_stack, 0);
    efsi.default_flags = efsi.flags = flags;
    efsi.escalation_threshold = GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD;
    efsi.escalation_decaystep = DEFAULT_DECAYSTEP;
    efsi.weight_offset = 0;
    memset(&(efsi.opt), 0, sizeof(grn_select_optarg));
    efsi.opt.weight_vector = NULL;
    efsi.weight_set = NULL;
    efsi.object_literal = NULL;
    efsi.array_literal = NULL;
    efsi.paren_depth = 0;
    efsi.pending_token.string = NULL;
    efsi.pending_token.string_length = 0;
    efsi.pending_token.token = 0;

    if (flags & (GRN_EXPR_SYNTAX_SCRIPT |
                 GRN_EXPR_SYNTAX_OUTPUT_COLUMNS |
                 GRN_EXPR_SYNTAX_ADJUSTER)) {
      efs_info *q = &efsi;
      if (flags & GRN_EXPR_SYNTAX_OUTPUT_COLUMNS) {
        PARSE(GRN_EXPR_TOKEN_START_OUTPUT_COLUMNS);
      } else if (flags & GRN_EXPR_SYNTAX_ADJUSTER) {
        PARSE(GRN_EXPR_TOKEN_START_ADJUSTER);
      }
      parse_script(ctx, &efsi);
    } else {
      parse_query(ctx, &efsi);
    }

    /*
        grn_obj strbuf;
        GRN_TEXT_INIT(&strbuf, 0);
        grn_expr_inspect_internal(ctx, &strbuf, expr);
        GRN_TEXT_PUTC(ctx, &strbuf, '\0');
        GRN_LOG(ctx, GRN_LOG_NOTICE, "query=(%s)", GRN_TEXT_VALUE(&strbuf));
        GRN_OBJ_FIN(ctx, &strbuf);
    */

    /*
    efsi.opt.vector_size = DEFAULT_WEIGHT_VECTOR_SIZE;
    efsi.opt.func = efsi.weight_set ? section_weight_cb : NULL;
    efsi.opt.func_arg = efsi.weight_set;
    efsi.snip_conds = NULL;
    */
    GRN_OBJ_FIN(ctx, &efsi.op_stack);
    GRN_OBJ_FIN(ctx, &efsi.mode_stack);
    GRN_OBJ_FIN(ctx, &efsi.max_interval_stack);
    GRN_OBJ_FIN(ctx, &efsi.similarity_threshold_stack);
    GRN_OBJ_FIN(ctx, &efsi.quorum_threshold_stack);
    GRN_OBJ_FIN(ctx, &efsi.weight_stack);
    GRN_OBJ_FIN(ctx, &efsi.column_stack);
    GRN_OBJ_FIN(ctx, &efsi.token_stack);
    GRN_OBJ_FIN(ctx, &efsi.buf);
    if (efsi.object_literal) {
      grn_obj *value;
      GRN_HASH_EACH(ctx, efsi.object_literal, i, NULL, NULL, (void **)&value, {
        GRN_OBJ_FIN(ctx, value);
      });
      grn_hash_close(ctx, efsi.object_literal);
    }
    if (efsi.array_literal) {
      GRN_OBJ_FIN(ctx, efsi.array_literal);
    }
    if (grn_enable_reference_count) {
      grn_obj_unlink(ctx, efsi.table);
    }
  } else {
    ERR(GRN_INVALID_ARGUMENT, "variable is not defined correctly");
  }
  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_expr_parser_close(grn_ctx *ctx)
{
  if (ctx->impl->parser) {
    yyParser *parser = (yyParser *)ctx->impl->parser;
    ctx->impl->parser = NULL;
    grn_expr_parserFree(parser, free);
  }
  return ctx->rc;
}

typedef grn_rc (*grn_expr_syntax_expand_term_func)(grn_ctx *ctx,
                                                   const char *term,
                                                   unsigned int term_len,
                                                   grn_obj *substituted_term,
                                                   grn_user_data *user_data);
static grn_rc
grn_expr_syntax_expand_term_by_func(grn_ctx *ctx,
                                    const char *term, unsigned int term_len,
                                    grn_obj *expanded_term,
                                    grn_user_data *user_data)
{
  grn_rc rc;
  grn_obj *expander = user_data->ptr;
  grn_obj grn_term;
  grn_obj *caller;
  grn_obj *rc_object;
  int nargs = 0;

  GRN_TEXT_INIT(&grn_term, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_SET(ctx, &grn_term, term, term_len);
  grn_ctx_push(ctx, &grn_term);
  nargs++;
  grn_ctx_push(ctx, expanded_term);
  nargs++;

  caller = grn_expr_create(ctx, NULL, 0);
  rc = grn_proc_call(ctx, expander, nargs, caller);
  GRN_OBJ_FIN(ctx, &grn_term);
  rc_object = grn_ctx_pop(ctx);
  rc = GRN_INT32_VALUE(rc_object);
  grn_obj_unlink(ctx, caller);

  return rc;
}

typedef struct {
  grn_obj *table;
  grn_obj *column;
} grn_expr_syntax_expand_term_by_column_data;

static grn_rc
grn_expr_syntax_expand_term_by_column(grn_ctx *ctx,
                                      const char *term, unsigned int term_len,
                                      grn_obj *expanded_term,
                                      grn_user_data *user_data)
{
  grn_rc rc = GRN_END_OF_DATA;
  grn_id id;
  grn_expr_syntax_expand_term_by_column_data *data = user_data->ptr;
  grn_obj *table, *column;

  table = data->table;
  column = data->column;
  if ((id = grn_table_get(ctx, table, term, term_len))) {
    if ((column->header.type == GRN_COLUMN_VAR_SIZE) &&
        ((column->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) == GRN_OBJ_COLUMN_VECTOR)) {
      unsigned int i, n;
      grn_obj values;
      GRN_TEXT_INIT(&values, GRN_OBJ_VECTOR);
      grn_obj_get_value(ctx, column, id, &values);
      n = grn_vector_size(ctx, &values);
      if (n > 1) { GRN_TEXT_PUTC(ctx, expanded_term, '('); }
      for (i = 0; i < n; i++) {
        const char *value;
        unsigned int length;
        if (i > 0) {
          GRN_TEXT_PUTS(ctx, expanded_term, " OR ");
        }
        if (n > 1) { GRN_TEXT_PUTC(ctx, expanded_term, '('); }
        length = grn_vector_get_element(ctx, &values, i, &value, NULL, NULL);
        GRN_TEXT_PUT(ctx, expanded_term, value, length);
        if (n > 1) { GRN_TEXT_PUTC(ctx, expanded_term, ')'); }
      }
      if (n > 1) { GRN_TEXT_PUTC(ctx, expanded_term, ')'); }
      GRN_OBJ_FIN(ctx, &values);
    } else {
      grn_obj_get_value(ctx, column, id, expanded_term);
    }
    rc = GRN_SUCCESS;
  }
  return rc;
}

typedef struct {
  grn_obj *table;
  grn_obj *term_column;
  grn_obj *expanded_term_column;
} grn_expr_syntax_expand_term_by_table_data;

static grn_rc
grn_expr_syntax_expand_term_by_table(grn_ctx *ctx,
                                     const char *term, unsigned int term_len,
                                     grn_obj *expanded_term,
                                     grn_user_data *user_data)
{
  grn_rc rc = GRN_END_OF_DATA;
  grn_expr_syntax_expand_term_by_table_data *data = user_data->ptr;
  grn_obj *table;
  grn_obj *term_column;
  grn_obj *expanded_term_column;
  grn_obj *expression;
  grn_obj *variable;
  grn_obj *found_terms;
  int n_terms;

  table = data->table;
  term_column = data->term_column;
  expanded_term_column = data->expanded_term_column;

  GRN_EXPR_CREATE_FOR_QUERY(ctx, table, expression, variable);
  if (ctx->rc != GRN_SUCCESS) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc,
        "[query][expand][table] "
        "failed to create expression: <%s>",
        errbuf);
    return ctx->rc;
  }
  grn_expr_append_const(ctx, expression, term_column, GRN_OP_GET_VALUE, 1);
  grn_expr_append_const_str(ctx, expression, term, term_len, GRN_OP_PUSH, 1);
  grn_expr_append_op(ctx, expression, GRN_OP_EQUAL, 2);
  if (ctx->rc != GRN_SUCCESS) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    grn_obj_close(ctx, expression);
    ERR(ctx->rc,
        "[query][expand][table] "
        "failed to build expression: <%s>",
        errbuf);
    return ctx->rc;
  }

  found_terms = grn_table_select(ctx, table, expression, NULL, GRN_OP_OR);
  grn_obj_close(ctx, expression);
  if (!found_terms) {
    char errbuf[GRN_CTX_MSGSIZE];
    grn_strcpy(errbuf, GRN_CTX_MSGSIZE, ctx->errbuf);
    ERR(ctx->rc,
        "[query][expand][table] "
        "failed to find term: <%.*s>: <%s>",
        (int)term_len,
        term,
        errbuf);
    return ctx->rc;
  }

  n_terms = grn_table_size(ctx, found_terms);
  if (n_terms == 0) {
    grn_obj_close(ctx, found_terms);
    return rc;
  }

  {
    int nth_term;

    GRN_TEXT_PUTC(ctx, expanded_term, '(');
    nth_term = 0;
    GRN_TABLE_EACH_BEGIN(ctx, found_terms, cursor, id) {
      void *key;
      grn_id record_id;

      grn_table_cursor_get_key(ctx, cursor, &key);
      record_id = *((grn_id *)key);
      if (grn_obj_is_vector_column(ctx, expanded_term_column)) {
        unsigned int j, n_values;
        grn_obj values;
        GRN_TEXT_INIT(&values, GRN_OBJ_VECTOR);
        grn_obj_get_value(ctx, expanded_term_column, record_id, &values);
        n_values = grn_vector_size(ctx, &values);
        n_terms += n_values - 1;
        for (j = 0; j < n_values; j++) {
          const char *value;
          unsigned int length;
          if (nth_term > 0) {
            GRN_TEXT_PUTS(ctx, expanded_term, " OR ");
          }
          if (n_terms > 1) {
            GRN_TEXT_PUTC(ctx, expanded_term, '(');
          }
          length = grn_vector_get_element(ctx, &values, j, &value, NULL, NULL);
          GRN_TEXT_PUT(ctx, expanded_term, value, length);
          if (n_terms > 1) {
            GRN_TEXT_PUTC(ctx, expanded_term, ')');
          }
          nth_term++;
        }
        GRN_OBJ_FIN(ctx, &values);
      } else {
        if (nth_term > 0) {
          GRN_TEXT_PUTS(ctx, expanded_term, " OR ");
        }
        if (n_terms > 1) { GRN_TEXT_PUTC(ctx, expanded_term, '('); }
        grn_obj_get_value(ctx, expanded_term_column, record_id, expanded_term);
        if (n_terms > 1) { GRN_TEXT_PUTC(ctx, expanded_term, ')'); }
        nth_term++;
      }
    } GRN_TABLE_EACH_END(ctx, cursor);
    GRN_TEXT_PUTC(ctx, expanded_term, ')');
  }
  rc = GRN_SUCCESS;
  grn_obj_close(ctx, found_terms);

  return rc;
}

static grn_rc
grn_expr_syntax_expand_query_terms(grn_ctx *ctx,
                                   const char *query, unsigned int query_size,
                                   grn_expr_flags flags,
                                   grn_obj *expanded_query,
                                   grn_expr_syntax_expand_term_func expand_term_func,
                                   grn_user_data *user_data)
{
  grn_obj buf;
  unsigned int len;
  const char *start, *cur = query, *query_end = query + (size_t)query_size;
  GRN_TEXT_INIT(&buf, 0);
  for (;;) {
    while (cur < query_end && grn_isspace(cur, ctx->encoding)) {
      if (!(len = grn_charlen(ctx, cur, query_end))) { goto exit; }
      GRN_TEXT_PUT(ctx, expanded_query, cur, len);
      cur += len;
    }
    if (query_end <= cur) { break; }
    switch (*cur) {
    case '\0' :
      goto exit;
      break;
    case GRN_QUERY_AND :
    case GRN_QUERY_ADJ_INC :
    case GRN_QUERY_ADJ_DEC :
    case GRN_QUERY_ADJ_NEG :
    case GRN_QUERY_AND_NOT :
    case GRN_QUERY_PARENL :
    case GRN_QUERY_PARENR :
    case GRN_QUERY_PREFIX :
      GRN_TEXT_PUTC(ctx, expanded_query, *cur);
      cur++;
      break;
    case GRN_QUERY_QUOTEL :
      GRN_BULK_REWIND(&buf);
      for (start = cur++; cur < query_end; cur += len) {
        if (!(len = grn_charlen(ctx, cur, query_end))) {
          goto exit;
        } else if (len == 1) {
          if (*cur == GRN_QUERY_QUOTER) {
            cur++;
            break;
          } else if (cur + 1 < query_end && *cur == GRN_QUERY_ESCAPE) {
            cur++;
            len = grn_charlen(ctx, cur, query_end);
          }
        }
        GRN_TEXT_PUT(ctx, &buf, cur, len);
      }
      if (expand_term_func(ctx, GRN_TEXT_VALUE(&buf), GRN_TEXT_LEN(&buf),
                           expanded_query, user_data)) {
        GRN_TEXT_PUT(ctx, expanded_query, start, cur - start);
      }
      break;
    case 'O' :
      if (cur + 2 <= query_end && cur[1] == 'R' &&
          (cur + 2 == query_end || grn_isspace(cur + 2, ctx->encoding))) {
        GRN_TEXT_PUT(ctx, expanded_query, cur, 2);
        cur += 2;
        break;
      }
      /* fallthru */
    default :
      for (start = cur; cur < query_end; cur += len) {
        if (!(len = grn_charlen(ctx, cur, query_end))) {
          goto exit;
        } else if (grn_isspace(cur, ctx->encoding)) {
          break;
        } else if (len == 1) {
          if (*cur == GRN_QUERY_PARENL ||
              *cur == GRN_QUERY_PARENR ||
              *cur == GRN_QUERY_PREFIX) {
            break;
          } else if (flags & GRN_EXPR_ALLOW_COLUMN && *cur == GRN_QUERY_COLUMN) {
            if (cur + 1 < query_end) {
              switch (cur[1]) {
              case '!' :
              case '@' :
              case '^' :
              case '$' :
                cur += 2;
                break;
              case '=' :
                cur += (flags & GRN_EXPR_ALLOW_UPDATE) ? 2 : 1;
                break;
              case '<' :
              case '>' :
                cur += (cur + 2 < query_end && cur[2] == '=') ? 3 : 2;
                break;
              default :
                cur += 1;
                break;
              }
            } else {
              cur += 1;
            }
            GRN_TEXT_PUT(ctx, expanded_query, start, cur - start);
            start = cur;
            break;
          }
        }
      }
      if (start < cur) {
        if (expand_term_func(ctx, start, cur - start,
                             expanded_query, user_data)) {
          GRN_TEXT_PUT(ctx, expanded_query, start, cur - start);
        }
      }
      break;
    }
  }
exit :
  GRN_OBJ_FIN(ctx, &buf);
  return GRN_SUCCESS;
}

grn_rc
grn_expr_syntax_expand_query(grn_ctx *ctx,
                             const char *query, int query_size,
                             grn_expr_flags flags,
                             grn_obj *expander,
                             grn_obj *expanded_query)
{
  GRN_API_ENTER;

  if (query_size < 0) {
    query_size = strlen(query);
  }

  switch (expander->header.type) {
  case GRN_PROC :
    if (((grn_proc *)expander)->type == GRN_PROC_FUNCTION) {
      grn_user_data user_data;
      user_data.ptr = expander;
      grn_expr_syntax_expand_query_terms(ctx,
                                         query, query_size,
                                         flags,
                                         expanded_query,
                                         grn_expr_syntax_expand_term_by_func,
                                         &user_data);
    } else {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      name_size = grn_obj_name(ctx, expander, name, GRN_TABLE_MAX_KEY_SIZE);
      ERR(GRN_INVALID_ARGUMENT,
          "[query][expand][proc] "
          "proc query expander must be a function proc: <%.*s>",
          name_size, name);
    }
    break;
  case GRN_COLUMN_FIX_SIZE :
  case GRN_COLUMN_VAR_SIZE :
    {
      grn_obj *expansion_table;
      expansion_table = grn_column_table(ctx, expander);
      if (expansion_table) {
        grn_user_data user_data;
        grn_expr_syntax_expand_term_by_column_data data;
        user_data.ptr = &data;
        data.table = expansion_table;
        data.column = expander;
        grn_expr_syntax_expand_query_terms(ctx,
                                           query, query_size,
                                           flags,
                                           expanded_query,
                                           grn_expr_syntax_expand_term_by_column,
                                           &user_data);
      } else {
        char name[GRN_TABLE_MAX_KEY_SIZE];
        int name_size;
        name_size = grn_obj_name(ctx, expander, name, GRN_TABLE_MAX_KEY_SIZE);
        ERR(GRN_INVALID_ARGUMENT,
            "[query][expand][column] "
            "failed to get table of query expansion column: <%.*s>",
            name_size, name);
      }
    }
    break;
  default :
    {
      char name[GRN_TABLE_MAX_KEY_SIZE];
      int name_size;
      grn_obj type_name;

      name_size = grn_obj_name(ctx, expander, name, GRN_TABLE_MAX_KEY_SIZE);
      GRN_TEXT_INIT(&type_name, 0);
      grn_inspect_type(ctx, &type_name, expander->header.type);
      ERR(GRN_INVALID_ARGUMENT,
          "[query][expand] "
          "query expander must be a data column or function proc: <%.*s>(%.*s)",
          name_size, name,
          (int)GRN_TEXT_LEN(&type_name), GRN_TEXT_VALUE(&type_name));
      GRN_OBJ_FIN(ctx, &type_name);
    }
    break;
  }

  GRN_API_RETURN(ctx->rc);
}

grn_rc
grn_expr_syntax_expand_query_by_table(grn_ctx *ctx,
                                      const char *query, int query_size,
                                      grn_expr_flags flags,
                                      grn_obj *term_column,
                                      grn_obj *expanded_term_column,
                                      grn_obj *expanded_query)
{
  grn_obj *table;
  grn_bool term_column_is_key;

  GRN_API_ENTER;

  if (query_size < 0) {
    query_size = strlen(query);
  }

  if (!grn_obj_is_data_column(ctx, expanded_term_column)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, expanded_term_column);
    ERR(GRN_INVALID_ARGUMENT,
        "[query][expand][table] "
        "expanded term column must be a data column: <%.*s>",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(ctx->rc);
  }
  table = grn_column_table(ctx, expanded_term_column);

  if (!term_column) {
    term_column_is_key = GRN_TRUE;
  } else {
    if (grn_obj_is_key_accessor(ctx, term_column)) {
      term_column_is_key = GRN_TRUE;
    } else if (grn_obj_is_data_column(ctx, term_column)) {
      term_column_is_key = GRN_FALSE;
    } else {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, term_column);
      ERR(GRN_INVALID_ARGUMENT,
          "[query][expand][table] "
          "term column must be NULL, _key or a data column: <%.*s>",
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      GRN_API_RETURN(ctx->rc);
    }
    if (term_column->header.domain != expanded_term_column->header.domain) {
      grn_obj inspected_term_column;
      grn_obj inspected_expanded_term_column;
      GRN_TEXT_INIT(&inspected_term_column, 0);
      GRN_TEXT_INIT(&inspected_expanded_term_column, 0);
      grn_inspect(ctx, &inspected_term_column, term_column);
      grn_inspect(ctx, &inspected_expanded_term_column, expanded_term_column);
      ERR(GRN_INVALID_ARGUMENT,
          "[query][expand][table] "
          "term column and expanded term column must belong to the same table: "
          "term column: <%.*s>, "
          "expanded term column: <%*.s>",
          (int)GRN_TEXT_LEN(&inspected_term_column),
          GRN_TEXT_VALUE(&inspected_term_column),
          (int)GRN_TEXT_LEN(&inspected_expanded_term_column),
          GRN_TEXT_VALUE(&inspected_expanded_term_column));
      GRN_OBJ_FIN(ctx, &inspected_term_column);
      GRN_OBJ_FIN(ctx, &inspected_expanded_term_column);
      GRN_API_RETURN(ctx->rc);
    }
  }

  if (term_column_is_key) {
    grn_user_data user_data;
    grn_expr_syntax_expand_term_by_column_data data;
    user_data.ptr = &data;
    data.table = table;
    data.column = expanded_term_column;
    grn_expr_syntax_expand_query_terms(ctx,
                                       query, query_size,
                                       flags,
                                       expanded_query,
                                       grn_expr_syntax_expand_term_by_column,
                                       &user_data);
  } else {
    grn_user_data user_data;
    grn_expr_syntax_expand_term_by_table_data data;
    user_data.ptr = &data;
    data.table = table;
    data.term_column = term_column;
    data.expanded_term_column = expanded_term_column;
    grn_expr_syntax_expand_query_terms(ctx,
                                       query, query_size,
                                       flags,
                                       expanded_query,
                                       grn_expr_syntax_expand_term_by_table,
                                       &user_data);
  }

  GRN_API_RETURN(ctx->rc);
}

/*
 * TODO: It's very loose implementations. It just splits regexp by
 * meta characters to extract keywords. For example, "r.*nga" has "r"
 * and "nga" keywords.
 */
static void
grn_expr_get_keywords_regexp(grn_ctx *ctx, grn_obj *keywords, grn_obj *regexp)
{
  const char *all_off_options = "?-mix:";
  size_t all_off_options_length = strlen(all_off_options);
  const char *regexp_raw;
  const char *regexp_raw_end;
  grn_bool escaping = GRN_FALSE;
  grn_obj keyword;

  regexp_raw = GRN_TEXT_VALUE(regexp);
  regexp_raw_end = regexp_raw + GRN_TEXT_LEN(regexp);

  GRN_TEXT_INIT(&keyword, 0);
  while (regexp_raw < regexp_raw_end) {
    unsigned int char_len;

    char_len = grn_charlen(ctx, regexp_raw, regexp_raw_end);

    if (char_len == 1) {
      if (escaping) {
        escaping = GRN_FALSE;
        switch (regexp_raw[0]) {
        case 'A' :
        case 'z' :
          if (GRN_TEXT_LEN(&keyword) > 0) {
            grn_vector_add_element(ctx,
                                   keywords,
                                   GRN_TEXT_VALUE(&keyword),
                                   GRN_TEXT_LEN(&keyword),
                                   0,
                                   GRN_DB_TEXT);
            GRN_BULK_REWIND(&keyword);
          }
          break;
        default :
          GRN_TEXT_PUTC(ctx, &keyword, regexp_raw[0]);
          break;
        }
      } else {
        switch (regexp_raw[0]) {
        case '(' :
          regexp_raw += all_off_options_length;
          break;
        case ')' :
          break;
        case '.' :
          break;
        case '*' :
          if (GRN_TEXT_LEN(&keyword) > 0) {
            grn_vector_add_element(ctx,
                                   keywords,
                                   GRN_TEXT_VALUE(&keyword),
                                   GRN_TEXT_LEN(&keyword),
                                   0,
                                   GRN_DB_TEXT);
            GRN_BULK_REWIND(&keyword);
          }
          break;
        case '\\' :
          escaping = GRN_TRUE;
          break;
        default :
          GRN_TEXT_PUTC(ctx, &keyword, regexp_raw[0]);
          break;
        }
      }
    } else {
      escaping = GRN_FALSE;
      GRN_TEXT_PUT(ctx, &keyword, regexp_raw, char_len);
    }

    regexp_raw += char_len;
  }

  if (GRN_TEXT_LEN(&keyword) > 0) {
    grn_vector_add_element(ctx,
                           keywords,
                           GRN_TEXT_VALUE(&keyword),
                           GRN_TEXT_LEN(&keyword),
                           0,
                           GRN_DB_TEXT);
  }

  GRN_OBJ_FIN(ctx, &keyword);
}

grn_rc
grn_expr_get_keywords(grn_ctx *ctx, grn_obj *expr, grn_obj *keywords)
{
  int i, n;
  scan_info **sis, *si;
  GRN_API_ENTER;
  if ((sis = grn_scan_info_build(ctx, expr, &n, GRN_OP_OR, GRN_FALSE))) {
    int butp = 0, nparens = 0, npbut = 0;
    grn_obj but_stack;
    GRN_UINT32_INIT(&but_stack, GRN_OBJ_VECTOR);
    for (i = n; i--;) {
      si = sis[i];
      if (si->flags & SCAN_POP) {
        nparens++;
        if (si->logical_op == GRN_OP_AND_NOT) {
          GRN_UINT32_PUT(ctx, &but_stack, npbut);
          npbut = nparens;
          butp = 1 - butp;
        }
      } else {
        if (butp == (si->logical_op == GRN_OP_AND_NOT) &&
            grn_obj_is_text_family_bulk(ctx, si->query)) {
          switch (si->op) {
          case GRN_OP_MATCH :
            if (keywords->header.type == GRN_PVECTOR) {
              GRN_PTR_PUT(ctx, keywords, si->query);
            } else {
              grn_vector_add_element(ctx,
                                     keywords,
                                     GRN_TEXT_VALUE(si->query),
                                     GRN_TEXT_LEN(si->query),
                                     0,
                                     GRN_DB_TEXT);
            }
            break;
          case GRN_OP_REGEXP :
            /* TODO: It should be refined. */
            if (is_index_searchable_regexp(ctx, si->query)) {
              if (keywords->header.type == GRN_PVECTOR) {
                GRN_PTR_PUT(ctx, keywords, si->query);
              } else {
                grn_expr_get_keywords_regexp(ctx, keywords, si->query);
              }
            }
            break;
          case GRN_OP_SIMILAR :
          case GRN_OP_QUORUM :
            if (keywords->header.type == GRN_VECTOR &&
                GRN_BULK_VSIZE(&(si->index)) > 0) {
              grn_token_cursor *token_cursor;
              unsigned int token_flags = 0;
              grn_obj *index = GRN_PTR_VALUE(&(si->index));
              grn_obj *lexicon;

              lexicon = grn_ctx_at(ctx, index->header.domain);
              token_cursor = grn_token_cursor_open(ctx,
                                                   lexicon,
                                                   GRN_TEXT_VALUE(si->query),
                                                   GRN_TEXT_LEN(si->query),
                                                   GRN_TOKENIZE_GET,
                                                   token_flags);
              if (token_cursor) {
                grn_obj *source_table;
                uint32_t n_records_threshold;
                source_table = grn_ctx_at(ctx, grn_obj_get_range(ctx, index));
                n_records_threshold = grn_table_size(ctx, source_table) / 2;
                while (token_cursor->status != GRN_TOKEN_CURSOR_DONE) {
                  grn_id token_id;
                  uint32_t n_estimated_records;
                  token_id = grn_token_cursor_next(ctx, token_cursor);
                  if (token_id == GRN_ID_NIL) {
                    continue;
                  }
                  n_estimated_records =
                    grn_ii_estimate_size(ctx, (grn_ii *)index, token_id);
                  if (n_estimated_records >= n_records_threshold) {
                    continue;
                  }
                  grn_vector_add_element(ctx,
                                         keywords,
                                         token_cursor->curr,
                                         token_cursor->curr_size,
                                         0,
                                         GRN_DB_TEXT);
                }
                grn_token_cursor_close(ctx, token_cursor);
              }
            }
            break;
          default :
            break;
          }
        }
        if (si->flags & SCAN_PUSH) {
          if (nparens == npbut) {
            butp = 1 - butp;
            GRN_UINT32_POP(&but_stack, npbut);
          }
          nparens--;
        }
      }
    }
    GRN_OBJ_FIN(ctx, &but_stack);
    for (i = n; i--;) { SI_FREE(sis[i]); }
    GRN_FREE(sis);
  }
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_rc
grn_expr_snip_add_conditions(grn_ctx *ctx, grn_obj *expr, grn_obj *snip,
                             unsigned int n_tags,
                             const char **opentags, unsigned int *opentag_lens,
                             const char **closetags, unsigned int *closetag_lens)
{
  grn_rc rc;
  grn_obj keywords;

  GRN_API_ENTER;

  GRN_PTR_INIT(&keywords, GRN_OBJ_VECTOR, GRN_ID_NIL);
  rc = grn_expr_get_keywords(ctx, expr, &keywords);
  if (rc != GRN_SUCCESS) {
    GRN_OBJ_FIN(ctx, &keywords);
    GRN_API_RETURN(rc);
  }

  if (n_tags) {
    int i;
    for (i = 0;; i = (i + 1) % n_tags) {
      grn_obj *keyword;
      GRN_PTR_POP(&keywords, keyword);
      if (!keyword) { break; }
      grn_snip_add_cond(ctx, snip,
                        GRN_TEXT_VALUE(keyword), GRN_TEXT_LEN(keyword),
                        opentags[i], opentag_lens[i],
                        closetags[i], closetag_lens[i]);
    }
  } else {
    for (;;) {
      grn_obj *keyword;
      GRN_PTR_POP(&keywords, keyword);
      if (!keyword) { break; }
      grn_snip_add_cond(ctx, snip,
                        GRN_TEXT_VALUE(keyword), GRN_TEXT_LEN(keyword),
                        NULL, 0, NULL, 0);
    }
  }
  GRN_OBJ_FIN(ctx, &keywords);

  GRN_API_RETURN(GRN_SUCCESS);
}

grn_obj *
grn_expr_snip(grn_ctx *ctx, grn_obj *expr, int flags,
              unsigned int width, unsigned int max_results,
              unsigned int n_tags,
              const char **opentags, unsigned int *opentag_lens,
              const char **closetags, unsigned int *closetag_lens,
              grn_snip_mapping *mapping)
{
  grn_obj *res = NULL;
  GRN_API_ENTER;
  if ((res = grn_snip_open(ctx, flags, width, max_results,
                           NULL, 0, NULL, 0, mapping))) {
    grn_expr_snip_add_conditions(ctx, expr, res,
                                 n_tags,
                                 opentags, opentag_lens,
                                 closetags, closetag_lens);
  }
  GRN_API_RETURN(res);
}

/*
  So far, grn_column_filter() is nothing but a very rough prototype.
  Although GRN_COLUMN_EACH() can accelerate many range queries,
  the following stuff must be resolved one by one.

  * support accessors as column
  * support tables which have deleted records
  * support various operators
  * support various column types
*/
grn_rc
grn_column_filter(grn_ctx *ctx, grn_obj *column,
                  grn_operator operator,
                  grn_obj *value, grn_obj *result_set,
                  grn_operator set_operation)
{
  uint32_t *vp;
  grn_posting_internal posting = {0};
  uint32_t value_ = grn_atoui(GRN_TEXT_VALUE(value), GRN_BULK_CURR(value), NULL);
  posting.sid = 1;
  posting.pos = 0;
  posting.weight_float = 1;
  GRN_COLUMN_EACH(ctx, column, id, vp, {
    if (*vp < value_) {
      posting.rid = id;
      grn_ii_posting_add_float(ctx,
                               (grn_posting *)(&posting),
                               (grn_hash *)result_set,
                               set_operation);
    }
  });
  grn_ii_resolve_sel_and(ctx, (grn_hash *)result_set, set_operation);
  return ctx->rc;
}

grn_rc
grn_expr_syntax_escape(grn_ctx *ctx, const char *string, int string_size,
                       const char *target_characters,
                       char escape_character,
                       grn_obj *escaped_string)
{
  grn_rc rc = GRN_SUCCESS;
  const char *current, *string_end;

  if (!string) {
    return GRN_INVALID_ARGUMENT;
  }

  GRN_API_ENTER;
  if (string_size < 0) {
    string_size = strlen(string);
  }
  string_end = string + string_size;

  current = string;
  while (current < string_end) {
    unsigned int char_size;
    char_size = grn_charlen(ctx, current, string_end);
    switch (char_size) {
    case 0 :
      /* string includes malformed multibyte character. */
      return GRN_INVALID_ARGUMENT;
      break;
    case 1 :
      if (strchr(target_characters, *current)) {
        GRN_TEXT_PUTC(ctx, escaped_string, escape_character);
      }
      GRN_TEXT_PUT(ctx, escaped_string, current, char_size);
      current += char_size;
      break;
    default :
      GRN_TEXT_PUT(ctx, escaped_string, current, char_size);
      current += char_size;
      break;
    }
  }

  GRN_API_RETURN(rc);
}

grn_rc
grn_expr_syntax_escape_query(grn_ctx *ctx, const char *query, int query_size,
                             grn_obj *escaped_query)
{
  const char target_characters[] = {
    GRN_QUERY_AND,
    GRN_QUERY_AND_NOT,
    GRN_QUERY_ADJ_INC,
    GRN_QUERY_ADJ_DEC,
    GRN_QUERY_ADJ_NEG,
    GRN_QUERY_PREFIX,
    GRN_QUERY_PARENL,
    GRN_QUERY_PARENR,
    GRN_QUERY_QUOTEL,
    GRN_QUERY_ESCAPE,
    GRN_QUERY_COLUMN,
    '\0',
  };
  return grn_expr_syntax_escape(ctx, query, query_size,
                                target_characters, GRN_QUERY_ESCAPE,
                                escaped_query);
}

grn_rc
grn_expr_dump_plan(grn_ctx *ctx, grn_obj *expr, grn_obj *buffer)
{
  grn_scanner *scanner;

  GRN_API_ENTER;
  scanner = grn_scanner_open(ctx, expr, GRN_OP_OR, GRN_FALSE);
  if (scanner) {
    grn_inspect_scan_info_list(ctx, buffer, scanner->sis, scanner->n_sis);
    grn_scanner_close(ctx, scanner);
  } else {
    GRN_TEXT_PUTS(ctx, buffer, "sequential search\n");
  }
  GRN_API_RETURN(GRN_SUCCESS);
}

static unsigned int
grn_expr_estimate_size_raw(grn_ctx *ctx, grn_obj *expr, grn_obj *table)
{
  return grn_table_size(ctx, table);
}

unsigned int
grn_expr_estimate_size(grn_ctx *ctx, grn_obj *expr)
{
  grn_obj *table;
  grn_obj *variable;
  unsigned int size;

  variable = grn_expr_get_var_by_offset(ctx, expr, 0);
  if (!variable) {
    ERR(GRN_INVALID_ARGUMENT, "at least one variable must be defined");
    return 0;
  }

  table = grn_ctx_at(ctx, variable->header.domain);
  if (!table) {
    ERR(GRN_INVALID_ARGUMENT,
        "variable refers unknown domain: <%u>", variable->header.domain);
    return 0;
  }

  GRN_API_ENTER;
#ifdef GRN_WITH_MRUBY
  grn_ctx_impl_mrb_ensure_init(ctx);
  if (ctx->rc != GRN_SUCCESS) {
    GRN_API_RETURN(0);
  }
  if (ctx->impl->mrb.state) {
    size = grn_mrb_expr_estimate_size(ctx, expr, table);
  } else {
    size = grn_expr_estimate_size_raw(ctx, expr, table);
  }
#else
  size = grn_expr_estimate_size_raw(ctx, expr, table);
#endif
  GRN_API_RETURN(size);
}

grn_bool
grn_expr_is_simple_function_call(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *codes = e->codes;
  grn_expr_code *codes_end = codes + e->codes_curr;

  if (codes == codes_end) {
    return GRN_FALSE;
  }

  for (; codes < codes_end; codes++) {
    switch (codes[0].op) {
    case GRN_OP_PUSH :
      break;
    case GRN_OP_CALL :
      if (codes + 1 != codes_end) {
        return GRN_FALSE;
      }
      break;
    default :
      return GRN_FALSE;
    }
  }

  return GRN_TRUE;
}

grn_obj *
grn_expr_simple_function_call_get_function(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;

  return e->codes[0].value;
}

grn_rc
grn_expr_simple_function_call_get_arguments(grn_ctx *ctx,
                                            grn_obj *expr,
                                            grn_obj *arguments)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *codes = e->codes;
  grn_expr_code *codes_end = codes + e->codes_curr;

  for (codes++; codes < codes_end - 1; codes++) {
    grn_obj *value = codes[0].value;
    switch (codes[0].op) {
    case GRN_OP_PUSH :
      grn_vector_add_element(ctx,
                             arguments,
                             GRN_BULK_HEAD(value),
                             GRN_BULK_VSIZE(value),
                             0,
                             value->header.domain);
      break;
    default :
      return GRN_INVALID_ARGUMENT;
      break;
    }
  }

  return GRN_SUCCESS;
}

grn_bool
grn_expr_is_module_list(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *codes = e->codes;
  grn_expr_code *codes_end = codes + e->codes_curr;

  if (codes == codes_end) {
    return GRN_FALSE;
  }

  for (; codes < codes_end; codes++) {
    switch (codes[0].op) {
    case GRN_OP_PUSH :
      break;
    case GRN_OP_CALL :
      if (codes + 1 != codes_end) {
        return GRN_FALSE;
      }
      break;
    case GRN_OP_COMMA :
      break;
    default :
      return GRN_FALSE;
    }
  }

  return GRN_TRUE;
}

unsigned int
grn_expr_module_list_get_n_modules(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *codes = e->codes;
  grn_expr_code *codes_end = codes + e->codes_curr;
  unsigned int n = 1;

  for (; codes < codes_end; codes++) {
    if (codes[0].op == GRN_OP_COMMA) {
      n++;
    }
  }

  return n;
}

static void
grn_expr_module_list_detect_module(grn_ctx *ctx,
                                   grn_obj *expr,
                                   unsigned int i,
                                   grn_expr_code **module_start,
                                   grn_expr_code **module_end)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *codes = e->codes;
  grn_expr_code *codes_end = codes + e->codes_curr;
  unsigned int j = 0;

  *module_start = codes;
  *module_end = codes_end;

  if (i == 0) {
    for (codes = e->codes; codes < codes_end; codes++) {
      switch (codes[0].op) {
      case GRN_OP_CALL :
        *module_start = codes - codes[0].nargs;
        *module_end = codes;
        return;
      case GRN_OP_COMMA :
        if (codes[-1].op == GRN_OP_CALL) {
          *module_start = codes - codes[-1].nargs - 1;
          *module_end = codes - codes[-1].nargs;
        } else {
          *module_start = codes - 2;
          *module_end = codes - 1;
        }
        return;
      default :
        break;
      }
    }
    return;
  } else {
    for (codes = e->codes; codes < codes_end; codes++) {
      if (codes[0].op != GRN_OP_COMMA) {
        continue;
      }
      j++;
      if (i == j) {
        if (codes > e->codes && codes[-1].op == GRN_OP_CALL) {
          *module_start = codes - codes[-1].nargs;
          *module_end = codes - 1;
        } else {
          *module_start = codes - 1;
          *module_end = codes;
        }
        return;
      }
    }
  }

  *module_start = NULL;
  *module_end = NULL;
}

grn_obj *
grn_expr_module_list_get_function(grn_ctx *ctx,
                                  grn_obj *expr,
                                  unsigned int i)
{
  grn_expr_code *module_start;
  grn_expr_code *module_end;

  grn_expr_module_list_detect_module(ctx, expr, i, &module_start, &module_end);

  if (module_start) {
    return module_start[0].value;
  } else {
    return NULL;
  }
}

grn_rc
grn_expr_module_list_get_arguments(grn_ctx *ctx,
                                   grn_obj *expr,
                                   unsigned int i,
                                   grn_obj *arguments)
{
  grn_expr_code *codes;
  grn_expr_code *module_start;
  grn_expr_code *module_end;

  grn_expr_module_list_detect_module(ctx, expr, i, &module_start, &module_end);

  for (codes = module_start + 1; codes < module_end; codes++) {
    grn_obj *value = codes[0].value;
    switch (codes[0].op) {
    case GRN_OP_PUSH :
      grn_vector_add_element(ctx,
                             arguments,
                             GRN_BULK_HEAD(value),
                             GRN_BULK_VSIZE(value),
                             0,
                             value->header.domain);
      break;
    default :
      return GRN_INVALID_ARGUMENT;
      break;
    }
  }

  return GRN_SUCCESS;
}

grn_rc
grn_expr_set_query_log_tag_prefix(grn_ctx *ctx,
                                  grn_obj *expr,
                                  const char *prefix,
                                  int prefix_len)
{
  GRN_API_ENTER;
  if (prefix_len < 0) {
    if (prefix) {
      prefix_len = strlen(prefix);
    } else {
      prefix_len = 0;
    }
  }

  grn_expr *e = (grn_expr *)expr;
  if (prefix_len == 0) {
    GRN_BULK_REWIND(&(e->query_log_tag_prefix));
  } else {
    GRN_TEXT_SET(ctx, &(e->query_log_tag_prefix), prefix, prefix_len);
  }
  GRN_TEXT_PUTC(ctx, &(e->query_log_tag_prefix), '\0');

  GRN_API_RETURN(GRN_SUCCESS);
}

const char *
grn_expr_get_query_log_tag_prefix(grn_ctx *ctx, grn_obj *expr)
{
  GRN_API_ENTER;
  grn_expr *e = (grn_expr *)expr;
  const char *prefix = GRN_TEXT_VALUE(&(e->query_log_tag_prefix));
  GRN_API_RETURN(prefix);
}

grn_rc
grn_expr_set_parent(grn_ctx *ctx,
                    grn_obj *expr,
                    grn_obj *parent)
{
  GRN_API_ENTER;
  grn_expr *e = (grn_expr *)expr;
  e->parent = parent;
  GRN_API_RETURN(GRN_SUCCESS);
}

grn_obj *
grn_expr_get_parent(grn_ctx *ctx, grn_obj *expr)
{
  GRN_API_ENTER;
  grn_expr *e = (grn_expr *)expr;
  GRN_API_RETURN(e->parent);
}
