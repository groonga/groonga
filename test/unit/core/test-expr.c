/* -*- c-basic-offset: 2; coding: utf-8 -*- */
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
static grn_obj *cond, *res, *expr;
static grn_obj textbuf, intbuf;

void test_accessor(void);
void test_expr(void);
void test_persistent_expr(void);
void test_expr_query(void);

void test_table_select_equal(void);
void test_table_select_equal_indexed(void);
void test_table_select_select(void);
void test_table_select_search(void);
void test_table_select_select_search(void);
void test_table_select_match(void);
void test_table_select_match_equal(void);
void test_table_select_match_nonexistent(void);

void data_expr_parse(void);
void test_expr_parse(gconstpointer data);
void test_expr_set_value(void);
void test_expr_set_value_with_implicit_variable_reference(void);
void test_expr_set_value_with_query(void);
void test_expr_proc_call(void);
void test_expr_score_set(void);
void test_expr_key_equal(void);
void test_expr_value_access(void);
void test_expr_snip(void);
void test_expr_snip_without_tags(void);
void data_expr_comparison_operator(void);
void test_expr_comparison_operator(gconstpointer data);
void data_expr_arithmetic_operator(void);
void test_expr_arithmetic_operator(gconstpointer data);
void data_expr_arithmetic_operator_error(void);
void test_expr_arithmetic_operator_error(gconstpointer data);
void test_expr_parse_float(void);
void test_expr_parse_int32(void);
void test_expr_parse_int64(void);
void test_expr_parse_long_integer_literal(void);

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

  cond = NULL;
  expr = NULL;
  res = NULL;

  GRN_TEXT_INIT(&textbuf, 0);
  GRN_UINT32_INIT(&intbuf, 0);
}

void
cut_teardown(void)
{
  grn_obj_close(&context, &textbuf);
  grn_obj_close(&context, &intbuf);

  if (res)
    grn_obj_close(&context, res);
  if (expr)
    grn_obj_close(&context, expr);
  if (cond)
    grn_obj_close(&context, cond);

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
  grn_obj *t1, *t2, *c1, *c2, r1, r2;
  t1 = grn_table_create(&context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);
  t2 = grn_table_create(&context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t2);
  c1 = grn_column_create(&context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(&context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_RECORD_INIT(&r1, 0, grn_obj_id(&context, t1));
  GRN_RECORD_INIT(&r2, 0, grn_obj_id(&context, t2));
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(&context, t1, NULL, 0, NULL);
    i2 = grn_table_add(&context, t2, NULL, 0, NULL);
    GRN_RECORD_SET(&context, &r1, i1);
    GRN_RECORD_SET(&context, &r2, i2);
    grn_obj_set_value(&context, c1, i1, &r2, GRN_OBJ_SET);
    grn_obj_set_value(&context, c2, i2, &r1, GRN_OBJ_SET);
  }
  {
    grn_id id;
    uint64_t et;
    int nerr = 0;
    struct timeval tvb, tve;
    grn_obj *a = grn_obj_column(&context, t1, "c1.c2.c1", 8);
    grn_table_cursor *tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(a);
    cut_assert_not_null(tc);
    gettimeofday(&tvb, NULL);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_BULK_REWIND(&r2);
      grn_obj_get_value(&context, a, id, &r2);
      if (GRN_RECORD_VALUE(&r2) != id) { nerr++; }
    }
    gettimeofday(&tve, NULL);
    et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
    // printf("et=%zu\n", et);
    cut_assert_equal_uint(0, nerr);
    grn_test_assert(grn_table_cursor_close(&context, tc));
    grn_test_assert(grn_obj_close(&context, a));
  }
  grn_test_assert(grn_obj_close(&context, &r1));
  grn_test_assert(grn_obj_close(&context, &r2));
}

void
test_expr(void)
{
  int i;
  grn_obj *t1, *t2, *c1, *c2, r1, r2, buf;
  t1 = grn_table_create(&context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);
  t2 = grn_table_create(&context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t2);
  c1 = grn_column_create(&context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(&context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_TEXT_INIT(&buf, 0);
  GRN_RECORD_INIT(&r1, 0, grn_obj_id(&context, t1));
  GRN_RECORD_INIT(&r2, 0, grn_obj_id(&context, t2));
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(&context, t1, NULL, 0, NULL);
    i2 = grn_table_add(&context, t2, NULL, 0, NULL);
    GRN_RECORD_SET(&context, &r1, i1);
    GRN_RECORD_SET(&context, &r2, i2);
    grn_obj_set_value(&context, c1, i1, &r2, GRN_OBJ_SET);
    grn_obj_set_value(&context, c2, i2, &r1, GRN_OBJ_SET);
  }
  {
    grn_obj *r, *v;

    expr = grn_expr_create(&context, NULL, 0);
    cut_assert_not_null(expr);
    v = grn_expr_add_var(&context, expr, NULL, 0);
    GRN_RECORD_INIT(v, 0, grn_obj_id(&context, t1));
    grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);

    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c2");
    grn_expr_append_const(&context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf, GRN_OP_PUSH, 1);


//    GRN_TEXT_SETS(&context, &buf, "c1.c2.c1");
//    grn_expr_append_const(&context, expr, &buf);

    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    grn_expr_compile(&context, expr);
    {
      grn_id id;
      uint64_t et;
      int nerr = 0;
      grn_table_cursor *tc;
      struct timeval tvb, tve;
      tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0, -1, 0);
      cut_assert_not_null(tc);
      gettimeofday(&tvb, NULL);
      while ((id = grn_table_cursor_next(&context, tc))) {
        GRN_RECORD_SET(&context, v, id);
        grn_expr_exec(&context, expr, 0);
        r = grn_ctx_pop(&context);
        if (GRN_RECORD_VALUE(r) != id) { nerr++; }
      }
      gettimeofday(&tve, NULL);
      et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
      // printf("et=%zu\n", et);
      cut_assert_equal_uint(0, nerr);
      grn_test_assert(grn_table_cursor_close(&context, tc));
    }
  }
  grn_test_assert(grn_obj_close(&context, &r1));
  grn_test_assert(grn_obj_close(&context, &r2));
  grn_test_assert(grn_obj_close(&context, &buf));
}

