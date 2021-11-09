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

#include <groonga.h>
#include <grn_db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define COORDINATE(hours, minutes, seconds) \
  GRN_TEST_GEO_COORDINATE(hours, minutes, seconds)

#define POINT(latitude_hours, latitude_minutes, latitude_seconds,       \
              longitude_hours, longitude_minutes, longitude_seconds)    \
  GRN_TEST_GEO_POINT_STRING(                                            \
    COORDINATE(latitude_hours, latitude_minutes, latitude_seconds),     \
    COORDINATE(longitude_hours, longitude_minutes, longitude_seconds))

#define TAKEN_POINT(latitude_hours, latitude_minutes, latitude_seconds, \
                    longitude_hours, longitude_minutes, longitude_seconds) \
  cut_take_string(POINT(latitude_hours, latitude_minutes, latitude_seconds, \
                        longitude_hours, longitude_minutes, longitude_seconds))

void data_prefix_error(void);
void test_prefix_error(gpointer data);
void data_prefix_short_text(void);
void test_prefix_short_text(gpointer data);
void data_prefix_geo_point(void);
void test_prefix_geo_point(gpointer data);
void data_prefix_rk(void);
void test_prefix_rk(gpointer data);
void data_near_uint32(void);
void test_near_uint32(gpointer data);
void data_near_geo_point(void);
void test_near_geo_point(gpointer data);
void data_common_prefix_search(void);
void test_common_prefix_search(gpointer data);
void data_by_id_encoded(void);
void test_by_id_encoded(gpointer data);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database, *table;
static grn_table_cursor *cursor;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "table-patricia-trie-cursor",
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

  grn_obj_close(context, database);
  grn_ctx_fin(context);
  g_free(context);
}

static void
create_short_text_table(const GList *texts)
{
  const gchar *table_name = "ShortTextPat";
  GString *command;
  const GList *node;

  assert_send_commands(
    cut_take_printf("table_create %s TABLE_PAT_KEY ShortText", table_name));

  command = g_string_new(NULL);
  g_string_append_printf(command, "load --table %s\n", table_name);
  g_string_append(command, "[\n");
  g_string_append(command, "  [\"_key\"],\n");
  for (node = texts; node; node = g_list_next(node)) {
    const gchar *text = node->data;
    g_string_append_printf(command, "  [\"%s\"]", text);
    if (g_list_next(node)) {
      g_string_append(command, ",");
    }
    g_string_append(command, "\n");
  }
  g_string_append(command, "]");
  assert_send_commands(cut_take_string(g_string_free(command, FALSE)));

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

static void
create_geo_point_table(const gchar *data)
{
  const char *table_name = "GeoPointPat";

  assert_send_command(
    cut_take_printf("table_create %s TABLE_PAT_KEY WGS84GeoPoint", table_name));
  assert_send_command(
    cut_take_printf("load --table %s\n"
                    "[\n"
                    " [\"_key\"],\n"
                    "%s"
                    "]",
                    table_name,
                    data));

  table = grn_ctx_get(context, table_name, strlen(table_name));
}

static void
cast_to_geo_point(grn_obj *geo_point, const gchar *geo_point_string)
{
  grn_obj geo_point_text;

  if (!geo_point_string) {
    return;
  }

  GRN_TEXT_INIT(&geo_point_text, 0);
  GRN_TEXT_PUTS(context, &geo_point_text, geo_point_string);
  grn_obj_cast(context, &geo_point_text, geo_point, FALSE);
  grn_obj_unlink(context, &geo_point_text);
}

void
data_prefix_error(void)
{
#define ADD_DATA(label, rc, message, offset, limit)             \
  gcut_add_datum(label,                                         \
                 "rc", G_TYPE_UINT, rc,                         \
                 "message", G_TYPE_STRING, message,             \
                 "offset", G_TYPE_INT, offset,                  \
                 "limit", G_TYPE_INT, limit,                    \
                 NULL)

  ADD_DATA("negative offset",
           GRN_TOO_SMALL_OFFSET,
           "[table][cursor][open] "
           "can't use negative offset with GRN_CURSOR_PREFIX: -1",
           -1, -1);
  ADD_DATA("large offset",
           GRN_TOO_LARGE_OFFSET,
           "[table][cursor][open] "
           "offset is not less than table size: offset:100, table_size:8",
           100, -1);
  ADD_DATA("negative limit",
           GRN_TOO_SMALL_LIMIT,
           "[table][cursor][open] "
           "can't use smaller limit than -1 with GRN_CURSOR_PREFIX: -2",
           0, -2);

#undef ADD_DATA
}

void
test_prefix_error(gpointer data)
{
  const gchar *min = "ab";
  int offset, limit;

  create_short_text_table(gcut_take_new_list_string("abra",
                                                    "abracada",
                                                    "abracadabra",
                                                    "abubu",
                                                    "あ",
                                                    "ああ",
                                                    "あああ",
                                                    "い",
                                                    NULL));

  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  cursor = grn_table_cursor_open(context, table,
                                 min, strlen(min),
                                 NULL, 0,
                                 offset, limit,
                                 GRN_CURSOR_PREFIX);
  grn_test_assert_error(gcut_data_get_uint(data, "rc"),
                        gcut_data_get_string(data, "message"),
                        context);
}

void
data_prefix_short_text(void)
{
#define ADD_DATA(label, expected, min, offset, limit, flags)    \
  gcut_add_datum(label,                                         \
                 "expected", G_TYPE_POINTER,                    \
                 expected, gcut_list_string_free,               \
                 "min", G_TYPE_STRING, min,                     \
                 "offset", G_TYPE_INT, offset,                  \
                 "limit", G_TYPE_INT, limit,                    \
                 "flags", G_TYPE_INT, flags,                    \
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
  const gchar *min;
  int offset, limit, flags;
  const GList *expected_keys;
  GList *actual_keys = NULL;

  create_short_text_table(gcut_take_new_list_string("abra",
                                                    "abracada",
                                                    "abracadabra",
                                                    "abubu",
                                                    "あ",
                                                    "ああ",
                                                    "あああ",
                                                    "い",
                                                    NULL));

  min = gcut_data_get_string(data, "min");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 min, strlen(min),
                                 NULL, 0,
                                 offset, limit,
                                 flags | GRN_CURSOR_PREFIX);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    gchar *key;
    int key_size;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys, g_strndup(key, key_size));
  }
  gcut_take_list(actual_keys, g_free);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}

