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

if test -z "$RUBY"; then
    RUBY="`make -s -C $BASE_DIR echo-ruby`"
fi
export RUBY

if test -z "$GROONGA"; then
    GROONGA="`make -s -C $BASE_DIR echo-groonga`"
fi
export GROONGA

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

no_test=1

cutter_result=0
if test "$NO_CUTTER" != "yes" -a -n "$CUTTER"; then
    $CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BASE_DIR
    cutter_result=$?
    no_test=0
fi

ruby_result=0
if test "$NO_RUBY" != "yes" -a -n "$RUBY"; then
    $RUBY $BASE_DIR/run-test.rb $RUBY_TEST_ARGS "$@"
    ruby_result=$?
    no_test=0
fi

if [ "$IGNORE_EXIT_STATUS" = "yes" ]; then
    exit 0
else
    if [ $no_test = 0 -a $cutter_result = 0 -a $ruby_result = 0 ]; then
	exit 0
    else
	exit 1
    fi
fi
