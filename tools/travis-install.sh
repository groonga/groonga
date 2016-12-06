#!/bin/sh

set -e
set -u

: ${ENABLE_MRUBY:=no}

case "${TRAVIS_OS_NAME}" in
  linux)
    curl --silent --location https://raw.github.com/clear-code/cutter/master/data/travis/setup.sh | sh
    sudo apt-get install -qq -y \
         autotools-dev \
         zlib1g-dev \
         libmsgpack-dev \
         libevent-dev \
         libmecab-dev \
         mecab-naist-jdic \
         cmake
    ;;
  osx)
    brew update > /dev/null
    brew outdated pkg-config || brew upgrade pkg-config
    brew reinstall libtool
    brew install \
         msgpack \
         libevent \
         mecab \
         mecab-ipadic \
         pcre \
         cutter
    pkg-config --modversion libssl
    pkg-config --cflags --libs libssl
    ;;
esac

if [ "${ENABLE_MRUBY}" = "yes" ]; then
  gem install pkg-config groonga-client
fi