static const gchar *
geo_byte_parse(const gchar *geo_byte_string)
{
    gint i = 0;
    uint8_t geo_byte[sizeof(grn_geo_point)];
    uint64_t geo_uint;
    grn_geo_point geo_point;

    while (geo_byte_string[0]) {
      switch (geo_byte_string[0]) {
      case '0':
        geo_byte[i / 8] &= ~(1 << (7 - (i % 8)));
        i++;
        break;
      case '1':
        geo_byte[i / 8] |= 1 << (7 - (i % 8));
        i++;
        break;
      default:
        break;
      }
      geo_byte_string++;
    }
    grn_memcpy(&geo_uint, geo_byte, sizeof(grn_geo_point));
    grn_ntog((uint8_t *)(&geo_point), &geo_uint, sizeof(grn_geo_point));
    return cut_take_printf("%dx%d",
                           geo_point.latitude,
                           geo_point.longitude);
}

static GList *
geo_byte_list_new_va_list(const gchar *value, va_list args)
{
  GList *list = NULL;

  while (value) {
    list = g_list_prepend(list, g_strdup(geo_byte_parse(value)));
    value = va_arg(args, const gchar *);
  }

  return g_list_reverse(list);
}

static const gchar *
geo_byte_load_data(const gchar *value, ...)
{
  GString *data;
  GList *list, *node;
  va_list args;

  va_start(args, value);
  list = geo_byte_list_new_va_list(value, args);
  va_end(args);

  data = g_string_new(NULL);
  for (node = list; node; node = g_list_next(node)) {
    const gchar *point = node->data;
    g_string_append_printf(data, "[\"%s\"]", point);
    if (g_list_next(node)) {
      g_string_append_printf(data, ",\n");
    }
  }
  gcut_list_string_free(list);

  return cut_take_string(g_string_free(data, FALSE));
}

