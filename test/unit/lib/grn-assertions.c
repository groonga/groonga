/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cutter/cut-helper.h>
#include <gcutter.h>
#include "grn-assertions.h"

#include <groonga_in.h>
#include <util.h>
#include <str.h>

grn_rc grn_expr_inspect(grn_ctx *ctx, grn_obj *buf, grn_obj *expr);

void
grn_test_assert_helper(grn_rc rc, const gchar *expression)
{
  if (rc == GRN_SUCCESS) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("expected: <%s> == grn_success\n"
                                  " but was: <%s>",
                                  expression,
                                  grn_rc_to_string(rc)));
  }
}

void
grn_test_assert_equal_rc_helper(grn_rc expected, grn_rc actual,
                                const gchar *expression_expected,
                                const gchar *expression_actual)
{
  if (expected == actual) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("<%s> == <%s>\n"
                                  "expected: <%s>\n"
                                  " but was: <%s>",
                                  expression_expected,
                                  expression_actual,
                                  grn_rc_to_string(expected),
                                  grn_rc_to_string(actual)));
  }
}

void
grn_test_assert_equal_id_helper(grn_ctx *context,
                                grn_id expected, grn_id actual,
                                const gchar *expression_context,
                                const gchar *expression_expected,
                                const gchar *expression_actual)
{
  if (expected == actual) {
    cut_test_pass();
  } else {
    grn_obj *expected_object, *actual_object;
    grn_obj inspected_expected_object, inspected_actual_object;

    expected_object = grn_ctx_at(context, expected);
    actual_object = grn_ctx_at(context, actual);
    GRN_TEXT_INIT(&inspected_expected_object, 0);
    GRN_TEXT_INIT(&inspected_actual_object, 0);
    grn_inspect(context, &inspected_expected_object, expected_object);
    grn_inspect(context, &inspected_actual_object, actual_object);
    cut_set_expected(
      cut_take_printf("%d: %.*s",
                      expected,
                      (int)GRN_TEXT_LEN(&inspected_expected_object),
                      GRN_TEXT_VALUE(&inspected_expected_object)));
    cut_set_actual(
      cut_take_printf("%d: %.*s",
                      actual,
                      (int)GRN_TEXT_LEN(&inspected_actual_object),
                      GRN_TEXT_VALUE(&inspected_actual_object)));
    grn_obj_unlink(context, &inspected_expected_object);
    grn_obj_unlink(context, &inspected_actual_object);
    grn_obj_unlink(context, expected_object);
    grn_obj_unlink(context, actual_object);
    cut_test_fail(cut_take_printf("<%s> == <%s> (%s)\n"
                                  " context: <%p>",
                                  expression_expected,
                                  expression_actual,
                                  expression_context,
                                  context));
  }
}

void
grn_test_assert_nil_helper(grn_id id, const gchar *expression)
{
  if (id == GRN_ID_NIL) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("<%s> == <GRN_ID_NIL>\n"
                                  "expected: <%u>\n"
                                  " but was: <%u>",
                                  expression,
                                  GRN_ID_NIL, id));
  }
}

void
grn_test_assert_not_nil_helper(grn_id id, const gchar *expression)
{
  if (id != GRN_ID_NIL) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("<%s> != <GRN_ID_NIL>\n"
                                  "expected: <%u> is not <%u>",
                                  expression, id, GRN_ID_NIL));
  }
}

void
grn_test_assert_context_helper (grn_ctx *context, const gchar *expression)
{
  if (!context) {
    cut_test_with_user_message(
      cut_assert_null_helper(context, expression),
      cut_message("context should not NULL"));
  } else if (context->rc == GRN_SUCCESS) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("<(%s)->rc> != <GRN_SUCCESS>\n"
                                  "expected: <%s> is <%s>\n"
                                  "%s:%d: %s(): %s",
                                  expression,
                                  grn_rc_to_string(context->rc),
                                  grn_rc_to_string(GRN_SUCCESS),
                                  context->errfile, context->errline,
                                  context->errfunc, context->errbuf));
  }
}

