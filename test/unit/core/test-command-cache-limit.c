/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>

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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <ctx.h>

void test_get(void);
void test_set(void);
void test_set_minus(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

static uint32_t default_cache_n_entries;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "cache-limit",
                                   NULL);
  default_cache_n_entries = *grn_cache_max_nentries();
}

void
cut_shutdown(void)
{
  g_free(tmp_directory);
}

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

void
cut_setup(void)
{
  const gchar *database_path;
  uint32_t *cache_max_n_entries;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);

  cache_max_n_entries = grn_cache_max_nentries();
  *cache_max_n_entries = default_cache_n_entries;
}

void
cut_teardown(void)
{
  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_get(void)
{
  cut_assert_equal_string("100", send_command("cache_limit"));
}

void
test_set(void)
{
  cut_assert_equal_string("100", send_command("cache_limit --max 1000"));
  cut_assert_equal_string("1000", send_command("cache_limit"));
}

void
test_set_invalid_minus(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "max value is invalid unsigned integer format: <-1>",
    "cache_limit --max -1");
  cut_assert_equal_string("100", send_command("cache_limit"));
}

void
test_set_invalid_string(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "max value is invalid unsigned integer format: <LIMIT>",
    "cache_limit --max LIMIT");
  cut_assert_equal_string("100", send_command("cache_limit"));
}
