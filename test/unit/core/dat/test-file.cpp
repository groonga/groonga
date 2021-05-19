/*
  Copyright (C) 2011-2012  Brazil

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
#include <cppcutter.h>

#include <grn-assertions.h>
#include <dat/file.hpp>

#include <cstring>

namespace test_dat_file
{
  const gchar *base_dir = NULL;

  void cut_setup(void)
  {
    base_dir = grn_test_get_tmp_dir();
    cut_remove_path(base_dir, NULL);
    g_mkdir_with_parents(base_dir, 0755);
  }

  void cut_teardown(void)
  {
    if (base_dir) {
      cut_remove_path(base_dir, NULL);
    }
  }

  void test_invalid_file(void)
  {
    const grn::dat::File file;

    cppcut_assert_null(file.ptr());
    cppcut_assert_equal(grn::dat::UInt64(0), file.size());
  }

  void test_create_without_path(void)
  {
    grn::dat::File file;

    try {
      file.create(NULL, 0);
      cut_fail("A zero-byte request is not allowed.");
    } catch (const grn::dat::Exception &) {
    }

    file.create(NULL, 32);
    cppcut_assert_not_null(file.ptr());
    cppcut_assert_equal(grn::dat::UInt64(32), file.size());

    grn::dat::UInt8 * const buf = static_cast<grn::dat::UInt8 *>(file.ptr());
    for (grn::dat::UInt64 i = 0; i < file.size(); ++i) {
      buf[i] = static_cast<grn::dat::UInt8>(i);
      cppcut_assert_equal(static_cast<grn::dat::UInt8>(i), buf[i]);
    }
  }

  void test_create_with_path(void)
  {
    char path[PATH_MAX];
    std::strcpy(path, base_dir);
    std::strcat(path, "/test_create_with_path.dat");

    grn::dat::File file;

    try {
      file.create(path, 0);
      cut_fail("A zero-byte request is not allowed.");
    } catch (const grn::dat::Exception &) {
    }

    file.create(path, 32);
    cppcut_assert_not_null(file.ptr());
    cppcut_assert_equal(grn::dat::UInt64(32), file.size());

    grn::dat::UInt8 * const buf = static_cast<grn::dat::UInt8 *>(file.ptr());
    for (grn::dat::UInt64 i = 0; i < file.size(); ++i) {
      buf[i] = static_cast<grn::dat::UInt8>(i);
      cppcut_assert_equal(static_cast<grn::dat::UInt8>(i), buf[i]);
    }
  }

  void test_open(void)
  {
    char path[PATH_MAX];
    std::strcpy(path, base_dir);
    std::strcat(path, "/test_open.dat");

    grn::dat::File file;

    try {
      file.open(NULL);
      cut_fail("A null-path request is not allowed.");
    } catch (const grn::dat::Exception &) {
    }

    file.create(path, 32);
    std::strcpy(static_cast<char *>(file.ptr()), "This is a pen.");

    file.close();
    cut_assert_null(file.ptr());
    cppcut_assert_equal(grn::dat::UInt64(0), file.size());

    file.open(path);
    cppcut_assert_not_null(file.ptr());
    cppcut_assert_equal(grn::dat::UInt64(32), file.size());
    cppcut_assert_equal(0, std::strcmp(static_cast<char *>(file.ptr()),
                                       "This is a pen."));
  }

  void test_swap(void)
  {
    grn::dat::File file;

    file.create(NULL, 100);
    cppcut_assert_not_null(file.ptr());
    cppcut_assert_equal(grn::dat::UInt64(100), file.size());

    grn::dat::File file_new;
    file_new.swap(&file);

    cut_assert_null(file.ptr());
    cppcut_assert_equal(grn::dat::UInt64(0), file.size());

    cppcut_assert_not_null(file_new.ptr());
    cppcut_assert_equal(grn::dat::UInt64(100), file_new.size());
  }
}
