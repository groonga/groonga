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
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
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
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, path, NULL);
}

void
cut_teardown(void)
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
    grn_expr_append_op(&context, expr, GRN_OP_OBJ_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c2");
    grn_expr_append_const(&context, expr, &buf);
    grn_expr_append_op(&context, expr, GRN_OP_OBJ_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf);

    /*
    GRN_TEXT_SETS(&context, &buf, "c1.c2.c1");
    grn_expr_append_const(&context, expr, &buf);
    */

    grn_expr_append_op(&context, expr, GRN_OP_OBJ_GET_VALUE, 2);
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
    grn_expr_append_op(&context, expr, GRN_OP_OBJ_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c2");
    grn_expr_append_const(&context, expr, &buf);
    grn_expr_append_op(&context, expr, GRN_OP_OBJ_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf);

    /*
    GRN_TEXT_SETS(&context, &buf, "c1.c2.c1");
    grn_expr_append_const(&context, expr, &buf);
    */

    grn_expr_append_op(&context, expr, GRN_OP_OBJ_GET_VALUE, 2);
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

void
test_expr_query(void)
{
  grn_obj *t1, *c1, *lc, *ft, *v, *expr;
  grn_obj textbuf, intbuf;
  grn_id r1, r2, r3, r4;

  /* actual table */
  t1 = grn_table_create(&context, "t1", 2, NULL,
			GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(t1);

  /* lexicon table */
  lc = grn_table_create(&context, "lc", 2, NULL,
			GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                        grn_ctx_at(&context, GRN_DB_SHORTTEXT), 0);
  cut_assert_not_null(lc);
  grn_test_assert(grn_obj_set_info(&context, lc, GRN_INFO_DEFAULT_TOKENIZER,
				   grn_ctx_at(&context, GRN_DB_BIGRAM)));

  /* actual column */
  c1 = grn_column_create(&context, t1, "c1", 2, NULL,
			 GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
			 grn_ctx_at(&context, GRN_DB_TEXT));
  cut_assert_not_null(c1);

  /* fulltext index */
  ft = grn_column_create(&context, lc, "ft", 2, NULL,
			 GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(ft);

  GRN_TEXT_INIT(&textbuf, 0);
  GRN_UINT32_INIT(&intbuf, 0);

  /* link between actual column and fulltext index */
  GRN_UINT32_SET(&context, &intbuf, grn_obj_id(&context, c1));
  grn_obj_set_info(&context, ft, GRN_INFO_SOURCE, &intbuf); /* need to use grn_id */

  /* insert row */
  r1 = grn_table_add(&context, t1, NULL, 0, NULL);
  cut_assert_equal_int(1, r1);
  GRN_TEXT_SETS(&context, &textbuf, "abcde");
  grn_test_assert(grn_obj_set_value(&context, c1, r1, &textbuf, GRN_OBJ_SET));

  r2 = grn_table_add(&context, t1, NULL, 0, NULL);
  cut_assert_equal_int(2, r2);
  GRN_TEXT_SETS(&context, &textbuf, "fghij");
  grn_test_assert(grn_obj_set_value(&context, c1, r2, &textbuf, GRN_OBJ_SET));

  r3 = grn_table_add(&context, t1, NULL, 0, NULL);
  cut_assert_equal_int(3, r3);
  GRN_TEXT_SETS(&context, &textbuf, "11 22 33");
  grn_test_assert(grn_obj_set_value(&context, c1, r3, &textbuf, GRN_OBJ_SET));

  r4 = grn_table_add(&context, t1, NULL, 0, NULL);
  cut_assert_equal_int(4, r4);
  GRN_TEXT_SETS(&context, &textbuf, "44 22 55");
  grn_test_assert(grn_obj_set_value(&context, c1, r4, &textbuf, GRN_OBJ_SET));

  /* confirm record are inserted in both column and index */
  cut_assert_equal_int(4, grn_table_size(&context, t1));
  cut_assert_equal_int(19, grn_table_size(&context, lc));

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));

  v = grn_expr_add_var(&context, expr, NULL, 0);

  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_const(&context, expr, &textbuf);
  GRN_UINT32_SET(&context, &intbuf, GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC);
  grn_expr_append_const(&context, expr, &intbuf);
  grn_expr_append_obj(&context, expr, t1);
  GRN_UINT32_SET(&context, &intbuf, 0);
  grn_expr_append_const(&context, expr, &intbuf);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_CREATE, 4);

  grn_expr_append_obj(&context, expr, v);
  grn_expr_append_op(&context, expr, GRN_OP_VAR_SET_VALUE, 2);

  grn_expr_append_obj(&context, expr, ft);
  GRN_TEXT_SETS(&context, &textbuf, "hij");
  grn_expr_append_const(&context, expr, &textbuf);
  grn_expr_append_obj(&context, expr, v);
  GRN_UINT32_SET(&context, &intbuf, GRN_SEL_OR);
  grn_expr_append_const(&context, expr, &intbuf);
  grn_expr_append_op(&context, expr, GRN_OP_OBJ_SEARCH, 4);

  grn_expr_append_obj(&context, expr, v);
  GRN_TEXT_SETS(&context, &textbuf, ".c1 .:score");
  grn_expr_append_const(&context, expr, &textbuf);
  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_obj(&context, expr, &textbuf);
  grn_expr_append_op(&context, expr, GRN_OP_JSON_PUT, 3);

  grn_expr_compile(&context, expr);

  grn_expr_exec(&context, expr);

  cut_assert_equal_uint(0, grn_obj_close(&context, expr));

  cut_assert_equal_substring("[[\"fghij\", 1]]",
                             GRN_TEXT_VALUE(&textbuf), GRN_TEXT_LEN(&textbuf));

  grn_obj_close(&context, &textbuf);
  grn_obj_close(&context, ft);
  grn_obj_close(&context, c1);
  grn_obj_close(&context, lc);
  grn_obj_close(&context, t1);
}

static grn_obj *docs, *terms, *size, *body, *index_body;

#define INSERT_DATA(str) {\
  uint32_t s = (uint32_t)strlen(str);\
  grn_id docid = grn_table_add(&context, docs, NULL, 0, NULL);\
  GRN_TEXT_SET(&context, &textbuf, str, s);\
  grn_test_assert(grn_obj_set_value(&context, body, docid, &textbuf, GRN_OBJ_SET));\
  GRN_UINT32_SET(&context, &intbuf, s);\
  grn_test_assert(grn_obj_set_value(&context, size, docid, &intbuf, GRN_OBJ_SET));\
}

