/*
  Copyright (C) 2010-2021  Sutou Kouhei <kou@clear-code.com>

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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <grn_str.h>
#include <grn_util.h>

#define get(name) grn_ctx_get(context, name, strlen(name))

void test_null(void);
void test_void(void);
void test_int8(void);
void test_int16(void);
void test_int32(void);
void test_int64(void);
void test_uint8(void);
void test_uint16(void);
void test_uint32(void);
void test_uint64(void);
void test_float(void);
void test_time(void);
void test_bool_true(void);
void test_bool_false(void);
void test_text(void);
void test_geo_point_tokyo(void);
void test_geo_point_wgs84(void);
void test_array_empty(void);
void test_array_with_records(void);
void test_hash_empty(void);
void test_hash_with_records(void);
void test_patricia_trie_empty(void);
void test_patricia_trie_with_records(void);
void test_patricia_trie_cursor_empty(void);
void test_patricia_trie_cursor_with_records(void);
void test_ptr_empty(void);
void test_ptr_with_object(void);
void test_uvector_empty(void);
void test_uvector_with_records(void);
void test_uvector_bool(void);
void test_vector_empty(void);
void test_pvector_empty(void);
void test_pvector_with_records(void);
void data_accessor_column_name(void);
void test_accessor_column_name(gconstpointer data);
void data_accessor_dynamic_pseudo_column_name(void);
void test_accessor_dynamic_pseudo_column_name(gconstpointer data);
void test_column_fix_size(void);
void test_column_var_size(void);
void test_column_index(void);
void test_type(void);
void test_record(void);
void test_proc_command(void);
void test_proc_function(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

static grn_obj *inspected;

static grn_obj *void_value;
static grn_obj *int8, *int16, *int32, *int64;
static grn_obj *uint8, *uint16, *uint32, *uint64;
static grn_obj *float_value;
static grn_obj *time_value;
static grn_obj *bool_value;
static grn_obj *text;
static grn_obj *geo_point_tokyo, *geo_point_wgs84;
static grn_obj *ptr;
static grn_obj *uvector;
static grn_obj *pvector;
static grn_obj *vector;
static grn_obj *record;
static grn_table_cursor *cursor;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "inspect",
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

static void
setup_values(void)
{
  void_value = NULL;
  int8 = int16 = int32 = int64 = NULL;
  uint8 = uint16 = uint32 = uint64 = NULL;
  float_value = NULL;
  time_value = NULL;
  bool_value = NULL;
  text = NULL;
  geo_point_tokyo = geo_point_wgs84 = NULL;
  ptr = NULL;
  uvector = NULL;
  pvector = NULL;
  vector = NULL;
  record = NULL;
  cursor = NULL;
}

void
cut_setup(void)
{
  const gchar *database_path;

  inspected = NULL;
  setup_values();

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);
}

static void
teardown_values(void)
{
  grn_obj_unlink(context, void_value);
  grn_obj_unlink(context, int8);
  grn_obj_unlink(context, int16);
  grn_obj_unlink(context, int32);
  grn_obj_unlink(context, int64);
  grn_obj_unlink(context, uint8);
  grn_obj_unlink(context, uint16);
  grn_obj_unlink(context, uint32);
  grn_obj_unlink(context, uint64);
  grn_obj_unlink(context, float_value);
  grn_obj_unlink(context, time_value);
  grn_obj_unlink(context, bool_value);
  grn_obj_unlink(context, text);
  grn_obj_unlink(context, geo_point_tokyo);
  grn_obj_unlink(context, geo_point_wgs84);
  grn_obj_unlink(context, ptr);
  grn_obj_unlink(context, uvector);
  grn_obj_unlink(context, pvector);
  grn_obj_unlink(context, vector);
  grn_obj_unlink(context, record);
  grn_table_cursor_close(context, cursor);
}

void
cut_teardown(void)
{
  teardown_values();

  grn_obj_unlink(context, inspected);

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);

  remove_tmp_directory();
}

static const gchar *
inspected_string (void)
{
  return cut_take_printf("%.*s",
                         (int)GRN_TEXT_LEN(inspected),
                         GRN_TEXT_VALUE(inspected));
}

void
test_null(void)
{
  inspected = grn_inspect(context, NULL, NULL);
  cut_assert_equal_string("(NULL)", inspected_string());
}

void
test_void_value(void)
{
  void_value = grn_obj_open(context, GRN_BULK, 0, GRN_DB_VOID);
  GRN_TEXT_PUTS(context, void_value, "void");
  inspected = grn_inspect(context, NULL, void_value);
  cut_assert_equal_string("\"void\"", inspected_string());
}

void
test_int8(void)
{
  int8 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_INT8);
  GRN_INT8_SET(context, int8, G_MAXINT8);
  inspected = grn_inspect(context, NULL, int8);
  cut_assert_equal_string(cut_take_printf("%d", G_MAXINT8),
                          inspected_string());
}

void
test_int16(void)
{
  int16 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_INT16);
  GRN_INT16_SET(context, int16, G_MAXINT16);
  inspected = grn_inspect(context, NULL, int16);
  cut_assert_equal_string(cut_take_printf("%" G_GINT16_FORMAT, G_MAXINT16),
                          inspected_string());
}

void
test_int32(void)
{
  int32 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_INT32);
  GRN_INT32_SET(context, int32, G_MAXINT32);
  inspected = grn_inspect(context, NULL, int32);
  cut_assert_equal_string(cut_take_printf("%" G_GINT32_FORMAT, G_MAXINT32),
                          inspected_string());
}

void
test_int64(void)
{
  int64 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_INT64);
  GRN_INT64_SET(context, int64, G_MAXINT64);
  inspected = grn_inspect(context, NULL, int64);
  cut_assert_equal_string(cut_take_printf("%" G_GINT64_FORMAT, G_MAXINT64),
                          inspected_string());
}

void
test_uint8(void)
{
  uint8 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_UINT8);
  GRN_UINT8_SET(context, uint8, G_MAXUINT8);
  inspected = grn_inspect(context, NULL, uint8);
  cut_assert_equal_string(cut_take_printf("%u", G_MAXUINT8),
                          inspected_string());
}

void
test_uint16(void)
{
  uint16 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_UINT16);
  GRN_UINT16_SET(context, uint16, G_MAXUINT16);
  inspected = grn_inspect(context, NULL, uint16);
  cut_assert_equal_string(cut_take_printf("%" G_GUINT16_FORMAT, G_MAXUINT16),
                          inspected_string());
}

void
test_uint32(void)
{
  uint32 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_UINT32);
  GRN_UINT32_SET(context, uint32, G_MAXUINT32);
  inspected = grn_inspect(context, NULL, uint32);
  cut_assert_equal_string(cut_take_printf("%" G_GUINT32_FORMAT, G_MAXUINT32),
                          inspected_string());
}

void
test_uint64(void)
{
  uint64 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_UINT64);
  GRN_UINT64_SET(context, uint64, G_MAXUINT64);
  inspected = grn_inspect(context, NULL, uint64);
  cut_assert_equal_string(cut_take_printf("%" G_GUINT64_FORMAT, G_MAXUINT64),
                          inspected_string());
}

void
test_float(void)
{
  float_value = grn_obj_open(context, GRN_BULK, 0, GRN_DB_FLOAT);
  GRN_FLOAT_SET(context, float_value, 0.29);
  inspected = grn_inspect(context, NULL, float_value);
  cut_assert_equal_string("0.29", inspected_string());
}

void
test_time(void)
{
  GTimeVal g_time_value;

  g_time_val_from_iso8601("2010-05-31T11:50:29.29+0900", &g_time_value);
  time_value = grn_obj_open(context, GRN_BULK, 0, GRN_DB_TIME);
  GRN_TIME_SET(context, time_value,
               (gint64)g_time_value.tv_sec * G_USEC_PER_SEC +
               g_time_value.tv_usec);
  inspected = grn_inspect(context, NULL, time_value);
  cut_assert_equal_string(cut_take_printf("%ld.290000", g_time_value.tv_sec),
                          inspected_string());
}

void
test_bool_true(void)
{
  bool_value = grn_obj_open(context, GRN_BULK, 0, GRN_DB_BOOL);
  GRN_BOOL_SET(context, bool_value, GRN_TRUE);
  inspected = grn_inspect(context, NULL, bool_value);
  cut_assert_equal_string("true", inspected_string());
}

void
test_bool_false(void)
{
  bool_value = grn_obj_open(context, GRN_BULK, 0, GRN_DB_BOOL);
  GRN_BOOL_SET(context, bool_value, GRN_FALSE);
  inspected = grn_inspect(context, NULL, bool_value);
  cut_assert_equal_string("false", inspected_string());
}

void
test_text(void)
{
  text = grn_obj_open(context, GRN_BULK, 0, GRN_DB_TEXT);
  GRN_TEXT_PUTS(context, text, "niku");
  inspected = grn_inspect(context, NULL, text);
  cut_assert_equal_string("\"niku\"", inspected_string());
}

void
test_geo_point_tokyo(void)
{
  gint takane_latitude, takane_longitude;

  takane_latitude = grn_test_coordinate_in_milliseconds(35.6954581363924);
  takane_longitude = grn_test_coordinate_in_milliseconds(139.564207350021);

  geo_point_tokyo = grn_obj_open(context, GRN_BULK, 0, GRN_DB_TOKYO_GEO_POINT);
  GRN_GEO_POINT_SET(context, geo_point_tokyo, takane_latitude, takane_longitude);
  inspected = grn_inspect(context, NULL, geo_point_tokyo);
  cut_assert_equal_string("[(128503649,502431146) "
                          "((35, 41, 43, 649),(139, 33, 51, 146)) "
                          "[00000001 01111011 11011101 10000100 "
                          "10110101 11111011 01101100 01000110]]",
                          inspected_string());
}

void
test_geo_point_wgs84(void)
{
  gint takane_latitude, takane_longitude;

  takane_latitude = grn_test_coordinate_in_milliseconds(35.6986901);
  takane_longitude = grn_test_coordinate_in_milliseconds(139.56099);

  geo_point_wgs84 = grn_obj_open(context, GRN_BULK, 0, GRN_DB_WGS84_GEO_POINT);
  GRN_GEO_POINT_SET(context, geo_point_wgs84, takane_latitude, takane_longitude);
  inspected = grn_inspect(context, NULL, geo_point_wgs84);
  cut_assert_equal_string("[(128515284,502419564) "
                          "((35, 41, 55, 284),(139, 33, 39, 564)) "
                          "[00000001 01111011 11011101 10000100 "
                          "10111011 10100000 10110110 01110000]]",
                          inspected_string());
}

void
test_array_empty(void)
{
  assert_send_command("table_create Sites TABLE_NO_KEY");
  inspected = grn_inspect(context, NULL, get("Sites"));
  cut_assert_equal_string("#<table:no_key "
                          "Sites "
                          "value:(nil) "
                          "size:0 "
                          "columns:[] "
                          "ids:[] "
                          "subrec:none"
                          ">",
                          inspected_string());
}

void
test_array_with_records(void)
{
  assert_send_command("table_create Sites TABLE_NO_KEY");
  assert_send_command("column_create Sites name COLUMN_SCALAR Text");
  assert_send_command("load "
                      "'[[\"name\"],[\"groonga.org\"],[\"razil.jp\"]]' "
                      "Sites");
  inspected = grn_inspect(context, NULL, get("Sites"));
  cut_assert_equal_string("#<table:no_key "
                          "Sites "
                          "value:(nil) "
                          "size:2 "
                          "columns:[name] "
                          "ids:[1, 2] "
                          "subrec:none"
                          ">",
                          inspected_string());
}

void
test_hash_empty(void)
{
  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  inspected = grn_inspect(context, NULL, get("Sites"));
  cut_assert_equal_string("#<table:hash "
                          "Sites "
                          "key:ShortText "
                          "value:(nil) "
                          "size:0 "
                          "columns:[] "
                          "default_tokenizer:(nil) "
                          "normalizers:(nil) "
                          "keys:[] "
                          "subrec:none"
                          ">",
                          inspected_string());
}

void
test_hash_with_records(void)
{
  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Sites name COLUMN_SCALAR Text");
  assert_send_command("load "
                      "'["
                      "[\"_key\",\"name\"],"
                      "[\"groonga.org\",\"groonga\"],"
                      "[\"razil.jp\",\"Brazil\"]"
                      "]' "
                      "Sites");
  inspected = grn_inspect(context, NULL, get("Sites"));
  cut_assert_equal_string("#<table:hash "
                          "Sites "
                          "key:ShortText "
                          "value:(nil) "
                          "size:2 "
                          "columns:[name] "
                          "default_tokenizer:(nil) "
                          "normalizers:(nil) "
                          "keys:[\"groonga.org\", \"razil.jp\"] "
                          "subrec:none"
                          ">",
                          inspected_string());
}

void
test_patricia_trie_empty(void)
{
  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  inspected = grn_inspect(context, NULL, get("Sites"));
  cut_assert_equal_string("#<table:pat "
                          "Sites "
                          "key:ShortText "
                          "value:(nil) "
                          "size:0 "
                          "columns:[] "
                          "default_tokenizer:(nil) "
                          "normalizers:(nil) "
                          "keys:[] "
                          "subrec:none "
                          "nodes:{}"
                          ">",
                          inspected_string());
}

void
test_patricia_trie_with_records(void)
{
  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Sites name COLUMN_SCALAR Text");
  assert_send_command("load "
                      "'["
                      "[\"_key\",\"name\"],"
                      "[\"groonga.org\",\"groonga\"],"
                      "[\"razil.jp\",\"Brazil\"]"
                      "]' "
                      "Sites");
  inspected = grn_inspect(context, NULL, get("Sites"));
  cut_assert_equal_string(
    "#<table:pat "
    "Sites "
    "key:ShortText "
    "value:(nil) "
    "size:2 "
    "columns:[name] "
    "default_tokenizer:(nil) "
    "normalizers:(nil) "
    "keys:[\"groonga.org\", \"razil.jp\"] "
    "subrec:none "
    "nodes:{\n"
    "2{0,3,0}\n"
    "  L:1{10,7,0}\n"
    "    L:0{0,0,0}\n"
    "    R:1{10,7,0}(\"groonga.org\")[01100111 01110010 01101111 01101111 "
                                     "01101110 01100111 01100001 00101110 "
                                     "01101111 01110010 01100111]\n"
    "  R:2{0,3,0}(\"razil.jp\")[01110010 01100001 01111010 01101001 "
                               "01101100 00101110 01101010 01110000]\n"
    "}"
    ">",
    inspected_string());
}

void
test_patricia_trie_cursor_empty(void)
{
  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  cursor = grn_table_cursor_open(context, get("Sites"),
                                 NULL, 0,
                                 NULL, 0,
                                 0, -1, GRN_CURSOR_ASCENDING);
  inspected = grn_inspect(context, NULL, cursor);
  cut_assert_equal_string("#<cursor:pat:Sites "
                          "current:0 "
                          "tail:0 "
                          "flags:ascending|greater|less "
                          "rest:0 "
                          "entries:[]"
                          ">",
                          inspected_string());
}

void
test_patricia_trie_cursor_with_records(void)
{
  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Sites name COLUMN_SCALAR Text");
  assert_send_command("load "
                      "'["
                      "[\"_key\",\"name\"],"
                      "[\"groonga.org\",\"groonga\"],"
                      "[\"razil.jp\",\"Brazil\"]"
                      "]' "
                      "Sites");
  cursor = grn_table_cursor_open(context, get("Sites"),
                                 NULL, 0,
                                 NULL, 0,
                                 0, -1, GRN_CURSOR_ASCENDING);
  inspected = grn_inspect(context, NULL, cursor);
  cut_assert_equal_string("#<cursor:pat:Sites "
                          "current:0 "
                          "tail:0 "
                          "flags:ascending|greater|less "
                          "rest:2 "
                          "entries:[[2,{0,3,0}], [1,{0,3,0}]]"
                          ">",
                          inspected_string());
}

void
test_ptr_empty(void)
{
  ptr = grn_obj_open(context, GRN_PTR, 0, GRN_ID_NIL);
  inspected = grn_inspect(context, NULL, ptr);
  cut_assert_equal_string("#<ptr:(empty)>", inspected_string());
}

void
test_ptr_with_object(void)
{
  text = grn_obj_open(context, GRN_BULK, 0, GRN_DB_TEXT);
  GRN_TEXT_PUTS(context, text, "niku");

  ptr = grn_obj_open(context, GRN_PTR, 0, GRN_ID_NIL);
  GRN_PTR_SET(context, ptr, text);
  inspected = grn_inspect(context, NULL, ptr);
  cut_assert_equal_string("#<ptr:\"niku\">", inspected_string());
}

void
test_uvector_empty(void)
{
  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  uvector = grn_obj_open(context, GRN_UVECTOR, 0,
                         grn_obj_id(context, get("Sites")));
  inspected = grn_inspect(context, NULL, uvector);
  cut_assert_equal_string("[]", inspected_string());
}

void
test_uvector_with_records(void)
{
  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("load "
                      "'[[\"_key\"],[\"groonga.org\"],[\"razil.jp\"]]' "
                      "Sites");
  uvector = grn_obj_open(context, GRN_UVECTOR, 0,
                         grn_obj_id(context, get("Sites")));
  GRN_RECORD_PUT(context, uvector, 1);
  GRN_RECORD_PUT(context, uvector, 2);
  inspected = grn_inspect(context, NULL, uvector);
  cut_assert_equal_string(
    "["
    "#<element record:#<record:pat:Sites id:1 key:\"groonga.org\">, weight:0.000000>, "
    "#<element record:#<record:pat:Sites id:2 key:\"razil.jp\">, weight:0.000000>"
    "]",
    inspected_string());
}

void
test_uvector_bool(void)
{
  uvector = grn_obj_open(context, GRN_UVECTOR, 0, GRN_DB_BOOL);
  GRN_BOOL_PUT(context, uvector, TRUE);
  GRN_BOOL_PUT(context, uvector, FALSE);
  inspected = grn_inspect(context, NULL, uvector);
  cut_assert_equal_string("[true,false]", inspected_string());
}

void
test_pvector_empty(void)
{
  pvector = grn_obj_open(context, GRN_PVECTOR, 0, GRN_ID_NIL);
  inspected = grn_inspect(context, NULL, pvector);
  cut_assert_equal_string("[]", inspected_string());
}

void
test_pvector_with_records(void)
{
  grn_obj *groonga, *razil;

  pvector = grn_obj_open(context, GRN_PVECTOR, 0, GRN_ID_NIL);
  groonga = grn_obj_open(context, GRN_BULK, 0, GRN_DB_SHORT_TEXT);
  razil = grn_obj_open(context, GRN_BULK, 0, GRN_DB_SHORT_TEXT);
  GRN_TEXT_PUTS(context, groonga, "groonga");
  GRN_TEXT_PUTS(context, razil, "razil");
  GRN_PTR_PUT(context, pvector, groonga);
  GRN_PTR_PUT(context, pvector, razil);
  inspected = grn_inspect(context, NULL, pvector);
  grn_obj_unlink(context, groonga);
  grn_obj_unlink(context, razil);
  cut_assert_equal_string("[\"groonga\", \"razil\"]", inspected_string());
}

void
test_vector_empty(void)
{
  vector = grn_obj_open(context, GRN_VECTOR, 0, GRN_DB_TEXT);
  inspected = grn_inspect(context, NULL, vector);
  cut_assert_equal_string("[]", inspected_string());
}

void
data_accessor_column_name(void)
{
#define ADD_DATUM(table, accessor, inspected)           \
  gcut_add_datum(table "." accessor,                    \
                 "table", G_TYPE_STRING, table,         \
                 "accessor", G_TYPE_STRING, accessor,   \
                 "inspected", G_TYPE_STRING, inspected, \
                 NULL)

  ADD_DATUM("Sites",
            "_id",
            "#<accessor _id(Sites)>");
  ADD_DATUM("Sites",
            "_key",
            "#<accessor _key(Sites)>");
  ADD_DATUM("Sites",
            "_value",
            "#<accessor _value(Sites)>");
  ADD_DATUM("Sites",
            "name._id",
            "#<accessor name(Sites)._id(Names)>");
  ADD_DATUM("Sites",
            "name._key",
            "#<accessor name(Sites)._key(Names)>");
  ADD_DATUM("Sites",
            "name._value",
            "#<accessor name(Sites)._value(Names)>");
  ADD_DATUM("Sites",
            "name.site",
            "#<accessor name(Sites).site(Names)>");
  ADD_DATUM("Sites",
            "name.site.name",
            "#<accessor name(Sites).site(Names).name(Sites)>");
  ADD_DATUM("Names",
            "site.name.site",
            "#<accessor site(Names).name(Sites).site(Names)>");

#undef ADD_DATUM
}

void
test_accessor_column_name(gconstpointer data)
{
  const char *table_name = gcut_data_get_string(data, "table");
  const char *accessor_name = gcut_data_get_string(data, "accessor");
  const char *inspected_accessor = gcut_data_get_string(data, "inspected");
  grn_obj *object, *accessor;

  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText Int32");
  assert_send_command("table_create Names TABLE_PAT_KEY ShortText UInt32");
  assert_send_command("column_create Sites name COLUMN_SCALAR Names");
  assert_send_command("column_create Names site COLUMN_SCALAR Sites");

  object = get_object(table_name);
  accessor = grn_obj_column(context, object,
                            accessor_name, strlen(accessor_name));
  grn_obj_unlink(context, object);
  cut_assert_not_null(accessor);
  inspected = grn_inspect(context, NULL, accessor);
  grn_obj_unlink(context, accessor);
  cut_assert_equal_string(inspected_accessor, inspected_string());
}

void
data_accessor_dynamic_pseudo_column_name(void)
{
#define ADD_DATUM(accessor, inspected)                  \
  gcut_add_datum(accessor,                              \
                 "accessor", G_TYPE_STRING, accessor,   \
                 "inspected", G_TYPE_STRING, inspected, \
                 NULL)

  ADD_DATUM("_score", "#<accessor _score>");

#undef ADD_DATUM
}

void
test_accessor_dynamic_pseudo_column_name(gconstpointer data)
{
  const char *query, *accessor_name, *inspected_accessor;
  grn_obj *table, *result, *expression, *variable, *accessor;

  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Sites uri COLUMN_SCALAR ShortText");
  assert_send_command("load "
                      "'["
                      "[\"_key\",\"uri\"],"
                      "[\"groonga\",\"http://groonga.org/\"]"
                      "]' "
                      "Sites");

  accessor_name = gcut_data_get_string(data, "accessor");
  inspected_accessor = gcut_data_get_string(data, "inspected");

  table = get_object("Sites");
  GRN_EXPR_CREATE_FOR_QUERY(context, table, expression, variable);
  query = "_key:groonga";
  grn_expr_parse(context, expression,
                 query, strlen(query),
                 NULL, GRN_OP_MATCH, GRN_OP_AND,
                 GRN_EXPR_SYNTAX_QUERY | GRN_EXPR_ALLOW_COLUMN);
  result = grn_table_select(context, table, expression, NULL, GRN_OP_AND);
  accessor = grn_obj_column(context, result,
                            accessor_name, strlen(accessor_name));
  cut_assert_not_null(accessor);
  inspected = grn_inspect(context, NULL, accessor);
  grn_obj_unlink(context, accessor);
  cut_assert_equal_string(inspected_accessor, inspected_string());
}

void
test_column_fix_size(void)
{
  grn_obj *column;

  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Sites score COLUMN_SCALAR Int32");

  column = get_object("Sites.score");
  inspected = grn_inspect(context, NULL, column);
  cut_assert_equal_string("#<column:fix_size "
                          "Sites.score "
                          "range:Int32 "
                          "type:scalar "
                          "compress:none"
                          ">",
                          inspected_string());
}

void
test_column_var_size(void)
{
  grn_obj *column;

  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Sites name COLUMN_SCALAR ShortText");

  column = get_object("Sites.name");
  inspected = grn_inspect(context, NULL, column);
  cut_assert_equal_string("#<column:var_size "
                          "Sites.name "
                          "range:ShortText "
                          "type:scalar "
                          "compress:none"
                          ">",
                          inspected_string());
}

void
test_column_index(void)
{
  grn_obj *column;

  assert_send_command("table_create Sites TABLE_PAT_KEY ShortText");
  assert_send_command("table_create Terms TABLE_PAT_KEY ShortText");
  assert_send_command("column_create Terms Sites_key COLUMN_INDEX Sites _key");

  column = get_object("Terms.Sites_key");
  inspected = grn_inspect(context, NULL, column);
  cut_assert_equal_string("#<column:index "
                          "Terms.Sites_key "
                          "range:Sites "
                          "sources:[Sites] "
                          "flags:NONE>",
                          inspected_string());
}

void
test_type(void)
{
  grn_obj *type;

  type = get_object("ShortText");
  inspected = grn_inspect(context, NULL, type);
  cut_assert_equal_string("#<type "
                          "ShortText "
                          "size:4096 "
                          "type:var_size"
                          ">",
                          inspected_string());
}

void
test_record(void)
{
  assert_send_command("table_create Sites TABLE_HASH_KEY ShortText");
  assert_send_command("column_create Sites name COLUMN_SCALAR Text");
  assert_send_command("load "
                      "'["
                      "[\"_key\",\"name\"],"
                      "[\"groonga.org\",\"groonga\"],"
                      "[\"razil.jp\",\"Brazil\"]"
                      "]' "
                      "Sites");

  record = grn_obj_open(context, GRN_BULK, 0,
                        grn_obj_id(context, get_object("Sites")));
  GRN_RECORD_SET(context, record, 1);
  inspected = grn_inspect(context, NULL, record);
  cut_assert_equal_string("#<record:hash:Sites "
                          "id:1 "
                          "key:\"groonga.org\" "
                          "name:\"groonga\""
                          ">",
                          inspected_string());
}

void
test_proc_command(void)
{
  grn_obj *proc;

  proc = get_object("column_remove");
  inspected = grn_inspect(context, NULL, proc);
  cut_assert_equal_string("#<proc:command "
                          "column_remove "
                          "arguments:[table, name]"
                          ">",
                          inspected_string());
}

void
test_proc_function(void)
{
  grn_obj *proc;

  proc = get_object("geo_distance");
  inspected = grn_inspect(context, NULL, proc);
  cut_assert_equal_string("#<proc:function "
                          "geo_distance "
                          "arguments:[]"
                          ">",
                          inspected_string());
}
