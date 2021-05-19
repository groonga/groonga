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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grn_db.h"
#include <stdio.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

static gchar *tmp_directory;
static gchar *path;
static grn_ctx *context;
static grn_obj *database;
static grn_obj *expr;
static grn_obj text_buf, int_buf, ptr_buf;

void test_accessor(void);
void test_expr(void);
void test_persistent_expr(void);
void test_expr_query(void);

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "test-expr",
                                   NULL);
}

void
cut_shutdown(void)
{
  g_free(tmp_directory);
}

void
cut_setup(void)
{
  cut_remove_path(tmp_directory, NULL);
  g_mkdir_with_parents(tmp_directory, 0700);
  path = g_build_filename(tmp_directory, "text-expr", NULL);
  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);
  database = grn_db_create(context, path, NULL);

  expr = NULL;

  GRN_TEXT_INIT(&text_buf, 0);
  GRN_UINT32_INIT(&int_buf, 0);
  GRN_PTR_INIT(&ptr_buf, 0, GRN_ID_NIL);
}

void
cut_teardown(void)
{
  grn_obj_close(context, &text_buf);
  grn_obj_close(context, &int_buf);
  grn_obj_close(context, &ptr_buf);

  if (expr)
    grn_obj_close(context, expr);

  grn_db_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  cut_remove_path(tmp_directory, NULL);
  g_free(path);
}

#define NRECORDS 1000000

