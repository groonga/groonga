#!/bin/sh

set -e
set -u

base_dir="$(cd $(dirname "$0") && pwd)/.."

mecab_version=$(cat ${base_dir}/bundled_mecab_version)
mecab_naist_jdic_version=$(cat ${base_dir}/bundled_mecab_naist_jdic_version)

mecab_base=mecab-${mecab_version}
mecab_tar_gz=${mecab_base}.tar.gz
mecab_naist_jdic_base=mecab-naist-jdic-${mecab_naist_jdic_version}
mecab_naist_jdic_tar_gz=${mecab_naist_jdic_base}.tar.gz

curl -L -J -O \
  'https://drive.google.com/uc?export=download&id=0B4y35FiV1wh7cENtOXlicTFaRUE'
rm -rf ${mecab_base}
tar xzf ${mecab_tar_gz}
rm -rf ${mecab_tar_gz}

curl -L -O \
  "http://osdn.dl.sourceforge.jp/naist-jdic/48487/${mecab_naist_jdic_tar_gz}"
rm -rf ${mecab_naist_jdic_base}
tar xzf ${mecab_naist_jdic_tar_gz}
rm -rf ${mecab_naist_jdic_tar_gz}
