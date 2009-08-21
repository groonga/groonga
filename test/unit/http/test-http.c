/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Yuto HAYAMIZU <y.hayamizu@gmail.com>

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

#include <glib/gstdio.h>
#include <soupcutter.h> /* must be included after memcached.h */

#include "../lib/grn-assertions.h"

#define GROONGA_TEST_PORT 4545

/* globals */
static gchar *tmp_directory;

static GCutEgg *egg;

static SoupSession *session;

static SoupCutClient *client;

static SoupMessage *message;

static grn_ctx context;
static grn_obj *database;

void
cut_setup(void)
{
  GError *error = NULL;
  const gchar *db_path;

  tmp_directory = g_build_filename(grn_test_get_base_dir(), "tmp", NULL);
  cut_remove_path(tmp_directory, NULL);
  if (g_mkdir_with_parents(tmp_directory, 0700) == -1) {
    cut_assert_errno();
  }

  session = NULL;
  client = NULL;
  message = NULL;

  grn_ctx_init(&context, 0);
  database = NULL;
  db_path = cut_take_printf("%s%s%s",
                            tmp_directory,
                            G_DIR_SEPARATOR_S,
                            "http.db");
  
  egg = gcut_egg_new(GROONGA, "-s",
                     "-p", cut_take_printf("%d", GROONGA_TEST_PORT),
                     "-n", db_path,
                     NULL);
  gcut_egg_hatch(egg, &error);
  gcut_assert_error(error);

  session = soup_session_sync_new();

  g_usleep(G_USEC_PER_SEC);

  database = grn_db_open(&context, db_path);
}

void
cut_teardown(void)
{
  if (egg) {
    g_object_unref(egg);
  }

  if (message) {
    g_object_unref(message);
  }

  if (session) {
    g_object_unref(session);
  }

  if (client) {
    g_object_unref(client);
  }

  if (database) {
    grn_obj_unlink(&context, database);
  }
  
  grn_ctx_fin(&context);
  
  cut_remove_path(tmp_directory, NULL);
}

static void
assert_get(const gchar *path, const gchar *first_param, ...)
{
  va_list args;
  SoupURI *uri;
  GHashTable *params;
  guint status;

  if(message) {
    g_object_unref(message);
  }

  uri = soup_uri_new(NULL);
  soup_uri_set_scheme(uri, SOUP_URI_SCHEME_HTTP);
  soup_uri_set_host(uri, "localhost");
  soup_uri_set_port(uri, GROONGA_TEST_PORT);
  soup_uri_set_path(uri, path);

  va_start(args, first_param);
  params = gcut_hash_table_string_string_new_va_list(first_param, args);
  va_end(args);

  soup_uri_set_query_from_form(uri, params);
  g_hash_table_unref(params);

  message = soup_message_new("GET", cut_take_string(soup_uri_to_string(uri, FALSE)));
  soup_uri_free(uri);

  status = soup_session_send_message(session, message);

  cut_assert_equal_uint(SOUP_STATUS_OK, status);
}

static void
assert_equal_response_body(const gchar *expected, SoupMessage *message)
{
  cut_assert_equal_string(expected, message->response_body->data);
}

static void
assert_equal_content_type(const gchar *expected, SoupMessage *message)
{
  const gchar *actual;
  actual = soup_message_headers_get_content_type(message->response_headers, NULL);
  cut_assert_equal_string(expected, actual);
}

void
test_get_root(void)
{
  client = soupcut_client_new();
  soupcut_client_get(client, cut_take_printf("http://localhost:%u/", GROONGA_TEST_PORT), NULL);

  soupcut_client_assert_response(client);
  soupcut_client_assert_equal_content_type("text/javascript", client);
  soupcut_client_assert_equal_body("", client);
}

void
test_get_status(void)
{
  assert_get("/status", NULL);
  
  assert_equal_content_type("text/javascript", message);
  cut_assert_match("{\"starttime\":\\d+,\"uptime\":\\d+}",
                   message->response_body->data);
}

