#!/bin/bash

export SOURCE_DIR="`dirname $0`"
if test -z "$BUILD_DIR"; then
  BUILD_DIR="$SOURCE_DIR"
fi
export BUILD_DIR

source_top_dir="$SOURCE_DIR/../.."
source_top_dir=$(cd -P "$source_top_dir" 2>/dev/null || cd "$source_top_dir"; pwd)

build_top_dir="$BUILD_DIR/../.."
build_top_dir=$(cd -P "$build_top_dir" 2>/dev/null || cd "$build_top_dir"; pwd)

. "${build_top_dir}/config.sh"

case $(uname) in
  Linux|*BSD)
    LD_LIBRARY_PATH="$build_top_dir/lib/.libs:$build_top_dir/lib:$LD_LIBRARY_PATH"
    export LD_LIBRARY_PATH
    ;;
  Darwin)
    DYLD_LIBRARY_PATH="$build_top_dir/lib/.libs:$build_top_dir/lib:$DYLD_LIBRARY_PATH"
    export DYLD_LIBRARY_PATH
    ;;
  *)
    :
    ;;
esac

if test -z "$RUBY"; then
  exit 1
fi

$GDB "$RUBY" "${SOURCE_DIR}/run-test.rb" "$@"
