#!/bin/sh

OPTIONS=`getopt -n $0 -u -o " " -l protocol: -- "$@"`
if test $? -ne 0; then
    exit 1
fi
set -- $OPTIONS
while true;
do
    case $1 in
	--protocol)
	    protocol=$2
	    shift
            ;;
	--)
	    shift
	    break
	    ;;
    esac
    shift
done

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

if test -z "$RUBY"; then
    RUBY="`make -s -C $top_dir echo-ruby`"
fi
export RUBY

if test -z "$GROONGA"; then
    GROONGA="`make -s -C $top_dir echo-groonga`"
fi
export GROONGA

if test -z "$GROONGA_SUGGEST_CREATE_DATASET"; then
    GROONGA_SUGGEST_CREATE_DATASET="`make -s -C $top_dir echo-groonga-suggest-create-dataset`"
fi
export GROONGA_SUGGEST_CREATE_DATASET

GRN_PLUGINS_DIR="$top_dir/plugins"
export GRN_PLUGINS_DIR

case `uname` in
    Darwin)
	DYLD_LIBRARY_PATH="$top_dir/lib/.libs:$DYLD_LIBRARY_PATH"
	export DYLD_LIBRARY_PATH
	;;
    *)
	:
	;;
esac

if test -z "$RUBY"; then
    exit 1
fi

grntest_dir="$BASE_DIR/grntest"
if ! test -d "$grntest_dir"; then
    git clone git://github.com/groonga/grntest.git "$grntest_dir"
fi

if test $# -eq 0; then
    targets="$BASE_DIR/suite"
else
    targets=
fi

if test -z $protocol; then
    protocol="gqtp"
fi

$RUBY -I "$grntest_dir/lib" \
    "$grntest_dir/bin/grntest" \
    --groonga "$GROONGA" \
    --groonga-suggest-create-dataset "$GROONGA_SUGGEST_CREATE_DATASET" \
    --base-directory "$BASE_DIR" \
    --protocol "$protocol" \
    "$targets" "$@"
