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
find_package(msgpackc ${find_package_args})
set(Groongamsgpackc_FOUND ${msgpackc_FOUND})
if(Groongamsgpackc_FOUND)
  if(TARGET msgpackc)
    add_library(Groonga::msgpackc ALIAS msgpackc)
  else()
    add_library(Groonga::msgpackc ALIAS msgpackc-static)
  endif()
endif()

if(NOT Groongamsgpackc_FOUND)
  find_package(msgpack ${find_package_args})
  set(Groongamsgpackc_FOUND ${msgpack_FOUND})
  if(Groongamsgpackc_FOUND)
    if(TARGET msgpackc)
      add_library(Groonga::msgpackc ALIAS msgpackc)
    else()
      add_library(Groonga::msgpackc ALIAS msgpackc-static)
    endif()
  endif()
endif()

if(NOT Groongamsgpackc_FOUND)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    pkg_check_modules(Groongamsgpack_pkg_msgpack-c IMPORTED_TARGET
                      "msgpack-c${pkg_check_modules_version}")
    set(Groongamsgpackc_FOUND ${Groongamsgpack_pkg_msgpack-c_FOUND})
    if(Groongamsgpackc_FOUND)
      add_library(Groonga::msgpackc ALIAS
                  PkgConfig::Groongamsgpack_pkg_msgpack-c)
    else()
      pkg_check_modules(Groongamsgpack_pkg_msgpack IMPORTED_TARGET
                        "msgpack${pkg_check_modules_version}")
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
