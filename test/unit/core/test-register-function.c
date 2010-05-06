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

#include <str.h>

void test_register(void);

static gchar *tmp_directory, *function_modules_dir, *function_modules_dir_env;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "register-function",
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
setup_function_modules_dir(void)
{
  function_modules_dir = g_build_filename(grn_test_get_base_dir(),
                                          "fixtures",
                                          "function-modules",
                                          ".libs",
                                          NULL);
  function_modules_dir_env = g_strdup(g_getenv("GRN_FUNCTION_MODULES_DIR"));
  g_setenv("GRN_FUNCTION_MODULES_DIR", function_modules_dir, TRUE);
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

  setup_function_modules_dir();
}

static void
teardown_function_modules_dir(void)
{
  if (function_modules_dir_env) {
    g_setenv("GRN_FUNCTION_MODULES_DIR", function_modules_dir_env, TRUE);
  } else {
    g_unsetenv("GRN_FUNCTION_MODULES_DIR");
  }
  g_free(function_modules_dir_env);
  g_free(function_modules_dir);
}

void
cut_teardown(void)
{
  teardown_function_modules_dir();

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_register(void)
{
  assert_send_command("register_function string");
  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Terms "
                      "TABLE_PAT_KEY|KEY_NORMALIZE ShortText " \
                      "--default_tokenizer TokenBigram");
  assert_send_command("column_create Terms Sites_key "
                      "COLUMN_INDEX|WITH_POSITION Sites _key");
  assert_send_command("load '[[\"_key\"],[\"groonga.org\"]]' Sites");
  cut_assert_equal_string("[[[1],[[\"_score\",\"Int32\"]],[11]]]",
                          send_command("select Sites "                  \
                                       "--output_columns _score "       \
                                       "--match_columns _key "          \
                                       "--query groonga "               \
                                       "--scorer '_score=str_len(_key)'"));
}
