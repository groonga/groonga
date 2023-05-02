# Copyright(C) 2016-2018  Brazil
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

set(LZ4_VERSION ${GRN_BUNDLED_LZ4_VERSION})

set(LZ4_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lz4-${LZ4_VERSION}")
set(LZ4_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/../lz4-${LZ4_VERSION}")

if(GRN_WITH_BUNDLED_LZ4)
  set(LIBLZ4_SOURCES
    "${LZ4_SOURCE_DIR}/lib/lz4.c"
    "${LZ4_SOURCE_DIR}/lib/lz4.h"
    "${LZ4_SOURCE_DIR}/lib/lz4frame.c"
    "${LZ4_SOURCE_DIR}/lib/lz4frame.h"
    "${LZ4_SOURCE_DIR}/lib/lz4frame_static.h"
    "${LZ4_SOURCE_DIR}/lib/lz4hc.c"
    "${LZ4_SOURCE_DIR}/lib/lz4hc.h"
    "${LZ4_SOURCE_DIR}/lib/xxhash.c"
    "${LZ4_SOURCE_DIR}/lib/xxhash.h")

  set(LZ4_C_COMPILE_FLAGS "${GRN_C_COMPILE_FLAGS}")

  set_source_files_properties(${LIBLZ4_SOURCES}
    PROPERTIES
    COMPILE_FLAGS "${LZ4_C_COMPILE_FLAGS}")

  if(GRN_BUNDLED)
    add_library(liblz4 STATIC ${LIBLZ4_SOURCES})
    set_target_properties(
      liblz4
      PROPERTIES
      POSITION_INDEPENDENT_CODE ON)
  else()
    add_library(liblz4 SHARED ${LIBLZ4_SOURCES})
    if(MSVC)
      target_compile_definitions(liblz4 PRIVATE "LZ4_DLL_EXPORT=1")
      target_compile_definitions(liblz4 INTERFACE "LZ4_DLL_IMPORT=1")
    endif()
  endif()
  target_include_directories(liblz4 BEFORE PUBLIC "${LZ4_SOURCE_DIR}/lib")
  target_compile_definitions(liblz4 PRIVATE "XXH_NAMESPACE=LZ4_")

  if(NOT MSVC)
    set_target_properties(liblz4 PROPERTIES OUTPUT_NAME "lz4")
  endif()

  if(NOT GRN_BUNDLED)
    install(TARGETS Groonga::liblz4
      ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    if(MSVC)
      install(
	FILES
	$<TARGET_PDB_FILE:Groonga::liblz4>
	DESTINATION "${CMAKE_INSTALL_BINDIR}"
	OPTIONAL)
    endif()
    install(FILES
      "${LZ4_SOURCE_DIR}/lib/lz4.h"
      "${LZ4_SOURCE_DIR}/lib/lz4frame.h"
      "${LZ4_SOURCE_DIR}/lib/lz4hc.h"
      DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
  endif()

  install(FILES
    "${LZ4_SOURCE_DIR}/lib/LICENSE"
    "${LZ4_SOURCE_DIR}/programs/COPYING"
    "${LZ4_SOURCE_DIR}/README.md"
    DESTINATION "${GRN_DATA_DIR}/lz4")
endif()
