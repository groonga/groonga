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

#include <cutter/cut-helper.h>
#include <gcutter.h>
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
