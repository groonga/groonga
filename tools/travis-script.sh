#!/bin/bash

set -e
set -u

: ${ENABLE_MRUBY:=no}

prefix=/tmp/local

command_test_options="--reporter=mark --timeout=60"

set -x

export COLUMNS=79

retry()
{
  local i=0
  while ! "$@"; do
    if [ $i -eq 3 ]; then
      exit 1
    fi
    i=$((i + 1))
  done
}

if [ "${TRAVIS_OS_NAME}" = "osx" ]; then
  memory_fs_size=$[512 * 1024 * 1024] # 512MiB
  byte_per_sector=512
  n_sectors=$[${memory_fs_size} / ${byte_per_sector}]
  memory_fs_device_path=$(hdid -nomount ram://${n_sectors})
  newfs_hfs ${memory_fs_device_path}
  mkdir -p tmp
  mount -t hfs ${memory_fs_device_path} tmp

  export PKG_CONFIG_PATH="$(brew --prefix openssl)/lib/pkgconfig"

  command_test_options="${command_test_options} --n-workers=2"
else
  command_test_options="${command_test_options} --n-workers=4"
fi

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
    retry test/command/run-test.sh ${command_test_options} --interface http
    mkdir -p ${prefix}/var/log/groonga/httpd
    retry test/command/run-test.sh ${command_test_options} --testee groonga-httpd
    ;;
  cmake)
    test/command/run-test.sh ${command_test_options}
    ;;
esac
