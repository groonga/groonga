/*
  Copyright (C) 2010-2013  Kouhei Sutou <kou@clear-code.com>

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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_table(void);
void test_table(gconstpointer data);
void data_normalize(void);
void test_normalize(gconstpointer data);

static grn_logger_info *logger;

static gchar *tmp_directory;
static const gchar *database_path;

static grn_ctx *context;
static grn_obj *database;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "table-cursor",
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

  context = NULL;

  logger = setup_grn_logger();

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);
}

void
cut_teardown(void)
{
  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  teardown_grn_logger(logger);

  remove_tmp_directory();
}

void
data_table(void)
{
#define ADD_DATA(label, flags)                  \
  gcut_add_datum(label,                         \
                 "flags", G_TYPE_INT, flags,    \
                 NULL)

  ADD_DATA("no-key", GRN_OBJ_TABLE_NO_KEY);
  ADD_DATA("hash", GRN_OBJ_TABLE_HASH_KEY);
  ADD_DATA("patricia trie", GRN_OBJ_TABLE_PAT_KEY);

#undef ADD_DATA
}

void
test_table(gconstpointer data)
{
  grn_obj *table;
  grn_obj_flags flags = gcut_data_get_int(data, "flags");
  grn_table_cursor *cursor;

  table = grn_table_create(context, NULL, 0, NULL,
                           flags,
                           get_object("ShortText"),
                           NULL);
  cursor = grn_table_cursor_open(context, table, NULL, 0, NULL, 0, 0, -1, 0);
  /* FIXME: grn_test_assert_equal_object() */
  cut_assert_equal_pointer(table, grn_table_cursor_table(context, cursor));
}

void
data_normalize(void)
{
#define ADD_DATA(label, table_type, key)                        \
  gcut_add_datum(label,                                         \
                 "table_type", G_TYPE_STRING, table_type,       \
                 "key", G_TYPE_STRING, key,                     \
                 NULL)

  ADD_DATA("hash table - lower",        "TABLE_HASH_KEY", "alice");
  ADD_DATA("hash table - upper",        "TABLE_HASH_KEY", "ALICE");
  ADD_DATA("hash table - mixed",        "TABLE_HASH_KEY", "AlIcE");

  ADD_DATA("patricia trie - lower",     "TABLE_PAT_KEY",  "alice");
  ADD_DATA("patricia trie - upper",     "TABLE_PAT_KEY",  "ALICE");
  ADD_DATA("patricia trie - mixed",     "TABLE_PAT_KEY",  "AlIcE");

  ADD_DATA("double array trie - lower", "TABLE_DAT_KEY",  "alice");
  ADD_DATA("double array trie - upper", "TABLE_DAT_KEY",  "ALICE");
  ADD_DATA("double array trie - mixed", "TABLE_DAT_KEY",  "AlIcE");

#undef ADD_DATA
}

void
test_normalize(gconstpointer data)
{
  grn_obj *table;
  const gchar *search_key = gcut_data_get_string(data, "key");
  GList *actual_keys = NULL;
  grn_table_cursor *cursor;

  assert_send_command(
    cut_take_printf("table_create Users %s ShortText "
                      "--normalizer NormalizerAuto",
                    gcut_data_get_string(data, "table_type")));
  cut_assert_equal_string(
    "2",
    send_command("load --table Users --columns _key\n"
                 "[\n"
                 "  [\"Alice\"],\n"
                 "  [\"Bob\"]\n"
                 "]"));

  table = grn_ctx_get(context, "Users", -1);
  cursor = grn_table_cursor_open(context, table,
                                 search_key, strlen(search_key),
                                 search_key, strlen(search_key),
                                 0, -1, 0);
  while (grn_table_cursor_next(context, cursor) != GRN_ID_NIL) {
    void *key;
    int key_length;
    key_length = grn_table_cursor_get_key(context, cursor, &key);
    actual_keys = g_list_append(actual_keys, g_strndup(key, key_length));
  }
  grn_table_cursor_close(context, cursor);
  gcut_take_list(actual_keys, g_free);

  gcut_assert_equal_list_string(gcut_take_new_list_string("alice", NULL),
                                actual_keys);
}

