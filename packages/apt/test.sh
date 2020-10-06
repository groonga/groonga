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
  rm command/suite/tokenizers/document_vector_bm25/alphabet.test
  rm command/suite/tokenizers/document_vector_bm25/normalize_false.test
  rm command/suite/tokenizers/document_vector_bm25/reindex.test
fi

if ! apt info libxxhash-dev; then
  rm command/suite/select/drilldowns/keys/multiple_large.test
fi

apt install -V -y \
  gcc \
  make \
  ruby-dev
gem install grntest

export TZ=Asia/Tokyo

grntest_options=()
grntest_options+=(--base-directory=command)
grntest_options+=(--n-retries=3)
grntest_options+=(--n-workers=$(nproc))
grntest_options+=(--reporter=mark)
grntest_options+=(command/suite)
grntest "${grntest_options[@]}"
grntest "${grntest_options[@]}" --interface http
grntest "${grntest_options[@]}" --interface http --testee groonga-httpd
