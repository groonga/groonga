#!/bin/bash

set -exu

apt update
apt install -V -y lsb-release

code_name=$(lsb_release --codename --short)
architecture=$(dpkg --print-architecture)
repositories_dir=/groonga/packages/apt/repositories
apt install -V -y \
  ${repositories_dir}/debian/pool/${code_name}/main/*/*/*_{${architecture},all}.deb

groonga --version

apt install -V -y \
  gcc \
  make \
  ruby-dev
gem install grntest

export TZ=Asia/Tokyo

grntest_options=()
grntest_options+=(--base-directory=/groonga/test/command)
grntest_options+=(--n-retries=3)
grntest_options+=(--n-workers=$(nproc))
grntest_options+=(--reporter=mark)
grntest_options+=(/groonga/test/command/suite)

grntest "${grntest_options[@]}"

# TODO: Require Apache Arrow for testing HTTP interface
exit 0

grntest "${grntest_options[@]}" --interface http
grntest "${grntest_options[@]}" --interface http --testee groonga-httpd
