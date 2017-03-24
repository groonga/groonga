/* -*- c-basic-offset: 2 -*- */
/*
  Copyright(C) 2010-2017 Brazil

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
#include "grn_expr_executor.h"

#ifdef GRN_WITH_ONIGMO
# define GRN_SUPPORT_REGEXP
#endif

#ifdef GRN_SUPPORT_REGEXP
# include "grn_normalizer.h"
# include <onigmo.h>
#endif

typedef union {
  struct {
    grn_obj result_buffer;
  } constant;
  struct {
    grn_obj *column;
    grn_obj value_buffer;
  } value;
#ifdef GRN_SUPPORT_REGEXP
  struct {
    grn_obj result_buffer;
    OnigRegex regex;
    grn_obj value_buffer;
    grn_obj *normalizer;
  } simple_regexp;
#endif /* GRN_SUPPORT_REGEXP */
  struct {
    grn_bool need_exec;
    grn_obj result_buffer;
    grn_obj value_buffer;
    grn_obj constant_buffer;
    grn_bool (*exec)(grn_ctx *ctx, grn_obj *x, grn_obj *y);
  } simple_condition;
} grn_expr_executor_data;

typedef grn_obj *(*grn_expr_executor_exec_func)(grn_ctx *ctx,
                                                grn_expr_executor *executor,
                                                grn_id id);
typedef void (*grn_expr_executor_fin_func)(grn_ctx *ctx,
                                           grn_expr_executor *executor);

struct _grn_expr_executor {
  grn_obj *expr;
  grn_obj *variable;
  grn_expr_executor_exec_func exec;
  grn_expr_executor_fin_func fin;
  grn_expr_executor_data data;
};

static void
grn_expr_executor_init_general(grn_ctx *ctx,
                               grn_expr_executor *executor)
{
}

static grn_obj *
grn_expr_executor_exec_general(grn_ctx *ctx,
                               grn_expr_executor *executor,
                               grn_id id)
{
  GRN_RECORD_SET(ctx, executor->variable, id);
  return grn_expr_exec(ctx, executor->expr, 0);
}

static void
grn_expr_executor_fin_general(grn_ctx *ctx,
                              grn_expr_executor *executor)
{
}

