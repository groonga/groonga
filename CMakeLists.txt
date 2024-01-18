# Copyright (C) 2012-2018  Brazil
# Copyright (C) 2018-2024  Sutou Kouhei <kou@clear-code.com>
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

# * Ubuntu 20.04: 3.16
# * CentOS 7's EPEL: 3.17
cmake_minimum_required(VERSION 3.16)

message(STATUS "CMake: ${CMAKE_VERSION}")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/base_version" GRN_VERSION)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/version.sh")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.sh" GRN_VERSION_FULL)
else()
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/version.sh")
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git" AND EXISTS "/bin/sh")
      execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/version-gen.sh"
                      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
  endif()
  if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/version.sh")
    file(READ "${CMAKE_CURRENT_BINARY_DIR}/version.sh" GRN_VERSION_FULL)
  else()
    set(GRN_VERSION_FULL "${GRN_VERSION}")
  endif()
endif()
string(REGEX REPLACE "(^.*=|\n)" "" GRN_VERSION_FULL "${GRN_VERSION_FULL}")
string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.([^.-]+)$" "\\1" GRN_VERSION_MAJOR
                     "${GRN_VERSION}")
string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.([^.-]+)$" "\\2" GRN_VERSION_MINOR
                     "${GRN_VERSION}")
string(REGEX REPLACE "^([^.]+)\\.([^.]+)\\.([^.-]+)$" "\\3" GRN_VERSION_MICRO
                     "${GRN_VERSION}")
string(REGEX REPLACE "\\." "," GRN_VERSION_RC "${GRN_VERSION}")
string(REGEX REPLACE "-.*$" "" GRN_VERSION_RC "${GRN_VERSION_RC}")

# https://cmake.org/cmake/help/latest/policy/CMP0054.html
#
# Only interpret if() arguments as variables or keywords when unquoted.
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0135.html
#
# When using the URL download method with the ExternalProject_Add()
# command, CMake 3.23 and below sets the timestamps of the extracted
# contents to the same as the timestamps in the archive. When the URL
# changes, the new archive is downloaded and extracted, but the
# timestamps of the extracted contents might not be newer than the
# previous contents. Anything that depends on the extracted contents
# might not be rebuilt, even though the contents may change.
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

