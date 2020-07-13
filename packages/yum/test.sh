#!/bin/bash

set -exu

version=$(cut -d: -f5 /etc/system-release-cpe)
case ${version} in
  6|7)
    DNF=yum
    ;;
  *)
    DNF="dnf --enablerepo=PowerTools"
    ;;
esac

${DNF} install -y \
  https://packages.groonga.org/centos/groonga-release-latest.noarch.rpm

repositories_dir=/groonga/packages/yum/repositories
${DNF} install -y \
  ${repositories_dir}/centos/${version}/x86_64/Packages/*.rpm

groonga --version

case ${version} in
  6|7)
    exit 0
    ;;
  *)
    ;;
esac

${DNF} install -y \
  gcc \
  make \
  redhat-rpm-config \
  ruby-devel
gem install grntest

export TZ=Asia/Tokyo

grntest_options=()
grntest_options+=(--base-directory=/groonga/test/command)
grntest_options+=(--n-retries=3)
grntest_options+=(--n-workers=$(nproc))
grntest_options+=(--reporter=mark)
grntest_options+=(/groonga/test/command/suite)

grntest "${grntest_options[@]}"
grntest "${grntest_options[@]}" --interface http
grntest "${grntest_options[@]}" --interface http --testee groonga-httpd
