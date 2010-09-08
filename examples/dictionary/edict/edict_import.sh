#!/bin/sh

if [ 2 != $# ]; then
  echo "usage: $0 db_path edict.gz_path"
  exit 1
fi

if zcat $2 | ./edict2grn.rb | groonga $1 > /dev/null; then
  echo "edict data loaded."
fi
