/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright(C) 2010-2011 Kouhei Sutou <kou@clear-code.com>

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

#include "str.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_no_operator_and_parentheses_column(void);
void test_match_against_not_string(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "command-select-filter-invalid",
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
setup_data(void)
{
  assert_send_commands("table_create Sites TABLE_HASH_KEY ShortText\n"
                       "column_create Sites uri COLUMN_SCALAR ShortText\n"
                       "load --table Sites\n"
                       "[\n"
                       "[\"_key\",\"uri\"],\n"
                       "[\"groonga\",\"http://groonga.org/\"],\n"
                       "[\"razil\",\"http://razil.jp/\"]\n"
                       "]");
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

  setup_data();
}

void
cut_teardown(void)
{
  if (context) {
    grn_obj_unlink(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_no_operator_and_parentheses_column(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "invalid function: <\"groonga\">",
    "select Sites --filter \"_key != \\\"groonga\\\" ()\"");
}

void
test_match_against_not_string(void)
{
  grn_test_assert_send_command_error(
    context,
    GRN_INVALID_ARGUMENT,
    "invalid expression: can't use column as a value: "
    "<Sites.uri>: "
    "<noname($1:null){2_key GET_VALUE,0uri GET_VALUE,0MATCH}>",
    "select Sites --filter \"_key @ uri");
}
