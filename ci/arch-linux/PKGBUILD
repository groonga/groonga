# Maintainer: Hiroshi Hatake <cosmo0920.wp[at]gmail.com>

pkgname=groonga
pkgdesc="An open-source fulltext search engine and column store."
pkgver=15.1.5
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
  "70f0d4c087d9826dd3161054ce1b529240c0797769a3ee99486b9cad4b294ead3fda99ded4a21c5a042106808f7566a38dd4cd911eec96c7066b28f65dc8b752"
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
