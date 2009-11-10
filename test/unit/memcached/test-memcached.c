/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Tasuku SUENAGA <a@razil.jp>

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

#include <glib/gstdio.h>
#include "libmemcached/memcached.h"
#include <gcutter.h> /* must be included after memcached.h */

#include <unistd.h>

#include "../lib/grn-assertions.h"
#include "../lib/grn-test-memcached-assertions.h"
#include "../lib/grn-test-server.h"

/* globals */
static gchar *val1 = NULL, *val2 = NULL;
static size_t val1_len, val2_len;
static struct memcached_st *memc = NULL;
static struct memcached_server_st *servers = NULL;

static GrnTestServer *server;

void
cut_setup(void)
{
  GError *error = NULL;

  memc = NULL;
  servers = NULL;

  server = grn_test_server_new();
  grn_test_server_start(server, &error);
  gcut_assert_error(error);

  memc = memcached_create(NULL);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);

  servers =
    memcached_servers_parse(grn_test_server_get_memcached_address(server));
  grn_test_memcached_assert(memcached_server_push(memc, servers),
                            cut_message("memcached server connect failed."));

  memcached_flush(memc, 0); /* flush immediately for debug daemon */
}

void
cut_teardown(void)
{
  if (val1) { free(val1); val1 = NULL; }
  if (val2) { free(val2); val2 = NULL; }

  if (servers) {
    memcached_server_list_free(servers);
  }
  if (memc) {
    memcached_free(memc);
  }

  if (server) {
    g_object_unref(server);
  }
}

