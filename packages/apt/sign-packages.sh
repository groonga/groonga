#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 2 ]; then
    echo "Usage: $0 GPG_UID CODES"
    echo " e.g.: $0 'F10399C0' 'lenny unstable hardy karmic'"
    exit 1
fi

GPG_UID=$1
CODES=$2

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

for code_name in ${CODES}; do
    case ${code_name} in
	lenny|squeeze|wheezy|unstable)
	    distribution=debian
	    ;;
	*)
	    distribution=ubuntu
	    ;;
    esac

    base_directory=packages/${distribution}
    debsign --re-sign -k${GPG_UID} \
	$(find ${base_directory} -name '*.dsc' -or -name '*.changes') &
    if [ "${PARALLEL}" != "yes" ]; then
	wait
    fi
done

wait
