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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

#include <str.h>
#include <util.h>

void test_null(void);
void test_int8(void);
void test_int16(void);
void test_int32(void);
void test_int64(void);
void test_uint8(void);
void test_uint16(void);
void test_uint32(void);
void test_uint64(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_obj *database;

static grn_obj *inspected;
static grn_obj *int8, *int16, *int32, *int64;
static grn_obj *uint8, *uint16, *uint32, *uint64;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_base_dir(),
                                   "tmp",
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

void
cut_setup(void)
{
  const gchar *database_path;

  inspected = NULL;
  int8 = int16 = int32 = int64 = NULL;
  uint8 = uint16 = uint32 = uint64 = NULL;

  remove_tmp_directory();
  g_mkdir_with_parents(tmp_directory, 0700);

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);

  database_path = cut_build_path(tmp_directory, "database.groonga", NULL);
  database = grn_db_create(context, database_path, NULL);
}

void
cut_teardown(void)
{
  grn_obj_close(context, int8);
  grn_obj_close(context, int16);
  grn_obj_close(context, int32);
  grn_obj_close(context, int64);
  grn_obj_close(context, uint8);
  grn_obj_close(context, uint16);
  grn_obj_close(context, uint32);
  grn_obj_close(context, uint64);
  grn_obj_close(context, inspected);

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

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

