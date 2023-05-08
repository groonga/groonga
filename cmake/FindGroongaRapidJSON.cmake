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

if(GroongaRapidJSON_FOUND)
  return()
endif()

set(find_package_args "")
set(pkg_check_modules_version "")
if(GroongaRapidJSON_FIND_VERSION)
  list(APPEND find_package_args ${GroongaRapidJSON_FIND_VERSION})
  set(pkg_check_modules_version ">=${GroongaRapidJSON_FIND_VERSION}")
endif()
if(GroongaRapidJSON_FIND_QUIETLY)
  list(APPEND find_package_args QUIET)
endif()
find_package(RapidJSON ${find_package_args})
set(GroongaRapidJSON_FOUND ${RapidJSON_FOUND})
if(GroongaRapidJSON_FOUND)
  add_library(Groonga::RapidJSON INTERFACE IMPORTED)
  target_include_directories(Groonga::RapidJSON
                             INTERFACE "${RAPIDJSON_INCLUDE_DIRS}")
endif()

if(NOT GroongaRapidJSON_FOUND)
  find_package(PkgConfig)
  if(PkgConfig_FOUND)
    pkg_check_modules(GroongaRapidJSON_pkg_RapidJSON IMPORTED_TARGET
                      "RapidJSON${pkg_check_modules_version}")
    set(GroongaRapidJSON_FOUND ${GroongaRapidJSON_pkg_RapidJSON_FOUND})
    if(GroongaRapidJSON_FOUND)
      add_library(Groonga::RapidJSON ALIAS
                  PkgConfig::GroongaRapidJSON_pkg_RapidJSON)
    endif()
  endif()
endif()

if(NOT GroongaRapidJSON_FOUND)
  find_file(GroongaRapidJSON_rapidjson_h rapidjson/rapidjson.h)
  if(GroongaRapidJSON_rapidjson_h)
    add_library(Groonga::RapidJSON INTERFACE IMPORTED)
    get_filename_component(GroongaRapidJSON_INCLUDE_DIR
                           "${GroongaRapidJSON_rapidjson_h}" DIRECTORY)
    get_filename_component(GroongaRapidJSON_INCLUDE_DIR
                           "${GroongaRapidJSON_INCLUDE_DIR}" DIRECTORY)
    target_include_directories(Groonga::RapidJSON
                               INTERFACE "${GroongaRapidJSON_INCLUDE_DIR}")
    set(GroongaRapidJSON_FOUND TRUE)
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GroongaRapidJSON
                                  REQUIRED_VARS GroongaRapidJSON_FOUND)
