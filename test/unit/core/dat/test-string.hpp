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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef GRN_DAT_TEST_STRING_HPP_
#define GRN_DAT_TEST_STRING_HPP_

#include <dat/string.hpp>

#include <iostream>

namespace grn {
namespace dat {

inline std::ostream &operator<<(std::ostream &stream,
                                const grn::dat::String &str) {
  return stream.write(static_cast<const char *>(str.ptr()), str.length());
}

}  // namespace dat
}  // namespace grn

#endif  // GRN_DAT_TEST_STRING_HPP_
