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

function(grn_sphinx SOURCE_DIR LOCALE SOURCES HTML_FILES)
  set(ABSOLUT_SOURCES)
  set(ALL_SOURCES)
  foreach(SOURCE ${SOURCES} conf.py)
    list(APPEND ABSOLUTE_SOURCES ${SOURCE_DIR}/${SOURCE})
    list(APPEND ALL_ABSOLUTE_SOURCES ${SOURCE_DIR}/${SOURCE})
    if("${SOURCE}" STREQUAL "conf.py")
      # target
    elseif(
      "${SOURCE}" STREQUAL "install/server-use.rst"
      OR "${SOURCE}" STREQUAL "install/server-use.md"
      OR "${SOURCE}" STREQUAL "reference/commands/compress_filter.rst"
      OR "${SOURCE}" STREQUAL "reference/scoring_note.rst")
      # not target
      continue()
    elseif(NOT "${SOURCE}" MATCHES "\\.(rst|md)\$")
      continue()
    endif()
    string(REGEX REPLACE "\\.[^.]+\$" "" SOURCE_BASE_NAME ${SOURCE})
    set(POT "${LOCALE}/gettext/${SOURCE_BASE_NAME}.pot")
    set(PO
        "${SOURCE_DIR}/../locale/${LOCALE}/LC_MESSAGES/${SOURCE_BASE_NAME}.po")
    set(PO_TIME_STAMP "${LOCALE}/LC_MESSAGES/${SOURCE_BASE_NAME}.po.time_stamp")
    set(EDIT_PO "${LOCALE}/LC_MESSAGES/${SOURCE_BASE_NAME}.edit.po")
    add_custom_command(
      OUTPUT ${EDIT_PO}
      COMMAND ${RUBY} ${CMAKE_CURRENT_SOURCE_DIR}/../doc/update-edit-po.rb
              ${LOCALE} ${POT} ${PO} ${PO_TIME_STAMP} ${EDIT_PO}
      DEPENDS ${LOCALE}-gettext.time_stamp ${LOCALE}/gettext/conf.pot)
    add_custom_command(
      OUTPUT ${PO} ${PO_TIME_STAMP}
      COMMAND ${RUBY} ${CMAKE_CURRENT_SOURCE_DIR}/../doc/update-po.rb ${PO}
              ${PO_TIME_STAMP} ${EDIT_PO}
      DEPENDS ${EDIT_PO})
    list(APPEND ALL_ABSOLUTE_SOURCES ${PO})
  endforeach()

  find_program(XGETTEXT xgettext REQUIRED)
  add_custom_command(
    OUTPUT ${LOCALE}/gettext/conf.pot
    COMMAND ${XGETTEXT} --language Python --output ${LOCALE}/gettext/conf.pot
            ${SOURCE_DIR}/conf.py
    DEPENDS ${SOURCE_DIR}/conf.py)

  find_program(SPHINX_BUILD sphinx-build REQUIRED)
  foreach(BUILDER "gettext" "html" "markdown")
    if("${BUILDER}" STREQUAL "gettext")
      set(TARGET_SOURCES ${ABSOLUTE_SOURCES})
    else()
      set(TARGET_SOURCES ${ALL_ABSOLUTE_SOURCES})
    endif()
    set(OUTPUTS ${LOCALE}/doctrees/${BUILDER} ${LOCALE}-${BUILDER}.time_stamp)
    if("${BUILDER}" STREQUAL "html")
      list(APPEND OUTPUTS ${HTML_FILES})
    endif()
    add_custom_command(
      OUTPUT ${OUTPUTS}
      COMMAND
        ${CMAKE_COMMAND} -E env DOCUMENT_VERSION=${GRN_VERSION}
        DOCUMENT_VERSION_FULL=${GRN_VERSION_FULL} LOCALE=${LOCALE}
        ${SPHINX_BUILD} -j auto -D language=${LOCALE} -b ${BUILDER} -d
        ${LOCALE}/doctrees/${BUILDER} ${SOURCE_DIR} ${LOCALE}/${BUILDER}
      COMMAND ${CMAKE_COMMAND} -E touch ${LOCALE}-${BUILDER}.time_stamp
      DEPENDS ${TARGET_SOURCES})
    if("${BUILDER}" STREQUAL "html")
      add_custom_target(doc_${LOCALE}_${BUILDER} ALL DEPENDS ${OUTPUTS})
    else()
      add_custom_target(doc_${LOCALE}_${BUILDER} DEPENDS ${OUTPUTS})
    endif()
  endforeach()
endfunction()
