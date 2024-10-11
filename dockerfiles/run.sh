#!/usr/bin/env bash
#
# Copyright (C) 2020-2024  Sutou Kouhei <kou@clear-code.com>
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

set -eu

echo "::group::Build"
set -x
cmake \
  -S /source \
  -B build \
  -DCMAKE_INSTALL_PREFIX=/tmp/local \
  -DGRN_WITH_APACHE_ARROW=OFF \
  --preset=debug-maximum
cmake --build build
cmake --install build
set +x
echo "::endgroup"

echo "::group::Test"
set -x
export LD_LIBRARY_PATH=/tmp/local/lib
export PATH=/tmp/local/bin:${PATH}
export TZ=Asia/Tokyo
cp -a /source/test/command test
grntest \
  --base-directory=test \
  --interface=http \
  --n-retries=2 \
  --read-timeout=30 \
  --reporter=mark \
  test/suite
set +x
echo "::endgroup"
