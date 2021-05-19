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

void data_alphabet(void);
void test_alphabet(gconstpointer data);
void data_japanese(void);
void test_japanese(gconstpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "function-edit-distance",
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
}

void
cut_teardown(void)
{
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

void
data_alphabet(void)
{
#define ADD_DATUM(label, expected, string)              \
    gcut_add_datum(label,                               \
                   "expected", G_TYPE_STRING, expected, \
                   "string", G_TYPE_STRING, string,     \
                   NULL)

    ADD_DATUM("same",
              "[\"mori\",0],"
              "[\"topo\",3],"
              "[\"tako\",4],"
              "[\"tapa\",4],"
              "[\"tapo\",4],"
              "[\"yu\",4],"
              "[\"tapopo\",5],"
              "[\"gunyara-kun\",10]",
              "mori");

    ADD_DATUM("long",
              "[\"mori\",4],"
              "[\"tapo\",4],"
              "[\"tako\",5],"
              "[\"tapa\",5],"
              "[\"topo\",5],"
              "[\"tapopo\",6],"
              "[\"yu\",8],"
              "[\"gunyara-kun\",10]",
              "moritapo");

    ADD_DATUM("short",
              "[\"mori\",2],"
              "[\"yu\",2],"
              "[\"tako\",3],"
              "[\"tapo\",3],"
              "[\"topo\",3],"
              "[\"tapa\",4],"
              "[\"tapopo\",5],"
              "[\"gunyara-kun\",11]",
              "mo");

#undef ADD_DATUM
}

void
test_alphabet(gconstpointer data)
{
  const gchar *expected, *command;

  assert_send_command("table_create Users TABLE_NO_KEY");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("load --table Users\n"
                      "[\n"
                      "[\"name\"],\n"
                      "[\"mori\"],\n"
                      "[\"gunyara-kun\"],\n"
                      "[\"yu\"],\n"
                      "[\"tapo\"],\n"
                      "[\"tapopo\"],\n"
                      "[\"tapa\"],\n"
                      "[\"tako\"],\n"
                      "[\"topo\"]\n"
                      "]");

  expected =
      cut_take_printf("[[[8],"
                      "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
                      "%s"
                      "]]",
                      gcut_data_get_string(data, "expected"));
  command = cut_take_printf("select Users "
                            "--output_columns name,_score "
                            "--filter true "
                            "--sort_keys _score,name "
                            "--scorer '_score=edit_distance(name, \"%s\")'",
                            gcut_data_get_string(data, "string"));

  cut_assert_equal_string(expected, send_command(command));
}

void
data_japanese(void)
{
#define ADD_DATUM(label, expected, string)              \
    gcut_add_datum(label,                               \
                   "expected", G_TYPE_STRING, expected, \
                   "string", G_TYPE_STRING, string,     \
                   NULL)

    ADD_DATUM("same",
              "[\"もり\",0],"
              "[\"たこ\",2],"
              "[\"たぱ\",2],"
              "[\"たぽ\",2],"
              "[\"とぽ\",2],"
              "[\"ゆう\",2],"
              "[\"たぽぽ\",3],"
              "[\"グニャラくん\",6]",
              "もり");

    ADD_DATUM("long",
              "[\"たぽ\",2],"
              "[\"もり\",2],"
              "[\"たこ\",3],"
              "[\"たぱ\",3],"
              "[\"たぽぽ\",3],"
              "[\"とぽ\",3],"
              "[\"ゆう\",4],"
              "[\"グニャラくん\",6]",
              "もりたぽ");

    ADD_DATUM("short",
              "[\"もり\",1],"
              "[\"たこ\",2],"
              "[\"たぱ\",2],"
              "[\"たぽ\",2],"
              "[\"とぽ\",2],"
              "[\"ゆう\",2],"
              "[\"たぽぽ\",3],"
              "[\"グニャラくん\",6]",
              "も");

#undef ADD_DATUM
}

void
test_japanese(gconstpointer data)
{
  const gchar *expected, *command;

  assert_send_command("table_create Users TABLE_NO_KEY");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("load --table Users\n"
                      "[\n"
                      "[\"name\"],\n"
                      "[\"もり\"],\n"
                      "[\"グニャラくん\"],\n"
                      "[\"ゆう\"],\n"
                      "[\"たぽ\"],\n"
                      "[\"たぽぽ\"],\n"
                      "[\"たぱ\"],\n"
                      "[\"たこ\"],\n"
                      "[\"とぽ\"]\n"
                      "]");

  expected =
      cut_take_printf("[[[8],"
                      "[[\"name\",\"ShortText\"],[\"_score\",\"Int32\"]],"
                      "%s"
                      "]]",
                      gcut_data_get_string(data, "expected"));
  command = cut_take_printf("select Users "
                            "--output_columns name,_score "
                            "--filter true "
                            "--sort_keys _score,name "
                            "--scorer '_score=edit_distance(name, \"%s\")'",
                            gcut_data_get_string(data, "string"));

  cut_assert_equal_string(expected, send_command(command));
}
