# Copyright(C) 2012-2018  Brazil
# Copyright(C) 2023  Sutou Kouhei <kou@clear-code.com>
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

set(GRN_SUGGEST_PLUGIN_DIR "${GRN_RELATIVE_PLUGINS_DIR}/suggest")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/sources.am SUGGEST_SOURCES)
set_source_files_properties(${SUGGEST_SOURCES}
                            PROPERTIES COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS}")
if(GRN_EMBED)
  add_library(suggest STATIC ${SUGGEST_SOURCES})
  set_target_properties(suggest PROPERTIES POSITION_INDEPENDENT_CODE ON)
else()
  add_library(suggest MODULE ${SUGGEST_SOURCES})
  set_target_properties(suggest PROPERTIES PREFIX "")
  install(TARGETS suggest DESTINATION "${GRN_SUGGEST_PLUGIN_DIR}")
  if(MSVC)
    install(
      FILES $<TARGET_PDB_FILE:suggest>
      DESTINATION "${GRN_SUGGEST_PLUGIN_DIR}"
      OPTIONAL)
  endif()
endif()
target_link_libraries(suggest libgroonga)
