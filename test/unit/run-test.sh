#!/bin/sh

export BASE_DIR="`dirname $0`"
top_dir="$BASE_DIR/../.."

if test x"$NO_MAKE" != x"yes"; then
    make -C $top_dir > /dev/null || exit 1
fi

if test -z "$CUTTER"; then
    CUTTER="`make -s -C $BASE_DIR echo-cutter`"
fi
export CUTTER

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

CUTTER_ARGS="$CUTTER_ARGS -s $BASE_DIR"
CUTTER_ARGS="$CUTTER_ARGS --exclude-directory fixtures"
CUTTER_ARGS="$CUTTER_ARGS --exclude-directory lib"
CUTTER_ARGS="$CUTTER_ARGS --exclude-file test-performance.so"

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

result=0
if test -n "$CUTTER"; then
    $CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BASE_DIR
    result=$?
fi

if test -n "$RUBY"; then
    if ! $RUBY $BASE_DIR/run-test.rb; then
	result=$?
    fi
fi

exit $result
