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

#ifndef __SEN_ASSERTIONS_H__
#define __SEN_ASSERTIONS_H__

#include "sen-test-utils.h"

#define sen_test_assert(expression, ...)                \
  cut_trace_with_info_expression(                       \
    sen_test_assert_helper((expression), #expression,   \
                          ## __VA_ARGS__, NULL),        \
    sen_test_assert(expression, ## __VA_ARGS__))

#define sen_test_assert_equal_rc(expected, actual, ...)         \
  cut_trace_with_info_expression(                               \
    sen_test_assert_equal_rc_helper((expected), (actual),       \
                                    #expected, #actual,         \
                                    ## __VA_ARGS__, NULL),      \
    sen_test_assert_equal_rc(expected, actual, ## __VA_ARGS__))

#define sen_test_assert_nil(expression, ...)                    \
  cut_trace_with_info_expression(                               \
    sen_test_assert_nil_helper((expression), #expression,       \
                              ## __VA_ARGS__, NULL),            \
    sen_test_assert_nil(expression, ## __VA_ARGS__))

#define sen_test_assert_not_nil(expression, ...)                \
  cut_trace_with_info_expression(                               \
    sen_test_assert_not_nil_helper((expression), #expression,   \
                                  ## __VA_ARGS__, NULL),        \
    sen_test_assert_not_nil(expression, ## __VA_ARGS__))


void     sen_test_assert_helper         (sen_rc       rc,
                                         const gchar *expression,
                                         const gchar *user_message_format,
                                         ...);
void     sen_test_assert_equal_rc_helper(sen_rc       expected,
                                         sen_rc       actual,
                                         const gchar *expression_expected,
                                         const gchar *expression_actual,
                                         const gchar *user_message_format,
                                         ...);
void     sen_test_assert_nil_helper     (sen_id       id,
                                         const gchar *expression,
                                         const gchar *user_message_format,
                                         ...);
void     sen_test_assert_not_nil_helper (sen_id       id,
                                         const gchar *expression,
                                         const gchar *user_message_format,
                                         ...);

#endif
