/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2009  Kouhei Sutou <kou@clear-code.com>

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

#include <db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#define OBJECT(name) (grn_ctx_get(&context, (name), strlen(name)))

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
cast_text(const gchar *text)
{
  grn_obj_reinit(&context, &src, GRN_DB_TEXT, 0);
  GRN_TEXT_PUTS(&context, &src, text);
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

  ADD_DATA("true", GRN_TRUE, "true");
  ADD_DATA("false", GRN_FALSE, "false");
  ADD_DATA("unknown", GRN_FALSE, "unknown");

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
  cut_assert_equal_int(-29, GRN_INT32_VALUE(&dest));
}

void
test_text_to_uint8(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT8, 0);
  cast_text("29");
  cut_assert_equal_uint(29, GRN_UINT32_VALUE(&dest));
}

void
test_text_to_int16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_INT16, 0);
  cast_text("-2929");
  cut_assert_equal_int(-2929, GRN_INT32_VALUE(&dest));
}

void
test_text_to_uint16(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT16, 0);
  cast_text("2929");
  cut_assert_equal_uint(2929, GRN_UINT32_VALUE(&dest));
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
  cut_assert_equal_int(-2929292929292929, GRN_INT64_VALUE(&dest));
}

void
test_text_to_uint64(void)
{
  grn_obj_reinit(&context, &dest, GRN_DB_UINT64, 0);
  cast_text("2929292929292929");
  cut_assert_equal_uint(2929292929292929, GRN_UINT64_VALUE(&dest));
}
