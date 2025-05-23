# Copyright (C) 2012-2018  Brazil
# Copyright (C) 2019-2024  Sutou Kouhei <kou@clear-code.com>
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

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/c_sources.am LIBGROONGA_C_SOURCES)
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/cpp_sources.am
               LIBGROONGA_CPP_SOURCES)
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/dat/sources.am LIBGRNDAT_SOURCES)
string(REGEX REPLACE "([^;]+)" "dat/\\1" LIBGRNDAT_SOURCES
                     "${LIBGRNDAT_SOURCES}")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/sources.am LIBGRNMRB_SOURCES)
string(REGEX REPLACE "([^;]+)" "mrb/\\1" LIBGRNMRB_SOURCES
                     "${LIBGRNMRB_SOURCES}")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/proc/c_sources.am
               LIBGRNPROC_C_SOURCES)
string(REGEX REPLACE "([^;]+)" "proc/\\1" LIBGRNPROC_C_SOURCES
                     "${LIBGRNPROC_C_SOURCES}")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/proc/cpp_sources.am
               LIBGRNPROC_CPP_SOURCES)
string(REGEX REPLACE "([^;]+)" "proc/\\1" LIBGRNPROC_CPP_SOURCES
                     "${LIBGRNPROC_CPP_SOURCES}")

