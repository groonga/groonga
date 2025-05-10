#!/bin/bash
#
# Copyright (C) 2025  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

set -eux

echo "::group::Prepare"
set -x
cp /source/groonga-*.tar.gz ./
version=$(echo groonga-*.tar.gz | grep -E -o '[0-9]+\.[0-9]+\.[0-9]+')
touch groonga-${version}.tar.gz.asc
sed \
  -e "s/^pkgver=.*/pkgver=${version}/" \
  /source/ci/arch-linux/PKGBUILD > PKGBUILD
set +x
echo "::endgroup::"

echo "::group::Install dependencies in AUR"
set -x
for package in $(grep mecab PKGBUILD); do
  git clone https://aur.archlinux.org/${package}.git
  pushd ${package}
  makepkg \
    --install \
    --needed \
    --noconfirm \
    --rmdeps \
    --skipchecksums \
    --skippgpcheck \
    --syncdeps
  popd
  rm -rf ${package}
done
set +x
echo "::endgroup::"

echo "::group::Build"
set -x
makepkg \
  --needed \
  --noconfirm \
  --rmdeps \
  --skipchecksums \
  --skippgpcheck \
  --syncdeps
set +x
echo "::endgroup::"

echo "::group::Install"
set -x
sudo pacman \
  --needed \
  --noconfirm \
  --upgrade \
  ./*.pkg.tar.*
set +x
echo "::endgroup::"

echo "::group::Prepare test"
set -x
sudo pacman \
  --needed \
  --noconfirm \
  --sync \
  make \
  ruby-erb
rm -rf groonga-${version}
tar xf groonga-${version}.tar.gz
(
  cd groonga-${version}
  export GEM_HOME="${PWD}/gem"
  PATH="${GEM_HOME}/bin:${PATH}"
  MAKEFLAGS="-j$(nproc)" gem install --no-user-install grntest
  export TZ=Asia/Tokyo
  ln -s /dev/shm tmp
  grntest \
    --base-dir=test/command \
    --n-retries=2 \
    --read-timeout=30 \
    --reporter=mark \
    test/command/suite
)
set +x
echo "::endgroup::"

echo "::group::Copy"
set -x
sudo cp *.pkg.tar.* /source/
set +x
echo "::endgroup::"
