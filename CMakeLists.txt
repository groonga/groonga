# Copyright(C) 2012-2018 Brazil
# Copyright(C) 2018-2023 Sutou Kouhei <kou@clear-code.com>
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
      file(READ "${CMAKE_CURRENT_BINARY_DIR}/version.sh" GRN_VERSION)
    else()
      set(GRN_VERSION "${VERSION}")
    endif()
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
  VERSION "${GROONGA_VERSION}"
  DESCRIPTION "An Embeddable Fulltext Search Engine"
  LANGUAGES  C CXX)
set(GRN_PROJECT_LABEL "Groonga")

# https://cmake.org/cmake/help/latest/policy/CMP0054.html
# Only interpret if() arguments as variables or keywords when unquoted.
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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

find_package(PkgConfig)

if(DEFINED GRN_EMBED)
  set(GRN_EMBED_DEFAULT ${GRN_EMBED})
else()
  set(GRN_EMBED_DEFAULT OFF)
endif()
set(GRN_EMBED ${GRN_EMBED_DEFAULT} CACHE BOOL
  "Build as a static library to embed into an application")

set(BIN_DIR "bin")
set(SBIN_DIR "sbin")
set(LIB_DIR "lib")
set(INCLUDE_DIR "include")
set(GRN_INCLUDE_DIR "include/groonga")
set(DATA_DIR "share")
if(NOT DEFINED GRN_DATA_DIR)
  set(GRN_DATA_DIR "${DATA_DIR}/${GRN_PROJECT_NAME}")
endif()
set(CONFIG_DIR "etc")
set(GRN_CONFIG_DIR "${CONFIG_DIR}/${GRN_PROJECT_NAME}")
set(GRN_CONFIG_PATH "${CMAKE_INSTALL_PREFIX}/${GRN_CONFIG_DIR}/groonga.conf")

set(GRN_LOG_PATH
  "${CMAKE_INSTALL_PREFIX}/var/log/${GRN_PROJECT_NAME}/${GRN_PROJECT_NAME}.log"
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
  "share/${GRN_PROJECT_NAME}/${GRN_DEFAULT_DOCUMENT_ROOT_BASE}"
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
  "${LIB_DIR}/${GRN_PROJECT_NAME}/plugins")
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
  "${LIB_DIR}/${GRN_PROJECT_NAME}/scripts/ruby")
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
  if(CMAKE_VERSION VERSION_LESS 3.1)
    set(GRN_C_COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS} -std=gnu99")
  else()
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED TRUE)
  endif()
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

add_definitions(
  -DHAVE_CONFIG_H
  )
if(GRN_EMBED)
  add_definitions(-DGRN_EMBEDDED)
endif()

if(GRN_C_COMPILER_GNU_LIKE)
  set(_GNU_SOURCE TRUE)
endif()

