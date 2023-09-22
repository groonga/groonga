# Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>
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

if(Groongalz4_FOUND)
  return()
endif()

set(find_package_args "")
set(pkg_check_modules_version "")
if(Groongalz4_FIND_VERSION)
  list(APPEND find_package_args ${Groongalz4_FIND_VERSION})
  set(pkg_check_modules_version ">=${Groongalz4_FIND_VERSION}")
endif()
if(Groongalz4_FIND_QUIETLY)
  list(APPEND find_package_args QUIET)
endif()
find_package(lz4 ${find_package_args})
set(Groongalz4_FOUND ${lz4_FOUND})
if(Groongalz4_FOUND)
  if(TARGET lz4::liblz4_shared)
    set(GRN_LZ4_TARGET lz4::liblz4_shared)
  else()
    set(GRN_LZ4_TARGET lz4::liblz4_static)
  endif()
  return()
endif()

if(NOT Groongalz4_FOUND)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      pkg_check_modules(Groongalz4_pkg_liblz4 IMPORTED_TARGET
                        "liblz4${pkg_check_modules_version}")
    else()
      pkg_check_modules(Groongalz4_pkg_liblz4 IMPORTED_TARGET GLOBAL
                        "liblz4${pkg_check_modules_version}")
    endif()
    set(Groongalz4_FOUND ${Groongalz4_pkg_liblz4_FOUND})
    if(Groongalz4_pkg_liblz4_FOUND)
      set(GRN_LZ4_TARGET PkgConfig::Groongalz4_pkg_liblz4)
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Groongalz4 REQUIRED_VARS Groongalz4_FOUND)
