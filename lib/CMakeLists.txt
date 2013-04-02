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

add_definitions(
  -DGRN_DAT_EXPORT
  )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/dat
  )

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/sources.am LIBGROONGA_SOURCES)
read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/dat/sources.am LIBGRNDAT_SOURCES)
string(REGEX REPLACE "([^;]+)" "dat/\\1"
  LIBGRNDAT_SOURCES "${LIBGRNDAT_SOURCES}")

add_library(libgroonga SHARED ${LIBGROONGA_SOURCES} ${LIBGRNDAT_SOURCES})
set_target_properties(libgroonga PROPERTIES OUTPUT_NAME "groonga")
if(WITH_STATIC)
  add_library(libgroonga_static STATIC ${LIBGROONGA_SOURCES} ${LIBGRNDAT_SOURCES})
  set_target_properties(libgroonga_static PROPERTIES OUTPUT_NAME "groonga")
endif()
target_link_libraries(libgroonga
  ${PTHREAD_LIBS}
  ${Z_LIBS}
  ${LZO2_LIBS}
  ${DL_LIBS}
  ${WS2_32_LIBS})

install(TARGETS libgroonga
  ARCHIVE DESTINATION "${LIB_DIR}"
  LIBRARY DESTINATION "${LIB_DIR}"
  RUNTIME DESTINATION "${BIN_DIR}")
if(WITH_STATIC)
  install(TARGETS
    libgroonga_static
    ARCHIVE DESTINATION "${LIB_DIR}"
    LIBRARY DESTINATION "${LIB_DIR}"
    RUNTIME DESTINATION "${BIN_DIR}")
endif()
