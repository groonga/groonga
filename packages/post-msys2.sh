#!/usr/bin/env bash
#
# Copyright(C) 2023 Takashi Hashida <hashida@clear-code.com>
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

set -ue

if [ "$#" -ne 2 ]; then
  echo "Usage: $0 <version> <msys2/MINGW-packages' fork repository>"
  exit 1
fi

version=$1
repository=$2

if [ ! -d "${repository}" ]; then
  echo "msys2/MINGW-packages' fork repository doesn't exist: ${repository}"
  exit 1
fi

cd "${repository}"
if [ ! -d .git ]; then
  echo "not a Git repository: ${repository}"
  exit 1
fi

if ! git remote | grep -q '^upstream$'; then
  echo "'upstream' remote doesn't exist: ${repository}"
  echo "Run the following command line in ${repository}:"
  echo "  git remote add upstream https://github.com/msys2/MINGW-packages.git"
  exit 1
fi

echo "Updating repository: ${repository}"
git fetch --all --prune --tags --force
git checkout master
git rebase upstream/master

branch="groonga-${version}"
echo "Creating branch: ${branch}"
git branch -D ${branch} || :
git checkout -b ${branch}

pkgbuild=mingw-w64-groonga/PKGBUILD
echo "Updating PKGBUILD: ${pkgbuild}"

sha256sum=$(curl --location --output - \
              https://packages.groonga.org/source/groonga/groonga-${version}.tar.gz | \
              sha256sum - | \
              cut -d ' ' -f 1)

sed \
  -i.bak \
  -e "s/^pkgver=.*\$/pkgver=${version}/" \
  -e "s/^pkgrel=.*\$/pkgrel=1/" \
  -e "s/^sha256sums=.*\$/sha256sums=('${sha256sum}'/" \
  ${pkgbuild}
rm ${pkgbuild}.bak

git add ${pkgbuild}
git commit -m "Groonga: Update to ${version}"
git push origin ${branch}

owner=$(git remote get-url origin | \
          grep -o '[a-zA-Z0-9_-]*/MINGW-packages' | \
          cut -d/ -f1)
echo "Create a pull request:"
echo "  https://github.com/${owner}/MINGW-packages/pull/new/${branch}"
echo "with title: 'Groonga: Update to ${version}'"