# Copyright(C) 2012-2013 Brazil
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
set(GRN_PROJECT_NAME "groonga")
project("${GRN_PROJECT_NAME}")

file(READ "${CMAKE_CURRENT_SOURCE_DIR}/base_version" VERSION)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/version.sh")
  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/version.sh" GRN_VERSION)
else()
  if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/version.sh")
    if(WIN32)
      set(GRN_VERSION "${VERSION}")
    else()
      execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/version-gen.sh")
      file(READ "${CMAKE_CURRENT_BINARY_DIR}/version.sh" GRN_VERSION)
    endif()
  endif()
endif()
string(REGEX REPLACE "(^.*=|\n)" "" GRN_VERSION "${GRN_VERSION}")

include(CheckIncludeFile)
include(CheckFunctionExists)
include(CheckLibraryExists)
include(FindPkgConfig)
include(${CMAKE_CURRENT_SOURCE_DIR}/build/cmake_modules/ReadFileList.cmake)

set(BIN_DIR "bin")
set(SBIN_DIR "sbin")
set(LIB_DIR "lib")
set(INCLUDE_DIR "include")
set(GRN_INCLUDE_DIR "include/groonga")
set(DATA_DIR "share")
set(GRN_DATA_DIR "${DATA_DIR}/${GRN_PROJECT_NAME}")
set(CONFIG_DIR "etc")
set(GRN_CONFIG_DIR "${CONFIG_DIR}/${GRN_PROJECT_NAME}")

set(GRN_LOG_PATH
  "${CMAKE_INSTALL_PREFIX}/var/log/${GRN_PROJECT_NAME}/${GRN_PROJECT_NAME}.log"
  CACHE FILEPATH "log file path")
set(GRN_DEFAULT_MATCH_ESCALATION_THRESHOLD
  0
  CACHE STRING "groonga default match escalation threshold")
set(GRN_DEFAULT_DOCUMENT_ROOT_BASE
  "html/admin"
  CACHE PATH "groonga default document root base path")
set(GRN_DEFAULT_RELATIVE_DOCUMENT_ROOT
  "share/${GRN_PROJECT_NAME}/${GRN_DEFAULT_DOCUMENT_ROOT_BASE}"
  CACHE PATH "groonga default relative document root")
set(GRN_DEFAULT_DOCUMENT_ROOT
  "${GRN_DATA_DIR}/${GRN_DEFAULT_DOCUMENT_ROOT_BASE}"
  CACHE PATH "groonga default document root")
set(GRN_STACK_SIZE
  1024
  CACHE STRING
  "DANGER!!! groonga stack size. Normarlly, you should not change this variable.")
set(GRN_RELATIVE_PLUGINS_DIR
  "${GRN_PROJECT_NAME}/plugins"
  CACHE PATH "groonga plugins' relative directory")
set(GRN_PLUGINS_DIR
  "${LIB_DIR}/${GRN_RELATIVE_PLUGINS_DIR}"
  CACHE PATH "groonga plugins' directory")
set(GRN_PLUGIN_SUFFIX "${CMAKE_SHARED_MODULE_SUFFIX}")
set(GRN_DLL_FILENAME
  "${CMAKE_SHARED_LIBRARY_PREFIX}groonga${CMAKE_SHARED_LIBRARY_SUFFIX}"
  CACHE PATH "groonga DLL filename")
set(GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE "synonyms.tsv")
set(GRN_QUERY_EXPANDER_TSV_SYNONYMS_FILE
  "${GRN_DATA_DIR}/${GRN_QUERY_EXPANDER_TSV_RELATIVE_SYNONYMS_FILE}")

if(CMAKE_COMPILER_IS_GNUCXX)
  set(GRN_NO_WARNING_COMPILE_FLAGS
    "${GRN_NO_WARNING_COMPILE_FLAGS} -Wno-unused-but-set-variable")
  set(GRN_NO_WARNING_COMPILE_FLAGS
    "${GRN_NO_WARNING_COMPILE_FLAGS} -Wno-sign-compare")
  set(GRN_C_COMPILE_FLAGS "${GRN_NO_WARNING_COMPILE_FLAGS} -Wno-pointer-sign")
  set(GRN_CXX_COMPILE_FLAGS "${GRN_NO_WARNING_COMPILE_FLAGS} -fexceptions")
