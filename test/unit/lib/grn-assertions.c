/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>

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

#include <str.h>

#include <cutter/cut-helper.h>
#include <gcutter.h>
#include "grn-assertions.h"

#include <groonga_in.h>
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
                               grn_obj *text_column,
                               const gchar *expected_expression,
                               const gchar *select_result_expression,
                               const gchar *text_column_expression)
{
  GList *records = NULL, *sorted_expected;
  grn_table_cursor *cursor;

  cursor = grn_table_cursor_open(context, select_result,
                                 NULL, 0, NULL, 0, 0, -1, 0);
  cut_assert_not_null(cursor);
  while (grn_table_cursor_next(context, cursor) != GRN_ID_NIL) {
    void *value;
    int size;
    grn_obj record_value;
    GString *null_terminated_key;

    grn_table_cursor_get_key(context, cursor, &value);
    GRN_TEXT_INIT(&record_value, 0);
    grn_obj_get_value(context, text_column, *((grn_id *)value), &record_value);
    value = GRN_TEXT_VALUE(&record_value);
    size = GRN_TEXT_LEN(&record_value);

    null_terminated_key = g_string_new_len(value, size);
    records = g_list_append(records, null_terminated_key->str);
    g_string_free(null_terminated_key, FALSE);
  }
  grn_test_assert(grn_table_cursor_close(context, cursor));

  sorted_expected = g_list_copy((GList *)expected);
  sorted_expected = g_list_sort(sorted_expected, (GCompareFunc)g_utf8_collate);
  gcut_take_list(sorted_expected, NULL);

  records = g_list_sort(records, (GCompareFunc)g_utf8_collate);
  gcut_take_list(records, g_free);

  gcut_assert_equal_list_string(sorted_expected, records);
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
