#!/bin/sh

export BASE_DIR="`dirname $0`"
if test -z "$BUILD_DIR"; then
    BUILD_DIR="$BASE_DIR"
fi
export BUILD_DIR

top_dir="$BUILD_DIR/../.."
top_dir=$(cd -P "$top_dir" 2>/dev/null || cd "$top_dir"; pwd)

if test x"$NO_MAKE" != x"yes"; then
    MAKE_ARGS=
    case `uname` in
	Linux)
	    MAKE_ARGS="-j$(grep '^processor' /proc/cpuinfo | wc -l)"
	    ;;
	Darwin)
	    MAKE_ARGS="-j$(/usr/sbin/sysctl -n hw.ncpu)"
	    ;;
	*)
	    :
	    ;;
    esac
    make $MAKE_ARGS -C $top_dir > /dev/null || exit 1
fi

. "${top_dir}/config.sh"

TZ=Asia/Tokyo
export TZ

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
if test x"$CUTTER_VERBOSE" = x"yes"; then
    CUTTER_ARGS="$CUTTER_ARGS -v v"
fi
CUTTER_ARGS="$CUTTER_ARGS --exclude-directory fixtures"
CUTTER_ARGS="$CUTTER_ARGS --exclude-directory lib"

GRN_PLUGINS_DIR="$top_dir/plugins"
export GRN_PLUGINS_DIR

GRN_RUBY_SCRIPTS_DIR="$top_dir/lib/mrb/scripts"
export GRN_RUBY_SCRIPTS_DIR

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

tmpfs_candidates="/dev/shm /run/shm"
for tmpfs in $tmpfs_candidates; do
    if test -d $tmpfs -a -w $tmpfs; then
	rm -rf "$BASE_DIR/tmp"
	ln -s $tmpfs "$BASE_DIR/tmp"
    fi
done

$CUTTER_WRAPPER $CUTTER $CUTTER_ARGS "$@" $BUILD_DIR
