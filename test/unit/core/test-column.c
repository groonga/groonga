/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define LOOKUP(name) (grn_ctx_get(&context, name, strlen(name)))

void test_fix_size_set_value_set(void);

static grn_logger_info *logger;
static grn_ctx context;
static grn_obj *database;
static grn_obj *bookmarks;
static grn_obj *count_column;
static grn_id groonga_bookmark_id;

static void
create_bookmarks_table(void)
{
  const gchar bookmarks_table_name[] = "bookmarks";

  bookmarks = grn_table_create(&context,
                               bookmarks_table_name,
                               strlen(bookmarks_table_name),
                               NULL,
                               GRN_OBJ_TABLE_HASH_KEY,
                               LOOKUP("<shorttext>"),
                               1024);
  grn_test_assert_context(&context);
  cut_set_message("%s", cut_take_string(grn_collect_logger_to_string(logger)));
  cut_assert_not_null(bookmarks);
}

static void
add_count_column_to_bookmarks_table (void)
{
  const gchar count_column_name[] = "count";

  count_column = grn_column_create(&context,
                                   bookmarks,
                                   count_column_name,
                                   strlen(count_column_name),
                                   NULL, 0,
                                   LOOKUP("<int>"));
  grn_test_assert_context(&context);
  cut_set_message("%s", cut_take_string(grn_collect_logger_to_string(logger)));
  cut_assert_not_null(count_column);
}

static void
add_groonga_bookmark(void)
{
  gchar key[] = "groonga";
  groonga_bookmark_id = grn_table_add(&context, bookmarks,
                                      &key, strlen(key), NULL);
  grn_test_assert_context(&context);
  cut_set_message("%s", cut_take_string(grn_collect_logger_to_string(logger)));
  grn_test_assert_not_nil(groonga_bookmark_id);
}

void
cut_setup(void)
{
  logger = setup_grn_logger();
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, NULL, NULL);

  create_bookmarks_table();
  add_count_column_to_bookmarks_table();
  add_groonga_bookmark();
}

void
cut_teardown(void)
{
  grn_ctx_fin(&context);
  teardown_grn_logger(logger);
}

void
test_fix_size_set_value_set(void)
{
  gint32 count = 29;
  gint32 retrieved_count;
  grn_obj *record_value;
  grn_obj *retrieved_record_value;

  record_value = grn_obj_open(&context, GRN_BULK, 0, 0);
  grn_bulk_write(&context, record_value, (const char *)&count, sizeof(count));
  grn_test_assert(grn_obj_set_value(&context, count_column, groonga_bookmark_id,
                                    record_value, GRN_OBJ_SET));

  retrieved_record_value = grn_obj_get_value(&context, count_column,
                                             groonga_bookmark_id, NULL);
  memcpy(&retrieved_count,
         GRN_BULK_HEAD(retrieved_record_value),
         GRN_BULK_VSIZE(retrieved_record_value));
  cut_assert_equal_int(count, retrieved_count);
}

void
test_fix_size_set_value_increment(void)
{
  gint32 count = 29;
  gint32 increment_count = 5;
  gint32 retrieved_count;
  grn_obj *record_value;
  grn_obj *retrieved_record_value;

  record_value = grn_obj_open(&context, GRN_BULK, 0, 0);
  grn_bulk_write(&context, record_value, (const char *)&count, sizeof(count));
  grn_test_assert(grn_obj_set_value(&context, count_column, groonga_bookmark_id,
                                    record_value, GRN_OBJ_SET));
  grn_obj_close(&context, record_value);

  record_value = grn_obj_open(&context, GRN_BULK, 0, 0);
  grn_bulk_write(&context, record_value,
                 (const char *)&increment_count, sizeof(increment_count));
  grn_test_assert(grn_obj_set_value(&context, count_column, groonga_bookmark_id,
                                    record_value, GRN_OBJ_INCR));
  grn_obj_close(&context, record_value);

  retrieved_record_value = grn_obj_get_value(&context, count_column,
                                             groonga_bookmark_id, NULL);
  memcpy(&retrieved_count,
         GRN_BULK_HEAD(retrieved_record_value),
         GRN_BULK_VSIZE(retrieved_record_value));
  cut_assert_equal_int(count + increment_count, retrieved_count);
}