void
data_prefix_geo_point(void)
{
#define ADD_DATA(label, expected, min, min_size, offset, limit, flags)  \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, gcut_list_string_free,                       \
                 "min", G_TYPE_STRING, min,                             \
                 "min-size", G_TYPE_UINT, min_size,                     \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 "flags", G_TYPE_INT, flags,                            \
                 NULL)

  ADD_DATA(
    "bit - ascending",
    gcut_list_string_new(
      "00000000 00111111 01010000 00000000 01111101 00010000 00011101 00001111",
      "00000000 00111111 01010000 00000001 00011110 01010001 01101001 00110000",
      "00000000 00111111 01010000 00001101 01011101 01011011 01011001 01010011",
      "00000000 00111111 01010000 00001111 00101011 00011111 00110011 00001001",
      "00000000 00111111 01010000 00010010 00110001 00001000 00001010 00110011",
      "00000000 00111111 01010000 00010010 00110001 00110111 01111000 01110000",
      "00000000 00111111 01010000 00011000 01110000 00001011 00101110 01001010",
      "00000000 00111111 01010000 00100000 00010111 01000111 00110100 00101010",
      "00000000 00111111 01010000 00100010 00100111 01000011 00000010 01101001",
      "00000000 00111111 01010000 00100010 00111011 01000000 00111000 01100100",
      "00000000 00111111 01010000 00100011 00000001 00000111 01011100 01110011",
      "00000000 00111111 01010000 00100011 00001010 00000000 00001101 00111010",
      "00000000 00111111 01010000 00100011 01100100 01011000 00000111 01110010",
      "00000000 00111111 01010000 00101101 00101000 00111111 01010110 00010110",
      "00000000 00111111 01010000 00101101 01111100 01101100 00111000 01111001",
      "00000000 00111111 01010000 00101110 01010011 00101001 00101001 00100011",
      "00000000 00111111 01010000 00101110 01110010 00111001 00011011 01101010",
      "00000000 00111111 01010000 00101111 00011000 01000110 00100101 01011110",
      "00000000 00111111 01010000 00101111 01001010 01101000 01000100 01100011",
      "00000000 00111111 01010000 00110000 01001010 01011100 01101010 00010001",
      "00000000 00111111 01010000 00111000 01100100 01101011 01111100 01111011",
      "00000000 00111111 01010000 00111001 00111101 00001001 00001011 01010011",
      "00000000 00111111 01010000 00111010 01011111 00000010 00101001 01010000",
      NULL),
    "00000000 00111111 01010000 00000000 00000000 00000000 00000000 00000000",
    26,
    0, -1,
    GRN_CURSOR_SIZE_BY_BIT);
  ADD_DATA(
    "bit - descending",
    gcut_list_string_new(
      "00000000 00111111 01010000 00111010 01011111 00000010 00101001 01010000",
      "00000000 00111111 01010000 00111001 00111101 00001001 00001011 01010011",
      "00000000 00111111 01010000 00111000 01100100 01101011 01111100 01111011",
      "00000000 00111111 01010000 00110000 01001010 01011100 01101010 00010001",
      "00000000 00111111 01010000 00101111 01001010 01101000 01000100 01100011",
      "00000000 00111111 01010000 00101111 00011000 01000110 00100101 01011110",
      "00000000 00111111 01010000 00101110 01110010 00111001 00011011 01101010",
      "00000000 00111111 01010000 00101110 01010011 00101001 00101001 00100011",
      "00000000 00111111 01010000 00101101 01111100 01101100 00111000 01111001",
      "00000000 00111111 01010000 00101101 00101000 00111111 01010110 00010110",
      "00000000 00111111 01010000 00100011 01100100 01011000 00000111 01110010",
      "00000000 00111111 01010000 00100011 00001010 00000000 00001101 00111010",
      "00000000 00111111 01010000 00100011 00000001 00000111 01011100 01110011",
      "00000000 00111111 01010000 00100010 00111011 01000000 00111000 01100100",
      "00000000 00111111 01010000 00100010 00100111 01000011 00000010 01101001",
      "00000000 00111111 01010000 00100000 00010111 01000111 00110100 00101010",
      "00000000 00111111 01010000 00011000 01110000 00001011 00101110 01001010",
      "00000000 00111111 01010000 00010010 00110001 00110111 01111000 01110000",
      "00000000 00111111 01010000 00010010 00110001 00001000 00001010 00110011",
      "00000000 00111111 01010000 00001111 00101011 00011111 00110011 00001001",
      "00000000 00111111 01010000 00001101 01011101 01011011 01011001 01010011",
      "00000000 00111111 01010000 00000001 00011110 01010001 01101001 00110000",
      "00000000 00111111 01010000 00000000 01111101 00010000 00011101 00001111",
      NULL),
    "00000000 00111111 01010000 00000000 00000000 00000000 00000000 00000000",
    26,
    0, -1,
    GRN_CURSOR_SIZE_BY_BIT | GRN_CURSOR_DESCENDING);
  ADD_DATA(
    "bit - different prefix",
    gcut_list_string_new(
      "00000000 00111101 01010101 00111101 01110000 01001011 01110011 00101100",
      NULL),
    "00000000 00111101 01010101 00111101 01110000 00000000 00000000 00000001",
    38,
    0, -1,
    GRN_CURSOR_SIZE_BY_BIT);
  ADD_DATA(
    "bit - border prefix",
    gcut_list_string_new(
      "00000000 00111101 00000101 00111101 01110000 01001011 01110011 00101100",
      NULL),
    "00000000 00111101 00000101 00111101 01110000 00000000 00000000 00000001",
    18,
    0, -1,
    GRN_CURSOR_SIZE_BY_BIT);

