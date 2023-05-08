# Copyright(C) 2012-2018  Brazil
# Copyright(C) 2018-2023  Sutou Kouhei <kou@clear-code.com>
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

# Ubuntu 20.04: 3.16
# CentOS 7's EPEL: 3.17
cmake_minimum_required(VERSION 3.16)

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/base_version" VERSION)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/version.sh")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.sh" GRN_VERSION)
else()
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/version.sh")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git" AND EXISTS "/bin/sh")
      execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/version-gen.sh"
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  endif()
  if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/version.sh")
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/version.sh" GRN_VERSION)
  else()
    set(GRN_VERSION "${VERSION}")
  endif()
endif()
string(REGEX REPLACE "(^.*=|\n)" "" GRN_VERSION "${GRN_VERSION}")
string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.([^.-]+)$" "\\1"
  GRN_VERSION_MAJOR "${VERSION}")
string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.([^.-]+)$" "\\2"
  GRN_VERSION_MINOR "${VERSION}")
string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.([^.-]+)$" "\\3"
  GRN_VERSION_MICRO "${VERSION}")
string(REGEX REPLACE "\\." "," GRN_VERSION_RC "${GRN_VERSION}")
string(REGEX REPLACE "-.*$" "" GRN_VERSION_RC "${GRN_VERSION_RC}")

project(groonga
  VERSION "${VERSION}"
  DESCRIPTION "An Embeddable Fulltext Search Engine"
  LANGUAGES  C CXX)
set(GRN_PROJECT_NAME "${PROJECT_NAME}")
set(GRN_PROJECT_LABEL "Groonga")

# https://cmake.org/cmake/help/latest/policy/CMP0054.html
# Only interpret if() arguments as variables or keywords when unquoted.
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  set(GRN_BUNDLED FALSE)
else()
  set(GRN_BUNDLED TRUE)
endif()

if(MSVC)
  if(MSVC_VERSION LESS 1800)
    set(GRN_OLD_MSVC_MESSAGE "Groonga supports only MSVC 2013 or later")
    if(GRN_BUNDLED)
      message(STATUS ${GRN_OLD_MSVC_MESSAGE})
      return()
    else()
      message(FATAL_ERROR ${GRN_OLD_MSVC_MESSAGE})
    endif()
  endif()
endif()

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_C_COMPILER_ID STREQUAL "Clang")
  set(GRN_C_COMPILER_GNU_LIKE TRUE)
else()
  set(GRN_C_COMPILER_GNU_LIKE FALSE)
endif()

configure_file(
  include/groonga/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/include/groonga/version.h)

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(${CMAKE_CURRENT_SOURCE_DIR}/build/cmake_modules/ReadFileList.cmake)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

find_package(PkgConfig)

if(DEFINED GRN_EMBED)
  set(GRN_EMBED_DEFAULT ${GRN_EMBED})
else()
  set(GRN_EMBED_DEFAULT OFF)
endif()
set(GRN_EMBED ${GRN_EMBED_DEFAULT} CACHE BOOL
  "Build as a static library to embed into an application")

