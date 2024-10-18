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

function setup_with_apt () {
  if [ -f /etc/debian_version ]; then
    ${SUDO} apt update
    ${SUDO} apt install -y -V lsb-release wget

    wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
    ${SUDO} apt install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
    rm apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
    ${SUDO} apt update
  fi

  if type lsb_release > /dev/null 2>&1; then
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
        gettext
        libarrow-dev
        libedit-dev
        liblz4-dev
        libmecab-dev
        libmsgpack-dev
        libsimdjson-dev
        libstemmer-dev
        libxxhash-dev
        libzstd-dev
        ninja-build
        pkg-config
        python3-pip
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
}

function setup_with_dnf () {
  os=$(cut -d: -f4 /etc/system-release-cpe)
  case ${os} in
    amazon)
      os=amazon-linux
      version=$(cut -d: -f6 /etc/system-release-cpe)
      ;;
    *) # For AlmaLinux
      version=$(cut -d: -f5 /etc/system-release-cpe | sed -e 's/\.[0-9]$//')
      ;;
  esac

  package_names=(
    arrow-devel
    cmake
    intltool
    libedit-devel
    libevent-devel
    libstemmer-devel
    libzstd-devel
    lz4-devel
    ninja-build
    openssl-devel
    pkgconfig
    ruby
    tar
    wget
    xxhash-devel
    zlib-devel
  )
  case "${os}" in
    almalinux)
      ${SUDO} dnf install -y epel-release "dnf-command(config-manager)"
      case "${version}" in
        8)
          ${SUDO} dnf config-manager --set-enabled powertools
          ;;
        9)
          ${SUDO} dnf config-manager --set-enabled crb
          ;;
      esac

      package_names+=(
        ccache
        mecab-devel
        msgpack-devel
        simdjson-devel
      )
      ;;
    amazon-linux)
      ;;
    *)
      echo "This OS setup is not supported."
      exit 1
      ;;
  esac

  ${SUDO} dnf update -y
  ${SUDO} dnf install -y \
    https://apache.jfrog.io/artifactory/arrow/${os}/${version}/apache-arrow-release-latest.rpm
  ${SUDO} dnf groupinstall -y "Development Tools"
  ${SUDO} dnf install -y "${package_names[@]}"
}

if [ -f /etc/debian_version ]; then
  setup_with_apt
elif type dnf > /dev/null 2>&1; then
  setup_with_dnf
elif type brew > /dev/null 2>&1; then
  brew bundle --file="$(dirname "${0}")/Brewfile"
else
  echo "This OS setup is not supported."
  exit 1
fi
