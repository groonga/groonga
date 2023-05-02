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

find_path(libstemmer_header libstemmer.h)
if(libstemmer_header)
  get_filename_component(libstemmer_include_dir "${libstemmer_header}" DIRECTORY)
endif()
find_library(libstemmer_library stemmer)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Groongalibstemmer
  REQUIRED_VARS libstemmer_include_dir libstemmer_library)

if(Groongalibstemmer_FOUND AND NOT TARGET Groonga::libstemmer)
  add_library(Groonga::libstemmer UNKNOWN IMPORTED)
  target_include_directories(Groonga::libstemmer INTERFACE
    ${libstemmer_include_dir})
  set_target_properties(Groonga::libstemmer PROPERTIES
    IMPORTED_LOCATION ${libstemmer_library})
endif()
