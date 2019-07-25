#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 3 ]; then
  echo "Usage: $0 PACKAGE DESTINATION DISTRIBUTIONS"
  echo " e.g.: $0 milter-manager repositories/ 'fedora centos'"
  exit 1
fi

PACKAGE=$1
DESTINATION=$2
DISTRIBUTIONS=$3

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
      distribution_versions="20"
      ;;
    centos)
      distribution_label=CentOS
      distribution_versions="6 7"
      ;;
  esac
  run cat <<EOR > groonga-centos.repo
[$PACKAGE-centos]
name=Groonga for CentOS \$releasever - \$basearch
baseurl=https://packages.groonga.org/$distribution/\$releasever/\$basearch/
gpgcheck=1
enabled=1
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-$PACKAGE
       file:///etc/pki/rpm-gpg/RPM-GPG-KEY-$PACKAGE-RSA4096
EOR
  run cat <<EOR > groonga-amazon-linux.repo
[$PACKAGE-amazon-linux]
name=Groonga for Amazon Linux 2 - \$basearch
baseurl=https://packages.groonga.org/centos/7/\$basearch/
gpgcheck=1
enabled=0
gpgkey=file:///etc/pki/rpm-gpg/RPM-GPG-KEY-groonga
       file:///etc/pki/rpm-gpg/RPM-GPG-KEY-groonga-RSA4096
EOR
  run tar cfz $rpm_base_dir/SOURCES/${PACKAGE}-release.tar.gz \
      -C ${script_base_dir} RPM-GPG-KEY-${PACKAGE} RPM-GPG-KEY-${PACKAGE}-RSA4096 \
         groonga-centos.repo groonga-amazon-linux.repo
  run cp ${script_base_dir}/${PACKAGE}-release.spec $rpm_base_dir/SPECS/

  run rpmbuild -ba $rpm_base_dir/SPECS/${PACKAGE}-release.spec

  top_dir=${DESTINATION}${distribution}

  run mkdir -p $top_dir
  run cp -p \
      $rpm_base_dir/RPMS/noarch/${PACKAGE}-release-* \
      $rpm_base_dir/SRPMS/${PACKAGE}-release-* \
      ${script_base_dir}/RPM-GPG-KEY-${PACKAGE} \
      $top_dir

  release_spec=$rpm_base_dir/SPECS/${PACKAGE}-release.spec
  release_version=$(grep '^Version: ' ${release_spec} | \
                      sed -e 's/^Version: //')
  release_release=$(grep '^Release: ' ${release_spec} | \
                      sed -e 's/^Release: //')
  run cd $top_dir
  run ln -fs \
      ${PACKAGE}-release-${release_version}-${release_release}.noarch.rpm \
      ${PACKAGE}-release-latest.noarch.rpm
  run cd -

  for distribution_version in $distribution_versions; do
    cp $top_dir/*.src.rpm $top_dir/$distribution_version/source/SRPMS/
    if [ -d "$top_dir/$distribution_version/i386/Packages" ]; then
      cp $top_dir/*.noarch.rpm $top_dir/$distribution_version/i386/Packages/
    fi
    cp $top_dir/*.noarch.rpm $top_dir/$distribution_version/x86_64/Packages/
  done
done
