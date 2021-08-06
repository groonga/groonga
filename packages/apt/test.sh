#!/bin/bash

set -exu

apt update
apt install -V -y lsb-release wget

code_name=$(lsb_release --codename --short)
architecture=$(dpkg --print-architecture)

wget \
  https://packages.groonga.org/debian/groonga-apt-source-latest-${code_name}.deb
apt install -V -y ./groonga-apt-source-latest-${code_name}.deb
apt update

repositories_dir=/groonga/packages/apt/repositories
apt install -V -y \
  ${repositories_dir}/debian/pool/${code_name}/main/*/*/*_{${architecture},all}.deb

groonga --version

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

# libxxhash-dev 0.8.0 or later is required.
rm command/suite/select/drilldowns/keys/multiple_large.test

apt install -V -y \
  gcc \
  make \
  ruby-dev
MAKEFLAGS=-j$(nproc) gem install grntest

if groonga --version | grep -q apache-arrow; then
  apt install -V -y \
    g++
  MAKEFLAGS=-j$(nproc) gem install red-arrow
fi

export TZ=Asia/Tokyo

grntest_options=()
grntest_options+=(--base-directory=command)
grntest_options+=(--n-retries=2)
grntest_options+=(--n-workers=$(nproc))
grntest_options+=(--reporter=mark)
grntest_options+=(command/suite)
grntest "${grntest_options[@]}"
if [ "${architecture}" != "arm64" ]; then
  grntest "${grntest_options[@]}" --interface http
  grntest "${grntest_options[@]}" --interface http --testee groonga-httpd
fi
