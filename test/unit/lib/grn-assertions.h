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

#ifndef __GRN_ASSERTIONS_H__
#define __GRN_ASSERTIONS_H__

#include "grn-test-utils.h"

#define grn_test_assert(expression)                     \
  cut_trace_with_info_expression(                       \
    grn_test_assert_helper((expression), #expression),  \
    grn_test_assert(expression))

#define grn_test_assert_equal_rc(expected, actual)              \
  cut_trace_with_info_expression(                               \
    grn_test_assert_equal_rc_helper((expected), (actual),       \
                                    #expected, #actual),        \
    grn_test_assert_equal_rc(expected, actual))

#define grn_test_assert_nil(expression)                         \
  cut_trace_with_info_expression(                               \
    grn_test_assert_nil_helper((expression), #expression),      \
    grn_test_assert_nil(expression))

#define grn_test_assert_not_nil(expression)                     \
  cut_trace_with_info_expression(                               \
    grn_test_assert_not_nil_helper((expression), #expression),  \
    grn_test_assert_not_nil(expression))

#define grn_test_assert_context(expression)                     \
  cut_trace_with_info_expression(                               \
    grn_test_assert_context_helper((expression), #expression),  \
    grn_test_assert_context(expression))


void     grn_test_assert_helper         (grn_rc       rc,
                                         const gchar *expression);
void     grn_test_assert_equal_rc_helper(grn_rc       expected,
                                         grn_rc       actual,
                                         const gchar *expression_expected,
                                         const gchar *expression_actual);
void     grn_test_assert_nil_helper     (grn_id       id,
                                         const gchar *expression);
void     grn_test_assert_not_nil_helper (grn_id       id,
                                         const gchar *expression);
void     grn_test_assert_context_helper (grn_ctx     *context,
                                         const gchar *expression);

#endif
