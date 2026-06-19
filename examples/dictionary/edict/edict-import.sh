#!/bin/sh

set -eu

base_dir=$(dirname $0)

if [ $# -ne 1 -a $# -ne 2 ]; then
  echo "usage: $0 db_path [.../edict2.gz]"
  exit 1
fi

db_path="$1"
if [ $# -eq 1 ]; then
  edict2_gz=edict2.gz
  if [ ! -f "$edict2_gz" ]; then
    wget -O "$edict2_gz" http://ftp.edrdg.org/pub/Nihongo/edict2.gz
  fi
else
  edict2_gz="$2"
fi

if type gzcat > /dev/null 2>&1; then
  zcat="gzcat"
else
  zcat="zcat"
fi

$zcat "${edict2_gz}" | \
  "${base_dir}/edict2grn.rb" | \
  groonga "${db_path}" > /dev/null
echo "edict data loaded."
