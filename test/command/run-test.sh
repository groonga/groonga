#!/bin/bash

export SOURCE_DIR="$(dirname "$0")"
if test -z "$BUILD_DIR"; then
  BUILD_DIR="$SOURCE_DIR"
fi
export BUILD_DIR

source_top_dir="$SOURCE_DIR/../.."
source_top_dir=$(cd -P "$source_top_dir" 2>/dev/null || cd "$source_top_dir"; pwd)

build_top_dir="$BUILD_DIR/../.."
build_top_dir=$(cd -P "$build_top_dir" 2>/dev/null || cd "$build_top_dir"; pwd)

n_processors=1
case $(uname) in
  Linux)
    n_processors="$(nproc)"
    ;;
  Darwin)
    n_processors="$(/usr/sbin/sysctl -n hw.ncpu)"
    ;;
  *)
    :
    ;;
esac

# For backward compatibility
: ${NO_BUILD:=$NO_MAKE}
if [ "${NO_BUILD}" != "yes" ]; then
  echo "${build_top_dir}/build.ninja"
  if [ -f "${build_top_dir}/build.ninja" ]; then
    ninja -C "${build_top_dir}" > /dev/null || exit 1
  else
    MAKE_ARGS=
    if [ ${n_processors} -gt 1 ]; then
      MAKE_ARGS="${MAKE_ARGS} -j${n_processors}"
    fi
    make -C $build_top_dir ${MAKE_ARGS} > /dev/null || exit 1
  fi
fi

. "${build_top_dir}/config.sh"

GRN_PLUGINS_DIR="$source_top_dir/plugins"
GRN_PLUGINS_PATH="$build_top_dir/plugins"
export GRN_PLUGINS_DIR
export GRN_PLUGINS_PATH

GRN_RUBY_SCRIPTS_DIR="$source_top_dir/lib/mrb/scripts"
export GRN_RUBY_SCRIPTS_DIR

case `uname` in
  Linux|*BSD)
    LD_LIBRARY_PATH="$build_top_dir/lib/.libs:$LD_LIBRARY_PATH"
    LD_LIBRARY_PATH="$build_top_dir/lib:$LD_LIBRARY_PATH"
    export LD_LIBRARY_PATH
    ;;
  Darwin)
    DYLD_LIBRARY_PATH="$build_top_dir/lib/.libs:$DYLD_LIBRARY_PATH"
    DYLD_LIBRARY_PATH="$build_top_dir/lib:$DYLD_LIBRARY_PATH"
    export DYLD_LIBRARY_PATH
    ;;
  *)
    :
    ;;
esac

if test -z "$RUBY"; then
  exit 1
fi

if ! type bundle 2>&1 > /dev/null; then
  $RUBY -S gem install bundler
fi

grntest_dir="$SOURCE_DIR/grntest"
if ! test -d "$grntest_dir"; then
  grntest_dir="$BUILD_DIR/grntest"
  git clone --depth 1 https://github.com/groonga/grntest.git "$grntest_dir"
  (cd "$grntest_dir" && bundle install)
fi
(cd "$grntest_dir";
 if [ "Gemfile" -nt "Gemfile.lock" ]; then
   $RUBY -S bundle update
 fi)

groonga_command_dir="$SOURCE_DIR/groonga-command"
if ! test -d "$groonga_command_dir"; then
  groonga_command_dir="$BUILD_DIR/groonga-command"
fi
if ! test -d "$groonga_command_dir"; then
  git clone --depth 1 \
      https://github.com/groonga/groonga-command.git \
      "$groonga_command_dir"
fi

groonga_command_parser_dir="$SOURCE_DIR/groonga-command-parser"
if ! test -d "$groonga_command_parser_dir"; then
  groonga_command_parser_dir="$BUILD_DIR/groonga-command-parser"
fi
if ! test -d "$groonga_command_parser_dir"; then
  git clone --depth 1 \
      https://github.com/groonga/groonga-command-parser.git \
      "$groonga_command_parser_dir"
