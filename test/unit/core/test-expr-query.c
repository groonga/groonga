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
#include <time.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

static gchar *tmp_directory;
static gchar *path;
static grn_ctx context;
static grn_obj *database;
static grn_obj *res, *expr;
static grn_obj textbuf, intbuf, floatbuf, timebuf;

void data_allow_column(void);
void test_allow_column(gconstpointer data);
void data_allow_update(void);
void test_allow_update(gconstpointer data);

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "test-expr-query",
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

  expr = NULL;
  res = NULL;

  GRN_TEXT_INIT(&textbuf, 0);
  GRN_UINT32_INIT(&intbuf, 0);
  GRN_FLOAT_INIT(&floatbuf, 0);
  GRN_TIME_INIT(&timebuf, 0);
}

void
cut_teardown(void)
{
  grn_obj_close(&context, &textbuf);
  grn_obj_close(&context, &intbuf);
  grn_obj_close(&context, &floatbuf);
  grn_obj_close(&context, &timebuf);

  if (res)
    grn_obj_close(&context, res);
  if (expr)
    grn_obj_close(&context, expr);

  grn_db_close(&context, database);
  grn_ctx_fin(&context);
  cut_remove_path(tmp_directory, NULL);
  g_free(path);
}

#define PARSE(expr, str, flags) \
  grn_test_assert(grn_expr_parse(&context, (expr), (str), strlen(str), \
                                 body, GRN_OP_MATCH, GRN_OP_AND, flags))

static grn_obj *docs, *terms, *body, *created_at, *index_body;
static grn_obj *size, *size_in_string, *size_in_float;

static void
insert_document(const gchar *body_content)
{
  uint32_t s = (uint32_t)strlen(body_content);
  grn_id docid = grn_table_add(&context, docs, NULL, 0, NULL);
  const gchar *size_string;
  struct tm time;

  GRN_TEXT_SET(&context, &textbuf, body_content, s);
  grn_test_assert(grn_obj_set_value(&context, body, docid, &textbuf,
                                    GRN_OBJ_SET));

  GRN_UINT32_SET(&context, &intbuf, s);
  grn_test_assert(grn_obj_set_value(&context, size, docid, &intbuf,
                                    GRN_OBJ_SET));

  size_string = cut_take_printf("%u", s);
  GRN_TEXT_SET(&context, &textbuf, size_string, strlen(size_string));
  grn_test_assert(grn_obj_set_value(&context, size_in_string, docid, &textbuf,
                                    GRN_OBJ_SET));

  GRN_FLOAT_SET(&context, &floatbuf, s);
  grn_test_assert(grn_obj_set_value(&context, size_in_float, docid, &floatbuf,
                                    GRN_OBJ_SET));

  time.tm_sec = s;
  time.tm_min = 16;
  time.tm_hour = 15;
  time.tm_mday = 2;
  time.tm_mon = 11;
  time.tm_year = 109;
  time.tm_wday = 3;
  time.tm_yday = 336;
  time.tm_isdst = 0;
  GRN_TIME_SET(&context, &timebuf, GRN_TIME_PACK(mktime(&time), 0));
  grn_test_assert(grn_obj_set_value(&context, created_at, docid, &timebuf,
                                    GRN_OBJ_SET));
}

#define INSERT_DOCUMENT(body) \
  cut_trace(insert_document(body))

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

  size_in_string = grn_column_create(&context, docs, "size_in_string", 14, NULL,
                                     GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                                     grn_ctx_at(&context, GRN_DB_TEXT));
  cut_assert_not_null(size_in_string);

  size_in_float = grn_column_create(&context, docs, "size_in_float", 13, NULL,
                                     GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                                     grn_ctx_at(&context, GRN_DB_FLOAT));
  cut_assert_not_null(size_in_float);

  created_at = grn_column_create(&context, docs, "created_at", 10, NULL,
                                 GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
                                 grn_ctx_at(&context, GRN_DB_TIME));
  cut_assert_not_null(created_at);

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
  INSERT_DOCUMENT("=poyo_moge_hoge_moge_moge_moge");
}

static void
prepare_data(void)
{
  create_documents_table();
  create_terms_table();
  insert_data();
}

void
data_allow_column(void)
{
#define ADD_DATUM(label, expected_keys, query)                          \
  gcut_add_datum(label,                                                 \
                 "expected_keys", G_TYPE_POINTER, expected_keys,        \
                 gcut_list_string_free,                                 \
                 "query", G_TYPE_STRING, query,                         \
                 NULL)

  ADD_DATUM("empty",
            gcut_list_string_new("fuga fuga", "hoge hoge", NULL),
            "size:9");
  ADD_DATUM("= - without ALLOW_UPDATE",
            gcut_list_string_new("=poyo_moge_hoge_moge_moge_moge", NULL),
            "body:=poyo_moge_hoge_moge_moge_moge");

#undef ADD_DATUM
}

void
test_allow_column(gconstpointer data)
{
  grn_obj *v;

  prepare_data();

  expr = grn_expr_create(&context, NULL, 0);
  cut_assert_not_null(expr);
  v = grn_expr_add_var(&context, expr, NULL, 0);
  cut_assert_not_null(v);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  PARSE(expr,
        gcut_data_get_string(data, "query"),
        GRN_EXPR_SYNTAX_QUERY | GRN_EXPR_ALLOW_COLUMN);

  res = grn_table_select(&context, docs, expr, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select(&context,
                         gcut_data_get_pointer(data, "expected_keys"),
                         res,
                         "body");
}

void
data_allow_update(void)
{
#define ADD_DATUM(label, expected_keys, query)                          \
  gcut_add_datum(label,                                                 \
                 "expected_keys", G_TYPE_POINTER, expected_keys,        \
                 gcut_list_string_free,                                 \
                 "query", G_TYPE_STRING, query,                         \
                 NULL)

  ADD_DATUM("=",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size:<=9 size:=9 size:9");

#undef ADD_DATUM
}

void
test_allow_update(gconstpointer data)
{
  grn_obj *v;

  prepare_data();

  expr = grn_expr_create(&context, NULL, 0);
  cut_assert_not_null(expr);
  v = grn_expr_add_var(&context, expr, NULL, 0);
  cut_assert_not_null(v);
  GRN_RECORD_INIT(v, 0, grn_obj_id(&context, docs));
  PARSE(expr,
        gcut_data_get_string(data, "query"),
        GRN_EXPR_SYNTAX_QUERY | GRN_EXPR_ALLOW_COLUMN | GRN_EXPR_ALLOW_UPDATE);

  res = grn_table_select(&context, docs, expr, NULL, GRN_OP_OR);
  cut_assert_not_null(res);
  grn_test_assert_select(&context,
                         gcut_data_get_pointer(data, "expected_keys"),
                         res,
                         "body");
}
