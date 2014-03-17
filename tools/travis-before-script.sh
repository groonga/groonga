#!/bin/sh

set -e

case "${BUILD_TOOL}" in
    autotools)
	./autogen.sh

	configure_args=""
	#if [ "$CC" = "clang" ]; then
	    configure_args="${configure_args} --enable-debug"
	#fi
	if [ "$ENABLE_MRUBY" = "yes" ]; then
	    configure_args="${configure_args} --enable-mruby"
	fi

	./configure --with-ruby19 ${configure_args}
	;;
    cmake)
	cmake_args=""
	cmake_args="${cmake_args} -DGRN_WITH_DEBUG=yes"
	if [ "$ENABLE_MRUBY" = "yes" ]; then
	    cmake_args="${cmake_args} -DGRN_WITH_MRUBY=yes"
	fi

	cmake . ${cmake_args}
	;;
esac

n_processors="$(grep '^processor' /proc/cpuinfo | wc -l)"
make -j${n_processors} > /dev/null