void
test_set_and_get(void)
{
  uint32_t flags;
  memcached_return rc;

  grn_test_memcached_assert(
    memcached_set(memc, "key", 3, "value", 5, 0, 0xdeadbeefU),
    cut_message("memcached set failed."));

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  grn_test_memcached_assert(rc, cut_message("memcached get failed."));
  cut_assert_equal_string("value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);
}

void
test_set_and_get_with_expire(void)
{
  uint32_t flags;
  memcached_return rc;
  const int timeout = 1;

  grn_test_memcached_assert(
    memcached_set(memc, "key", 1, "value", 5, timeout, 0xdeadbeefU),
    cut_message("memcached set with expiration failed."));

  val1 = memcached_get(memc, "key", 1, &val1_len, &flags, &rc);
  grn_test_memcached_assert(
    rc, cut_message("memcached get with expiration failed."));
  cut_assert_equal_string("value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);

  sleep(timeout + 1);

  val2 = memcached_get(memc, "key", 1, &val2_len, &flags, &rc);
  grn_test_memcached_assert_equal_rc(
    MEMCACHED_NOTFOUND, rc,
    cut_message("memcached get with expiration error."));
}

void
test_noop(void)
{
  /* TODO: implement */
  memcached_return rc = MEMCACHED_SUCCESS;
  grn_test_memcached_assert(rc);
}

void
test_memcached_add(void)
{
  uint32_t flags;
  memcached_return rc;

  grn_test_memcached_assert(
    memcached_add(memc, "key", 3, "value", 5, 0, 0xdeadbeefU),
    cut_message("memcached add failed."));

  grn_test_memcached_assert_equal_rc(
    MEMCACHED_NOTSTORED,
    memcached_add(memc, "key", 3, "new-value", 9, 0, 0xdeadbeefU),
    cut_message("memcached add succeeded."));

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  grn_test_memcached_assert(rc, cut_message("memcached get failed."));
  cut_assert_equal_string("value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);
}

void
test_memcached_replace(void)
{
  uint32_t flags;
  memcached_return rc;

  grn_test_memcached_assert_equal_rc(
    MEMCACHED_NOTSTORED,
    memcached_replace(memc, "key", 3, "value", 5, 0, 0xdeadbeefU),
    cut_message("memcached replace succeeded."));

  sleep(1);

  grn_test_memcached_assert(
    memcached_add(memc, "key", 3, "value", 5, 0, 0xdeadbeefU),
    cut_message("memcached add failed."));

  grn_test_memcached_assert(
    memcached_replace(memc, "key", 3, "new-value", 9, 0, 0xdeadbeefU),
    cut_message("memcached replace failed."));

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  grn_test_memcached_assert(rc, cut_message("memcached get failed."));
  cut_assert_equal_string("new-value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);
}

void
test_memcached_flush(void)
{
  uint32_t flags;
  memcached_return rc;

  grn_test_memcached_assert(
    memcached_set(memc, "key", 3, "to be flushed", 13, 0, 0xdeadbeefU),
    cut_message("memcached set failed."));

  memcached_flush(memc, 0);

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  grn_test_memcached_assert_equal_rc(
    MEMCACHED_NOTFOUND, rc,
    cut_message("memcached get succeeded."));
}

void
test_memcached_flush_with_time(void)
{
  const int sleep_time = 1;
  uint32_t flags;
  memcached_return rc;

  grn_test_memcached_assert(
    memcached_set(memc, "key", 3, "to be flushed", 13, 0, 0xdeadbeefU),
    cut_message("memcached set failed."));

  memcached_flush(memc, sleep_time);

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  grn_test_memcached_assert(rc, cut_message("memcached get failed."));
  cut_assert_equal_string("to be flushed", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);

  sleep(sleep_time + 1);

  val2 = memcached_get(memc, "key", 3, &val2_len, &flags, &rc);
  grn_test_memcached_assert_equal_rc(
    MEMCACHED_NOTFOUND, rc,
    cut_message("memcached get succeeded."));
}

void
test_memcached_cas(void)
{
  memcached_return rc;

  const char *key = "caskey";
  size_t key_len = strlen(key);
  const char* keys[2] = { (char *)key, NULL };
  size_t key_lens[2] = { key_len, 0 };

  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_SUPPORT_CAS, 1);
  grn_test_memcached_assert(
    memcached_set(memc, key, key_len, "cas test", 8, 0, 0xdeadbeefU),
    cut_message("memcached set failed."));

  {
    uint64_t cas;
    memcached_result_st *results;
    memcached_result_st results_obj;

    results = memcached_result_create(memc, &results_obj);
    grn_test_memcached_assert(
      memcached_mget(memc, keys, key_lens, 1),
      cut_message("memcached mget failed."));
    results = memcached_fetch_result(memc, &results_obj, &rc);
    cut_assert_not_null(results,
                        cut_message("memcached fetch result failed."));

    cas = memcached_result_cas(results);
    cut_assert_operator(cas, !=, 0,
                        cut_message("memcached cas value is non-zero."));

    grn_test_memcached_assert(
      memcached_cas(memc, key, key_len, "cas changed", 12, 0, 0, cas),
      cut_message("memcached cas failed."));

    /* TODO: fix rc after libmemcached fix */
    grn_test_memcached_assert_equal_rc(
      MEMCACHED_PROTOCOL_ERROR, /* MEMCACHED_DATA_EXISTS */
      memcached_cas(memc, key, key_len, "cas changed", 12, 0, 0, cas),
      cut_message("memcached cas value is same."));

    memcached_result_free(&results_obj);
  }
}

void
test_memcached_increment(void)
{
  uint64_t intval;

  grn_test_memcached_assert(
    memcached_increment_with_initial(memc, "incr", 4, 1, 30, 0, &intval));
  gcut_assert_equal_uint64(30, intval);

  grn_test_memcached_assert(
    memcached_increment_with_initial(memc, "incr", 4, 3, 0, 0, &intval));
  gcut_assert_equal_uint64(33, intval);

  fflush(stderr);
}

void
test_memcached_decrement(void)
{
  uint64_t intval;

  grn_test_memcached_assert(
    memcached_increment_with_initial(memc, "decr", 4, 30, 99, 0, &intval));
  gcut_assert_equal_uint64(99, intval);

  grn_test_memcached_assert(
    memcached_decrement_with_initial(memc, "decr", 4, 17, 0, 0, &intval));
  gcut_assert_equal_uint64(82, intval);
}
