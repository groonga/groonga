# Copyright(C) 2012-2018  Brazil
# Copyright(C) 2019-2021  Sutou Kouhei <kou@clear-code.com>
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

add_definitions(
  -DGRN_DAT_EXPORT
  ${LIBXXHASH_DEFINITIONS}
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/dat
  ${ONIGMO_INCLUDE_DIRS}
  ${MRUBY_INCLUDE_DIRS}
  # We need to put LIBXXHASH_INCLUDE_DIRS before LIBLZ4_INCLUDE_DIRS
  # because bundled LZ4 includes its xxhash.h.
  ${LIBXXHASH_INCLUDE_DIRS}
  ${LIBLZ4_INCLUDE_DIRS}
  ${MESSAGE_PACK_INCLUDE_DIRS})

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/c_sources.am LIBGROONGA_C_SOURCES)
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/cpp_sources.am LIBGROONGA_CPP_SOURCES)
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/dat/sources.am LIBGRNDAT_SOURCES)
string(REGEX REPLACE "([^;]+)" "dat/\\1"
  LIBGRNDAT_SOURCES "${LIBGRNDAT_SOURCES}")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/sources.am LIBGRNMRB_SOURCES)
string(REGEX REPLACE "([^;]+)" "mrb/\\1"
  LIBGRNMRB_SOURCES "${LIBGRNMRB_SOURCES}")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/proc/c_sources.am
  LIBGRNPROC_C_SOURCES)
string(REGEX REPLACE "([^;]+)" "proc/\\1"
  LIBGRNPROC_C_SOURCES "${LIBGRNPROC_C_SOURCES}")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/proc/cpp_sources.am
  LIBGRNPROC_CPP_SOURCES)
string(REGEX REPLACE "([^;]+)" "proc/\\1"
  LIBGRNPROC_CPP_SOURCES "${LIBGRNPROC_CPP_SOURCES}")

if(WIN32)
  configure_file(
    "metadata.rc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/metadata.rc"
    @ONLY)
  set(LIBGROONGA_METADATA_SOURCES
    "${CMAKE_CURRENT_BINARY_DIR}/metadata.rc")
else()
  set(LIBGROONGA_METADATA_SOURCES)
endif()

set_source_files_properties(
  ${LIBGROONGA_C_SOURCES}
  ${LIBGRNMRB_SOURCES}
  ${LIBGRNPROC_C_SOURCES}
  PROPERTIES
  COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS}")
set_source_files_properties(
  ${LIBGROONGA_C_SOURCES}
  ${LIBGROONGA_CPP_SOURCES}
  ${LIBGRNMRB_SOURCES}
  PROPERTIES
  COMPILE_DEFINITIONS "${MRUBY_DEFINITIONS}")
set_source_files_properties(
  ${LIBGROONGA_CPP_SOURCES}
  ${LIBGRNDAT_SOURCES}
  ${LIBGRNPROC_CPP_SOURCES}
  PROPERTIES
  COMPILE_FLAGS "${GRN_CXX_COMPILE_FLAGS}")

set(GRN_ALL_SOURCES
  ${LIBGROONGA_C_SOURCES}
  ${LIBGROONGA_CPP_SOURCES}
  ${LIBGRNDAT_SOURCES}
  ${LIBGRNMRB_SOURCES}
  ${LIBGRNPROC_C_SOURCES}
  ${LIBGRNPROC_CPP_SOURCES}
  ${LIBGROONGA_METADATA_SOURCES})
if(GRN_EMBED)
  add_library(libgroonga STATIC ${GRN_ALL_SOURCES})
  set_target_properties(
    libgroonga
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON)
else()
  add_library(libgroonga SHARED ${GRN_ALL_SOURCES})
endif()
if(NOT MSVC)
  set_target_properties(libgroonga PROPERTIES OUTPUT_NAME "groonga")
endif()
if(GRN_WITH_APACHE_ARROW AND Arrow_VERSION VERSION_GREATER 9.0.0)
  set_target_properties(libgroonga PROPERTIES CXX_STANDARD 17)