void
test_get_table_list(void)
{
  grn_obj *users;
  grn_obj_flags flags;
  const gchar *table_name = "users";
  
  assert_get("/table_list", NULL);

  assert_equal_content_type("text/javascript", message);
  assert_equal_response_body("[[\"id\",\"name\",\"path\",\"flags\",\"domain\"]]",
                             message);
  flags = GRN_OBJ_PERSISTENT | GRN_OBJ_TABLE_PAT_KEY;
  assert_get("/table_create",
             "name", table_name,
             "flags", cut_take_printf("%u", flags),
             "key_type", "Int8",
             "value_type", "Object",
             "default_tokenizer", "",
             NULL);
  assert_equal_response_body("true", message);
  users = grn_ctx_get(&context, table_name, strlen(table_name));
  grn_test_assert_not_null(&context, users);
  assert_get("/table_list", NULL);
  assert_equal_content_type("text/javascript", message);
  assert_equal_response_body(
    cut_take_printf("["
                    "[\"id\",\"name\",\"path\",\"flags\",\"domain\"],"
                    "[%u,\"%s\",\"%s\",%u,%u]"
                    "]",
                    grn_obj_id(&context, users),
                    table_name,
                    grn_obj_path(&context, users),
                    flags | GRN_OBJ_KEY_INT,
                    GRN_DB_INT8),
    message);
}

void
test_get_column_list(void)
{
  const gchar *table_name = "users";
  const gchar *column_name = "age";
  const gchar *full_column_name;
  grn_obj *users;
  grn_obj *age;
  full_column_name = cut_take_printf("%s.%s", table_name, column_name);
  users = grn_table_create(&context, table_name, strlen(table_name),
                           NULL, GRN_OBJ_PERSISTENT | GRN_OBJ_TABLE_PAT_KEY,
                           grn_ctx_at(&context, GRN_DB_INT8),
                           grn_ctx_at(&context, GRN_DB_OBJECT));
  grn_test_assert_not_null(&context, users);
  assert_get("/column_list", "table", table_name, NULL);
  assert_equal_response_body(
    "[[\"id\",\"name\",\"path\",\"type\",\"flags\",\"domain\"]]",
    message);

  assert_get("/column_create",
             "table", table_name,
             "name", column_name,
             "flags", cut_take_printf("%u", GRN_OBJ_COLUMN_SCALAR),
             "type", "Int8",
             NULL);
  assert_equal_response_body("true", message);
  age = grn_ctx_get(&context, full_column_name, strlen(full_column_name));
  grn_test_assert_not_null(&context, age);
  assert_get("/column_list", "table", table_name, NULL);
  assert_equal_response_body(
    cut_take_printf("["
                    "[\"id\",\"name\",\"path\",\"type\",\"flags\",\"domain\"],"
                    "[%u,\"%s\",\"%s\",\"fix\",%u,%u]"
                    "]",
                    grn_obj_id(&context, age),
                    column_name,
                    grn_obj_path(&context, age),
                    GRN_OBJ_COLUMN_SCALAR | GRN_OBJ_PERSISTENT | GRN_OBJ_KEY_INT,
                    grn_obj_id(&context, users)),
    message);
}

void
test_select(void)
{
  const gchar *table_name = "users";
  const gchar *column_name = "age";
  const gchar *hayamizu_name = "Hayamizu";
  gint hayamizu_age = 22;
  grn_obj *users;
  grn_obj *age;
  grn_id hayamizu_id;
  grn_obj age_value;
  
  users = grn_table_create(&context, table_name, strlen(table_name),
                           NULL, GRN_OBJ_PERSISTENT | GRN_OBJ_TABLE_PAT_KEY,
                           grn_ctx_at(&context, GRN_DB_SHORT_TEXT),
                           NULL);
  grn_test_assert_not_null(&context, users);
  age = grn_column_create(&context, users, column_name, strlen(column_name),
                          NULL, GRN_OBJ_PERSISTENT | GRN_OBJ_COLUMN_SCALAR,
                          grn_ctx_at(&context, GRN_DB_INT32));
  grn_test_assert_not_null(&context, age);

  hayamizu_id = grn_table_add(&context, users,
                              hayamizu_name, strlen(hayamizu_name),
                              NULL);
  grn_test_assert_not_nil(hayamizu_id);
  cut_assert_equal_uint(1, grn_table_size(&context, users));

  GRN_INT32_INIT(&age_value, 0);
  GRN_INT32_SET(&context, &age_value, 22);
  grn_obj_set_value(&context, age, hayamizu_id, &age_value, GRN_OBJ_SET);
  grn_obj_unlink(&context, &age_value);

  assert_get("/select",
             "table", table_name,
             "query", cut_take_printf("%s:%d", column_name, hayamizu_age),
             NULL);
  assert_equal_response_body(
    cut_take_printf("[1,"
                    "[\"_id\",\"_key\",\"%s\"],"
                    "[%u,\"%s\",%d]"
                    "]",
                    column_name, hayamizu_id, hayamizu_name, hayamizu_age),
    message);
}
