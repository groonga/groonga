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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"
#include "grn_store.h"

#include <grn_str.h>

void test_vector_empty_load(void);

static gchar *tmp_directory;

static grn_ctx *context;
static grn_ja *ja;
static grn_obj *vector;

void
cut_startup(void)
{
  tmp_directory = g_build_filename(grn_test_get_tmp_dir(),
                                   "store-ja",
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

  context = g_new0(grn_ctx, 1);
  grn_ctx_init(context, 0);
  ja = grn_ja_create(context, NULL, 65536, 0);
  vector = grn_obj_open(context, GRN_BULK, GRN_OBJ_VECTOR, GRN_DB_VOID);
}

void
cut_teardown(void)
{
  if (vector) {
    grn_obj_unlink(context, vector);
  }

  if (ja) {
    grn_ja_close(context, ja);
  }

  if (context) {
    grn_ctx_fin(context);
    g_free(context);
  }

  remove_tmp_directory();
}

void
test_vector_empty_load(void)
{
  void *ptr;
  uint32_t len;
  grn_io_win iw;

  cut_assert_equal_int(0, grn_ja_size(context, ja, 1));
  ptr = grn_ja_ref(context, ja, 1, &iw, &len);
  grn_ja_unref(context, &iw);
  cut_assert_null(ptr);
  cut_assert_equal_uint(0, len);

  grn_test_assert(grn_ja_putv(context, ja, 1, vector, GRN_OBJ_SET));

  cut_assert_equal_int(1, grn_ja_size(context, ja, 1));
  ptr = grn_ja_ref(context, ja, 1, &iw, &len);
  grn_ja_unref(context, &iw);
  cut_assert_not_null(ptr);
  cut_assert_equal_uint(1, len);
}
