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

#include <gcutter.h>

#include <hash.h>

#include "sen-test-hash-assertions.h"

void
sen_test_assert_create_hash_helper(sen_hash **hash, SenTestHashFactory *factory)
{
  GError *error = NULL;
  sen_logger_info *logger;

  logger = sen_test_hash_factory_get_logger(factory);
  cut_assert_not_null(logger);
  sen_collect_logger_clear_messages(logger);
  *hash = sen_test_hash_factory_create(factory, &error);
  gcut_assert_error(error);
  cut_assert_not_null(*hash);
  gcut_assert_equal_list_string(NULL, sen_collect_logger_get_messages(logger));
}

void
sen_test_assert_open_hash_helper(sen_hash **hash, SenTestHashFactory *factory)
{
  GError *error = NULL;
  sen_logger_info *logger;

  logger = sen_test_hash_factory_get_logger(factory);
  cut_assert_not_null(logger);
  sen_collect_logger_clear_messages(logger);
  *hash = sen_test_hash_factory_open(factory, &error);
  gcut_assert_error(error);
  cut_assert_not_null(*hash);
  gcut_assert_equal_list_string(NULL, sen_collect_logger_get_messages(logger));
}

void
sen_test_assert_fail_open_hash_helper(sen_hash **hash,
                                      SenTestHashFactory *factory)
{
  GError *expected_error = NULL;
  GError *actual_error = NULL;

  *hash = sen_test_hash_factory_open(factory, &actual_error);
  expected_error = g_error_new(SEN_TEST_HASH_FACTORY_ERROR,
                               SEN_TEST_HASH_FACTORY_ERROR_NULL,
                               "failed to open sen_hash");
  gcut_take_error(expected_error);
  gcut_take_error(actual_error);
  gcut_assert_equal_error(expected_error, actual_error);
  cut_assert_null(*hash);
}
