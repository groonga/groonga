/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License version 2.1 as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_prefix_short_text(void);
void test_prefix_short_text(gpointer data);
void data_near_uint32(void);
void test_near_uint32(gpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *table;
static grn_table_cursor *cursor;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "table-patricia-trie-cursor",
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
  table = NULL;
  cursor = NULL;
}

void
cut_teardown(void)
{
  if (cursor) {
    grn_obj_unlink(context, cursor);
  }

  if (table) {
    grn_obj_unlink(context, table);
  }

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }
}

static void
create_short_text_table(void)
{
  const char *table_name = "ShortTextPat";

  assert_send_commands(
    cut_take_printf("table_create %s TABLE_PAT_KEY ShortText", table_name));
  assert_send_commands(
    cut_take_printf("load --table %s\n"
                    "[\n"
                    " [\"_key\"],\n"
                    " [\"abra\"],"
                    " [\"abracada\"],"
                    " [\"abracadabra\"],"
                    " [\"abubu\"],"
                    " [\"あ\"],"
                    " [\"ああ\"],"
                    " [\"あああ\"],"
                    " [\"い\"]"
                    "]",
                    table_name));

  table = grn_ctx_get(context, table_name, strlen(table_name));
}

static void
create_uint32_table(void)
{
  const char *table_name = "UInt32Pat";

  assert_send_commands(
    cut_take_printf("table_create %s TABLE_PAT_KEY UInt32", table_name));
  assert_send_commands(
    cut_take_printf("load --table %s\n"
                    "[\n"
                    " [\"_key\"],\n"
                    " [%u],"
                    " [%u],"
                    " [%u],"
                    " [%u],"
                    " [%u]"
                    "]",
                    table_name,
                    0x00000000U,
                    0x00000004U,
                    0x00000080U,
                    0xdeadbeefU,
                    0xffffffffU));

  table = grn_ctx_get(context, table_name, strlen(table_name));
}

void
data_prefix_short_text(void)
{
#define ADD_DATA(label, expected, key_min, offset, limit, flags)        \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, gcut_list_string_free,                       \
                 "key-min", G_TYPE_STRING, key_min,                     \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                           \
                 "flags", G_TYPE_INT, flags,                            \
                 NULL)

  ADD_DATA("alphabet - ascending",
           gcut_list_string_new("abra", "abracada", "abracadabra", "abubu",
                                NULL),
           "ab",
           0, -1,
           0);
  ADD_DATA("alphabet - descending",
           gcut_list_string_new("abubu", "abracadabra", "abracada", "abra",
                                NULL),
           "ab",
           0, -1,
           GRN_CURSOR_DESCENDING);
  ADD_DATA("alphabet - ascending - greater than",
           gcut_list_string_new("abracada", "abracadabra", NULL),
           "abra",
           0, -1,
           GRN_CURSOR_GT);
  ADD_DATA("alphabet - descending - greater than",
           gcut_list_string_new("abracadabra", "abracada", NULL),
           "abra",
           0, -1,
           GRN_CURSOR_DESCENDING | GRN_CURSOR_GT);
  ADD_DATA("alphabet - offset and limit",
           gcut_list_string_new("abracadabra", NULL),
           "ab",
           2, 1,
           0);
  ADD_DATA("no match",
           NULL,
           "bubuzera",
           0, -1,
           0);
  ADD_DATA("no match - common prefix",
           NULL,
           "abraura",
           0, -1,
           0);
  ADD_DATA("empty key",
           gcut_list_string_new("abra", "abracada", "abracadabra", "abubu",
                                "あ", "ああ", "あああ", "い",
                                NULL),
           "",
           0, -1,
           0);
  {
    gchar *long_key;
    long_key = g_alloca(GRN_TABLE_MAX_KEY_SIZE + 2);
    memset(long_key, 'a', GRN_TABLE_MAX_KEY_SIZE + 1);
    ADD_DATA("long key",
             NULL,
             long_key,
             0, -1,
             0);
  }

#undef ADD_DATA
}

void
test_prefix_short_text(gpointer data)
{
  grn_id id;
  const gchar *key_min;
  int offset, limit, flags;
  const GList *expected_keys;
  GList *actual_keys = NULL;

  create_short_text_table();

  key_min = gcut_data_get_string(data, "key-min");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 key_min, strlen(key_min),
                                 NULL, 0,
                                 offset, limit,
                                 flags | GRN_CURSOR_PREFIX);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    gchar *key;
    int key_length;

    key_length = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys, g_strndup(key, key_length));
  }
  gcut_take_list(actual_keys, g_free);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}

static GList *
uint_list_new(gint n, guint value, ...)
{
  GList *list = NULL;
  va_list args;
  gint i;

  va_start(args, value);
  for (i = 0; i < n; i++) {
    list = g_list_prepend(list, GUINT_TO_POINTER(value));
    value = va_arg(args, guint);
  }
  va_end(args);

  return g_list_reverse(list);
}

void
data_near_uint32(void)
{
#define ADD_DATA(label, expected, min_length, max, offset, limit, flags) \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, g_list_free,                                 \
                 "min-length", G_TYPE_INT, min_length,                  \
                 "max", G_TYPE_UINT, max,                               \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 "flags", G_TYPE_INT, flags,                            \
                 NULL)

  ADD_DATA("no limit",
           uint_list_new(5,
                         0x00000000U, 0x00000004U, 0x00000080U,
                         0xdeadbeefU, 0xffffffffU),
           0, 0,
           0, -1,
           0);
  ADD_DATA("min limit",
           uint_list_new(3, 0x00000000U, 0x00000004U, 0x00000080U),
           1, 0,
           0, -1,
           0);

#undef ADD_DATA
}

void
test_near_uint32(gpointer data)
{
  grn_id id;
  int min_length, offset, limit, flags;
  guint32 max;
  const GList *expected_keys;
  GList *actual_keys = NULL;

  create_uint32_table();

  min_length = gcut_data_get_int(data, "min-length");
  max = gcut_data_get_uint(data, "max");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 NULL, min_length,
                                 &max, sizeof(max),
                                 offset, limit,
                                 flags | GRN_CURSOR_PREFIX);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    guint32 *key;
    int key_length;

    key_length = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys, GUINT_TO_POINTER(*key));
  }
  gcut_take_list(actual_keys, NULL);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_uint(expected_keys, actual_keys);
}
