/*
  Copyright(C) 2013 Kouhei Sutou <kou@clear-code.com>

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

#include <groonga.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

static gchar *tmp_directory;
static gchar *path;
static grn_ctx context;
static grn_obj *database;
static grn_obj escaped_query;

void data_escape_query(void);
void test_escape_query(gconstpointer data);

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "test-expr-syntax",
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
  path = g_build_filename(tmp_directory, "text-expr-syntax", NULL);
  grn_ctx_init(&context, 0);
  database = grn_db_create(&context, path, NULL);

  GRN_TEXT_INIT(&escaped_query, 0);
}

void
cut_teardown(void)
{
  grn_obj_close(&context, &escaped_query);
  grn_obj_close(&context, database);
  grn_ctx_fin(&context);
  cut_remove_path(tmp_directory, NULL);
  g_free(path);
}

void
data_escape_query(void)
{
#define ADD_DATUM(label,                                                \
                  expected, query)                                      \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_STRING, expected,                   \
                 "query", G_TYPE_STRING, query,                         \
                 NULL)

  ADD_DATUM("+",  "a\\+b",  "a+b");
  ADD_DATUM("-",  "a\\-b",  "a-b");
  ADD_DATUM(">",  "a\\>b",  "a>b");
  ADD_DATUM("<",  "a\\<b",  "a<b");
  ADD_DATUM("~",  "a\\~b",  "a~b");
  ADD_DATUM("*",  "a\\*b",  "a*b");
  ADD_DATUM("(",  "a\\(b",  "a(b");
  ADD_DATUM(")",  "a\\)b",  "a)b");
  ADD_DATUM("\"", "a\\\"b", "a\"b");
  ADD_DATUM("\\", "a\\\\b", "a\\b");
  ADD_DATUM(":",  "a\\:b",  "a:b");

#undef ADD_DATUM
}

void
test_escape_query(gconstpointer data)
{
  const gchar *expected;
  const gchar *query;

  query = gcut_data_get_string(data, "query");
  grn_test_assert(grn_expr_syntax_escape_query(&context,
                                               query, -1,
                                               &escaped_query));

  expected = gcut_data_get_string(data, "expected");
  GRN_TEXT_PUTC(&context, &escaped_query, '\0');
  cut_assert_equal_string(expected, GRN_TEXT_VALUE(&escaped_query));
}