#undef ADD_DATA
}

void
test_prefix_geo_point(gpointer data)
{
  grn_id id;
  grn_obj min;
  int offset, limit, flags;
  unsigned min_size;
  const GList *expected_keys;
  GList *actual_keys = NULL;
  GRN_WGS84_GEO_POINT_INIT(&min, 0);

  create_geo_point_table(
    geo_byte_load_data(
      "00000000 00111111 01010000 00110000 01001010 01011100 01101010 00010001",
      "00000000 00111111 01010000 00001101 01011101 01011011 01011001 01010011",
      "00000000 00111111 01010000 00000001 00011110 01010001 01101001 00110000",
      "00000000 00111111 01010000 00011000 01110000 00001011 00101110 01001010",
      "00000000 00111111 01010000 00010010 00110001 00110111 01111000 01110000",
      "00000000 00111111 01010000 00010010 00110001 00001000 00001010 00110011",
      "00000000 00111111 01010000 00101110 01110010 00111001 00011011 01101010",
      "00000000 00111111 01010000 00101101 00101000 00111111 01010110 00010110",
      "00000000 00111111 01010000 00101111 01001010 01101000 01000100 01100011",
      "00000000 00111111 01010000 00101111 00011000 01000110 00100101 01011110",
      "00000000 00111111 01000101 01011001 01010110 00000111 00110100 01111111",
      "00000000 00111111 01000101 01010010 01100101 01100110 00010111 01111110",
      "00000000 00111111 01000101 01111111 01011011 01111101 00001001 01100001",
      "00000000 00111111 01010000 00100011 00001010 00000000 00001101 00111010",
      "00000000 00111111 01010000 00101110 01010011 00101001 00101001 00100011",
      "00000000 00111111 01010000 00111010 01011111 00000010 00101001 01010000",
      "00000000 00111111 01010000 00111001 00111101 00001001 00001011 01010011",
      "00000000 00111111 01000101 01011100 00001100 01000001 01011010 00010011",
      "00000000 00111111 01010000 00100011 00000001 00000111 01011100 01110011",
      "00000000 00111111 01010000 00100011 01100100 01011000 00000111 01110010",
      "00000000 00111111 01010000 00111000 01100100 01101011 01111100 01111011",
      "00000000 00111111 01010000 00001111 00101011 00011111 00110011 00001001",
      "00000000 00111111 01000101 01111011 01001011 01101011 00001001 00000001",
      "00000000 00111111 01000101 01011010 00110100 00000010 01111010 00000000",
      "00000000 00111111 01000101 01011011 00011010 00010111 00011000 00100000",
      "00000000 00111111 01010000 00100000 00010111 01000111 00110100 00101010",
      "00000000 00111111 01010000 00000000 01111101 00010000 00011101 00001111",
      "00000000 00111111 01000101 01000100 01010010 00100100 01100011 00111011",
      "00000000 00111111 01000101 01010001 00011100 01010110 00100110 00000110",
      "00000000 00111111 01010000 00101101 01111100 01101100 00111000 01111001",
      "00000000 00111111 01000101 01001101 00111110 00000101 00101010 01000101",
      "00000000 00111111 01000101 01000100 01111100 01101011 01101111 00010101",
      "00000000 00111111 01000101 01110111 01010100 01110100 01111000 01111000",
      "00000000 00111111 01010000 00100010 00111011 01000000 00111000 01100100",
      "00000000 00111111 01010000 00100010 00100111 01000011 00000010 01101001",
      "00000000 00111111 01000101 01011100 00110110 00100010 00111000 01100001",
      "00000000 00111101 01010101 00111101 01110000 01001011 01110011 00101100",
      "00000000 00111101 00000101 00111101 01110000 01001011 01110011 00101100",
      NULL));

  cast_to_geo_point(&min, geo_byte_parse(gcut_data_get_string(data, "min")));

  min_size = gcut_data_get_uint(data, "min-size");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 GRN_BULK_HEAD(&min), min_size,
                                 NULL, 0,
                                 offset, limit,
                                 flags | GRN_CURSOR_PREFIX);
  grn_obj_unlink(context, &min);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    grn_geo_point *key;
    size_t i;
    int j, key_size;
    uint8_t encoded_key[sizeof(grn_geo_point)];
    GString *geo_byte;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    grn_gton(encoded_key, key, key_size);
    geo_byte = g_string_new(NULL);
    for (i = 0; i < sizeof(grn_geo_point); i++) {
      if (i != 0) {
        g_string_append(geo_byte, " ");
      }
      for (j = 0; j < 8; j++) {
        g_string_append_printf(geo_byte, "%d", (encoded_key[i] >> (7 - j)) & 1);
      }
    }
    actual_keys = g_list_append(actual_keys, g_string_free(geo_byte, FALSE));
  }
  gcut_take_list(actual_keys, g_free);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}

