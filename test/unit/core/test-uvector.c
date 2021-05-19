/*
  Copyright (C) 2014  Kouhei Sutou <kou@clear-code.com>

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

#include <groonga.h>

void test_size_bool(void);
void test_size_int8(void);
void test_size_uint8(void);
void test_size_int16(void);
void test_size_uint16(void);
void test_size_int32(void);
void test_size_uint32(void);
void test_size_int64(void);
void test_size_uint64(void);
void test_size_float(void);
void test_size_time(void);
void test_size_tokyo_geo_point(void);
void test_size_wgs84_geo_point(void);

static grn_ctx *context;
static grn_obj *uvector;

void
cut_setup(void)
{
  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  uvector = g_new0(grn_obj, 1);
}

void
cut_teardown(void)
{
  grn_obj_close(context, uvector);
  g_free(uvector);

  grn_ctx_fin(context);
  g_free(context);
}

void
test_size_bool(void)
{
  GRN_BOOL_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(1, grn_uvector_element_size(context, uvector));
}

void
test_size_int8(void)
{
  GRN_INT8_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(1, grn_uvector_element_size(context, uvector));
}

void
test_size_uint8(void)
{
  GRN_UINT8_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(1, grn_uvector_element_size(context, uvector));
}

void
test_size_int16(void)
{
  GRN_INT16_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(2, grn_uvector_element_size(context, uvector));
}

void
test_size_uint16(void)
{
  GRN_UINT16_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(2, grn_uvector_element_size(context, uvector));
}

void
test_size_int32(void)
{
  GRN_INT32_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(4, grn_uvector_element_size(context, uvector));
}

void
test_size_uint32(void)
{
  GRN_UINT32_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(4, grn_uvector_element_size(context, uvector));
}

void
test_size_int64(void)
{
  GRN_INT64_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(8, grn_uvector_element_size(context, uvector));
}

void
test_size_uint64(void)
{
  GRN_UINT64_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(8, grn_uvector_element_size(context, uvector));
}

void
test_size_float(void)
{
  GRN_FLOAT_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(8, grn_uvector_element_size(context, uvector));
}

void
test_size_time(void)
{
  GRN_TIME_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(8, grn_uvector_element_size(context, uvector));
}

void
test_size_tokyo_geo_point(void)
{
  GRN_TOKYO_GEO_POINT_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(8, grn_uvector_element_size(context, uvector));
}

void
test_size_wgs84_geo_point(void)
{
  GRN_WGS84_GEO_POINT_INIT(uvector, GRN_OBJ_VECTOR);
  cut_assert_equal_uint(8, grn_uvector_element_size(context, uvector));
}