set(GRN_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/groonga")
set(GRN_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Groonga")
if(NOT DEFINED GRN_DATA_DIR)
  set(GRN_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${GRN_PROJECT_NAME}")
endif()
set(GRN_CONFIG_DIR "${CMAKE_INSTALL_SYSCONFDIR}/${GRN_PROJECT_NAME}")
set(GRN_CONFIG_PATH "${CMAKE_INSTALL_FULL_SYSCONFDIR}/${GRN_PROJECT_NAME}/${GRN_PROJECT_NAME}.conf")

set(GRN_LOG_PATH
  "${CMAKE_INSTALL_FULL_LOCALSTATEDIR}/log/${GRN_PROJECT_NAME}/${GRN_PROJECT_NAME}.log"
  CACHE FILEPATH "log file path")
set(GRN_DEFAULT_ENCODING
  "utf8"
  CACHE STRING "Groonga's default encoding")
set(GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD
  0
  CACHE STRING "Groonga's default match escalation threshold")
set(GRN_DEFAULT_DOCUMENT_ROOT_BASE
  "html/admin"
  CACHE PATH "Groonga's default document root base path")
set(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT
  "${CMAKE_INSTALL_DATADIR}/${GRN_PROJECT_NAME}/${GRN_DEFAULT_DOCUMENT_ROOT_BASE}"
  CACHE PATH "Groonga's default relative document root")
set(GRN_DEFAULT_DOCUMENT_ROOT
  "${CMAKE_INSTALL_PREFIX}/${GRN_DATA_DIR}/${GRN_DEFAULT_DOCUMENT_ROOT_BASE}"
  CACHE PATH "Groonga's default document root")
set(GRN_DEFAULT_DB_KEY
  "auto"
  CACHE STRING "Groonga's default DB key management algorithm")
set(GRN_STACK_SIZE
  1024
  CACHE STRING
  "DANGER!!! Groonga's stack size. Normarlly, you should not change this variable.")
set(GRN_LOCK_TIMEOUT
  900000
  CACHE STRING
  "timeout to acquire a lock.")
set(GRN_LOCK_WAIT_TIME_NANOSECOND
  1000000
  CACHE STRING
  "wait time in nanosecond to acquire a lock.")
set(GRN_RELATIVE_PLUGINS_DIR
  "${CMAKE_INSTALL_LIBDIR}/${GRN_PROJECT_NAME}/plugins")
set(GRN_PLUGINS_DIR
  "${CMAKE_INSTALL_PREFIX}/${GRN_RELATIVE_PLUGINS_DIR}")
set(GRN_PLUGIN_SUFFIX "${CMAKE_SHARED_MODULE_SUFFIX}")
set(GRN_DLL_FILENAME
  "libgroonga${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE
  "${GRN_CONFIG_DIR}/synonyms.tsv")
set(GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE
  "${CMAKE_INSTALL_PREFIX}/${GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE}")
set(GRN_RELATIVE_RUBY_SCRIPTS_DIR
  "${CMAKE_INSTALL_LIBDIR}/${GRN_PROJECT_NAME}/scripts/ruby")
set(GRN_RUBY_SCRIPTS_DIR
  "${CMAKE_INSTALL_PREFIX}/${GRN_RELATIVE_RUBY_SCRIPTS_DIR}")


if(GRN_C_COMPILER_GNU_LIKE)
  set(GRN_C_COMPILE_FLAGS
    "${GRN_C_COMPILE_FLAGS} $<$<CONFIG:Debug>:-g3 -O0>")
  set(GRN_CXX_COMPILE_FLAGS
    "${GRN_CXX_COMPILE_FLAGS} $<$<CONFIG:Debug>:-g3 -O0>")
  set(GRN_C_COMPILE_FLAGS
    "${GRN_C_COMPILE_FLAGS} $<$<CONFIG:RelWithDebInfo>:-g3>")
  set(GRN_CXX_COMPILE_FLAGS
    "${GRN_CXX_COMPILE_FLAGS} $<$<CONFIG:RelWithDebInfo>:-g3>")
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED TRUE)
endif()

# /Ob1 is used with RelWithDebInfo by default but it's slower than
# /Ob2 which is the default with Release. We need performance with
# debug information.
if(MSVC)
  set(GRN_C_COMPILE_FLAGS
    "${GRN_C_COMPILE_FLAGS} $<$<CONFIG:RelWithDebInfo>:/Ob2>")
  set(GRN_CXX_COMPILE_FLAGS
    "${GRN_CXX_COMPILE_FLAGS} $<$<CONFIG:RelWithDebInfo>:/Ob2>")
endif()


macro(check_cflag flag)
  string(REGEX REPLACE "[-=]" "_" temporary_variable_name ${flag})
  string(TOUPPER "${temporary_variable_name}" temporary_variable_name)
  set(temporary_variable_name "CFLAG${temporary_variable_name}")
  check_c_compiler_flag(${flag} ${temporary_variable_name})
  if(${temporary_variable_name})
    set(GRN_C_COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS} ${flag}")
  endif()
endmacro()

macro(check_cxxflag flag)
  string(REGEX REPLACE "[-=]" "_" temporary_variable_name ${flag})
  string(TOUPPER "${temporary_variable_name}" temporary_variable_name)
  set(temporary_variable_name "CXXFLAG${temporary_variable_name}")
  check_cxx_compiler_flag(${flag} ${temporary_variable_name})
  if(${temporary_variable_name})
    set(GRN_CXX_COMPILE_FLAGS "${GRN_CXX_COMPILE_FLAGS} ${flag}")
  endif()
endmacro()

macro(check_build_flag flag)
  check_cflag(${flag})
  check_cxxflag(${flag})
endmacro()

if(GRN_C_COMPILER_GNU_LIKE)
  check_build_flag("-Wall")
  check_build_flag("-Wno-unused-but-set-variable")
  check_build_flag("-Wno-unused-parameter")
  check_cflag("-Wno-pointer-sign")
  check_build_flag("-Wformat")
  check_build_flag("-Wno-format-truncation")
  check_build_flag("-Wstrict-aliasing=2")
  check_build_flag("-fno-strict-aliasing")
  check_build_flag("-Wno-disabled-optimization")
  check_build_flag("-Wpointer-arith")
  check_cflag("-Wbad-function-cast")
  check_build_flag("-Wwrite-strings")
  check_build_flag("-Wsign-compare")
  check_build_flag("-Wmissing-field-initializers")
  check_cflag("-Wno-declaration-after-statement")
  check_cxxflag("-fexceptions")
  check_cxxflag("-fimplicit-templates")
  check_build_flag("-Wno-implicit-fallthrough")
endif()

option(GRN_ALLOW_WARNING "allow warning." ON)
if(NOT GRN_ALLOW_WARNING)
  if(GRN_C_COMPILER_GNU_LIKE)
    set(GRN_C_COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS} -Werror")
    set(GRN_CXX_COMPILE_FLAGS "${GRN_CXX_COMPILE_FLAGS} -Werror")
  elseif(MSVC)
    set(GRN_C_COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS} /WX")
    set(GRN_CXX_COMPILE_FLAGS "${GRN_CXX_COMPILE_FLAGS} /WX")
  endif()
endif()


option(GRN_WITH_WINDOWS_BACK_TRACE "enable back trace on Windows." ON)

add_library(grn_dependencies INTERFACE)
if(GRN_EMBED)
  target_compile_definitions(grn_dependencies INTERFACE GRN_EMBEDDED)
endif()

if(GRN_C_COMPILER_GNU_LIKE)
  set(_GNU_SOURCE TRUE)
endif()

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

macro(ac_check_symbols symbol files)
  string(TOUPPER "${symbol}" output_variable_name)
  set(output_variable_name "HAVE_${output_variable_name}")
  check_symbol_exists(${symbol} ${files} ${output_variable_name})
endmacro()

macro(ac_check_lib library function)
  string(REGEX REPLACE "[/.]" "_" output_variable_base_name ${library})
  string(TOUPPER "${output_variable_base_name}" output_variable_base_name)
  set(output_variable_name "HAVE_LIB${output_variable_base_name}")
  set(location "${ARG2}")
  check_library_exists(${library} ${function} "${location}"
    ${output_variable_name})
  if(${output_variable_name})
    target_link_libraries(grn_dependencies INTERFACE "${library}")
  endif()
endmacro()

include(build/ac_macros/check_headers.m4)
include(build/ac_macros/check_functions.m4)

ac_check_symbols(fpclassify math.h)
ac_check_lib(m fpclassify)

ac_check_lib(execinfo backtrace)
if(HAVE_LIBEXECINFO)
  set(HAVE_BACKTRACE TRUE)
else()
  ac_check_funcs(backtrace)
endif()
ac_check_lib(rt clock_gettime)
if(HAVE_LIBRT)
  set(HAVE_CLOCK_GETTIME TRUE)
endif()
if(NOT (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64" OR
      CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^i[0-9]86$"))
  ac_check_lib(atomic __atomic_store_8)
endif()

check_struct_has_member(
  "struct tm"
  "tm_gmtoff"
  "time.h"
  HAVE_STRUCT_TM_TM_GMTOFF)

find_package(Threads REQUIRED)
set(HAVE_PTHREAD_H ${CMAKE_USE_PTHREADS_INIT})

option(GRN_WITH_NFKC "use NFKC based UTF8 normalization." ON)

if(WIN32)
  target_link_libraries(grn_dependencies INTERFACE dbghelp.lib)
  target_link_libraries(grn_dependencies INTERFACE psapi.lib)

  set(USE_EXACT_ALLOC_COUNT TRUE)
  set(USE_SELECT TRUE)
else()
  ac_check_headers(sys/epoll.h)
  if(${HAVE_SYS_EPOLL_H})
    ac_check_funcs(epoll_create)
    if(${HAVE_EPOLL_CREATE})
      set(USE_EPOLL TRUE)
    endif()
  endif()

  if(NOT USE_EPOLL)
    ac_check_headers(sys/event.h)
    if(${HAVE_SYS_EVENT_H})
      ac_check_funcs(kevent)
      if(${HAVE_KEVENT})
        set(USE_KQUEUE TRUE)
      endif()
    endif()

    if(NOT USE_KQUEUE)
      ac_check_headers(poll.h)
      if(${HAVE_SYS_POLL_H})
        ac_check_funcs(poll)
        if(${HAVE_POLL})
          set(USE_POLL TRUE)
        endif()
      endif()

      if(NOT USE_POLL)
        ac_check_funcs(select)
        if(${HAVE_SELECT})
          set(USE_SELECT TRUE)
          ac_check_headers(sys/select.h)
        endif()

        if(NOT USE_SELECT)
          message(FATAL_ERROR "All epoll/kqueue/poll/select are missing")
        endif()
      endif()
    endif()
  endif()
endif()

option(GRN_WITH_APACHE_ARROW "use Apache Arrow" OFF)
if(GRN_WITH_APACHE_ARROW)
  find_package(Arrow REQUIRED)
  target_link_libraries(grn_dependencies INTERFACE Arrow::arrow_shared)
endif()

set(GRN_WITH_ZLIB "auto"
  CACHE STRING "Support data compression by zlib.")
if(NOT ${GRN_WITH_ZLIB} STREQUAL "no")
  # This is for building in MariaDB.
  if(TARGET zlib)
    target_link_libraries(grn_dependencies INTERFACE zlib)
    set(GRN_WITH_ZLIB TRUE)
  else()
    if(${GRN_WITH_ZLIB} STREQUAL "yes")
      find_package(ZLIB REQUIRED)
    else()
      find_package(ZLIB)
    endif()
    if(ZLIB_FOUND)
      target_link_libraries(grn_dependencies INTERFACE ZLIB::ZLIB)
    endif()
    set(GRN_WITH_ZLIB ${ZLIB_FOUND})
  endif()
endif()


include(ExternalProject)
set(GRN_EP_COMMON_OPTIONS
  LOG_DOWNLOAD TRUE
  LOG_UPDATE TRUE
  LOG_PATCH TRUE
  LOG_CONFIGURE TRUE
  LOG_BUILD TRUE
  LOG_INSTALL TRUE
  LOG_TEST TRUE
  LOG_MERGED_STDOUTERR TRUE
  LOG_OUTPUT_ON_FAILURE TRUE)

set(GRN_ZSTD_EP_VERSION "1.4.8")
set(GRN_WITH_ZSTD "auto"
  CACHE STRING "Support data compression by Zstandard.")
if(NOT "${GRN_WITH_ZSTD}" STREQUAL "no")
  if("${GRN_WITH_ZSTD}" STREQUAL "bundled")
    set(Groongazstd_FOUND FALSE)
  else()
    if("${GRN_WITH_ZSTD}" STREQUAL "system")
      find_package(Groongazstd REQUIRED)
    else()
      find_package(Groongazstd)
    endif()
  endif()
  if(Groongazstd_FOUND)
    set(GRN_WITH_ZSTD TRUE)
    message(STATUS "Zstandard: system")
  else()
    set(ZSTD_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/zstd-ep-install")
    if(MSVC)
      set(ZSTD_LIBRARY_BASE_NAME zstd_static)
    else()
      set(ZSTD_LIBRARY_BASE_NAME zstd)
    endif()
    set(ZSTD_STATIC_LIBRARY "${ZSTD_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    string(APPEND ZSTD_STATIC_LIBRARY "/${CMAKE_STATIC_LIBRARY_PREFIX}")
    string(APPEND ZSTD_STATIC_LIBRARY "${ZSTD_LIBRARY_BASE_NAME}")
    string(APPEND ZSTD_STATIC_LIBRARY "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(ZSTD_CMAKE_ARGS
      -DCMAKE_INSTALL_PREFIX=${ZSTD_PREFIX}
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DZSTD_BUILD_PROGRAMS=OFF
      -DZSTD_BUILD_SHARED=OFF
      -DZSTD_BUILD_STATIC=ON
      -DZSTD_USE_STATIC_RUNTIME=ON)
    set(ZSTD_SOURCE_BASE_NAME "zstd-${GRN_ZSTD_EP_VERSION}.tar.gz")
    set(ZSTD_SOURCE_LOCAL_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${ZSTD_SOURCE_BASE_NAME}")
    if(EXISTS ${ZSTD_SOURCE_LOCAL_PATH})
      set(ZSTD_SOURCE_URL ${ZSTD_SOURCE_LOCAL_PATH})
    else()
      set(ZSTD_SOURCE_URL
        "https://github.com/facebook/zstd/releases/download")
      string(APPEND ZSTD_SOURCE_URL "/v${GRN_ZSTD_EP_VERSION}")
      string(APPEND ZSTD_SOURCE_URL "/${ZSTD_SOURCE_BASE_NAME}")
    endif()
    externalproject_add(zstd-ep
      ${GRN_EP_COMMON_OPTIONS}
      CMAKE_ARGS ${ZSTD_CMAKE_ARGS}
      SOURCE_SUBDIR "build/cmake"
      URL ${ZSTD_SOURCE_URL}
      BUILD_BYPRODUCTS ${ZSTD_STATIC_LIBRARY})
    file(MAKE_DIRECTORY "${ZSTD_PREFIX}/include")
    add_library(Groonga::libzstd STATIC IMPORTED)
    set_target_properties(Groonga::libzstd PROPERTIES
      IMPORTED_LOCATION ${ZSTD_STATIC_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_PREFIX}/include")
    add_dependencies(Groonga::libzstd zstd-ep)
    set(ZSTD_SOURCE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/zstd-ep-prefix/src/zstd-ep")
    install(FILES
      "${ZSTD_SOURCE_DIR}/CHANGELOG"
      "${ZSTD_SOURCE_DIR}/COPYING"
      "${ZSTD_SOURCE_DIR}/LICENSE"
      "${ZSTD_SOURCE_DIR}/README.md"
      DESTINATION "${GRN_DATA_DIR}/zstd")
    set(GRN_WITH_ZSTD TRUE)
    message(STATUS "Zstandard: ${ZSTD_STATIC_LIBRARY} (bundled)")
  endif()
  target_link_libraries(grn_dependencies INTERFACE Groonga::libzstd)
else()
  set(GRN_WITH_ZSTD FALSE)
endif()


file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_rapidjson_version"
  GRN_BUNDLED_RAPIDJSON_VERSION)
string(STRIP
  "${GRN_BUNDLED_RAPIDJSON_VERSION}"
  GRN_BUNDLED_RAPIDJSON_VERSION)
set(GRN_RAPIDJSON_EP_VERSION ${GRN_BUNDLED_RAPIDJSON_VERSION})

set(GRN_WITH_RAPIDJSON "auto"
  CACHE STRING "Support JSON processing by RapidJSON.")
if(NOT "${GRN_WITH_RAPIDJSON}" STREQUAL "no")
  find_package(GroongaRapidJSON)
  if(GroongaRapidJSON_FOUND)
    set(GRN_WITH_RAPIDJSON TRUE)
    target_link_libraries(grn_dependencies INTERFACE Groonga::RapidJSON)
  else()
    set(RAPIDJSON_SOURCE_URL
      "https://github.com/Tencent/rapidjson/archive/v${GRN_RAPIDJSON_EP_VERSION}.tar.gz")
    set(RAPIDJSON_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/rapidjson-ep-install")
    set(RAPIDJSON_CMAKE_ARGS
      -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
      -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
      -DCMAKE_INSTALL_PREFIX=${RAPIDJSON_PREFIX}
      -DRAPIDJSON_BUILD_DOC=OFF
      -DRAPIDJSON_BUILD_EXAMPLES=OFF
      -DRAPIDJSON_BUILD_TESTS=OFF)
    externalproject_add(rapidjson-ep
      ${GRN_EP_COMMON_OPTIONS}
      CMAKE_ARGS ${RAPIDJSON_CMAKE_ARGS}
      URL ${RAPIDJSON_SOURCE_URL})
    add_library(Groonga::RapidJSON INTERFACE IMPORTED)
    file(MAKE_DIRECTORY "${RAPIDJSON_PREFIX}/include")
    target_include_directories(Groonga::RapidJSON INTERFACE
      "${RAPIDJSON_PREFIX}/include")
    add_dependencies(Groonga::RapidJSON rapidjson-ep)
    set(RAPIDJSON_SOURCE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/rapidjson-ep-prefix/src/rapidjson-ep")
    install(FILES
      "${RAPIDJSON_SOURCE_DIR}/CHANGELOG.md"
      "${RAPIDJSON_SOURCE_DIR}/license.txt"
      "${RAPIDJSON_SOURCE_DIR}/readme.md"
      DESTINATION "${GRN_DATA_DIR}/rapidjson")
    set(GRN_WITH_RAPIDJSON TRUE)
    target_link_libraries(grn_dependencies INTERFACE Groonga::RapidJSON)
  endif()
else()
  set(GRN_WITH_RAPIDJSON FALSE)
endif()


file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_mecab_version"
  GRN_BUNDLED_MECAB_VERSION)
string(STRIP
  "${GRN_BUNDLED_MECAB_VERSION}"
  GRN_BUNDLED_MECAB_VERSION)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_mecab_naist_jdic_version"
  GRN_BUNDLED_MECAB_NAIST_JDIC_VERSION)
string(STRIP
  "${GRN_BUNDLED_MECAB_NAIST_JDIC_VERSION}"
  GRN_BUNDLED_MECAB_NAIST_JDIC_VERSION)

set(GRN_WITH_BUNDLED_MECAB FALSE)
set(GRN_WITH_MECAB "auto"
  CACHE STRING "use MeCab for morphological analysis")
if(NOT "${GRN_WITH_MECAB}" STREQUAL "no")
  if(WIN32)
    find_path(MECAB_INCLUDE_DIR NAMES mecab.h)
    find_library(MECAB_LIBRARIES NAMES mecab)
  else()
    set(GRN_MECAB_CONFIG "mecab-config" CACHE FILEPATH "mecab-config path")
    find_program(GRN_MECAB_CONFIG_ABSOLUTE_PATH "${GRN_MECAB_CONFIG}")
    if(EXISTS "${GRN_MECAB_CONFIG_ABSOLUTE_PATH}")
      execute_process(COMMAND "${GRN_MECAB_CONFIG_ABSOLUTE_PATH}" --inc-dir
        OUTPUT_VARIABLE MECAB_INCLUDE_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      execute_process(COMMAND "${GRN_MECAB_CONFIG_ABSOLUTE_PATH}" --libs-only-L
        OUTPUT_VARIABLE MECAB_LIBRARY_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      find_library(MECAB_LIBRARIES
        NAMES mecab
        PATHS ${MECAB_LIBRARY_DIRS}
        NO_DEFAULT_PATH)
    endif()
  endif()
  if(MECAB_LIBRARIES)
    set(GRN_WITH_MECAB TRUE)
    add_library(libmecab INTERFACE IMPORTED)
    target_include_directories(libmecab INTERFACE ${MECAB_INCLUDE_DIR})
    target_link_libraries(libmecab INTERFACE ${MECAB_LIBRARIES})
    message(STATUS "MeCab: ${MECAB_LIBRARIES}")
  else()
    set(GRN_BUNDLED_MECAB_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mecab-${GRN_BUNDLED_MECAB_VERSION}")
    if(EXISTS ${GRN_BUNDLED_MECAB_DIR})
      set(GRN_WITH_MECAB TRUE)
      set(GRN_WITH_BUNDLED_MECAB TRUE)
      message(STATUS "MeCab: (bundled)")
    else()
      if("${GRN_WITH_MECAB}" STREQUAL "yes")
        if(WIN32)
          message(FATAL_ERROR
            "No MeCab found: "
            "include directories: <${MECAB_INCLUDE_DIRS}>, "
            "library directories: <${MECAB_LIBRARY_DIRS}>")
        else()
          message(FATAL_ERROR
            "No MeCab found: "
            "include directories: <${MECAB_INCLUDE_DIRS}>, "
            "library directories: <${MECAB_LIBRARY_DIRS}>, "
            "mecab-config: <${GRN_MECAB_CONFIG_ABSOLUTE_PATH}>")
        endif()
      endif()
      set(GRN_WITH_MECAB FALSE)
      message(STATUS "No MeCab")
    endif()
  endif()
else()
  set(GRN_WITH_MECAB FALSE)
endif()

set(GRN_WITH_KYTEA "auto"
  CACHE STRING "use KyTea for morphological analysis")
if(NOT ${GRN_WITH_KYTEA} STREQUAL "no")
  if(PkgConfig_FOUND)
    pkg_check_modules(pkg_kytea kytea IMPORTED_TARGET)
    endif()
  if(pkg_kytea_FOUND)
    set(GRN_WITH_KYTEA TRUE)
    add_library(kytea ALIAS PkgConfig::pkg_kytea)
  else()
    if(${GRN_WITH_KYTEA} STREQUAL "yes")
      message(FATAL_ERROR "No KyTea found")
    endif()
    set(GRN_WITH_KYTEA FALSE)
  endif()
else()
  set(GRN_WITH_KYTEA FALSE)
endif()

set(GRN_WITH_LIBSTEMMER "auto"
  CACHE STRING "use libstemmer for stemming token filter")
if(NOT ${GRN_WITH_LIBSTEMMER} STREQUAL "no")
  if("${GRN_WITH_LIBSTEMMER}" STREQUAL "yes")
    find_package(Groongalibstemmer REQUIRED)
  else()
    find_package(Groongalibstemmer)
  endif()
  if(Groongalibstemmer_FOUND)
    set(GRN_WITH_LIBSTEMMER TRUE)
  else()
    set(GRN_WITH_LIBSTEMMER FALSE)
  endif()
else()
  set(GRN_WITH_LIBSTEMMER FALSE)
endif()

set(GRN_WITH_ZEROMQ "auto"
  CACHE STRING "use ZeroMQ for suggestion")
if(NOT ${GRN_WITH_ZEROMQ} STREQUAL "no")
  if(PkgConfig_FOUND)
    pkg_check_modules(pkg_libzmq libzmq IMPORTED_TARGET)
  endif()
  if(pkg_libzmq_FOUND)
    set(GRN_WITH_ZEROMQ TRUE)
    add_library(Groonga::zeromq ALIAS PkgConfig::pkg_libzmq)
  else()
    if(${GRN_WITH_ZEROMQ} STREQUAL "yes")
      message(FATAL_ERROR "No ZeroMQ found")
    endif()
    set(GRN_WITH_ZEROMQ FALSE)
  endif()
else()
  set(GRN_WITH_ZEROMQ FALSE)
endif()

set(GRN_WITH_LIBEVENT "auto"
  CACHE STRING "use libevent for suggestion")
if(NOT ${GRN_WITH_LIBEVENT} STREQUAL "no")
  if(PkgConfig_FOUND)
    pkg_check_modules(pkg_libevent libevent IMPORTED_TARGET)
  endif()
  if(pkg_libevent_FOUND)
    set(GRN_WITH_LIBEVENT TRUE)
    add_library(Groonga::libevent ALIAS PkgConfig::pkg_libevent)
  else()
    if(${GRN_WITH_LIBEVENT} STREQUAL "yes")
      message(FATAL_ERROR "No libevent found")
    endif()
    set(GRN_WITH_LIBEVENT FALSE)
  endif()
else()
  set(GRN_WITH_LIBEVENT FALSE)
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_message_pack_version"
  GRN_BUNDLED_MESSAGE_PACK_VERSION)
string(STRIP
  "${GRN_BUNDLED_MESSAGE_PACK_VERSION}"
  GRN_BUNDLED_MESSAGE_PACK_VERSION)

set(GRN_WITH_BUNDLED_MESSAGE_PACK FALSE)
set(GRN_WITH_MESSAGE_PACK "auto"
  CACHE STRING "use MessagePack for suggestion")
if(NOT ${GRN_WITH_MESSAGE_PACK} STREQUAL "no")
  find_package(Groongamsgpackc)
  if(Groongamsgpackc_FOUND)
    set(GRN_WITH_MESSAGE_PACK TRUE)
    target_link_libraries(grn_dependencies INTERFACE Groonga::msgpackc)
    if(NOT GRN_EMBED)
      install(FILES "cmake/FindGroongamsgpackc.cmake"
	DESTINATION "${GRN_INSTALL_CMAKE_DIR}")
    endif()
  else()
    set(GRN_BUNDLED_MESSAGE_PACK_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/msgpack-${GRN_BUNDLED_MESSAGE_PACK_VERSION}")
    if(EXISTS "${GRN_BUNDLED_MESSAGE_PACK_DIR}")
      set(GRN_WITH_BUNDLED_MESSAGE_PACK TRUE)
      set(GRN_WITH_MESSAGE_PACK TRUE)
      target_link_libraries(grn_dependencies INTERFACE msgpackc)
      message(STATUS "MessagePack: (bundled)")
    else()
      if(${GRN_WITH_MESSAGE_PACK} STREQUAL "yes")
        message(FATAL_ERROR "No MessagePack found")
      endif()
      set(GRN_WITH_MESSAGE_PACK FALSE)
      message(STATUS "No MessagePack")
    endif()
  endif()
else()
  set(GRN_WITH_MESSAGE_PACK FALSE)
endif()


file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_xxhash_version"
  GRN_BUNDLED_XXHASH_VERSION)
string(STRIP
  "${GRN_BUNDLED_XXHASH_VERSION}"
  GRN_BUNDLED_XXHASH_VERSION)

set(GRN_WITH_BUNDLED_XXHASH FALSE)
set(GRN_WITH_XXHASH "auto"
  CACHE STRING "Support drilldown by hashed key with xxHash.")
if(NOT "${GRN_WITH_XXHASH}" STREQUAL "no")
  if("${GRN_WITH_XXHASH}" STREQUAL "bundled")
    set(GroongaxxHash_FOUND FALSE)
  else()
    if("${GRN_WITH_XXHASH}" STREQUAL "system")
      find_package(GroongaxxHash REQUIRED)
    else()
      find_package(GroongaxxHash)
    endif()
  endif()
  if(GroongaxxHash_FOUND)
    set(GRN_WITH_XXHASH TRUE)
    target_link_libraries(grn_dependencies INTERFACE Groonga::xxhash)
  else()
    set(GRN_BUNDLED_XXHASH_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/xxHash-${GRN_BUNDLED_XXHASH_VERSION}")
    if(EXISTS ${GRN_BUNDLED_XXHASH_DIR})
      set(GRN_WITH_BUNDLED_XXHASH TRUE)
      set(GRN_WITH_XXHASH TRUE)
      add_library(xxhash INTERFACE)
      target_include_directories(xxhash BEFORE INTERFACE ${GRN_BUNDLED_XXHASH_DIR})
      target_compile_definitions(xxhash INTERFACE XXH_INLINE_ALL)
      target_link_libraries(grn_dependencies INTERFACE xxhash)
      message(STATUS "xxHash: (bundled)")
      install(FILES
        "${GRN_BUNDLED_XXHASH_DIR}/LICENSE"
        "${GRN_BUNDLED_XXHASH_DIR}/README.md"
        DESTINATION "${GRN_DATA_DIR}/xxhash")
    else()
      if(${GRN_WITH_XXHASH} STREQUAL "yes")
        message(FATAL_ERROR "No xxHash found")
      endif()
      set(GRN_WITH_XXHASH FALSE)
      message(STATUS "No xxHash")
    endif()
  endif()
else()
  set(GRN_WITH_XXHASH FALSE)
endif()

# We need to find LZ4 after xxHash because bundled LZ4 includes its
# xxhash.h.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_lz4_version"
  GRN_BUNDLED_LZ4_VERSION)
string(STRIP
  "${GRN_BUNDLED_LZ4_VERSION}"
  GRN_BUNDLED_LZ4_VERSION)

set(GRN_WITH_BUNDLED_LZ4 FALSE)
set(GRN_WITH_LZ4 "auto"
  CACHE STRING "Support data compression by LZ4.")
if(NOT "${GRN_WITH_LZ4}" STREQUAL "no")
  if("${GRN_WITH_LZ4}" STREQUAL "bundled")
    set(Groongalz4_FOUND FALSE)
  else()
    if("${GRN_WITH_XXHASH}" STREQUAL "system")
      find_package(Groongalz4 REQUIRED)
    else()
      find_package(Groongalz4)
    endif()
  endif()
  if(Groongalz4_FOUND)
    set(GRN_WITH_LZ4 TRUE)
    target_link_libraries(grn_dependencies INTERFACE Groonga::liblz4)
  else()
    set(GRN_BUNDLED_LZ4_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lz4-${GRN_BUNDLED_LZ4_VERSION}")
    if(EXISTS ${GRN_BUNDLED_LZ4_DIR})
      set(GRN_WITH_BUNDLED_LZ4 TRUE)
      set(GRN_WITH_LZ4 TRUE)
      target_link_libraries(grn_dependencies INTERFACE liblz4)
      message(STATUS "LZ4: (bundled)")
    else()
      if("${GRN_WITH_LZ4}" STREQUAL "yes")
        message(FATAL_ERROR "No LZ4 found")
      endif()
      set(GRN_WITH_LZ4 FALSE)
      message(STATUS "No LZ4")
    endif()
  endif()
else()
  set(GRN_WITH_LZ4 FALSE)
endif()


find_program(RUBY NAMES
  "ruby"
  "ruby3.0" "ruby30"
  "ruby2.7" "ruby27"
  "ruby2.6" "ruby26"
  "ruby2.5" "ruby25"
  "ruby2.4" "ruby24"
  "ruby2.3" "ruby23"
  "ruby2.2" "ruby22"
  "ruby2.1" "ruby21")

option(GRN_WITH_MRUBY "use mruby" OFF)

# For mruby-file-stat
if(NOT WIN32)
  ac_check_headers(sys/sysmacros.h)
endif()

set(GRN_WITH_ONIGMO ON)
option(GRN_WITH_BUNDLED_ONIGMO "use bundled Onigmo" ON)
if(NOT GRN_WITH_BUNDLED_ONIGMO)
  pkg_check_modules(pkg_onigmo onigmo REQUIRED IMPORTED_TARGET)
  add_library(onigmo ALIAS PkgConfig::pkg_onigmo)
endif()

add_subdirectory(vendor)
if(GRN_WITH_MRUBY)
  target_link_libraries(grn_dependencies INTERFACE mruby)
endif()
target_link_libraries(grn_dependencies INTERFACE onigmo)
if(GRN_EMBED)
  add_subdirectory(plugins)
endif()
add_subdirectory(lib)
if(NOT GRN_EMBED)
  add_subdirectory(src)
  add_subdirectory(plugins)
  add_subdirectory(include)
  add_subdirectory(data)
endif()

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(GROONGA "${CMAKE_CURRENT_BINARY_DIR}/src/groonga")
set(GROONGA_SUGGEST_CREATE_DATASET
  "${CMAKE_CURRENT_BINARY_DIR}/src/suggest/groonga-suggest-create-dataset")
set(GROONGA_BENCHMARK "${CMAKE_CURRENT_BINARY_DIR}/src/groonga-benchmark")
configure_file(config.sh.in "${CMAKE_CURRENT_BINARY_DIR}/config.sh" @ONLY)

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
set(bindir "\${exec_prefix}/${CMAKE_INSTALL_BINDIR}")
set(sbindir "\${exec_prefix}/${CMAKE_INSTALL_SBINDIR}")
set(libdir "\${prefix}/${CMAKE_ISNTALL_LIBDIR}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set(datarootdir "\${prefix}/${CMAKE_INSTALL_DATAROOTDIR}")
set(datadir "\${datarootdir}")
set(expanded_pluginsdir "${GRN_PLUGINS_DIR}")
set(GRN_EXPANDED_DEFAULT_DOCUMENT_ROOT "${GRN_DEFAULT_DOCUMENT_ROOT}")
set(EXEEXT "${CMAKE_EXECUTABLE_SUFFIX}")
configure_file(groonga.pc.in "${CMAKE_CURRENT_BINARY_DIR}/groonga.pc" @ONLY)

if(NOT GRN_EMBED)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/groonga.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
endif()

install(FILES
  "COPYING"
  "README.md"
  DESTINATION "${GRN_DATA_DIR}")

add_subdirectory(vendor/plugins)