#ifdef ENABLE_PERSISTENT_EXPR
void
test_persistent_expr(void)
{
  int i;
  grn_obj *t1, *t2, *c1, *c2, r1, r2, buf;
  t1 = grn_table_create(&context, "t1", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);
  t2 = grn_table_create(&context, "t2", 2, NULL,
                        GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t2);
  c1 = grn_column_create(&context, t1, "c1", 2, NULL,
                         GRN_OBJ_PERSISTENT, t2);
  cut_assert_not_null(c1);
  c2 = grn_column_create(&context, t2, "c2", 2, NULL,
                         GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(c2);
  GRN_TEXT_INIT(&buf, 0);
  GRN_RECORD_INIT(&r1, 0, grn_obj_id(&context, t1));
  GRN_RECORD_INIT(&r2, 0, grn_obj_id(&context, t2));
  for (i = 0; i < NRECORDS; i++) {
    grn_id i1, i2;
    i1 = grn_table_add(&context, t1, NULL, 0, NULL);
    i2 = grn_table_add(&context, t2, NULL, 0, NULL);
    GRN_RECORD_SET(&context, &r1, i1);
    GRN_RECORD_SET(&context, &r2, i2);
    grn_obj_set_value(&context, c1, i1, &r2, GRN_OBJ_SET);
    grn_obj_set_value(&context, c2, i2, &r1, GRN_OBJ_SET);
  }
  {
    grn_obj *v;
    expr = grn_expr_create(&context, "test", 4);
    cut_assert_not_null(expr);
    v = grn_expr_add_var(&context, expr, "foo", 3);
    GRN_RECORD_INIT(v, 0, grn_obj_id(&context, t1));
    grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);

    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c2");
    grn_expr_append_const(&context, expr, &buf, GRN_OP_PUSH, 1);
    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    GRN_TEXT_SETS(&context, &buf, "c1");
    grn_expr_append_const(&context, expr, &buf, GRN_OP_PUSH, 1);

    /*
    GRN_TEXT_SETS(&context, &buf, "c1.c2.c1");
    grn_expr_append_const(&context, expr, &buf);
    */

    grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
    grn_expr_compile(&context, expr);
    grn_test_assert(grn_obj_close(&context, expr));
    expr = NULL;
  }
  grn_test_assert(grn_obj_close(&context, &buf));

  grn_db_close(&context, database);
  database = grn_db_open(&context, path);

  {
    grn_id id;
    uint64_t et;
    int nerr = 0;
    grn_obj *r, *v;
    grn_table_cursor *tc;
    struct timeval tvb, tve;
    expr = grn_ctx_get(&context, "test", 4);
    v = grn_expr_get_var(&context, expr, "foo", 3);
    t1 = grn_ctx_get(&context, "t1", 2);
    tc = grn_table_cursor_open(&context, t1, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(tc);
    gettimeofday(&tvb, NULL);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_RECORD_SET(&context, v, id);
      grn_expr_exec(&context, expr, 0);
      r = grn_ctx_pop(&context);
      if (GRN_RECORD_VALUE(r) != id) { nerr++; }
    }
    gettimeofday(&tve, NULL);
    et = (tve.tv_sec - tvb.tv_sec) * 1000000 + (tve.tv_usec - tvb.tv_usec);
    // printf("et=%zu\n", et);
    cut_assert_equal_uint(0, nerr);
    grn_test_assert(grn_table_cursor_close(&context, tc));
  }
  grn_test_assert(grn_obj_close(&context, &r1));
  grn_test_assert(grn_obj_close(&context, &r2));
}
#endif /* ENABLE_PERSISTENT_EXPR */

