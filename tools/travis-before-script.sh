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
	if [ -n "${JA_SEGREGATE_THRESHOLD}" ]; then
	    configure_args="${configure_args} --with-ja-segregate-threshold=${JA_SEGREGATE_THRESHOLD}"
	fi

	./configure --with-ruby19 ${configure_args}
	;;
    cmake)
	cmake_args=""
	cmake_args="${cmake_args} -DGRN_WITH_DEBUG=yes"
	if [ "$ENABLE_MRUBY" = "yes" ]; then
	    cmake_args="${cmake_args} -DGRN_WITH_MRUBY=yes"
	fi
	if [ -n "${JA_SEGREGATE_THRESHOLD}" ]; then
	    cmake_args="${cmake_args} -DGRN_JA_SEGREGATE_THRESHOLD=${JA_SEGREGATE_THRESHOLD}"
	fi

	cmake . ${cmake_args}
	;;
esac

n_processors="$(grep '^processor' /proc/cpuinfo | wc -l)"
make -j${n_processors} > /dev/null