endif()

add_definitions(
  -DHAVE_CONFIG_H
  )

include_directories(
  BEFORE
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  )

macro(ac_check_headers header)
  string(REGEX REPLACE "[/.]" "_" output_variable_name ${header})
  string(TOUPPER "${output_variable_name}" output_variable_name)
  set(output_variable_name "HAVE_${output_variable_name}")
  unset(${output_variable_name})
  check_include_file(${header} ${output_variable_name})
endmacro()

macro(ac_check_funcs function)
  string(TOUPPER "${function}" output_variable_name)
  set(output_variable_name "HAVE_${output_variable_name}")
  unset(${output_variable_name})
  check_function_exists(${function} ${output_variable_name})
endmacro()

macro(ac_check_lib library function)
  string(REGEX REPLACE "[/.]" "_" output_variable_base_name ${library})
  string(TOUPPER "${output_variable_base_name}" output_variable_base_name)
  set(output_variable_name "HAVE_LIB${output_variable_base_name}")
  set(location "${ARG2}")
  unset(${output_variable_name})
  check_library_exists(${library} ${function} "${location}"
    ${output_variable_name})
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
ac_check_lib(rt clock_gettime)
if(HAVE_LIBRT)
  set(HAVE_CLOCK_GETTIME TRUE)
endif()

if(UNIX)
  ac_check_headers(pthread.h)
  ac_check_lib(pthread pthread_mutex_init)
  if(NOT ${HAVE_LIBPTHREAD} EQUAL 1)
    message(FATAL_ERROR "No libpthread found")
  endif()
endif()

option(GRN_WITH_NFKC "use NFKC based UTF8 normalization." ON)
option(GRN_WITH_STATIC "build static library." OFF)

if(WIN32)
  ac_check_headers(winsock2.h)
  if(NOT ${HAVE_WINSOCK2_H} EQUAL 1)
    message(FATAL_ERROR "No winsock2.h found")
  endif()
  ac_check_lib(ws2_32 select)
  if(NOT ${HAVE_WS2_32_LIB} EQUAL 1)
    message(FATAL_ERROR "select() isn't found in ws2_32.lib")
  endif()
  set(USE_SELECT TRUE)
else()
  ac_check_headers(sys/epoll.h)
  if(${HAVE_SYS_EPOLL_H} EQUAL 1)
    ac_check_funcs(epoll_create)
    if(${HAVE_EPOLL_CREATE} EQUAL 1)
      set(USE_EPOLL TRUE)
    endif()
  endif()

  if(NOT USE_EPOLL)
    ac_check_headers(sys/event.h)
    if(${HAVE_SYS_EVENT_H} EQUAL 1)
      ac_check_funcs(kevent)
      if(${HAVE_KEVENT} EQUAL 1)
        set(USE_KQUEUE TRUE)
      endif()
    endif()

    if(NOT USE_KQUEUE)
      ac_check_headers(sys/poll.h)
      if(${HAVE_SYS_POLL_H} EQUAL 1)
        ac_check_funcs(poll)
        if(${HAVE_POLL} EQUAL 1)
          set(USE_POLL TRUE)
        endif()
      endif()

      if(NOT USE_POLL)
        ac_check_funcs(select)
        if(${HAVE_SELECT} EQUAL 1)
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

option(GRN_WITH_ZLIB "use zlib for data compression." OFF)
if(GRN_WITH_ZLIB)
  ac_check_lib(z compress)
  if(NOT HAVE_LIBZ)
    message(FATAL_ERROR "No libz found")
  endif()
endif()

option(GRN_WITH_LZO "use LZO for data compression." OFF)
if(GRN_WITH_LZO)
  ac_check_lib(lzo2 lzo1_compress)
  if(NOT HAVE_LIBLZO2)
    message(FATAL_ERROR "No liblzo2 found")
  endif()
endif()

set(GRN_WITH_MECAB "auto"
  CACHE STRING "use MeCab for morphological analysis")
