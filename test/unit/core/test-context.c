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

#include <grn_ctx.h>
#include <grn_db.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_at_nonexistent(void);
void test_dynamic_malloc_change(void);
void test_command_version(void);
void test_support_zlib(void);
void test_support_lz4(void);
void test_output_type(void);

static grn_ctx *context;
static grn_obj *database;
static int default_flags;
static void *memory;

void
cut_setup(void)
{
  context = NULL;
  database = NULL;
  default_flags = 0;
  memory = NULL;
}

void
cut_teardown(void)
{
  if (context) {
    if (memory) {
      grn_ctx *ctx = context;
      GRN_FREE(memory);
    }
    if (database) {
      grn_db_close(context, database);
    }
    grn_ctx_fin(context);
    g_free(context);
  }
}

#define cut_assert_ensure_context() do                          \
{                                                               \
  if (!context) {                                               \
    context = g_new0(grn_ctx, 1);                               \
    grn_test_assert(grn_ctx_init(context, default_flags));      \
  }                                                             \
} while (0)

#define cut_assert_ensure_database() do                 \
{                                                       \
  cut_assert_ensure_context();                          \
  if (!database) {                                      \
    database = grn_db_create(context, NULL, NULL);      \
  }                                                     \
} while (0)

void
test_at_nonexistent(void)
{
  cut_assert_ensure_database();
  cut_assert_null(grn_ctx_at(context, 99999));
}

#ifdef USE_DYNAMIC_MALLOC_CHANGE
static void *
malloc_always_fail(grn_ctx *ctx, size_t size,
                   const char *file, int line, const char *func)
{
  return NULL;
}
#endif

void
test_dynamic_malloc_change(void)
{
#ifdef USE_DYNAMIC_MALLOC_CHANGE
  cut_assert_ensure_context();
  {
    grn_ctx *ctx = context;

    memory = GRN_MALLOC(1);
    cut_assert_not_null(memory);
    GRN_FREE(memory);

    grn_ctx_set_malloc(ctx, malloc_always_fail);
    memory = GRN_MALLOC(1);
    cut_assert_null(memory);
  }
#endif
}

void
test_command_version(void)
{
  cut_assert_equal_int(GRN_COMMAND_VERSION_STABLE,
                       grn_get_default_command_version());

  cut_assert_ensure_context();
  cut_assert_equal_int(GRN_COMMAND_VERSION_STABLE,
                       grn_ctx_get_command_version(context));
}

void
test_support_zlib(void)
{
  int support_p;
  grn_obj grn_support_p;

  cut_assert_ensure_context();
  GRN_BOOL_INIT(&grn_support_p, 0);
  grn_obj_get_info(context, NULL, GRN_INFO_SUPPORT_ZLIB, &grn_support_p);
  support_p = GRN_BOOL_VALUE(&grn_support_p);
  GRN_OBJ_FIN(context, &grn_support_p);

#ifdef GRN_WITH_ZLIB
  cut_assert_true(support_p);
#else
  cut_assert_false(support_p);
#endif
}

void
test_support_lz4(void)
{
  int support_p;
  grn_obj grn_support_p;

  cut_assert_ensure_context();
  GRN_BOOL_INIT(&grn_support_p, 0);
  grn_obj_get_info(context, NULL, GRN_INFO_SUPPORT_LZ4, &grn_support_p);
  support_p = GRN_BOOL_VALUE(&grn_support_p);
  GRN_OBJ_FIN(context, &grn_support_p);

#ifdef GRN_WITH_LZ4
  cut_assert_true(support_p);
#else
  cut_assert_false(support_p);
#endif
}

void
test_output_type(void)
{
  cut_assert_ensure_database();

  cut_assert_equal_int(GRN_CONTENT_NONE, grn_ctx_get_output_type(context));
  grn_test_assert(grn_ctx_set_output_type(context, GRN_CONTENT_JSON));
  cut_assert_equal_int(GRN_CONTENT_JSON, grn_ctx_get_output_type(context));
}
