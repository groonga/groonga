#!/bin/sh

set -e
set -u

: ${DOCKER:=}
: ${TARGET:=}

touch lib/grn_ecmascript.c

if [ -n "${DOCKER}" ]; then
  ./version-gen.sh
  ./autogen.sh
  docker build \
         -t groonga/groonga-${DOCKER} \
         -f travis/Dockerfile.${DOCKER} \
         .
  exit $?
fi

if [ -n "${TARGET}" ]; then
  BUILD_TOOL=autotools
  ENABLE_MRUBY=yes
  ENABLE_DOCUMENT=yes
  ENABLE_MAINTAINER_MODE=yes
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

if [ -n "${TARGET}" ]; then
  make dist > /dev/null
else
  make -j${n_processors}
fi
