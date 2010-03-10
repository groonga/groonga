#!/bin/sh
GRN_VERSION_SH=version.sh

if test -f version
then
	GRN_VN=$(cat version)
elif test -d .git -o -f .git 
then
	GRN_VN=$(git describe --abbrev=7 HEAD 2>/dev/null)
fi

GRN_VN=$(expr "$GRN_VN" : v*'\(.*\)')

if test -r $GRN_VERSION_SH
then
	GRN_VN_OLD=$(sed -e 's/^GROONGA_VERSION=//' <$GRN_VERSION_SH)
else
	GRN_VN_OLD=unset
fi

test "$GRN_VN_OLD" = "$GRN_VN" || {
	echo "GROONGA_VERSION=$GRN_VN" >$GRN_VERSION_SH
}
