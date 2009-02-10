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

#ifndef __SEN_TEST_HASH_ASSERTIONS_H__
#define __SEN_TEST_HASH_ASSERTIONS_H__

#include "sen-assertions.h"
#include "sen-test-hash-factory.h"

#define sen_test_assert_create_hash(hash, factory)      \
  cut_trace_with_info_expression(                       \
    sen_test_assert_create_hash_helper(hash, factory),  \
    sen_test_assert_create_hash(hash, factory))

#define sen_test_assert_open_hash(hash, factory)        \
  cut_trace_with_info_expression(                       \
    sen_test_assert_open_hash_helper(hash, factory),    \
    sen_test_assert_open_hash(hash, factory))

#define sen_test_assert_fail_open_hash(hash, factory)           \
  cut_trace_with_info_expression(                               \
    sen_test_assert_fail_open_hash_helper(hash, factory),       \
    sen_test_assert_fail_open_hash(hash, factory))


void     sen_test_assert_create_hash_helper    (sen_hash           **hash,
                                                SenTestHashFactory  *factory);
void     sen_test_assert_open_hash_helper      (sen_hash           **hash,
                                                SenTestHashFactory  *factory);
void     sen_test_assert_fail_open_hash_helper (sen_hash           **hash,
                                                SenTestHashFactory  *factory);

#endif
