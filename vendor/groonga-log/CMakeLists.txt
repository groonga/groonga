# Copyright(C) 2017 Brazil
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

set(GROONGA_LOG_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../groonga-log-source")

if(GRN_WITH_MRUBY)
  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/ruby_scripts_sources.am
    RUBY_SCRIPTS_SOURCES)
  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/ruby_scripts_groonga_log_sources.am
    RUBY_SCRIPTS_GROONGA_LOG_SOURCES)
  install(
    FILES ${RUBY_SCRIPTS_SOURCES}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}")
  install(
    FILES ${RUBY_SCRIPTS_GROONGA_LOG_SOURCES}
    DESTINATION "${GRN_RELATIVE_RUBY_SCRIPTS_DIR}/groonga-log")
  install(
    FILES
    "${GROONGA_LOG_SOURCE_DIR}/README.md"
    "${GROONGA_LOG_SOURCE_DIR}/doc/text/lgpl-2.1.txt"
    DESTINATION "${GRN_DATA_DIR}/groonga-log")
endif()
