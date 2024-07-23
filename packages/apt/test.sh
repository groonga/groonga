#!/bin/bash

set -exu

apt update
apt install -V -y lsb-release wget

distribution=$(lsb_release --id --short | tr 'A-Z' 'a-z')
code_name=$(lsb_release --codename --short)
architecture=$(dpkg --print-architecture)

wget https://apache.jfrog.io/artifactory/arrow/${distribution}/apache-arrow-apt-source-latest-${code_name}.deb
apt install -V -y ./apache-arrow-apt-source-latest-${code_name}.deb

wget \
  https://packages.groonga.org/${distribution}/groonga-apt-source-latest-${code_name}.deb
apt install -V -y ./groonga-apt-source-latest-${code_name}.deb
apt update

repositories_dir=/groonga/packages/apt/repositories
apt install -V -y \
  ${repositories_dir}/debian/pool/${code_name}/main/*/*/*_{${architecture},all}.deb

groonga --version
if [ "${architecture}" != "i386" ]; then
  if ! groonga --version | grep -q apache-arrow; then
    echo "Apache Arrow isn't enabled"
    exit 1
  fi
fi

mkdir -p /test
cd /test
cp -a /groonga/test/command ./
if [ "${architecture}" = "i386" ]; then
  rm command/suite/ruby/eval/convert/string_to_time/over_int32.test
  # TODO: debug this
  rm command/suite/select/filter/geo_in_circle/no_index/north_east.test
  # Float32 value format is different.
  rm command/suite/tokenizers/document_vector_tf_idf/alphabet.test
  rm command/suite/tokenizers/document_vector_tf_idf/reindex.test
  rm command/suite/tokenizers/document_vector_tf_idf/token_column.test
  rm command/suite/tokenizers/document_vector_tf_idf/token_column_different_lexicon.test
  rm command/suite/tokenizers/document_vector_bm25/alphabet.test
  rm command/suite/tokenizers/document_vector_bm25/normalize_false.test
  rm command/suite/tokenizers/document_vector_bm25/reindex.test
elif [ "${architecture}" = "arm64" ]; then
  # Float32 value format is different.
  rm command/suite/tokenizers/document_vector_bm25/alphabet.test
  rm command/suite/tokenizers/document_vector_bm25/reindex.test
  rm command/suite/tokenizers/document_vector_bm25/token_column.test
  rm command/suite/tokenizers/document_vector_bm25/token_column_different_lexicon.test
fi

apt install -V -y \
  gcc \
  make \
  ruby-dev
MAKEFLAGS=-j$(nproc) gem install grntest

if groonga --version | grep -q apache-arrow; then
  apt install -V -y \
    g++ \
    libre2-dev
  MAKEFLAGS=-j$(nproc) gem install red-arrow
fi

export TZ=Asia/Tokyo

grntest_options=()
grntest_options+=(--base-directory=command)
grntest_options+=(--n-retries=2)
grntest_options+=(--reporter=mark)
grntest_options+=(command/suite)
# There are some problems for running arm64 tests:
#   * Too long test time because of QEMU
#   * Unknown crash with bullseye
if [ "${architecture}" != "arm64" ]; then
  grntest "${grntest_options[@]}"
  # Run only one job to reduce CI time
  if [ "${code_name}" == "bookworm" ]; then
    grntest "${grntest_options[@]}" --interface http
  fi
fi