# https://cmake.org/cmake/help/latest/policy/CMP0141.html
#
# MSVC debug information format flags are selected by an abstraction.
#
# This must be done BEFORE project().
if(POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  # For ccache
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()

project(
  groonga
  VERSION "${GRN_VERSION}"
  DESCRIPTION "An Embeddable Fulltext Search Engine"
  LANGUAGES C CXX)
set(GRN_PROJECT_NAME "${PROJECT_NAME}")
set(GRN_PROJECT_LABEL "Groonga")

if(NOT DEFINED CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()
set(CMAKE_C_STANDARD_REQUIRED TRUE)
if(NOT DEFINED CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  set(GRN_BUNDLED FALSE)
else()
  set(GRN_BUNDLED TRUE)
endif()

if(NOT GRN_BUNDLED)
  find_program(CCACHE ccache)
  if(CCACHE)
    if(NOT CMAKE_C_COMPILER_LAUNCHER)
      message(STATUS "Use ccache for compiling C: ${CCACHE}")
      set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    endif()
    if(NOT CMAKE_CXX_COMPILER_LAUNCHER)
      message(STATUS "Use ccache for compiling C++: ${CCACHE}")
      set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    endif()
  endif()
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

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
  set(GRN_C_COMPILER_GNU_LIKE TRUE)
else()
  set(GRN_C_COMPILER_GNU_LIKE FALSE)
endif()

configure_file(include/groonga/version.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/groonga/version.h)
configure_file(include/groonga/build_option_cmake.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/include/groonga/build_option.h)

include(CheckIncludeFile)
include(CheckIncludeFileCXX)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCSourceCompiles)
include(${CMAKE_CURRENT_SOURCE_DIR}/build/cmake_modules/ReadFileList.cmake)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

find_package(PkgConfig)

if(DEFINED GRN_EMBED)
  set(GRN_EMBED_DEFAULT ${GRN_EMBED})
else()
  set(GRN_EMBED_DEFAULT OFF)
endif()
set(GRN_EMBED
    ${GRN_EMBED_DEFAULT}
    CACHE BOOL "Build as a static library to embed into an application")

set(GRN_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/groonga")
set(GRN_INSTALL_CMAKE_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/Groonga")
if(NOT DEFINED GRN_DATA_DIR)
  set(GRN_DATA_DIR "${CMAKE_INSTALL_DATADIR}/${GRN_PROJECT_NAME}")
endif()
set(GRN_CONFIG_DIR "${CMAKE_INSTALL_SYSCONFDIR}/${GRN_PROJECT_NAME}")
set(GRN_FULL_CONFIG_DIR "${CMAKE_INSTALL_FULL_SYSCONFDIR}/${GRN_PROJECT_NAME}")
set(GRN_CONFIG_PATH "${GRN_FULL_CONFIG_DIR}/${GRN_PROJECT_NAME}.conf")

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
    CACHE
      STRING
      "DANGER!!! Groonga's stack size. Normarlly, you should not change this variable."
)
set(GRN_LOCK_TIMEOUT
    900000
    CACHE STRING "timeout to acquire a lock.")
set(GRN_LOCK_WAIT_TIME_NANOSECOND
    1000000
    CACHE STRING "wait time in nanosecond to acquire a lock.")
set(GRN_RELATIVE_PLUGINS_DIR
    "${CMAKE_INSTALL_LIBDIR}/${GRN_PROJECT_NAME}/plugins")
set(GRN_PLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/${GRN_RELATIVE_PLUGINS_DIR}")
set(GRN_PLUGIN_SUFFIX "${CMAKE_SHARED_MODULE_SUFFIX}")
set(GRN_DLL_FILENAME "libgroonga${CMAKE_SHARED_LIBRARY_SUFFIX}")
set(GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE
    "${GRN_CONFIG_DIR}/synonyms.tsv")
set(GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE "${GRN_FULL_CONFIG_DIR}/synonyms.tsv")
set(GRN_RELATIVE_RUBY_SCRIPTS_DIR
    "${CMAKE_INSTALL_LIBDIR}/${GRN_PROJECT_NAME}/scripts/ruby")
set(GRN_RUBY_SCRIPTS_DIR
    "${CMAKE_INSTALL_PREFIX}/${GRN_RELATIVE_RUBY_SCRIPTS_DIR}")

if(GRN_C_COMPILER_GNU_LIKE)
  set(GRN_C_COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS} $<$<CONFIG:Debug>:-g3 -O0>")
  set(GRN_CXX_COMPILE_FLAGS
      "${GRN_CXX_COMPILE_FLAGS} $<$<CONFIG:Debug>:-g3 -O0>")
  set(GRN_C_COMPILE_FLAGS
      "${GRN_C_COMPILE_FLAGS} $<$<CONFIG:RelWithDebInfo>:-g3>")
  set(GRN_CXX_COMPILE_FLAGS
      "${GRN_CXX_COMPILE_FLAGS} $<$<CONFIG:RelWithDebInfo>:-g3>")
endif()

# /Ob1 is used with RelWithDebInfo by default but it's slower than /Ob2 which is
# the default with Release. We need performance with debug information.
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

set(GRN_C_FLAGS_AVX "")
set(GRN_C_FLAGS_AVX2 "")
set(GRN_C_FLAGS_AVX512 "")
if(MSVC)
  if(CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
    set(GRN_C_FLAGS_AVX "/arch:AVX")
    set(GRN_C_FLAGS_AVX2 "/arch:AVX2")
    set(GRN_C_FLAGS_AVX512 "/arch:AVX512")
  endif()
else()
  check_c_compiler_flag("-mavx" GRN_HAVE_AVX_FLAG)
  if(GRN_HAVE_AVX_FLAG)
    set(GRN_C_FLAGS_AVX "-mavx")
  endif()
  check_c_compiler_flag("-mavx2" GRN_HAVE_AVX2_FLAG)
  if(GRN_HAVE_AVX2_FLAG)
    set(GRN_C_FLAGS_AVX2 "-mavx2")
  endif()
  check_c_compiler_flag("-mavx512f" GRN_HAVE_AVX512F_FLAG)
  if(GRN_HAVE_AVX512F_FLAG)
    set(GRN_C_FLAGS_AVX512 "-mavx512f")
  endif()
endif()

set(GRN_WITH_SIMD OFF)
if(GRN_C_FLAGS_AVX)
  set(GRN_WITH_SIMD_AVX ON)
  set(GRN_WITH_SIMD ON)
else()
  set(GRN_WITH_SIMD_AVX OFF)
endif()
if(GRN_C_FLAGS_AVX2)
  set(GRN_WITH_SIMD_AVX2 ON)
  set(GRN_WITH_SIMD ON)
else()
  set(GRN_WITH_SIMD_AVX2 OFF)
endif()
if(GRN_C_FLAGS_AVX512)
  set(GRN_WITH_SIMD_AVX512 ON)
  set(GRN_WITH_SIMD ON)
else()
  set(GRN_WITH_SIMD_AVX512 OFF)
endif()

if(GRN_C_COMPILER_GNU_LIKE)
  check_build_flag("-Wall")
  check_build_flag("-Wno-unused-but-set-variable")
  check_build_flag("-Wno-unused-parameter")
  check_cflag("-Wno-pointer-sign")
  check_build_flag("-Wfloat-equal")
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

option(GRN_WARN_CONVERSION "Enable -Wconversion" OFF)
if(GRN_WARN_CONVERSION)
  check_build_flag("-Wconversion")
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

set(GRN_WITH_BFLOAT16
    "auto"
    CACHE STRING "Support bfloat16.")
check_c_source_compiles(
  [=[
int
main(int argc, char *argv)
{
  __bf16 value = 1.0;
  return 0;
}
]=]
  GRN_HAVE_BFLOAT16)
if(GRN_WITH_BFLOAT16 STREQUAL "yes")
  if(!GRN_HAVE_BFLOAT16)
    message(FATAL_ERROR "Compiler doesn't support bfloat16")
  endif()
elseif(GRN_WITH_BFLOAT16 STREQUAL "no")
  set(GRN_HAVE_BFLOAT16 NO)
endif()

option(GRN_WITH_WINDOWS_BACK_TRACE "enable back trace on Windows." ON)

add_library(grn_dependencies INTERFACE)

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
if(NOT (CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64"
        OR CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^i[0-9]86$"))
  ac_check_lib(atomic __atomic_store_8)
endif()

check_struct_has_member("struct tm" "tm_gmtoff" "time.h"
                        HAVE_STRUCT_TM_TM_GMTOFF)

find_package(Threads REQUIRED)
if(WIN32)
  set(HAVE_PTHREAD_H FALSE)
else()
  set(HAVE_PTHREAD_H ${CMAKE_USE_PTHREADS_INIT})
endif()

option(GRN_WITH_NFKC "use NFKC based UTF8 normalization." ON)

option(GRN_WITH_MEMORY_DEBUG "use debug memory management" OFF)

if(WIN32)
  target_link_libraries(grn_dependencies INTERFACE dbghelp.lib)
  target_link_libraries(grn_dependencies INTERFACE psapi.lib)

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

set(GRN_WITH_ZLIB
    "auto"
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

include(FetchContent)
set(GRN_FETCH_CONTENT_COMMON_OPTIONS)
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
  list(APPEND GRN_FETCH_CONTENT_COMMON_OPTIONS EXCLUDE_FROM_ALL TRUE)
endif()

include(ExternalProject)
set(GRN_EP_COMMON_OPTIONS
    LOG_DOWNLOAD
    TRUE
    LOG_UPDATE
    TRUE
    LOG_PATCH
    TRUE
    LOG_CONFIGURE
    TRUE
    LOG_BUILD
    TRUE
    LOG_INSTALL
    TRUE
    LOG_TEST
    TRUE
    LOG_MERGED_STDOUTERR
    TRUE
    LOG_OUTPUT_ON_FAILURE
    TRUE)

set(GRN_ZSTD_BUNDLED_VERSION "1.5.5")
set(GRN_ZSTD_BUNDLED_SHA256
    "9c4396cc829cfae319a6e2615202e82aad41372073482fce286fac78646d3ee4")
set(GRN_WITH_ZSTD
    "auto"
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
    function(grn_build_zstd)
      set(ZSTD_SOURCE_BASE_NAME "zstd-${GRN_ZSTD_BUNDLED_VERSION}.tar.gz")
      set(ZSTD_SOURCE_LOCAL_PATH
          "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${ZSTD_SOURCE_BASE_NAME}")
      if(EXISTS ${ZSTD_SOURCE_LOCAL_PATH})
        set(ZSTD_SOURCE_URL ${ZSTD_SOURCE_LOCAL_PATH})
      else()
        set(ZSTD_SOURCE_URL
            "https://github.com/facebook/zstd/releases/download")
        string(APPEND ZSTD_SOURCE_URL "/v${GRN_ZSTD_BUNDLED_VERSION}")
        string(APPEND ZSTD_SOURCE_URL "/${ZSTD_SOURCE_BASE_NAME}")
      endif()
      fetchcontent_declare(
        zstd
        ${GRN_FETCH_CONTENT_COMMON_OPTIONS} SOURCE_SUBDIR "build/cmake"
        URL ${ZSTD_SOURCE_URL}
        URL_HASH "SHA256=${GRN_ZSTD_BUNDLED_SHA256}")
      set(BUILD_TESTING OFF)
      set(ZSTD_BUILD_PROGRAMS OFF)
      set(ZSTD_BUILD_SHARED OFF)
      set(ZSTD_BUILD_STATIC ON)
      set(ZSTD_USE_STATIC_RUNTIME ON)
      fetchcontent_makeavailable(zstd)
      if(CMAKE_VERSION VERSION_LESS 3.28)
        set_property(DIRECTORY "${zstd_SOURCE_DIR}/build/cmake"
                     PROPERTY EXCLUDE_FROM_ALL TRUE)
      endif()
      set(ZSTD_INCLUDE_DIR
          "${zstd_SOURCE_DIR}/lib"
          PARENT_SCOPE)
      add_library(libzstd_headers INTERFACE)
      target_include_directories(
        libzstd_headers INTERFACE "$<BUILD_INTERFACE:${zstd_SOURCE_DIR}/lib>")
      target_link_libraries(grn_dependencies INTERFACE libzstd_headers)
      install(FILES "${zstd_SOURCE_DIR}/CHANGELOG" "${zstd_SOURCE_DIR}/COPYING"
                    "${zstd_SOURCE_DIR}/LICENSE" "${zstd_SOURCE_DIR}/README.md"
              DESTINATION "${GRN_DATA_DIR}/zstd")
    endfunction()
    grn_build_zstd()
    set(GRN_ZSTD_TARGET libzstd_static)
    set(GRN_WITH_BUNDLED_ZSTD TRUE)
    set(GRN_WITH_ZSTD TRUE)
    message(STATUS "Zstandard: (bundled)")
  endif()
  target_link_libraries(grn_dependencies INTERFACE ${GRN_ZSTD_TARGET})
else()
  set(GRN_WITH_ZSTD FALSE)
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_rapidjson_version"
     GRN_BUNDLED_RAPIDJSON_VERSION)
string(STRIP "${GRN_BUNDLED_RAPIDJSON_VERSION}" GRN_BUNDLED_RAPIDJSON_VERSION)
set(GRN_RAPIDJSON_EP_VERSION ${GRN_BUNDLED_RAPIDJSON_VERSION})

set(GRN_WITH_RAPIDJSON
    "auto"
    CACHE STRING "Support JSON processing by RapidJSON.")
if(NOT "${GRN_WITH_RAPIDJSON}" STREQUAL "no")
  if("${GRN_WITH_RAPIDJSON}" STREQUAL "bundled")
    set(GroongaRapidJSON_FOUND FALSE)
  else()
    if("${GRN_WITH_RAPIDJSON}" STREQUAL "system")
      find_package(GroongaRapidJSON REQUIRED)
    else()
      find_package(GroongaRapidJSON)
    endif()
  endif()
  if(GroongaRapidJSON_FOUND)
    set(GRN_WITH_RAPIDJSON TRUE)
    message(STATUS "RapidJSON: system")
    target_link_libraries(grn_dependencies INTERFACE Groonga::RapidJSON)
  else()
    set(GRN_BUNDLED_RAPIDJSON_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/rapidjson-${GRN_BUNDLED_RAPIDJSON_VERSION}"
    )
    if(EXISTS ${GRN_BUNDLED_RAPIDJSON_DIR})
      set(RAPIDJSON_SOURCE_URL ${GRN_BUNDLED_RAPIDJSON_DIR})
    else()
      set(RAPIDJSON_SOURCE_URL
          "https://github.com/Tencent/rapidjson/archive/v${GRN_RAPIDJSON_EP_VERSION}.tar.gz"
      )
    endif()
    set(RAPIDJSON_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/rapidjson-ep-install")
    set(RAPIDJSON_CMAKE_ARGS
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_INSTALL_PREFIX=${RAPIDJSON_PREFIX}
        -DRAPIDJSON_BUILD_DOC=OFF
        -DRAPIDJSON_BUILD_EXAMPLES=OFF
        -DRAPIDJSON_BUILD_TESTS=OFF)
    externalproject_add(
      rapidjson-ep
      ${GRN_EP_COMMON_OPTIONS}
      CMAKE_ARGS ${RAPIDJSON_CMAKE_ARGS}
      URL ${RAPIDJSON_SOURCE_URL})
    add_library(Groonga::RapidJSON INTERFACE IMPORTED)
    file(MAKE_DIRECTORY "${RAPIDJSON_PREFIX}/include")
    target_include_directories(
      Groonga::RapidJSON
      INTERFACE "$<BUILD_INTERFACE:${RAPIDJSON_PREFIX}/include>")
    add_dependencies(Groonga::RapidJSON rapidjson-ep)
    set(RAPIDJSON_SOURCE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/rapidjson-ep-prefix/src/rapidjson-ep")
    install(
      FILES "${RAPIDJSON_SOURCE_DIR}/CHANGELOG.md"
            "${RAPIDJSON_SOURCE_DIR}/license.txt"
            "${RAPIDJSON_SOURCE_DIR}/readme.md"
      DESTINATION "${GRN_DATA_DIR}/rapidjson")
    set(GRN_WITH_RAPIDJSON TRUE)
    message(STATUS "RapidJSON: bundled")
    target_link_libraries(grn_dependencies INTERFACE Groonga::RapidJSON)
  endif()
else()
  set(GRN_WITH_RAPIDJSON FALSE)
endif()

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_mecab_version"
     GRN_BUNDLED_MECAB_VERSION)
string(STRIP "${GRN_BUNDLED_MECAB_VERSION}" GRN_BUNDLED_MECAB_VERSION)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_mecab_naist_jdic_version"
     GRN_BUNDLED_MECAB_NAIST_JDIC_VERSION)
string(STRIP "${GRN_BUNDLED_MECAB_NAIST_JDIC_VERSION}"
             GRN_BUNDLED_MECAB_NAIST_JDIC_VERSION)

set(GRN_WITH_BUNDLED_MECAB FALSE)
set(GRN_WITH_MECAB
    "auto"
    CACHE STRING "use MeCab for morphological analysis")
if(NOT "${GRN_WITH_MECAB}" STREQUAL "no")
  if(WIN32)
    find_path(MECAB_INCLUDE_DIR NAMES mecab.h)
    find_library(MECAB_LIBRARIES NAMES mecab)
  else()
    set(GRN_MECAB_CONFIG
        "mecab-config"
        CACHE FILEPATH "mecab-config path")
    find_program(GRN_MECAB_CONFIG_ABSOLUTE_PATH "${GRN_MECAB_CONFIG}")
    if(EXISTS "${GRN_MECAB_CONFIG_ABSOLUTE_PATH}")
      execute_process(
        COMMAND "${GRN_MECAB_CONFIG_ABSOLUTE_PATH}" --inc-dir
        OUTPUT_VARIABLE MECAB_INCLUDE_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      execute_process(
        COMMAND "${GRN_MECAB_CONFIG_ABSOLUTE_PATH}" --libs-only-L
        OUTPUT_VARIABLE MECAB_LIBRARY_DIRS
        OUTPUT_STRIP_TRAILING_WHITESPACE)
      find_library(
        MECAB_LIBRARIES
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
          message(
            FATAL_ERROR
              "No MeCab found: "
              "include directories: <${MECAB_INCLUDE_DIRS}>, "
              "library directories: <${MECAB_LIBRARY_DIRS}>")
        else()
          message(
            FATAL_ERROR
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

set(GRN_WITH_KYTEA
    "auto"
    CACHE STRING "use KyTea for morphological analysis")
if(NOT ${GRN_WITH_KYTEA} STREQUAL "no")
  if(PkgConfig_FOUND)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      pkg_check_modules(pkg_kytea kytea IMPORTED_TARGET)
    else()
      pkg_check_modules(pkg_kytea kytea IMPORTED_TARGET GLOBAL)
    endif()
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

set(GRN_WITH_LIBSTEMMER
    "auto"
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

set(GRN_WITH_ZEROMQ
    "auto"
    CACHE STRING "use ZeroMQ for suggestion")
if(NOT ${GRN_WITH_ZEROMQ} STREQUAL "no")
  if(PkgConfig_FOUND)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      pkg_check_modules(pkg_libzmq libzmq IMPORTED_TARGET)
    else()
      pkg_check_modules(pkg_libzmq libzmq IMPORTED_TARGET GLOBAL)
    endif()
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

set(GRN_WITH_LIBEVENT
    "auto"
    CACHE STRING "use libevent for suggestion")
if(NOT ${GRN_WITH_LIBEVENT} STREQUAL "no")
  if(PkgConfig_FOUND)
    if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
      pkg_check_modules(pkg_libevent libevent IMPORTED_TARGET)
    else()
      pkg_check_modules(pkg_libevent libevent IMPORTED_TARGET GLOBAL)
    endif()
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
string(STRIP "${GRN_BUNDLED_MESSAGE_PACK_VERSION}"
             GRN_BUNDLED_MESSAGE_PACK_VERSION)

set(GRN_WITH_BUNDLED_MESSAGE_PACK FALSE)
set(GRN_WITH_MESSAGE_PACK
    "auto"
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
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/msgpack-${GRN_BUNDLED_MESSAGE_PACK_VERSION}"
    )
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
string(STRIP "${GRN_BUNDLED_XXHASH_VERSION}" GRN_BUNDLED_XXHASH_VERSION)
set(GRN_XXHASH_REQUIRED_VERSION "0.8.0")

set(GRN_WITH_BUNDLED_XXHASH FALSE)
set(GRN_WITH_XXHASH
    "auto"
    CACHE STRING "Support drilldown by hashed key with xxHash.")
if(NOT "${GRN_WITH_XXHASH}" STREQUAL "no")
  if("${GRN_WITH_XXHASH}" STREQUAL "bundled")
    set(GroongaxxHash_FOUND FALSE)
  else()
    if("${GRN_WITH_XXHASH}" STREQUAL "system")
      find_package(GroongaxxHash ${GRN_XXHASH_REQUIRED_VERSION} REQUIRED)
    else()
      find_package(GroongaxxHash ${GRN_XXHASH_REQUIRED_VERSION})
    endif()
  endif()
  if(GroongaxxHash_FOUND)
    set(GRN_WITH_XXHASH TRUE)
    target_link_libraries(grn_dependencies INTERFACE Groonga::xxhash)
  else()
    set(GRN_BUNDLED_XXHASH_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/xxHash-${GRN_BUNDLED_XXHASH_VERSION}"
    )
    if(EXISTS ${GRN_BUNDLED_XXHASH_DIR})
      set(GRN_WITH_BUNDLED_XXHASH TRUE)
      set(GRN_WITH_XXHASH TRUE)
      add_library(xxhash INTERFACE)
      target_include_directories(
        xxhash BEFORE INTERFACE "$<BUILD_INTERFACE:${GRN_BUNDLED_XXHASH_DIR}>")
      target_compile_definitions(xxhash INTERFACE XXH_INLINE_ALL)
      target_link_libraries(grn_dependencies INTERFACE xxhash)
      message(STATUS "xxHash: (bundled)")
      install(FILES "${GRN_BUNDLED_XXHASH_DIR}/LICENSE"
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

# We need to find LZ4 after xxHash because bundled LZ4 includes its xxhash.h.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/bundled_lz4_version"
     GRN_BUNDLED_LZ4_VERSION)
string(STRIP "${GRN_BUNDLED_LZ4_VERSION}" GRN_BUNDLED_LZ4_VERSION)

set(GRN_WITH_BUNDLED_LZ4 FALSE)
set(GRN_WITH_LZ4
    "auto"
    CACHE STRING "Support data compression by LZ4.")
if(NOT "${GRN_WITH_LZ4}" STREQUAL "no")
  if("${GRN_WITH_LZ4}" STREQUAL "bundled")
    set(Groongalz4_FOUND FALSE)
  else()
    if("${GRN_WITH_LZ4}" STREQUAL "system")
      find_package(Groongalz4 REQUIRED)
    else()
      find_package(Groongalz4)
    endif()
  endif()
  if(Groongalz4_FOUND)
    set(GRN_WITH_LZ4 TRUE)
    target_link_libraries(grn_dependencies INTERFACE ${GRN_LZ4_TARGET})
  else()
    set(GRN_BUNDLED_LZ4_DIR
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lz4-${GRN_BUNDLED_LZ4_VERSION}")
    if(EXISTS ${GRN_BUNDLED_LZ4_DIR})
      set(GRN_WITH_BUNDLED_LZ4 TRUE)
      set(GRN_WITH_LZ4 TRUE)
      add_subdirectory(vendor/lz4)
      set(GRN_LZ4_TARGET liblz4)
      target_link_libraries(grn_dependencies INTERFACE ${GRN_LZ4_TARGET})
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

set(GRN_CROARING_EP_VERSION "1.1.2")
set(GRN_WITH_ROARING_BITMAPS
    "no"
    CACHE STRING "Support posting compression by roaring bitmaps.")
if(NOT "${GRN_WITH_ROARING_BITMAPS}" STREQUAL "no")
  if("${GRN_WITH_ROARING_BITMAPS}" STREQUAL "bundled")
    set(roaring_FOUND FALSE)
  else()
    if("${GRN_WITH_ROARING_BITMAPS}" STREQUAL "system")
      find_package(roaring REQUIRED)
    else()
      find_package(roaring)
    endif()
  endif()
  if(roaring_FOUND)
    set(GRN_WITH_ROARING_BITMAPS TRUE)
    target_link_libraries(grn_dependencies INTERFACE roaring::roaring)
    message(STATUS "Roaring bitmaps: system")
  else()
    set(CROARING_PREFIX "${CMAKE_CURRENT_BINARY_DIR}/croaring-ep-install")
    set(CROARING_STATIC_LIBRARY "${CROARING_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
    string(APPEND CROARING_STATIC_LIBRARY "/${CMAKE_STATIC_LIBRARY_PREFIX}")
    string(APPEND CROARING_STATIC_LIBRARY "roaring")
    string(APPEND CROARING_STATIC_LIBRARY "${CMAKE_STATIC_LIBRARY_SUFFIX}")
    set(CROARING_CMAKE_ARGS
        -DBUILD_SHARED_LIBS=OFF
        -DBUILD_TESTING=OFF
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_INSTALL_LIBDIR=${CMAKE_INSTALL_LIBDIR}
        -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
        -DENABLE_ROARING_TESTS=OFF)
    set(CROARING_SOURCE_BASE_NAME "CRoaring-${GRN_CROARING_EP_VERSION}.tar.gz")
    set(CROARING_SOURCE_LOCAL_PATH
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${CROARING_SOURCE_BASE_NAME}")
    if(EXISTS ${CROARING_SOURCE_LOCAL_PATH})
      set(CROARING_SOURCE_URL ${CROARING_SOURCE_LOCAL_PATH})
    else()
      set(CROARING_SOURCE_URL
          "https://github.com/RoaringBitmap/CRoaring/archive/refs/tags/")
      string(APPEND CROARING_SOURCE_URL "/v${GRN_CROARING_EP_VERSION}.tar.gz")
    endif()
    externalproject_add(
      croaring-ep
      ${GRN_EP_COMMON_OPTIONS}
      BUILD_BYPRODUCTS ${CROARING_STATIC_LIBRARY}
      CMAKE_ARGS ${CROARING_CMAKE_ARGS}
      INSTALL_DIR ${CROARING_PREFIX}
      URL ${CROARING_SOURCE_URL})
    file(MAKE_DIRECTORY "${CROARING_PREFIX}/include")
    add_library(roaring STATIC IMPORTED)
    set_target_properties(
      roaring
      PROPERTIES IMPORTED_LOCATION ${CROARING_STATIC_LIBRARY}
                 INTERFACE_INCLUDE_DIRECTORIES "${CROARING_PREFIX}/include")
    add_dependencies(roaring croaring-ep)
    set(CROARING_SOURCE_DIR
        "${CMAKE_CURRENT_BINARY_DIR}/croaring-ep-prefix/src/croaring-ep")
    install(
      FILES "${CROARING_SOURCE_DIR}/AUTHORS" "${CROARING_SOURCE_DIR}/LICENSE"
            "${CROARING_SOURCE_DIR}/README.md"
      DESTINATION "${GRN_DATA_DIR}/croaring")
    set(GRN_WITH_CROARING TRUE)
    target_link_libraries(grn_dependencies INTERFACE roaring)
    message(STATUS "Roaring bitmaps: ${CROARING_STATIC_LIBRARY} (bundled)")
  endif()
else()
  set(GRN_WITH_ROARING_BITMAPS FALSE)
endif()

set(GRN_BLOSC_REQUIRED_VERSION "2.10.0")
set(GRN_BLOSC_BUNDLED_VERSION "2.10.3")
set(GRN_BLOSC_BUNDLED_SHA256
    "ddd28cb4cfaa9143a5461a8cf5b2b8f4c7fcb4c84018c823d2a507998b937869")
set(GRN_WITH_BLOSC
    "no"
    CACHE STRING "Support column compression by Blosc.")
if(NOT "${GRN_WITH_BLOSC}" STREQUAL "no")
  if("${GRN_WITH_BLOSC}" STREQUAL "bundled")
    set(Blosc2_FOUND FALSE)
  else()
    if("${GRN_WITH_BLOSC}" STREQUAL "system")
      find_package(Blosc2 ${GRN_BLOSC_REQUIRED_VERSION} REQUIRED)
    else()
      find_package(Blosc2 ${GRN_BLOSC_REQUIRED_VERSION})
    endif()
  endif()
  if(Blosc2_FOUND)
    set(GRN_WITH_BLOSC TRUE)
    target_link_libraries(grn_dependencies INTERFACE Blosc2::blosc2_shared)
    message(STATUS "Blosc: system")
  else()
    # We need to use function to create a variable scope
    function(grn_build_blosc)
      set(BLOSC_SOURCE_BASE_NAME "c-blosc2-${GRN_BLOSC_BUNDLED_VERSION}.tar.gz")
      set(BLOSC_SOURCE_LOCAL_PATH
          "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${BLOSC_SOURCE_BASE_NAME}")
      if(EXISTS ${BLOSC_SOURCE_LOCAL_PATH})
        set(BLOSC_SOURCE_URL ${BLOSC_SOURCE_LOCAL_PATH})
      else()
        set(BLOSC_SOURCE_URL
            "https://github.com/Blosc/c-blosc2/archive/refs/tags")
        string(APPEND BLOSC_SOURCE_URL "/v${GRN_BLOSC_BUNDLED_VERSION}.tar.gz")
      endif()
      fetchcontent_declare(
        blosc
        ${GRN_FETCH_CONTENT_COMMON_OPTIONS}
        URL ${BLOSC_SOURCE_URL}
        URL_HASH "SHA256=${GRN_BLOSC_BUNDLED_SHA256}")
      set(BUILD_BENCHMARKS OFF)
      set(BUILD_EXAMPLES OFF)
      set(BUILD_FUZZERS OFF)
      set(BUILD_SHARED OFF)
      set(BUILD_STATIC ON)
      set(BUILD_TESTS OFF)
      set(PREFER_EXTERNAL_LZ4 ON)
      if(GRN_WITH_LZ4)
        set(LZ4_INCLUDE_DIR
            "$<TARGET_PROPERTY:${GRN_LZ4_TARGET},INTERFACE_INCLUDE_DIRECTORIES>"
        )
        if(GRN_WITH_BUNDLED_LZ4)
          if(MSVC)
            if(CMAKE_VERSION VERSION_LESS "3.27")
              message(
                FATAL_ERROR
                  "CMake 3.27 or later is required for bundled LZ4 and bundled Blosc"
              )
            endif()
            # Because we use shared library for now. We may want to use
            # static library for LZ4 like Zstandard.
            set(LZ4_LIBRARY "$<TARGET_IMPORT_FILE:${GRN_LZ4_TARGET}>")
          else()
            set(LZ4_LIBRARY "$<TARGET_FILE:${GRN_LZ4_TARGET}>")
          endif()
        else()
          set(LZ4_LIBRARY
              "$<TARGET_PROPERTY:${GRN_LZ4_TARGET},INTERFACE_LINK_LIBRARIES>")
        endif()
      endif()
      set(PREFER_EXTERNAL_ZLIB ON)
      set(PREFER_EXTERNAL_ZSTD ON)
      if(GRN_WITH_ZSTD)
        if(GRN_WITH_BUNDLED_ZSTD)
          set(ZSTD_LIBRARY "$<TARGET_FILE:${GRN_ZSTD_TARGET}>")
        else()
          set(ZSTD_INCLUDE_DIR
              "$<TARGET_PROPERTY:${GRN_ZSTD_TARGET},INTERFACE_INCLUDE_DIRECTORIES>"
          )
          set(ZSTD_LIBRARY
              "$<TARGET_PROPERTY:${GRN_ZSTD_TARGET},INTERFACE_LINK_LIBRARIES>")
        endif()
      endif()
      fetchcontent_makeavailable(blosc)
      if(CMAKE_VERSION VERSION_LESS 3.28)
        set_property(DIRECTORY ${blosc_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL
                                                            TRUE)
      endif()
      target_include_directories(
        blosc2_static
        INTERFACE "$<BUILD_INTERFACE:${blosc_SOURCE_DIR}/include>")
      install(
        FILES "${blosc_SOURCE_DIR}/LICENSE.txt"
              "${blosc_SOURCE_DIR}/LICENSES/BITSHUFFLE.txt"
              "${blosc_SOURCE_DIR}/LICENSES/FASTLZ.txt"
              "${blosc_SOURCE_DIR}/README.rst"
        DESTINATION "${GRN_DATA_DIR}/c-blosc2")
    endfunction()
    grn_build_blosc()
    set(GRN_WITH_BLOSC TRUE)
    target_link_libraries(grn_dependencies INTERFACE blosc2_static)
    message(STATUS "Blosc: bundled")
  endif()
else()
  set(GRN_WITH_BLOSC FALSE)
endif()

set(GRN_XSIMD_BUNDLED_VERSION "12.1.1")
set(GRN_XSIMD_BUNDLED_SHA256
    "73f94a051278ef3da4533b691d31244d12074d5d71107473a9fd8d7be15f0110")
set(GRN_WITH_XSIMD
    "auto"
    CACHE STRING "Support SIMD by xsimd.")
if(NOT GRN_WITH_SIMD)
  set(GRN_WITH_XSIMD "no")
endif()
if(NOT "${GRN_WITH_XSIMD}" STREQUAL "no")
  if("${GRN_WITH_XSIMD}" STREQUAL "bundled")
    set(xsimd_FOUND FALSE)
  else()
    if("${GRN_WITH_XSIMD}" STREQUAL "system")
      find_package(xsimd REQUIRED)
    else()
      find_package(xsimd)
    endif()
  endif()
  if(xsimd_FOUND)
    set(GRN_WITH_XSIMD TRUE)
    target_link_libraries(grn_dependencies INTERFACE xsimd)
    message(STATUS "xsimd: system")
  else()
    # We need to use function to create a variable scope
    function(grn_build_xsimd)
      set(XSIMD_SOURCE_BASE_NAME "xsimd-${GRN_XSIMD_BUNDLED_VERSION}.tar.gz")
      set(XSIMD_SOURCE_LOCAL_PATH
          "${CMAKE_CURRENT_SOURCE_DIR}/vendor/${XSIMD_SOURCE_BASE_NAME}")
      if(EXISTS ${XSIMD_SOURCE_LOCAL_PATH})
        set(XSIMD_SOURCE_URL ${XSIMD_SOURCE_LOCAL_PATH})
      else()
        set(XSIMD_SOURCE_URL
            "https://github.com/xtensor-stack/xsimd/archive/refs/tags")
        string(APPEND XSIMD_SOURCE_URL "/${GRN_XSIMD_BUNDLED_VERSION}.tar.gz")
      endif()
      fetchcontent_declare(
        xsimd
        ${GRN_FETCH_CONTENT_COMMON_OPTIONS}
        URL ${XSIMD_SOURCE_URL}
        URL_HASH "SHA256=${GRN_XSIMD_BUNDLED_SHA256}")
      set(BUILD_BENCHMARKS OFF)
      set(BUILD_EXAMPLES OFF)
      set(BUILD_TESTS OFF)
      fetchcontent_makeavailable(xsimd)
      if(CMAKE_VERSION VERSION_LESS 3.28)
        set_property(DIRECTORY ${xsimd_SOURCE_DIR} PROPERTY EXCLUDE_FROM_ALL
                                                            TRUE)
      endif()
      target_include_directories(
        xsimd INTERFACE "$<BUILD_INTERFACE:${xsimd_SOURCE_DIR}/include>")
      install(FILES "${xsimd_SOURCE_DIR}/LICENSE"
                    "${xsimd_SOURCE_DIR}/README.md"
              DESTINATION "${GRN_DATA_DIR}/xsimd")
    endfunction()
    grn_build_xsimd()
    set(GRN_WITH_XSIMD TRUE)
    target_link_libraries(grn_dependencies INTERFACE xsimd)
    message(STATUS "xsimd: bundled")
  endif()
else()
  set(GRN_WITH_XSIMD FALSE)
endif()
if(NOT GRN_WITH_XSIMD)
  set(GRN_WITH_SIMD OFF)
  set(GRN_WITH_SIMD_AVX OFF)
  set(GRN_WITH_SIMD_AVX2 OFF)
  set(GRN_WITH_SIMD_AVX512 OFF)
endif()

find_program(
  RUBY
  NAMES "ruby"
        "ruby3.0"
        "ruby30"
        "ruby2.7"
        "ruby27"
        "ruby2.6"
        "ruby26"
        "ruby2.5"
        "ruby25"
        "ruby2.4"
        "ruby24"
        "ruby2.3"
        "ruby23"
        "ruby2.2"
        "ruby22"
        "ruby2.1"
        "ruby21")

option(GRN_WITH_MRUBY "use mruby" OFF)

# For mruby-file-stat
if(NOT WIN32)
  ac_check_headers(sys/sysmacros.h)
endif()

set(GRN_WITH_ONIGMO ON)
option(GRN_WITH_BUNDLED_ONIGMO "use bundled Onigmo" ON)
if(NOT GRN_WITH_BUNDLED_ONIGMO)
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.18")
    pkg_check_modules(pkg_onigmo onigmo REQUIRED IMPORTED_TARGET)
  else()
    pkg_check_modules(pkg_onigmo onigmo REQUIRED IMPORTED_TARGET GLOBAL)
  endif()
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
option(GRN_WITH_DOC "build document" OFF)
if(GRN_WITH_DOC)
  add_subdirectory(doc)
endif()

configure_file(config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(GROONGA "${CMAKE_CURRENT_BINARY_DIR}/src/groonga")
set(GROONGA_SUGGEST_CREATE_DATASET
    "${CMAKE_CURRENT_BINARY_DIR}/src/suggest/groonga-suggest-create-dataset")
configure_file(config.sh.in "${CMAKE_CURRENT_BINARY_DIR}/config.sh" @ONLY)

set(prefix "${CMAKE_INSTALL_PREFIX}")
set(exec_prefix "\${prefix}")
set(bindir "\${exec_prefix}/${CMAKE_INSTALL_BINDIR}")
set(sbindir "\${exec_prefix}/${CMAKE_INSTALL_SBINDIR}")
set(libdir "\${prefix}/${CMAKE_INSTALL_LIBDIR}")
set(includedir "\${prefix}/${CMAKE_INSTALL_INCLUDEDIR}")
set(datarootdir "\${prefix}/${CMAKE_INSTALL_DATAROOTDIR}")
set(datadir "\${datarootdir}")
set(expanded_pluginsdir "${GRN_PLUGINS_DIR}")
set(GRN_EXPANDED_DEFAULT_DOCUMENT_ROOT "${GRN_DEFAULT_DOCUMENT_ROOT}")
set(EXEEXT "${CMAKE_EXECUTABLE_SUFFIX}")
configure_file(groonga.pc.in "${CMAKE_CURRENT_BINARY_DIR}/groonga.pc" @ONLY)

if(NOT GRN_EMBED)
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/groonga.pc"
          DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
endif()

install(FILES "COPYING" "README.md" DESTINATION "${GRN_DATA_DIR}")

add_subdirectory(vendor/plugins)

option(GRN_FOR_RHEL "install RHEL related files" OFF)
if(GRN_FOR_RHEL)
  install(FILES data/tmpfiles.d/groonga.conf
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/tmpfiles.d/)
  install(FILES data/logrotate.d/rhel/groonga-server-gqtp
                data/logrotate.d/rhel/groonga-server-http
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/logrotate.d/)
  install(FILES data/systemd/rhel/sysconfig/groonga-server-gqtp
                data/systemd/rhel/sysconfig/groonga-server-http
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/sysconfig/)
  install(FILES data/systemd/rhel/groonga-server-gqtp.service
                data/systemd/rhel/groonga-server-http.service
          DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/systemd/system/)
endif()

option(GRN_WITH_MUNIN_PLUGINS "install Munin plugins" OFF)
if(GRN_WITH_MUNIN_PLUGINS)
  install(
    PROGRAMS data/munin/groonga_cpu_load_
             data/munin/groonga_cpu_time_
             data/munin/groonga_delta_
             data/munin/groonga_status_
             data/munin/groonga_memory_
             data/munin/groonga_n_records_
             data/munin/groonga_query_performance_
             data/munin/groonga_disk_
             data/munin/groonga_throughput_
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${GRN_PROJECT_NAME}/munin/plugins)
endif()

option(GRN_WITH_BENCHMARKS "build benchmarks" OFF)
if(GRN_WITH_BENCHMARKS)
  add_subdirectory(benchmark)
endif()

option(GRN_WITH_EXAMPLES "install examples" OFF)
if(GRN_WITH_EXAMPLES)
  install(
    DIRECTORY examples/dictionary
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${GRN_PROJECT_NAME}/examples
    USE_SOURCE_PERMISSIONS FILES_MATCHING
    PATTERN "*"
    PATTERN "Makefile.*" EXCLUDE)
endif()

option(GRN_WITH_TOOLS "install tools" OFF)
if(GRN_WITH_TOOLS)
  install(
    DIRECTORY tools
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${GRN_PROJECT_NAME}/tools
    USE_SOURCE_PERMISSIONS FILES_MATCHING
    PATTERN "*.rb"
    PATTERN "*.sh")
endif()
