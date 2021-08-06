#!/bin/bash

set -exu

version=$(cut -d: -f5 /etc/system-release-cpe)
case ${version} in
  7)
    DNF=yum
    ;;
  *)
    DNF="dnf --enablerepo=powertools"
    ;;
esac

${DNF} install -y \
  https://packages.groonga.org/centos/groonga-release-latest.noarch.rpm

repositories_dir=/groonga/packages/yum/repositories
${DNF} install -y \
  ${repositories_dir}/centos/${version}/x86_64/Packages/*.rpm

groonga --version

case ${version} in
  7)
    exit 0
    ;;
  *)
    ;;
esac

# TODO: mecab-devel is needed
exit

${DNF} install -y \
  gcc \
  make \
  redhat-rpm-config \
  ruby-devel
MAKEFLAGS=-j$(nproc) gem install grntest

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
