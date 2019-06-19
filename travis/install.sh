#!/bin/sh

set -e
set -u

: ${DOCKER:=}
: ${TARGET:=}

git submodule update --init --depth 1

if [ -n "${DOCKER}" ]; then
  curl \
    --silent \
    --location \
    https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh
  exit $?
fi

if [ -n "${TARGET}" ]; then
  ENABLE_MRUBY=yes
  ENABLE_DOCUMENT=yes
fi

: ${ENABLE_MRUBY:=no}
: ${ENABLE_DOCUMENT:=no}

case "${TRAVIS_OS_NAME}" in
  linux)
    curl \
      --silent \
      --location \
      https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh
    if [ "${ENABLE_DOCUMENT}" = "yes" ]; then
      sudo apt-get install -qq -y \
           python3-pip
      sudo pip3 install Sphinx
    fi
    ;;
  osx)
    sudo \
      installer \
      -pkg /Library/Developer/CommandLineTools/Packages/macOS_SDK_headers_for_macOS_10.14.pkg \
      -target /
    if [ "${ENABLE_DOCUMENT}" = "yes" ]; then
      pip3 install Sphinx
    fi
    ;;
esac

if [ "${ENABLE_MRUBY}" = "yes" ]; then
  gem install pkg-config groonga-client test-unit
fi
