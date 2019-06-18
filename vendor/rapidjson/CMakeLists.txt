# Copyright(C) 2019 Sutou Kouhei <kou@clear-code.com>
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

set(RAPIDJSON_VERSION ${GRN_BUNDLED_RAPIDJSON_VERSION})

set(RAPIDJSON_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../rapidjson-${RAPIDJSON_VERSION}")
set(RAPIDJSON_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/../rapidjson-${RAPIDJSON_VERSION}")

if(GRN_WITH_BUNDLED_RAPIDJSON)
  install(FILES
    "${RAPIDJSON_SOURCE_DIR}/license.txt"
    "${RAPIDJSON_SOURCE_DIR}/readme.md"
    DESTINATION "${GRN_DATA_DIR}/rapidjson")
endif()
