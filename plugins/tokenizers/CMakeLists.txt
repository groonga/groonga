# Copyright(C) 2012 Brazil
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

include(${CMAKE_SOURCE_DIR}/build/cmake_modules/ReadFileList.cmake)

include_directories(
  ${CMAKE_SOURCE_DIR}/lib
  )

if(WITH_MECAB)
  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mecab_sources.am MECAB_SOURCES)
  include_directories(${MECAB_INCLUDE_DIRS})
  link_directories(${MECAB_LIBRARY_DIRS})
  add_library(mecab_tokenizer MODULE ${MECAB_SOURCES})
  set_target_properties(mecab_tokenizer PROPERTIES PREFIX "")
  target_link_libraries(mecab_tokenizer ${LIBGROONGA} ${MECAB_LIBRARIES})
endif()

if(WITH_KYTEA)
  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/kytea_sources.am KYTEA_SOURCES)
  include_directories(${KYTEA_INCLUDE_DIRS})
  link_directories(${KYTEA_LIBRARY_DIRS})
  add_library(kytea_tokenizer MODULE ${KYTEA_SOURCES})
  set_target_properties(kytea_tokenizer PROPERTIES PREFIX "")
  target_link_libraries(kytea_tokenizer ${LIBGROONGA} ${KYTEA_LIBRARIES})
endif()
