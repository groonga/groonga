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
#include <stdio.h>

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
}

void
teardown(void)
{
  grn_db_close(&context, database);
  grn_ctx_fin(&context);
  cut_remove_path(tmp_directory, NULL);
  g_free(path);
}

#define NRECORDS 1000000

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
  for (i = 0; i < NRECORDS; i++) {
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
    uint64_t et;
    int nerr = 0;
    struct timeval tvb, tve;
    grn_obj *a = grn_obj_column(&context, t1, "c1.c2.c1", 8);
    grn_table_cursor *tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0);
    cut_assert_not_null(a);
    cut_assert_not_null(tc);
    gettimeofday(&tvb, NULL);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_BULK_REWIND(&buf);
      grn_obj_get_value(&context, a, id, &buf);
      if (GRN_RECORD_VALUE(&buf) != id) { nerr++; }
    }
    gettimeofday(&tve, NULL);
    et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
    // printf("et=%zu\n", et);
    cut_assert_equal_uint(0, nerr);
    cut_assert_equal_uint(0, grn_table_cursor_close(&context, tc));
    cut_assert_equal_uint(0, grn_obj_close(&context, a));
  }
  cut_assert_equal_uint(0, grn_obj_close(&context, &buf));
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
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(&context, t1, NULL, 0, NULL);
    i2 = grn_table_add(&context, t2, NULL, 0, NULL);
    GRN_BULK_REWIND(&buf);
    grn_bulk_write(&context, &buf, (char *)&i2, sizeof(grn_id));
    grn_obj_set_value(&context, c1, i1, &buf, GRN_OBJ_SET);
    grn_obj_set_value(&context, c2, i2, &buf, GRN_OBJ_SET);
  }
  {
    grn_obj *expr = grn_expr_create(&context, NULL, 0);
    grn_obj *r, *v;
    cut_assert_not_null(expr);
    v = grn_expr_add_var(&context, expr, NULL, 0);
    GRN_RECORD_INIT(v, 0, grn_obj_id(&context, t1));
    grn_expr_append_obj(&context, expr, v);

    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c2");
    grn_expr_append_const(&context, expr, &buf);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf);

    /*
    GRN_TEXT_SETS(&context, &buf, "c1.c2.c1");
    grn_expr_append_const(&context, expr, &buf);
    */

    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    grn_expr_compile(&context, expr);
    {
      grn_id id;
      uint64_t et;
      int nerr = 0;
      grn_table_cursor *tc;
      struct timeval tvb, tve;
      tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0);
      cut_assert_not_null(tc);
      gettimeofday(&tvb, NULL);
      while ((id = grn_table_cursor_next(&context, tc))) {
        GRN_RECORD_SET(&context, v, id);
        r = grn_expr_exec(&context, expr);
        if (GRN_RECORD_VALUE(r) != id) { nerr++; }
      }
      gettimeofday(&tve, NULL);
      et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
      // printf("et=%zu\n", et);
      cut_assert_equal_uint(0, nerr);
      cut_assert_equal_uint(0, grn_table_cursor_close(&context, tc));
    }
    cut_assert_equal_uint(0, grn_obj_close(&context, expr));
  }
  cut_assert_equal_uint(0, grn_obj_close(&context, &buf));
}

void
test_persistent_expr(void)
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
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(&context, t1, NULL, 0, NULL);
    i2 = grn_table_add(&context, t2, NULL, 0, NULL);
    GRN_BULK_REWIND(&buf);
    grn_bulk_write(&context, &buf, (char *)&i2, sizeof(grn_id));
    grn_obj_set_value(&context, c1, i1, &buf, GRN_OBJ_SET);
    grn_obj_set_value(&context, c2, i2, &buf, GRN_OBJ_SET);
  }
  {
    grn_obj *expr = grn_expr_create(&context, "test", 4);
    grn_obj *v;
    cut_assert_not_null(expr);
    v = grn_expr_add_var(&context, expr, "foo", 3);
    GRN_RECORD_INIT(v, 0, grn_obj_id(&context, t1));
    grn_expr_append_obj(&context, expr, v);

    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c2");
    grn_expr_append_const(&context, expr, &buf);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf);

    /*
    GRN_TEXT_SETS(&context, &buf, "c1.c2.c1");
    grn_expr_append_const(&context, expr, &buf);
    */

    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    grn_expr_compile(&context, expr);
  }
  cut_assert_equal_uint(0, grn_obj_close(&context, &buf));

  grn_db_close(&context, database);
  database = grn_db_open(&context, path);

  GRN_TEXT_INIT(&buf, 0);

  {
    grn_id id;
    uint64_t et;
    int nerr = 0;
    grn_obj *r, *v;
    grn_table_cursor *tc;
    struct timeval tvb, tve;
    grn_obj *expr = grn_ctx_get(&context, "test", 4);
    v = grn_expr_get_var(&context, expr, "foo", 3);
    t1 = grn_ctx_get(&context, "t1", 2);
    tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0);
    cut_assert_not_null(tc);
    gettimeofday(&tvb, NULL);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_RECORD_SET(&context, v, id);
      r = grn_expr_exec(&context, expr);
      if (GRN_RECORD_VALUE(r) != id) { nerr++; }
    }
    gettimeofday(&tve, NULL);
    et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
    // printf("et=%zu\n", et);
    cut_assert_equal_uint(0, nerr);
    cut_assert_equal_uint(0, grn_table_cursor_close(&context, tc));
  }
  cut_assert_equal_uint(0, grn_obj_close(&context, &buf));
}
