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

#ifndef __GRN_ASSERTIONS_H__
#define __GRN_ASSERTIONS_H__

#include "grn-test-utils.h"
#include "grn-test-shortcut.h"

#define grn_test_assert(expression, ...)                        \
  cut_trace_with_info_expression(                               \
    cut_test_with_user_message(                                 \
      grn_test_assert_helper((expression), #expression),        \
      __VA_ARGS__),                                             \
    grn_test_assert(expression))

#define grn_test_assert_equal_rc(expected, actual, ...)         \
  cut_trace_with_info_expression(                               \
    cut_test_with_user_message(                                 \
      grn_test_assert_equal_rc_helper((expected), (actual),     \
                                      #expected, #actual),      \
      __VA_ARGS__),                                             \
    grn_test_assert_equal_rc(expected, actual))

#define grn_test_assert_equal_id(context, expected, actual, ...)        \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_equal_id_helper((context),                        \
                                      (expected), (actual),             \
                                      #context,                         \
                                      #expected, #actual),              \
      __VA_ARGS__),                                                     \
    grn_test_assert_equal_id(context, expected, actual))

#define grn_test_assert_equal_type(context, expected, actual, ...)      \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_equal_type_helper((context),                      \
                                        (expected), (actual),           \
                                        #context,                       \
                                        #expected, #actual),            \
      __VA_ARGS__),                                                     \
    grn_test_assert_equal_type(context, expected, actual))

#define grn_test_assert_equal_record_id(context, table,                 \
                                        expected, actual, ...)          \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_equal_record_id_helper((context), (table),        \
                                             (expected), (actual),      \
                                             #context, #table,          \
                                             #expected, #actual),       \
      __VA_ARGS__),                                                     \
    grn_test_assert_equal_record_id(context, table, expected, actual))

#define grn_test_assert_nil(expression, ...)                    \
  cut_trace_with_info_expression(                               \
    cut_test_with_user_message(                                 \
      grn_test_assert_nil_helper((expression), #expression),    \
      __VA_ARGS__),                                             \
    grn_test_assert_nil(expression))

#define grn_test_assert_not_nil(expression, ...)                        \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_not_nil_helper((expression), #expression),        \
      __VA_ARGS__),                                                     \
    grn_test_assert_not_nil(expression))

#define grn_test_assert_context(context, ...)                   \
  cut_trace_with_info_expression(                               \
    cut_test_with_user_message(                                 \
      grn_test_assert_context_helper((context), #context),      \
      __VA_ARGS__),                                             \
    grn_test_assert_context(context))

#define grn_test_assert_error(rc, message, context, ...)                \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_error_helper((rc), (message), (context), #context), \
      __VA_ARGS__),                                                     \
    grn_test_assert_error(context))

#define grn_test_assert_null(context, object, ...)                      \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_null_helper((context), (object), #object),        \
      __VA_ARGS__),                                                     \
    grn_test_assert_null(context, object))

#define grn_test_assert_not_null(context, object, ...)                  \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_not_null_helper((context), (object), #object),    \
      __VA_ARGS__),                                                     \
    grn_test_assert_not_null(context, object))

#define grn_test_assert_select(context, expected, select_result,        \
                               text_column_name, ...)                   \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_select_helper((context), (expected),              \
                                    (select_result),                    \
                                    (text_column_name),                 \
                                    #expected,                          \
                                    #select_result, #text_column_name), \
      __VA_ARGS__),                                                     \
    grn_test_assert_select(context, expected, select_result))

#define grn_test_assert_expr(context, inspected, expr, ...)             \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_expr_helper((context), (inspected), (expr),       \
                                  #inspected, #expr),                   \
      __VA_ARGS__),                                                     \
    grn_test_assert_expr(context, inspected, expr))

#define grn_test_assert_equal_encoding(expected, actual, ...)           \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_equal_encoding_helper((expected), (actual),       \
                                            #expected, #actual),        \
      __VA_ARGS__),                                                     \
    grn_test_assert_equal_encoding(expected, actual))

