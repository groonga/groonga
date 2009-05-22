/* Copyright(C) 2009 Brazil

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "db.h"

#include <gcutter.h>

#include "../lib/grn-assertions.h"

static gchar *tmp_directory;
static gchar *path;
static grn_ctx context;
static grn_obj *database;

void
startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "test-expr",
                                   NULL);
}

void
shutdown(void)
{
  g_free(tmp_directory);
}

void
setup(void)
{
  cut_remove_path(tmp_directory, NULL);
  g_mkdir_with_parents(tmp_directory, 0700);
  path = g_build_filename(tmp_directory, "text-expr", NULL);
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, path, NULL);
  g_free(path);
}

void
teardown(void)
{
  grn_db_close(&context, database);
  grn_ctx_fin(&context);
  cut_remove_path(tmp_directory, NULL);
}

void
test_accessor(void)
{
  int i;
  grn_obj *t1, *t2, *c1, *c2, buf;
  t1 = grn_table_create(&context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(t1);
  t2 = grn_table_create(&context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(t2);
  c1 = grn_column_create(&context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(&context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_TEXT_INIT(&buf, 0);
  for (i = 0; i < 100; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(&context, t1, NULL, 0, NULL);
    i2 = grn_table_add(&context, t2, NULL, 0, NULL);
    GRN_BULK_REWIND(&buf);
    grn_bulk_write(&context, &buf, (char *)&i2, sizeof(grn_id));
    grn_obj_set_value(&context, c1, i1, &buf, GRN_OBJ_SET);
    grn_obj_set_value(&context, c2, i2, &buf, GRN_OBJ_SET);
  }
  {
    grn_id id;
    grn_obj *a = grn_obj_column(&context, t1, "c1.c2.c1", 8);
    grn_table_cursor *tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0);
    cut_assert_not_null(a);
    cut_assert_not_null(tc);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_BULK_REWIND(&buf);
      grn_obj_get_value(&context, a, id, &buf);
      cut_assert_equal_uint(sizeof(grn_id), GRN_BULK_VSIZE(&buf));
      cut_assert_equal_uint(id, *((grn_id *)GRN_BULK_HEAD(&buf)));
    }
    cut_assert_equal_uint(0, grn_table_cursor_close(&context, tc));
  }
}

void
test_expr(void)
{
  int i;
  grn_obj *t1, *t2, *c1, *c2, buf;
  t1 = grn_table_create(&context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(t1);
  t2 = grn_table_create(&context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(t2);
  c1 = grn_column_create(&context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(&context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_TEXT_INIT(&buf, 0);
  for (i = 0; i < 100; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(&context, t1, NULL, 0, NULL);
    i2 = grn_table_add(&context, t2, NULL, 0, NULL);
    GRN_BULK_REWIND(&buf);
    grn_bulk_write(&context, &buf, (char *)&i2, sizeof(grn_id));
    grn_obj_set_value(&context, c1, i1, &buf, GRN_OBJ_SET);
    grn_obj_set_value(&context, c2, i2, &buf, GRN_OBJ_SET);
  }
  {
    grn_id id;
    grn_obj *a = grn_obj_column(&context, t1, "c1.c2.c1", 8);
    grn_table_cursor *tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0);
    cut_assert_not_null(a);
    cut_assert_not_null(tc);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_BULK_REWIND(&buf);
      grn_obj_get_value(&context, a, id, &buf);
      cut_assert_equal_uint(sizeof(grn_id), GRN_BULK_VSIZE(&buf));
      cut_assert_equal_uint(id, *((grn_id *)GRN_BULK_HEAD(&buf)));
    }
    cut_assert_equal_uint(0, grn_table_cursor_close(&context, tc));
  }
}
