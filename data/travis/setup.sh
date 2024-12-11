#!/bin/sh
#
# Copyright (C) 2013-2024  Sutou Kouhei <kou@clear-code.com>
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

# set -x
set -e

if [ "$GROONGA_MAIN" = "yes" ]; then
  sudo apt-get update -qq
  sudo apt install -qq -y -V \
       lsb-release \
       wget
  wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
  sudo apt install -qq -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
  sudo apt-get update -qq
  sudo apt-get install -qq -y -V \
       ccache \
       cmake \
       libarrow-dev \
       libevent-dev \
       libmecab-dev \
       libmsgpack-dev \
       libstemmer-dev \
       mecab-naist-jdic \
       ninja-build \
       pkg-config
  git clone --recursive --depth 1 --branch main https://github.com/groonga/groonga.git
  touch groonga/lib/grn_ecmascript.c
  cmake \
    -S groonga \
    -B groonga.build \
    --preset debug-maximum \
    -DCMAKE_INSTALL_LOCALSTATEDIR=/var \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DCMAKE_INSTALL_SYSCONFDIR=/etc
  ninja -C groonga.build > /dev/null
  sudo ninja -C groonga.build install > /dev/null
else
  if dpkg -l libzmq3 > /dev/null 2>&1; then
    sudo apt-get purge libzmq3
  fi

  distribution=$(lsb_release --short --id | tr 'A-Z' 'a-z')
  case $distribution in
    debian)
      code_name=$(lsb_release --short --codename)
      component=main
      apt_url_base=https://packages.groonga.org
      cat <<EOF | sudo tee /etc/apt/sources.list.d/groonga.list
deb ${apt_url_base}/${distribution}/ ${code_name} ${component}
deb-src ${apt_url_base}/${distribution}/ ${code_name} ${component}
EOF
      sudo apt-get update -qq
      sudo apt-get install -qq -y --allow-unauthenticated groonga-keyring
      ;;
    ubuntu)
      sudo apt-get install -qq -y -V software-properties-common
      sudo add-apt-repository -y ppa:groonga/ppa
      ;;
  esac

  sudo apt-get update -qq
  sudo apt-get install -qq -y -V groonga libgroonga-dev
fi
