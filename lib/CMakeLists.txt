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

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/dat
  )

include(${CMAKE_SOURCE_DIR}/build/cmake_modules/ReadFileList.cmake)

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/sources.am LIBGROONGA_SOURCES)
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/dat/sources.am LIBGRN_DAT_SOURCES)

add_library(groonga SHARED ${LIBGROONGA_SOURCES} ${LIBGRNDAT_SOURCES})
