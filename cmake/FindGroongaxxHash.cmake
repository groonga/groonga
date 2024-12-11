# Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

if(GroongaxxHash_FOUND)
  return()
endif()

set(find_package_args "")
set(pkg_check_modules_version "")
if(GroongaxxHash_FIND_VERSION)
  list(APPEND find_package_args ${GroongaxxHash_FIND_VERSION})
  set(pkg_check_modules_version ">=${GroongaxxHash_FIND_VERSION}")
endif()
if(GroongaxxHash_FIND_QUIETLY)
  list(APPEND find_package_args QUIET)
endif()
find_package(xxHash ${find_package_args})
set(GroongaxxHash_FOUND ${xxHash_FOUND})
if(GroongaxxHash_FOUND)
  add_library(Groonga::xxhash ALIAS xxHash::xxhash)
  return()
endif()

if(NOT GroongaxxHash_FOUND)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      pkg_check_modules(GroongaxxHash_pkg_libxxhash IMPORTED_TARGET
                        "libxxhash${pkg_check_modules_version}")
    else()
      pkg_check_modules(GroongaxxHash_pkg_libxxhash IMPORTED_TARGET GLOBAL
                        "libxxhash${pkg_check_modules_version}")
    endif()
    set(GroongaxxHash_FOUND ${GroongaxxHash_pkg_libxxhash_FOUND})
    if(GroongaxxHash_pkg_libxxhash_FOUND)
      if(GroongaxxHash_pkg_libxxhash_VERSION VERSION_LESS "0.8.0")
        target_compile_definitions(PkgConfig::GroongaxxHash_pkg_libxxhash
                                   INTERFACE XXH_INLINE_ALL)
      endif()
      add_library(Groonga::xxhash ALIAS PkgConfig::GroongaxxHash_pkg_libxxhash)
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GroongaxxHash
                                  REQUIRED_VARS GroongaxxHash_FOUND)
