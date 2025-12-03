# Copyright(C) 2025  Horimoto Yasuhiro <horimoto@clear-code.com>
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

if(Groongaopenzl_FOUND)
  return()
endif()

set(find_package_args "")
set(pkg_check_modules_version "")
if(Groongaopenzl_FIND_VERSION)
  list(APPEND find_package_args ${Groongaopenzl_FIND_VERSION})
  set(pkg_check_modules_version ">=${Groongaopenzl_FIND_VERSION}")
endif()
if(Groongaopenzl_FIND_QUIETLY)
  list(APPEND find_package_args QUIET)
endif()
find_package(openzl ${find_package_args})
set(Groongaopenzl_FOUND ${openzl_FOUND})
if(Groongaopenzl_FOUND)
  set(GRN_OPENZL_TARGET openzl::libopenzl)
  return()
endif()

if(NOT Groongaopenzl_FOUND)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    pkg_check_modules(Groongaopenzl_pkg_libopenzl IMPORTED_TARGET
                      "libopenzl${pkg_check_modules_version}")
  endif()
  set(Groongaopenzl_FOUND ${Groongaopenzl_pkg_libopenzl_FOUND})
  if(Groongaopenzl_pkg_libopenzl_FOUND)
    set(GRN_OPENZL_TARGET PkgConfig::Groongaopenzl_pkg_libopenzl)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Groongaopenzl
                                  REQUIRED_VARS Groongaopenzl_FOUND)
