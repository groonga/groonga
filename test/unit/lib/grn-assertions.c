/*
  Copyright (C) 2008-2012  Kouhei Sutou <kou@clear-code.com>

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

#include <cutter/cut-helper.h>
#include <gcutter.h>

#include <grn.h>
#include <grn_db.h>
#include <grn_util.h>
#include <grn_str.h>

#include "grn-assertions.h"

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
      cut_take_printf("%d: <%.*s>",
                      expected,
                      (int)GRN_TEXT_LEN(&inspected_expected_object),
                      GRN_TEXT_VALUE(&inspected_expected_object)));
    cut_set_actual(
      cut_take_printf("%d: <%.*s>",
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
grn_test_assert_equal_type_helper(grn_ctx *context,
                                  unsigned char expected,
                                  unsigned char actual,
                                  const gchar *expression_context,
                                  const gchar *expression_expected,
                                  const gchar *expression_actual)
{
  if (expected == actual) {
    cut_test_pass();
  } else {
    grn_obj inspected_expected, inspected_actual;

    GRN_TEXT_INIT(&inspected_expected, 0);
    GRN_TEXT_INIT(&inspected_actual, 0);
    grn_inspect_type(context, &inspected_expected, expected);
    grn_inspect_type(context, &inspected_actual, actual);
    cut_set_expected(
      cut_take_printf("%x: <%.*s>",
                      expected,
                      (int)GRN_TEXT_LEN(&inspected_expected),
                      GRN_TEXT_VALUE(&inspected_expected)));
    cut_set_actual(
      cut_take_printf("%x: <%.*s>",
                      actual,
                      (int)GRN_TEXT_LEN(&inspected_actual),
                      GRN_TEXT_VALUE(&inspected_actual)));
    grn_obj_unlink(context, &inspected_expected);
    grn_obj_unlink(context, &inspected_actual);
    cut_test_fail(cut_take_printf("<%s> == <%s> (%s)\n"
                                  " context: <%p>",
                                  expression_expected,
                                  expression_actual,
                                  expression_context,
                                  context));
  }
}

void
grn_test_assert_equal_record_id_helper(grn_ctx *context, grn_obj *table,
                                       grn_id expected, grn_id actual,
                                       const gchar *expression_context,
                                       const gchar *expression_table,
                                       const gchar *expression_expected,
                                       const gchar *expression_actual)
{
  if (expected == actual) {
    cut_test_pass();
  } else {
    gchar key[GRN_TABLE_MAX_KEY_SIZE];
    int key_size;
    const gchar *message;
    grn_obj inspected_table;

    key_size = grn_table_get_key(context, table, expected, key, sizeof(key));
    cut_set_expected(cut_take_printf("%d: <%.*s>", expected, key_size, key));
    key_size = grn_table_get_key(context, table, actual, key, sizeof(key));
    cut_set_actual(cut_take_printf("%d: <%.*s>", actual, key_size, key));

    GRN_TEXT_INIT(&inspected_table, 0);
    grn_inspect(context, &inspected_table, table);
    message = cut_take_printf("<%s> == <%s> (context: <%s>, table: <%s>)\n"
                              " context: <%p>\n"
                              "   table: <%.*s>",
                              expression_expected,
                              expression_actual,
                              expression_context,
                              expression_table,
                              context,
                              (int)GRN_TEXT_LEN(&inspected_table),
                              GRN_TEXT_VALUE(&inspected_table));
    grn_obj_unlink(context, &inspected_table);
    cut_test_fail(message);
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
                                  grn_encoding_to_string(expected),
                                  grn_encoding_to_string(actual)));
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
grn_test_assert_send_command_error_helper (grn_ctx     *context,
                                           grn_rc       expected_rc,
                                           const gchar *expected_message,
                                           const gchar *command,
                                           const gchar *expected_rc_expression,
                                           const gchar *expected_message_expression,
                                           const gchar *command_expression)
{
  const gchar **lines;

  lines = cut_take_string_array(g_strsplit(command, "\n", 0));
  for (; *lines; lines++) {
    grn_ctx_send(context, *lines, strlen(*lines), 0);
    if (context->rc) {
      break;
    }
  }

  if (context->rc == expected_rc &&
      cut_equal_string(expected_message, context->errbuf)) {
    gchar *command_result;
    unsigned int command_result_length;
    int flags = 0;

    cut_test_pass();
    grn_ctx_recv(context, &command_result, &command_result_length, &flags);
  } else {
    cut_set_expected(cut_take_printf("<%s>(%s)",
                                     expected_message,
                                     grn_rc_to_string(expected_rc)));
    cut_set_actual(cut_take_printf("<%s>(%s)",
                                   context->errbuf,
                                   grn_rc_to_string(context->rc)));
    cut_test_fail(cut_take_printf("<send(\"%s\")>\n"
                                  "%s:%d: %s():",
                                  command_expression,
                                  context->errfile, context->errline,
                                  context->errfunc));
  }
}
