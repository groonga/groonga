/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2008-2009  Kouhei Sutou <kou@cozmixng.org>

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

#include <ctx.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/grn-assertions.h"

void test_dynamic_malloc_change(void);

static grn_ctx *context;
static int default_flags;
static void *memory;

void
cut_setup(void)
{
  context = NULL;
  default_flags = GRN_CTX_USE_QL;
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
    grn_ctx_fin(context);
  }
}

#define open_context()                                                  \
  context = grn_ctx_open(NULL, default_flags)

#define cut_assert_open_context() do            \
{                                               \
  open_context();                               \
  cut_assert(context);                          \
} while (0)

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
  cut_assert_open_context();
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
