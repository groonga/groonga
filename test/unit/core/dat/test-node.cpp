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
#include <dat/node.hpp>

namespace test_dat_node
{
  void test_base(void)
  {
    grn::dat::Node node;
    grn::dat::Base base;

    cut_assert(node.base() == base);

    node.set_key_pos(100);
    base.set_key_pos(100);
    cut_assert(node.base() == base);
    cppcut_assert_equal(node.is_linker(), base.is_linker());
    cppcut_assert_equal(node.key_pos(), base.key_pos());

    node.set_offset(1000);
    base.set_offset(1000);
    cut_assert(node.base() == base);
    cppcut_assert_equal(node.is_linker(), base.is_linker());
    cppcut_assert_equal(node.offset(), base.offset());
  }

  void test_check(void)
  {
    grn::dat::Node node;
    grn::dat::Check check;

    cut_assert(node.check() == check);

    node.set_is_offset(true);
    check.set_is_offset(true);
    cut_assert(node.check() == check);
    cppcut_assert_equal(node.is_offset(), check.is_offset());

    node.set_offset(grn::dat::INVALID_OFFSET);

    node.set_is_phantom(true);
    check.set_is_phantom(true);
    cut_assert(node.check() == check);
    cppcut_assert_equal(node.is_phantom(), check.is_phantom());

    node.set_next(101);
    node.set_prev(99);
    check.set_next(101);
    check.set_prev(99);
    cut_assert(node.check() == check);
    cppcut_assert_equal(node.next(), check.next());
    cppcut_assert_equal(node.prev(), check.prev());

    node.set_is_phantom(false);
    check.set_is_phantom(false);
    cut_assert(node.check() == check);
    cppcut_assert_equal(node.is_phantom(), check.is_phantom());
    cppcut_assert_equal(node.label(), check.label());
    cppcut_assert_equal(node.child(), check.child());
    cppcut_assert_equal(node.sibling(), check.sibling());

    node.set_label('a');
    check.set_label('a');
    cut_assert(node.check() == check);
    cppcut_assert_equal(node.label(), check.label());
  }
}
