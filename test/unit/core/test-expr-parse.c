/* Copyright(C) 2009-2014 Brazil

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
static grn_obj textbuf, intbuf;

void data_parse(void);
void test_parse(gconstpointer data);
void test_set_value(void);
void test_set_value_with_implicit_variable_reference(void);
void test_set_value_with_query(void);
void test_proc_call(void);
void test_score_set(void);
void test_key_equal(void);
void test_value_access(void);
void test_snip(void);
void test_snip_without_tags(void);
void test_float_literal(void);
void test_int32_literal(void);
void test_lager_than_int32_literal(void);
void test_int64_literal(void);
void test_long_integer_literal(void);
void test_syntax_equal_string_reference_key(void);

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "test-expr-parse",
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
  path = g_build_filename(tmp_directory, "text-expr-parse", NULL);
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

static grn_obj *properties, *docs, *terms, *size, *body, *author, *index_body;

static void
insert_document(const gchar *author_content, const gchar *body_content)
{
  uint32_t s = (uint32_t)strlen(body_content);
  grn_id docid = grn_table_add(&context, docs, NULL, 0, NULL);

  if (author_content) {
    GRN_TEXT_SET(&context, &textbuf, author_content, strlen(author_content));
    grn_test_assert(grn_obj_set_value(&context, author, docid, &textbuf,
                                      GRN_OBJ_SET));
  }

  GRN_TEXT_SET(&context, &textbuf, body_content, s);
  grn_test_assert(grn_obj_set_value(&context, body, docid, &textbuf,
                                    GRN_OBJ_SET));

  GRN_UINT32_SET(&context, &intbuf, s);
  grn_test_assert(grn_obj_set_value(&context, size, docid, &intbuf,
                                    GRN_OBJ_SET));
}

#define INSERT_DOCUMENT(author, body)            \
  cut_trace(insert_document(author, body))

static void grn_test_assert_select_all(grn_obj *result);
static void grn_test_assert_select_none(grn_obj *result);

static void
grn_test_assert_select_all(grn_obj *result)
{
  grn_test_assert_select(&context,
                         gcut_take_new_list_string("hoge",
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
                                                   NULL),
                         result,
                         "body");
}

static void
grn_test_assert_select_none(grn_obj *result)
{
  cut_assert_equal_uint(0, grn_table_size(&context, result));
}

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

  GRN_UINT32_SET(&context, &intbuf, grn_obj_id(&context, body));
  grn_obj_set_info(&context, index_body, GRN_INFO_SOURCE, &intbuf);
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

#define PARSE(expr,str,flags) \
  grn_test_assert(grn_expr_parse(&context, (expr), (str), strlen(str), \
                                 body, GRN_OP_MATCH, GRN_OP_AND, flags))

void
data_parse(void)
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
            "body:@hoge + body:@moge",
            GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA|GRN_EXPR_ALLOW_COLUMN,
            "body:@poyo", GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_COLUMN,
            "size:14", GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_COLUMN);
  ADD_DATUM("expression parse level",
            "body@\"hoge\" && body@\"moge\"",
            GRN_EXPR_SYNTAX_SCRIPT|GRN_EXPR_ALLOW_UPDATE,
            "body@\"poyo\"", GRN_EXPR_SYNTAX_SCRIPT,
            "size == 14", GRN_EXPR_SYNTAX_SCRIPT);
#undef ADD_DATUM
}

void
test_parse(gconstpointer data)
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
  grn_test_assert_expr(
    &context,
    "#<expr\n"
    "  vars:{\n"
    "    $1:#<record:no_key:docs id:(no value)>\n"
    "  },\n"
    "  codes:{\n"
    "    0:<get_value n_args:1, flags:0, modify:2, "
    "value:#<column:var_size docs.body range:Text type:scalar compress:none>>,\n"
    "    1:<push n_args:1, flags:0, modify:0, value:\"hoge\">,\n"
    "    2:<match n_args:2, flags:1, modify:4, value:(NULL)>,\n"
    "    3:<get_value n_args:1, flags:0, modify:2, "
    "value:#<column:var_size docs.body range:Text type:scalar compress:none>>,\n"
    "    4:<push n_args:1, flags:0, modify:0, value:\"moge\">,\n"
    "    5:<match n_args:2, flags:1, modify:0, value:(NULL)>,\n"
    "    6:<and n_args:2, flags:1, modify:4, value:(NULL)>,\n"
    "    7:<get_value n_args:1, flags:0, modify:2, "
    "value:#<column:var_size docs.body range:Text type:scalar compress:none>>,\n"
    "    8:<push n_args:1, flags:0, modify:0, value:\"poyo\">,\n"
    "    9:<match n_args:2, flags:1, modify:0, value:(NULL)>,\n"
    "    10:<and n_args:2, flags:0, modify:0, value:(NULL)>\n"
    "  }\n"
    ">",
    cond);
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
  grn_test_assert_expr(
    &context,
    "#<expr\n"
    "  vars:{\n"
    "    $1:#<record:no_key:docs id:(no value)>\n"
    "  },\n"
    "  codes:{\n"
    "    0:<get_value n_args:1, flags:0, modify:2, "
    "value:#<column:fix_size docs.size range:UInt32 type:scalar compress:none>>,\n"
    "    1:<push n_args:1, flags:0, modify:0, value:14>,\n"
    "    2:<equal n_args:2, flags:0, modify:0, value:(NULL)>\n"
    "  }\n"
    ">",
    cond);
  res = grn_table_create(&context, NULL, 0, NULL,
                         GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, docs, NULL);
  cut_assert_not_null(res);
  cut_assert_not_null(grn_table_select(&context, docs, cond, res, GRN_OP_OR));

  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL),
                         res,
                         "body");
}

void
test_set_value(void)
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
  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL),
                         res,
                         "body");
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
test_set_value_with_implicit_variable_reference(void)
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
  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                        "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL),
                         res,
                         "body");
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
test_set_value_with_query(void)
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
  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "moge hoge hoge",
                                                   NULL),
                         res,
                         "body");
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
test_proc_call(void)
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
  grn_test_assert_select(&context,
                         gcut_take_new_list_string("hoge moge moge moge",
                                                   "moge hoge fuga fuga",
                                                   "moge hoge moge moge moge",
                                                   "poyo moge hoge "
                                                     "moge moge moge",
                                                   NULL),
                         res,
                         "body");
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
test_score_set(void)
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
  grn_test_assert_select(&context,
                         gcut_take_new_list_string("moge moge moge",
                                                   "hoge fuga fuga",
                                                   "hoge moge moge moge",
                                                   "moge hoge hoge",
                                                   "moge hoge fuga fuga",
                                                   "moge hoge moge moge moge",
                                                   "poyo moge hoge "
                                                   "moge moge moge",
                                                   NULL),
                         res2,
                         "body");
  grn_test_assert(grn_obj_close(&context, res2));
}

void
test_key_equal(void)
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
test_value_access(void)
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
test_snip(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);
  cut_assert_not_null(expr);

  PARSE(expr, "search engine column",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA);

  {
    grn_obj *snip;
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

    grn_test_assert(grn_obj_close(&context, snip));
  }
}

void
test_snip_without_tags(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, expr, v);
  cut_assert_not_null(expr);

  PARSE(expr, "search engine column",
        GRN_EXPR_SYNTAX_QUERY|GRN_EXPR_ALLOW_PRAGMA);

  {
    grn_obj *snip;
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

    grn_test_assert(grn_obj_close(&context, snip));
  }
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
                                 NULL, GRN_OP_NOP, GRN_OP_NOP,
                                 GRN_EXPR_SYNTAX_SCRIPT |
                                 GRN_EXPR_ALLOW_UPDATE));

  grn_test_assert(grn_expr_compile(&context, expr));
  grn_expr_exec(&context, expr, 0);
  grn_test_assert(context.rc);

  return var;
}

void
test_float_literal(void)
{
  grn_obj *var;
  const char *str_expr = "var = 3.14159265";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_FLOAT, GRN_OBJ_GET_DOMAIN(var));
  cut_assert_equal_double(3.14159265, 0.00000001, GRN_FLOAT_VALUE(var));
}

void
test_int32_literal(void)
{
  grn_obj *var;
  const char *str_expr = "var = 123456";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_INT32, GRN_OBJ_GET_DOMAIN(var));
  cut_assert_equal_int(123456, GRN_INT32_VALUE(var));
}

void
test_lager_than_int32_literal(void)
{
  grn_obj *var;
  const char *str_expr = "var = 3456789012";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_INT64, GRN_OBJ_GET_DOMAIN(var));
  cut_assert_equal_int(G_GINT64_CONSTANT(3456789012), GRN_INT64_VALUE(var));
}

void
test_int64_literal(void)
{
  grn_obj *var;
  const char *str_expr = "var = 123456789012";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_INT64, GRN_OBJ_GET_DOMAIN(var));
  cut_assert_equal_int_least64(G_GINT64_CONSTANT(123456789012),
                               GRN_INT64_VALUE(var));
}

void
test_long_integer_literal(void)
{
  grn_obj *var;
  const char *str_expr = "var = 12345678901234567890";

  var = parse_numeric_literal(str_expr);
  cut_assert_equal_int(GRN_DB_FLOAT, GRN_OBJ_GET_DOMAIN(var));
  /* IEEE 754 says "double" can presisely hold about 16 digits.
     To be safe, assume only 15 digits are preserved. */
  cut_assert_equal_double(12345678901234567890., 100000., GRN_FLOAT_VALUE(var));
}

void
test_syntax_equal_string_reference_key(void)
{
  grn_obj *v;

  prepare_data();

  GRN_EXPR_CREATE_FOR_QUERY(&context, docs, cond, v);
  cut_assert_not_null(cond);
  cut_assert_not_null(v);
  PARSE(cond, "author == \"morita\"", GRN_EXPR_SYNTAX_SCRIPT);
  grn_test_assert_expr(
    &context,
    "#<expr\n"
    "  vars:{\n"
    "    $1:#<record:no_key:docs id:(no value)>\n"
    "  },\n"
    "  codes:{\n"
    "    0:<get_value n_args:1, flags:0, modify:2, "
    "value:#<column:fix_size docs.author range:properties type:scalar compress:none>>,\n"
    "    1:<push n_args:1, flags:0, modify:0, "
    "value:#<record:hash:properties id:1 key:\"morita\">>,\n"
    "    2:<equal n_args:2, flags:0, modify:0, value:(NULL)>\n"
    "  }\n"
    ">",
    cond);
}
