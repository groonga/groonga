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

void data_comparison_operator(void);
void test_comparison_operator(gconstpointer data);
void data_arithmetic_operator(void);
void test_arithmetic_operator(gconstpointer data);
void data_arithmetic_operator_error(void);
void test_arithmetic_operator_error(gconstpointer data);

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "test-expr-script",
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

#define PARSE(expr,str,flags) \
  grn_test_assert(grn_expr_parse(&context, (expr), (str), strlen(str), \
                                 body, GRN_OP_MATCH, GRN_OP_AND, flags))

static grn_obj *docs, *terms, *size, *size_in_string, *body, *index_body;

static void
insert_document(const gchar *body_content)
{
  uint32_t s = (uint32_t)strlen(body_content);
  grn_id docid = grn_table_add(&context, docs, NULL, 0, NULL);
  const gchar *size_string;

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
data_comparison_operator(void)
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
test_comparison_operator(gconstpointer data)
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
  grn_test_assert_select(&context,
                         gcut_data_get_pointer(data, "expected_keys"),
                         res,
                         body);
}

#define ADD_DATUM(label, expected_keys, query)                          \
  gcut_add_datum(label,                                                 \
                 "expected_keys", G_TYPE_POINTER, expected_keys,        \
                 gcut_list_string_free,                                 \
                 "query", G_TYPE_STRING, query,                         \
                 NULL)
static void
data_arithmetic_operator_plus(void)
{
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
}

static void
data_arithmetic_operator_minus(void)
{
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
}

static void
data_arithmetic_operator_star(void)
{
  ADD_DATUM("*",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 3 * 3");
  ADD_DATUM("int * string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 3 * \"3\"");
  ADD_DATUM("float * int",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 3.1 * 3");
}

static void
data_arithmetic_operator_slash(void)
{
  ADD_DATUM("/",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 27 / 3");
  ADD_DATUM("int / string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 27 / \"3\"");
  ADD_DATUM("float / int",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 27.1 / 3");
}

static void
data_arithmetic_operator_mod(void)
{
  ADD_DATUM("%",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 19 % 10");
  ADD_DATUM("int % string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 19 % \"10\"");
  ADD_DATUM("float % int",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 19.1 % 10");
  ADD_DATUM("float % string",
            gcut_list_string_new("fuga fuga", "hoge", "hoge hoge", NULL),
            "size <= 19.1 % \"9.9\"");
}

static void
data_arithmetic_operator_incr(void)
{
  ADD_DATUM("++integer",
            gcut_list_string_new("hoge", NULL),
            "++size <= 9");
}

void
data_arithmetic_operator(void)
{
  data_arithmetic_operator_plus();
  data_arithmetic_operator_minus();
  data_arithmetic_operator_star();
  data_arithmetic_operator_slash();
  data_arithmetic_operator_mod();
  data_arithmetic_operator_incr();
}
#undef ADD_DATUM

void
test_arithmetic_operator(gconstpointer data)
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
  grn_test_assert_select(&context,
                         gcut_data_get_pointer(data, "expected_keys"),
                         res,
                         body);
}

void
data_arithmetic_operator_error(void)
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

  ADD_DATUM("++string",
            GRN_INVALID_ARGUMENT,
            cut_take_printf("invalid increment target type: %d "
                            "(FIXME: type name is needed)",
                            GRN_DB_TEXT),
            "++size_in_string <= 9");

#undef ADD_DATUM
}

void
test_arithmetic_operator_error(gconstpointer data)
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
