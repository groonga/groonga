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

#include <ctx.h>

#include <gcutter.h>
#include <glib/gstdio.h>

#include "../lib/sen-assertions.h"

void test_dynamic_malloc_change(void);

static sen_ctx *context;
static int default_flags;
static void *memory;

void
setup(void)
{
  context = NULL;
  default_flags = SEN_CTX_USE_QL;
  memory = NULL;
}

void
teardown(void)
{
  if (context) {
    if (memory) {
      sen_ctx *ctx = context;
      SEN_FREE(memory);
    }
    sen_ctx_close(context);
  }
}

#define open_context()                                                  \
  context = sen_ctx_open(NULL, default_flags)

#define cut_assert_open_context() do            \
{                                               \
  open_context();                               \
  cut_assert(context);                          \
} while (0)

#ifdef USE_DYNAMIC_MALLOC_CHANGE
static void *
malloc_always_fail(sen_ctx *ctx, size_t size,
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
    sen_ctx *ctx = context;

    memory = SEN_MALLOC(1);
    cut_assert_not_null(memory);
    SEN_FREE(memory);

    sen_ctx_set_malloc(ctx, malloc_always_fail);
    memory = SEN_MALLOC(1);
    cut_assert_null(memory);
  }
#else
  cut_omit("dynamic malloc change is disabled.");
#endif
}