elseif(GRN_WITH_RAPIDJSON OR GRN_WITH_APACHE_ARROW)
  set_target_properties(libgroonga PROPERTIES CXX_STANDARD 11)
endif()

set(GRN_ALL_LIBRARIES
    ${ATOMIC_LIBS}
    ${EXECINFO_LIBS}
    ${RT_LIBS}
    ${PTHREAD_LIBS}
    ${Z_LIBS}
    ${LIBLZ4_LIBS}
    ${GRN_ZSTD_TARGET}
    ${MESSAGE_PACK_LIBS}
    ${CMAKE_DL_LIBS}
    ${M_LIBS}
    ${MRUBY_LIBS}
    ${WS2_32_LIBS}
    ${DBGHELP_LIBS}
    ${PSAPI_LIBS}
    ${ONIGMO_LIBS}
    ${LIBXXHASH_LIBS})
if(GRN_WITH_RAPIDJSON)
  list(APPEND GRN_ALL_LIBRARIES rapidjson)
endif()
if(GRN_WITH_APACHE_ARROW)
  list(APPEND GRN_ALL_LIBRARIES arrow_shared)
endif()
if(GRN_EMBED)
  set(GRN_EMBEDDED_PLUGIN_LIBRARIES "")
  if(GRN_WITH_MECAB)
    list(APPEND GRN_EMBEDDED_PLUGIN_LIBRARIES mecab_tokenizer)
  endif()
  target_link_libraries(libgroonga
    ${GRN_ALL_LIBRARIES}
    ${STDCPP_LIBS}
    ${GRN_EMBEDDED_PLUGIN_LIBRARIES})
else()
  target_link_libraries(libgroonga
    ${GRN_ALL_LIBRARIES})
  install(TARGETS libgroonga
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  if(MSVC)
    install(FILES $<TARGET_PDB_FILE:libgroonga>
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
      OPTIONAL)
  endif()
endif()

if(GRN_WITH_MRUBY)
  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/sources.am
    RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/\\1"
    RUBY_SCRIPTS "${RUBY_SCRIPTS}")
  install(
    FILES ${RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/command_line/sources.am
    COMMANE_LINE_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/command_line/\\1"
    COMMANE_LINE_RUBY_SCRIPTS "${COMMANE_LINE_RUBY_SCRIPTS}")
  install(
    FILES ${COMMANE_LINE_RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/command_line")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/context/sources.am
    CONTEXT_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/context/\\1"
    CONTEXT_RUBY_SCRIPTS "${CONTEXT_RUBY_SCRIPTS}")
  install(
    FILES ${CONTEXT_RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/context")

  read_file_list(
    ${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/expression_rewriters/sources.am
    EXPRESSION_REWRITERS_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/expression_rewriters/\\1"
    EXPRESSION_REWRITERS_RUBY_SCRIPTS "${EXPRESSION_REWRITERS_RUBY_SCRIPTS}")
  install(
    FILES ${EXPRESSION_REWRITERS_RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/expression_rewriters")

  read_file_list(
    ${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/expression_tree/sources.am
    EXPRESSION_TREE_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/expression_tree/\\1"
    EXPRESSION_TREE_RUBY_SCRIPTS "${EXPRESSION_TREE_RUBY_SCRIPTS}")
  install(
    FILES ${EXPRESSION_TREE_RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/expression_tree")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/initialize/sources.am
    INITIALIZE_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/initialize/\\1"
    INITIALIZE_RUBY_SCRIPTS "${INITIALIZE_RUBY_SCRIPTS}")
  install(
    FILES ${INITIALIZE_RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/initialize")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/logger/sources.am
    LOGGER_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/logger/\\1"
    LOGGER_RUBY_SCRIPTS "${LOGGER_RUBY_SCRIPTS}")
  install(
    FILES ${LOGGER_RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/logger")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/query_logger/sources.am
    QUERY_LOGGER_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/query_logger/\\1"
    QUERY_LOGGER_RUBY_SCRIPTS "${QUERY_LOGGER_RUBY_SCRIPTS}")
  install(
    FILES ${QUERY_LOGGER_RUBY_SCRIPTS}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/query_logger")
endif()
