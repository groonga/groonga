# Copyright(C) 2021-2023  Sutou Kouhei <kou@clear-code.com>
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

if(Groongazstd_FOUND)
  return()
endif()

set(find_package_args "")
set(pkg_check_modules_version "")
if(Groongazstd_FIND_VERSION)
  list(APPEND find_package_args ${Groongazstd_FIND_VERSION})
  set(pkg_check_modules_version ">=${Groongazstd_FIND_VERSION}")
endif()
if(Groongazstd_FIND_QUIETLY)
  list(APPEND find_package_args QUIET)
endif()
find_package(zstd ${find_package_args})
set(Groongazstd_FOUND ${zstd_FOUND})
if(Groongazstd_FOUND)
  set(GRN_ZSTD_TARGET zstd::libzstd_shared)
  return()
endif()

if(NOT Groongazstd_FOUND)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      pkg_check_modules(Groongazstd_pkg_libzstd IMPORTED_TARGET
                        "libzstd${pkg_check_modules_version}")
    else()
      pkg_check_modules(Groongazstd_pkg_libzstd IMPORTED_TARGET GLOBAL
                        "libzstd${pkg_check_modules_version}")
    endif()
    set(Groongazstd_FOUND ${Groongazstd_pkg_libzstd_FOUND})
    if(Groongazstd_pkg_libzstd_FOUND)
      set(GRN_ZSTD_TARGET PkgConfig::Groongazstd_pkg_libzstd)
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Groongazstd REQUIRED_VARS Groongazstd_FOUND)
