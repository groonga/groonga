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

  if (n_args < 2) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s wrong number of arguments (%d for 1..)",
        tag,
        n_args - 1);
    return ctx->rc;
  }

  if (((n_args - 1) % 2) != 1) {
    ERR(GRN_INVALID_ARGUMENT,
        "%s wrong number of arguments (%d for 1, 3, 5, ...)",
        tag,
        n_args - 1);
    return ctx->rc;
  }

  grn_obj *sub_result_set =
    grn_table_create(ctx,
                     NULL, 0,
                     NULL,
                     GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC,
                     table,
                     NULL);

  int i;
  for (i = 1; i < n_args; i += 2) {
    grn_obj *condition_text = args[i];
    if (!grn_obj_is_text_family_bulk(ctx, condition_text)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, condition_text);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the %dth argument must be condition as string: %.*s",
          tag,
          (i - 1),
          (int)GRN_TEXT_LEN(&inspected),
          GRN_TEXT_VALUE(&inspected));
      GRN_OBJ_FIN(ctx, &inspected);
      goto exit;
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
                   (unsigned int)GRN_TEXT_LEN(condition_text),
                   NULL,
                   GRN_OP_MATCH,
                   GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    if (ctx->rc != GRN_SUCCESS) {
      grn_obj_close(ctx, condition);
      goto exit;
    }

    grn_table_select(ctx, table, condition, sub_result_set, GRN_OP_OR);
    grn_obj_close(ctx, condition);
    if (ctx->rc != GRN_SUCCESS) {
      goto exit;
    }

    if ((i + 1) == n_args) {
      break;
    }

    grn_obj *threashold = args[i + 1];
    if (!grn_obj_is_number_family_bulk(ctx, threashold)) {
      grn_obj inspected;
      GRN_TEXT_INIT(&inspected, 0);
      grn_inspect(ctx, &inspected, threashold);
      ERR(GRN_INVALID_ARGUMENT,
          "%s the %dth argument must be threshold as number: %.*s",
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
       (int64_t)grn_table_size(ctx, sub_result_set) <= raw_threshold);
    if (!need_to_evaluate_condition) {
      break;
    }
  }

  grn_table_setoperation(ctx,
                         result_set,
                         sub_result_set,
                         result_set,
                         op);

exit :
  if (sub_result_set) {
    grn_obj_close(ctx, sub_result_set);
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
