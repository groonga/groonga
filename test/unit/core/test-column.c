/*
  Copyright (C) 2009-2015  Kouhei Sutou <kou@clear-code.com>

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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_fix_size_set_value_set(void);
void test_fix_size_set_value_increment(void);
void test_create_on_temporary_table(void);

static grn_logger_info *logger;
static grn_ctx *context;
static grn_obj *database;
static grn_obj *bookmarks;
static grn_obj *count_column;
static grn_id groonga_bookmark_id;
static grn_hash *columns;

static void
create_bookmarks_table(void)
{
  const gchar bookmarks_table_name[] = "bookmarks";

  bookmarks = grn_table_create(context,
                               bookmarks_table_name,
                               strlen(bookmarks_table_name),
                               NULL,
                               GRN_OBJ_TABLE_HASH_KEY,
                               get_object("ShortText"),
                               NULL);
  grn_test_assert_context(context);
  cut_assert_not_null(
    bookmarks,
    cut_message("%s", grn_collect_logger_to_string(logger)));
}

static void
add_count_column_to_bookmarks_table (void)
{
  const gchar count_column_name[] = "count";

  count_column = grn_column_create(context,
                                   bookmarks,
                                   count_column_name,
                                   strlen(count_column_name),
                                   NULL, 0,
                                   get_object("Int32"));
  grn_test_assert_context(context);
  cut_assert_not_null(
    count_column,
    cut_message("%s", grn_collect_logger_to_string(logger)));
}

static void
add_groonga_bookmark(void)
{
  gchar key[] = "groonga";
  groonga_bookmark_id = grn_table_add(context, bookmarks,
                                      &key, strlen(key), NULL);
  grn_test_assert_context(context);
  grn_test_assert_not_nil(
    groonga_bookmark_id,
    cut_message("%s", grn_collect_logger_to_string(logger)));
}

void
cut_setup(void)
{
  logger = setup_grn_logger();
  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);
  database = grn_db_create(context, NULL, NULL);
  columns = grn_hash_create(context, NULL, sizeof(grn_id), 0,
                            GRN_OBJ_TABLE_HASH_KEY);

  create_bookmarks_table();
  add_count_column_to_bookmarks_table();
  add_groonga_bookmark();
}

void
cut_teardown(void)
{
  grn_hash_close(context, columns);
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);
  teardown_grn_logger(logger);
}

void
test_fix_size_set_value_set(void)
{
  gint32 count = 29;
  gint32 retrieved_count;
  grn_obj record_value;
  grn_obj retrieved_record_value;

  GRN_INT32_INIT(&record_value, 0);
  GRN_INT32_SET(context, &record_value, count);
  grn_test_assert(grn_obj_set_value(context, count_column, groonga_bookmark_id,
                                    &record_value, GRN_OBJ_SET));

  GRN_INT32_INIT(&retrieved_record_value, 0);
  grn_obj_get_value(context, count_column, groonga_bookmark_id, &retrieved_record_value);
  retrieved_count = GRN_INT32_VALUE(&retrieved_record_value);
  cut_assert_equal_int(count, retrieved_count);
  GRN_OBJ_FIN(context, &record_value);
  GRN_OBJ_FIN(context, &retrieved_record_value);
}

void
test_fix_size_set_value_increment(void)
{
  gint32 count = 29;
  gint32 increment_count = 5;
  gint32 retrieved_count;
  grn_obj *record_value;
  grn_obj *retrieved_record_value;

  record_value = grn_obj_open(context, GRN_BULK, 0, GRN_DB_INT32);
  grn_bulk_write(context, record_value, (const char *)&count, sizeof(count));
  grn_test_assert(grn_obj_set_value(context, count_column, groonga_bookmark_id,
                                    record_value, GRN_OBJ_SET));
  grn_obj_close(context, record_value);

  record_value = grn_obj_open(context, GRN_BULK, 0, GRN_DB_INT32);
  grn_bulk_write(context, record_value,
                 (const char *)&increment_count, sizeof(increment_count));
  grn_test_assert(grn_obj_set_value(context, count_column, groonga_bookmark_id,
                                    record_value, GRN_OBJ_INCR));
  grn_obj_close(context, record_value);

  retrieved_record_value = grn_obj_get_value(context, count_column,
                                             groonga_bookmark_id, NULL);
  memcpy(&retrieved_count,
         GRN_BULK_HEAD(retrieved_record_value),
         GRN_BULK_VSIZE(retrieved_record_value));
  cut_assert_equal_int(count + increment_count, retrieved_count);
  grn_obj_close(context, retrieved_record_value);
}

void
test_create_on_temporary_table(void)
{
  grn_obj *table;
  grn_obj *column;
  const gchar *column_name = "count";

  table = grn_table_create(context, NULL, 0, NULL,
                           GRN_OBJ_TABLE_NO_KEY,
                           NULL,
                           NULL);
  grn_test_assert_context(context);
  column = grn_column_create(context,
                             table,
                             column_name,
                             strlen(column_name),
                             NULL, 0,
                             get_object("Int32"));
  grn_test_assert_context(context);
  cut_assert_equal_int(1,
                       grn_table_columns(context, table, NULL, 0,
                                         (grn_obj *)columns));
  {
    grn_id found_column_id = GRN_ID_NIL;
    grn_hash_get_key(context, columns, 1, &found_column_id, sizeof(grn_id));
    cut_assert_equal_uint(grn_obj_id(context, column),
                          found_column_id);
  }
}
