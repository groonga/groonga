#!/bin/sh

set -e

./autogen.sh

configure_args=""
#if [ "$CC" = "clang" ]; then
    configure_args="${configure_args} --enable-debug"
#fi

./configure --with-ruby19 ${configure_args}
