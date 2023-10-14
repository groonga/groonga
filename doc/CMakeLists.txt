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
set(GRN_DOC_LOCALES en ja)
foreach(LOCALE ${GRN_DOC_LOCALES})
  set(GRN_DOC_HTML_FILES_LOCALE)
  foreach(file ${GRN_DOC_HTML_FILES})
    list(APPEND GRN_DOC_HTML_FILES_LOCALE
         ${CMAKE_CURRENT_BINARY_DIR}/${LOCALE}/html/${file})
  endforeach()
  if(EXISTS "${GRN_DOC_PRE_BUILD_STAMP}")
    install(
      DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/locale/${LOCALE}/html/
      COMPONENT Document
      DESTINATION ${CMAKE_INSTALL_DOCDIR}/${LOCALE}
      PATTERN ".buildinfo" EXCLUDE)
  else()
    grn_sphinx(${CMAKE_CURRENT_SOURCE_DIR}/source ${LOCALE}
               "${GRN_DOC_SOURCES}" "${GRN_DOC_HTML_FILES_LOCALE}")
    install(
      DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${LOCALE}/html/
      COMPONENT Document
      DESTINATION ${CMAKE_INSTALL_DOCDIR}/${LOCALE}
      PATTERN ".buildinfo" EXCLUDE)
  endif()
endforeach()

add_custom_target(
  doc_update_files
  COMMAND ${RUBY} ${CMAKE_CURRENT_SOURCE_DIR}/update-files.rb
  DEPENDS doc_en_html)

add_custom_target(
  doc_update_examples
  COMMAND
    ${CMAKE_COMMAND} -E env
    GRN_PLUGINS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../plugins
    GRN_PLUGINS_PATH=${CMAKE_CURRENT_BINARY_DIR}/../plugins
    GRN_RUBY_SCRIPTS_DIR=${CMAKE_CURRENT_SOURCE_DIR}/../lib/mrb/scripts
    LD_LIBRARY_PATH=${CMAKE_CURRENT_BINARY_DIR}/../lib:$ENV{LD_LIBRARY_PATH}
    PATH=${CMAKE_CURRENT_BINARY_DIR}/../src:${CMAKE_CURRENT_BINARY_DIR}/../src/suggest:$ENV{PATH}
    ${RUBY} ${CMAKE_CURRENT_SOURCE_DIR}/update-execution-example.rb
  DEPENDS groonga groonga-suggest-create-dataset groonga-suggest-httpd
          groonga-suggest-learner
  VERBATIM)

set(GRN_DOC_DEPLOY_DEPENDS)
foreach(LOCALE ${GRN_DOC_LOCALES})
  list(APPEND GRN_DOC_DEPLOY_DEPENDS doc_${LOCALE}_html)
endforeach()
add_custom_target(
  doc_deploy
  COMMAND ${RUBY} ${CMAKE_CURRENT_SOURCE_DIR}/deploy.rb
  DEPENDS ${GRN_DOC_DEPLOY_DEPENDS}
  VERBATIM)
