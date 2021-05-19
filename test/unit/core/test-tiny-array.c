/*
  Copyright (C) 2012-2014  Brazil Inc.

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

#include <string.h>

#include <grn_hash.h>

static grn_ctx ctx;

void
cut_setup(void)
{
  grn_ctx_init(&ctx, 0);
}

void
cut_teardown(void)
{
  grn_ctx_fin(&ctx);
}

static void
test_basic_operations(uint16_t element_size, uint16_t flags)
{
  grn_id id;
  grn_tiny_array array;
  grn_tiny_array_init(&ctx, &array, element_size, flags);
  cut_assert_null(grn_tiny_array_at(&array, 0));
  for (id = 1; id < (1 << 8); id++) {
    void * const ptr = grn_tiny_array_at(&array, id);
    cut_assert_not_null(ptr);
    memset(ptr, (unsigned char)id, element_size);
  }
  for (id = 1; id < (1 << 8); id++) {
    uint16_t byte_id;
    const unsigned char expected_byte = (unsigned char)id;
    const unsigned char * const ptr =
        (const unsigned char *)grn_tiny_array_at(&array, id);
    for (byte_id = 0; byte_id < element_size; byte_id++) {
      cut_assert_equal_int(expected_byte, ptr[byte_id]);
    }
  }
  grn_tiny_array_fin(&array);
}

void
test_at(void)
{
  uint16_t element_size;
  for (element_size = 1; element_size < 16; element_size++) {
    test_basic_operations(element_size, 0);
  }
}

void
test_id(void)
{
  grn_id id;
  grn_tiny_array array;
  grn_tiny_array_init(&ctx, &array, sizeof(int), GRN_TINY_ARRAY_CLEAR);
  grn_test_assert_equal_id(&ctx, (grn_id)0, grn_tiny_array_id(&array, NULL));
  for (id = 1; id < (1 << 10); id++) {
    const void * const ptr = grn_tiny_array_at(&array, id);
    cut_assert_not_null(ptr);
    grn_test_assert_equal_id(&ctx, (grn_id)id, grn_tiny_array_id(&array, ptr));
  }
  grn_tiny_array_fin(&array);
}

void
test_clear(void)
{
  grn_id id;
  grn_tiny_array array;
  grn_tiny_array_init(&ctx, &array, sizeof(int), GRN_TINY_ARRAY_CLEAR);
  for (id = 1; id < (1 << 10); id++) {
    const int expected_value = 0;
    const int * const value_ptr = (const int *)grn_tiny_array_at(&array, id);
    cut_assert_not_null(value_ptr);
    cut_assert_equal_memory(&expected_value, sizeof(expected_value),
                            value_ptr, sizeof(int));
  }
  grn_tiny_array_fin(&array);

  {
    uint16_t element_size;
    for (element_size = 1; element_size < 16; element_size++) {
      test_basic_operations(element_size, GRN_TINY_ARRAY_CLEAR);
    }
  }
}

void
test_threadsafe(void)
{
  /* TODO: A multi-threaded test should be done. */

  uint16_t element_size;
  for (element_size = 1; element_size < 16; element_size++) {
    test_basic_operations(element_size, GRN_TINY_ARRAY_THREADSAFE);
  }
}

void
test_use_malloc(void)
{
  uint16_t element_size;
  for (element_size = 1; element_size < 16; element_size++) {
    test_basic_operations(element_size, GRN_TINY_ARRAY_USE_MALLOC);
  }
}
