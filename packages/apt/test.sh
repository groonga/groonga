#!/bin/bash

set -exu

echo "debconf debconf/frontend select Noninteractive" | debconf-set-selections

apt update
apt install -V -y lsb-release wget

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
architecture=$(dpkg --print-architecture)

wget \
  https://packages.groonga.org/${distribution}/groonga-apt-source-latest-${code_name}.deb
apt install -V -y ./groonga-apt-source-latest-${code_name}.deb
apt update

repositories_dir=/groonga/packages/apt/repositories
apt install -V -y \
  ${repositories_dir}/${distribution}/pool/${code_name}/${component}/*/*/*_{${architecture},all}.deb

groonga --version
if ! groonga --version | grep -q apache-arrow; then
  echo "Apache Arrow isn't enabled"
  exit 1
fi

# There are some problems for running arm64 tests:
#   * Too long test time because of QEMU
if [ "${architecture}" == "arm64" ]; then
  exit
fi

mkdir -p /test
cd /test
cp -a /groonga/test/command ./

apt install -V -y \
  gcc \
  make \
  ruby-dev \
  tzdata
gem install rubygems-requirements-system
MAKEFLAGS=-j$(nproc) gem install grntest

if groonga --version | grep -q apache-arrow; then
  wget https://apache.jfrog.io/artifactory/arrow/${distribution}/apache-arrow-apt-source-latest-${code_name}.deb
  apt install -V -y ./apache-arrow-apt-source-latest-${code_name}.deb
  apt update
  apt install -V -y \
    g++ \
    libre2-dev
  MAKEFLAGS=-j$(nproc) gem install red-arrow
fi

export TZ=Asia/Tokyo
# Test only checks presence of MECAB_DICTIONARY_NAIST_JDIC. So the actual value
# is ignored.
export MECAB_DICTIONARY_NAIST_JDIC=YES

grntest_options=()
grntest_options+=(--base-directory=command)
grntest_options+=(--n-retries=2)
grntest_options+=(--reporter=mark)
grntest_options+=(command/suite)
grntest "${grntest_options[@]}"
# Run only one job to reduce CI time
if [ "${code_name}" == "bookworm" ]; then
  grntest "${grntest_options[@]}" --interface http
fi
