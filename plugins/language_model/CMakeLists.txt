# Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>
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

set(LANGUAGE_MODEL_DIR "${GRN_RELATIVE_PLUGINS_DIR}/language_model")

if(GRN_WITH_FAISS)
  set(KNN_SOURCES knn.cpp)
  set_source_files_properties(
    ${KNN_SOURCES} PROPERTIES COMPILE_FLAGS "${GRN_CXX_COMPILE_FLAGS}")
  if(GRN_EMBED)
    add_library(knn STATIC ${KNN_SOURCES})
    set_target_properties(knn PROPERTIES POSITION_INDEPENDENT_CODE ON)
  else()
    add_library(knn MODULE ${KNN_SOURCES})
    set_target_properties(knn PROPERTIES PREFIX "" OUTPUT_NAME "knn")
    install(TARGETS knn DESTINATION "${LANGUAGE_MODEL_DIR}")
    if(MSVC)
      install(
        FILES $<TARGET_PDB_FILE:knn
        DESTINATION "${LANGUAGE_MODEL_DIR}"
        OPTIONAL)
    endif()
  endif()
  target_link_libraries(knn libgroonga faiss)
endif()
