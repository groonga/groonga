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

set -eux

if type sudo > /dev/null 2>&1; then
  SUDO=sudo
else
  SUDO=
fi

if [ -f /etc/debian_version ]; then
  ${SUDO} apt update
  ${SUDO} apt install -y -V lsb-release wget

  wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')-rc/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
  ${SUDO} apt install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
  ${SUDO} apt update
fi

if type lsb_release  > /dev/null 2>&1; then
  distribution=$(lsb_release --id --short | tr 'A-Z' 'a-z')
  code_name=$(lsb_release --codename --short)
else
  distribution=unknown
  code_name=unknown
fi

package_names=()
case "${distribution}-${code_name}" in
  debian-*|ubuntu-*)
    package_names+=(
      ccache
      cmake
      g++
      gcc
      libarrow-dev
      liblz4-dev
      libmecab-dev
      libmsgpack-dev
      libstemmer-dev
      libxxhash-dev
      libzstd-dev
      ninja-build
      pkg-config
      rapidjson-dev
      zlib1g-dev
    )
    ;;
esac

case "${distribution}-${code_name}" in
  debian-*|ubuntu-*)
    ${SUDO} apt install -y -V "${package_names[@]}"
    ;;
esac

if [ -f "apache-arrow-apt-source-latest-${code_name}.deb" ]; then
  rm "apache-arrow-apt-source-latest-${code_name}.deb"
fi
