#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 2 ]; then
    echo "Usage: $0 PACKAGE DISTRIBUTIONS"
    echo " e.g.: $0 groonga 'fedora centos'"
    exit 1
fi

PACKAGE=$1
DISTRIBUTIONS=$2

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

rpm_base_dir=$HOME/rpm

if [ ! -f ~/.rpmmacros ]; then
    run cat <<EOM > ~/.rpmmacros
%_topdir $rpm_base_dir
EOM
fi

run mkdir -p $rpm_base_dir/SOURCES
run mkdir -p $rpm_base_dir/SPECS
run mkdir -p $rpm_base_dir/BUILD
run mkdir -p $rpm_base_dir/RPMS
run mkdir -p $rpm_base_dir/SRPMS

for distribution in ${DISTRIBUTIONS}; do
    case $distribution in
	fedora)
	    distribution_label=Fedora
	    ;;
	centos)
	    distribution_label=CentOS
	    ;;
    esac
    repo=${PACKAGE}.repo
    run cat <<EOR > $repo
[groonga]
name=groonga for $distribution_label \$releasever - \$basearch
baseurl=http://groonga.sourceforge.net/$distribution/\$releasever/\$basearch/
gpgcheck=1
enabled=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-groonga
metadata_expire=7d
EOR
    run tar cfz $rpm_base_dir/SOURCES/${PACKAGE}-repository.tar.gz \
	-C ${script_base_dir} ${repo} RPM-GPG-KEY-${PACKAGE}
    run cp ${script_base_dir}/${PACKAGE}-repository.spec $rpm_base_dir/SPECS/

    run rpmbuild -ba $rpm_base_dir/SPECS/${PACKAGE}-repository.spec

    top_dir=$script_base_dir/$distribution

    run mkdir -p $top_dir
    run cp -p $rpm_base_dir/RPMS/noarch/${PACKAGE}-repository-* $top_dir
    run cp -p $rpm_base_dir/SRPMS/${PACKAGE}-repository-* $top_dir

    run cp -p ${script_base_dir}/RPM-GPG-KEY-${PACKAGE} $top_dir
done