void
test_expr_query(void)
{
  grn_obj *t1, *c1, *lc, *ft, *v;
  grn_id r1, r2, r3, r4;

  /* actual table */
  t1 = grn_table_create(&context, "t1", 2, NULL,
			GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(t1);

  /* lexicon table */
  lc = grn_table_create(&context, "lc", 2, NULL,
			GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                        grn_ctx_at(&context, GRN_DB_SHORT_TEXT), NULL);
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
			 GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT|GRN_OBJ_WITH_POSITION, t1);
  cut_assert_not_null(ft);

  /* link between actual column and fulltext index */
  GRN_UINT32_SET(&context, &intbuf, grn_obj_id(&context, c1));
  grn_obj_set_info(&context, ft, GRN_INFO_SOURCE, &intbuf); /* need to use grn_id */

  /* insert row */
  r1 = grn_table_add(&context, t1, NULL, 0, NULL);
  cut_assert_equal_int(1, r1);
  GRN_TEXT_SETS(&context, &textbuf, "abhij");
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
  cut_assert_equal_int(20, grn_table_size(&context, lc));

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));

  v = grn_expr_add_var(&context, expr, NULL, 0);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);

  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, t1, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, 0);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_CREATE, 4);

  grn_expr_append_op(&context, expr, GRN_OP_ASSIGN, 2);

  grn_expr_append_obj(&context, expr, ft, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "hij");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_OP_OR);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_OBJ_SEARCH, 4);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, ".c1 .:score");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_obj(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_JSON_PUT, 3);

  grn_expr_compile(&context, expr);

  grn_expr_exec(&context, expr, 0);

  cut_assert_equal_substring("[[2],[\"abhij\",1],[\"fghij\",1]]",
                             GRN_TEXT_VALUE(&textbuf), GRN_TEXT_LEN(&textbuf));

  grn_obj_close(&context, ft);
  grn_obj_close(&context, c1);
  grn_obj_close(&context, lc);
  grn_obj_close(&context, t1);
}

static grn_obj *docs, *terms, *size, *body, *index_body;

static void
insert_document(const gchar *body_content)
{
  uint32_t s = (uint32_t)strlen(body_content);
  grn_id docid = grn_table_add(&context, docs, NULL, 0, NULL);
  GRN_TEXT_SET(&context, &textbuf, body_content, s);
  grn_test_assert(grn_obj_set_value(&context, body, docid, &textbuf, GRN_OBJ_SET));
  GRN_UINT32_SET(&context, &intbuf, s);
  grn_test_assert(grn_obj_set_value(&context, size, docid, &intbuf, GRN_OBJ_SET));
}

#define INSERT_DOCUMENT(body) \
  cut_trace(insert_document(body))

static void grn_test_assert_select(const GList* expected, grn_obj *result);
static void grn_test_assert_select_all(grn_obj *result);
static void grn_test_assert_select_none(grn_obj *result);

static void
grn_test_assert_select(const GList *expected, grn_obj *result)
{
  GList *records = NULL;
  grn_table_cursor *cursor;
  cursor = grn_table_cursor_open(&context, result, NULL, 0, NULL, 0, 0, -1, 0);
  cut_assert_not_null(cursor);
  while (grn_table_cursor_next(&context, cursor) != GRN_ID_NIL) {
    void *value;
    int size;
    grn_obj record_value;
    GString *null_terminated_key;

    grn_table_cursor_get_key(&context, cursor, &value);
    GRN_TEXT_INIT(&record_value, 0);
    grn_obj_get_value(&context, body, *((grn_id *)value), &record_value);
    value = GRN_TEXT_VALUE(&record_value);
    size = GRN_TEXT_LEN(&record_value);

    null_terminated_key = g_string_new_len(value, size);
    records = g_list_append(records, null_terminated_key->str);
    g_string_free(null_terminated_key, FALSE);
  }
  grn_test_assert(grn_table_cursor_close(&context, cursor));
  gcut_take_list(records, g_free);
  expected = g_list_sort((GList *)expected, (GCompareFunc)g_utf8_collate);
  records = g_list_sort((GList *)records, (GCompareFunc)g_utf8_collate);
  gcut_assert_equal_list_string(expected, records);
}

static void
grn_test_assert_select_all(grn_obj *result)
{
  grn_test_assert_select(gcut_take_new_list_string("hoge",
                                                   "fuga fuga",
                                                   "moge moge moge",
                                                   "hoge hoge",
                                                   "hoge fuga fuga",
                                                   "hoge moge moge moge",
                                                   "moge hoge hoge",
                                                   "moge hoge fuga fuga",
                                                   "moge hoge moge moge moge",
                                                   "poyo moge hoge "
                                                     "moge moge moge",
                                                   NULL), result);
}

static void
grn_test_assert_select_none(grn_obj *result)
{
  cut_assert_equal_uint(0, grn_table_size(&context, result));
}

static void
create_documents_table(void)
{
  docs = grn_table_create(&context, "docs", 4, NULL,
                          GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, NULL);
  cut_assert_not_null(docs);

  size = grn_column_create(&context, docs, "size", 4, NULL,
                           GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                           grn_ctx_at(&context, GRN_DB_UINT32));
  cut_assert_not_null(size);

  body = grn_column_create(&context, docs, "body", 4, NULL,
                           GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                           grn_ctx_at(&context, GRN_DB_TEXT));
  cut_assert_not_null(body);
}

