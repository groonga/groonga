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

#include <unistd.h> /* for exec */
#include <sys/types.h>
#include <signal.h>

#include "../lib/grn-assertions.h"

#define GROONGA_TEST_PORT "4545"

/* globals */
static gchar *tmp_directory;
static gchar *val1 = NULL, *val2 = NULL;
static size_t val1_len, val2_len;
static struct memcached_st *memc = NULL;
static struct memcached_server_st *servers = NULL;

static GCutEgg *egg;

void
cut_setup(void)
{
  GError *error = NULL;
  memcached_return rc;

  tmp_directory = g_build_filename(grn_test_get_base_dir(), "tmp", NULL);
  cut_remove_path(tmp_directory, NULL);
  if (g_mkdir_with_parents(tmp_directory, 0700) == -1) {
    cut_assert_errno();
  }

  egg = gcut_egg_new(GROONGA, "-s",
                     "-p", GROONGA_TEST_PORT,
                     cut_take_printf("%s%s%s",
                                     tmp_directory,
                                     G_DIR_SEPARATOR_S,
                                     "memcached.db"),
                     NULL);
  gcut_egg_hatch(egg, &error);
  gcut_assert_error(error);

  sleep(1); /* wait for groonga daemon */
  memc = memcached_create(NULL);
  memcached_behavior_set(memc, MEMCACHED_BEHAVIOR_BINARY_PROTOCOL, 1);

  servers = memcached_servers_parse("localhost:" GROONGA_TEST_PORT);
  rc = memcached_server_push(memc, servers);

  cut_set_message("memcached server connect failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  memcached_flush(memc, 0); /* flush immediately for debug daemon */
}

void
cut_teardown(void)
{
  if (val1) { free(val1); val1 = NULL; }
  if (val2) { free(val2); val2 = NULL; }

  if (egg) {
    g_object_unref(egg);
  }

  cut_remove_path(tmp_directory, NULL);

  if (servers) {
    memcached_server_list_free(servers);
  }
  if (memc) {
    memcached_free(memc);
  }
}

void
test_set_and_get(void)
{
  uint32_t flags;
  memcached_return rc;

  rc = memcached_set(memc, "key", 3, "value", 5, 0, 0xdeadbeefU);
  cut_set_message("memcached set failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  cut_set_message("memcached get failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_equal_string("value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);
}

void
test_set_and_get_with_expire(void)
{
  uint32_t flags;
  memcached_return rc;
  const int timeout = 1;

  rc = memcached_set(memc, "key", 1, "value", 5, timeout, 0xdeadbeefU);
  cut_set_message("memcached set with expiration failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  val1 = memcached_get(memc, "key", 1, &val1_len, &flags, &rc);
  cut_set_message("memcached get with expiration failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_equal_string("value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);

  sleep(timeout + 1);

  val2 = memcached_get(memc, "key", 1, &val2_len, &flags, &rc);
  cut_set_message("memcached get with expiration error.");
  cut_assert_equal_int(MEMCACHED_NOTFOUND, rc);
}

void
test_noop(void)
{
  /* TODO: implement */
  memcached_return rc = MEMCACHED_SUCCESS;
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
}

void
test_memcached_add(void)
{
  uint32_t flags;
  memcached_return rc;

  rc = memcached_add(memc, "key", 3, "value", 5, 0, 0xdeadbeefU);
  cut_set_message("memcached add failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  rc = memcached_add(memc, "key", 3, "new-value", 9, 0, 0xdeadbeefU);
  cut_set_message("memcached add succeeded.");
  /* TODO: fix rc after libmemcached fix */
  cut_assert_equal_int(MEMCACHED_PROTOCOL_ERROR, rc);

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  cut_set_message("memcached get failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_equal_string("value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);
}

void
test_memcached_replace(void)
{
  uint32_t flags;
  memcached_return rc;

  rc = memcached_replace(memc, "key", 3, "value", 5, 0, 0xdeadbeefU);
  cut_set_message("memcached replace succeeded.");
  /* TODO: fix rc after libmemcached fix */
  cut_assert_equal_int(MEMCACHED_PROTOCOL_ERROR, rc);

  sleep(1);

  rc = memcached_add(memc, "key", 3, "value", 5, 0, 0xdeadbeefU);
  cut_set_message("memcached add failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  rc = memcached_replace(memc, "key", 3, "new-value", 9, 0, 0xdeadbeefU);
  cut_set_message("memcached replace failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  cut_set_message("memcached get failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_equal_string("new-value", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);
}

void
test_memcached_flush(void)
{
  uint32_t flags;
  memcached_return rc;

  rc = memcached_set(memc, "key", 3, "to be flushed", 13, 0, 0xdeadbeefU);
  cut_set_message("memcached set failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  memcached_flush(memc, 0);

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  cut_set_message("memcached get succeeded.");
  cut_assert_equal_int(MEMCACHED_NOTFOUND, rc);
}

void
test_memcached_flush_with_time(void)
{
  const int sleep_time = 1;
  uint32_t flags;
  memcached_return rc;

  rc = memcached_set(memc, "key", 3, "to be flushed", 13, 0, 0xdeadbeefU);
  cut_set_message("memcached set failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  memcached_flush(memc, sleep_time);

  val1 = memcached_get(memc, "key", 3, &val1_len, &flags, &rc);
  cut_set_message("memcached get failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_equal_string("to be flushed", val1);
  cut_assert_equal_uint(0xdeadbeefU, flags);

  sleep(sleep_time + 1);

  val2 = memcached_get(memc, "key", 3, &val2_len, &flags, &rc);
  cut_set_message("memcached get succeeded.");
  cut_assert_equal_int(MEMCACHED_NOTFOUND, rc);
}

/* FIXME: uncomment when libmemcached supports initial value. */
/*
void
test_memcached_increment(void)
{
  uint32_t flags;
  uint64_t intval;
  memcached_return rc;

  rc = memcached_increment_with_initial(memc, "incr", 4, 1, 30, 0, &intval);
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_true(intval == 30);

  rc = memcached_increment_with_initial(memc, "incr", 4, 3, 0, 0, &intval);
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_true(intval == 33);

  fflush(stderr);
}

void
test_memcached_decrement(void)
{
  uint32_t flags;
  uint64_t intval;
  memcached_return rc;

  rc = memcached_increment_with_initial(memc, "decr", 4, 30, 99, 0, &intval);
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_true(intval == 99);

  rc = memcached_decrement_with_initial(memc, "decr", 4, 17, 0, 0, &intval);
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_true(intval == 82);
}
*/