#define ADD_DATA(label, expected, min, offset, limit)                   \
  gcut_add_datum(label " - [" min "]",                                  \
                 "expected", G_TYPE_POINTER,                            \
                 expected, gcut_list_string_free,                       \
                 "min", G_TYPE_STRING, min,                             \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 NULL)

static void
data_prefix_rk_basic(void)
{
  ADD_DATA("roman - 1byte",
           gcut_list_string_new("カネソナエタ",
                                "カノウ",
                                "キノウ",
                                "キョウカ",
                                "クミコミ",
                                "クミコム",
                                "ケンサク",
                                "ケンサクヨウキュウ",
                                "コウセイド",
                                "コウソク",
                                "コンパクト",
                                NULL),
           "k",
           0, -1);
  ADD_DATA("ひらがな",
           gcut_list_string_new("コウセイド",
                                "コウソク",
                                NULL),
           "こう",
           0, -1);
  ADD_DATA("カタカナ",
           gcut_list_string_new("コウセイド",
                                "コウソク",
                                NULL),
           "コウ",
           0, -1);
  ADD_DATA("ひらがな and カタカナ",
           gcut_list_string_new("コウセイド",
                                "コウソク",
                                NULL),
           "こウ",
           0, -1);
}

static void
data_prefix_rk_xtsu(void)
{
  ADD_DATA("roman - ッ - full",
           gcut_list_string_new("インデックス",
                                NULL),
           "indekk",
           0, -1);
  ADD_DATA("roman - ッ - half",
           gcut_list_string_new("インデックス",
                                NULL),
           "indek",
           0, -1);
  ADD_DATA("roman - ック - half",
           gcut_list_string_new("インデックス",
                                NULL),
           "indekk",
           0, -1);
  ADD_DATA("roman - xtu - half",
           gcut_list_string_new("インデックス",
                                NULL),
           "indextu",
           0, -1);
  ADD_DATA("roman - xtsu - half",
           gcut_list_string_new("インデックス",
                                NULL),
           "indextsu",
           0, -1);
  ADD_DATA("roman - ltu - half",
           gcut_list_string_new("インデックス",
                                NULL),
           "indeltu",
           0, -1);
  ADD_DATA("roman - ltsu - half",
           gcut_list_string_new("インデックス",
                                NULL),
           "indeltsu",
           0, -1);
  ADD_DATA("ひらがな - ッ",
           gcut_list_string_new("インデックス",
                                NULL),
           "いんでっ",
           0, -1);
  ADD_DATA("カタカナ - ッ",
           gcut_list_string_new("インデックス",
                                NULL),
           "インデッ",
           0, -1);
}

static void
data_prefix_rk_xyu(void)
{
  ADD_DATA("roman - ュ - full",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "youkyu",
           0, -1);
  ADD_DATA("roman - ュ - x",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "youkix",
           0, -1);
  ADD_DATA("roman - ュ - xy",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "youkixy",
           0, -1);
  ADD_DATA("roman - ュ - xyu",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "youkixyu",
           0, -1);
  ADD_DATA("roman - ュ - ly",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "youkily",
           0, -1);
  ADD_DATA("roman - ュ - lyu",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "youkilyu",
           0, -1);
  ADD_DATA("ひらがな - ュ",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "ようきゅ",
           0, -1);
  ADD_DATA("カタカナ - ュ",
           gcut_list_string_new("ヨウキュウ",
                                NULL),
           "ヨウキュ",
           0, -1);
}

static void
data_prefix_rk_offset_and_limit(void)
{
  ADD_DATA("offset",
           gcut_list_string_new("キノウ",
                                "キョウカ",
                                "クミコミ",
                                "クミコム",
                                "ケンサクヨウキュウ",
                                "コウセイド",
                                "コウソク",
                                "コンパクト",
                                NULL),
           "k",
           3, -1);
  ADD_DATA("limit",
           gcut_list_string_new("カネソナエタ",
                                "カノウ",
                                "ケンサク",
                                NULL),
           "k",
           0, 3);
  ADD_DATA("offset - limit",
           gcut_list_string_new("キノウ",
                                "キョウカ",
                                "ケンサクヨウキュウ",
                                "コウセイド",
                                "コウソク",
                                NULL),
           "k",
           3, 5);
}

