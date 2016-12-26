#!/bin/sh

LANG=C

run()
{
  "$@"
  if test $? -ne 0; then
    echo "Failed $@"
    exit 1
  fi
}

. /vagrant/tmp/env.sh

swap_file=/tmp/swap
run sudo dd if=/dev/zero of="$swap_file" bs=1024 count=4096K
run sudo mkswap "$swap_file"
run sudo swapon "$swap_file"

run sudo sed -i'' -e 's/httpredir/ftp.jp/g' /etc/apt/sources.list

run sudo apt-get update
run sudo apt-get install -y lsb-release

distribution=$(lsb_release --id --short | tr 'A-Z' 'a-z')
code_name=$(lsb_release --codename --short)
case "${distribution}" in
  debian)
    component=main
    ;;
  ubuntu)
    component=universe
    ;;
esac

have_liblz4=yes
if ! apt-cache show liblz4-dev > /dev/null 2>&1; then
  have_liblz4=no
fi
have_libzstd=yes
if ! apt-cache show libzstd-dev > /dev/null 2>&1; then
  have_libzstd=no
fi

if [ "${have_liblz4}" = "no" ]; then
  DEPENDED_PACKAGES="$(echo ${DEPENDED_PACKAGES} | sed -e 's/liblz4-dev//')"
fi
if [ "${have_libzstd}" = "no" ]; then
  DEPENDED_PACKAGES="$(echo ${DEPENDED_PACKAGES} | sed -e 's/libzstd-dev//')"
fi
run sudo apt-get install -V -y build-essential devscripts ${DEPENDED_PACKAGES}

run mkdir -p build
run cp /vagrant/tmp/${PACKAGE}-${VERSION}.tar.gz \
  build/${PACKAGE}_${VERSION}.orig.tar.gz
run cd build
run tar xfz ${PACKAGE}_${VERSION}.orig.tar.gz
run cd ${PACKAGE}-${VERSION}/
run cp -rp /vagrant/tmp/debian debian
if [ "${have_liblz4}" = "no" ]; then
  grep -v liblz4 /vagrant/tmp/debian/control > debian/control
fi
if [ "${have_libzstd}" = "no" ]; then
  grep -v libzstd debian/control > debian/control.tmp
  cp -f debian/control.tmp debian/control
  rm -f debian/control.tmp
fi
# export DEB_BUILD_OPTIONS=noopt
run debuild -us -uc
run cd -

package_initial=$(echo "${PACKAGE}" | sed -e 's/\(.\).*/\1/')
pool_dir="/vagrant/repositories/${distribution}/pool/${code_name}/${component}/${package_initial}/${PACKAGE}"
run mkdir -p "${pool_dir}/"
run cp *.tar.* *.dsc *.deb "${pool_dir}/"
