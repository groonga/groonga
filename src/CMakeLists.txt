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

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/groonga_sources.am GROONGA_SOURCES)
add_executable(groonga ${GROONGA_SOURCES})
target_link_libraries(groonga ${LIBGROONGA})

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/grnslap_sources.am GRNSLAP_SOURCES)
add_executable(grnslap ${GRNSLAP_SOURCES})
target_link_libraries(grnslap ${LIBGROONGA})

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/groonga_benchmark_sources.am
  GROONGA_BENCHMARK_SOURCES)
add_executable(groonga-benchmark ${GROONGA_BENCHMARK_SOURCES})
target_link_libraries(groonga-benchmark ${LIBGROONGA})
