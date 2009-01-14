/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>

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

#include <groonga.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/sen-assertions.h"

void test_load(void);

static sen_db *db;
static sen_ctx *context;
static gchar *base_dir;
static gchar *default_path;
static int default_db_flags, default_context_flags;
static sen_encoding default_encoding;
static gchar *sample_ql_program;

void
setup(void)
{
  db = NULL;
  context = NULL;

  base_dir = g_build_filename(sen_test_get_base_dir(), "tmp", NULL);
  default_path = g_build_filename(base_dir, "db", NULL);
  default_db_flags = 0;
  default_encoding = sen_enc_default;
  default_context_flags = SEN_CTX_USE_QL;

  cut_remove_path(base_dir, NULL);
  g_mkdir_with_parents(base_dir, 0755);

  sample_ql_program = g_build_filename(sen_test_get_base_dir(),
                                       "..", "ql", "bookmark.scm", NULL);
}

void
teardown(void)
{
  if (db) {
    sen_db_close(db);
  }

  if (context) {
    sen_ctx_close(context);
  }

  if (default_path) {
    g_free(default_path);
  }

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
    g_free(base_dir);
  }

  if (sample_ql_program) {
    g_free(sample_ql_program);
  }
}

#define create_db()                                                     \
  db = sen_db_create(default_path, default_db_flags, default_encoding)

#define open_context()                                                  \
  context = sen_ctx_open(db, default_context_flags)

#define cut_assert_open_context() do            \
{                                               \
  create_db();                                  \
  cut_assert(db);                               \
  open_context();                               \
  cut_assert(context);                          \
} while (0)

void
test_load(void)
{
  cut_assert_open_context();
  sen_test_assert(sen_ctx_load(context, sample_ql_program));
}
