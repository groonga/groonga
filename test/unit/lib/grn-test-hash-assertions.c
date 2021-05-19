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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <gcutter.h>

#include <grn_hash.h>

#include "grn-test-hash-assertions.h"

void
grn_test_assert_create_hash_helper(grn_hash **hash, GrnTestHashFactory *factory)
{
  GError *error = NULL;
  grn_logger_info *logger;

  logger = grn_test_hash_factory_get_logger(factory);
  cut_assert_not_null(logger);
  grn_collect_logger_clear_messages(logger);
  *hash = grn_test_hash_factory_create(factory, &error);
  gcut_assert_error(error);
  cut_assert_not_null(*hash);
  gcut_assert_equal_list_string(NULL, grn_collect_logger_get_messages(logger));
}

void
grn_test_assert_open_hash_helper(grn_hash **hash, GrnTestHashFactory *factory)
{
  GError *error = NULL;
  grn_logger_info *logger;

  logger = grn_test_hash_factory_get_logger(factory);
  cut_assert_not_null(logger);
  grn_collect_logger_clear_messages(logger);
  *hash = grn_test_hash_factory_open(factory, &error);
  gcut_assert_error(error);
  cut_assert_not_null(*hash);
  gcut_assert_equal_list_string(NULL, grn_collect_logger_get_messages(logger));
}

void
grn_test_assert_fail_open_hash_helper(grn_hash **hash,
                                      GrnTestHashFactory *factory)
{
  GError *expected_error = NULL;
  GError *actual_error = NULL;

  *hash = grn_test_hash_factory_open(factory, &actual_error);
  expected_error = g_error_new(GRN_TEST_HASH_FACTORY_ERROR,
                               GRN_TEST_HASH_FACTORY_ERROR_NULL,
                               "failed to open grn_hash");
  gcut_take_error(expected_error);
  gcut_take_error(actual_error);
  gcut_assert_equal_error(expected_error, actual_error);
  cut_assert_null(*hash);
}
