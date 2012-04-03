#!/bin/sh

export BASE_DIR="`dirname $0`"
if test -z "$BUILD_DIR"; then
    BUILD_DIR="$BASE_DIR"
fi
export BUILD_DIR

top_dir="$BUILD_DIR/../.."
top_dir=$(cd -P "$top_dir" 2>/dev/null || cd "$top_dir"; pwd)

if test x"$NO_MAKE" != x"yes"; then
    make -C $top_dir > /dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="`make -s -C $top_dir echo-cutter`"
fi
export CUTTER

if test -z "$RUBY"; then
    RUBY="`make -s -C $top_dir echo-ruby`"
fi
export RUBY

if test -z "$GROONGA"; then
    GROONGA="`make -s -C $top_dir echo-groonga`"
fi
export GROONGA

if test -z "$GROONGA_BENCHMARK"; then
    GROONGA_BENCHMARK="`make -s -C $top_dir echo-groonga-benchmark`"
fi
export GROONGA_BENCHMARK

CUTTER_ARGS=
CUTTER_WRAPPER=
if test x"$CUTTER_DEBUG" = x"yes"; then
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute gdb --args"
    CUTTER_ARGS="--keep-opening-modules"
elif test x"$CUTTER_CHECK_LEAK" = x"yes"; then
    CUTTER_WRAPPER="$top_dir/libtool --mode=execute valgrind "
    CUTTER_WRAPPER="$CUTTER_WRAPPER --leak-check=full --show-reachable=yes -v"
    CUTTER_ARGS="--keep-opening-modules"
fi

RUBY_TEST_ARGS=
if test x"$PRIORITY_MODE" = x"yes"; then
    RUBY_TEST_ARGS="$RUBY_TEST_ARGS --priority-mode"
    # CUTTER_ARGS="$CUTTER_ARGS --priority-mode"
fi

CUTTER_ARGS="$CUTTER_ARGS -s $BASE_DIR"
if test x"$CUTTER_VERBOSE" = x"yes"; then
    CUTTER_ARGS="$CUTTER_ARGS -v v"
fi
CUTTER_ARGS="$CUTTER_ARGS --exclude-directory fixtures"
CUTTER_ARGS="$CUTTER_ARGS --exclude-directory lib"
CUTTER_ARGS="$CUTTER_ARGS --exclude-file test-performance.so"

GRN_PLUGINS_DIR="$top_dir/plugins"
export GRN_PLUGINS_DIR

case `uname` in
    Darwin)
	DYLD_LIBRARY_PATH="$top_dir/lib/.libs:$DYLD_LIBRARY_PATH"
	DYLD_LIBRARY_PATH="$top_dir/test/unit/lib/.libs:$DYLD_LIBRARY_PATH"
	export DYLD_LIBRARY_PATH
	;;
    *)
	:
	;;
esac

no_test=1

cutter_result=0
if test "$NO_CUTTER" != "yes" -a -n "$CUTTER"; then
    $CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BUILD_DIR
    cutter_result=$?
    no_test=0
fi

ruby_result=0
if test "$NO_RUBY" != "yes" -a -n "$RUBY"; then
    : ${TEST_UNIT_MAX_DIFF_TARGET_STRING_SIZE:=30000}
    export TEST_UNIT_MAX_DIFF_TARGET_STRING_SIZE
    BUNDLE_GEMFILE="$BUILD_DIR/Gemfile"
    export BUNDLE_GEMFILE
    if [ ! -e "$BUNDLE_GEMFILE" ]; then
	ln -s "$BASE_DIR/Gemfile" "$BUNDLE_GEMFILE"
    fi
    if ! type bundle > /dev/null; then
	$RUBY -S gem install bundler
    fi
    if [ "$BUNDLE_GEMFILE" -nt "$BUNDLE_GEMFILE.lock" ]; then
	$RUBY -S bundle install
    fi
    $RUBY $BASE_DIR/run-test.rb $RUBY_TEST_ARGS "$@"
    ruby_result=$?
    no_test=0
fi

if [ $no_test = 0 -a $cutter_result = 0 -a $ruby_result = 0 ]; then
    exit 0
else
    exit 1
fi