fi

gqtp_dir="$SOURCE_DIR/gqtp"
if ! test -d "$gqtp_dir"; then
  gqtp_dir="$BUILD_DIR/gqtp"
fi
if ! test -d "$gqtp_dir"; then
  git clone --depth 1 \
      https://github.com/ranguba/gqtp.git \
      "$gqtp_dir"
fi

groonga_client_dir="$SOURCE_DIR/groonga-client"
if ! test -d "$groonga_client_dir"; then
  groonga_client_dir="$BUILD_DIR/groonga-client"
fi
if ! test -d "$groonga_client_dir"; then
  git clone --depth 1 \
      https://github.com/ranguba/groonga-client.git \
      "$groonga_client_dir"
fi

groonga_log_dir="$SOURCE_DIR/groonga-log"
if ! test -d "$groonga_log_dir"; then
  groonga_log_dir="$BUILD_DIR/groonga-log"
fi
if ! test -d "$groonga_log_dir"; then
  git clone --depth 1 \
      https://github.com/groonga/groonga-log.git \
      "$groonga_log_dir"
fi

groonga_query_log_dir="$SOURCE_DIR/groonga-query-log"
if ! test -d "$groonga_query_log_dir"; then
  groonga_query_log_dir="$BUILD_DIR/groonga-query-log"
fi
if ! test -d "$groonga_query_log_dir"; then
  git clone --depth 1 \
      https://github.com/groonga/groonga-query-log.git \
      "$groonga_query_log_dir"
fi

have_targets="false"
use_gdb="false"
use_rr="false"
use_valgrind="false"
next_argument_is_long_option_value="false"
for argument in "$@"; do
  case "$argument" in
    --*=*)
      ;;
    --keep-database|--stop-on-failure|--no-*|--version|--help)
      # no argument options
      ;;
    --gdb)
      # no argument options
      use_gdb="true"
      ;;
    --rr)
      # no argument options
      use_rr="true"
      ;;
    --valgrind)
      # no argument options
      use_valgrind="true"
      ;;
    --*)
      next_argument_is_long_option_value="true"
      continue
      ;;
    -*)
      ;;
    *)
      if test "$next_argument_is_long_option_value" != "true"; then
        have_targets="true"
      fi
      ;;
  esac
  next_argument_is_long_option_value="false"
done

grntest_options=("$@")
if test "$use_gdb" = "true" -o \
        "$use_rr" = "true" -o \
        "$use_valgrind" = "true"; then
  grntest_options=("--reporter" "stream" "${grntest_options[@]}")
else
  grntest_options=("--n-workers" "${n_processors}" "${grntest_options[@]}")
fi
if test "$CI" = "true"; then
  grntest_options=("--reporter" "mark" "${grntest_options[@]}")
fi
if test "$have_targets" != "true"; then
  grntest_options=("${grntest_options[@]}" "${SOURCE_DIR}/suite")
fi

tmpfs_candidates=("/dev/shm" "/run/shm")
for tmpfs in "${tmpfs_candidates[@]}"; do
  if test -d $tmpfs -a -w $tmpfs; then
    rm -rf "tmp"
    ln -s $tmpfs "tmp"
  fi
done

export TZ=Asia/Tokyo

$RUBY \
  -I "$grntest_dir/lib" \
  -I "$groonga_command_dir/lib" \
  -I "$groonga_command_parser_dir/lib" \
  -I "$gqtp_dir/lib" \
  -I "$groonga_client_dir/lib" \
  -I "$groonga_log_dir/lib" \
  -I "$groonga_query_log_dir/lib" \
  "$grntest_dir/bin/grntest" \
  --groonga "$GROONGA" \
  --groonga-httpd "$GROONGA_HTTPD" \
  --groonga-suggest-create-dataset "$GROONGA_SUGGEST_CREATE_DATASET" \
  --base-directory "$SOURCE_DIR" \
  "${grntest_options[@]}"
