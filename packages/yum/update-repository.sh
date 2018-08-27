#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 4 ]; then
    echo "Usage: $0 GPG_UID GPG_KEY_NAME DESTINATION DISTRIBUTIONS"
    echo " e.g.: $0 1BD22CD1 mitler-manager repositories/ 'fedora centos'"
    exit 1
fi

GPG_UID=$1
GPG_KEY_NAME=$2
DESTINATION=$3
DISTRIBUTIONS=$4

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

for distribution in ${DISTRIBUTIONS}; do
    for dir in ${DESTINATION}${distribution}/*/*; do
        case $dir in
            repositories/centos/5*)
                continue
                ;;
            *)
                test -d $dir &&	run createrepo $dir
                ;;
        esac
    done;

    run gpg --armor --export ${GPG_UID} > \
	${DESTINATION}${distribution}/RPM-GPG-KEY-${GPG_KEY_NAME};
done
