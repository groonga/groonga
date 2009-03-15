/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>

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

#include <ii.h>

#include <gcutter.h>

#include "../lib/grn-assertions.h"

void test_create(void);
void test_create_with_null_path(void);
void test_create_with_long_path(void);
void test_create_with_null_lexicon(void);
void test_open(void);
void test_open_nonexistence_path(void);
void test_open_invalid_segment_file(void);
void test_open_invalid_chunk_file(void);
void test_open_with_null_lexicon(void);
void test_crud(void);

#define TYPE_SIZE 1024
#define VALUE_SIZE 1024

static grn_logger_info *logger;

static GList *expected_messages;
static GList *record_ids;

static gchar *tmp_directory;
static gchar *path;
static grn_ctx *context;
static grn_obj *db;
static grn_obj *type;
static grn_obj *lexicon;
static grn_ii *inverted_index;
/*
static grn_vgram *vgram;
*/

void
startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
                                   "test-inverted-index",
                                   NULL);
}

void
shutdown(void)
{
  g_free(tmp_directory);
}

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

void
setup(void)
{
  gchar *table_path, *vgram_path;
  const gchar *type_name, *table_name;

  cut_set_fixture_data_dir(grn_test_get_base_dir(),
                           "fixtures",
                           "inverted-index");

  logger = setup_grn_logger();

  expected_messages = NULL;
  record_ids = NULL;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);
  path = g_build_filename(tmp_directory, "inverted-index", NULL);

  context = g_new0(grn_ctx, 1);
  grn_test_assert(grn_ctx_init(context, GRN_CTX_USE_QL, GRN_ENC_DEFAULT));

  db = grn_db_create(context, NULL, NULL);
  grn_ctx_use(context, db);

  type_name = "name";
  type = grn_type_create(context, type_name, strlen(type_name),
                         GRN_OBJ_KEY_VAR_SIZE, TYPE_SIZE);

  table_name = "lexicon";
  table_path = g_build_filename(tmp_directory, "lexicon-table", NULL);
  lexicon = grn_table_create(context,
                             table_name, strlen(table_name),
                             table_path,
                             GRN_OBJ_PERSISTENT | GRN_OBJ_TABLE_PAT_KEY,
                             type, VALUE_SIZE, GRN_ENC_UTF8);
  g_free(table_path);

  vgram_path = g_build_filename(tmp_directory, "vgram", NULL);
/*
  vgram = grn_vgram_create(vgram_path);
*/
  g_free(vgram_path);

  inverted_index = NULL;
}

static void
inverted_index_free(void)
{
  if (inverted_index) {
    grn_ii_close(context, inverted_index);
    inverted_index = NULL;
  }
}

static void
expected_messages_free(void)
{
  if (expected_messages) {
    gcut_list_string_free(expected_messages);
    expected_messages = NULL;
  }
}

static void
record_ids_free(void)
{
  if (record_ids) {
    gcut_list_string_free(record_ids);
    record_ids = NULL;
  }
}

void
teardown(void)
{
  if (context) {
    inverted_index_free();
    if (path)
      grn_ii_remove(context, path);
    grn_ctx_fin(context);
    g_free(context);
  }

/*
  if (vgram)
    grn_vgram_close(vgram);
*/

  if (path) {
    g_free(path);
    path = NULL;
  }

  remove_tmp_directory();

  record_ids_free();

  expected_messages_free();

  teardown_grn_logger(logger);
}

#define clear_messages()                        \
  grn_collect_logger_clear_messages(logger)

#define messages()                              \
  grn_collect_logger_get_messages(logger)

#define cut_assert_create() do                                  \
{                                                               \
  inverted_index = grn_ii_create(context, path, lexicon, 0);    \
  cut_assert_not_null(inverted_index);                          \
  cut_assert_file_exist(strconcat(path, ".c"));                 \
} while (0)

static const gchar *
strconcat(const char *string1, const char *string2)
{
  return cut_take_string(g_strconcat(string1, string2, NULL));
}

void
test_create(void)
{
  cut_assert_create();
}

void
test_create_with_null_path(void)
{
  cut_omit("grn_ii_create() doesn't check NULL path. should be checked?");

  inverted_index = grn_ii_create(context, NULL, lexicon, 0);
  cut_assert_null(inverted_index);
}

void
test_create_with_long_path(void)
{
  gssize max_size = PATH_MAX - 6;
  GString *long_path;
  const gchar last_component[] = G_DIR_SEPARATOR_S "index";

  long_path = grn_long_path_new(path, max_size - strlen(last_component) - 1);
  g_free(path);

  g_mkdir_with_parents(long_path->str, 0700);
  g_string_append(long_path, last_component);
  path = g_string_free(long_path, FALSE);

  cut_assert_equal_int(max_size, strlen(path) + 1);
  cut_assert_create();

  inverted_index_free();

  long_path = g_string_new(path);
  g_free(path);

  g_string_append(long_path, "X");
  path = g_string_free(long_path, FALSE);

  inverted_index = grn_ii_create(context, path, lexicon, 0);
  cut_assert_null(inverted_index);
}

void
test_create_with_null_lexicon(void)
{
  inverted_index = grn_ii_create(context, path, NULL, 0);
  cut_assert_null(inverted_index);
}

void
test_open(void)
{
  cut_assert_create();
  inverted_index_free();

  inverted_index = grn_ii_open(context, path, lexicon);
  cut_assert(inverted_index);
}

