# Copyright(C) 2023-2024  Sutou Kouhei <kou@clear-code.com>
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

if(Groongamsgpackc_FOUND)
  return()
endif()

set(find_package_args "")
set(pkg_check_modules_version "")
if(Groongamsgpackc_FIND_VERSION)
  list(APPEND find_package_args ${Groongamsgpackc_FIND_VERSION})
  set(pkg_check_modules_version ">=${Groongamsgpackc_FIND_VERSION}")
endif()
if(Groongamsgpackc_FIND_QUIETLY)
  list(APPEND find_package_args QUIET)
endif()

find_package(msgpack-c ${find_package_args})
set(Groongamsgpackc_FOUND ${msgpack-c_FOUND})
if(Groongamsgpackc_FOUND)
  if(TARGET msgpack-c)
    add_library(Groonga::msgpackc ALIAS msgpack-c)
  else()
    add_library(Groonga::msgpackc ALIAS msgpack-c-static)
  endif()
endif()

if(NOT Groongamsgpackc_FOUND)
  find_package(msgpackc ${find_package_args})
  set(Groongamsgpackc_FOUND ${msgpackc_FOUND})
  if(Groongamsgpackc_FOUND)
    if(TARGET msgpackc)
      add_library(Groonga::msgpackc ALIAS msgpackc)
    else()
      add_library(Groonga::msgpackc ALIAS msgpackc-static)
    endif()
  endif()
endif()

if(NOT Groongamsgpackc_FOUND AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
  find_package(msgpack ${find_package_args})
  set(Groongamsgpackc_FOUND ${msgpack_FOUND})
  if(Groongamsgpackc_FOUND)
    if(TARGET msgpackc)
      add_library(Groonga::msgpackc ALIAS msgpackc)
    elseif(TARGET msgpackc-static)
      add_library(Groonga::msgpackc ALIAS msgpackc-static)
    else()
      # msgpackc-cxx's msgpack CMake package is found...
      set(Groongamsgpackc_FOUND FALSE)
    endif()
  endif()
endif()

if(NOT Groongamsgpackc_FOUND)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      pkg_check_modules(Groongamsgpack_pkg_msgpack-c IMPORTED_TARGET
                        "msgpack-c${pkg_check_modules_version}")
    else()
      pkg_check_modules(Groongamsgpack_pkg_msgpack-c IMPORTED_TARGET GLOBAL
                        "msgpack-c${pkg_check_modules_version}")
    endif()
    set(Groongamsgpackc_FOUND ${Groongamsgpack_pkg_msgpack-c_FOUND})
    if(Groongamsgpackc_FOUND)
      add_library(Groonga::msgpackc ALIAS
                  PkgConfig::Groongamsgpack_pkg_msgpack-c)
    else()
      if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
        pkg_check_modules(Groongamsgpack_pkg_msgpack IMPORTED_TARGET
                          "msgpack${pkg_check_modules_version}")
      else()
        pkg_check_modules(Groongamsgpack_pkg_msgpack IMPORTED_TARGET GLOBAL
                          "msgpack${pkg_check_modules_version}")
      endif()
      set(Groongamsgpackc_FOUND ${Groongamsgpack_pkg_msgpack_FOUND})
      if(Groongamsgpackc_FOUND)
        add_library(Groonga::msgpackc ALIAS
                    PkgConfig::Groongamsgpack_pkg_msgpack)
      endif()
    endif()
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Groongamsgpackc
                                  REQUIRED_VARS Groongamsgpackc_FOUND)
