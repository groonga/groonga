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

#include <gcutter.h>
#include "grn-test-memcached-assertions.h"

static const gchar *
grn_memcached_rc_to_string(memcached_return rc)
{
  switch (rc) {
  case MEMCACHED_SUCCESS:
    return "MEMCACHED_SUCCESS";
  case MEMCACHED_FAILURE:
    return "MEMCACHED_FAILURE";
  case MEMCACHED_HOST_LOOKUP_FAILURE:
    return "MEMCACHED_HOST_LOOKUP_FAILURE";
  case MEMCACHED_CONNECTION_FAILURE:
    return "MEMCACHED_CONNECTION_FAILURE";
  case MEMCACHED_CONNECTION_BIND_FAILURE:
    return "MEMCACHED_CONNECTION_BIND_FAILURE";
  case MEMCACHED_WRITE_FAILURE:
    return "MEMCACHED_WRITE_FAILURE";
  case MEMCACHED_READ_FAILURE:
    return "MEMCACHED_READ_FAILURE";
  case MEMCACHED_UNKNOWN_READ_FAILURE:
    return "MEMCACHED_UNKNOWN_READ_FAILURE";
  case MEMCACHED_PROTOCOL_ERROR:
    return "MEMCACHED_PROTOCOL_ERROR";
  case MEMCACHED_CLIENT_ERROR:
    return "MEMCACHED_CLIENT_ERROR";
  case MEMCACHED_SERVER_ERROR:
    return "MEMCACHED_SERVER_ERROR";
  case MEMCACHED_CONNECTION_SOCKET_CREATE_FAILURE:
    return "MEMCACHED_CONNECTION_SOCKET_CREATE_FAILURE";
  case MEMCACHED_DATA_EXISTS:
    return "MEMCACHED_DATA_EXISTS";
  case MEMCACHED_DATA_DOES_NOT_EXIST:
    return "MEMCACHED_DATA_DOES_NOT_EXIST";
  case MEMCACHED_NOTSTORED:
    return "MEMCACHED_NOTSTORED";
  case MEMCACHED_STORED:
    return "MEMCACHED_STORED";
  case MEMCACHED_NOTFOUND:
    return "MEMCACHED_NOTFOUND";
  case MEMCACHED_MEMORY_ALLOCATION_FAILURE:
    return "MEMCACHED_MEMORY_ALLOCATION_FAILURE";
  case MEMCACHED_PARTIAL_READ:
    return "MEMCACHED_PARTIAL_READ";
  case MEMCACHED_SOME_ERRORS:
    return "MEMCACHED_SOME_ERRORS";
  case MEMCACHED_NO_SERVERS:
    return "MEMCACHED_NO_SERVERS";
  case MEMCACHED_END:
    return "MEMCACHED_END";
  case MEMCACHED_DELETED:
    return "MEMCACHED_DELETED";
  case MEMCACHED_VALUE:
    return "MEMCACHED_VALUE";
  case MEMCACHED_STAT:
    return "MEMCACHED_STAT";
  case MEMCACHED_ITEM:
    return "MEMCACHED_ITEM";
  case MEMCACHED_ERRNO:
    return "MEMCACHED_ERRNO";
  case MEMCACHED_FAIL_UNIX_SOCKET:
    return "MEMCACHED_FAIL_UNIX_SOCKET";
  case MEMCACHED_NOT_SUPPORTED:
    return "MEMCACHED_NOT_SUPPORTED";
  case MEMCACHED_NO_KEY_PROVIDED:
    return "MEMCACHED_NO_KEY_PROVIDED";
  case MEMCACHED_FETCH_NOTFINISHED:
    return "MEMCACHED_FETCH_NOTFINISHED";
  case MEMCACHED_TIMEOUT:
    return "MEMCACHED_TIMEOUT";
  case MEMCACHED_BUFFERED:
    return "MEMCACHED_BUFFERED";
  case MEMCACHED_BAD_KEY_PROVIDED:
    return "MEMCACHED_BAD_KEY_PROVIDED";
  case MEMCACHED_INVALID_HOST_PROTOCOL:
    return "MEMCACHED_INVALID_HOST_PROTOCOL";
  case MEMCACHED_SERVER_MARKED_DEAD:
    return "MEMCACHED_SERVER_MARKED_DEAD";
  case MEMCACHED_UNKNOWN_STAT_KEY:
    return "MEMCACHED_UNKNOWN_STAT_KEY";
  case MEMCACHED_E2BIG:
    return "MEMCACHED_E2BIG";
  default:
    return "MEMCACHED_UNKNOWN_STATUS";
  }
}

void
grn_test_memcached_assert_helper(memcached_return rc, const gchar *expression)
{
  if (rc == MEMCACHED_SUCCESS) {
    cut_test_pass();
  } else {
    cut_test_fail(cut_take_printf("expected: <%s> == MEMCACHED_SUCCESS\n"
                                  " but was: <%s>",
                                  expression,
                                  grn_memcached_rc_to_string(rc)));
  }
}

void
grn_test_memcached_assert_equal_rc_helper(memcached_return expected,
                                          memcached_return actual,
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
                                  grn_memcached_rc_to_string(expected),
                                  grn_memcached_rc_to_string(actual)));
  }
}
