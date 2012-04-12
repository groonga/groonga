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

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/sources.am LIBGROONGA_SOURCES)
# Remove variable declaration at the first line:
#   "libgroonga_la_SOURCES =	\" -> ""
string(REGEX REPLACE "^.*=[ \t]*\\\\" ""
  LIBGROONGA_SOURCES "${LIBGROONGA_SOURCES}")
# Remove white spaces: "	com.c	\\\n	com.h	\\\n" -> "com.c\\com.h"
string(REGEX REPLACE "[ \t\n]" "" LIBGROONGA_SOURCES "${LIBGROONGA_SOURCES}")
# Convert string to list: "com.c\\com.h" -> "com.c;com.h"
# NOTE: List in CMake is ";" separated string.
string(REGEX REPLACE "\\\\" ";" LIBGROONGA_SOURCES "${LIBGROONGA_SOURCES}")

add_library(groonga SHARED ${LIBGROONGA_SOURCES})