static void
create_terms_table(void)
{
  terms = grn_table_create(&context, "terms", 5, NULL,
                           GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                           grn_ctx_at(&context, GRN_DB_SHORT_TEXT), NULL);
  cut_assert_not_null(terms);
  grn_test_assert(grn_obj_set_info(&context, terms, GRN_INFO_DEFAULT_TOKENIZER,
				   grn_ctx_at(&context, GRN_DB_BIGRAM)));

  index_body = grn_column_create(&context, terms, "docs_body", 4, NULL,
                                 GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT|GRN_OBJ_WITH_POSITION,
                                 docs);
  cut_assert_not_null(index_body);

  GRN_UINT32_SET(&context, &intbuf, grn_obj_id(&context, body));
  grn_obj_set_info(&context, index_body, GRN_INFO_SOURCE, &intbuf);
}

static void
insert_data(void)
{
  INSERT_DOCUMENT("hoge");
  INSERT_DOCUMENT("fuga fuga");
  INSERT_DOCUMENT("moge moge moge");
  INSERT_DOCUMENT("hoge hoge");
  INSERT_DOCUMENT("hoge fuga fuga");
  INSERT_DOCUMENT("hoge moge moge moge");
  INSERT_DOCUMENT("moge hoge hoge");
  INSERT_DOCUMENT("moge hoge fuga fuga");
  INSERT_DOCUMENT("moge hoge moge moge moge");
  INSERT_DOCUMENT("poyo moge hoge moge moge moge");
}

static void
prepare_data(void)
{
  create_documents_table();
  create_terms_table();
  insert_data();
}

void
test_table_select_equal(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "body");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_TEXT_SETS(&context, &textbuf, "poyo moge hoge moge moge moge");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(gcut_take_new_list_string("poyo moge hoge "
                                                   "moge moge moge",
                                                   NULL), res);
}

void
test_table_select_equal_indexed(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "body");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_GET_VALUE, 2);
  GRN_TEXT_SETS(&context, &textbuf, "hoge");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(gcut_take_new_list_string("hoge", NULL), res);
}

void
test_table_select_select(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "size");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &intbuf, 14);
  grn_expr_append_const(&context, cond, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));
  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, cond, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, res, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_OP_OR);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_SELECT, 4);

  grn_expr_exec(&context, expr, 0);

  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL), res);
}

void
test_table_select_search(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "size");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &intbuf, 14);
  grn_expr_append_const(&context, cond, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));

  v = grn_expr_add_var(&context, expr, NULL, 0);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);

  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, 0);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_CREATE, 4);

  grn_expr_append_op(&context, expr, GRN_OP_ASSIGN, 2);

  grn_expr_append_obj(&context, expr, index_body, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "moge");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_OP_OR);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_OBJ_SEARCH, 4);

  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, cond, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_OP_AND);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_SELECT, 4);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, ".size .:score .body");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_obj(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_JSON_PUT, 3);

  grn_expr_exec(&context, expr, 0);

  cut_assert_equal_substring("[[2],[14,4,\"moge moge moge\"],[14,2,\"moge hoge hoge\"]]",
                             GRN_TEXT_VALUE(&textbuf), GRN_TEXT_LEN(&textbuf));
}

void
test_table_select_select_search(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "size");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &intbuf, 14);
  grn_expr_append_const(&context, cond, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));

  v = grn_expr_add_var(&context, expr, NULL, 0);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);

  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, 0);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_CREATE, 4);

  grn_expr_append_op(&context, expr, GRN_OP_ASSIGN, 2);

  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, cond, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_OP_OR);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_SELECT, 4);

  grn_expr_append_obj(&context, expr, index_body, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "moge");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &intbuf, GRN_OP_AND);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_OBJ_SEARCH, 4);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, ".size .:score .body");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  GRN_BULK_REWIND(&textbuf);
  grn_expr_append_obj(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_JSON_PUT, 3);

  grn_expr_exec(&context, expr, 0);

  cut_assert_equal_substring("[[2],[14,4,\"moge moge moge\"],[14,2,\"moge hoge hoge\"]]",
                             GRN_TEXT_VALUE(&textbuf), GRN_TEXT_LEN(&textbuf));
}

void
test_table_select_match(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "body");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_TEXT_SETS(&context, &textbuf, "moge");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_MATCH, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "hoge moge moge moge",
                                                   "moge hoge hoge",
                                                   "moge hoge fuga fuga",
                                                   "moge hoge moge moge moge",
                                                   "poyo moge hoge "
                                                     "moge moge moge",
                                                   NULL), res);
}

void
test_table_select_match_equal(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));

  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "body");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_TEXT_SETS(&context, &textbuf, "moge");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_MATCH, 2);

  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "size");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &intbuf, 14);
  grn_expr_append_const(&context, cond, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);

  grn_expr_append_op(&context, cond, GRN_OP_AND, 2);

  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "moge hoge hoge",
                                                   NULL), res);
}

void
test_table_select_match_nonexistent(void)
{
  grn_obj *v;

  create_documents_table();
  insert_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));

  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "body");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_TEXT_SETS(&context, &textbuf, "moge");
  grn_expr_append_const(&context, cond, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_MATCH, 2);

  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(NULL, res);
}

#define PARSE(expr,str,flags) \
  grn_test_assert(grn_expr_parse(&context, (expr), (str), strlen(str), \
                                 body, GRN_OP_MATCH, GRN_OP_AND, flags))