static void
prepare_data(void)
{
  grn_obj textbuf, intbuf;

  GRN_TEXT_INIT(&textbuf, 0);
  GRN_UINT32_INIT(&intbuf, 0);

  docs = grn_table_create(&context, "docs", 4, NULL,
                          GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(docs);
  terms = grn_table_create(&context, "terms", 5, NULL,
                           GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(terms);
  size = grn_column_create(&context, docs, "size", 4, NULL,
                           GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                           grn_ctx_at(&context, GRN_DB_UINT32));
  cut_assert_not_null(size);
  body = grn_column_create(&context, docs, "body", 4, NULL,
                           GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                           grn_ctx_at(&context, GRN_DB_TEXT));
  cut_assert_not_null(body);

  index_body = grn_column_create(&context, terms, "docs_body", 4, NULL,
                                 GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT, docs);
  cut_assert_not_null(index_body);

  GRN_UINT32_SET(&context, &intbuf, grn_obj_id(&context, body));
  grn_obj_set_info(&context, index_body, GRN_INFO_SOURCE, &intbuf);

  INSERT_DATA("hoge");
  INSERT_DATA("fuga fuga");
  INSERT_DATA("moge mogge moge");
  INSERT_DATA("hoge hoge");
  INSERT_DATA("hoge fuga fuga");
  INSERT_DATA("hoge moge mogge moge");
  INSERT_DATA("moge hoge hoge");
  INSERT_DATA("moge hoge fuga fuga");
  INSERT_DATA("moge hoge moge mogge moge");
  INSERT_DATA("poyo moge hoge moge mogge moge");
}

void
test_table_scan(void)
{
  prepare_data();
}
