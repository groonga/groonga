/* -*- c-basic-offset: 2; coding: utf-8 -*- */
/*
  Copyright (C) 2011  Brazil

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

#include <gcutter.h>
#include <cppcutter.h>

#include <grn-assertions.h>
#include <dat/base.hpp>

namespace test_dat_base
{
  void test_initial_values(void)
  {
    grn::dat::Base base;

    cppcut_assert_equal(base.is_linker(), false);
    cppcut_assert_equal(base.offset(), static_cast<grn::dat::UInt32>(0));
  }

  void test_linker(void)
  {
    grn::dat::Base base;

    base.set_key_pos(100);
    cppcut_assert_equal(base.is_linker(), true);
    cppcut_assert_equal(base.key_pos(), static_cast<grn::dat::UInt32>(100));
  }

  void test_nonlinker(void)
  {
    grn::dat::Base base;

    base.set_offset(1000);
    cppcut_assert_equal(base.is_linker(), false);
    cppcut_assert_equal(base.offset(), static_cast<grn::dat::UInt32>(1000));
  }
}