if(WIN32)
  configure_file("metadata.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/metadata.rc"
                 @ONLY)
  set(LIBGROONGA_METADATA_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/metadata.rc")
else()
  set(LIBGROONGA_METADATA_SOURCES)
endif()

set_source_files_properties(
  ${LIBGROONGA_C_SOURCES} ${LIBGRNMRB_SOURCES} ${LIBGRNPROC_C_SOURCES}
  PROPERTIES COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS}")
set_source_files_properties(
  ${LIBGROONGA_CPP_SOURCES} ${LIBGRNDAT_SOURCES} ${LIBGRNPROC_CPP_SOURCES}
  PROPERTIES COMPILE_FLAGS "${GRN_CXX_COMPILE_FLAGS}")

if(GRN_C_COMPILER_GNU_LIKE)
  set_property(
    SOURCE distance.cpp
    APPEND_STRING
    PROPERTY COMPILE_FLAGS " -Wno-unknown-pragmas -Wno-float-equal")
endif()
if(GRN_WITH_XSIMD)
  if(GRN_WITH_SIMD_AVX)
    list(APPEND LIBGROONGA_CPP_SOURCES distance_avx.cpp)
    set_property(
      SOURCE distance_avx.cpp
      APPEND_STRING
      PROPERTY COMPILE_FLAGS " ${GRN_C_FLAGS_AVX}")
  endif()
  if(GRN_WITH_SIMD_AVX2)
    list(APPEND LIBGROONGA_CPP_SOURCES distance_avx2.cpp)
    set_property(
      SOURCE distance_avx2.cpp
      APPEND_STRING
      PROPERTY COMPILE_FLAGS " ${GRN_C_FLAGS_AVX2}")
  endif()
  if(GRN_WITH_SIMD_AVX512)
    list(APPEND LIBGROONGA_CPP_SOURCES distance_avx512.cpp)
    set_property(
      SOURCE distance_avx512.cpp
      APPEND_STRING
      PROPERTY COMPILE_FLAGS " ${GRN_C_FLAGS_AVX512}")
  endif()
  if(GRN_WITH_SIMD_NEON64)
    list(APPEND LIBGROONGA_CPP_SOURCES distance_neon64.cpp)
    set_property(
      SOURCE distance_neon64.cpp
      APPEND_STRING
      PROPERTY COMPILE_FLAGS " ${GRN_C_FLAGS_NEON64}")
  endif()
  list(APPEND LIBGROONGA_CPP_SOURCES distance_generic.cpp)
  if(GRN_C_COMPILER_GNU_LIKE)
    set_property(
      SOURCE distance_generic.cpp
      APPEND_STRING
      PROPERTY COMPILE_FLAGS " -Wno-float-equal")
  endif()
endif()

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
  target_compile_definitions(libgroonga PUBLIC GRN_STATIC)
  target_compile_definitions(libgroonga PUBLIC GRN_EMBEDDED)
  set_target_properties(libgroonga PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
  add_library(libgroonga SHARED ${GRN_ALL_SOURCES})
  target_compile_definitions(libgroonga PRIVATE GRN_DAT_EXPORT)
endif()
target_compile_definitions(libgroonga PUBLIC "$<BUILD_INTERFACE:HAVE_CONFIG_H>")
target_include_directories(libgroonga BEFORE
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/dat)
target_include_directories(
  libgroonga BEFORE
  PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/..>"
         "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../include>"
         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>"
         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
if(NOT MSVC)
  set_target_properties(libgroonga PROPERTIES OUTPUT_NAME "groonga")
endif()
set_target_properties(libgroonga PROPERTIES VERSION "0.0.0" SOVERSION "0")

find_program(LEMON lemon)
if(LEMON)
  set_property(
    SOURCE expr.c
    PROPERTY OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/grn_ecmascript.c"
             "${CMAKE_CURRENT_SOURCE_DIR}/grn_ecmascript.h")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/grn_ecmascript.c"
           "${CMAKE_CURRENT_SOURCE_DIR}/grn_ecmascript.h"
    COMMAND ${LEMON} grn_ecmascript.lemon
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/grn_ecmascript.lemon"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    VERBATIM)
endif()

if(GRN_EMBED)
  set(GRN_EMBEDDED_PLUGIN_LIBRARIES "")
  if(GRN_WITH_MECAB)
    list(APPEND GRN_EMBEDDED_PLUGIN_LIBRARIES mecab_tokenizer)
  endif()
  target_link_libraries(libgroonga PRIVATE grn_dependencies
                                           ${GRN_EMBEDDED_PLUGIN_LIBRARIES})
else()
  target_link_libraries(libgroonga PRIVATE grn_dependencies)
  target_include_directories(
    libgroonga
    INTERFACE "$<INSTALL_INTERFACE:${CMAKE_INSTALL_FULL_INCLUDEDIR}/groonga>")
  install(
    TARGETS libgroonga
    EXPORT libgroonga_export
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  set(CONFIG_CMAKE "GroongaConfig.cmake")
  set(BUILT_CONFIG_CMAKE "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_CMAKE}")
  configure_package_config_file("${CONFIG_CMAKE}.in" "${BUILT_CONFIG_CMAKE}"
                                INSTALL_DESTINATION "${GRN_INSTALL_CMAKE_DIR}")
  set(CONFIG_VERSION_CMAKE "GroongaConfigVersion.cmake")
  set(BUILT_CONFIG_VERSION_CMAKE
      "${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_VERSION_CMAKE}")
  write_basic_package_version_file("${BUILT_CONFIG_VERSION_CMAKE}"
                                   COMPATIBILITY AnyNewerVersion)
  install(FILES "${BUILT_CONFIG_CMAKE}" "${BUILT_CONFIG_VERSION_CMAKE}"
          DESTINATION "${GRN_INSTALL_CMAKE_DIR}")
  install(
    EXPORT libgroonga_export
    DESTINATION "${GRN_INSTALL_CMAKE_DIR}"
    NAMESPACE "Groonga::"
    FILE "GroongaTargets.cmake")
  if(MSVC)
    install(
      FILES $<TARGET_PDB_FILE:libgroonga>
      DESTINATION "${CMAKE_INSTALL_BINDIR}"
      OPTIONAL)
  endif()
endif()
if(GRN_WITH_MESSAGE_PACK)
  if(GRN_WITH_BUNDLED_MESSAGE_PACK)
    target_link_libraries(libgroonga
                          INTERFACE "$<BUILD_INTERFACE:Groonga::msgpackc>")
  else()
    target_link_libraries(libgroonga INTERFACE Groonga::msgpackc)
  endif()
endif()
if(GRN_WITH_LLAMA_CPP_BUNDLED)
  target_link_libraries(libgroonga
                        INTERFACE "$<BUILD_INTERFACE:Groonga::llama>")
  target_link_libraries(libgroonga INTERFACE "$<BUILD_INTERFACE:Groonga::ggml>")
  target_link_libraries(libgroonga
                        INTERFACE "$<BUILD_INTERFACE:Groonga::ggml-base>")
endif()
if(NOT WASI)
  target_link_libraries(libgroonga PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
endif()
if(WIN32)
  target_link_libraries(libgroonga PRIVATE ws2_32.lib)
endif()

if(GRN_WITH_MRUBY)
  target_link_libraries(libgroonga INTERFACE "$<BUILD_INTERFACE:mruby_headers>")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/sources.am
                 RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/\\1" RUBY_SCRIPTS
                       "${RUBY_SCRIPTS}")
  install(FILES ${RUBY_SCRIPTS} DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/command_line/sources.am
                 COMMANE_LINE_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/command_line/\\1"
                       COMMANE_LINE_RUBY_SCRIPTS "${COMMANE_LINE_RUBY_SCRIPTS}")
  install(FILES ${COMMANE_LINE_RUBY_SCRIPTS}
          DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/command_line")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/context/sources.am
                 CONTEXT_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/context/\\1" CONTEXT_RUBY_SCRIPTS
                       "${CONTEXT_RUBY_SCRIPTS}")
  install(FILES ${CONTEXT_RUBY_SCRIPTS}
          DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/context")

  read_file_list(
    ${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/expression_rewriters/sources.am
    EXPRESSION_REWRITERS_RUBY_SCRIPTS)
  string(
    REGEX
    REPLACE "([^;]+)" "mrb/scripts/expression_rewriters/\\1"
            EXPRESSION_REWRITERS_RUBY_SCRIPTS
            "${EXPRESSION_REWRITERS_RUBY_SCRIPTS}")
  install(FILES ${EXPRESSION_REWRITERS_RUBY_SCRIPTS}
          DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/expression_rewriters")

  read_file_list(
    ${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/expression_tree/sources.am
    EXPRESSION_TREE_RUBY_SCRIPTS)
  string(REGEX
         REPLACE "([^;]+)" "mrb/scripts/expression_tree/\\1"
                 EXPRESSION_TREE_RUBY_SCRIPTS "${EXPRESSION_TREE_RUBY_SCRIPTS}")
  install(FILES ${EXPRESSION_TREE_RUBY_SCRIPTS}
          DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/expression_tree")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/initialize/sources.am
                 INITIALIZE_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/initialize/\\1"
                       INITIALIZE_RUBY_SCRIPTS "${INITIALIZE_RUBY_SCRIPTS}")
  install(FILES ${INITIALIZE_RUBY_SCRIPTS}
          DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/initialize")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/logger/sources.am
                 LOGGER_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/logger/\\1" LOGGER_RUBY_SCRIPTS
                       "${LOGGER_RUBY_SCRIPTS}")
  install(FILES ${LOGGER_RUBY_SCRIPTS}
          DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/logger")

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mrb/scripts/query_logger/sources.am
                 QUERY_LOGGER_RUBY_SCRIPTS)
  string(REGEX REPLACE "([^;]+)" "mrb/scripts/query_logger/\\1"
                       QUERY_LOGGER_RUBY_SCRIPTS "${QUERY_LOGGER_RUBY_SCRIPTS}")
  install(FILES ${QUERY_LOGGER_RUBY_SCRIPTS}
          DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/query_logger")
endif()
