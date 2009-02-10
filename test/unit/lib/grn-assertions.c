/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

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
#include "sen-assertions.h"

void
sen_test_assert_helper(sen_rc rc, const gchar *expression,
                       const gchar *user_message_format, ...)
{
  if (rc == sen_success) {
    cut_test_pass();
  } else {
    cut_test_fail_va_list(cut_take_printf("expected: <%s> == sen_success\n"
                                          " but was: <%s>",
                                          expression,
                                          sen_rc_to_string(rc)),
                          user_message_format);
  }
}

void
sen_test_assert_equal_rc_helper(sen_rc expected, sen_rc actual,
                                const gchar *expression_expected,
                                const gchar *expression_actual,
                                const gchar *user_message_format, ...)
{
  if (expected == actual) {
    cut_test_pass();
  } else {
    cut_test_fail_va_list(cut_take_printf("<%s> == <%s>\n"
                                          "expected: <%s>\n"
                                          " but was: <%s>",
                                          expression_expected,
                                          expression_actual,
                                          sen_rc_to_string(expected),
                                          sen_rc_to_string(actual)),
                          user_message_format);
  }
}

void
sen_test_assert_nil_helper(sen_id id, const gchar *expression,
                           const gchar *user_message_format, ...)
{
  if (id == SEN_ID_NIL) {
    cut_test_pass();
  } else {
    cut_test_fail_va_list(cut_take_printf("<%s> == <SEN_ID_NI>\n"
                                          "expected: <%u>\n"
                                          " but was: <%u>",
                                          expression,
                                          SEN_ID_NIL, id),
                          user_message_format);
  }
}

void
sen_test_assert_not_nil_helper(sen_id id, const gchar *expression,
                               const gchar *user_message_format, ...)
{
  if (id != SEN_ID_NIL) {
    cut_test_pass();
  } else {
    cut_test_fail_va_list(cut_take_printf("<%s> != <SEN_ID_NIL>\n"
                                          "expected: <%u> is not <%u>",
                                          expression, id, SEN_ID_NIL),
                          user_message_format);
  }
}
