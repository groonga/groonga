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
include(${CMAKE_CURRENT_SOURCE_DIR}/files.cmake)

set(GRN_DOC_PRE_BUILD_STAMP "${CMAKE_CURRENT_SOURCE_DIR}/build-stamp")
foreach(LOCALE en ja)
  set(GRN_DOC_HTML_FILES_LOCALE)
  foreach(file ${GRN_DOC_HTML_FILES})
    list(APPEND GRN_DOC_HTML_FILES_LOCALE
         ${CMAKE_CURRENT_BINARY_DIR}/${LOCALE}/html/${file})
  endforeach()
  if(EXISTS "${GRN_DOC_PRE_BUILD_STAMP}")
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${LOCALE}/html/
            DESTINATION ${CMAKE_INSTALL_DOCDIR}/${LOCALE})
  else()
    grn_sphinx(${CMAKE_CURRENT_SOURCE_DIR}/source ${LOCALE}
               "${GRN_DOC_SOURCES}" "${GRN_DOC_HTML_FILES_LOCALE}")
    install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${LOCALE}/html/
            DESTINATION ${CMAKE_INSTALL_DOCDIR}/${LOCALE})
  endif()
endforeach()

add_custom_target(
  doc_update_files
  COMMAND ${RUBY} ${CMAKE_CURRENT_SOURCE_DIR}/update-files.rb
  DEPENDS doc_en_html)