static void
grn_assert_expr(gchar *inspected, grn_obj *expr)
{
  grn_obj strbuf;
  GRN_TEXT_INIT(&strbuf, 0);
  grn_expr_inspect(&context, &strbuf, expr);
  GRN_TEXT_PUTC(&context, &strbuf, '\0');
  cut_assert_equal_string(inspected, GRN_TEXT_VALUE(&strbuf));
  GRN_OBJ_FIN(&context, &strbuf);
}

void
data_expr_parse(void)
{
#define ADD_DATUM(label,                                                \
                  query_hoge_moge, query_hoge_moge_parse_level,         \
                  query_poyo, query_poyo_parse_level,                   \
                  query_size, query_size_parse_level)                   \
  gcut_add_datum(label,                                                 \
                 "query_hoge_moge", G_TYPE_STRING, query_hoge_moge,     \
                 "query_hoge_moge_parse_level",                         \
                 G_TYPE_INT, query_hoge_moge_parse_level,               \
                 "query_poyo", G_TYPE_STRING, query_poyo,               \
                 "query_poyo_parse_level",                              \
                 G_TYPE_INT, query_poyo_parse_level,                    \
                 "query_size", G_TYPE_STRING, query_size,               \
                 "query_size_parse_level",                              \
                 G_TYPE_INT, query_size_parse_level,                    \
                 NULL)

  ADD_DATUM("column query parse level",
            "hoge + moge",
            GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN,
            "poyo", GRN_EXPR_SYNTAX_QUERY,
            "size:14", GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_COLUMN);
  ADD_DATUM("table query parse level",
            "body:%hoge + body:%moge",
            GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN,
            "body:%poyo", GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_COLUMN,
            "size:14", GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_COLUMN);
  ADD_DATUM("expression parse level",
            "body@\"hoge\" && body@\"moge\"",
            GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE,
            "body@\"poyo\"", GRN_EXPR_SYNTAX_SCRIPT,
            "size == 14", GRN_EXPR_SYNTAX_SCRIPT);
#undef ADD_DATUM
}

void
test_expr_parse(gconstpointer data)
{
  grn_obj *v;

  prepare_data();

  cond = grn_expr_create(&context, NULL, 0);
  cut_assert_not_null(cond);
  v = grn_expr_add_var(&context, cond, NULL, 0);
  cut_assert_not_null(v);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  PARSE(cond, gcut_data_get_string(data, "query_hoge_moge"),
              gcut_data_get_int(data, "query_hoge_moge_parse_level"));
  PARSE(cond, gcut_data_get_string(data, "query_poyo"),
              gcut_data_get_int(data, "query_poyo_parse_level"));
  grn_expr_append_op(&context, cond, GRN_OP_AND, 2);
  grn_assert_expr("noname(?0:\"\"){body GET_VALUE \"hoge\" MATCH "
                                  "body GET_VALUE \"moge\" MATCH AND "
                                  "body GET_VALUE \"poyo\" MATCH AND}", cond);
  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);
  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(gcut_take_new_list_string("poyo moge hoge "
                                                   "moge moge moge",
                                                   NULL), res);
  grn_test_assert(grn_obj_close(&context, res));
  res = NULL;
  grn_test_assert(grn_obj_close(&context, cond));
  cond = NULL;

  cond = grn_expr_create(&context, NULL, 0);
  cut_assert_not_null(cond);
  v = grn_expr_add_var(&context, cond, NULL, 0);
  cut_assert_not_null(v);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  PARSE(cond, gcut_data_get_string(data, "query_size"),
              gcut_data_get_int(data, "query_size_parse_level"));
  grn_assert_expr("noname(?0:\"\"){size GET_VALUE 14 EQUAL}", cond);
  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);
  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL), res);
}

void
test_expr_set_value(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "size:14", GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL), res);
  grn_test_assert(grn_obj_close(&context, res));
  res = NULL;

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &textbuf, "size");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &intbuf, 14);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_ASSIGN, 2);
  {
    grn_id id;
    grn_table_cursor *tc;
    tc = grn_table_cursor_open(&context, docs, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(tc);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_RECORD_SET(&context, v, id);
      grn_expr_exec(&context, expr, 0);
    }
    grn_test_assert(grn_table_cursor_close(&context, tc));
  }

  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select_all(res);
}

void
test_expr_set_value_with_implicit_variable_reference(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "size:14",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL), res);
  grn_test_assert(grn_obj_close(&context, res));
  res = NULL;

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);

  GRN_TEXT_SETS(&context, &textbuf, "size");
  grn_expr_append_const(&context, expr, &textbuf, GRN_OP_GET_VALUE, 1);
  GRN_UINT32_SET(&context, &intbuf, 14);
  grn_expr_append_const(&context, expr, &intbuf, GRN_OP_ASSIGN, 2);
  {
    grn_id id;
    grn_table_cursor *tc;
    tc = grn_table_cursor_open(&context, docs, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(tc);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_RECORD_SET(&context, v, id);
      grn_expr_exec(&context, expr, 0);
    }
    grn_test_assert(grn_table_cursor_close(&context, tc));
  }

  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select_all(res);
}