static grn_bool
grn_expr_executor_is_constant(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *target;

  if (e->codes_curr != 1) {
    return GRN_FALSE;
  }

  target = &(e->codes[0]);

  if (target->op != GRN_OP_PUSH) {
    return GRN_FALSE;
  }
  if (!target->value) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static void
grn_expr_executor_init_constant(grn_ctx *ctx,
                                grn_expr_executor *executor)
{
  grn_obj *result_buffer = &(executor->data.constant.result_buffer);
  grn_obj *result;

  GRN_VOID_INIT(result_buffer);
  result = grn_expr_exec(ctx, executor->expr, 0);
  if (ctx->rc == GRN_SUCCESS) {
    grn_obj_reinit(ctx,
                   result_buffer,
                   result->header.domain,
                   result->header.flags);
    /* TODO: Support vector */
    grn_bulk_write(ctx,
                   result_buffer,
                   GRN_BULK_HEAD(result),
                   GRN_BULK_VSIZE(result));
  }
}

static grn_obj *
grn_expr_executor_exec_constant(grn_ctx *ctx,
                                grn_expr_executor *executor,
                                grn_id id)
{
  return &(executor->data.constant.result_buffer);
}

static void
grn_expr_executor_fin_constant(grn_ctx *ctx,
                               grn_expr_executor *executor)
{
  GRN_OBJ_FIN(ctx, &(executor->data.constant.result_buffer));
}

static grn_bool
grn_expr_executor_is_value(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *target;

  if (e->codes_curr != 1) {
    return GRN_FALSE;
  }

  target = &(e->codes[0]);

  if (target->op != GRN_OP_GET_VALUE) {
    return GRN_FALSE;
  }
  if (!target->value) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static void
grn_expr_executor_init_value(grn_ctx *ctx,
                             grn_expr_executor *executor)
{
  grn_expr *e = (grn_expr *)(executor->expr);

  executor->data.value.column = e->codes[0].value;
  GRN_VOID_INIT(&(executor->data.value.value_buffer));
}

static grn_obj *
grn_expr_executor_exec_value(grn_ctx *ctx,
                             grn_expr_executor *executor,
                             grn_id id)
{
  grn_obj *value_buffer = &(executor->data.value.value_buffer);

  GRN_BULK_REWIND(value_buffer);
  grn_obj_get_value(ctx, executor->data.value.column, id, value_buffer);

  return value_buffer;
}

static void
grn_expr_executor_fin_value(grn_ctx *ctx,
                            grn_expr_executor *executor)
{
  GRN_OBJ_FIN(ctx, &(executor->data.value.value_buffer));
}

#ifdef GRN_SUPPORT_REGEXP
static grn_bool
grn_expr_executor_is_simple_regexp(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *target;
  grn_expr_code *pattern;
  grn_expr_code *operator;

  if (e->codes_curr != 3) {
    return GRN_FALSE;
  }

  target = &(e->codes[0]);
  pattern = &(e->codes[1]);
  operator = &(e->codes[2]);

  if (operator->op != GRN_OP_REGEXP) {
    return GRN_FALSE;
  }
  if (operator->nargs != 2) {
    return GRN_FALSE;
  }

  if (target->op != GRN_OP_GET_VALUE) {
    return GRN_FALSE;
  }
  if (target->nargs != 1) {
    return GRN_FALSE;
  }
  if (!target->value) {
    return GRN_FALSE;
  }
  if (target->value->header.type != GRN_COLUMN_VAR_SIZE) {
    return GRN_FALSE;
  }
  if ((target->value->header.flags & GRN_OBJ_COLUMN_TYPE_MASK) !=
      GRN_OBJ_COLUMN_SCALAR) {
    return GRN_FALSE;
  }
  switch (grn_obj_get_range(ctx, target->value)) {
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    break;
  default :
    return GRN_FALSE;
  }

  if (pattern->op != GRN_OP_PUSH) {
    return GRN_FALSE;
  }
  if (pattern->nargs != 1) {
    return GRN_FALSE;
  }
  if (!pattern->value) {
    return GRN_FALSE;
  }
  if (pattern->value->header.type != GRN_BULK) {
    return GRN_FALSE;
  }
  switch (pattern->value->header.domain) {
  case GRN_DB_SHORT_TEXT :
  case GRN_DB_TEXT :
  case GRN_DB_LONG_TEXT :
    break;
  default :
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static void
grn_expr_executor_init_simple_regexp(grn_ctx *ctx,
                                     grn_expr_executor *executor)
{
  grn_expr *e = (grn_expr *)(executor->expr);
  grn_obj *result_buffer = &(executor->data.simple_regexp.result_buffer);
  OnigEncoding onig_encoding;
  int onig_result;
  OnigErrorInfo onig_error_info;
  grn_obj *pattern;

  GRN_BOOL_INIT(result_buffer, 0);
  GRN_BOOL_SET(ctx, result_buffer, GRN_FALSE);

  if (ctx->encoding == GRN_ENC_NONE) {
    executor->data.simple_regexp.regex = NULL;
    return;
  }

  switch (ctx->encoding) {
  case GRN_ENC_EUC_JP :
    onig_encoding = ONIG_ENCODING_EUC_JP;
    break;
  case GRN_ENC_UTF8 :
    onig_encoding = ONIG_ENCODING_UTF8;
    break;
  case GRN_ENC_SJIS :
    onig_encoding = ONIG_ENCODING_CP932;
    break;
  case GRN_ENC_LATIN1 :
    onig_encoding = ONIG_ENCODING_ISO_8859_1;
    break;
  case GRN_ENC_KOI8R :
    onig_encoding = ONIG_ENCODING_KOI8_R;
    break;
  default :
    executor->data.simple_regexp.regex = NULL;
    return;
  }

  pattern = e->codes[1].value;
  onig_result = onig_new(&(executor->data.simple_regexp.regex),
                         GRN_TEXT_VALUE(pattern),
                         GRN_TEXT_VALUE(pattern) + GRN_TEXT_LEN(pattern),
                         ONIG_OPTION_ASCII_RANGE |
                         ONIG_OPTION_MULTILINE,
                         onig_encoding,
                         ONIG_SYNTAX_RUBY,
                         &onig_error_info);
  if (onig_result != ONIG_NORMAL) {
    char message[ONIG_MAX_ERROR_MESSAGE_LEN];
    onig_error_code_to_str(message, onig_result, onig_error_info);
    ERR(GRN_INVALID_ARGUMENT,
        "[expr-executor][regexp] "
        "failed to create regular expression object: <%.*s>: %s",
        (int)GRN_TEXT_LEN(pattern), GRN_TEXT_VALUE(pattern),
        message);
    return;
  }

  GRN_VOID_INIT(&(executor->data.simple_regexp.value_buffer));

  executor->data.simple_regexp.normalizer =
    grn_ctx_get(ctx, GRN_NORMALIZER_AUTO_NAME, -1);
}

static grn_obj *
grn_expr_executor_exec_simple_regexp(grn_ctx *ctx,
                                     grn_expr_executor *executor,
                                     grn_id id)
{
  grn_expr *e = (grn_expr *)(executor->expr);
  OnigRegex regex = executor->data.simple_regexp.regex;
  grn_obj *value_buffer = &(executor->data.simple_regexp.value_buffer);
  grn_obj *result_buffer = &(executor->data.simple_regexp.result_buffer);

  if (ctx->rc) {
    GRN_BOOL_SET(ctx, result_buffer, GRN_FALSE);
    return result_buffer;
  }

  if (!regex) {
    return result_buffer;
  }

  grn_obj_reinit_for(ctx, value_buffer, e->codes[0].value);
  grn_obj_get_value(ctx, e->codes[0].value, id, value_buffer);
  {
    grn_obj *norm_target;
    const char *norm_target_raw;
    unsigned int norm_target_raw_length_in_bytes;

    norm_target = grn_string_open(ctx,
                                  GRN_TEXT_VALUE(value_buffer),
                                  GRN_TEXT_LEN(value_buffer),
                                  executor->data.simple_regexp.normalizer,
                                  0);
    grn_string_get_normalized(ctx, norm_target,
                              &norm_target_raw,
                              &norm_target_raw_length_in_bytes,
                              NULL);

    {
      OnigPosition position;
      position = onig_search(regex,
                             norm_target_raw,
                             norm_target_raw + norm_target_raw_length_in_bytes,
                             norm_target_raw,
                             norm_target_raw + norm_target_raw_length_in_bytes,
                             NULL,
                             ONIG_OPTION_NONE);
      grn_obj_close(ctx, norm_target);
      GRN_BOOL_SET(ctx, result_buffer, (position != ONIG_MISMATCH));
      return result_buffer;
    }
  }
}

static void
grn_expr_executor_fin_simple_regexp(grn_ctx *ctx,
                                    grn_expr_executor *executor)
{
  GRN_OBJ_FIN(ctx, &(executor->data.simple_regexp.result_buffer));

  if (!executor->data.simple_regexp.regex) {
    return;
  }

  onig_free(executor->data.simple_regexp.regex);
  GRN_OBJ_FIN(ctx, &(executor->data.simple_regexp.value_buffer));
}
#endif /* GRN_SUPPORT_REGEXP */

static grn_bool
grn_expr_executor_is_simple_condition(grn_ctx *ctx, grn_obj *expr)
{
  grn_expr *e = (grn_expr *)expr;
  grn_expr_code *target;
  grn_expr_code *constant;
  grn_expr_code *operator;

  if (e->codes_curr != 3) {
    return GRN_FALSE;
  }

  target = &(e->codes[0]);
  constant = &(e->codes[1]);
  operator = &(e->codes[2]);

  switch (operator->op) {
  case GRN_OP_EQUAL :
  case GRN_OP_NOT_EQUAL :
  case GRN_OP_LESS :
  case GRN_OP_GREATER :
  case GRN_OP_LESS_EQUAL :
  case GRN_OP_GREATER_EQUAL :
    break;
  default :
    return GRN_FALSE;
  }
  if (operator->nargs != 2) {
    return GRN_FALSE;
  }

  if (target->op != GRN_OP_GET_VALUE) {
    return GRN_FALSE;
  }
  if (target->nargs != 1) {
    return GRN_FALSE;
  }
  if (!grn_obj_is_scalar_column(ctx, target->value)) {
    return GRN_FALSE;
  }

  if (constant->op != GRN_OP_PUSH) {
    return GRN_FALSE;
  }
  if (constant->nargs != 1) {
    return GRN_FALSE;
  }
  if (!constant->value) {
    return GRN_FALSE;
  }
  if (constant->value->header.type != GRN_BULK) {
    return GRN_FALSE;
  }

  return GRN_TRUE;
}

static void
grn_expr_executor_init_simple_condition(grn_ctx *ctx,
                                        grn_expr_executor *executor)
{
  grn_expr *e = (grn_expr *)(executor->expr);
  grn_obj *target;
  grn_obj *constant;
  grn_operator op;
  grn_obj *result_buffer;
  grn_obj *value_buffer;
  grn_obj *constant_buffer;
  grn_rc rc;

  target = e->codes[0].value;
  constant = e->codes[1].value;
  op = e->codes[2].op;

  executor->data.simple_condition.need_exec = GRN_TRUE;

  result_buffer = &(executor->data.simple_condition.result_buffer);
  GRN_BOOL_INIT(result_buffer, 0);
  GRN_BOOL_SET(ctx, result_buffer, GRN_FALSE);

  value_buffer = &(executor->data.simple_condition.value_buffer);
  GRN_VOID_INIT(value_buffer);
  grn_obj_reinit_for(ctx, value_buffer, target);

  switch (op) {
  case GRN_OP_EQUAL :
    executor->data.simple_condition.exec = grn_operator_exec_equal;
    break;
  case GRN_OP_NOT_EQUAL :
    executor->data.simple_condition.exec = grn_operator_exec_not_equal;
    break;
  case GRN_OP_LESS :
    executor->data.simple_condition.exec = grn_operator_exec_less;
    break;
  case GRN_OP_GREATER :
    executor->data.simple_condition.exec = grn_operator_exec_greater;
    break;
  case GRN_OP_LESS_EQUAL :
    executor->data.simple_condition.exec = grn_operator_exec_less_equal;
    break;
  case GRN_OP_GREATER_EQUAL :
    executor->data.simple_condition.exec = grn_operator_exec_greater_equal;
    break;
  default :
    break;
  }

  constant_buffer = &(executor->data.simple_condition.constant_buffer);
  GRN_VOID_INIT(constant_buffer);
  grn_obj_reinit_for(ctx, constant_buffer, target);
  rc = grn_obj_cast(ctx, constant, constant_buffer, GRN_FALSE);
  if (rc != GRN_SUCCESS) {
    grn_obj *type;

    type = grn_ctx_at(ctx, constant_buffer->header.domain);
    if (grn_obj_is_table(ctx, type)) {
      GRN_BOOL_SET(ctx, result_buffer, (op == GRN_OP_NOT_EQUAL));
      executor->data.simple_condition.need_exec = GRN_FALSE;
    } else {
      int type_name_size;
      char type_name[GRN_TABLE_MAX_KEY_SIZE];
      grn_obj inspected;

      type_name_size = grn_obj_name(ctx, type, type_name,
                                    GRN_TABLE_MAX_KEY_SIZE);
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, constant);
      ERR(rc,
          "[expr-executor][condition] "
          "failed to cast to <%.*s>: <%.*s>",
          type_name_size, type_name,
          (int)GRN_TEXT_LEN(&inspected), GRN_TEXT_VALUE(&inspected));
    }
  }
}

static grn_obj *
grn_expr_executor_exec_simple_condition(grn_ctx *ctx,
                                        grn_expr_executor *executor,
                                        grn_id id)
{
  grn_expr *e = (grn_expr *)(executor->expr);
  grn_obj *target;
  grn_obj *result_buffer = &(executor->data.simple_condition.result_buffer);
  grn_obj *value_buffer = &(executor->data.simple_condition.value_buffer);
  grn_obj *constant_buffer = &(executor->data.simple_condition.constant_buffer);

  if (ctx->rc) {
    GRN_BOOL_SET(ctx, result_buffer, GRN_FALSE);
    return result_buffer;
  }

  if (!executor->data.simple_condition.need_exec) {
    return result_buffer;
  }

  target = e->codes[0].value;
  GRN_BULK_REWIND(value_buffer);
  grn_obj_get_value(ctx, target, id, value_buffer);

  if (executor->data.simple_condition.exec(ctx, value_buffer, constant_buffer)) {
    GRN_BOOL_SET(ctx, result_buffer, GRN_TRUE);
  } else {
    GRN_BOOL_SET(ctx, result_buffer, GRN_FALSE);
  }
  return result_buffer;
}

static void
grn_expr_executor_fin_simple_condition(grn_ctx *ctx,
                                       grn_expr_executor *executor)
{
  GRN_OBJ_FIN(ctx, &(executor->data.simple_condition.result_buffer));
  GRN_OBJ_FIN(ctx, &(executor->data.simple_condition.value_buffer));
  GRN_OBJ_FIN(ctx, &(executor->data.simple_condition.constant_buffer));
}

grn_expr_executor *
grn_expr_executor_open(grn_ctx *ctx, grn_obj *expr)
{
  grn_obj *variable;
  grn_expr_executor *executor;

  GRN_API_ENTER;

  if (!grn_obj_is_expr(ctx, expr)) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, expr);
    ERR(ctx->rc,
        "[expr-executor][open] invalid expression: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(NULL);
  }

  variable = grn_expr_get_var_by_offset(ctx, expr, 0);
  if (!variable) {
    grn_obj inspected;
    GRN_TEXT_INIT(&inspected, 0);
    grn_inspect(ctx, &inspected, expr);
    ERR(ctx->rc,
        "[expr-executor][open] expression has no variable: %.*s",
        (int)GRN_TEXT_LEN(&inspected),
        GRN_TEXT_VALUE(&inspected));
    GRN_OBJ_FIN(ctx, &inspected);
    GRN_API_RETURN(NULL);
  }

  executor = GRN_CALLOC(sizeof(grn_expr_executor));
  if (!executor) {
    ERR(ctx->rc,
        "[expr-executor][open] failed to allocate: %s",
        ctx->errbuf);
    GRN_API_RETURN(NULL);
  }

  executor->expr = expr;
  executor->variable = variable;
  if (grn_expr_executor_is_constant(ctx, expr)) {
    grn_expr_executor_init_constant(ctx, executor);
    executor->exec = grn_expr_executor_exec_constant;
    executor->fin = grn_expr_executor_fin_constant;
  } else if (grn_expr_executor_is_value(ctx, expr)) {
    grn_expr_executor_init_value(ctx, executor);
    executor->exec = grn_expr_executor_exec_value;
    executor->fin = grn_expr_executor_fin_value;
#ifdef GRN_SUPPORT_REGEXP
  } else if (grn_expr_executor_is_simple_regexp(ctx, expr)) {
    grn_expr_executor_init_simple_regexp(ctx, executor);
    executor->exec = grn_expr_executor_exec_simple_regexp;
    executor->fin = grn_expr_executor_fin_simple_regexp;
#endif /* GRN_SUPPORT_REGEXP */
  } else if (grn_expr_executor_is_simple_condition(ctx, expr)) {
    grn_expr_executor_init_simple_condition(ctx, executor);
    executor->exec = grn_expr_executor_exec_simple_condition;
    executor->fin = grn_expr_executor_fin_simple_condition;
  } else {
    grn_expr_executor_init_general(ctx, executor);
    executor->exec = grn_expr_executor_exec_general;
    executor->fin = grn_expr_executor_fin_general;
  }

  GRN_API_RETURN(executor);
}

grn_obj *
grn_expr_executor_exec(grn_ctx *ctx, grn_expr_executor *executor, grn_id id)
{
  grn_obj *value;

  GRN_API_ENTER;

  if (!executor) {
    GRN_API_RETURN(NULL);
  }

  value = executor->exec(ctx, executor, id);

  GRN_API_RETURN(value);
}

grn_rc
grn_expr_executor_close(grn_ctx *ctx, grn_expr_executor *executor)
{
  GRN_API_ENTER;

  if (!executor) {
    GRN_API_RETURN(GRN_SUCCESS);
  }

  executor->fin(ctx, executor);
  GRN_FREE(executor);

  GRN_API_RETURN(GRN_SUCCESS);
}
