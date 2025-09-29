# Maintainer: Hiroshi Hatake <cosmo0920.wp[at]gmail.com>

pkgname=groonga
pkgdesc="An open-source fulltext search engine and column store."
pkgver=15.1.7
pkgrel=1
arch=('i686' 'x86_64')
url="https://groonga.org/"
license=('LGPL2.1-or-later')
depends=(
  arrow
  blosc2
  gcc
  libedit
  libevent
  libstemmer
  lz4
  mecab-git
  mecab-ipadic
  msgpack-c
  simdjson
  xsimd
  xxhash
  zeromq
)
makedepends=(
  cmake
  ninja
  pkgconf
  ruby
  ruby-rake
)
checkdepends=(
  git
  make
  ruby-bundler
  ruby-erb
)
source=(
  "https://github.com/groonga/groonga/releases/download/v${pkgver}/${pkgname}-${pkgver}.tar.gz"
  "https://github.com/groonga/groonga/releases/download/v${pkgver}/${pkgname}-${pkgver}.tar.gz.asc"
)
sha512sums=(
  "ded60e2736fd4b4d44046525041101978b47896ec5c0619ea3d10a7042e6cc6fdd544c0dd0e7006ca9f384fc2198ce32659eb446b1b8ee5a6bb4e8ad8f9f5b87"
  "SKIP"
)
validpgpkeys=(2701F317CFCCCB975CADE9C2624CF77434839225)

# See also: https://wiki.archlinux.org/title/CMake_package_guidelines
build() {
  rm -rf build
  local cmake_options=(
    -B build
    -S "${pkgname}-${pkgver}"
    -G Ninja
    -W no-dev
    -D CMAKE_BUILD_TYPE=None
    -D CMAKE_INSTALL_PREFIX=/usr
    -D CMAKE_SKIP_RPATH=ON
    -D GRN_WITH_APACHE_ARROW=ON
    -D GRN_WITH_BLOSC=system
    -D GRN_WITH_MRUBY=ON
  )
  cmake "${cmake_options[@]}"
  cmake --build build
}

check() {
  cd build
  export GEM_HOME="${PWD}/gem"
  PATH="${GEM_HOME}/bin:${PATH}"
  MAKEFLAGS="-j$(nproc)" gem install --no-user-install grntest
  BUILD_DIR="${PWD}/test/command" \
    "../${pkgname}-${pkgver}/test/command/run-test.sh" \
    --n-retries=2 \
    --read-timeout=30 \
    --reporter=mark
}

package() {
  DESTDIR="${pkgdir}" cmake --install build
}
