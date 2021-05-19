/*
  Copyright (C) 2015  Kouhei Sutou <kou@clear-code.com>

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

#define get(name) grn_ctx_get(context, name, strlen(name))

void data_exec_equal_true(void);
void test_exec_equal_true(gconstpointer data);
void data_exec_equal_false(void);
void test_exec_equal_false(gconstpointer data);
void data_exec_not_equal_true(void);
void test_exec_not_equal_true(gconstpointer data);
void data_exec_not_equal_false(void);
void test_exec_not_equal_false(gconstpointer data);
void data_exec_less_true(void);
void test_exec_less_true(gconstpointer data);
void data_exec_less_false(void);
void test_exec_less_false(gconstpointer data);
void data_exec_greater_true(void);
void test_exec_greater_true(gconstpointer data);
void data_exec_greater_false(void);
void test_exec_greater_false(gconstpointer data);
void data_exec_less_equal_true(void);
void test_exec_less_equal_true(gconstpointer data);
void data_exec_less_equal_false(void);
void test_exec_less_equal_false(gconstpointer data);
void data_exec_greater_equal_true(void);
void test_exec_greater_equal_true(gconstpointer data);
void data_exec_greater_equal_false(void);
void test_exec_greater_equal_false(gconstpointer data);
void data_exec_match_true(void);
void test_exec_match_true(gconstpointer data);
void data_exec_match_false(void);
void test_exec_match_false(gconstpointer data);
void data_exec_prefix_true(void);
void test_exec_prefix_true(gconstpointer data);
void data_exec_prefix_false(void);
void test_exec_prefix_false(gconstpointer data);
#ifdef GRN_WITH_ONIGMO
void data_exec_regexp_true(void);
void test_exec_regexp_true(gconstpointer data);
void data_exec_regexp_false(void);
void test_exec_regexp_false(gconstpointer data);
#endif

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

static grn_obj lhs;
static grn_obj rhs;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "operator",
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
  const gchar *database_path;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);

  GRN_VOID_INIT(&lhs);
  GRN_VOID_INIT(&rhs);
}

void
cut_teardown(void)
{
  GRN_OBJ_FIN(context, &lhs);
  GRN_OBJ_FIN(context, &rhs);

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

static void
set_text(grn_obj *bulk, const gchar *value)
{
  grn_obj_reinit(context, bulk, GRN_DB_TEXT, 0);
  GRN_TEXT_SETS(context, bulk, value);
}

static void
set_one(grn_obj *value, const gchar *type)
{
  if (strcmp(type, "text") == 0) {
    set_text(value, "1");
  } else if (strcmp(type, "int32") == 0) {
    grn_obj_reinit(context, value, GRN_DB_INT32, 0);
    GRN_INT32_SET(context, value, 1);
  }
}

static void
set_two(grn_obj *value, const gchar *type)
{
  if (strcmp(type, "text") == 0) {
    set_text(value, "2");
  } else if (strcmp(type, "int32") == 0) {
    grn_obj_reinit(context, value, GRN_DB_INT32, 0);
    GRN_INT32_SET(context, value, 2);
  }
}

void
data_exec_equal_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " == " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_equal_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);
  set_one(&rhs, rhs_type);

  cut_assert_true(grn_operator_exec_equal(context, &lhs, &rhs));
}

void
data_exec_equal_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " == " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_equal_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);
  set_two(&rhs, rhs_type);

  cut_assert_false(grn_operator_exec_equal(context, &lhs, &rhs));
}

void
data_exec_not_equal_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " != " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_not_equal_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);
  set_two(&rhs, rhs_type);

  cut_assert_true(grn_operator_exec_not_equal(context, &lhs, &rhs));
}

void
data_exec_not_equal_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " != " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_not_equal_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);
  set_one(&rhs, rhs_type);

  cut_assert_false(grn_operator_exec_not_equal(context, &lhs, &rhs));
}

void
data_exec_less_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " < " rhs_type,                       \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("int32", "int32");
  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_less_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);
  set_two(&rhs, rhs_type);

  cut_assert_true(grn_operator_exec_less(context, &lhs, &rhs));
}

void
data_exec_less_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " < " rhs_type,                       \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("int32", "int32");
  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_less_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);
  set_one(&rhs, rhs_type);

  cut_assert_false(grn_operator_exec_less(context, &lhs, &rhs));
}

void
data_exec_greater_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " > " rhs_type,                       \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("int32", "int32");
  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_greater_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_two(&lhs, lhs_type);
  set_one(&rhs, rhs_type);

  cut_assert_true(grn_operator_exec_greater(context, &lhs, &rhs));
}

void
data_exec_greater_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " > " rhs_type,                       \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("int32", "int32");
  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_greater_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);
  set_one(&rhs, rhs_type);

  cut_assert_false(grn_operator_exec_greater(context, &lhs, &rhs));
}

void
data_exec_less_equal_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " <= " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("int32", "int32");
  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_less_equal_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_one(&lhs, lhs_type);

  set_one(&rhs, rhs_type);
  cut_assert_true(grn_operator_exec_less_equal(context, &lhs, &rhs));

  set_two(&rhs, rhs_type);
  cut_assert_true(grn_operator_exec_less_equal(context, &lhs, &rhs));
}

void
data_exec_less_equal_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " <= " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("int32", "int32");
  ADD_DATA("text", "text");
  ADD_DATA("text", "int32");
  ADD_DATA("int32", "text");

#undef ADD_DATA
}

void
test_exec_less_equal_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_two(&lhs, lhs_type);
  set_one(&rhs, rhs_type);

  cut_assert_false(grn_operator_exec_less_equal(context, &lhs, &rhs));
}

void
data_exec_match_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " @ " rhs_type,                       \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");

#undef ADD_DATA
}

void
test_exec_match_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_text(&lhs, "Hello");
  set_text(&rhs, "ll");
  cut_assert_true(grn_operator_exec_match(context, &lhs, &rhs));
}

void
data_exec_match_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " @ " rhs_type,                       \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");

#undef ADD_DATA
}

void
test_exec_match_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_text(&lhs, "Hello");
  set_text(&rhs, "lo!");
  cut_assert_false(grn_operator_exec_match(context, &lhs, &rhs));
}

void
data_exec_prefix_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " @^ " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");

#undef ADD_DATA
}

void
test_exec_prefix_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_text(&lhs, "Hello");
  set_text(&rhs, "He");
  cut_assert_true(grn_operator_exec_prefix(context, &lhs, &rhs));
}

void
data_exec_prefix_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " @^ " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");

#undef ADD_DATA
}

void
test_exec_prefix_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_text(&lhs, "Hello");
  set_text(&rhs, "ell");
  cut_assert_false(grn_operator_exec_prefix(context, &lhs, &rhs));
}

#ifdef GRN_WITH_ONIGMO
void
data_exec_regexp_true(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " @~ " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");

#undef ADD_DATA
}

void
test_exec_regexp_true(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_text(&lhs, "hello");
  set_text(&rhs, "\\Ahe");
  cut_assert_true(grn_operator_exec_regexp(context, &lhs, &rhs));
}

void
data_exec_regexp_false(void)
{
#define ADD_DATA(lhs_type, rhs_type)                            \
  gcut_add_datum(lhs_type " @~ " rhs_type,                      \
                 "lhs_type", G_TYPE_STRING, lhs_type,           \
                 "rhs_type", G_TYPE_STRING, rhs_type,           \
                 NULL)

  ADD_DATA("text", "text");

#undef ADD_DATA
}

void
test_exec_regexp_false(gconstpointer data)
{
  const gchar *lhs_type;
  const gchar *rhs_type;

  lhs_type = gcut_data_get_string(data, "lhs_type");
  rhs_type = gcut_data_get_string(data, "rhs_type");

  set_text(&lhs, "hello");
  set_text(&rhs, "llox\\z");
  cut_assert_false(grn_operator_exec_regexp(context, &lhs, &rhs));
}
#endif
