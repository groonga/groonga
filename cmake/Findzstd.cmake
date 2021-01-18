# Copyright(C) 2021 Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License version 2.1 as published by the Free Software Foundation.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

find_package(zstd CONFIG)
if(zstd_FOUND)
  return()
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBZSTD libzstd)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(zstd
  REQUIRED_VARS LIBZSTD_LINK_LIBRARIES
  VERSION_VAR LIBZSTD_VERSION)

if(zstd_FOUND AND NOT TARGET zstd::libzstd_shared)
  add_library(zstd::libzstd_shared SHARED IMPORTED)
  set_target_properties(zstd::libzstd_shared PROPERTIES
    IMPORTED_LOCATION "${LIBZSTD_LINK_LIBRARIES}"
    INTERFACE_COMPILE_OPTIONS "${LIBZSTD_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBZSTD_INCLUDE_DIRS}")
endif()
