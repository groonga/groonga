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

cmake_minimum_required(VERSION 2.6)
project(groonga)

file(READ base_version GRN_VERSION)

include(CheckIncludeFile)

add_subdirectory(lib)

add_definitions(
  -DHAVE_CONFIG_H
  -DGROONGA_VERSION=\"${GROONGA_VERSION}\"
  )
add_definitions(-DGROONGA_VERSION=\"${GROONGA_VERSION}\")

macro(ac_check_headers header)
  string(REGEX REPLACE "[/.]" "_" output_variable_name ${header})
  string(TOUPPER "${output_variable_name}" output_variable_name)
  set(output_variable_name "HAVE_${output_variable_name}")
  check_include_file(${header} ${output_variable_name})
endmacro()

include(build/ac-macros/check-headers.m4)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
