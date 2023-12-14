#!/bin/bash

source_dir=$(cd $(dirname $0) && pwd)
: ${BASE_DIR:=$source_dir}
: ${BUILD_DIR:=$source_dir}
build_dir=$(cd "${BUILD_DIR}" && pwd)

export BASE_DIR

source_top_dir="$source_dir/../.."
source_top_dir=$(cd "$source_top_dir" && pwd)

build_top_dir="$build_dir/../.."
build_top_dir=$(cd "$build_top_dir" && pwd)

if test x"$NO_MAKE" != x"yes"; then
  MAKE_ARGS=()
  case $(uname) in
    Linux)
      MAKE_ARGS+=("-j$(nproc)")
      ;;
    Darwin)
      MAKE_ARGS+=("-j$(/usr/sbin/sysctl -n hw.ncpu)")
      ;;
    *)
      :
      ;;
  esac
  make "${MAKE_ARGS[@]}" -C $build_top_dir > /dev/null || exit 1
fi

. "${build_top_dir}/config.sh"

TZ=Asia/Tokyo
export TZ

CUTTER_ARGS=()
CUTTER_WRAPPER=()
if test x"$CUTTER_DEBUG" = x"yes"; then
  CUTTER_WRAPPER+=("$build_top_dir/libtool" "--mode=execute" "gdb" "--args")
  CUTTER_ARGS+=("--keep-opening-modules")
elif test x"$CUTTER_CHECK_LEAK" = x"yes"; then
  CUTTER_WRAPPER+=("$build_top_dir/libtool" "--mode=execute" "valgrind")
  CUTTER_WRAPPER+=("--leak-check=full" "--show-reachable=yes" "-v")
  CUTTER_ARGS+=("--keep-opening-modules")
fi

CUTTER_ARGS+=("-s" "$build_dir")
if test x"$CUTTER_VERBOSE" = x"yes"; then
  CUTTER_ARGS+=("-v" "v")
fi
CUTTER_ARGS+=("--exclude-directory" "fixtures")
CUTTER_ARGS+=("--exclude-directory" "lib")

GRN_PLUGINS_DIR="$source_top_dir/plugins"
GRN_PLUGINS_PATH="$build_top_dir/plugins"
export GRN_PLUGINS_DIR
export GRN_PLUGINS_PATH

GRN_RUBY_SCRIPTS_DIR="$source_top_dir/lib/mrb/scripts"
export GRN_RUBY_SCRIPTS_DIR

case $(uname) in
  Linux|*BSD)
    LD_LIBRARY_PATH="$build_top_dir/lib/.libs:$LD_LIBRARY_PATH"
    LD_LIBRARY_PATH="$build_top_dir/test/unit/lib/.libs:$LD_LIBRARY_PATH"
    export LD_LIBRARY_PATH
    ;;
  Darwin)
    DYLD_LIBRARY_PATH="$build_top_dir/lib/.libs:$DYLD_LIBRARY_PATH"
    DYLD_LIBRARY_PATH="$build_top_dir/test/unit/lib/.libs:$DYLD_LIBRARY_PATH"
    export DYLD_LIBRARY_PATH
    ;;
  *)
    :
    ;;
esac

tmpfs_candidates="/dev/shm /run/shm"
for tmpfs in $tmpfs_candidates; do
  if test -d $tmpfs -a -w $tmpfs; then
    rm -rf "$build_dir/tmp"
    groonga_tmp_dir="${tmpfs}/groonga"
    rm -rf "${groonga_tmp_dir}"
    mkdir -p "${groonga_tmp_dir}"
    ln -s "${groonga_tmp_dir}" "$build_dir/tmp"
  fi
done

"${CUTTER_WRAPPER[@]}" $CUTTER "${CUTTER_ARGS[@]}" "$@" $build_dir
