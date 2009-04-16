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
#include <gcutter.h> /* must be included adter memcached.h */

#include <unistd.h> /* for exec */
#include <sys/types.h>
#include <signal.h>

#include "../lib/grn-assertions.h"

#define GROONGA_TEST_PORT "4545"
#define GROONGA_TEST_DB "/tmp/groonga-memcached.db"

/* globals */
static pid_t groonga_pid;
static struct memcached_st *memc;
static struct memcached_server_st *servers;
static char *val;

static GCutEgg *egg;

void test_set_and_get(void);

void
setup(void)
{
  GError *error = NULL;
  memcached_return rc;

  memc = NULL;
  servers = NULL;

  egg = gcut_egg_new(GROONGA, "-s",
                     "-p", GROONGA_TEST_PORT,
                     GROONGA_TEST_DB,
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
}

void
teardown(void)
{
  if (egg)
    g_object_unref(egg);

  if (servers)
    memcached_server_list_free(servers);
  if (memc)
    memcached_free(memc);
}

void
test_set_and_get(void)
{
  size_t val_len;
  uint32_t flags;
  memcached_return rc;

  rc = memcached_set(memc, "key", 3, "value", 5, 0, 0xdeadbeefU);
  cut_set_message("memcached set failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  val = memcached_get(memc, "key", 3, &val_len, &flags, &rc);
  cut_set_message("memcached get failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_equal_string("value", val);
  cut_assert_equal_uint(0xdeadbeefU, flags);
}

void
test_set_and_get_with_expire(void)
{
  size_t val_len;
  uint32_t flags;
  memcached_return rc;

  rc = memcached_set(memc, "key", 1, "value", 5, 3, 0xdeadbeefU);
  cut_set_message("memcached set with expiration failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);

  val = memcached_get(memc, "key", 1, &val_len, &flags, &rc);
  cut_set_message("memcached get with expiration failed.");
  cut_assert_equal_int(MEMCACHED_SUCCESS, rc);
  cut_assert_equal_string("value", val);
  cut_assert_equal_uint(0xdeadbeefU, flags);

  if (val) {
    free(val);
    val = NULL;
  }

  sleep(2);

  val = memcached_get(memc, "key", 1, &val_len, &flags, &rc);
  cut_set_message("memcached get with expiration error.");
  cut_assert_equal_int(MEMCACHED_NOTFOUND, rc);
}
