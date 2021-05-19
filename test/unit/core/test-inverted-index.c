/*
  Copyright (C) 2008-2015  Kouhei Sutou <kou@clear-code.com>

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

#include <grn_ii.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <float.h>

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
void test_array_index(void);
void test_scalar_index(void);
void test_int_index(void);
void test_mroonga_index(void);
void test_mroonga_index_score(void);
void test_estimate_size_for_query(void);

#define TYPE_SIZE 1024

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

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "inverted-index",
                                   NULL);
}

void
cut_shutdown(void)
{
  g_free(tmp_directory);
  cut_remove_path(grn_test_get_tmp_dir(), NULL);
}

static void
remove_tmp_directory(void)
{
  cut_remove_path(tmp_directory, NULL);
}

void
cut_setup(void)
{
  gchar *table_path;
  const gchar *type_name, *table_name;

  cut_set_fixture_data_dir(grn_test_get_base_dir(),
                           "fixtures",
                           "inverted-index",
                           NULL);

  logger = setup_grn_logger();

  expected_messages = NULL;
  record_ids = NULL;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);
  path = g_build_filename(tmp_directory, "inverted-index", NULL);

  context = g_new0(grn_ctx, 1);
  grn_test_assert(grn_ctx_init(context, 0));
  GRN_CTX_SET_ENCODING(context, GRN_ENC_UTF8);

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
                             type, NULL);

  grn_obj_set_info(context, lexicon, GRN_INFO_DEFAULT_TOKENIZER,
                   grn_ctx_at(context, GRN_DB_BIGRAM));

  g_free(table_path);

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
cut_teardown(void)
{
  if (context) {
    inverted_index_free();
    if (path)
      grn_ii_remove(context, path);
    grn_obj_close(context, db);
    grn_ctx_fin(context);
    g_free(context);
  }

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
  ((grn_db_obj *)inverted_index)->header.domain = GRN_DB_VOID;  \
  cut_assert_not_null(inverted_index);                          \
  cut_assert_file_exist(cut_take_printf("%s.c", path));         \
} while (0)

void
test_create(void)
{
  cut_assert_create();
}

void
test_create_with_null_path(void)
{
  inverted_index = grn_ii_create(context, NULL, lexicon, 0);
  ((grn_db_obj *)inverted_index)->header.domain = GRN_DB_VOID;
  cut_assert_not_null(inverted_index);
}

void
test_create_with_long_path(void)
{
  gssize max_size = PATH_MAX - 6;
  GString *long_path;
  const gchar last_component[] = G_DIR_SEPARATOR_S "index";

  if (getenv("TRAVIS")) {
    cut_omit("It is crashed on Travis CI. Why?");
  }

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
  const gchar *expected_error_message = "system call error";

  io = grn_io_create(context, path, 10, 10, 10,
                     GRN_IO_AUTO, GRN_IO_EXPIRE_SEGMENT);
  cut_assert_not_null(io);
  id_string = grn_io_header(io);
  strcpy(id_string, "WRONG-ID");
  grn_io_close(context, io);

  clear_messages();
  inverted_index = grn_ii_open(context, path, lexicon);
  cut_assert_null(inverted_index);

  cut_assert_equal_substring(expected_error_message,
                             messages()->data,
                             strlen(expected_error_message));
}

void
test_open_invalid_chunk_file(void)
{
  grn_io *io;
  gchar *id_string;
  const gchar *expected_error_message =
    cut_take_printf("[column][index] file type must be 0x48: <%#04x>", 0);

  io = grn_io_create(context, path, 10, 10, 10, GRN_IO_AUTO, GRN_IO_EXPIRE_SEGMENT);
  cut_assert_not_null(io);
  id_string = grn_io_header(io);
  strcpy(id_string, "WRONG-ID");
  grn_io_close(context, io);

  io = grn_io_create(context, cut_take_printf("%s.c", path),
                     10, 10, 10, GRN_IO_AUTO, GRN_IO_EXPIRE_SEGMENT);
  cut_assert_not_null(io);
  grn_io_close(context, io);

  clear_messages();
  inverted_index = grn_ii_open(context, path, lexicon);
  cut_assert_null(inverted_index);

  cut_assert_equal_substring(expected_error_message,
                             messages()->data,
                             strlen(expected_error_message));
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

  GRN_TEXT_INIT(&old_value, GRN_OBJ_DO_SHALLOW_COPY);
  GRN_TEXT_INIT(&new_value, GRN_OBJ_DO_SHALLOW_COPY);

  if (old_name) {
    old_data = cut_get_fixture_data_string(old_name, NULL);
    GRN_TEXT_SET_REF(&old_value, old_data, strlen(old_data));
  }

  if (new_name) {
    new_data = cut_get_fixture_data_string(new_name, NULL);
    GRN_TEXT_SET_REF(&new_value, new_data, strlen(new_data));
  }

  grn_ii_column_update(context, inverted_index, record_id, section,
                       &old_value, &new_value, NULL);
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
  grn_posting *posting;

  term_id = grn_table_get(context, lexicon, term, strlen(term));
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
  gcut_assert_equal_list_string(NULL, retrieve_record_ids("検索"),
                                cut_message("this assertion is wrong?"));
}

static grn_rc
set_index_source(grn_obj *index, grn_obj *source)
{
  grn_rc rc;
  grn_obj buf;
  grn_id id = grn_obj_id(context, source);
  GRN_TEXT_INIT(&buf, 0);
  grn_bulk_write(context, &buf, (void *)&id, sizeof(grn_id));
  cut_assert_equal_int(grn_obj_get_nhooks(context, source, GRN_HOOK_SET), 0);
  rc = grn_obj_set_info(context, index, GRN_INFO_SOURCE, &buf);
  cut_assert_equal_int(grn_obj_get_nhooks(context, source, GRN_HOOK_SET), 1);
  GRN_BULK_REWIND(&buf);
  cut_assert_null(grn_obj_get_hook(context, source, GRN_HOOK_SET, 0, &buf));
  cut_assert_equal_int(*((grn_id *)GRN_BULK_HEAD(&buf)), grn_obj_id(context, index));
  grn_obj_close(context, &buf);
  return rc;
}

static void
insert_and_search(grn_obj *users, grn_obj *items, grn_obj *checks, grn_obj *checked)
{
  grn_id user1 = grn_table_add(context, users, NULL, 0, NULL);
  grn_id user2 = grn_table_add(context, users, NULL, 0, NULL);
  grn_id item = grn_table_add(context, items, NULL, 0, NULL);
  grn_obj value, *res;
  GRN_TEXT_INIT(&value, 0);
  res = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_HASH_KEY, users, 0);
  cut_assert_not_null(res);
  grn_bulk_write(context, &value, (void *)&item, sizeof(grn_id));
  value.header.domain = grn_obj_id(context, items);
  grn_test_assert(grn_obj_set_value(context, checks, user1, &value, GRN_OBJ_SET));
  grn_test_assert(grn_obj_search(context, checked, &value, res, GRN_OP_OR, NULL));
  cut_assert_equal_int(grn_table_size(context, res), 1);
  grn_test_assert(grn_obj_set_value(context, checks, user2, &value, GRN_OBJ_SET));
  grn_test_assert(grn_obj_search(context, checked, &value, res, GRN_OP_OR, NULL));
  cut_assert_equal_int(grn_table_size(context, res), 2);
  grn_obj_close(context, &value);
  grn_obj_close(context, res);
}

void
test_array_index(void)
{
  gchar *db_path;
  const gchar *name;
  grn_obj *users, *items, *checks, *checked;

  grn_obj_close(context, db);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);
  db_path = g_build_filename(tmp_directory, "inverted-index", NULL);
  db = grn_db_create(context, db_path, NULL);
  g_free(db_path);

  name = "users";
  users = grn_table_create(context, name, strlen(name), NULL,
                           GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(users);

  name = "items";
  items = grn_table_create(context, name, strlen(name), NULL,
                           GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(items);

  name = "checks";
  checks = grn_column_create(context, users, name, strlen(name), NULL,
                             GRN_OBJ_COLUMN_VECTOR|GRN_OBJ_PERSISTENT, items);
  cut_assert_not_null(checks);

  name = "checked";
  checked = grn_column_create(context, items, name, strlen(name), NULL,
                              GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT, users);
  cut_assert_not_null(checked);

  grn_test_assert(set_index_source(checked, checks));

  insert_and_search(users, items, checks, checked);

  grn_obj_close(context, checks);
  grn_obj_close(context, checked);
  grn_obj_close(context, items);
  grn_obj_close(context, users);
}

void
test_scalar_index(void)
{
  gchar *db_path;
  const gchar *name;
  grn_obj *users, *items, *checks, *checked;

  grn_obj_close(context, db);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);
  db_path = g_build_filename(tmp_directory, "inverted-index", NULL);
  db = grn_db_create(context, db_path, NULL);
  g_free(db_path);

  name = "users";
  users = grn_table_create(context, name, strlen(name), NULL,
                           GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(users);

  name = "items";
  items = grn_table_create(context, name, strlen(name), NULL,
                           GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(items);

  name = "checks";
  checks = grn_column_create(context, users, name, strlen(name), NULL,
                             GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT, items);
  cut_assert_not_null(checks);

  name = "checked";
  checked = grn_column_create(context, items, name, strlen(name), NULL,
                              GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT, users);
  cut_assert_not_null(checked);

  grn_test_assert(set_index_source(checked, checks));

  insert_and_search(users, items, checks, checked);

  grn_obj_close(context, checks);
  grn_obj_close(context, checked);
  grn_obj_close(context, items);
  grn_obj_close(context, users);
}

void
test_int_index(void)
{
  gchar *db_path;
  const gchar *name;
  grn_obj *users, *items, *checks, *checked, *int_type;

  grn_obj_close(context, db);

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);
  db_path = g_build_filename(tmp_directory, "inverted-index", NULL);
  db = grn_db_create(context, db_path, NULL);
  g_free(db_path);

  int_type = grn_ctx_at(context, GRN_DB_INT32);
  cut_assert_not_null(int_type);

  name = "users";
  users = grn_table_create(context, name, strlen(name), NULL,
                           GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(users);

  name = "items";
  items = grn_table_create(context, name, strlen(name), NULL,
                           GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT, int_type, 0);
  cut_assert_not_null(items);

  name = "checks";
  checks = grn_column_create(context, users, name, strlen(name), NULL,
                             GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT, items);
  cut_assert_not_null(checks);

  name = "checked";
  checked = grn_column_create(context, items, name, strlen(name), NULL,
                              GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT, users);
  cut_assert_not_null(checked);

  grn_test_assert(set_index_source(checked, checks));

  {
    int32_t key = 1;
    grn_obj value, query, *res;
    grn_id user1 = grn_table_add(context, users, NULL, 0, NULL);
    grn_id user2 = grn_table_add(context, users, NULL, 0, NULL);
    grn_id item = grn_table_add(context, items, &key, sizeof(int32_t), NULL);
    GRN_TEXT_INIT(&value, 0);
    GRN_TEXT_INIT(&query, 0);
    res = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_HASH_KEY, users, 0);
    cut_assert_not_null(res);
    grn_bulk_write(context, &value, (void *)&item, sizeof(grn_id));
    value.header.domain = grn_obj_id(context, items);
    grn_bulk_write(context, &query, (void *)&key, sizeof(int32_t));
    query.header.domain = GRN_DB_INT32;
    grn_test_assert(grn_obj_set_value(context, checks, user1, &value, GRN_OBJ_SET));
    grn_test_assert(grn_obj_search(context, checked, &value, res, GRN_OP_OR, NULL));
    cut_assert_equal_int(grn_table_size(context, res), 1);
    grn_test_assert(grn_obj_set_value(context, checks, user2, &value, GRN_OBJ_SET));
    grn_test_assert(grn_obj_search(context, checked, &query, res, GRN_OP_OR, NULL));
    cut_assert_equal_int(grn_table_size(context, res), 2);
    grn_obj_close(context, &query);
    grn_obj_close(context, &value);
    grn_obj_close(context, res);
  }

  grn_obj_close(context, checks);
  grn_obj_close(context, checked);
  grn_obj_close(context, items);
  grn_obj_close(context, users);
}

void
test_mroonga_index(void)
{
  grn_obj *t1,*c1,*lc,*ft;
  grn_obj buff;
  grn_id c1_id,r1,r2,r3,r4;
  const gchar *mrn_dir;

  mrn_dir = cut_build_path(tmp_directory, "mrn", NULL);
  g_mkdir_with_parents(mrn_dir, 0700);

  grn_obj_close(context, db);
  db = grn_db_create(context,
                     cut_build_path(mrn_dir, "mroonga.grn", NULL),
                     NULL);
  cut_assert_not_null(db);

  /* actual table */
  t1 = grn_table_create(context, "t1", 2,
                        cut_build_path(mrn_dir, "t1.grn", NULL),
			GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(t1);

  /* lexicon table */
  lc = grn_table_create(context, "lc", 2,
                        cut_build_path(mrn_dir, "lc.grn", NULL),
			GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                        grn_ctx_at(context, GRN_DB_SHORT_TEXT), 0);
  cut_assert_not_null(lc);
  grn_test_assert(grn_obj_set_info(context, lc, GRN_INFO_DEFAULT_TOKENIZER,
				   grn_ctx_at(context, GRN_DB_BIGRAM)));

  /* actual column */
  c1 = grn_column_create(context, t1, "c1", 2,
                         cut_build_path(mrn_dir, "t1.c1.grn", NULL),
			 GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
			 grn_ctx_at(context, GRN_DB_TEXT));
  cut_assert_not_null(c1);

  /* fulltext index */
  ft = grn_column_create(context, lc, "ft", 2,
                         cut_build_path(mrn_dir, "lc.ft.grn", NULL),
			 GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(ft);

  GRN_TEXT_INIT(&buff,0);

  /* link between actual column and fulltext index */
  c1_id = grn_obj_id(context, c1);
  GRN_TEXT_SET(context, &buff, (char*)&c1_id, sizeof(grn_id));
  grn_obj_set_info(context, ft, GRN_INFO_SOURCE, &buff); /* need to use grn_id */

  /* insert row */
  r1 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(1,r1);
  GRN_TEXT_SET(context, &buff, "abcde", 5);
  grn_test_assert(grn_obj_set_value(context, c1, r1, &buff, GRN_OBJ_SET));

  r2 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(2,r2);
  GRN_TEXT_SET(context, &buff, "fghij", 5);
  grn_test_assert(grn_obj_set_value(context, c1, r2, &buff, GRN_OBJ_SET));

  r3 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(3,r3);
  GRN_TEXT_SET(context, &buff, "11 22 33", 8);
  grn_test_assert(grn_obj_set_value(context, c1, r3, &buff, GRN_OBJ_SET));

  r4 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(4,r4);
  GRN_TEXT_SET(context, &buff, "44 22 55", 8);
  grn_test_assert(grn_obj_set_value(context, c1, r4, &buff, GRN_OBJ_SET));

  /* confirm record are inserted in both column and index */
  cut_assert_equal_int(4,grn_table_size(context,t1));
  cut_assert_equal_int(23,grn_table_size(context,lc));

  /* nlq search */
  {
    grn_id id, docid;
    grn_obj *res;
    grn_table_cursor *tc;
    res = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_HASH_KEY, t1, 0);
    GRN_BULK_REWIND(&buff);
    GRN_TEXT_SET(context, &buff, "hi", 2);
    grn_obj_search(context, ft, &buff, res, GRN_OP_OR, NULL);
    cut_assert_equal_int(1, grn_table_size(context, res));
    tc = grn_table_cursor_open(context, res, NULL, 0, NULL, 0, 0, -1, 0);
    while ((id = grn_table_cursor_next(context, tc))) {
      GRN_BULK_REWIND(&buff);
      grn_table_get_key(context, res, id, &docid, sizeof(grn_id));
      cut_assert_equal_int(2, docid);
      cut_assert_not_null(grn_obj_get_value(context, c1, docid, &buff));
      cut_assert_equal_int(5 ,GRN_TEXT_LEN(&buff));
      cut_assert_equal_substring("fghij", (char*) GRN_BULK_HEAD(&buff),GRN_TEXT_LEN(&buff));
    }
    grn_table_cursor_close(context, tc);
    grn_obj_close(context, res);
  }

  /* boolean search */
  {
    grn_id id, docid;
    grn_obj *match_columns, *match_columns_variable;
    grn_obj *expression, *expression_variable;
    grn_obj *res;
    grn_table_cursor *tc;
    const char *match_columns_expression = "c1";
    const char *qstr = "+22 -55";

    GRN_EXPR_CREATE_FOR_QUERY(context, t1,
                              match_columns, match_columns_variable);
    grn_expr_parse(context, match_columns,
                   match_columns_expression,
                   strlen(match_columns_expression),
                   NULL, GRN_OP_MATCH, GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    GRN_EXPR_CREATE_FOR_QUERY(context, t1, expression, expression_variable);
    res = grn_table_create(context, NULL, 0, NULL, GRN_TABLE_HASH_KEY, t1, 0);
    grn_test_assert(grn_expr_parse(context, expression,
                                   qstr, strlen(qstr),
                                   match_columns,
                                   GRN_OP_MATCH, GRN_OP_OR,
                                   GRN_EXPR_SYNTAX_QUERY));
    grn_table_select(context, t1, expression, res, GRN_OP_OR);
    cut_assert_equal_int(1, grn_table_size(context, res));
    tc = grn_table_cursor_open(context, res, NULL, 0, NULL, 0, 0, -1, 0);
    while ((id = grn_table_cursor_next(context, tc))) {
      GRN_BULK_REWIND(&buff);
      grn_table_get_key(context, res, id, &docid, sizeof(grn_id));
      cut_assert_equal_int(3, docid);
      cut_assert_not_null(grn_obj_get_value(context, c1, docid, &buff));
      cut_assert_equal_int(8 ,GRN_TEXT_LEN(&buff));
      cut_assert_equal_substring("11 22 33", (char*) GRN_BULK_HEAD(&buff),GRN_TEXT_LEN(&buff));
    }
    grn_obj_close(context, expression);
    grn_obj_close(context, match_columns);
    grn_table_cursor_close(context ,tc);
    grn_obj_close(context, res);
  }

  grn_obj_close(context, &buff);
  grn_obj_close(context, ft);
  grn_obj_close(context, c1);
  grn_obj_close(context, lc);
  grn_obj_close(context, t1);
}

void
test_mroonga_index_score(void)
{
  grn_obj *t1,*c1,*lc,*ft;
  grn_obj buff;
  grn_id r1,r2,r3,r4;
  const gchar *mrn_dir;

  mrn_dir = cut_build_path(tmp_directory, "mrn", NULL);
  g_mkdir_with_parents(mrn_dir, 0700);

  grn_obj_close(context, db);
  db = grn_db_create(context,
                     cut_build_path(mrn_dir, "mroonga.grn", NULL),
                     NULL);
  cut_assert_not_null(db);

  /* actual table */
  t1 = grn_table_create(context, "t1", 2,
                        cut_build_path(mrn_dir, "t1.grn", NULL),
			GRN_OBJ_TABLE_NO_KEY|GRN_OBJ_PERSISTENT, NULL, 0);
  cut_assert_not_null(t1);

  /* lexicon table */
  lc = grn_table_create(context, "lc", 2,
                        cut_build_path(mrn_dir, "lc.grn", NULL),
			GRN_OBJ_TABLE_PAT_KEY|GRN_OBJ_PERSISTENT,
                        grn_ctx_at(context, GRN_DB_SHORT_TEXT), 0);
  cut_assert_not_null(lc);
  grn_test_assert(grn_obj_set_info(context, lc, GRN_INFO_DEFAULT_TOKENIZER,
				   grn_ctx_at(context, GRN_DB_BIGRAM)));

  /* actual column */
  c1 = grn_column_create(context, t1, "c1", 2,
                         cut_build_path(mrn_dir, "t1.c1.grn", NULL),
			 GRN_OBJ_COLUMN_SCALAR|GRN_OBJ_PERSISTENT,
			 grn_ctx_at(context, GRN_DB_TEXT));
  cut_assert_not_null(c1);

  /* fulltext index */
  ft = grn_column_create(context, lc, "ft", 2,
                         cut_build_path(mrn_dir, "lc.ft.grn", NULL),
			 GRN_OBJ_COLUMN_INDEX|GRN_OBJ_PERSISTENT, t1);
  cut_assert_not_null(ft);

  GRN_TEXT_INIT(&buff,0);

  /* link between actual column and fulltext index */
  GRN_UINT32_SET(context, &buff, grn_obj_id(context, c1));
  grn_obj_set_info(context, ft, GRN_INFO_SOURCE, &buff); /* need to use grn_id */

  /* insert row */
  r1 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(1,r1);
  GRN_TEXT_SETS(context, &buff, "abcde");
  grn_test_assert(grn_obj_set_value(context, c1, r1, &buff, GRN_OBJ_SET));

  r2 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(2,r2);
  GRN_TEXT_SETS(context, &buff, "fghij");
  grn_test_assert(grn_obj_set_value(context, c1, r2, &buff, GRN_OBJ_SET));

  r3 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(3,r3);
  GRN_TEXT_SETS(context, &buff, "11 22 33");
  grn_test_assert(grn_obj_set_value(context, c1, r3, &buff, GRN_OBJ_SET));

  r4 = grn_table_add(context, t1, NULL, 0, NULL);
  cut_assert_equal_int(4,r4);
  GRN_TEXT_SETS(context, &buff, "44 22 55");
  grn_test_assert(grn_obj_set_value(context, c1, r4, &buff, GRN_OBJ_SET));

  /* confirm record are inserted in both column and index */
  cut_assert_equal_int(4,grn_table_size(context,t1));
  cut_assert_equal_int(23,grn_table_size(context,lc));

  /* nlq search */
  {
    grn_id id, docid;
    grn_obj *res;
    grn_table_cursor *tc;
    grn_obj score, *score_column;
    res = grn_table_create(context, NULL, 0, NULL,
                           GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, t1, 0);
    GRN_FLOAT_INIT(&score, 0);
    GRN_BULK_REWIND(&buff);
    GRN_TEXT_SETS(context, &buff, "hij");
    grn_obj_search(context, ft, &buff, res, GRN_OP_OR, NULL);
    cut_assert_equal_int(1, grn_table_size(context, res));
    score_column = grn_obj_column(context, res, "_score", 6);
    tc = grn_table_cursor_open(context, res, NULL, 0, NULL, 0, 0, -1, 0);
    while ((id = grn_table_cursor_next(context, tc))) {
      GRN_BULK_REWIND(&buff);
      grn_table_get_key(context, res, id, &docid, sizeof(grn_id));
      cut_assert_equal_int(2, docid);
      cut_assert_not_null(grn_obj_get_value(context, c1, docid, &buff));
      cut_assert_equal_int(5 ,GRN_TEXT_LEN(&buff));
      cut_assert_equal_substring("fghij", (char*) GRN_BULK_HEAD(&buff),GRN_TEXT_LEN(&buff));
      grn_obj_get_value(context, score_column, id, &score);
      cut_assert_equal_double(1.0, DBL_EPSILON, GRN_FLOAT_VALUE(&score));
    }
    grn_table_cursor_close(context, tc);
    grn_obj_close(context, score_column);
    grn_obj_close(context, res);
  }

  /* boolean search */
  {
    grn_id id, docid;
    grn_obj *res;
    grn_obj *match_columns, *match_columns_variable;
    grn_obj *expression, *expression_variable;
    grn_table_cursor *tc;
    grn_obj score, *score_column;
    const char *match_columns_expression = "c1 * 5";
    const char *qstr = "+22 -55";

    GRN_EXPR_CREATE_FOR_QUERY(context, t1,
                              match_columns, match_columns_variable);
    grn_expr_parse(context, match_columns,
                   match_columns_expression,
                   strlen(match_columns_expression),
                   NULL, GRN_OP_MATCH, GRN_OP_AND,
                   GRN_EXPR_SYNTAX_SCRIPT);
    GRN_EXPR_CREATE_FOR_QUERY(context, t1, expression, expression_variable);
    res = grn_table_create(context, NULL, 0, NULL,
                           GRN_TABLE_HASH_KEY|GRN_OBJ_WITH_SUBREC, t1, 0);
    grn_test_assert(grn_expr_parse(context, expression,
                                   qstr, strlen(qstr),
                                   match_columns,
                                   GRN_OP_MATCH, GRN_OP_OR,
                                   GRN_EXPR_SYNTAX_QUERY));
    grn_table_select(context, t1, expression, res, GRN_OP_OR);
    cut_assert_equal_int(1, grn_table_size(context, res));
    GRN_FLOAT_INIT(&score, 0);
    score_column = grn_obj_column(context, res, "_score", 6);
    tc = grn_table_cursor_open(context, res, NULL, 0, NULL, 0, 0, -1, 0);
    while ((id = grn_table_cursor_next(context, tc))) {
      GRN_BULK_REWIND(&buff);
      grn_table_get_key(context, res, id, &docid, sizeof(grn_id));
      cut_assert_equal_int(3, docid);
      cut_assert_not_null(grn_obj_get_value(context, c1, docid, &buff));
      cut_assert_equal_int(8, GRN_TEXT_LEN(&buff));
      cut_assert_equal_substring("11 22 33", (char*) GRN_BULK_HEAD(&buff),GRN_TEXT_LEN(&buff));
      grn_obj_get_value(context, score_column, id, &score);
      cut_assert_equal_double(5, DBL_EPSILON, GRN_FLOAT_VALUE(&score));
    }
    grn_obj_close(context, expression);
    grn_obj_close(context, match_columns);
    grn_table_cursor_close(context ,tc);
    grn_obj_close(context, score_column);
    grn_obj_close(context, res);
  }

  grn_obj_close(context, &buff);
  grn_obj_close(context, ft);
  grn_obj_close(context, c1);
  grn_obj_close(context, lc);
  grn_obj_close(context, t1);
}

void
test_estimate_size_for_query(void)
{
  grn_obj *index_column;
  grn_ii *ii;

  grn_obj_close(context, db);
  db = grn_db_create(context,
                     cut_build_path(tmp_directory, "estimate.grn", NULL),
                     NULL);

  assert_send_command("table_create Memos TABLE_NO_KEY");
  assert_send_command("column_create Memos content COLUMN_SCALAR Text");
  assert_send_command("table_create Terms TABLE_PAT_KEY ShortText "
                      "--default_tokenizer TokenBigramSplitSymbolAlphaDigit "
                      "--normalizer NormalizerAuto");
  assert_send_command("column_create Terms index COLUMN_INDEX|WITH_POSITION "
                      "Memos content");
  assert_send_command("load --table Memos\n"
                      "["
                      "[\"content\"],"
                      "[\"Groonga\"],"
                      "[\"Rroonga\"],"
                      "[\"Mroonga\"]"
                      "]");

  index_column = grn_ctx_get(context, "Terms.index", strlen("Terms.index"));
  ii = (grn_ii *)index_column;

  cut_assert_equal_double(1, DBL_EPSILON,
                          grn_ii_estimate_size_for_query(context,
                                                         ii,
                                                         "Groonga",
                                                         strlen("Groonga"),
                                                         NULL));
}