void
test_expr_set_value_with_query(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "size:14",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL), res);
  grn_test_assert(grn_obj_close(&context, res));
  res = NULL;

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);
  PARSE(expr, "size = 14", GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
  {
    grn_id id;
    grn_table_cursor *tc;
    tc = grn_table_cursor_open(&context, docs, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(tc);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_RECORD_SET(&context, v, id);
      grn_expr_exec(&context, expr, 0);
    }
    grn_test_assert(grn_table_cursor_close(&context, tc));
  }

  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select_all(res);
}

void
test_expr_proc_call(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "size:>14",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select(gcut_take_new_list_string("hoge moge moge moge",
                                                   "moge hoge fuga fuga",
                                                   "moge hoge moge moge moge",
                                                   "poyo moge hoge "
                                                     "moge moge moge",
                                                   NULL), res);
  grn_test_assert(grn_obj_close(&context, res));
  res = NULL;

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);
  PARSE(expr, "size = rand(14)", GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
  {
    grn_id id;
    grn_table_cursor *tc;
    tc = grn_table_cursor_open(&context, docs, NULL, 0, NULL, 0, 0, -1, 0);
    cut_assert_not_null(tc);
    while ((id = grn_table_cursor_next(&context, tc))) {
      GRN_RECORD_SET(&context, v, id);
      grn_expr_exec(&context, expr, 0);
    }
    grn_test_assert(grn_table_cursor_close(&context, tc));
  }

  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select_none(res);
}

void
test_expr_score_set(void)
{
  grn_obj *v, *res2;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "size:>0",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select_all(res);
  grn_test_assert(grn_obj_close(&context, cond));
  cond = NULL;

  GRN_EXPR_CREATE_FOR_QUERY(&context, res, expr, v);
  PARSE(expr, "_score = size",
        GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
  GRN_TABLE_EACH(&context, res, 0, 0, id, NULL, 0, NULL, {
    GRN_RECORD_SET(&context, v, id);
    grn_expr_exec(&context, expr, 0);
  });

  GRN_EXPR_CREATE_FOR_QUERY(&context, res, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "_score:>9",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res2 = grn_table_select(&context, res, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res2);
  grn_test_assert_select(gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "hoge moge moge moge",
                                                   "moge hoge hoge",
                                                   "moge hoge fuga fuga",
                                                   "moge hoge moge moge moge",
                                                   "poyo moge hoge "
                                                   "moge moge moge",
                                                   NULL), res2);
  grn_test_assert(grn_obj_close(&context, res2));
}

void
test_expr_key_equal(void)
{
  grn_obj *v;

  docs = grn_table_create(&context, "docs", 4, NULL,
                          GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                          grn_ctx_at(&context, GRN_DB_SHORT_TEXT), NULL);

  grn_table_add(&context, docs, "hoge", 4, NULL);
  grn_table_add(&context, docs, "moge", 4, NULL);

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "_key:moge",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  cut_assert_equal_uint(1, grn_table_size(&context, res));
  grn_test_assert(grn_obj_close(&context, res));
  res = NULL;
  grn_test_assert(grn_obj_close(&context, cond));
  cond = NULL;

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "_key:poge",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  cut_assert_equal_uint(0, grn_table_size(&context, res));
}

void
test_expr_value_access(void)
{
  grn_id id;
  grn_obj *v;

  docs = grn_table_create(&context, "docs", 4, NULL,
                          GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                          grn_ctx_at(&context, GRN_DB_SHORT_TEXT),
                          grn_ctx_at(&context, GRN_DB_INT32));

  id = grn_table_add(&context, docs, "one", 3, NULL);
  GRN_UINT32_SET(&context, &intbuf, 1);
  grn_test_assert(grn_obj_set_value(&context, docs, id, &intbuf, GRN_OBJ_SET));
  id = grn_table_add(&context, docs, "two", 3, NULL);
  GRN_UINT32_SET(&context, &intbuf, 2);
  grn_test_assert(grn_obj_set_value(&context, docs, id, &intbuf, GRN_OBJ_SET));

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "_value:1",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  cut_assert_equal_uint(1, grn_table_size(&context, res));
  grn_test_assert(grn_obj_close(&context, res));
  res = NULL;
  grn_test_assert(grn_obj_close(&context, cond));
  cond = NULL;

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);
  PARSE(expr, "_value = 5", GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE);
  GRN_TABLE_EACH(&context, docs, 0, 0, id, NULL, 0, NULL, {
    GRN_RECORD_SET(&context, v, id);
    grn_expr_exec(&context, expr, 0);
  });

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "_value:5",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  cut_assert_equal_uint(2, grn_table_size(&context, res));
}

