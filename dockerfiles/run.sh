#!/bin/bash

set -eux

/source/configure \
  --prefix=/tmp/local \
  --enable-debug \
  --with-ruby \
  --enable-mruby

make -j$(nproc) > /dev/null

mkdir -p /tmp/local/var/log/groonga/httpd/

rsync -a --include "*.rb" --include "*/" --exclude "*" \
  /source/plugins/ \
  plugins/

mkdir -p test/command
rsync -a --delete \
  /source/test/command/suite/ \
  test/command/suite/

# BUILD_DIR=test/unit \
#   /source/test/unit/run-test.sh

BUILD_DIR=test/mruby \
  /source/test/mruby/run-test.rb

BUILD_DIR=test/command_line \
  /source/test/command_line/run-test.rb

if [ $(ruby -e 'print(RUBY_VERSION >= "2.3")') = "false" ]; then
  # Disable test/command/ test suites for now because Red Arrow 0.16.0
  # doesn't support Ruby 2.3.
  exit 0
fi

BUILD_DIR=test/command \
  /source/test/command/run-test.sh \
    --reporter mark \
    --read-timeout 30 \
    test/command/suite

BUILD_DIR=test/command \
  /source/test/command/run-test.sh \
    --reporter mark \
    --read-timeout 30 \
    --interface http \
    test/command/suite

BUILD_DIR=test/command \
  /source/test/command/run-test.sh \
    --reporter mark \
    --read-timeout 30 \
    --testee groonga-httpd \
    test/command/suite
