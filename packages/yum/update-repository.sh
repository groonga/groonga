#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 3 ]; then
    echo "Usage: $0 GPG_KEY_NAME DESTINATION DISTRIBUTIONS"
    echo " e.g.: $0 mitler-manager repositories/ 'fedora centos'"
    exit 1
fi

GPG_KEY_NAME=$1
DISTRIBUTIONS=$2

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

for distribution in ${DISTRIBUTIONS}; do
    for dir in $script_base_dir/${distribution}/*/*; do
	test -d $dir &&	run createrepo $dir
    done;

    run cp $script_base_dir/RPM-GPG-KEY-${GPG_KEY_NAME} \
	$script_base_dir/${distribution}/RPM-GPG-KEY-${GPG_KEY_NAME};
done
