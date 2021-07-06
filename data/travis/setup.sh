#!/bin/sh
#
# Copyright (C) 2013-2020  Kouhei Sutou <kou@clear-code.com>
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

# set -x
set -e

if [ "$GROONGA_MASTER" = "yes" ]; then
  sudo apt-get update -qq
  sudo apt install -qq -y -V \
       lsb-release \
       wget
  wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-archive-keyring-latest-$(lsb_release --codename --short).deb
  sudo apt install -qq -y -V ./apache-arrow-archive-keyring-latest-$(lsb_release --codename --short).deb
  sudo apt-get update -qq
  sudo apt-get install -qq -y -V \
       autoconf-archive \
       autotools-dev \
       libarrow-dev \
       libevent-dev \
       libmecab-dev \
       libmsgpack-dev \
       libstemmer-dev \
       mecab-naist-jdic \
       pkg-config
  git clone --recursive --depth 1 --branch master https://github.com/groonga/groonga.git
  cd groonga
  touch lib/grn_ecmascript.c
  ./autogen.sh
  ./configure --prefix=/usr --localstatedir=/var --enable-debug
  make -j$(nproc) > /dev/null
  sudo make install > /dev/null
  cd ..
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
