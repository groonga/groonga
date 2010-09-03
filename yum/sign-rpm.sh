#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 1 ]; then
    echo "Usage: $0 DISTRIBUTIONS"
    echo " e.g.: $0 'fedora centos'"
    exit 1
fi

DISTRIBUTIONS=$1

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

for distribution in ${DISTRIBUTIONS}; do
    for rpm in $script_base_dir/${distribution}/*/*/*/*.rpm; do
	run rpm -D "_gpg_name `$script_base_dir/gpg-uid.sh`" --resign $rpm
    done;
done
