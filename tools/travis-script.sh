#!/bin/sh

set -e

case "${BUILD_TOOL}" in
  autotools)
    test/unit/run-test.sh
    test/command/run-test.sh
    if [ "${ENABLE_MRUBY" = "yes" ]; then
      test/query_optimizer/run-test.rb
    fi
    # test/command/run-test.sh --interface http
    # test/command/run-test.sh --interface http --testee groonga-httpd
    ;;
  cmake)
    test/command/run-test.sh
    ;;
esac
