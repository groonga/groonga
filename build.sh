#!/bin/bash
#
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

if [ $# -ne 3 ]; then
  echo "Usage: $0 SOURCE_DIRECTORY BUILD_DIRECTORY INSTALL_PREFIX"
  echo " e.g.: $0 . /tmp/build /tmp/local"
  exit 1
fi

set -eux

source_directory="$1"
shift
build_directory="$1"
shift
install_prefix="$1"
shift

rm -rf "${build_directory}"
cmake_args=(
  -S "${source_directory}"
  -B "${build_directory}"
  --preset debug-default
  -DCMAKE_INSTALL_PREFIX="${install_prefix}"
)
cmake "${cmake_args[@]}" "$@"
cmake --build "${build_directory}"
cmake --install "${build_directory}"