if(NOT ${GRN_WITH_MECAB} STREQUAL "no")
  set(MECAB_CONFIG "mecab-config" CACHE FILEPATH "mecab-config path")
  if(NOT CMAKE_CROSSCOMPILING)
    find_program(MECAB_CONFIG_ABSOLUTE_PATH "${MECAB_CONFIG}")
  endif()
  if(EXISTS "${MECAB_CONFIG_ABSOLUTE_PATH}")
    execute_process(COMMAND "${MECAB_CONFIG_ABSOLUTE_PATH}" --inc-dir
      OUTPUT_VARIABLE MECAB_INCLUDE_DIRS
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(COMMAND "${MECAB_CONFIG_ABSOLUTE_PATH}" --libs-only-L
      OUTPUT_VARIABLE MECAB_LIBRARY_DIRS
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(MECAB_LIBRARIES "mecab")
    ac_check_lib(${MECAB_LIBRARIES} mecab_new)
    if(HAVE_LIBMECAB)
      set(GRN_WITH_MECAB TRUE)
    else()
      if(${GRN_WITH_MECAB} STREQUAL "yes")
	message(FATAL_ERROR
	  "No MeCab library found: "
	  "include directories: <${MECAB_INCLUDE_DIRS}>, "
	  "library directories: <${MECAB_LIBRARY_DIRS}>")
      endif()
      set(GRN_WITH_MECAB FALSE)
    endif()
  else()
    if(${GRN_WITH_MECAB} STREQUAL "yes")
      message(FATAL_ERROR "No mecab-config found: <${MECAB_CONFIG}>")
    endif()
    set(GRN_WITH_MECAB FALSE)
  endif()
else()
  set(GRN_WITH_MECAB FALSE)
endif()

set(GRN_WITH_KYTEA "auto"
  CACHE STRING "use KyTea for morphological analysis")
if(NOT ${GRN_WITH_KYTEA} STREQUAL "no")
  pkg_check_modules(KYTEA kytea)
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

set(GRN_WITH_ZEROMQ "auto"
  CACHE STRING "use ZeroMQ for suggestion")
if(NOT ${GRN_WITH_ZEROMQ} STREQUAL "no")
  pkg_check_modules(ZEROMQ libzmq)
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

set(GRN_WITH_MESSAGE_PACK "auto"
  CACHE STRING "use MessagePack for suggestion")
if(NOT ${GRN_WITH_MESSAGE_PACK} STREQUAL "no")
  if("${GRN_WITH_MESSAGE_PACK}" STREQUAL "yes" OR
      "${GRN_WITH_MESSAGE_PACK}" STREQUAL "auto")
    set(MESSAGE_PACK_INCLUDE_DIRS "")
    set(MESSAGE_PACK_LIBRARY_DIRS "")
  else()
    set(MESSAGE_PACK_INCLUDE_DIRS "${GRN_WITH_MESSAGE_PACK}/include")
    set(MESSAGE_PACK_LIBRARY_DIRS "${GRN_WITH_MESSAGE_PACK}/lib")
  endif()
  set(CMAKE_REQUIRED_INCLUDES_SAVE ${CMAKE_REQUIRED_INCLUDES})
  ac_check_lib(msgpack msgpack_version "${MESSAGE_PACK_LIBRARY_DIRS}")
  set(CMAKE_REQUIRED_INCLUDES ${CMAKE_REQUIRED_INCLUDES_SAVE})
  if(HAVE_LIBMSGPACK)
    set(MESSAGE_PACK_LIBRARIES "msgpack")
    set(GRN_WITH_MESSAGE_PACK TRUE)
  else()
    if(${GRN_WITH_MESSAGE_PACK} STREQUAL "yes")
      message(FATAL_ERROR "No MessagePack found")
    endif()
    set(GRN_WITH_MESSAGE_PACK FALSE)
  endif()
else()
  set(GRN_WITH_MESSAGE_PACK FALSE)
endif()

add_subdirectory(lib)
add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(include)
add_subdirectory(data)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

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

install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/groonga.pc"
  DESTINATION "${LIB_DIR}/pkgconfig/")

add_subdirectory(vendor/plugins)
