# Copyright(C) 2013-2018  Brazil
# Copyright(C) 2019-2023  Sutou Kouhei <kou@clear-code.com>
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

if(GRN_WITH_MRUBY)
  set(MRUBY_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../mruby-source")

  read_file_list("${CMAKE_CURRENT_SOURCE_DIR}/sources.am" MRUBY_SOURCES)
  string(REGEX REPLACE "\\.\\./" "${CMAKE_CURRENT_SOURCE_DIR}/../"
    MRUBY_SOURCES "${MRUBY_SOURCES}")

  read_file_list("${CMAKE_CURRENT_SOURCE_DIR}/built_sources.am"
    MRUBY_BUILT_SOURCES)
  set(mruby_pre_build_timestamp
    "${CMAKE_CURRENT_SOURCE_DIR}/mruby_build.timestamp")
  if(EXISTS "${mruby_pre_build_timestamp}")
    set(MRUBY_LEGAL_FILE "${CMAKE_CURRENT_SOURCE_DIR}/LEGAL")
    string(REGEX REPLACE "([^;]+)" "${CMAKE_CURRENT_SOURCE_DIR}/\\1"
      MRUBY_BUILT_SOURCES "${MRUBY_BUILT_SOURCES}")
  else()
    set(mruby_build_timestamp
      "${CMAKE_CURRENT_BINARY_DIR}/mruby_build.timestamp")
    if(NOT EXISTS "${mruby_build_timestamp}")
      set(MRUBY_BUILD_COMMAND)
      if(MSVC)
	list(APPEND MRUBY_BUILD_COMMAND
	  ${CMAKE_COMMAND}
	  -E
	  env
	  "CC=\"${CMAKE_C_COMPILER}\""
	  "CXX=\"${CMAKE_CXX_COMPILER}\""
	  "MRUBY_VC=yes")
      endif()
      list(APPEND MRUBY_BUILD_COMMAND
	"${RUBY}"
	"${CMAKE_CURRENT_SOURCE_DIR}/mruby_build.rb"
	"${CMAKE_CURRENT_SOURCE_DIR}/build_config.rb"
	"${MRUBY_SOURCE_DIR}"
	"${CMAKE_CURRENT_BINARY_DIR}/../mruby-build"
	"${CMAKE_CURRENT_SOURCE_DIR}/../onigmo-source"
	"${mruby_build_timestamp}"
	WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
	RESULT_VARIABLE MRUBY_BUILD_RESULT)
      execute_process(COMMAND ${MRUBY_BUILD_COMMAND})
      if(NOT MRUBY_BUILD_RESULT EQUAL "0")
	message(FATAL_ERROR "Failed to build mruby files")
      endif()
    endif()
    set(MRUBY_LEGAL_FILE "${CMAKE_CURRENT_BINARY_DIR}/LEGAL")
    string(REGEX REPLACE "([^;]+)" "${CMAKE_CURRENT_BINARY_DIR}/\\1"
      MRUBY_BUILT_SOURCES "${MRUBY_BUILT_SOURCES}")
  endif()
  if(HAVE_SYS_SYSMACROS_H)
    set(MRUBY_FILE_STAT_CONFIG_H_CONTENT "#define HAVE_SYS_SYSMACROS_H 1")
  else()
    set(MRUBY_FILE_STAT_CONFIG_H_CONTENT "")
  endif()
  file(WRITE
    "${CMAKE_CURRENT_BINARY_DIR}/mruby-file-stat/src/config.h"
    "${MRUBY_FILE_STAT_CONFIG_H_CONTENT}")

  set(MRUBY_ALL_SOURCES
    ${MRUBY_SOURCES}
    ${MRUBY_BUILT_SOURCES}
    "${CMAKE_CURRENT_BINARY_DIR}/mruby-file-stat/src/config.h"
    )
  add_library(mruby_headers INTERFACE)
  target_include_directories(mruby_headers INTERFACE
    "${MRUBY_SOURCE_DIR}/include")
  add_library(mruby STATIC ${MRUBY_ALL_SOURCES})
  target_include_directories(mruby
    PUBLIC
    "${MRUBY_SOURCE_DIR}/include"
    PRIVATE
    "${MRUBY_SOURCE_DIR}/src"
    "${MRUBY_SOURCE_DIR}/mrbgems/mruby-compiler/core"
    "${MRUBY_SOURCE_DIR}/mrbgems/mruby-io/include"
    "${MRUBY_SOURCE_DIR}/mrbgems/mruby-time/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../onigmo-source"
  )
  target_compile_definitions(mruby PUBLIC "MRB_INT64" "HAVE_ONIGMO_H")
  if(WIN32)
    target_compile_definitions(mruby PRIVATE "MRB_BUILD_AS_DLL" "MRB_CORE")
  endif()
  set_target_properties(
    mruby
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON)

  set(MRUBY_C_COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS}")
  if(GRN_C_COMPILER_GNU_LIKE)
    set(MRUBY_C_COMPILE_FLAGS
      "${MRUBY_C_COMPILE_FLAGS} -Wno-bad-function-cast")
    set(MRUBY_C_COMPILE_FLAGS
      "${MRUBY_C_COMPILE_FLAGS} -Wno-float-equal")
    set(MRUBY_C_COMPILE_FLAGS
      "${MRUBY_C_COMPILE_FLAGS} -Wno-missing-field-initializers")
    set(MRUBY_C_COMPILE_FLAGS
      "${MRUBY_C_COMPILE_FLAGS} -Wno-sign-compare")
  elseif(MSVC)
    # 'token' : signed/unsigned mismatch
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4018
    set(MRUBY_C_COMPILE_FLAGS "${MRUBY_C_COMPILE_FLAGS} /wd4018")
    # 'argument' : conversion from 'type1' to 'type2', possible loss of data
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-2-c4244
    set(MRUBY_C_COMPILE_FLAGS "${MRUBY_C_COMPILE_FLAGS} /wd4244")
    # 'var' : conversion from 'size_t' to 'type', possible loss of data
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4267
    set(MRUBY_C_COMPILE_FLAGS "${MRUBY_C_COMPILE_FLAGS} /wd4267")
    # Your code uses a function, class member, variable, or typedef
    # that's marked deprecated.
    # https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warning-level-3-c4996
    set(MRUBY_C_COMPILE_FLAGS "${MRUBY_C_COMPILE_FLAGS} /wd4996")
  endif()
  set_source_files_properties(${MRUBY_ALL_SOURCES}
    PROPERTIES
    COMPILE_FLAGS "${MRUBY_C_COMPILE_FLAGS}")

  install(FILES
    "${MRUBY_LEGAL_FILE}"
    "${MRUBY_SOURCE_DIR}/AUTHORS"
    "${MRUBY_SOURCE_DIR}/README.md"
    "${MRUBY_SOURCE_DIR}/LICENSE"
    DESTINATION "${GRN_DATA_DIR}/mruby")
endif()
