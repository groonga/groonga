/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

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

#ifndef __GRN_TEST_MEMCACHED_ASSERTIONS_H__
#define __GRN_TEST_MEMCACHED_ASSERTIONS_H__

#include <libmemcached/memcached.h>

#include "grn-test-utils.h"

#define grn_test_memcached_assert(expression, ...)                      \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_memcached_assert_helper((expression), #expression),      \
      __VA_ARGS__),                                                     \
    grn_test_assert(expression))

#define grn_test_memcached_assert_equal_rc(expected, actual, ...)       \
  cut_trace_with_info_expression(                                       \
    cut_test_with_user_message(                                         \
      grn_test_memcached_assert_equal_rc_helper((expected), (actual),   \
                                                #expected, #actual),    \
      __VA_ARGS__),                                                     \
    grn_test_assert_equal_rc(expected, actual))


void     grn_test_memcached_assert_helper      (memcached_return  rc,
                                                const gchar      *expression);
void     grn_test_memcached_assert_equal_rc_helper
                                               (memcached_return  expected,
                                                memcached_return  actual,
                                                const gchar      *expression_expected,
                                                const gchar      *expression_actual);

#endif
