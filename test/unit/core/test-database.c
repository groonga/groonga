/*
  Copyright (C) 2009-2012  Kouhei Sutou <kou@clear-code.com>

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

void test_domain(void);
void test_range(void);
void test_cursor(void);
void test_get_persistent_object_from_opened_database(void);
void test_recreate_temporary_object_on_opened_database(void);
void test_size(void);
void test_expire_cache_on_recreate(void);
void test_expression_lifetime_over_database(void);
void test_get(void);
void test_at(void);
void test_open_locked_database_and_unlock(void);

static gchar *tmp_directory;

static grn_ctx *context, *context2;
static grn_obj *database, *database2;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "test-database",
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
  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);
  database = NULL;

  context2 = g_new0(grn_ctx, 1);
  grn_ctx_init(context2, 0);
  database2 = NULL;
}

void
cut_teardown(void)
{
  if (context) {
    if (database) {
      grn_obj_close(context, database);
    }
    grn_ctx_fin(context);
    g_free(context);
  }

  if (context2) {
    if (database2) {
      grn_obj_close(context2, database2);
    }
    grn_ctx_fin(context2);
    g_free(context2);
  }

  remove_tmp_directory();
}

void
test_domain(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_nil(database->header.domain);
}

void
test_range(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_nil(grn_obj_get_range(context, database));
}

void
test_cursor(void)
{
  grn_table_cursor *c;
  database = grn_db_create(context, NULL, NULL);
  c = grn_table_cursor_open(context, database, NULL, 0, NULL, 0, 0, -1, 0);
  cut_assert_true(grn_table_cursor_next(context, c));
  grn_table_cursor_close(context, c);
}

void
test_get_persistent_object_from_opened_database(void)
{
  const gchar table_name[] = "Users";
  const gchar *path;

  path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, path, NULL);
  grn_test_assert_not_null(context, database);
  grn_test_assert_not_null(context,
                           grn_table_create(context,
                                            table_name,
                                            strlen(table_name),
                                            NULL,
                                            GRN_OBJ_TABLE_HASH_KEY |
                                            GRN_OBJ_PERSISTENT,
                                            grn_ctx_at(context, GRN_DB_UINT32),
                                            NULL));

  database2 = grn_db_open(context2, path);
  grn_test_assert_not_null(context2, database2);
  grn_test_assert_not_null(context2,
                           grn_ctx_get(context2,
                                       table_name,
                                       strlen(table_name)));
}

void
test_recreate_temporary_object_on_opened_database(void)
{
  const gchar table_name[] = "<users>";
  const gchar *path;

  path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, path, NULL);
  grn_test_assert_not_null(context, database);
  grn_test_assert_null(context,
                           grn_table_create(context,
                                            table_name,
                                            strlen(table_name),
                                            NULL,
                                            GRN_OBJ_TABLE_HASH_KEY,
                                            grn_ctx_at(context, GRN_DB_UINT32),
                                            NULL));

  database2 = grn_db_open(context2, path);
  grn_test_assert_not_null(context2, database2);
  grn_test_assert_null(context2,
                       grn_ctx_get(context2,
                                   table_name,
                                   strlen(table_name)));
  grn_test_assert_null(context2,
                           grn_table_create(context,
                                            table_name,
                                            strlen(table_name),
                                            NULL,
                                            GRN_OBJ_TABLE_HASH_KEY,
                                            grn_ctx_at(context, GRN_DB_UINT32),
                                            NULL));
}

void
test_size(void)
{
  guint n_builtin_objects = 255;
  const gchar table_name[] = "bookmarks";

  database = grn_db_create(context, NULL, NULL);

  cut_assert_equal_uint(n_builtin_objects, grn_table_size(context, database));
  grn_test_assert_context(context);

  grn_table_create(context,
                   table_name,
                   strlen(table_name),
                   NULL,
                   GRN_OBJ_TABLE_HASH_KEY,
                   grn_ctx_at(context, GRN_DB_UINT32),
                   NULL);
  cut_assert_equal_uint(n_builtin_objects + 1,
                        grn_table_size(context, database));
  grn_test_assert_context(context);
}

void
test_expire_cache_on_recreate(void)
{
  const gchar *path;

  path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, path, NULL);
  assert_send_command("table_create Sites 0 ShortText");
  assert_send_command("load '[[\"_key\"],[\"groonga.org\"]]' Sites");
  cut_assert_equal_string("[[[1],[[\"_key\",\"ShortText\"]],[\"groonga.org\"]]]",
                          send_command("select Sites --output_columns _key"));
  assert_send_command("table_remove Sites");
  grn_obj_close(context, database);

  database = grn_db_open(context, path);
  assert_send_command("table_create Sites 0 ShortText");
  cut_assert_equal_string("[[[0],[[\"_key\",\"ShortText\"]]]]",
                          send_command("select Sites --output_columns _key"));
}

void
test_expression_lifetime_over_database(void)
{
  const gchar *path;
  gint i, n_tries = 100;
  grn_obj *expression;

  cut_omit("will be SEGVed.");
  path = cut_build_path(tmp_directory, "database.groonga", NULL);
  for (i = 0; i < n_tries; i++) {
    gint j, n_records = 100;
    const gchar *query;
    grn_obj *table, *variable;
    grn_obj default_column;

    database = grn_db_create(context, path, NULL);
    grn_test_assert_context(context);

    assert_send_command("table_create Sites 0 ShortText");
    assert_send_command("column_create Sites point COLUMN_SCALAR Int32");
    for (j = 0; j < n_records; j++) {
      gchar *command;

      command = g_strdup_printf("load '"
                                "[[\"_key\", \"point\"],"
                                "[\"http://groonga.org/version/%d\",%d]]' "
                                "Sites",
                                j, j);
      assert_send_command(command);
      g_free(command);
    }

    table = get_object("Sites");
    GRN_EXPR_CREATE_FOR_QUERY(context, table, expression, variable);
    grn_obj_unlink(context, table);

    GRN_TEXT_INIT(&default_column, 0);
    GRN_TEXT_PUTS(context, &default_column, "point");
    query = "point:50";
    grn_expr_parse(context, expression,
                   query, strlen(query),
                   &default_column,
                   GRN_OP_MATCH, GRN_OP_AND,
                   GRN_EXPR_SYNTAX_QUERY | GRN_EXPR_ALLOW_COLUMN);
    grn_test_assert_context(context);
    grn_obj_unlink(context, &default_column);
    grn_expr_compile(context, expression);

    grn_obj_close(context, database);
    database = NULL;

    remove_tmp_directory();
    g_mkdir_with_parents(tmp_directory, 0700);
  }

  grn_ctx_fin(context);
  g_free(context);
  context = NULL;
}

void
test_get(void)
{
  const gchar *short_text_type_name = "ShortText";

  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_equal_id(context,
                           GRN_DB_SHORT_TEXT,
                           grn_table_get(context, database,
                                         short_text_type_name,
                                         strlen(short_text_type_name)));
}

void
test_at(void)
{
  database = grn_db_create(context, NULL, NULL);
  grn_test_assert_equal_id(context,
                           GRN_DB_SHORT_TEXT,
                           grn_table_at(context, database, GRN_DB_SHORT_TEXT));
}

void
test_open_locked_database_and_unlock(void)
{
  const gchar *path;

  path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, path, NULL);
  grn_obj_lock(context, database, GRN_ID_NIL, 0);
  cut_assert_true(grn_obj_is_locked(context, database));

  database2 = grn_db_open(context2, path);
  grn_obj_unlock(context2, database2, GRN_ID_NIL);
  cut_assert_false(grn_obj_is_locked(context, database));
}
