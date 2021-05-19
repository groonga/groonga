/*
  Copyright (C) 2011-2017  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "../../../config.h"

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_is_builtin(void);
void test_is_builtin(gconstpointer data);
void test_is_bulk(void);
void test_is_text_family_bulk(void);
void data_is_table(void);
void test_is_table(gconstpointer data);
void data_is_column(void);
void test_is_column(gconstpointer data);
void data_is_scalar_column(void);
void test_is_scalar_column(gconstpointer data);
void data_is_vector_column(void);
void test_is_vector_column(gconstpointer data);
void data_is_weight_vector_column(void);
void test_is_weight_vector_column(gconstpointer data);
void data_is_reference_column(void);
void test_is_reference_column(gconstpointer data);
void data_is_data_column(void);
void test_is_data_column(gconstpointer data);
void data_is_index_column(void);
void test_is_index_column(gconstpointer data);
void data_is_accessor(void);
void test_is_accessor(gconstpointer data);
void data_is_key_accessor(void);
void test_is_key_accessor(gconstpointer data);
void data_is_type(void);
void test_is_type(gconstpointer data);
void data_is_text_family_type(void);
void test_is_text_family_type(gconstpointer data);
void data_is_proc(void);
void test_is_proc(gconstpointer data);
void data_is_tokenizer_proc(void);
void test_is_tokenizer_proc(gconstpointer data);
void data_is_function_proc(void);
void test_is_function_proc(gconstpointer data);
void data_is_selector_proc(void);
void test_is_selector_proc(gconstpointer data);
void data_is_normalizer_proc(void);
void test_is_normalizer_proc(gconstpointer data);
void data_is_token_filter_proc(void);
void test_is_token_filter_proc(gconstpointer data);
void data_is_scorer_proc(void);
void test_is_scorer_proc(gconstpointer data);
void data_is_window_function_proc(void);
void test_is_window_function_proc(gconstpointer data);
void test_is_expr(void);
void data_type_to_string(void);
void test_type_to_string(gconstpointer data);
void data_name_is_column(void);
void test_name_is_column(gconstpointer data);

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "object",
                                   NULL);
}

void
cut_shutdown(void)
{
  g_free(tmp_directory);
}

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

void
cut_setup(void)
{
  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);
}

void
cut_teardown(void)
{
  if (context) {
    grn_obj_close(context, database);
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
data_is_builtin(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ? "built-in - " name : "custom - " name),    \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "TokenBigram");
#ifdef GRN_WITH_MECAB
  ADD_DATUM(TRUE, "TokenMecab");
#endif
  ADD_DATUM(FALSE, "Users");
  ADD_DATUM(FALSE, "Users.name");
  ADD_DATUM(FALSE, "suggest");

#undef ADD_DATUM
}

void
test_is_builtin(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("register suggest/suggest");
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_builtin(context, object));
  } else {
    cut_assert_false(grn_obj_is_builtin(context, object));
  }
}

void
test_is_bulk(void)
{
  grn_obj bulk;
  grn_obj *tokenizer;

  GRN_UINT32_INIT(&bulk, 0);
  cut_assert_true(grn_obj_is_bulk(context, &bulk));
  GRN_OBJ_FIN(context, &bulk);

  tokenizer = grn_ctx_get(context, "TokenBigram", -1);
  cut_assert_false(grn_obj_is_bulk(context, tokenizer));
}

void
test_is_text_family_bulk(void)
{
  grn_obj uint32_bulk;
  grn_obj short_text_bulk;
  grn_obj text_bulk;
  grn_obj long_text_bulk;

  GRN_UINT32_INIT(&uint32_bulk, 0);
  GRN_SHORT_TEXT_INIT(&short_text_bulk, 0);
  GRN_TEXT_INIT(&text_bulk, 0);
  GRN_LONG_TEXT_INIT(&long_text_bulk, 0);

  cut_assert_false(grn_obj_is_text_family_bulk(context, &uint32_bulk));
  cut_assert_true(grn_obj_is_text_family_bulk(context, &short_text_bulk));
  cut_assert_true(grn_obj_is_text_family_bulk(context, &text_bulk));
  cut_assert_true(grn_obj_is_text_family_bulk(context, &long_text_bulk));

  GRN_OBJ_FIN(context, &uint32_bulk);
  GRN_OBJ_FIN(context, &short_text_bulk);
  GRN_OBJ_FIN(context, &text_bulk);
  GRN_OBJ_FIN(context, &long_text_bulk);
}

void
data_is_table(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ? "table - " name : "column - " name),       \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "Users");
  ADD_DATUM(FALSE, "Users.name");

#undef ADD_DATUM
}

void
test_is_table(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_table(context, object));
  } else {
    cut_assert_false(grn_obj_is_table(context, object));
  }
}

void
data_is_column(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("table",           FALSE, "Users");
  ADD_DATUM("fix size column", TRUE,  "Users.age");
  ADD_DATUM("var size column", TRUE,  "Users.name");
  ADD_DATUM("index column",    TRUE,  "Names.users");

#undef ADD_DATUM
}

void
test_is_column(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users age COLUMN_SCALAR UInt8");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("table_create Names TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Names users COLUMN_INDEX Users name");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_column(context, object));
  } else {
    cut_assert_false(grn_obj_is_column(context, object));
  }
}

void
data_is_scalar_column(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("table",                  FALSE, "Users");
  ADD_DATUM("fix size scalar column",  TRUE, "Users.age");
  ADD_DATUM("var size scalar column",  TRUE, "Users.name");
  ADD_DATUM("vector column",          FALSE, "Users.tags");
  ADD_DATUM("index column",           FALSE, "Names.users");

#undef ADD_DATUM
}

void
test_is_scalar_column(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users age COLUMN_SCALAR UInt8");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("column_create Users tags COLUMN_VECTOR ShortText");
  assert_send_command("table_create Names TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Names users COLUMN_INDEX Users name");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_scalar_column(context, object));
  } else {
    cut_assert_false(grn_obj_is_scalar_column(context, object));
  }
}

void
data_is_vector_column(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("table",                  FALSE, "Users");
  ADD_DATUM("fix size scalar column", FALSE, "Users.age");
  ADD_DATUM("var size scalar column", FALSE, "Users.name");
  ADD_DATUM("vector column",          TRUE,  "Users.tags");
  ADD_DATUM("index column",           FALSE, "Names.users");

#undef ADD_DATUM
}

void
test_is_vector_column(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users age COLUMN_SCALAR UInt8");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");
  assert_send_command("column_create Users tags COLUMN_VECTOR ShortText");
  assert_send_command("table_create Names TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Names users COLUMN_INDEX Users name");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_vector_column(context, object));
  } else {
    cut_assert_false(grn_obj_is_vector_column(context, object));
  }
}

void
data_is_weight_vector_column(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("vector column",        FALSE, "Users.tags");
  ADD_DATUM("weight vector column", TRUE,  "Users.weight_tags");

#undef ADD_DATUM
}

void
test_is_weight_vector_column(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users tags COLUMN_VECTOR ShortText");
  assert_send_command("column_create Users weight_tags "
                      "COLUMN_VECTOR|WITH_WEIGHT ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_weight_vector_column(context, object));
  } else {
    cut_assert_false(grn_obj_is_weight_vector_column(context, object));
  }
}

void
data_is_reference_column(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("table",            FALSE, "Users");
  ADD_DATUM("value column",     FALSE, "Users.age");
  ADD_DATUM("reference column", TRUE,  "Users.name");

#undef ADD_DATUM
}

void
test_is_reference_column(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Names TABLE_PAT_KEY ShortText");
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users age COLUMN_SCALAR UInt8");
  assert_send_command("column_create Users name COLUMN_SCALAR Names");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_reference_column(context, object));
  } else {
    cut_assert_false(grn_obj_is_reference_column(context, object));
  }
}

void
data_is_data_column(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("table",         FALSE, "Users");
  ADD_DATUM("scalar column",  TRUE, "Users.age");
  ADD_DATUM("vector column",  TRUE, "Users.tags");
  ADD_DATUM("index column",  FALSE, "Ages.users_age");

#undef ADD_DATUM
}

void
test_is_data_column(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users age COLUMN_SCALAR UInt8");
  assert_send_command("column_create Users tags COLUMN_VECTOR ShortText");
  assert_send_command("table_create Ages TABLE_PAT_KEY UInt8");
  assert_send_command("column_create Ages users_age COLUMN_INDEX Users age");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_data_column(context, object));
  } else {
    cut_assert_false(grn_obj_is_data_column(context, object));
  }
}

void
data_is_index_column(void)
{
#define ADD_DATUM(label, expected, name)                                \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM("table",        FALSE, "Users");
  ADD_DATUM("value column", FALSE, "Users.age");
  ADD_DATUM("index column", TRUE,  "Ages.users_age");

#undef ADD_DATUM
}

void
test_is_index_column(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users age COLUMN_SCALAR UInt8");
  assert_send_command("table_create Ages TABLE_PAT_KEY UInt8");
  assert_send_command("column_create Ages users_age COLUMN_INDEX Users age");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_index_column(context, object));
  } else {
    cut_assert_false(grn_obj_is_index_column(context, object));
  }
}

void
data_is_accessor(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "accessor - " name :                                  \
                  "not accessor - " name),                              \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "_key");
  ADD_DATUM(FALSE, "name");

#undef ADD_DATUM
}

void
test_is_accessor(gconstpointer data)
{
  const gchar *name;
  grn_obj *table;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR ShortText");

  table = grn_ctx_get(context, "Users", -1);
  name = gcut_data_get_string(data, "name");
  object = grn_obj_column(context, table, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_accessor(context, object));
  } else {
    cut_assert_false(grn_obj_is_accessor(context, object));
  }
}

void
data_is_key_accessor(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "key accessor - " name :                              \
                  "not key accessor - " name),                          \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "_key");
  ADD_DATUM(FALSE, "name._key");

#undef ADD_DATUM
}

void
test_is_key_accessor(gconstpointer data)
{
  const gchar *name;
  grn_obj *table;
  grn_obj *object;

  assert_send_command("table_create Names TABLE_HASH_KEY ShortText");
  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Users name COLUMN_SCALAR Names");

  table = grn_ctx_get(context, "Users", -1);
  name = gcut_data_get_string(data, "name");
  object = grn_obj_column(context, table, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_key_accessor(context, object));
  } else {
    cut_assert_false(grn_obj_is_key_accessor(context, object));
  }
}

void
data_is_type(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "type - " name :                                      \
                  "not type - " name),                                  \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "ShortText");
  ADD_DATUM(FALSE, "Users");

#undef ADD_DATUM
}

void
test_is_type(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_type(context, object));
  } else {
    cut_assert_false(grn_obj_is_type(context, object));
  }
}

void
data_is_text_family_type(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "type - " name :                                      \
                  "not type - " name),                                  \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(FALSE, "Time");
  ADD_DATUM(TRUE, "ShortText");
  ADD_DATUM(TRUE, "Text");
  ADD_DATUM(TRUE, "LongText");
  ADD_DATUM(FALSE, "TokyoGeoPoint");

#undef ADD_DATUM
}

void
test_is_text_family_type(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_text_family_type(context, object));
  } else {
    cut_assert_false(grn_obj_is_text_family_type(context, object));
  }
}

void
data_is_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "proc - " name :                                      \
                  "not proc - " name),                                  \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "status");
  ADD_DATUM(FALSE, "Users");

#undef ADD_DATUM
}

void
test_is_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("table_create Users TABLE_HASH_KEY ShortText");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_proc(context, object));
  }
}

void
data_is_tokenizer_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "tokenizer-proc - " name :                            \
                  "not tokenizer-proc - " name),                        \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "TokenBigram");
  ADD_DATUM(FALSE, "status");

#undef ADD_DATUM
}

void
test_is_tokenizer_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_tokenizer_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_tokenizer_proc(context, object));
  }
}

void
data_is_function_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "function-proc - " name :                             \
                  "not function-proc - " name),                         \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "now");
  ADD_DATUM(FALSE, "status");

#undef ADD_DATUM
}

void
test_is_function_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_function_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_function_proc(context, object));
  }
}

void
data_is_selector_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "selector-proc - " name :                             \
                  "not selector-proc - " name),                         \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "geo_in_circle");
  ADD_DATUM(FALSE, "now");

#undef ADD_DATUM
}

void
test_is_selector_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_selector_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_selector_proc(context, object));
  }
}

void
data_is_normalizer_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "normalizer-proc - " name :                           \
                  "not normalizer-proc - " name),                       \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "NormalizerAuto");
  ADD_DATUM(FALSE, "TokenBigram");

#undef ADD_DATUM
}

void
test_is_normalizer_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_normalizer_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_normalizer_proc(context, object));
  }
}

void
data_is_token_filter_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "token-filter-proc - " name :                         \
                  "not token-filter-proc - " name),                     \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "TokenFilterStopWord");
  ADD_DATUM(FALSE, "TokenBigram");

#undef ADD_DATUM
}

void
test_is_token_filter_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  assert_send_command("plugin_register token_filters/stop_word");

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_token_filter_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_token_filter_proc(context, object));
  }
}

void
data_is_scorer_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "scorer-proc - " name :                               \
                  "not scorer-proc - " name),                           \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "scorer_tf_idf");
  ADD_DATUM(FALSE, "geo_in_circle");

#undef ADD_DATUM
}

void
test_is_scorer_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_scorer_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_scorer_proc(context, object));
  }
}

void
data_is_window_function_proc(void)
{
#define ADD_DATUM(expected, name)                                       \
  gcut_add_datum((expected ?                                            \
                  "window-function-proc - " name :                      \
                  "not window-function-proc - " name),                  \
                 "expected", G_TYPE_BOOLEAN, expected,                  \
                 "name", G_TYPE_STRING, name,                           \
                 NULL)

  ADD_DATUM(TRUE, "record_number");
  ADD_DATUM(FALSE, "geo_in_circle");

#undef ADD_DATUM
}

void
test_is_window_function_proc(gconstpointer data)
{
  const gchar *name;
  grn_obj *object;

  name = gcut_data_get_string(data, "name");
  object = grn_ctx_get(context, name, strlen(name));
  if (gcut_data_get_boolean(data, "expected")) {
    cut_assert_true(grn_obj_is_window_function_proc(context, object));
  } else {
    cut_assert_false(grn_obj_is_window_function_proc(context, object));
  }
}

void
test_is_expr(void)
{
  grn_obj *expr;

  cut_assert_false(grn_obj_is_expr(context, grn_ctx_get(context, "select", -1)));
  expr = grn_expr_create(context, NULL, 0);
  cut_assert_true(grn_obj_is_expr(context, expr));
  grn_obj_close(context, expr);
}

void
data_type_to_string(void)
{
#define ADD_DATUM(expected, type)                                      \
  gcut_add_datum(G_STRINGIFY(type),                                    \
                 "expected", G_TYPE_STRING, expected,                  \
                 "type", G_TYPE_UINT, type,                            \
                 NULL)

  ADD_DATUM("void", GRN_VOID);
  ADD_DATUM("bulk", GRN_BULK);
  ADD_DATUM("ptr", GRN_PTR);
  ADD_DATUM("uvector", GRN_UVECTOR);
  ADD_DATUM("pvector", GRN_PVECTOR);
  ADD_DATUM("vector", GRN_VECTOR);
  ADD_DATUM("msg", GRN_MSG);
  ADD_DATUM("query", GRN_QUERY);
  ADD_DATUM("accessor", GRN_ACCESSOR);
  ADD_DATUM("snip", GRN_SNIP);
  ADD_DATUM("patsnip", GRN_PATSNIP);
  ADD_DATUM("string", GRN_STRING);
  ADD_DATUM("cursor:table:hash_key", GRN_CURSOR_TABLE_HASH_KEY);
  ADD_DATUM("cursor:table:pat_key", GRN_CURSOR_TABLE_PAT_KEY);
  ADD_DATUM("cursor:table:dat_key", GRN_CURSOR_TABLE_DAT_KEY);
  ADD_DATUM("cursor:table:no_key", GRN_CURSOR_TABLE_NO_KEY);
  ADD_DATUM("cursor:column:index", GRN_CURSOR_COLUMN_INDEX);
  ADD_DATUM("cursor:column:geo_index", GRN_CURSOR_COLUMN_GEO_INDEX);
  ADD_DATUM("cursor:config", GRN_CURSOR_CONFIG);
  ADD_DATUM("type", GRN_TYPE);
  ADD_DATUM("proc", GRN_PROC);
  ADD_DATUM("expr", GRN_EXPR);
  ADD_DATUM("table:hash_key", GRN_TABLE_HASH_KEY);
  ADD_DATUM("table:pat_key", GRN_TABLE_PAT_KEY);
  ADD_DATUM("table:dat_key", GRN_TABLE_DAT_KEY);
  ADD_DATUM("table:no_key", GRN_TABLE_NO_KEY);
  ADD_DATUM("db", GRN_DB);
  ADD_DATUM("column:fix_size", GRN_COLUMN_FIX_SIZE);
  ADD_DATUM("column:var_size", GRN_COLUMN_VAR_SIZE);
  ADD_DATUM("column:index", GRN_COLUMN_INDEX);

#undef ADD_DATUM
}

void
test_type_to_string(gconstpointer data)
{
  const gchar *expected;
  guint type;

  expected = gcut_data_get_string(data, "expected");
  type = gcut_data_get_uint(data, "type");
  cut_assert_equal_string(expected,
                          grn_obj_type_to_string(type));
}

void
data_name_is_column(void)
{
#define ADD_DATUM(expected, name)                                      \
  gcut_add_datum(G_STRINGIFY(name),                                    \
                 "expected", G_TYPE_BOOLEAN, expected,                 \
                 "name", G_TYPE_STRING, name,                          \
                 NULL)

  ADD_DATUM(TRUE, "Users.age");
  ADD_DATUM(FALSE, "Users");

#undef ADD_DATUM
}

void
test_name_is_column(gconstpointer data)
{
  gboolean expected;
  const gchar *name;

  expected = gcut_data_get_boolean(data, "expected");
  name = gcut_data_get_string(data, "name");
  if (expected) {
    cut_assert_true(grn_obj_name_is_column(context, name, -1));
  } else {
    cut_assert_false(grn_obj_name_is_column(context, name, -1));
  }
}