void
test_expr_snip(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);
  cut_assert_not_null(expr);

  PARSE(expr, "search engine column",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA);

  {
    grn_snip *snip;
    int flags = 0;
    unsigned int width, max_results;
    const char *open_tags[] = {"[[", "<"};
    const char *close_tags[] = {"]]", ">"};
    unsigned int open_tag_lens[] = {2, 1};
    unsigned int close_tag_lens[] = {2, 1};
    unsigned int n_results;
    unsigned int max_tagged_len;
    gchar *result;
    unsigned int result_len;
    gchar text[] =
      "groonga is an open-source fulltext search engine and column store.\n"
      "It lets you write high-performance applications that requires "
      "fulltext search.";

    width = 100;
    max_results = 10;
    snip = grn_expr_snip(&context, expr, flags,
                         width, max_results,
                         sizeof(open_tags) / sizeof(open_tags[0]),
                         open_tags, open_tag_lens,
                         close_tags, close_tag_lens,
                         NULL);
    cut_assert_not_null(snip);

    grn_test_assert(grn_snip_exec(&context, snip, text, strlen(text),
                                  &n_results, &max_tagged_len));

    cut_assert_equal_uint(2, n_results);
    cut_assert_equal_uint(111, max_tagged_len);
    result = g_new(gchar, max_tagged_len);
    cut_take_memory(result);

    grn_test_assert(grn_snip_get_result(&context, snip, 0, result, &result_len));
    cut_assert_equal_string("groonga is an open-source fulltext "
                            "[[search]] <engine> and [[column]] store.\n"
                            "It lets you write high-performanc",
                            result);
    cut_assert_equal_uint(110, result_len);

    grn_test_assert(grn_snip_get_result(&context, snip, 1, result, &result_len));
    cut_assert_equal_string("e applications that requires "
                            "fulltext [[search]].",
                            result);
    cut_assert_equal_uint(49, result_len);

    grn_test_assert(grn_snip_close(&context, snip));
  }
}

void
test_expr_snip_without_tags(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);
  cut_assert_not_null(expr);

  PARSE(expr, "search engine column",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA);

  {
    grn_snip *snip;
    int flags = 0;
    unsigned int width, max_results;
    unsigned int n_results;
    unsigned int max_tagged_len;
    gchar *result;
    unsigned int result_len;
    gchar text[] =
      "groonga is an open-source fulltext search engine and column store.\n"
      "It lets you write high-performance applications that requires "
      "fulltext search.";

    width = 100;
    max_results = 10;
    snip = grn_expr_snip(&context, expr, flags,
                         width, max_results,
                         0,
                         NULL, NULL,
                         NULL, NULL,
                         NULL);
    cut_assert_not_null(snip);

    grn_test_assert(grn_snip_exec(&context, snip, text, strlen(text),
                                  &n_results, &max_tagged_len));

    cut_assert_equal_uint(2, n_results);
    cut_assert_equal_uint(101, max_tagged_len);
    result = g_new(gchar, max_tagged_len);
    cut_take_memory(result);

    grn_test_assert(grn_snip_get_result(&context, snip, 0, result, &result_len));
    cut_assert_equal_string("groonga is an open-source fulltext "
                            "search engine and column store.\n"
                            "It lets you write high-performanc",
                            result);
    cut_assert_equal_uint(100, result_len);

    grn_test_assert(grn_snip_get_result(&context, snip, 1, result, &result_len));
    cut_assert_equal_string("e applications that requires "
                            "fulltext search.",
                            result);
    cut_assert_equal_uint(45, result_len);

    grn_test_assert(grn_snip_close(&context, snip));
  }
}

