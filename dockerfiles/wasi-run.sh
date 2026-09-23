#!/usr/bin/env bash
#
# Copyright (C) 2026  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

set -eu

: ${PREFIX:=/tmp/local}

echo "::group::Build"
set -x
cmake \
  -S /source \
  -B build \
  -DCMAKE_INSTALL_PREFIX=${PREFIX} \
  --preset=release-wasi
cmake --build build
cmake --install build
set +x
echo "::endgroup::"

echo "::group::Test"
set -x
export TZ=Asia/Tokyo
cp -R /source/test/command test
# TODO: Run all tests. A test that needs a normalizer, a tokenizer or
# a function in plugins/ fails. WASI doesn't support dynamic linking.
# We can't link all plugins statically yet because all plugins define
# the same GRN_PLUGIN_INIT()/GRN_PLUGIN_REGISTER()/GRN_PLUGIN_FIN().
grntest \
  --base-directory=test \
  --groonga=${PREFIX}/bin/groonga.mjs \
  --n-retries=2 \
  --n-workers=4 \
  --read-timeout=30 \
  --reporter=mark \
  test/suite/cache_limit \
  test/suite/check \
  test/suite/column_create_similar \
  test/suite/column_remove \
  test/suite/column_rename \
  test/suite/command_list \
  test/suite/config_delete \
  test/suite/config_get \
  test/suite/config_set \
  test/suite/defrag \
  test/suite/extractors \
  test/suite/geo \
  test/suite/language_model \
  test/suite/log_level \
  test/suite/log_put \
  test/suite/normalize \
  test/suite/normalizer_list \
  test/suite/object_exist \
  test/suite/object_inspect \
  test/suite/object_list \
  test/suite/object_set_visibility \
  test/suite/query_log_flags_add \
  test/suite/query_log_flags_get \
  test/suite/query_log_flags_remove \
  test/suite/query_log_flags_set \
  test/suite/range_filter \
  test/suite/request_cancel \
  test/suite/response \
  test/suite/ruby \
  test/suite/schema \
  test/suite/sharding \
  test/suite/sleep \
  test/suite/suggest \
  test/suite/table_copy \
  test/suite/tables \
  test/suite/tokenize \
  test/suite/tokenizer_list \
  test/suite/wal_recover \
  test/suite/select/filter/logical_operation
set +x
echo "::endgroup::"
