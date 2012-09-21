#!/bin/sh

set -e
set -x

./autogen.sh

configure_args=""
#if [ "$CC" = "clang" ]; then
    configure_args="${configure_args} --with-debug"
#fi

./configure --with-ruby19 ${configure_args}