void
data_expr_comparison_operator(void)
{
#define ADD_DATUM(label, expected_keys, query)                          \
  gcut_add_datum(label,                                                 \
                 "expected_keys", G_TYPE_POINTER, expected_keys,        \
                 gcut_list_string_free,                                 \
                 "query", G_TYPE_STRING, query,                         \
                 NULL)

  ADD_DATUM("<",
            gcut_list_string_new("hoge", NULL),
            "size < 9");
  ADD_DATUM("<=",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 9");
  ADD_DATUM(">",
            gcut_list_string_new("moge hoge moge moge moge",
                                 "poyo moge hoge moge moge moge",
                                 NULL),
            "size > 19");
  ADD_DATUM(">=",
            gcut_list_string_new("hoge moge moge moge",
                                 "moge hoge fuga fuga",
                                 "moge hoge moge moge moge",
                                 "poyo moge hoge moge moge moge",
                                 NULL),
            "size >= 19");

#undef ADD_DATUM
}

void
test_expr_comparison_operator(gconstpointer data)
{
  grn_obj *v;

  prepare_data();

  cond = grn_expr_create(&context, NULL, 0);
  cut_assert_not_null(cond);
  v = grn_expr_add_var(&context, cond, NULL, 0);
  cut_assert_not_null(v);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  PARSE(cond,
        gcut_data_get_string(data, "query"),
        GRN_EXPR_SYNTAX_SCRIPT);

  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select(gcut_data_get_pointer(data, "expected_keys"),
                         res);
}

void
data_expr_arithmetic_operator(void)
{
#define ADD_DATUM(label, expected_keys, query)                          \
  gcut_add_datum(label,                                                 \
                 "expected_keys", G_TYPE_POINTER, expected_keys,        \
                 gcut_list_string_free,                                 \
                 "query", G_TYPE_STRING, query,                         \
                 NULL)

  ADD_DATUM("unary +",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= +9");
  ADD_DATUM("+",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= (4 + 5)");
  ADD_DATUM("string +",
            gcut_list_string_new("fuga fuga", NULL),
            "body == \"fuga \" + \"fuga\"");
  ADD_DATUM("string + int",
            gcut_list_string_new("nick NICK 29", NULL),
            "body == \"nick NICK \" + 29");
  ADD_DATUM("int + string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 4 + \"5\"");
  ADD_DATUM("int + float",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 4 + 5.0001");

  ADD_DATUM("unary -",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= -5 + 14");
  ADD_DATUM("-",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 14 - 5");
  ADD_DATUM("int - string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 14 - \"5\"");
  ADD_DATUM("float - int",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 14.1 - 5");

  ADD_DATUM("*",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 3 * 3");
  ADD_DATUM("int * string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 3 * \"3\"");
  ADD_DATUM("float * int",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 3.1 * 3");

  ADD_DATUM("/",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 27 / 3");
  ADD_DATUM("int * string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 27 / \"3\"");
  ADD_DATUM("float * int",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 27.1 / 3");

#undef ADD_DATUM
}

void
test_expr_arithmetic_operator(gconstpointer data)
{
  grn_obj *v;

  prepare_data();
  INSERT_DOCUMENT("nick NICK 29");

  cond = grn_expr_create(&context, NULL, 0);
  cut_assert_not_null(cond);
  v = grn_expr_add_var(&context, cond, NULL, 0);
  cut_assert_not_null(v);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  PARSE(cond,
        gcut_data_get_string(data, "query"),
        GRN_EXPR_SYNTAX_SCRIPT);

  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_context(&context);
  grn_test_assert_select(gcut_data_get_pointer(data, "expected_keys"),
                         res);
}

void
data_expr_arithmetic_operator_error(void)
{
#define ADD_DATUM(label, rc, message, query)                    \
  gcut_add_datum(label,                                         \
                 "rc", G_TYPE_UINT, rc,                         \
                 "message", G_TYPE_STRING, message,             \
                 "query", G_TYPE_STRING, query,                 \
                 NULL)

  ADD_DATUM("int + string",
            GRN_INVALID_ARGUMENT,
            "not a numerical format: <hoge>",
            "size == 100 + \"hoge\"");

  ADD_DATUM("string - string",
            GRN_INVALID_ARGUMENT,
            "\"string\" - \"string\" isn't supported",
            "body == \"fuga\" - \"hoge\"");

  ADD_DATUM("string * string",
            GRN_INVALID_ARGUMENT,
            "\"string\" * \"string\" isn't supported",
            "body == \"fuga\" * \"hoge\"");

  ADD_DATUM("string / string",
            GRN_INVALID_ARGUMENT,
            "\"string\" / \"string\" isn't supported",
            "body == \"fuga\" / \"hoge\"");

#undef ADD_DATUM
}

void
test_expr_arithmetic_operator_error(gconstpointer data)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  PARSE(cond,
        gcut_data_get_string(data, "query"),
        GRN_EXPR_SYNTAX_SCRIPT);

  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_error(gcut_data_get_uint(data, "rc"),
                        gcut_data_get_string(data, "message"),
                        &context);
}

static grn_obj *
parse_numeric_literal(const char *str_expr)
{
  grn_obj *expr;
  grn_obj *var;

  expr = grn_expr_create(&context, NULL, 0);
  cut_assert_not_null(expr);
  var = grn_expr_add_var(&context, expr, "var", strlen("var"));
  cut_assert_not_null(var);

  GRN_FLOAT_INIT(var, 0);
  grn_test_assert(grn_expr_parse(&context, expr, str_expr, strlen(str_expr),
                  NULL, GRN_OP_NOP, GRN_OP_NOP, GRN_EXPR_SYNTAX_SCRIPT));

  grn_test_assert(grn_expr_compile(&context, expr));
  grn_test_assert(grn_expr_exec(&context, expr, 0));

  return var;
}

void
test_expr_parse_float(void)
{
  grn_obj *var;
  const char *str_expr = "var = 3.14159265";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_FLOAT, GRN_OBJ_GET_DOMAIN(var));
  cut_assert_equal_double(3.14159265, 0.00000001, GRN_FLOAT_VALUE(var));
}

void
test_expr_parse_int32(void)
{
  grn_obj *var;
  const char *str_expr = "var = 123456";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_INT32, GRN_OBJ_GET_DOMAIN(var));
  cut_assert_equal_int(123456, GRN_INT32_VALUE(var));
}

void
test_expr_parse_int64(void)
{
  grn_obj *var;
  const char *str_expr = "var = 123456789012";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_INT64, GRN_OBJ_GET_DOMAIN(var));
  cut_assert_equal_int_least64(G_GINT64_CONSTANT(123456789012),
                               GRN_INT64_VALUE(var));
}

void
test_expr_parse_long_integer_literal(void)
{
  grn_obj *var;
  const char *str_expr = "var = 12345678901234567890";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_FLOAT, GRN_OBJ_GET_DOMAIN(var));
  //IEEE 754 says "double" can presisely hold about 16 digits.
  //To be safe, assume only 15 digits are preserved.
  cut_assert_equal_double(12345678901234567890., 100000., GRN_FLOAT_VALUE(var));
}
