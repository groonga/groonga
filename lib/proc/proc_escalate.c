/*
  Copyright(C) 2022  Sutou Kouhei <kou@clear-code.com>

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

#include "../grn_ctx.h"
#include "../grn_output.h"
#include "../grn_proc.h"

#include <groonga/plugin.h>

static grn_rc
selector_escalate(grn_ctx *ctx,
                  grn_obj *table,
                  grn_obj *index,
                  int n_args,
                  grn_obj **args,
                  grn_obj *result_set,
                  grn_operator op)
{
  const char *tag = "[escalate]";

  if (((n_args - 1) % 2) != 0) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s wrong number of arguments (%d for 0, 2, 4, 6, ...)",
        tag,
        n_args - 1);
    return ctx->rc;
  }

  if (n_args == 1) {
    return ctx->rc;
  }

  grn_obj *current_result_set = result_set;
  grn_obj *aggregated_result_set = NULL;

  int i;
  for (i = 1; i < n_args; i += 2) {
    grn_obj *threashold = args[i];
    grn_obj *condition_text = args[i + 1];

    if (!grn_obj_is_number_family_bulk(ctx, threashold)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, threashold);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the %dth argument must be threshold as number: %.*s",
          tag,
          i - 1,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }

    if (!grn_obj_is_text_family_bulk(ctx, condition_text)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, condition_text);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the %dth argument must be condition as string: %.*s",
          tag,
          (i - 1) + 1,
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
    }

    grn_obj casted_threshold;
    GRN_INT64_INIT(&casted_threshold, 0);
    grn_obj_cast(ctx, threashold, &casted_threshold, false);
    int64_t raw_threshold = GRN_INT64_VALUE(&casted_threshold);
    GRN_OBJ_FIN(ctx, &casted_threshold);
    bool need_to_evaluate_condition =
      (raw_threshold < 0 ||
       (int64_t)grn_table_size(ctx, current_result_set) <= raw_threshold);
    if (!need_to_evaluate_condition) {
      break;
    }

    grn_obj *dummy_variable;
    grn_obj *condition;
    GRN_EXPR_CREATE_FOR_QUERY(ctx, table, condition, dummy_variable);
    if (!condition) {
      goto exit;
    }

    grn_expr_parse(ctx,
                   condition,
                   GRN_TEXT_VALUE(condition_text),
                   GRN_TEXT_LEN(condition_text),
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_close(ctx, condition);
      goto exit;
    }

    grn_obj *target_result_set = NULL;
    if (op == GRN_OP_OR) {
      target_result_set = result_set;
    } else {
      /* We use a copy of the original result set as the base result set
       * to reduce result set size as much as possible. It's for
       * performance. */
      target_result_set =
        grn_table_create(ctx,
                         NULL, 0,
                         NULL,
                         GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                         table,
                         NULL);
      if (!target_result_set) {
        grn_obj_close(ctx, condition);
        goto exit;
      }
      grn_rc rc = grn_result_set_copy(ctx,
                                      (grn_hash *)result_set,
                                      (grn_hash *)target_result_set);
      if (rc != GRN_SUCCESS) {
        grn_obj_close(ctx, target_result_set);
        grn_obj_close(ctx, condition);
        goto exit;
      }
    }
    grn_operator select_op = op;
    if (op == GRN_OP_AND_NOT) {
      /* We need to use AND for AND_NOT because
       *
       *   (BASE &! (((BASE &! A) || (BASE &! B))))
       *
       * doesn't equal to
       *
       *   (BASE &! (A || B))
       *
       * . If we use AND here, the expression is the following:
       *
       *   (BASE &! ((BASE && A) || (BASE && B)))
       *
       * And it equals to
       *
       *   (BASE &! (A || B))
       *
       * . */
      select_op = GRN_OP_AND;
    }
    grn_table_select(ctx, table, condition, target_result_set, select_op);
    grn_obj_close(ctx, condition);
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_close(ctx, target_result_set);
      goto exit;
    }

    if (op != GRN_OP_OR) {
      if (!aggregated_result_set) {
        aggregated_result_set = target_result_set;
      } else {
        grn_table_setoperation(ctx,
                               aggregated_result_set,
                               target_result_set,
                               aggregated_result_set,
                               GRN_OP_OR);
        grn_obj_close(ctx, target_result_set);
        if (ctx->rc != GRN_SUCCESS) {
          goto exit;
        }
      }
      if (current_result_set != result_set) {
        grn_obj_close(ctx, current_result_set);
      }
      current_result_set =
        grn_table_create(ctx,
                         NULL, 0,
                         NULL,
                         GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                         table,
                         NULL);
      grn_rc rc = grn_result_set_copy(ctx,
                                      (grn_hash *)result_set,
                                      (grn_hash *)current_result_set);
      if (rc != GRN_SUCCESS) {
        goto exit;
      }
      grn_table_setoperation(ctx,
                             current_result_set,
                             aggregated_result_set,
                             current_result_set,
                             op);
      if (ctx->rc != GRN_SUCCESS) {
        goto exit;
      }
    }
  }

  if (op != GRN_OP_OR && aggregated_result_set) {
    grn_table_setoperation(ctx,
                           result_set,
                           aggregated_result_set,
                           result_set,
                           op);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }
  }

exit :
  if (aggregated_result_set) {
    grn_obj_close(ctx, aggregated_result_set);
  }
  if (current_result_set != result_set) {
    grn_obj_close(ctx, current_result_set);
  }

  return ctx->rc;
}

void
grn_proc_init_escalate(grn_ctx *ctx)
{
  grn_obj *selector_proc;

  selector_proc = grn_proc_create(ctx,
                                  "escalate", -1,
                                  GRN_PROC_FUNCTION,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0,
                                  NULL);
  grn_proc_set_selector(ctx, selector_proc, selector_escalate);
  grn_proc_set_selector_operator(ctx, selector_proc, GRN_OP_NOP);
}
