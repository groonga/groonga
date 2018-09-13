/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>

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
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <grn_str.h>
#include <grn_plugin.h>

void test_register_function(void);
void test_register_with_too_long_name(void);
void test_register_by_absolute_path(void);

static gchar *tmp_directory, *plugins_dir, *plugins_dir_env;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "plugin",
                                   NULL);
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

static void
setup_plugins_dir(void)
{
  plugins_dir = g_build_filename(grn_test_get_build_dir(),
                                 "fixtures",
                                 "plugins",
                                 ".libs",
                                 NULL);
  plugins_dir_env = g_strdup(g_getenv("GRN_PLUGINS_DIR"));
  g_setenv("GRN_PLUGINS_DIR", plugins_dir, TRUE);
  grn_plugin_init_from_env();
}

void
cut_setup(void)
{
  const gchar *database_path;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);

  setup_plugins_dir();
}

static void
teardown_plugins_dir(void)
{
  if (plugins_dir_env) {
    g_setenv("GRN_PLUGINS_DIR", plugins_dir_env, TRUE);
  } else {
    g_unsetenv("GRN_PLUGINS_DIR");
  }
  grn_plugin_init_from_env();
  g_free(plugins_dir_env);
  g_free(plugins_dir);
}

void
cut_teardown(void)
{
  teardown_plugins_dir();

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

void
test_register_function(void)
{
  assert_send_command("register string");
  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command("load '[[\"_key\"],[\"groonga.org\"]]' Sites");
  cut_assert_equal_string("[[[1],[[\"_score\",\"Int32\"]],[11]]]",
                          send_command("select Sites "
                                       "--output_columns _score "
                                       "--filter true "
                                       "--scorer '_score=str_len(_key)'"));
}

void
test_register_by_absolute_path(void)
{
  assert_send_command(cut_take_printf("register %s/string", plugins_dir));
  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command("load '[[\"_key\"],[\"groonga.org\"]]' Sites");
  cut_assert_equal_string("[[[1],[[\"_score\",\"Int32\"]],[11]]]",
                          send_command("select Sites "
                                       "--output_columns _score "
                                       "--filter true "
                                       "--scorer '_score=str_len(_key)'"));
}
