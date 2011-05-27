#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 1 ]; then
    echo "Usage: $0 CODES"
    echo " e.g.: $0 'lenny unstable hardy karmic'"
    exit 1
fi

CODES=$1

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

    release=${distribution}/dists/${code_name}/Release
    rm -f ${release}.gpg
    gpg --sign -ba -o ${release}.gpg ${release}
done