static void
data_prefix_rk_no_match(void)
{
  ADD_DATA("roman - no match",
           NULL,
           "kumikomuy",
           0, -1);
  ADD_DATA("roman - upcase - no match",
           NULL,
           "K",
           0, -1);
  ADD_DATA("ひらがな - no match",
           NULL,
           "くみこむよ",
           0, -1);
  ADD_DATA("カタカナ - no match",
           NULL,
           "クミコムヨ",
           0, -1);
}

void
data_prefix_rk(void)
{
  data_prefix_rk_basic();
  data_prefix_rk_xtsu();
  data_prefix_rk_xyu();
  data_prefix_rk_offset_and_limit();
  data_prefix_rk_no_match();
}
#undef ADD_DATA

void
test_prefix_rk(gpointer data)
{
  grn_id id;
  const gchar *min;
  int offset, limit;
  const GList *expected_keys;
  GList *actual_keys = NULL;

  create_short_text_table(
    gcut_take_new_list_string("インデックス",
                              "エヌグラム",
                              "エンジン",
                              "カネソナエタ",
                              "カノウ",
                              "キノウ",
                              "キョウカ",
                              "クミコミ",
                              "クミコム",
                              "グルンガ",
                              "ケンサク",
                              "ケンサクヨウキュウ",
                              "ゲンゴ",
                              "コウセイド",
                              "コウソク",
                              "コンパクト",
                              "サクセイ",
                              "ショリ",
                              "ショリケイ",
                              "ジッソウ",
                              "ジュンスイ",
                              "スクリプト",
                              "セッケイ",
                              "ゼンブン",
                              "タイプ",
                              "タンゴ",
                              "ダイキボ",
                              "テンチ",
                              "ディービーエムエス",
                              "トウ",
                              "トクチョウ",
                              "ブンショリョウ",
                              "ヨウキュウ",
                              NULL));

  min = gcut_data_get_string(data, "min");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  cursor = grn_table_cursor_open(context, table,
                                 min, strlen(min),
                                 NULL, 0,
                                 offset, limit,
                                 GRN_CURSOR_PREFIX | GRN_CURSOR_RK);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    gchar *key;
    int key_size;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys, g_strndup(key, key_size));
  }
  actual_keys = g_list_sort(actual_keys, (GCompareFunc)strcmp);
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
#define ADD_DATA(label, expected, min_size, max, offset, limit, flags)  \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, g_list_free,                                 \
                 "min-size", G_TYPE_INT, min_size,                      \
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
  int min_size, offset, limit, flags;
  guint32 max;
  const GList *expected_keys;
  GList *actual_keys = NULL;

  create_uint32_table();

  min_size = gcut_data_get_int(data, "min-size");
  max = gcut_data_get_uint(data, "max");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 NULL, min_size,
                                 &max, sizeof(max),
                                 offset, limit,
                                 flags | GRN_CURSOR_PREFIX);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    guint32 *key;
    int key_size;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys, GUINT_TO_POINTER(*key));
  }
  gcut_take_list(actual_keys, NULL);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_uint(expected_keys, actual_keys);
}

void
data_near_geo_point(void)
{
#define ADD_DATA(label, expected, min_size, max, offset, limit, flags)  \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, gcut_list_string_free,                       \
                 "min-size", G_TYPE_UINT, min_size,                     \
                 "max", G_TYPE_STRING, max,                             \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 "flags", G_TYPE_INT, flags,                            \
                 NULL)

  ADD_DATA("no limit",
           gcut_list_string_new(TAKEN_POINT(1, 2, 3,
                                            4, 5, 6),
                                TAKEN_POINT(1, 2, 3,
                                            7, 8, 9),
                                TAKEN_POINT(7, 8, 9,
                                            4, 5, 6),
                                TAKEN_POINT(88, 58, 58,
                                            178, 58, 58),
                                TAKEN_POINT(89, 59, 59,
                                            179, -59, -59),
                                TAKEN_POINT(89, 59, 59,
                                            179, 59, 59),
                                TAKEN_POINT(-89, -59, -59,
                                            179, 59, 59),
                                TAKEN_POINT(-89, -59, -59,
                                            -179, -59, -59),
                                TAKEN_POINT(-88, -58, -58,
                                            -178, -58, -58),
                                NULL),
           0,
           TAKEN_POINT(0, 0, 0,
                       0, 0, 0),
           0, -1,
           0);
  ADD_DATA("min-size",
           gcut_list_string_new(TAKEN_POINT(1, 2, 3,
                                            4, 5, 6),
                                TAKEN_POINT(1, 2, 3,
                                            7, 8, 9),
                                TAKEN_POINT(7, 8, 9,
                                            4, 5, 6),
                                NULL),
           1,
           TAKEN_POINT(0, 0, 0,
                       0, 0, 0),
           0, -1,
           0);