include_directories(
  BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/lib
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
    set(${output_variable_base_name}_LIBS "${library}")
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
if(CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64" OR
    CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^i[0-9]86$")
  set(ATOMIC_LIBS)
else()
  ac_check_lib(atomic __atomic_store_8)
endif()
if(GRN_EMBED)
  check_library_exists(stdc++ __cxa_begin_catch "${ARG2}"
    STDCPP)
  if(STDCPP)
    set(STDCPP_LIBS "stdc++")
  endif()
endif()

check_struct_has_member(
  "struct tm"
  "tm_gmtoff"
  "time.h"
  HAVE_STRUCT_TM_TM_GMTOFF)

if(UNIX)
  ac_check_headers(pthread.h)
  ac_check_lib(pthread pthread_mutex_init)
  if(NOT ${HAVE_LIBPTHREAD})
    message(FATAL_ERROR "No libpthread found")
  endif()
  ac_check_funcs(pthread_mutexattr_setpshared)
  ac_check_funcs(pthread_condattr_setpshared)
endif()

option(GRN_WITH_NFKC "use NFKC based UTF8 normalization." ON)

if(WIN32)
  ac_check_headers(winsock2.h)
  if(NOT ${HAVE_WINSOCK2_H})
    message(FATAL_ERROR "No winsock2.h found")
  endif()

  # FIXME: CMake couldn't detect ws2_32.lib on Windows 8 64bit.
  # It may be caused by missing library search path for ws2_32.lib.
  # It seems that Visual Studio (devenv.exe) can find it but link.exe
  # can't. "cmake --build" can find it because it uses Visual Studio
  # internally. So we assume that we always have ws2_32.lib on Windows.
  # ac_check_lib(ws2_32 select)
  set(HAVE_LIBWS2_32 TRUE)
  set(WS2_32_LIBS "ws2_32.lib")

  SET(DBGHELP_LIBS "dbghelp.lib")

  SET(PSAPI_LIBS "psapi.lib")

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
endif()

set(GRN_WITH_ZLIB "auto"
  CACHE STRING "Support data compression by zlib.")
if(NOT ${GRN_WITH_ZLIB} STREQUAL "no")
  ac_check_lib(z compress)
  if(NOT HAVE_LIBZ)
    if(${GRN_WITH_ZLIB} STREQUAL "yes")
      message(FATAL_ERROR "No libz found")
    endif()
    set(GRN_WITH_ZLIB "no")
  endif()
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_lz4_version"
  GRN_BUNDLED_LZ4_VERSION)
string(STRIP
  "${GRN_BUNDLED_LZ4_VERSION}"
  GRN_BUNDLED_LZ4_VERSION)

set(GRN_WITH_BUNDLED_LZ4 FALSE)
set(GRN_WITH_LZ4 "auto"
  CACHE STRING "Support data compression by LZ4.")
if(NOT ${GRN_WITH_LZ4} STREQUAL "no")
  if(MSVC)
    set(LIBLZ4_FOUND FALSE)
  else()
    if(NOT DEFINED LIBLZ4_FOUND)
      pkg_check_modules(LIBLZ4 liblz4)
      unset(LIBLZ4_LIBS)
      unset(LIBLZ4_LIBS CACHE)
    endif()
  endif()
  if(LIBLZ4_FOUND)
    if(${LIBLZ4_LIBRARY_DIRS})
      find_library(LIBLZ4_LIBS
	NAMES ${LIBLZ4_LIBRARIES}
	PATHS ${LIBLZ4_LIBRARY_DIRS}
	NO_DEFAULT_PATH)
    else()
      find_library(LIBLZ4_LIBS
	NAMES ${LIBLZ4_LIBRARIES})
    endif()
    set(GRN_WITH_LZ4 TRUE)
    message(STATUS "LZ4: ${LIBLZ4_LIBS}")
  else()
    set(GRN_BUNDLED_LZ4_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lz4-${GRN_BUNDLED_LZ4_VERSION}")
    if(EXISTS ${GRN_BUNDLED_LZ4_DIR})
      set(LIBLZ4_INCLUDE_DIRS "${GRN_BUNDLED_LZ4_DIR}/lib")
      set(LIBLZ4_LIBS liblz4)
      set(GRN_WITH_BUNDLED_LZ4 TRUE)
      set(GRN_WITH_LZ4 TRUE)
      message(STATUS "LZ4: ${LIBLZ4_LIBS} (bundled)")
    else()
      if(${GRN_WITH_LZ4} STREQUAL "yes")
	message(FATAL_ERROR "No LZ4 found")
      endif()
      set(GRN_WITH_LZ4 FALSE)
      message(STATUS "No LZ4")
    endif()
  endif()
else()
  set(GRN_WITH_LZ4 FALSE)
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
include(GNUInstallDirs)

set(GRN_ZSTD_EP_VERSION "1.4.8")
set(GRN_ZSTD_TARGET)
set(GRN_WITH_ZSTD "auto"
  CACHE STRING "Support data compression by Zstandard.")
if(NOT ${GRN_WITH_ZSTD} STREQUAL "no")
  if(${GRN_WITH_ZSTD} STREQUAL "bundled")
    set(zstd_FOUND FALSE)
  else()
    find_package(zstd)
  endif()
  if(zstd_FOUND)
    set(GRN_WITH_ZSTD TRUE)
    get_target_property(ZSTD_LIBRARY zstd::libzstd_shared IMPORTED_LOCATION)
    set(GRN_ZSTD_TARGET zstd::libzstd_shared)
    message(STATUS "Zstandard: ${ZSTD_LIBRARY}")
  else()
    if(CMAKE_VERSION VERSION_GREATER 3.6)
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
      add_library(zstd::libzstd_static STATIC IMPORTED)
      set_target_properties(zstd::libzstd_static PROPERTIES
	IMPORTED_LOCATION ${ZSTD_STATIC_LIBRARY}
	INTERFACE_INCLUDE_DIRECTORIES "${ZSTD_PREFIX}/include")
      add_dependencies(zstd::libzstd_static zstd-ep)
      set(ZSTD_SOURCE_DIR
	"${CMAKE_CURRENT_BINARY_DIR}/zstd-ep-prefix/src/zstd-ep")
      install(FILES
	"${ZSTD_SOURCE_DIR}/CHANGELOG"
	"${ZSTD_SOURCE_DIR}/COPYING"
	"${ZSTD_SOURCE_DIR}/LICENSE"
	"${ZSTD_SOURCE_DIR}/README.md"
	DESTINATION "${GRN_DATA_DIR}/zstd")
      set(GRN_ZSTD_TARGET zstd::libzstd_static)
      set(GRN_WITH_ZSTD TRUE)
      message(STATUS "Zstandard: ${ZSTD_STATIC_LIBRARY} (bundled)")
    else()
      if(${GRN_WITH_ZSTD} STREQUAL "yes")
	message(FATAL_ERROR "No Zstandard found")
      endif()
      set(GRN_WITH_ZSTD FALSE)
      message(STATUS "No Zstandard")
    endif()
  endif()
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
if(NOT ${GRN_WITH_RAPIDJSON} STREQUAL "no")
  pkg_check_modules(RAPIDJSON rapidjson)
  if(RAPIDJSON_FOUND)
    add_library(rapidjson INTERFACE IMPORTED)
    set_property(TARGET rapidjson PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${RAPIDJSON_INCLUDE_DIRS})
  endif()
  if(NOT TARGET rapidjson)
    find_package(RapidJSON)
  endif()
  if(NOT TARGET rapidjson)
    if(RAPIDJSON_INCLUDE_DIRS)
      add_library(rapidjson INTERFACE IMPORTED)
      set_property(TARGET rapidjson PROPERTY
	INTERFACE_INCLUDE_DIRECTORIES ${RAPIDJSON_INCLUDE_DIRS})
    elseif(RapidJSON_INCLUDE_DIRS)
      add_library(rapidjson INTERFACE IMPORTED)
      set_property(TARGET rapidjson PROPERTY
	INTERFACE_INCLUDE_DIRECTORIES ${RAPIDJSON_INCLUDE_DIRS})
    else()
      unset(RapidJSON_FOUND)
      check_include_file_cxx(rapidjson/rapidjson.h RapidJSON_FOUND)
      if(RapidJSON_FOUND)
	add_library(rapidjson INTERFACE IMPORTED)
      endif()
    endif()
  endif()
  if(TARGET rapidjson)
    set(GRN_WITH_RAPIDJSON TRUE)
    get_target_property(GRN_RAPIDJSON_INCLUDE_DIRS
      rapidjson
      INTERFACE_INCLUDE_DIRECTORIES)
    if(GRN_RAPIDJSON_INCLUDE_DIRS)
      message(STATUS "RapidJSON: ${GRN_RAPIDJSON_INCLUDE_DIRS} (system)")
    else()
      message(STATUS "RapidJSON: (system)")
    endif()
  elseif(CMAKE_VERSION VERSION_GREATER 3.0) # This is rough check
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
    add_library(rapidjson INTERFACE IMPORTED)
    file(MAKE_DIRECTORY "${RAPIDJSON_PREFIX}/include")
    set_property(TARGET rapidjson PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES "${RAPIDJSON_PREFIX}/include")
    add_dependencies(rapidjson rapidjson-ep)
    set(RAPIDJSON_SOURCE_DIR
      "${CMAKE_CURRENT_BINARY_DIR}/rapidjson-ep-prefix/src/rapidjson-ep")
    install(FILES
      "${RAPIDJSON_SOURCE_DIR}/CHANGELOG.md"
      "${RAPIDJSON_SOURCE_DIR}/license.txt"
      "${RAPIDJSON_SOURCE_DIR}/readme.md"
      DESTINATION "${GRN_DATA_DIR}/rapidjson")
    set(GRN_WITH_RAPIDJSON TRUE)
    get_target_property(GRN_RAPIDJSON_INCLUDE_DIRS
      rapidjson
      INTERFACE_INCLUDE_DIRECTORIES)
    message(STATUS "RapidJSON: ${GRN_RAPIDJSON_INCLUDE_DIRS} (bundled)")
  else()
    set(GRN_WITH_RAPIDJSON FALSE)
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
if(NOT ${GRN_WITH_MECAB} STREQUAL "no")
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
    message(STATUS "MeCab: ${MECAB_LIBRARIES}")
  else()
    set(GRN_BUNDLED_MECAB_DIR
      "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mecab-${GRN_BUNDLED_MECAB_VERSION}")
    if(EXISTS ${GRN_BUNDLED_MECAB_DIR})
      set(MECAB_INCLUDE_DIRS "${GRN_BUNDLED_MECAB_DIR}/src")
      set(MECAB_LIBRARY_DIRS "${CMAKE_CURRENT_BUILD_DIR}/vendor/mecab")
      set(MECAB_LIBRARIES libmecab)
      set(GRN_WITH_MECAB TRUE)
      set(GRN_WITH_BUNDLED_MECAB TRUE)
      message(STATUS "MeCab: ${MECAB_LIBRARIES} (bundled)")
    else()
      if(${GRN_WITH_MECAB} STREQUAL "yes")
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
      set(MECAB_LIBRARIES "")
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
  if(NOT DEFINED KYTEA_FOUND)
    pkg_check_modules(KYTEA kytea)
  endif()
  if(KYTEA_FOUND)
    set(GRN_WITH_KYTEA TRUE)
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
  if(NOT ("${GRN_WITH_LIBSTEMMER}" STREQUAL "yes" OR
	"${GRN_WITH_LIBSTEMMER}" STREQUAL "auto"))
    if("${LIBSTEMMER_INCLUDE_DIRS}" STREQUAL "")
      set(LIBSTEMMER_INCLUDE_DIRS "${GRN_WITH_LIBSTEMMER}/include")
    endif()
    if("${LIBSTEMMER_LIBRARY_DIRS}" STREQUAL "")
      set(LIBSTEMMER_LIBRARY_DIRS "${GRN_WITH_LIBSTEMMER}/lib")
    endif()
  endif()
  set(CMAKE_REQUIRED_INCLUDES_SAVE ${CMAKE_REQUIRED_INCLUDES})
  set(CMAKE_REQUIRED_INCLUDES
    ${CMAKE_REQUIRED_INCLUDES}
    ${LIBSTEMMER_INCLUDE_DIRS})
  ac_check_headers(libstemmer.h)
  ac_check_lib(stemmer sb_stemmer_list "${LIBSTEMMER_LIBRARY_DIRS}")
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVE})
  if(HAVE_LIBSTEMMER_H AND HAVE_LIBSTEMMER)
    set(LIBSTEMMER_LIBRARIES "stemmer")
    set(GRN_WITH_LIBSTEMMER TRUE)
  else()
    if(${GRN_WITH_LIBSTEMMER} STREQUAL "yes")
      message(FATAL_ERROR "No libstemmer found")
    endif()
    set(GRN_WITH_LIBSTEMMER FALSE)
  endif()
