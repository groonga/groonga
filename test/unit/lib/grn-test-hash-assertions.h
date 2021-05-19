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

#ifndef __GRN_TEST_HASH_ASSERTIONS_H__
#define __GRN_TEST_HASH_ASSERTIONS_H__

#include "grn-assertions.h"
#include "grn-test-hash-factory.h"

#define grn_test_assert_create_hash(hash, factory)      \
  cut_trace_with_info_expression(                       \
    grn_test_assert_create_hash_helper(hash, factory),  \
    grn_test_assert_create_hash(hash, factory))

#define grn_test_assert_open_hash(hash, factory)        \
  cut_trace_with_info_expression(                       \
    grn_test_assert_open_hash_helper(hash, factory),    \
    grn_test_assert_open_hash(hash, factory))

#define grn_test_assert_fail_open_hash(hash, factory)           \
  cut_trace_with_info_expression(                               \
    grn_test_assert_fail_open_hash_helper(hash, factory),       \
    grn_test_assert_fail_open_hash(hash, factory))


void     grn_test_assert_create_hash_helper    (grn_hash           **hash,
                                                GrnTestHashFactory  *factory);
void     grn_test_assert_open_hash_helper      (grn_hash           **hash,
                                                GrnTestHashFactory  *factory);
void     grn_test_assert_fail_open_hash_helper (grn_hash           **hash,
                                                GrnTestHashFactory  *factory);

#endif
