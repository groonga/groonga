/*
  Copyright(C) 2009  Brazil
  Copyright(C) 2011  Kouhei Sutou <kou@clear-code.com>

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
static grn_ctx context;
static grn_obj *database;
static grn_obj *cond, *res, *expr;
static grn_obj text_buf, int_buf, ptr_buf;

void test_equal(void);
void test_equal_indexed(void);
void test_equal_by_existent_reference_key(void);
void test_select(void);
void test_search(void);
void test_select_search(void);
void test_match(void);
void test_match_equal(void);
void test_match_without_index(void);

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "test-table-select",
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
  path = g_build_filename(tmp_directory, "text-table-select", NULL);
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, path, NULL);

  cond = NULL;
  expr = NULL;
  res = NULL;

  GRN_TEXT_INIT(&text_buf, 0);
  GRN_UINT32_INIT(&int_buf, 0);
  GRN_PTR_INIT(&ptr_buf, 0, GRN_ID_NIL);
}

void
cut_teardown(void)
{
  grn_obj_close(&context, &text_buf);
  grn_obj_close(&context, &int_buf);
  grn_obj_close(&context, &ptr_buf);

  if (res)
    grn_obj_close(&context, res);
  if (expr)
    grn_obj_close(&context, expr);
  if (cond)
    grn_obj_close(&context, cond);

  grn_obj_close(&context, database);
  grn_ctx_fin(&context);
  cut_remove_path(tmp_directory, NULL);
  g_free(path);
}

static grn_obj *properties, *docs, *terms, *size, *body, *author, *index_body;

static void
insert_document(const gchar *author_content, const gchar *body_content)
{
  uint32_t s = (uint32_t)strlen(body_content);
  grn_id docid = grn_table_add(&context, docs, NULL, 0, NULL);

  if (author_content) {
    GRN_TEXT_SET(&context, &text_buf, author_content, strlen(author_content));
    grn_test_assert(grn_obj_set_value(&context, author, docid, &text_buf,
                                      GRN_OBJ_SET));
  }

  GRN_TEXT_SET(&context, &text_buf, body_content, s);
  grn_test_assert(grn_obj_set_value(&context, body, docid, &text_buf,
                                    GRN_OBJ_SET));

  GRN_UINT32_SET(&context, &int_buf, s);
  grn_test_assert(grn_obj_set_value(&context, size, docid, &int_buf,
                                    GRN_OBJ_SET));
}

#define INSERT_DOCUMENT(author, body)            \
  cut_trace(insert_document(author, body))

static void
create_properties_table(void)
{
  const gchar *table_name = "properties";
  properties = grn_table_create(&context, table_name, strlen(table_name), NULL,
                                GRN_OBJ_TABLE_HASH_KEY|GRN_OBJ_PERSISTENT,
                                grn_ctx_at(&context, GRN_DB_SHORT_TEXT), NULL);
  cut_assert_not_null(properties);
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

  author = grn_column_create(&context, docs, "author", 6, NULL,
                             GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                             properties);
  cut_assert_not_null(author);
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

  GRN_UINT32_SET(&context, &int_buf, grn_obj_id(&context, body));
  grn_obj_set_info(&context, index_body, GRN_INFO_SOURCE, &int_buf);
}

static void
insert_data(void)
{
  INSERT_DOCUMENT("morita", "hoge");
  INSERT_DOCUMENT("morita", "fuga fuga");
  INSERT_DOCUMENT("gunyara-kun", "moge moge moge");
  INSERT_DOCUMENT(NULL, "hoge hoge");
  INSERT_DOCUMENT(NULL, "hoge fuga fuga");
  INSERT_DOCUMENT("gunyara-kun", "hoge moge moge moge");
  INSERT_DOCUMENT("yu", "moge hoge hoge");
  INSERT_DOCUMENT(NULL, "moge hoge fuga fuga");
  INSERT_DOCUMENT(NULL, "moge hoge moge moge moge");
  INSERT_DOCUMENT("morita", "poyo moge hoge moge moge moge");
}

static void
prepare_data(void)
{
  create_properties_table();
  create_documents_table();
  create_terms_table();
  insert_data();
}

#define PARSE(expr, str, flags) \
  grn_test_assert(grn_expr_parse(&context, (expr), (str), strlen(str), \
                                 body, GRN_OP_MATCH, GRN_OP_AND, flags))

void
test_equal(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "body");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 1);
  GRN_TEXT_SETS(&context, &text_buf, "poyo moge hoge moge moge moge");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(&context,
                         gcut_take_new_list_string("poyo moge hoge "
                                                   "moge moge moge",
                                                   NULL),
                         res,
                         "body");
}

void
test_equal_indexed(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "body");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_GET_VALUE, 1);
  GRN_TEXT_SETS(&context, &text_buf, "hoge");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(&context,
                         gcut_take_new_list_string("hoge", NULL),
                         res,
                         "body");
}

void
test_equal_by_existent_reference_key(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "author == \"morita\"", GRN_EXPR_SYNTAX_SCRIPT);
  res = grn_table_select(&context, docs, cond, NULL, GRN_OP_OR);
  cut_assert_not_null(res);

  grn_test_assert_select(&context,
                         gcut_take_new_list_string(
                           "fuga fuga",
                           "hoge",
                           "poyo moge hoge moge moge moge",
                           NULL),
                         res,
                         "body");
}

void
test_select(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "size");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &int_buf, 14);
  grn_expr_append_const(&context, cond, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));
  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, cond, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, res, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &int_buf, GRN_OP_OR);
  grn_expr_append_const(&context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_SELECT, 4);

  grn_expr_exec(&context, expr, 0);

  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL),
                         res,
                         "body");
}

void
test_search(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "size");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &int_buf, 14);
  grn_expr_append_const(&context, cond, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));

  v = grn_expr_add_var(&context, expr, NULL, 0);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);

  GRN_BULK_REWIND(&text_buf);
  grn_expr_append_const(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &int_buf, GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC);
  grn_expr_append_const(&context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  GRN_PTR_SET(&context, &ptr_buf, NULL);
  grn_expr_append_obj(&context, expr, &ptr_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_CREATE, 4);

  grn_expr_append_op(&context, expr, GRN_OP_ASSIGN, 2);

  grn_expr_append_obj(&context, expr, index_body, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "moge");
  grn_expr_append_const(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &int_buf, GRN_OP_OR);
  grn_expr_append_const(&context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_OBJ_SEARCH, 4);

  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, cond, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &int_buf, GRN_OP_AND);
  grn_expr_append_const(&context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_SELECT, 4);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, ".size ._score .body");
  grn_expr_append_const(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  GRN_BULK_REWIND(&text_buf);
  grn_expr_append_obj(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_JSON_PUT, 3);

  grn_expr_exec(&context, expr, 0);

  cut_assert_equal_substring("[[2],[14,4,\"moge moge moge\"],[14,2,\"moge hoge hoge\"]]",
                             GRN_TEXT_VALUE(&text_buf), GRN_TEXT_LEN(&text_buf));
}

void
test_select_search(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "size");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &int_buf, 14);
  grn_expr_append_const(&context, cond, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);
  grn_expr_compile(&context, cond);

  cut_assert_not_null((expr = grn_expr_create(&context, NULL, 0)));

  v = grn_expr_add_var(&context, expr, NULL, 0);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);

  GRN_BULK_REWIND(&text_buf);
  grn_expr_append_const(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &int_buf, GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC);
  grn_expr_append_const(&context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  GRN_PTR_SET(&context, &ptr_buf, NULL);
  grn_expr_append_obj(&context, expr, &ptr_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_CREATE, 4);

  grn_expr_append_op(&context, expr, GRN_OP_ASSIGN, 2);

  grn_expr_append_obj(&context, expr, docs, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, cond, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &int_buf, GRN_OP_OR);
  grn_expr_append_const(&context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_TABLE_SELECT, 4);

  grn_expr_append_obj(&context, expr, index_body, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "moge");
  grn_expr_append_const(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_UINT32_SET(&context, &int_buf, GRN_OP_AND);
  grn_expr_append_const(&context, expr, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_OBJ_SEARCH, 4);

  grn_expr_append_obj(&context, expr, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, ".size ._score .body");
  grn_expr_append_const(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  GRN_BULK_REWIND(&text_buf);
  grn_expr_append_obj(&context, expr, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, expr, GRN_OP_JSON_PUT, 3);

  grn_expr_exec(&context, expr, 0);

  cut_assert_equal_substring("[[2],[14,4,\"moge moge moge\"],[14,2,\"moge hoge hoge\"]]",
                             GRN_TEXT_VALUE(&text_buf), GRN_TEXT_LEN(&text_buf));
}

void
test_match(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "body");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_TEXT_SETS(&context, &text_buf, "moge");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_MATCH, 2);
  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                   "hoge moge moge moge",
                                                   "moge hoge hoge",
                                                   "moge hoge fuga fuga",
                                                   "moge hoge moge moge moge",
                                                   "poyo moge hoge "
                                                     "moge moge moge",
                                                   NULL),
                         res,
                         "body");
}

void
test_match_equal(void)
{
  grn_obj *v;

  prepare_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));

  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "body");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_TEXT_SETS(&context, &text_buf, "moge");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_MATCH, 2);

  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "size");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 2);
  GRN_UINT32_SET(&context, &int_buf, 14);
  grn_expr_append_const(&context, cond, &int_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_EQUAL, 2);

  grn_expr_append_op(&context, cond, GRN_OP_AND, 2);

  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                   "moge hoge hoge",
                                                   NULL),
                         res,
                         "body");
}

void
test_match_without_index(void)
{
  grn_obj *v;

  create_properties_table();
  create_documents_table();
  insert_data();

  cut_assert_not_null((cond = grn_expr_create(&context, NULL, 0)));
  v = grn_expr_add_var(&context, cond, NULL, 0);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));

  grn_expr_append_obj(&context, cond, v, GRN_OP_PUSH, 1);
  GRN_TEXT_SETS(&context, &text_buf, "body");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_GET_VALUE, 1);
  GRN_TEXT_SETS(&context, &text_buf, "moge");
  grn_expr_append_const(&context, cond, &text_buf, GRN_OP_PUSH, 1);
  grn_expr_append_op(&context, cond, GRN_OP_MATCH, 2);

  grn_expr_compile(&context, cond);

  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);

  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(
    &context,
    gcut_take_new_list_string("moge moge moge",
                              "hoge moge moge moge",
                              "moge hoge hoge",
                              "moge hoge fuga fuga",
                              "moge hoge moge moge moge",
                              "poyo moge hoge moge moge moge",
                              NULL),
    res,
    "body");
}
