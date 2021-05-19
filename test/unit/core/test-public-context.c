/*
  Copyright (C) 2008-2014  Kouhei Sutou <kou@clear-code.com>

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

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_load(void);

static grn_obj *db;
static grn_ctx *context;
static const gchar *base_dir;
static gchar *default_path;
static int default_context_flags;
static grn_encoding default_encoding;
static gchar *sample_ql_program;

void
cut_setup(void)
{
  db = NULL;
  context = g_new0(grn_ctx, 1);

  base_dir = grn_test_get_tmp_dir();
  default_path = g_build_filename(base_dir, "db", NULL);
  default_encoding = GRN_ENC_DEFAULT;
  default_context_flags = 0;

  cut_remove_path(base_dir, NULL);
  g_mkdir_with_parents(base_dir, 0755);

  sample_ql_program = g_build_filename(grn_test_get_base_dir(),
                                       "..", "ql", "bookmark.scm", NULL);
}

void
cut_teardown(void)
{
  if (context) {
    if (db) {
      grn_obj_close(context, db);
    }
    grn_ctx_fin(context);
    g_free(context);
  }

  if (default_path) {
    g_free(default_path);
  }

  if (base_dir) {
    cut_remove_path(base_dir, NULL);
  }

  if (sample_ql_program) {
    g_free(sample_ql_program);
  }
}

#define create_db() do                                  \
{                                                       \
  grn_db_create_optarg option;                          \
                                                        \
  option.builtin_type_names = NULL;                     \
  option.n_builtin_type_names = 0;                      \
  db = grn_db_create(context, default_path, &option);   \
} while (0)

#define open_context() do                                               \
{                                                                       \
  grn_test_assert(grn_ctx_init(context, default_context_flags));        \
  GRN_CTX_SET_ENCODING(context, default_encoding);                      \
} while (0)

#define cut_assert_open_context() do            \
{                                               \
  create_db();                                  \
  cut_assert(db);                               \
  open_context();                               \
  cut_assert(context);                          \
} while (0)
