#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 2 ]; then
    echo "Usage: $0 ARCHITECTURES CODES"
    echo " e.g.: $0 'i386 amd64' 'lenny unstable hardy karmic'"
    exit 1
fi

ARCHITECTURES=$1
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
	lenny|squeeze|unstable)
	    distribution=debian
	    component=main
	    ;;
	*)
	    distribution=ubuntu
	    component=universe
	    ;;
    esac

    (cd ${distribution}
	for architecture in ${ARCHITECTURES}; do
	    mkdir -p dists/${code_name}/${component}/binary-${architecture}
	    mkdir -p dists/${code_name}/${component}/source
	done
	rm -f *.db
	apt-ftparchive generate generate-${code_name}.conf
	rm -f dists/${code_name}/Release*
	apt-ftparchive -c release-${code_name}.conf \
	    release dists/${code_name} > /tmp/Release
	mv /tmp/Release dists/${code_name}
    );
done
