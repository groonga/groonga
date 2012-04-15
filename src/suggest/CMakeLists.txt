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

include(${CMAKE_SOURCE_DIR}/build/cmake_modules/ReadFileList.cmake)

read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/create_dataset_sources.am
  GROONGA_SUGGEST_CREATE_DATASET_SOURCES)
add_executable(groonga-suggest-create-dataset
  ${GROONGA_SUGGEST_CREATE_DATASET_SOURCES})
target_link_libraries(groonga-suggest-create-dataset ${LIBGROONGA})
install(
  TARGETS groonga-suggest-create-dataset
  DESTINATION ${BIN_DIR})

if(WITH_LIBEVENT AND WITH_ZEROMQ AND WITH_MESSAGE_PACK)
  set(WITH_SUGGEST_LEARNER TRUE)
else()
  set(WITH_SUGGEST_LEARNER FALSE)
endif()

if(WITH_SUGGEST_LEARNER)
  include_directories(
    ${CMAKE_BINARY_DIR}/lib
    ${LIBEVENT_INCLUDE_DIRS}
    ${ZEROMQ_INCLUDE_DIRS}
    ${MESSAGE_PACK_INCLUDE_DIRS}
    )
  link_directories(
    ${LIBEVENT_LIBRARY_DIRS}
    ${ZEROMQ_LIBRARY_DIRS}
    ${MESSAGE_PACK_LIBRARY_DIRS}
    )

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/util_sources.am
    GROONGA_SUGGEST_UTIL_SOURCES)
  add_library(groonga-suggest-util STATIC ${GROONGA_SUGGEST_UTIL_SOURCES})

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/learner_sources.am
    GROONGA_SUGGEST_LEARNER_SOURCES)
  add_executable(groonga-suggest-learner ${GROONGA_SUGGEST_LEARNER_SOURCES})
  target_link_libraries(groonga-suggest-learner
    groonga-suggest-util
    ${LIBGROONGA}
    ${LIBEVENT_LIBRARIES}
    ${ZEROMQ_LIBRARIES}
    ${MESSAGE_PACK_LIBRARIES})

  read_file_list(${CMAKE_CURRENT_SOURCE_DIR}/httpd_sources.am
    GROONGA_SUGGEST_HTTPD_SOURCES)
  add_executable(groonga-suggest-httpd ${GROONGA_SUGGEST_HTTPD_SOURCES})
  target_link_libraries(groonga-suggest-httpd
    groonga-suggest-util
    ${LIBGROONGA}
    ${LIBEVENT_LIBRARIES}
    ${ZEROMQ_LIBRARIES}
    ${MESSAGE_PACK_LIBRARIES})

  install(
    TARGETS groonga-suggest-learner groonga-suggest-httpd
    DESTINATION ${BIN_DIR})
endif()
