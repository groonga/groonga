# Copyright(C) 2013 Brazil
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
  ${CMAKE_SOURCE_DIR}/lib
  )

set(NORMALIZERS_DIR "${GRN_PLUGINS_DIR}/normalizers")
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/mysql_sources.am MYSQL_SOURCES)
add_library(mysql_normalizer MODULE ${MYSQL_SOURCES})
set_target_properties(mysql_normalizer PROPERTIES
  PREFIX ""
  OUTPUT_NAME "mysql")
target_link_libraries(mysql_normalizer libgroonga)
install(TARGETS mysql_normalizer DESTINATION "${NORMALIZERS_DIR}")
