#!/bin/sh

set -e

prefix=/tmp/local

command_test_options="--n-workers=4 --reporter=mark"

set -x

export COLUMNS=79

case "${BUILD_TOOL}" in
  autotools)
    # TODO: Re-enable me on OS X
    if [ "${TRAVIS_OS_NAME}" = "linux" ]; then
      test/unit/run-test.sh
    fi
    test/command/run-test.sh ${command_test_options}
    # TODO: Re-enable me on OS X
    if [ "${TRAVIS_OS_NAME}" = "linux" -a "${ENABLE_MRUBY}" = "yes" ]; then
      test/mruby/run-test.rb
      test/command_line/run-test.rb
    fi
    test/command/run-test.sh ${command_test_options} --interface http
    mkdir -p ${prefix}/var/log/groonga/httpd
    test/command/run-test.sh ${command_test_options} --testee groonga-httpd
    ;;
  cmake)
    test/command/run-test.sh ${command_test_options}
    ;;
esac
