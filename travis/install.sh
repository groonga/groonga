#!/bin/sh

set -e
set -u

: ${DOCKER:=}

git submodule update --init --recursive

if [ -n "${DOCKER}" ]; then
  curl \
    --silent \
    --location \
    https://raw.github.com/clear-code/cutter/HEAD/data/travis/setup.sh | sh
  exit $?
fi

: ${ENABLE_MRUBY:=no}
: ${ENABLE_DOCUMENT:=no}

case "${TRAVIS_OS_NAME}" in
  linux)
    curl \
      --silent \
      --location \
      https://raw.github.com/clear-code/cutter/HEAD/data/travis/setup.sh | sh
    if [ "${ENABLE_DOCUMENT}" = "yes" ]; then
      sudo apt install -qq -y \
           python3-pip
      sudo pip3 install setuptools
      sudo pip3 install Sphinx
    fi
    sudo wget -O /usr/share/keyrings/apache-arrow-keyring.gpg \
         https://apache.bintray.com/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-keyring.gpg
    sudo tee /etc/apt/sources.list.d/apache-arrow.list <<APT_LINE
deb [arch=amd64 signed-by=/usr/share/keyrings/apache-arrow-keyring.gpg] https://dl.bintray.com/apache/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/ $(lsb_release --codename --short) main
deb-src [signed-by=/usr/share/keyrings/apache-arrow-keyring.gpg] https://dl.bintray.com/apache/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/ $(lsb_release --codename --short) main
APT_LINE
    sudo apt update -qq
    sudo apt install -qq -y \
         libarrow-dev
    ;;
  osx)
    if [ "${ENABLE_DOCUMENT}" = "yes" ]; then
      pip3 install Sphinx
    fi
    ;;
esac

if [ "${ENABLE_MRUBY}" = "yes" ]; then
  gem install pkg-config groonga-client test-unit
fi
