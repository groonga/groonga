# Copyright(C) 2014-2018  Brazil
# Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>
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

set(TOKEN_FILTERS_DIR "${GRN_RELATIVE_PLUGINS_DIR}/token_filters")

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/stop_word_sources.am
               STOP_WORD_SOURCES)
set_source_files_properties(${STOP_WORD_SOURCES}
                            PROPERTIES COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS}")
if(GRN_EMBED)
  add_library(stop_word_token_filter STATIC ${STOP_WORD_SOURCES})
  set_target_properties(stop_word_token_filter
                        PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
  add_library(stop_word_token_filter MODULE ${STOP_WORD_SOURCES})
  set_target_properties(stop_word_token_filter PROPERTIES PREFIX "" OUTPUT_NAME
                                                                    "stop_word")
  install(TARGETS stop_word_token_filter DESTINATION "${TOKEN_FILTERS_DIR}")
  if(MSVC)
    install(
      FILES $<TARGET_PDB_FILE:stop_word_token_filter>
      DESTINATION "${TOKEN_FILTERS_DIR}"
      OPTIONAL)
  endif()
endif()
target_link_libraries(stop_word_token_filter libgroonga)

if(GRN_WITH_LIBSTEMMER)
  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/stem_sources.am STEM_SOURCES)
  set_source_files_properties(${STEM_SOURCES}
                              PROPERTIES COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS}")
  if(GRN_EMBED)
    add_library(stem_token_filter STATIC ${STEM_SOURCES})
    set_target_properties(stem_token_filter PROPERTIES POSITION_INDEPENDENT_CODE
                                                       ON)
  else()
    add_library(stem_token_filter MODULE ${STEM_SOURCES})
    set_target_properties(stem_token_filter PROPERTIES PREFIX "" OUTPUT_NAME
                                                                 "stem")
    install(TARGETS stem_token_filter DESTINATION "${TOKEN_FILTERS_DIR}")
    if(MSVC)
      install(
        FILES $<TARGET_PDB_FILE:stem_token_filter>
        DESTINATION "${TOKEN_FILTERS_DIR}"
        OPTIONAL)
    endif()
  endif()
  target_link_libraries(stem_token_filter libgroonga Groonga::libstemmer)
endif()
