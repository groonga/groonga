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

include(GroongaSphinx)

file(READ ${CMAKE_CURRENT_SOURCE_DIR}/files.am GRN_SPHINX_FILES_AM)
string(REGEX MATCH "absolute_source_files = [^=]*" GRN_SPHINX_SOURCES
             ${GRN_SPHINX_FILES_AM})
string(REGEX REPLACE "[^\n=]+=[ \t]*\\\\" "" GRN_SPHINX_SOURCES
                     ${GRN_SPHINX_SOURCES})
string(REGEX REPLACE "[ \t\n]" "" GRN_SPHINX_SOURCES ${GRN_SPHINX_SOURCES})
string(REGEX REPLACE "\\\$\\(top_srcdir\\)/doc/source/" "" GRN_SPHINX_SOURCES
                     ${GRN_SPHINX_SOURCES})
string(REGEX REPLACE "\\\\\\\$\\(NULL\\).*" "" GRN_SPHINX_SOURCES
                     ${GRN_SPHINX_SOURCES})
string(REGEX REPLACE "\\\\" ";" GRN_SPHINX_SOURCES ${GRN_SPHINX_SOURCES})

foreach(LOCALE en ja)
  grn_sphinx(${CMAKE_CURRENT_SOURCE_DIR}/source ${LOCALE}
             "${GRN_SPHINX_SOURCES}")
  # TODO: We want to run doc_html_ja/doc_html_en before "install" but
  # how to do it...?
  # install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${LOCALE}/html
  #         DESTINATION ${CMAKE_INSTALL_DOCDIR}/${LOCALE})
endforeach()
