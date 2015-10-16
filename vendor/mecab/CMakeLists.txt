# Copyright(C) 2015 Brazil
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

set(MECAB_VERSION "0.996")
set(MECAB_DICT_VERSION "102")
set(MECAB_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../mecab-${MECAB_VERSION}")
set(MECAB_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/../mecab-${MECAB_VERSION}")

if(EXISTS ${MECAB_SOURCE_DIR})
  include_directories(
    BEFORE
    ${MECAB_BINARY_DIR}
    ${MECAB_SOURCE_DIR}
    )

  set(MECAB_SOURCES
    "${MECAB_SOURCE_DIR}/src/char_property.cpp"
    "${MECAB_SOURCE_DIR}/src/char_property.h"
    "${MECAB_SOURCE_DIR}/src/common.h"
    "${MECAB_SOURCE_DIR}/src/connector.cpp"
    "${MECAB_SOURCE_DIR}/src/connector.h"
    "${MECAB_SOURCE_DIR}/src/context_id.cpp"
    "${MECAB_SOURCE_DIR}/src/context_id.h"
    "${MECAB_SOURCE_DIR}/src/darts.h"
    "${MECAB_SOURCE_DIR}/src/dictionary.cpp"
    "${MECAB_SOURCE_DIR}/src/dictionary.h"
    "${MECAB_SOURCE_DIR}/src/dictionary_compiler.cpp"
    "${MECAB_SOURCE_DIR}/src/dictionary_generator.cpp"
    "${MECAB_SOURCE_DIR}/src/dictionary_rewriter.cpp"
    "${MECAB_SOURCE_DIR}/src/dictionary_rewriter.h"
    "${MECAB_SOURCE_DIR}/src/eval.cpp"
    "${MECAB_SOURCE_DIR}/src/feature_index.cpp"
    "${MECAB_SOURCE_DIR}/src/feature_index.h"
    "${MECAB_SOURCE_DIR}/src/freelist.h"
    "${MECAB_SOURCE_DIR}/src/iconv_utils.cpp"
    "${MECAB_SOURCE_DIR}/src/iconv_utils.h"
    "${MECAB_SOURCE_DIR}/src/lbfgs.cpp"
    "${MECAB_SOURCE_DIR}/src/lbfgs.h"
    "${MECAB_SOURCE_DIR}/src/learner.cpp"
    "${MECAB_SOURCE_DIR}/src/learner_node.h"
    "${MECAB_SOURCE_DIR}/src/learner_tagger.cpp"
    "${MECAB_SOURCE_DIR}/src/learner_tagger.h"
    "${MECAB_SOURCE_DIR}/src/libmecab.cpp"
    # "${MECAB_SOURCE_DIR}/src/mecab-cost-train.cpp"
    # "${MECAB_SOURCE_DIR}/src/mecab-dict-gen.cpp"
    # "${MECAB_SOURCE_DIR}/src/mecab-dict-index.cpp"
    # "${MECAB_SOURCE_DIR}/src/mecab-system-eval.cpp"
    # "${MECAB_SOURCE_DIR}/src/mecab-test-gen.cpp"
    "${MECAB_SOURCE_DIR}/src/mecab.cpp"
    "${MECAB_SOURCE_DIR}/src/mecab.h"
    "${MECAB_SOURCE_DIR}/src/mmap.h"
    "${MECAB_SOURCE_DIR}/src/nbest_generator.cpp"
    "${MECAB_SOURCE_DIR}/src/nbest_generator.h"
    "${MECAB_SOURCE_DIR}/src/param.cpp"
    "${MECAB_SOURCE_DIR}/src/param.h"
    "${MECAB_SOURCE_DIR}/src/scoped_ptr.h"
    "${MECAB_SOURCE_DIR}/src/stream_wrapper.h"
    "${MECAB_SOURCE_DIR}/src/string_buffer.cpp"
    "${MECAB_SOURCE_DIR}/src/string_buffer.h"
    "${MECAB_SOURCE_DIR}/src/tagger.cpp"
    "${MECAB_SOURCE_DIR}/src/thread.h"
    "${MECAB_SOURCE_DIR}/src/tokenizer.cpp"
    "${MECAB_SOURCE_DIR}/src/tokenizer.h"
    "${MECAB_SOURCE_DIR}/src/ucs.h"
    "${MECAB_SOURCE_DIR}/src/ucstable.h"
    "${MECAB_SOURCE_DIR}/src/utils.cpp"
    "${MECAB_SOURCE_DIR}/src/utils.h"
    "${MECAB_SOURCE_DIR}/src/viterbi.cpp"
    "${MECAB_SOURCE_DIR}/src/viterbi.h"
    "${MECAB_SOURCE_DIR}/src/winmain.h"
    "${MECAB_SOURCE_DIR}/src/writer.cpp"
    "${MECAB_SOURCE_DIR}/src/writer.h"
    )

  set(MECAB_CXX_COMPILE_FLAGS "${GRN_CXX_COMPILE_FLAGS}")
  if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGCXX)
    set(MECAB_CXX_COMPILE_FLAGS
      "${MECAB_CXX_COMPILE_FLAGS} -Wno-type-limits")
    set(MECAB_CXX_COMPILE_FLAGS
      "${MECAB_CXX_COMPILE_FLAGS} -Wno-float-equal")
    set(MECAB_CXX_COMPILE_FLAGS
      "${MECAB_CXX_COMPILE_FLAGS} -Wno-ignored-qualifiers")
    set(MECAB_CXX_COMPILE_FLAGS
      "${MECAB_CXX_COMPILE_FLAGS} -Wno-unused-function")
  endif()

  add_definitions("-DPACKAGE=\"mecab\"")
  add_definitions("-DVERSION=\"${MECAB_VERSION}\"")
  add_definitions("-DDIC_VERSION=${MECAB_DICT_VERSION}")
  if(MSVC)
    add_definitions(
      "-DMECAB_DEFAULT_RC=\"c:\\Program Files\\mecab\\etc\\mecabrc\"")
    add_definitions(-DMECAB_USE_THREAD)
    add_definitions(-DDLL_EXPORT)
    add_definitions(-DHAVE_GETENV)
    add_definitions(-DHAVE_WINDOWS_H)
    add_definitions(-DUNICODE)
    add_definitions(-D_UNICODE)
  else()
    add_definitions(
      "-DMECAB_DEFAULT_RC=\"${CMAKE_INSTALL_PREFIX}${CONFIG_DIR}/mecab/mecabrc\"")
    add_definitions(-DHAVE_DIRENT_H)
    add_definitions(-DHAVE_FCNTL_H)
    add_definitions(-DHAVE_STDINT_H)
    add_definitions(-DHAVE_STRING_H)
    add_definitions(-DHAVE_SYS_MMAN_H)
    add_definitions(-DHAVE_SYS_STAT_H)
    add_definitions(-DHAVE_SYS_TYPES_H)
    add_definitions(-DHAVE_UNISTD_H)
  endif()
  set_source_files_properties(${MECAB_SOURCES}
    PROPERTIES
    COMPILE_FLAGS "${MECAB_CXX_COMPILE_FLAGS}")

  if(GRN_BUNDLED)
    add_library(mecab STATIC ${MECAB_SOURCES})
    set_target_properties(
      mecab
      PROPERTIES
      POSITION_INDEPENDENT_CODE ON)
  else()
    add_library(mecab SHARED ${MECAB_SOURCES})
  endif()

  configure_file(config.h.cmake "${MECAB_BINARY_DIR}/config.h")
endif()