#undef ADD_DATA
}

void
test_near_geo_point(gpointer data)
{
  grn_id id;
  int min_size, offset, limit, flags;
  grn_obj max;
  const GList *expected_keys;
  GList *actual_keys = NULL;
  GRN_WGS84_GEO_POINT_INIT(&max, 0);

  create_geo_point_table(
    cut_take_printf(" [\"%s\"],"
                    " [\"%s\"],"
                    " [\"%s\"],"
                    " [\"%s\"],"
                    " [\"%s\"],"
                    " [\"%s\"],"
                    " [\"%s\"],"
                    " [\"%s\"],"
                    " [\"%s\"]",
                    TAKEN_POINT(1, 2, 3,
                                4, 5, 6),
                    TAKEN_POINT(1, 2, 3,
                                7, 8, 9),
                    TAKEN_POINT(7, 8, 9,
                                4, 5, 6),
                    TAKEN_POINT(89, 59, 59,
                                179, 59, 59),
                    TAKEN_POINT(89, 59, 59,
                                179, -59, -59),
                    TAKEN_POINT(88, 58, 58,
                                178, 58, 58),
                    TAKEN_POINT(-89, -59, -59,
                                -179, -59, -59),
                    TAKEN_POINT(-89, -59, -59,
                                179, 59, 59),
                    TAKEN_POINT(-88, -58, -58,
                                -178, -58, -58)));

  min_size = gcut_data_get_int(data, "min-size");
  cast_to_geo_point(&max, gcut_data_get_string(data, "max"));
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 NULL, min_size,
                                 GRN_BULK_HEAD(&max), GRN_BULK_VSIZE(&max),
                                 offset, limit,
                                 flags | GRN_CURSOR_PREFIX);
  grn_obj_unlink(context, &max);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    grn_geo_point *key;
    int key_size;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys,
                                g_strdup_printf("%dx%d",
                                                key->latitude,
                                                key->longitude));
  }
  gcut_take_list(actual_keys, g_free);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}

void
data_common_prefix_search(void)
{
#define ADD_DATA(label, expected, min_size, max, offset, limit, flags)  \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, gcut_list_string_free,                       \
                 "min-size", G_TYPE_INT, min_size,                      \
                 "max", G_TYPE_STRING, max,                             \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 "flags", G_TYPE_INT, flags,                            \
                 NULL)

  ADD_DATA("alphabet",
           gcut_list_string_new("abracada", "abra",
                                NULL),
           0, "abracada",
           0, -1,
           0);
  ADD_DATA("alphabet - min size",
           gcut_list_string_new("abracadabra", "abracada",
                                NULL),
           5, "abracadabra",
           0, -1,
           0);
  ADD_DATA("alphabet - offset and limit",
           gcut_list_string_new("abra", NULL),
           0, "abracadabra",
           2, 1,
           0);
  ADD_DATA("no match",
           NULL,
           0, "bubuzera",
           0, -1,
           0);
  ADD_DATA("no match - common prefix",
           NULL,
           0, "aburaura",
           0, -1,
           0);
  ADD_DATA("empty key",
           gcut_list_string_new("abra", "abracada", "abracadabra", "abubu",
                                "あ", "ああ", "あああ", "い",
                                NULL),
           0, "",
           0, -1,
           0);
  {
    gchar *long_key;
    long_key = g_alloca(GRN_TABLE_MAX_KEY_SIZE + 2);
    memset(long_key, 'a', GRN_TABLE_MAX_KEY_SIZE + 1);
    ADD_DATA("long key",
             NULL,
             0, long_key,
             0, -1,
             0);
  }

#undef ADD_DATA
}

void
test_common_prefix_search(gpointer data)
{
  grn_id id;
  const gchar *max;
  int min_size, offset, limit, flags;
  const GList *expected_keys;
  GList *actual_keys = NULL;

  create_short_text_table(gcut_take_new_list_string("abra",
                                                    "abracada",
                                                    "abracadabra",
                                                    "abubu",
                                                    "あ",
                                                    "ああ",
                                                    "あああ",
                                                    "い",
                                                    NULL));

  min_size = gcut_data_get_int(data, "min-size");
  max = gcut_data_get_string(data, "max");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 NULL, min_size,
                                 max, strlen(max),
                                 offset, limit,
                                 flags | GRN_CURSOR_PREFIX);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    gchar *key;
    int key_size;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys, g_strndup(key, key_size));
  }
  gcut_take_list(actual_keys, g_free);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}