#define grn_test_assert_equal_table(context, expected, table,           \
                                    text_column_name, ...)              \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_equal_table_helper((context), (expected),         \
                                         (table), (text_column_name),   \
                                         #expected, #table,             \
                                         #text_column_name),            \
      __VA_ARGS__),                                                     \
    grn_test_assert_equal_table(context, expected, table))

#define grn_test_assert_send_command_error(context, expected_rc,        \
                                           expected_message, command,   \
                                           ...)                         \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_assert_send_command_error_helper((context),              \
                                                (expected_rc),          \
                                                (expected_message),     \
                                                (command),              \
                                                #expected_rc,           \
                                                #expected_message,      \
                                                #command),              \
      __VA_ARGS__),                                                     \
    grn_test_assert_send_command_error(context,                         \
                                       expected_rc,                     \
                                       expected_message,                \
                                       command))

G_BEGIN_DECLS

void     grn_test_assert_helper         (grn_rc       rc,
                                         const gchar *expression);
void     grn_test_assert_equal_rc_helper(grn_rc       expected,
                                         grn_rc       actual,
                                         const gchar *expression_expected,
                                         const gchar *expression_actual);
void     grn_test_assert_equal_id_helper(grn_ctx     *context,
                                         grn_id       expected,
                                         grn_id       actual,
                                         const gchar *expression_context,
                                         const gchar *expression_expected,
                                         const gchar *expression_actual);
void     grn_test_assert_equal_type_helper
                                        (grn_ctx     *context,
                                         unsigned char expected,
                                         unsigned char actual,
                                         const gchar *expression_context,
                                         const gchar *expression_expected,
                                         const gchar *expression_actual);
void     grn_test_assert_equal_record_id_helper
                                        (grn_ctx     *context,
                                         grn_obj     *table,
                                         grn_id       expected,
                                         grn_id       actual,
                                         const gchar *expression_context,
                                         const gchar *expression_table,
                                         const gchar *expression_expected,
                                         const gchar *expression_actual);
void     grn_test_assert_nil_helper     (grn_id       id,
                                         const gchar *expression);
void     grn_test_assert_not_nil_helper (grn_id       id,
                                         const gchar *expression);
void     grn_test_assert_context_helper (grn_ctx     *context,
                                         const gchar *expression);
void     grn_test_assert_error_helper   (grn_rc       expected_rc,
                                         const gchar *expected_message,
                                         grn_ctx     *context,
                                         const gchar *expression);
void     grn_test_assert_null_helper    (grn_ctx     *context,
                                         grn_obj     *object,
                                         const gchar *expression);
void     grn_test_assert_not_null_helper(grn_ctx     *context,
                                         grn_obj     *object,
                                         const gchar *expression);
void     grn_test_assert_select_helper  (grn_ctx     *context,
                                         const GList *expected,
                                         grn_obj     *select_result,
                                         const gchar *text_column_name,
                                         const gchar *expected_expression,
                                         const gchar *select_result_expression,
                                         const gchar *text_column_name_expression);
void     grn_test_assert_expr_helper    (grn_ctx     *context,
                                         const gchar *inspected,
                                         grn_obj     *expr,
                                         const gchar *inspected_expression,
                                         const gchar *expr_expression);
void     grn_test_assert_equal_encoding_helper
                                        (grn_encoding expected,
                                         grn_encoding actual,
                                         const gchar *expression_expected,
                                         const gchar *expression_actual);
void     grn_test_assert_equal_table_helper
                                        (grn_ctx     *context,
                                         const GList *expected,
                                         grn_obj     *table,
                                         const gchar *text_column_name,
                                         const gchar *expected_expression,
                                         const gchar *select_result_expression,
                                         const gchar *text_column_name_expression);
void     grn_test_assert_send_command_error_helper
                                        (grn_ctx     *context,
                                         grn_rc       expected_rc,
                                         const gchar *expected_message,
                                         const gchar *command,
                                         const gchar *expected_rc_expression,
                                         const gchar *expected_message_expression,
                                         const gchar *command_expression);

G_END_DECLS

#endif
