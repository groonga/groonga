#!/bin/sh

set -e
set -u

: ${DOCKER:=}

touch lib/grn_ecmascript.c

if [ -n "${DOCKER}" ]; then
  ./version-gen.sh
  ./autogen.sh
  docker build \
         -t groonga/groonga-${DOCKER} \
         -f dockerfiles/${DOCKER}.dockerfile \
         .
  exit $?
fi

: ${ENABLE_MRUBY:=no}
: ${ENABLE_DOCUMENT:=no}
: ${ENABLE_MAINTAINER_MODE:=no}

prefix=/tmp/local

set -x

case "${BUILD_TOOL}" in
  autotools)
    ./autogen.sh

    configure_args=""
    configure_args="${configure_args} --with-ruby=$(which ruby)"
    if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
      pkg_config_path="$(brew --prefix openssl)/lib/pkgconfig"
      configure_args="${configure_args} PKG_CONFIG_PATH=${pkg_config_path}"
    fi
    #if [ "$CC" = "clang" ]; then
      configure_args="${configure_args} --enable-debug"
    #fi
    if [ "${ENABLE_MRUBY}" = "yes" ]; then
      configure_args="${configure_args} --enable-mruby"
    fi
    if [ "${ENABLE_DOCUMENT}" = "yes" ]; then
      configure_args="${configure_args} --enable-document"
    fi
    if [ "${ENABLE_MAINTAINER_MODE}" = "yes" ]; then
      configure_args="${configure_args} --enable-maintainer-mode"
    fi

    (cd vendor/onigmo-source && autoreconf --force --install)
    ./configure --prefix=${prefix} ${configure_args}
    ;;
  cmake)
    if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
      echo 'export PATH="/usr/local/opt/bison/bin:$PATH"' >> ~/.bash_profile
      source ~/.bash_profile
    fi
    cmake_args=""
    cmake_args="${cmake_args} -DRUBY=$(which ruby)"
    cmake_args="${cmake_args} -DGRN_WITH_DEBUG=yes"
    if [ "${ENABLE_MRUBY}" = "yes" ]; then
      cmake_args="${cmake_args} -DGRN_WITH_MRUBY=yes"
    fi

    cmake . ${cmake_args}
    ;;
esac

case "$(uname)" in
  Linux)
    n_processors="$(nproc)"
    ;;
  Darwin)
    n_processors="$(/usr/sbin/sysctl -n hw.ncpu)"
    ;;
  *)
    n_processors="1"
    ;;
esac

make -j${n_processors}