void
data_by_id_encoded(void)
{
#define ADD_DATA(label, expected, min, min_size, max, max_size,         \
                 offset, limit, flags)                                  \
  gcut_add_datum(label,                                                 \
                 "expected", G_TYPE_POINTER,                            \
                 expected, gcut_list_string_free,                       \
                 "min", G_TYPE_STRING, min,                             \
                 "min-size", G_TYPE_UINT, min_size,                     \
                 "max", G_TYPE_STRING, max,                             \
                 "max-size", G_TYPE_UINT, max_size,                     \
                 "offset", G_TYPE_INT, offset,                          \
                 "limit", G_TYPE_INT, limit,                            \
                 "flags", G_TYPE_INT, flags,                            \
                 NULL)

  ADD_DATA("ascending",
           gcut_list_string_new("128592911x503145263",
                                "128597458x502942345",
                                "128572751x502866155",
                                "128513714x503319780",
                                "128320340x502334363",
                                NULL),
           NULL, 0,
           NULL, 0,
           0, -1,
           0);
  ADD_DATA("descending",
           gcut_list_string_new("128320340x502334363",
                                "128513714x503319780",
                                "128572751x502866155",
                                "128597458x502942345",
                                "128592911x503145263",
                                NULL),
           NULL, 0,
           NULL, 0,
           0, -1,
           GRN_CURSOR_DESCENDING);
  ADD_DATA("ascending - offset",
           gcut_list_string_new("128572751x502866155",
                                "128513714x503319780",
                                "128320340x502334363",
                                NULL),
           NULL, 0,
           NULL, 0,
           2, -1,
           0);
  ADD_DATA("descending - offset",
           gcut_list_string_new("128572751x502866155",
                                "128597458x502942345",
                                "128592911x503145263",
                                NULL),
           NULL, 0,
           NULL, 0,
           2, -1,
           GRN_CURSOR_DESCENDING);

#undef ADD_DATA
}

void
test_by_id_encoded(gpointer data)
{
  grn_id id;
  grn_obj min, max;
  unsigned min_size, max_size;
  int offset, limit, flags;
  const GList *expected_keys;
  GList *actual_keys = NULL;
  GRN_WGS84_GEO_POINT_INIT(&min, 0);
  GRN_WGS84_GEO_POINT_INIT(&max, 0);

  create_geo_point_table("[\"128592911x503145263\"],\n"
                         "[\"128565076x502976128\"],\n"
                         "[\"128597458x502942345\"],\n"
                         "[\"128572751x502866155\"],\n"
                         "[\"128521858x503341754\"],\n"
                         "[\"128513714x503319780\"],\n"
                         "[\"128534177x502693614\"],\n"
                         "[\"128320340x502334363\"]\n");
  assert_send_command("delete GeoPointPat \"128565076x502976128\"");
  assert_send_command("delete GeoPointPat \"128521858x503341754\"");
  assert_send_command("delete GeoPointPat \"128534177x502693614\"");

  cast_to_geo_point(&min, gcut_data_get_string(data, "min"));
  min_size = gcut_data_get_uint(data, "min-size");
  cast_to_geo_point(&max, gcut_data_get_string(data, "max"));
  max_size = gcut_data_get_uint(data, "max-size");
  offset = gcut_data_get_int(data, "offset");
  limit = gcut_data_get_int(data, "limit");
  flags = gcut_data_get_int(data, "flags");
  cursor = grn_table_cursor_open(context, table,
                                 min_size > 0 ? GRN_BULK_HEAD(&min) : NULL,
                                 min_size,
                                 max_size > 0 ? GRN_BULK_HEAD(&max) : NULL,
                                 max_size,
                                 offset, limit,
                                 flags | GRN_CURSOR_BY_ID);
  grn_obj_unlink(context, &min);
  grn_obj_unlink(context, &max);
  grn_test_assert_context(context);
  while ((id = grn_table_cursor_next(context, cursor))) {
    grn_geo_point *key;
    int key_size;

    key_size = grn_table_cursor_get_key(context, cursor, (void **)&key);
    actual_keys = g_list_append(actual_keys,
                                g_strdup_printf("%dx%d",
                                                key->latitude,
                                                key->longitude));
  }
  gcut_take_list(actual_keys, g_free);

  expected_keys = gcut_data_get_pointer(data, "expected");
  gcut_assert_equal_list_string(expected_keys, actual_keys);
}
