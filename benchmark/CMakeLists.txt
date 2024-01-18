# Copyright (C) 2024  Sutou Kouhei <kou@clear-code.com>
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

pkg_check_modules(gobject gobject-2.0 REQUIRED IMPORTED_TARGET)
add_library(grnbench
  SHARED
  lib/benchmark.c
  lib/benchmark.h
  lib/bench-reporter.c
  lib/bench-reporter.h
  lib/bench-utils.c
  lib/bench-utils.h)
target_link_libraries(grnbench PkgConfig::gobject)

add_executable(bench-between-sequential bench-between-sequential.c)
target_link_libraries(bench-between-sequential libgroonga grnbench)

add_executable(bench-cache bench-cache.c)
target_link_libraries(bench-cache libgroonga grnbench)

add_executable(bench-ctx-create bench-ctx-create.c)
target_link_libraries(bench-ctx-create libgroonga grnbench)

add_executable(bench-geo-distance bench-geo-distance.c)
target_link_libraries(bench-geo-distance libgroonga grnbench)

add_executable(bench-geo-select bench-geo-select.c)
target_link_libraries(bench-geo-select libgroonga grnbench)

add_executable(bench-nfkc bench-nfkc.c)
target_link_libraries(bench-nfkc libgroonga grnbench)

add_executable(bench-query-optimizer bench-query-optimizer.c)
target_link_libraries(bench-query-optimizer libgroonga grnbench)

add_executable(bench-range-select bench-range-select.c)
target_link_libraries(bench-range-select libgroonga grnbench)

add_executable(bench-result-set-raw bench-result-set-raw.c)
target_link_libraries(bench-result-set-raw libgroonga grnbench)

add_executable(bench-result-set bench-result-set.c)
target_link_libraries(bench-result-set libgroonga grnbench)

add_executable(bench-table-factory bench-table-factory.c)
target_link_libraries(bench-table-factory libgroonga grnbench)

