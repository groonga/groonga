#!/bin/sh

set -e

case "${BUILD_TOOL}" in
    autotools)
./autogen.sh

configure_args=""
#if [ "$CC" = "clang" ]; then
    configure_args="${configure_args} --enable-debug"
#fi

./configure --with-ruby19 ${configure_args}
;;
    cmake)
	cmake . -DGRN_WITH_DEBUG=yes -DGRN_WITH_MRUBY=yes
	;;
esac