void
grn_test_assert_error_helper (grn_rc expected_rc, const gchar *expected_message,
                              grn_ctx *context, const gchar *expression)
{
  if (!context) {
    cut_test_with_user_message(
      cut_assert_null_helper(context, expression),
      cut_message("context should not NULL"));
  } else if (context->rc == expected_rc &&
             cut_equal_string(expected_message, context->errbuf)) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("<%s>\n"
                                  "expected: <%s>(%s)\n"
                                  "  actual: <%s>(%s)\n"
                                  "%s:%d: %s():",
                                  expression,
                                  expected_message,
                                  grn_rc_to_string(expected_rc),
                                  context->errbuf,
                                  grn_rc_to_string(context->rc),
                                  context->errfile, context->errline,
                                  context->errfunc));
  }
}

void
grn_test_assert_null_helper (grn_ctx *context,
                             grn_obj *object, const gchar *expression)
{
  if (!object) {
    cut_test_pass();
  } else {
    GString *inspected;
    const gchar *taken_inspected;

    inspected = g_string_new(NULL);
    grn_test_object_inspect(inspected, context, object);
    taken_inspected = cut_take_string(inspected->str);
    cut_test_fail(cut_take_printf("expected: <%s> is NULL\n"
                                  "  actual: <%s>",
                                  expression,
                                  taken_inspected));
  }
}

void
grn_test_assert_not_null_helper (grn_ctx *context,
                                 grn_obj *object, const gchar *expression)
{
  if (object) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("expected: <%s> is not NULL: <%s>",
                                  expression,
                                  context->errbuf));
  }
}

void
grn_test_assert_select_helper (grn_ctx *context,
                               const GList *expected,
                               grn_obj *select_result,
                               const gchar *text_column_name,
                               const gchar *expected_expression,
                               const gchar *select_result_expression,
                               const gchar *text_column_name_expression)
{
  const GList *records;
  GList *sorted_records, *sorted_expected;

  records = grn_test_table_collect_string(context,
                                          select_result,
                                          text_column_name);

  sorted_expected = g_list_copy((GList *)expected);
  sorted_expected = g_list_sort(sorted_expected, (GCompareFunc)g_utf8_collate);
  gcut_take_list(sorted_expected, NULL);

  sorted_records = g_list_copy((GList *)records);
  sorted_records = g_list_sort(sorted_records, (GCompareFunc)g_utf8_collate);
  gcut_take_list(sorted_records, NULL);

  gcut_assert_equal_list_string(sorted_expected, sorted_records);
}

void
grn_test_assert_expr_helper (grn_ctx     *context,
                             const gchar *inspected,
                             grn_obj     *expr,
                             const gchar *inspected_expression,
                             const gchar *expr_expression)
{
  grn_obj buffer;
  const gchar *actual_inspected;

  GRN_TEXT_INIT(&buffer, 0);
  grn_expr_inspect(context, &buffer, expr);
  GRN_TEXT_PUTC(context, &buffer, '\0');
  actual_inspected = cut_take_strdup(GRN_TEXT_VALUE(&buffer));
  GRN_OBJ_FIN(context, &buffer);
  cut_assert_equal_string(inspected, actual_inspected);
}

void
grn_test_assert_equal_encoding_helper (grn_encoding expected,
                                       grn_encoding actual,
                                       const gchar *expression_expected,
                                       const gchar *expression_actual)
{
  if (expected == actual) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("<%s> == <%s>\n"
                                  "expected: <%s>\n"
                                  " but was: <%s>",
                                  expression_expected,
                                  expression_actual,
                                  grn_enctostr(expected),
                                  grn_enctostr(actual)));
  }
}

void
grn_test_assert_equal_table_helper (grn_ctx *context,
                                    const GList *expected,
                                    grn_obj *table,
                                    const gchar *text_column_name,
                                    const gchar *expected_expression,
                                    const gchar *select_result_expression,
                                    const gchar *text_column_name_expression)
{
  const GList *records;

  records = grn_test_table_collect_string(context, table, text_column_name);
  gcut_assert_equal_list_string(expected, records);
}

void
grn_test_assert_equal_view_helper (grn_ctx *context,
                                   const GList *expected,
                                   grn_obj *view,
                                   const gchar *text_column_name,
                                   const gchar *expected_expression,
                                   const gchar *view_expression,
                                   const gchar *text_column_name_expression)
{
  const GList *records;

  records = grn_test_view_collect_string(context, view, text_column_name);
  gcut_assert_equal_list_string(expected, records);
}