else()
  set(GRN_WITH_LIBSTEMMER FALSE)
endif()

set(GRN_WITH_ZEROMQ "auto"
  CACHE STRING "use ZeroMQ for suggestion")
if(NOT ${GRN_WITH_ZEROMQ} STREQUAL "no")
  if(NOT DEFINED ZEROMQ_FOUND)
    pkg_check_modules(ZEROMQ libzmq)
  endif()
  if(ZEROMQ_FOUND)
    set(GRN_WITH_ZEROMQ TRUE)
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
  if("${GRN_WITH_LIBEVENT}" STREQUAL "yes" OR
      "${GRN_WITH_LIBEVENT}" STREQUAL "auto")
    set(LIBEVENT_INCLUDE_DIRS "")
    set(LIBEVENT_LIBRARY_DIRS "")
  else()
    set(LIBEVENT_INCLUDE_DIRS "${GRN_WITH_LIBEVENT}/include")
    set(LIBEVENT_LIBRARY_DIRS "${GRN_WITH_LIBEVENT}/lib")
  endif()
  set(CMAKE_REQUIRED_INCLUDES_SAVE ${CMAKE_REQUIRED_INCLUDES})
  ac_check_lib(event event_init "${LIBEVENT_LIBRARY_DIRS}")
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVE})
  if(HAVE_LIBEVENT)
    set(LIBEVENT_LIBRARIES "event")
    set(GRN_WITH_LIBEVENT TRUE)
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
  if(NOT DEFINED MESSAGE_PACK_FOUND)
    pkg_check_modules(MESSAGE_PACK msgpack-c)
    if(NOT MESSAGE_PACK_FOUND)
      pkg_check_modules(MESSAGE_PACK msgpack)
    endif()
    unset(MESSAGE_PACK_LIBS)
    unset(MESSAGE_PACK_LIBS CACHE)
  endif()
  if(MESSAGE_PACK_FOUND)
    if(${MESSAGE_PACK_LIBRARY_DIRS})
      find_library(MESSAGE_PACK_LIBS
	NAMES ${MESSAGE_PACK_LIBRARIES}
	PATHS ${MESSAGE_PACK_LIBRARY_DIRS}
	NO_DEFAULT_PATH)
    else()
      find_library(MESSAGE_PACK_LIBS
	NAMES ${MESSAGE_PACK_LIBRARIES})
    endif()
    set(GRN_WITH_MESSAGE_PACK TRUE)
    message(STATUS "MessagePack: ${MESSAGE_PACK_LIBS}")
  else()
    if("${GRN_WITH_MESSAGE_PACK}" STREQUAL "yes" OR
	"${GRN_WITH_MESSAGE_PACK}" STREQUAL "auto")
      set(MESSAGE_PACK_INCLUDE_DIRS "")
      set(MESSAGE_PACK_LIBRARY_DIRS "")
      find_library(MESSAGE_PACK_LIBS NAMES msgpackc)
    else()
      set(MESSAGE_PACK_INCLUDE_DIRS "${GRN_WITH_MESSAGE_PACK}/include")
      set(MESSAGE_PACK_LIBRARY_DIRS "${GRN_WITH_MESSAGE_PACK}/lib")
      find_library(MESSAGE_PACK_LIBS
	NAMES msgpackc
	PATHS ${MESSAGE_PACK_LIBRARY_DIRS}
	NO_DEFAULT_PATH)
    endif()
    if(MESSAGE_PACK_LIBS)
      set(GRN_WITH_MESSAGE_PACK TRUE)
      message(STATUS "MessagePack: ${MESSAGE_PACK_LIBS}")
    else()
      set(GRN_BUNDLED_MESSAGE_PACK_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/vendor/msgpack-${GRN_BUNDLED_MESSAGE_PACK_VERSION}")
      if(EXISTS "${GRN_BUNDLED_MESSAGE_PACK_DIR}")
	set(MESSAGE_PACK_INCLUDE_DIRS "${GRN_BUNDLED_MESSAGE_PACK_DIR}/include")
	set(MESSAGE_PACK_LIBS msgpackc)
	set(GRN_WITH_BUNDLED_MESSAGE_PACK TRUE)
	set(GRN_WITH_MESSAGE_PACK TRUE)
	message(STATUS "MessagePack: ${MESSAGE_PACK_LIBS} (bundled)")
      else()
	if(${GRN_WITH_MESSAGE_PACK} STREQUAL "yes")
	  message(FATAL_ERROR "No MessagePack found")
	endif()
	set(MESSAGE_PACK_LIBS "")
	set(GRN_WITH_MESSAGE_PACK FALSE)
	message(STATUS "No MessagePack")
      endif()
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
if(NOT ${GRN_WITH_XXHASH} STREQUAL "no")
  set(LIBXXHASH_DEFINITIONS)
  if(${GRN_WITH_XXHASH} STREQUAL "bundled")
    set(xxHash_FOUND FALSE)
  else()
    find_package(xxHash)
  endif()
  if(xxHash_FOUND)
    get_target_property(LIBXXHASH_INCLUDE_DIRS xxHash::xxhash
      INTERFACE_INCLUDE_DIRECTORIES)
    get_target_property(LIBXXHASH_LIBS xxHash::xxhash IMPORTED_LOCATION_RELEASE)
    if(NOT LIBXXHASH_LIBS)
      get_target_property(LIBXXHASH_LIBS xxHash::xxhash IMPORTED_LOCATION)
    endif()
    message(STATUS "xxHash: ${LIBXXHASH_LIBS}")
  else()
    if(${GRN_WITH_XXHASH} STREQUAL "bundled")
      set(LIBXXHASH_FOUND FALSE)
    else()
      if(NOT DEFINED LIBXXHASH_FOUND)
	pkg_check_modules(LIBXXHASH libxxhash)
	unset(LIBXXHASH_LIBS)
	unset(LIBXXHASH_LIBS CACHE)
      endif()
    endif()
    if(LIBXXHASH_FOUND)
      if(${LIBXXHASH_LIBRARY_DIRS})
	find_library(LIBXXHASH_LIBS
          NAMES ${LIBXXHASH_LIBRARIES}
          PATHS ${LIBXXHASH_LIBRARY_DIRS}
          NO_DEFAULT_PATH)
      else()
	find_library(LIBXXHASH_LIBS
          NAMES ${LIBXXHASH_LIBRARIES})
      endif()
      if(LIBXXHASH_VERSION VERSION_LESS "0.8.0")
	set(LIBXXHASH_DEFINITIONS -DXXH_INLINE_ALL)
      endif()
      set(GRN_WITH_XXHASH TRUE)
      add_library(xxHash::xxhash INTERFACE IMPORTED)
      set_property(TARGET xxHash::xxhash PROPERTY
	INTERFACE_INCLUDE_DIRECTORIES "${RAPIDJSON_PREFIX}/include")
      message(STATUS "xxHash: ${LIBXXHASH_LIBS}")
    else()
      set(GRN_BUNDLED_XXHASH_DIR
	"${CMAKE_CURRENT_SOURCE_DIR}/vendor/xxHash-${GRN_BUNDLED_XXHASH_VERSION}")
      if(EXISTS ${GRN_BUNDLED_XXHASH_DIR})
	set(LIBXXHASH_INCLUDE_DIRS ${GRN_BUNDLED_XXHASH_DIR})
	set(LIBXXHASH_LIBS)
	set(LIBXXHASH_DEFINITIONS -DXXH_INLINE_ALL)
	set(GRN_WITH_BUNDLED_XXHASH TRUE)
	set(GRN_WITH_XXHASH TRUE)
	message(STATUS "xxHash: ${LIBXXHASH_INCLUDE_DIRS} (bundled)")
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
  endif()
else()
  set(GRN_WITH_XXHASH FALSE)
endif()


find_program(RUBY NAMES
  "ruby3.0" "ruby30"
  "ruby2.7" "ruby27"
  "ruby2.6" "ruby26"
  "ruby2.5" "ruby25"
  "ruby2.4" "ruby24"
  "ruby2.3" "ruby23"
  "ruby2.2" "ruby22"
  "ruby2.1" "ruby21"
  "ruby")

option(GRN_WITH_MRUBY "use mruby" OFF)
if(GRN_WITH_MRUBY)
  set(MRUBY_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/vendor/mruby-source/include")
  set(MRUBY_LIBS mruby)
else()
  set(MRUBY_INCLUDE_DIRS "")
  set(MRUBY_LIBS "")
endif()
set(MRUBY_DEFINITIONS "MRB_INT64" "HAVE_ONIGMO_H")

# For mruby-file-stat
if(NOT WIN32)
  ac_check_headers(sys/sysmacros.h)
endif()

set(GRN_WITH_ONIGMO ON)
option(GRN_WITH_BUNDLED_ONIGMO "use bundled Onigmo" ON)
if(GRN_WITH_BUNDLED_ONIGMO)
  set(ONIGMO_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/onigmo-source")
  set(ONIGMO_LIBS onigmo)

  # TODO: It's for Onigmo static link case. If we support system Onigmo,
  # we need to remove it and add -DEXPORT to Onigmo build.
  add_definitions(-DONIG_EXTERN=extern)
else()
  pkg_check_modules(ONIGMO REQUIRED onigmo)
endif()

add_subdirectory(vendor)
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
set(exec_prefix "${prefix}")
set(bindir "${CMAKE_INSTALL_PREFIX}/${BIN_DIR}")
set(sbindir "${CMAKE_INSTALL_PREFIX}/${SBIN_DIR}")
set(libdir "${CMAKE_INSTALL_PREFIX}/${LIB_DIR}")
set(includedir "${CMAKE_INSTALL_PREFIX}/${INCLUDE_DIR}")
set(datarootdir "${CMAKE_INSTALL_PREFIX}/${DATA_DIR}")
set(datadir "${datarootdir}")
set(expanded_pluginsdir "${GRN_PLUGINS_DIR}")
set(GRN_EXPANDED_DEFAULT_DOCUMENT_ROOT "${GRN_DEFAULT_DOCUMENT_ROOT}")
set(EXEEXT "${CMAKE_EXECUTABLE_SUFFIX}")
configure_file(groonga.pc.in "${CMAKE_CURRENT_BINARY_DIR}/groonga.pc" @ONLY)

if(NOT GRN_EMBED)
  install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/groonga.pc"
    DESTINATION "${LIB_DIR}/pkgconfig/")
endif()

install(FILES
  "COPYING"
  "README.md"
  DESTINATION "${GRN_DATA_DIR}")

add_subdirectory(vendor/plugins)
