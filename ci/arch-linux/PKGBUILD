# Maintainer: Hiroshi Hatake <cosmo0920.wp[at]gmail.com>

pkgname=groonga
pkgdesc="An open-source fulltext search engine and column store."
pkgver=15.2.4
pkgrel=1
arch=('i686' 'x86_64')
url="https://groonga.org/"
license=('LGPL2.1-or-later')
depends=(
  arrow
  blas
  blosc2
  gcc
  lapack
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
  "0ebee91d27154b1237704115b1ed47e2b8700ebe02a3c34469af7d9b6fdfa8f9cd57698cbe5a3e4189a89b254bfa497029f79dc803a036122e97b2a4eb54c9fc"
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
