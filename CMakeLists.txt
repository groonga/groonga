# Copyright(C) 2012 Brazil
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

cmake_minimum_required(VERSION 2.6)
project(groonga)

file(READ base_version GRN_VERSION)

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(FindPkgConfig)

set(GRN_LOG_PATH
  "${CMAKE_INSTALL_PREFIX}/var/log/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_NAME}.log"
  CACHE FILEPATH "log file path")
set(GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD
  0
  CACHE STRING "groonga default match escalation threshold")
set(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT
  "html/admin"
  CACHE PATH "groonga default relative document root")
set(GRN_DEFAULT_DOCUMENT_ROOT
  "${CMAKE_INSTALL_PREFIX}/share/${CMAKE_PROJECT_NAME}/${GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT}"
  CACHE PATH "groonga default document root")
set(GRN_PLUGINS_DIR
  "${CMAKE_INSTALL_PREFIX}/lib/${CMAKE_PROJECT_NAME}/plugins"
  CACHE PATH "groonga plugins' directory")

add_definitions(
  -DHAVE_CONFIG_H
  )

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/include
  )

macro(ac_check_headers header)
  string(REGEX REPLACE "[/.]" "_" output_variable_name ${header})
  string(TOUPPER "${output_variable_name}" output_variable_name)
  set(output_variable_name "HAVE_${output_variable_name}")
  check_include_file(${header} ${output_variable_name})
endmacro()

macro(ac_check_funcs function)
  string(TOUPPER "${function}" output_variable_name)
  set(output_variable_name "HAVE_${output_variable_name}")
  check_function_exists(${function} ${output_variable_name})
endmacro()

macro(ac_check_lib library function)
  string(REGEX REPLACE "[/.]" "_" output_variable_base_name ${library})
  string(TOUPPER "${output_variable_base_name}" output_variable_base_name)
  set(output_variable_name "HAVE_LIB${output_variable_base_name}")
  check_library_exists(${library} ${function} "" ${output_variable_name})
  if(${output_variable_name})
    set(${output_variable_base_name}_LIBS "${library}")
  endif()
endmacro()

include(build/ac_macros/check_headers.m4)
include(build/ac_macros/check_functions.m4)

ac_check_lib(dl dlopen)
ac_check_lib(execinfo backtrace)
if(HAVE_LIBEXECINFO)
  set(HAVE_BACKTRACE TRUE)
else()
  ac_check_funcs(backtrace)
endif()

if(UNIX)
  ac_check_headers(pthread.h)
  ac_check_lib(pthread pthread_mutex_init "" "")
  if(NOT ${HAVE_LIBPTHREAD} EQUAL 1)
    message(FATAL_ERROR "No libpthread found")
  endif()
endif()

option(WITH_ZLIB "use zlib for data compression." OFF)
if(WITH_ZLIB)
  ac_check_lib(z compress)
  if (NOT HAVE_LIBZ)
    message(FATAL_ERROR "No libz found")
  endif()
endif()

option(WITH_LZO "use LZO for data compression." OFF)
if(WITH_LZO)
  ac_check_lib(lzo2 lzo1_compress)
  if (NOT HAVE_LIBLZO2)
    message(FATAL_ERROR "No liblzo2 found")
  endif()
endif()

set(WITH_MECAB "auto"
  CACHE STRING "use MeCab for morphological analysis")
if(NOT ${WITH_MECAB} STREQUAL "no")
  set(MECAB_CONFIG "mecab-config" CACHE FILEPATH "mecab-config path")
  if(NOT CMAKE_CROSSCOMPILING)
    find_program(MECAB_CONFIG "${MECAB_CONFIG}")
  endif()
  if(EXISTS "${MECAB_CONFIG}")
    execute_process(COMMAND "${MECAB_CONFIG}" --inc-dir
      OUTPUT_VARIABLE MECAB_CPPFLAGS)
    execute_process(COMMAND "${MECAB_CONFIG}" --libs-only-L
      OUTPUT_VARIABLE MECAB_LDFLAGS)
    ac_check_lib(mecab mecab_new)
  else()
    set(WITH_MECAB FALSE)
    if(${WITH_MECAB} STREQUAL "yes")
      message(FATAL_ERROR "No MeCab found")
    endif()
  endif()
else()
  set(WITH_MECAB FALSE)
endif()

set(WITH_KYTEA "auto"
  CACHE STRING "use KyTea for morphological analysis")
if(NOT ${WITH_KYTEA} STREQUAL "no")
  pkg_check_modules(KYTEA kytea)
  set(WITH_KEYTEA KYTEA_FUOND)
  if(NOT WITH_KEYTEA AND ${WITH_KYTEA} STREQUAL "yes")
    message(FATAL_ERROR "No KyTea found")
  endif()
else()
  set(WITH_KEYTEA FALSE)
endif()

set(LIBGROONGA ${CMAKE_BINARY_DIR}/lib/libgroonga.so)

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(plugins)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
