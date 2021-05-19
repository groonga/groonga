/*
  Copyright (C) 2009-2011  Kouhei Sutou <kou@clear-code.com>

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

#include <grn_db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void data_text_to_bool(void);
void test_text_to_bool(gconstpointer data);
void test_text_to_int8(void);
void test_text_to_uint8(void);
void test_text_to_int16(void);
void test_text_to_uint16(void);
void test_text_to_int32(void);
void test_text_to_uint32(void);
void test_text_to_int64(void);
void test_text_to_uint64(void);
void test_text_to_float(void);
void test_text_to_time(void);
void test_text_to_geo_point(void);
void test_text_to_geo_point_empty(void);
void test_text_to_geo_point_garbage(void);
void test_text_to_geo_point_comma(void);
void test_text_to_geo_point_invalid(void);
void test_text_to_geo_point_in_degree(void);
void test_text_to_geo_point_in_degree_comma(void);
void test_text_to_geo_point_in_degree_invalid(void);
void test_text_to_geo_point_mixed(void);
void test_text_to_geo_point_mixed_comma(void);
void test_text_to_geo_point_mixed_invalid(void);

void data_text_error(void);
void test_text_error(gconstpointer data);

void test_bool_to_bool(void);

void data_int32_to_bool(void);
void test_int32_to_bool(gconstpointer data);
void test_int32_to_int8(void);
void test_int32_to_uint8(void);
void test_int32_to_int16(void);
void test_int32_to_uint16(void);
void test_int32_to_int32(void);
void test_int32_to_uint32(void);
void test_int32_to_int64(void);
void test_int32_to_uint64(void);
void test_int32_to_float(void);
void test_int32_to_time(void);
void test_int32_to_geo_point_zero(void);
void test_int32_to_geo_point_invalid(void);

void data_uint32_to_bool(void);
void test_uint32_to_bool(gconstpointer data);
void test_uint32_to_int8(void);
void test_uint32_to_uint8(void);
void test_uint32_to_int16(void);
void test_uint32_to_uint16(void);
void test_uint32_to_int32(void);
void test_uint32_to_uint32(void);
void test_uint32_to_int64(void);
void test_uint32_to_uint64(void);
void test_uint32_to_float(void);
void test_uint32_to_time(void);

void data_int64_to_bool(void);
void test_int64_to_bool(gconstpointer data);
void test_int64_to_int8(void);
void test_int64_to_uint8(void);
void test_int64_to_int16(void);
void test_int64_to_uint16(void);
void test_int64_to_int32(void);
void test_int64_to_uint32(void);
void test_int64_to_int64(void);
void test_int64_to_uint64(void);
void test_int64_to_float(void);
void test_int64_to_time(void);

void data_uint64_to_bool(void);
void test_uint64_to_bool(gconstpointer data);
void test_uint64_to_int8(void);
void test_uint64_to_uint8(void);
void test_uint64_to_int16(void);
void test_uint64_to_uint16(void);
void test_uint64_to_int32(void);
void test_uint64_to_uint32(void);
void test_uint64_to_int64(void);
void test_uint64_to_uint64(void);
void test_uint64_to_float(void);
void test_uint64_to_time(void);

void test_tokyo_geo_point_to_tokyo_geo_point(void);
void test_tokyo_geo_point_to_wgs84_geo_point(void);
void test_wgs84_geo_point_to_wgs84_geo_point(void);
void test_wgs84_geo_point_to_tokyo_geo_point(void);

static grn_logger_info *logger;
static grn_ctx context;
static grn_obj src, dest;

void
cut_setup(void)
{
  logger = setup_grn_logger();
  grn_ctx_init(&context, 0);
  GRN_VOID_INIT(&src);
  GRN_VOID_INIT(&dest);
}

void
cut_teardown(void)
{
  grn_obj_remove(&context, &src);
  grn_obj_remove(&context, &dest);
  grn_ctx_fin(&context);
  teardown_grn_logger(logger);
}

static void
set_text(const gchar *text)
{
  grn_obj_reinit(&context, &src, GRN_DB_TEXT, 0);
  if (text) {
    GRN_TEXT_PUTS(&context, &src, text);
  }
}

static void
cast_text(const gchar *text)
{
  set_text(text);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
data_text_to_bool(void)
{
#define ADD_DATA(label, expected, text)                 \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_UINT, expected,     \
                 "text", G_TYPE_STRING, text,           \
                 NULL)

  ADD_DATA("empty", GRN_FALSE, "");
  ADD_DATA("NULL", GRN_FALSE, NULL);
  ADD_DATA("true", GRN_TRUE, "true");
  ADD_DATA("false", GRN_TRUE, "false");

#undef ADD_DATA
}

void
test_text_to_bool(gconstpointer data)
{
  grn_obj_reinit(&context, &dest, GRN_DB_BOOL, 0);
  cast_text(gcut_data_get_string(data, "text"));
  cut_assert_equal_boolean(gcut_data_get_uint(data, "expected"),
                           GRN_BOOL_VALUE(&dest));
}

void
test_text_to_int8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT8, 0);
  cast_text("-29");
  cut_assert_equal_int(-29, GRN_INT8_VALUE(&dest));
}

void
test_text_to_uint8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT8, 0);
  cast_text("29");
  cut_assert_equal_uint(29, GRN_UINT8_VALUE(&dest));
}

void
test_text_to_int16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT16, 0);
  cast_text("-2929");
  cut_assert_equal_int(-2929, GRN_INT16_VALUE(&dest));
}

void
test_text_to_uint16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT16, 0);
  cast_text("2929");
  cut_assert_equal_uint(2929, GRN_UINT16_VALUE(&dest));
}

void
test_text_to_int32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT32, 0);
  cast_text("-29292929");
  cut_assert_equal_int(-29292929, GRN_INT32_VALUE(&dest));
}

void
test_text_to_uint32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT32, 0);
  cast_text("29292929");
  cut_assert_equal_uint(29292929, GRN_UINT32_VALUE(&dest));
}

void
test_text_to_int64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT64, 0);
  cast_text("-2929292929292929");
  gcut_assert_equal_int64(G_GINT64_CONSTANT(-2929292929292929),
                          GRN_INT64_VALUE(&dest));
}

void
test_text_to_uint64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT64, 0);
  cast_text("2929292929292929");
  gcut_assert_equal_uint64(G_GUINT64_CONSTANT(2929292929292929),
                           GRN_UINT64_VALUE(&dest));
}

void
test_text_to_float(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_FLOAT, 0);
  cast_text("29.029");
  cut_assert_equal_double(29.029, 0.001, GRN_FLOAT_VALUE(&dest));
}

void
test_text_to_time(void)
{
  long long int sec, usec;

  grn_obj_reinit(&context, &dest, GRN_DB_TIME, 0);
  cast_text("2009/11/24 05:52:10.02929");
  GRN_TIME_UNPACK(GRN_TIME_VALUE(&dest), sec, usec);
  cut_assert_equal_int(1259009530, sec);
  cut_assert_equal_int(29290, usec);
}

void
test_text_to_geo_point(void)
{
  gint takane_latitude, takane_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  cast_text("130194581x503802073");
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(130194581, takane_latitude);
  cut_assert_equal_int(503802073, takane_longitude);
}

void
test_text_to_geo_point_empty(void)
{
  gint empty_latitude, empty_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  cast_text("");
  GRN_GEO_POINT_VALUE(&dest, empty_latitude, empty_longitude);
  cut_assert_equal_int(0, empty_latitude);
  cut_assert_equal_int(0, empty_longitude);
}

void
test_text_to_geo_point_garbage(void)
{
  gint takane_latitude, takane_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
#define GEO_TEXT "130194581x503802073"
  set_text(GEO_TEXT ".0");
  cast_text(GEO_TEXT);
#undef GEO_TEXT
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(130194581, takane_latitude);
  cut_assert_equal_int(503802073, takane_longitude);
}

void
test_text_to_geo_point_comma(void)
{
  gint takane_latitude, takane_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  cast_text("130194581,503802073");
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(130194581, takane_latitude);
  cut_assert_equal_int(503802073, takane_longitude);
}

void
test_text_to_geo_point_invalid(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  set_text("130194581?503802073");
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_obj_cast(&context, &src, &dest, FALSE));
}

void
test_text_to_geo_point_in_degree(void)
{
  gint takane_latitude, takane_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  cast_text("35.6954581363924x139.564207350021");
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(128503649, takane_latitude);
  cut_assert_equal_int(502431146, takane_longitude);
}

void
test_text_to_geo_point_in_degree_comma(void)
{
  gint takane_latitude, takane_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  cast_text("35.6954581363924,139.564207350021");
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(128503649, takane_latitude);
  cut_assert_equal_int(502431146, takane_longitude);
}

void
test_text_to_geo_point_in_degree_invalid(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  set_text("35.6954581363924?139.564207350021");
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_obj_cast(&context, &src, &dest, FALSE));
}

void
test_text_to_geo_point_mixed(void)
{
  gint takane_latitude, takane_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  cast_text("35.6954581363924x503802073");
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(128503649, takane_latitude);
  cut_assert_equal_int(503802073, takane_longitude);
}

void
test_text_to_geo_point_mixed_comma(void)
{
  gint takane_latitude, takane_longitude;

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  cast_text("35.6954581363924,503802073");
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(128503649, takane_latitude);
  cut_assert_equal_int(503802073, takane_longitude);
}

void
test_text_to_geo_point_mixed_invalid(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  set_text("35.6954581363924x503802073garbage");
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_obj_cast(&context, &src, &dest, FALSE));
}

void
data_text_error(void)
{
#define ADD_DATA(label, expected, type, text)           \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_UINT, expected,     \
                 "type", G_TYPE_UINT, type,             \
                 "text", G_TYPE_STRING, text,           \
                 NULL)

  ADD_DATA("not numeric", GRN_INVALID_ARGUMENT, GRN_DB_INT32, "not-numeric");

#undef ADD_DATA
}

void
test_text_error(gconstpointer data)
{
  grn_obj_reinit(&context, &dest, gcut_data_get_uint(data, "type"), 0);
  grn_obj_reinit(&context, &src, GRN_DB_TEXT, 0);
  GRN_TEXT_PUTS(&context, &src, gcut_data_get_string(data, "text"));
  grn_test_assert_equal_rc(gcut_data_get_uint(data, "expected"),
                           grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

static void
set_bool(gboolean boolean)
{
  grn_obj_reinit(&context, &src, GRN_DB_BOOL, 0);
  GRN_BOOL_SET(&context, &src, boolean);
}

static void
cast_bool(gboolean boolean)
{
  set_bool(boolean);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
test_bool_to_bool(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_BOOL, 0);
  cast_bool(TRUE);
  cut_assert_true(GRN_BOOL_VALUE(&dest));
}

static void
set_int32(gint32 number)
{
  grn_obj_reinit(&context, &src, GRN_DB_INT32, 0);
  GRN_INT32_SET(&context, &src, number);
}

static void
cast_int32(gint32 number)
{
  set_int32(number);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
data_int32_to_bool(void)
{
#define ADD_DATA(label, expected, number)               \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_UINT, expected,     \
                 "number", G_TYPE_INT, number,          \
                 NULL)

  ADD_DATA("true", GRN_TRUE, 1);
  ADD_DATA("false", GRN_FALSE, 0);
  ADD_DATA("not zero", GRN_TRUE, 100);

#undef ADD_DATA
}

void
test_int32_to_bool(gconstpointer data)
{
  grn_obj_reinit(&context, &dest, GRN_DB_BOOL, 0);
  cast_int32(gcut_data_get_int(data, "number"));
  cut_assert_equal_boolean(gcut_data_get_uint(data, "expected"),
                           GRN_BOOL_VALUE(&dest));
}

void
test_int32_to_int8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT8, 0);
  cast_int32(-29);
  cut_assert_equal_int(-29, GRN_INT8_VALUE(&dest));
}

void
test_int32_to_uint8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT8, 0);
  cast_int32(29);
  cut_assert_equal_uint(29, GRN_UINT8_VALUE(&dest));
}

void
test_int32_to_int16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT16, 0);
  cast_int32(-2929);
  cut_assert_equal_int(-2929, GRN_INT16_VALUE(&dest));
}

void
test_int32_to_uint16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT16, 0);
  cast_int32(2929);
  cut_assert_equal_uint(2929, GRN_UINT16_VALUE(&dest));
}

void
test_int32_to_int32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT32, 0);
  cast_int32(-29292929);
  cut_assert_equal_int(-29292929, GRN_INT32_VALUE(&dest));
}

void
test_int32_to_uint32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT32, 0);
  cast_int32(29292929);
  cut_assert_equal_uint(29292929, GRN_UINT32_VALUE(&dest));
}

void
test_int32_to_int64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT64, 0);
  cast_int32(-29292929);
  gcut_assert_equal_int64(G_GINT64_CONSTANT(-29292929),
                          GRN_INT64_VALUE(&dest));
}

void
test_int32_to_uint64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT64, 0);
  cast_int32(29292929);
  gcut_assert_equal_uint64(G_GUINT64_CONSTANT(29292929),
                           GRN_UINT64_VALUE(&dest));
}

void
test_int32_to_float(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_FLOAT, 0);
  cast_int32(29);
  cut_assert_equal_double(29, 0.1, GRN_FLOAT_VALUE(&dest));
}

void
test_int32_to_time(void)
{
  long long int sec, usec;

  grn_obj_reinit(&context, &dest, GRN_DB_TIME, 0);
  cast_int32(1259009530);
  GRN_TIME_UNPACK(GRN_TIME_VALUE(&dest), sec, usec);
  cut_assert_equal_int(1259009530, sec);
  cut_assert_equal_int(0, usec);
}

void
test_int32_to_text(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_TEXT, 0);
  cast_int32(2929);
  cut_assert_equal_string("2929", GRN_TEXT_VALUE(&dest));
}

void
test_int32_to_geo_point_zero(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  set_int32(0);
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
test_int32_to_geo_point_invalid(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  set_int32(1);
  grn_test_assert_equal_rc(GRN_INVALID_ARGUMENT,
                           grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

static void
cast_uint32(guint32 number)
{
  grn_obj_reinit(&context, &src, GRN_DB_UINT32, 0);
  GRN_UINT32_SET(&context, &src, number);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
data_uint32_to_bool(void)
{
#define ADD_DATA(label, expected, number)               \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_UINT, expected,     \
                 "number", G_TYPE_UINT, number,         \
                 NULL)

  ADD_DATA("true", GRN_TRUE, 1);
  ADD_DATA("false", GRN_FALSE, 0);
  ADD_DATA("not zero", GRN_TRUE, 100);

#undef ADD_DATA
}

void
test_uint32_to_bool(gconstpointer data)
{
  grn_obj_reinit(&context, &dest, GRN_DB_BOOL, 0);
  cast_uint32(gcut_data_get_uint(data, "number"));
  cut_assert_equal_boolean(gcut_data_get_uint(data, "expected"),
                           GRN_BOOL_VALUE(&dest));
}

void
test_uint32_to_int8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT8, 0);
  cast_uint32(29);
  cut_assert_equal_int(29, GRN_UINT8_VALUE(&dest));
}

void
test_uint32_to_uint8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT8, 0);
  cast_uint32(29);
  cut_assert_equal_uint(29, GRN_UINT8_VALUE(&dest));
}

void
test_uint32_to_int16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT16, 0);
  cast_uint32(2929);
  cut_assert_equal_int(2929, GRN_UINT16_VALUE(&dest));
}

void
test_uint32_to_uint16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT16, 0);
  cast_uint32(2929);
  cut_assert_equal_uint(2929, GRN_UINT16_VALUE(&dest));
}

void
test_uint32_to_int32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT32, 0);
  cast_uint32(29292929);
  cut_assert_equal_int(29292929, GRN_INT32_VALUE(&dest));
}

void
test_uint32_to_uint32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT32, 0);
  cast_uint32(29292929);
  cut_assert_equal_uint(29292929, GRN_UINT32_VALUE(&dest));
}

void
test_uint32_to_int64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT64, 0);
  cast_uint32(29292929);
  gcut_assert_equal_int64(G_GINT64_CONSTANT(29292929),
                          GRN_INT64_VALUE(&dest));
}

void
test_uint32_to_uint64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT64, 0);
  cast_uint32(29292929);
  gcut_assert_equal_uint64(G_GUINT64_CONSTANT(29292929),
                           GRN_UINT64_VALUE(&dest));
}

void
test_uint32_to_float(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_FLOAT, 0);
  cast_uint32(29);
  cut_assert_equal_double(29, 0.1, GRN_FLOAT_VALUE(&dest));
}

void
test_uint32_to_time(void)
{
  long long int sec, usec;

  grn_obj_reinit(&context, &dest, GRN_DB_TIME, 0);
  cast_uint32(1259009530);
  GRN_TIME_UNPACK(GRN_TIME_VALUE(&dest), sec, usec);
  cut_assert_equal_int(1259009530, sec);
  cut_assert_equal_int(0, usec);
}

void
test_uint32_to_text(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_TEXT, 0);
  cast_uint32(2929);
  cut_assert_equal_string("2929", GRN_TEXT_VALUE(&dest));
}

static void
cast_int64(gint64 number)
{
  grn_obj_reinit(&context, &src, GRN_DB_INT64, 0);
  GRN_INT64_SET(&context, &src, number);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
data_int64_to_bool(void)
{
#define ADD_DATA(label, expected, number)               \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_UINT, expected,     \
                 "number", G_TYPE_INT, number,          \
                 NULL)

  ADD_DATA("true", GRN_TRUE, 1);
  ADD_DATA("false", GRN_FALSE, 0);
  ADD_DATA("not zero", GRN_TRUE, 100);

#undef ADD_DATA
}

void
test_int64_to_bool(gconstpointer data)
{
  grn_obj_reinit(&context, &dest, GRN_DB_BOOL, 0);
  cast_int64(gcut_data_get_int(data, "number"));
  cut_assert_equal_boolean(gcut_data_get_uint(data, "expected"),
                           GRN_BOOL_VALUE(&dest));
}

void
test_int64_to_int8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT8, 0);
  cast_int64(-29);
  cut_assert_equal_int(-29, GRN_INT8_VALUE(&dest));
}

void
test_int64_to_uint8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT8, 0);
  cast_int64(29);
  cut_assert_equal_uint(29, GRN_UINT8_VALUE(&dest));
}

void
test_int64_to_int16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT16, 0);
  cast_int64(-2929);
  cut_assert_equal_int(-2929, GRN_INT16_VALUE(&dest));
}

void
test_int64_to_uint16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT16, 0);
  cast_int64(2929);
  cut_assert_equal_uint(2929, GRN_UINT16_VALUE(&dest));
}

void
test_int64_to_int32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT32, 0);
  cast_int64(-29292929);
  cut_assert_equal_int(-29292929, GRN_INT32_VALUE(&dest));
}

void
test_int64_to_uint32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT32, 0);
  cast_int64(29292929);
  cut_assert_equal_uint(29292929, GRN_UINT32_VALUE(&dest));
}

void
test_int64_to_int64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT64, 0);
  cast_int64(-29292929);
  gcut_assert_equal_int64(G_GINT64_CONSTANT(-29292929),
                          GRN_INT64_VALUE(&dest));
}

void
test_int64_to_uint64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT64, 0);
  cast_int64(G_GINT64_CONSTANT(2929292929));
  gcut_assert_equal_uint64(G_GUINT64_CONSTANT(2929292929),
                           GRN_UINT64_VALUE(&dest));
}

void
test_int64_to_float(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_FLOAT, 0);
  cast_int64(29);
  cut_assert_equal_double(29, 0.1, GRN_FLOAT_VALUE(&dest));
}

void
test_int64_to_time(void)
{
  long long int sec, usec;

  grn_obj_reinit(&context, &dest, GRN_DB_TIME, 0);
  cast_int64(1259009530);
  GRN_TIME_UNPACK(GRN_TIME_VALUE(&dest), sec, usec);
  cut_assert_equal_int(1259009530, sec);
  cut_assert_equal_int(0, usec);
}

void
test_int64_to_text(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_TEXT, 0);
  cast_int64(2929);
  cut_assert_equal_string("2929", GRN_TEXT_VALUE(&dest));
}

static void
cast_uint64(guint64 number)
{
  grn_obj_reinit(&context, &src, GRN_DB_UINT64, 0);
  GRN_UINT64_SET(&context, &src, number);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
}

void
data_uint64_to_bool(void)
{
#define ADD_DATA(label, expected, number)               \
  gcut_add_datum(label,                                 \
                 "expected", G_TYPE_UINT, expected,     \
                 "number", G_TYPE_UINT, number,         \
                 NULL)

  ADD_DATA("true", GRN_TRUE, 1);
  ADD_DATA("false", GRN_FALSE, 0);
  ADD_DATA("not zero", GRN_TRUE, 100);

#undef ADD_DATA
}

void
test_uint64_to_bool(gconstpointer data)
{
  grn_obj_reinit(&context, &dest, GRN_DB_BOOL, 0);
  cast_uint64(gcut_data_get_uint(data, "number"));
  cut_assert_equal_boolean(gcut_data_get_uint(data, "expected"),
                           GRN_BOOL_VALUE(&dest));
}

void
test_uint64_to_int8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT8, 0);
  cast_uint64(29);
  cut_assert_equal_int(29, GRN_UINT8_VALUE(&dest));
}

void
test_uint64_to_uint8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT8, 0);
  cast_uint64(29);
  cut_assert_equal_uint(29, GRN_UINT8_VALUE(&dest));
}

void
test_uint64_to_int16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT16, 0);
  cast_uint64(2929);
  cut_assert_equal_int(2929, GRN_UINT16_VALUE(&dest));
}

void
test_uint64_to_uint16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT16, 0);
  cast_uint64(2929);
  cut_assert_equal_uint(2929, GRN_UINT16_VALUE(&dest));
}

void
test_uint64_to_int32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT32, 0);
  cast_uint64(29292929);
  cut_assert_equal_int(29292929, GRN_INT32_VALUE(&dest));
}

void
test_uint64_to_uint32(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT32, 0);
  cast_uint64(29292929);
  cut_assert_equal_uint(29292929, GRN_UINT32_VALUE(&dest));
}

void
test_uint64_to_int64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT64, 0);
  cast_uint64(G_GUINT64_CONSTANT(2929292929));
  gcut_assert_equal_int64(G_GINT64_CONSTANT(2929292929),
                          GRN_INT64_VALUE(&dest));
}

void
test_uint64_to_uint64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT64, 0);
  cast_uint64(G_GUINT64_CONSTANT(2929292929));
  gcut_assert_equal_uint64(G_GUINT64_CONSTANT(2929292929),
                           GRN_UINT64_VALUE(&dest));
}

void
test_uint64_to_float(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_FLOAT, 0);
  cast_uint64(29);
  cut_assert_equal_double(29, 0.1, GRN_FLOAT_VALUE(&dest));
}

void
test_uint64_to_time(void)
{
  long long int sec, usec;

  grn_obj_reinit(&context, &dest, GRN_DB_TIME, 0);
  cast_uint64(1259009530);
  GRN_TIME_UNPACK(GRN_TIME_VALUE(&dest), sec, usec);
  cut_assert_equal_int(1259009530, sec);
  cut_assert_equal_int(0, usec);
}

void
test_uint64_to_text(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_TEXT, 0);
  cast_uint64(2929);
  cut_assert_equal_string("2929", GRN_TEXT_VALUE(&dest));
}

void
test_tokyo_geo_point_to_tokyo_geo_point(void)
{
  gint takane_latitude, takane_longitude;
  gint takane_latitude_in_tokyo_geodetic_system = 130183139;
  gint takane_longitude_in_tokyo_geodetic_system = 503813760;

  grn_obj_reinit(&context, &src, GRN_DB_TOKYO_GEO_POINT, 0);
  GRN_GEO_POINT_SET(&context, &src,
                    takane_latitude_in_tokyo_geodetic_system,
                    takane_longitude_in_tokyo_geodetic_system);

  grn_obj_reinit(&context, &dest, GRN_DB_TOKYO_GEO_POINT, 0);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(takane_latitude_in_tokyo_geodetic_system,
                       takane_latitude);
  cut_assert_equal_int(takane_longitude_in_tokyo_geodetic_system,
                       takane_longitude);
}

void
test_tokyo_geo_point_to_wgs84_geo_point(void)
{
  gint takane_latitude, takane_longitude;
  gint takane_latitude_in_tokyo_geodetic_system = 130183139;
  gint takane_longitude_in_tokyo_geodetic_system = 503813760;
  gint takane_latitude_in_wgs84 = 130194581;
  gint takane_longitude_in_wgs84 = 503802072;

  grn_obj_reinit(&context, &src, GRN_DB_TOKYO_GEO_POINT, 0);
  GRN_GEO_POINT_SET(&context, &src,
                    takane_latitude_in_tokyo_geodetic_system,
                    takane_longitude_in_tokyo_geodetic_system);

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(takane_latitude_in_wgs84,
                       takane_latitude);
  cut_assert_equal_int(takane_longitude_in_wgs84,
                       takane_longitude);
}

void
test_wgs84_geo_point_to_wgs84_geo_point(void)
{
  gint takane_latitude, takane_longitude;
  gint takane_latitude_in_wgs84 = 130194581;
  gint takane_longitude_in_wgs84 = 503802073;

  grn_obj_reinit(&context, &src, GRN_DB_WGS84_GEO_POINT, 0);
  GRN_GEO_POINT_SET(&context, &src,
                    takane_latitude_in_wgs84,
                    takane_longitude_in_wgs84);

  grn_obj_reinit(&context, &dest, GRN_DB_WGS84_GEO_POINT, 0);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(takane_latitude_in_wgs84,
                       takane_latitude);
  cut_assert_equal_int(takane_longitude_in_wgs84,
                       takane_longitude);
}

void
test_wgs84_geo_point_to_tokyo_geo_point(void)
{
  gint takane_latitude, takane_longitude;
  gint takane_latitude_in_wgs84 = 130194581;
  gint takane_longitude_in_wgs84 = 503802073;
  gint takane_latitude_in_tokyo_geodetic_system = 130183140;
  gint takane_longitude_in_tokyo_geodetic_system = 503813761;

  grn_obj_reinit(&context, &src, GRN_DB_WGS84_GEO_POINT, 0);
  GRN_GEO_POINT_SET(&context, &src,
                    takane_latitude_in_wgs84,
                    takane_longitude_in_wgs84);

  grn_obj_reinit(&context, &dest, GRN_DB_TOKYO_GEO_POINT, 0);
  grn_test_assert(grn_obj_cast(&context, &src, &dest, GRN_FALSE));
  GRN_GEO_POINT_VALUE(&dest, takane_latitude, takane_longitude);
  cut_assert_equal_int(takane_latitude_in_tokyo_geodetic_system,
                       takane_latitude);
  cut_assert_equal_int(takane_longitude_in_tokyo_geodetic_system,
                       takane_longitude);
}