void
test_open_nonexistence_path(void)
{
  inverted_index = grn_ii_open(context, path, lexicon);
  cut_assert_null(inverted_index);
}

void
test_open_invalid_segment_file(void)
{
  grn_io *io;
  gchar *id_string;

  io = grn_io_create(context, path, 10, 10, 10, grn_io_auto, GRN_IO_WO_NREF);
  cut_assert_not_null(io);
  id_string = grn_io_header(io);
  strcpy(id_string, "WRONG-ID");
  grn_io_close(context, io);

  inverted_index = grn_ii_open(context, path, lexicon);
  cut_assert_null(inverted_index);

  expected_messages =
    gcut_list_string_new(cut_take_printf("syscall error '%s.c' (%s)",
                                         path, g_strerror(ENOENT)),
                         NULL);
  gcut_assert_equal_list_string(expected_messages, messages());
}

void
test_open_invalid_chunk_file(void)
{
  grn_io *io;
  gchar *id_string;

  io = grn_io_create(context, path, 10, 10, 10, grn_io_auto, GRN_IO_WO_NREF);
  cut_assert_not_null(io);
  id_string = grn_io_header(io);
  strcpy(id_string, "WRONG-ID");
  grn_io_close(context, io);

  io = grn_io_create(context, cut_take_printf("%s.c", path),
                     10, 10, 10, grn_io_auto, GRN_IO_WO_SEGREF);
  cut_assert_not_null(io);
  grn_io_close(context, io);

  inverted_index = grn_ii_open(context, path, lexicon);
  cut_assert_null(inverted_index);

  expected_messages = gcut_list_string_new("file type unmatch", NULL);
  gcut_assert_equal_list_string(expected_messages, messages());
}

void
test_open_with_null_lexicon(void)
{
  cut_assert_create();
  inverted_index_free();

  inverted_index = grn_ii_open(context, path, NULL);
  cut_assert_null(inverted_index);
}


static void
update_data(grn_id record_id, unsigned int section,
            const gchar *old_name, const gchar *new_name)
{
  grn_obj old_value, new_value;
  const gchar *old_data, *new_data;

  GRN_OBJ_INIT(&old_value, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY);
  if (old_name) {
    old_data = cut_get_fixture_data_string(old_name);
    GRN_BULK_SET(context, &old_value, old_data, strlen(old_data));
  }

  GRN_OBJ_INIT(&new_value, GRN_BULK, GRN_OBJ_DO_SHALLOW_COPY);
  if (new_name) {
    new_data = cut_get_fixture_data_string(new_name);
    GRN_BULK_SET(context, &new_value, new_data, strlen(new_data));
  }

  grn_ii_column_update(context, inverted_index, record_id, section,
                       &old_value, &new_value);
  grn_obj_close(context, &old_value);
  grn_obj_close(context, &new_value);
}

static void
add_data(grn_id record_id, unsigned int section, const gchar *name)
{
  update_data(record_id, section, NULL, name);
}

static void
remove_data(grn_id record_id, unsigned int section, const gchar *name)
{
  update_data(record_id, section, name, NULL);
}

static GList *
retrieve_record_ids(const gchar *term)
{
  grn_id term_id;
  grn_ii_cursor *cursor;
  grn_ii_posting *posting;
  grn_search_flags flags;

  flags = GRN_SEARCH_EXACT;
  term_id = grn_table_lookup(context, lexicon, term, strlen(term), &flags);
  if (term_id == GRN_ID_NIL)
    return NULL;

  record_ids_free();

  cursor = grn_ii_cursor_open(context, inverted_index, term_id,
                              GRN_ID_NIL, GRN_ID_MAX, 5, 0);
  if (!cursor)
    return NULL;

  while ((posting = grn_ii_cursor_next(context, cursor))) {
    record_ids = g_list_append(record_ids, g_strdup_printf("%d", posting->rid));
  }
  grn_ii_cursor_close(context, cursor);

  return record_ids;
}

void
test_crud(void)
{
  cut_assert_create();

  add_data(1, 1, "API.JA");
  add_data(2, 1, "CHECKINSTALL.JA");
  add_data(3, 1, "FUTUREWORKS.JA");
  add_data(4, 1, "INSTALL.JA");
  gcut_assert_equal_list_string(gcut_take_new_list_string("1", "2", "3", NULL),
                                retrieve_record_ids("検索"));

  remove_data(1, 1, "API.JA");
  gcut_assert_equal_list_string(gcut_take_new_list_string("2", "3", NULL),
                                retrieve_record_ids("検索"));

  update_data(3, 1, "FUTUREWORKS.JA", "Makefile.am");
  gcut_assert_equal_list_string(gcut_take_new_list_string("2", NULL),
                                retrieve_record_ids("検索"));

  remove_data(2, 1, "CHECKINSTALL.JA");
  add_data(3, 2, "FUTUREWORKS.JA");
  gcut_assert_equal_list_string(gcut_take_new_list_string("3", NULL),
                                retrieve_record_ids("検索"));

  update_data(4, 1, "INSTALL.JA", "README.JA");
  gcut_assert_equal_list_string(gcut_take_new_list_string("3", "4", NULL),
                                retrieve_record_ids("検索"));

  remove_data(4, 1, "README.JA");
  gcut_assert_equal_list_string(gcut_take_new_list_string("3", NULL),
                                retrieve_record_ids("検索"));

  remove_data(3, 2, "FUTUREWORKS.JA");
  cut_set_message("this assertion is wrong?");
  gcut_assert_equal_list_string(NULL, retrieve_record_ids("検索"));
}