void
test_accessor(void)
{
  int i;
  grn_obj *t1, *t2, *c1, *c2, r1, r2;
  t1 = grn_table_create(context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);
  t2 = grn_table_create(context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t2);
  c1 = grn_column_create(context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_RECORD_INIT(&r1, 0, grn_obj_id(context, t1));
  GRN_RECORD_INIT(&r2, 0, grn_obj_id(context, t2));
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(context, t1, NULL, 0, NULL);
    i2 = grn_table_add(context, t2, NULL, 0, NULL);
    GRN_RECORD_SET(context, &r1, i1);
    GRN_RECORD_SET(context, &r2, i2);
    grn_obj_set_value(context, c1, i1, &r2, GRN_OBJ_SET);
    grn_obj_set_value(context, c2, i2, &r1, GRN_OBJ_SET);
  }
  {
    grn_id id;
    uint64_t et;
    int nerr = 0;
    struct timeval tvb, tve;
    grn_obj *a = grn_obj_column(context, t1, "c1.c2.c1", 8);
    grn_table_cursor *tc = grn_table_cursor_open(context, t1, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(a);
    cut_assert_not_null(tc);
    gettimeofday(&tvb, NULL);
    while ((id = grn_table_cursor_next(context, tc))) {
      GRN_BULK_REWIND(&r2);
      grn_obj_get_value(context, a, id, &r2);
      if (GRN_RECORD_VALUE(&r2) != id) { nerr++; }
    }
    gettimeofday(&tve, NULL);
    et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
    // printf("et=%zu\n", et);
    cut_assert_equal_uint(0, nerr);
    grn_test_assert(grn_table_cursor_close(context, tc));
    grn_test_assert(grn_obj_close(context, a));
  }
  grn_test_assert(grn_obj_close(context, &r1));
  grn_test_assert(grn_obj_close(context, &r2));
}

void
test_expr(void)
{
  int i;
  grn_obj *t1, *t2, *c1, *c2, r1, r2, buf;
  t1 = grn_table_create(context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);
  t2 = grn_table_create(context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t2);
  c1 = grn_column_create(context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_TEXT_INIT(&buf, 0);
  GRN_RECORD_INIT(&r1, 0, grn_obj_id(context, t1));
  GRN_RECORD_INIT(&r2, 0, grn_obj_id(context, t2));
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(context, t1, NULL, 0, NULL);
    i2 = grn_table_add(context, t2, NULL, 0, NULL);
    GRN_RECORD_SET(context, &r1, i1);
    GRN_RECORD_SET(context, &r2, i2);
    grn_obj_set_value(context, c1, i1, &r2, GRN_OBJ_SET);
    grn_obj_set_value(context, c2, i2, &r1, GRN_OBJ_SET);
  }
  {
    grn_obj *r, *v;

    expr = grn_expr_create(context, NULL, 0);
    cut_assert_not_null(expr);
    v = grn_expr_add_var(context, expr, NULL, 0);
    GRN_RECORD_INIT(v, 0, grn_obj_id(context, t1));
    grn_expr_append_obj(context, expr, v, GRN_OP_PUSH, 1);

    GRN_TEXT_SETS(context, &buf, "c1");
    grn_expr_append_const(context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(context, &buf, "c2");
    grn_expr_append_const(context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(context, &buf, "c1");
    grn_expr_append_const(context, expr, &buf, GRN_OP_PUSH, 1);


//    GRN_TEXT_SETS(context, &buf, "c1.c2.c1");
//    grn_expr_append_const(context, expr, &buf);

    grn_expr_append_op(context, expr, GRN_OP_GET_VALUE, 2);
    grn_expr_compile(context, expr);
    {
      grn_id id;
      uint64_t et;
      int nerr = 0;
      grn_table_cursor *tc;
      struct timeval tvb, tve;
      tc = grn_table_cursor_open(context, t1, NULL, 0, NULL, 0, 0, -1, 0);
      cut_assert_not_null(tc);
      gettimeofday(&tvb, NULL);
      while ((id = grn_table_cursor_next(context, tc))) {
        GRN_RECORD_SET(context, v, id);
        r = grn_expr_exec(context, expr, 0);
        if (GRN_RECORD_VALUE(r) != id) { nerr++; }
      }
      gettimeofday(&tve, NULL);
      et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
      // printf("et=%zu\n", et);
      cut_assert_equal_uint(0, nerr);
      grn_test_assert(grn_table_cursor_close(context, tc));
    }
  }
  grn_test_assert(grn_obj_close(context, &r1));
  grn_test_assert(grn_obj_close(context, &r2));
  grn_test_assert(grn_obj_close(context, &buf));
}

#ifdef ENABLE_PERSISTENT_EXPR
void
test_persistent_expr(void)
{
  int i;
  grn_obj *t1, *t2, *c1, *c2, r1, r2, buf;
  t1 = grn_table_create(context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);
  t2 = grn_table_create(context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t2);
  c1 = grn_column_create(context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_TEXT_INIT(&buf, 0);
  GRN_RECORD_INIT(&r1, 0, grn_obj_id(context, t1));
  GRN_RECORD_INIT(&r2, 0, grn_obj_id(context, t2));
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(context, t1, NULL, 0, NULL);
    i2 = grn_table_add(context, t2, NULL, 0, NULL);
    GRN_RECORD_SET(context, &r1, i1);
    GRN_RECORD_SET(context, &r2, i2);
    grn_obj_set_value(context, c1, i1, &r2, GRN_OBJ_SET);
    grn_obj_set_value(context, c2, i2, &r1, GRN_OBJ_SET);
  }
  {
    grn_obj *v;
    expr = grn_expr_create(context, "test", 4);
    cut_assert_not_null(expr);
    v = grn_expr_add_var(context, expr, "foo", 3);
    GRN_RECORD_INIT(v, 0, grn_obj_id(context, t1));
    grn_expr_append_obj(context, expr, v, GRN_OP_PUSH, 1);

    GRN_TEXT_SETS(context, &buf, "c1");
    grn_expr_append_const(context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(context, &buf, "c2");
    grn_expr_append_const(context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(context, &buf, "c1");
    grn_expr_append_const(context, expr, &buf, GRN_OP_PUSH, 1);

    /*
    GRN_TEXT_SETS(context, &buf, "c1.c2.c1");
    grn_expr_append_const(context, expr, &buf);
    */

    grn_expr_append_op(context, expr, GRN_OP_GET_VALUE, 2);
    grn_expr_compile(context, expr);
    grn_test_assert(grn_obj_close(context, expr));
    expr = NULL;
  }
  grn_test_assert(grn_obj_close(context, &buf));

  grn_db_close(context, database);
  database = grn_db_open(context, path);

  {
    grn_id id;
    uint64_t et;
    int nerr = 0;
    grn_obj *r, *v;
    grn_table_cursor *tc;
    struct timeval tvb, tve;
    expr = get_object("test");
    v = grn_expr_get_var(context, expr, "foo", 3);
    t1 = get_object("t1");
    tc = grn_table_cursor_open(context, t1, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(tc);
    gettimeofday(&tvb, NULL);
    while ((id = grn_table_cursor_next(context, tc))) {
      GRN_RECORD_SET(context, v, id);
      r = grn_expr_exec(context, expr, 0);
      if (GRN_RECORD_VALUE(r) != id) { nerr++; }
    }
    gettimeofday(&tve, NULL);
    et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
    // printf("et=%zu\n", et);
    cut_assert_equal_uint(0, nerr);
    grn_test_assert(grn_table_cursor_close(context, tc));
  }
  grn_test_assert(grn_obj_close(context, &r1));
  grn_test_assert(grn_obj_close(context, &r2));
}
#endif /* ENABLE_PERSISTENT_EXPR */

void
test_expr_query(void)
{
  grn_obj *t1, *c1, *lc, *ft, *v;
  grn_id r1, r2, r3, r4;

  /* actual table */
  t1 = grn_table_create(context, "t1", 2, NULL,
			GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);

  /* lexicon table */
  lc = grn_table_create(context, "lc", 2, NULL,
			GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                        grn_ctx_at(context, GRN_DB_SHORT_TEXT), NULL);
  cut_assert_not_null(lc);
  grn_test_assert(grn_obj_set_info(context, lc, GRN_INFO_DEFAULT_TOKENIZER,
				   grn_ctx_at(context, GRN_DB_BIGRAM)));

  /* actual column */
  c1 = grn_column_create(context, t1, "c1", 2, NULL,
			 GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
			 grn_ctx_at(context, GRN_DB_TEXT));
  cut_assert_not_null(c1);

  /* fulltext index */
  ft = grn_column_create(context, lc, "ft", 2, NULL,
			 GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT|GRN_OBJ_WITH_POSITION, t1);
  cut_assert_not_null(ft);

  /* link between actual column and fulltext index */
  GRN_UINT32_SET(context, &int_buf, grn_obj_id(context, c1));
  grn_obj_set_info(context, ft, GRN_INFO_SOURCE, &int_buf); /* need to use grn_id */

  /* insert row */
  r1 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(1, r1);
  GRN_TEXT_SETS(context, &text_buf, "abhij");
  grn_test_assert(grn_obj_set_value(context, c1, r1, &text_buf, GRN_OBJ_SET));

  r2 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(2, r2);
  GRN_TEXT_SETS(context, &text_buf, "fghij");
  grn_test_assert(grn_obj_set_value(context, c1, r2, &text_buf, GRN_OBJ_SET));

  r3 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(3, r3);
  GRN_TEXT_SETS(context, &text_buf, "11 22 33");
  grn_test_assert(grn_obj_set_value(context, c1, r3, &text_buf, GRN_OBJ_SET));

  r4 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(4, r4);
  GRN_TEXT_SETS(context, &text_buf, "44 22 55");
  grn_test_assert(grn_obj_set_value(context, c1, r4, &text_buf, GRN_OBJ_SET));

  /* confirm record are inserted in both column and index */
  cut_assert_equal_int(4, grn_table_size(context, t1));
  cut_assert_equal_int(20, grn_table_size(context, lc));

  cut_assert_not_null((expr = grn_expr_create(context, NULL, 0)));

  v = grn_expr_add_var(context, expr, NULL, 0);

  grn_expr_append_obj(context, expr, v, GRN_OP_PUSH, 1);

  GRN_BULK_REWIND(&text_buf);
  grn_expr_append_const(context, expr, &text_buf, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(context, &int_buf, GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC);
  grn_expr_append_const(context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(context, expr, t1, GRN_OP_PUSH, 1);
  GRN_PTR_SET(context, &ptr_buf, NULL);
  grn_expr_append_obj(context, expr, &ptr_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(context, expr, GRN_OP_TABLE_CREATE, 4);

  grn_expr_append_op(context, expr, GRN_OP_ASSIGN, 2);

  grn_expr_append_obj(context, expr, ft, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(context, &text_buf, "hij");
  grn_expr_append_const(context, expr, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(context, &int_buf, GRN_OP_OR);
  grn_expr_append_const(context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(context, expr, GRN_OP_OBJ_SEARCH, 4);

  grn_expr_append_obj(context, expr, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(context, &text_buf, ".c1 ._score");
  grn_expr_append_const(context, expr, &text_buf, GRN_OP_PUSH, 1);
  GRN_BULK_REWIND(&text_buf);
  grn_expr_append_obj(context, expr, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(context, expr, GRN_OP_JSON_PUT, 3);

  grn_expr_compile(context, expr);

  grn_expr_exec(context, expr, 0);

  cut_assert_equal_substring("[[2],[\"abhij\",1],[\"fghij\",1]]",
                             GRN_TEXT_VALUE(&text_buf), GRN_TEXT_LEN(&text_buf));

  grn_obj_close(context, ft);
  grn_obj_close(context, c1);
  grn_obj_close(context, lc);
  grn_obj_close(context, t1);
}
