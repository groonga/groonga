#!/bin/sh

if [ 2 != $# ]; then
  echo "usage: $0 db_path gene.txt_path"
  exit 1
fi

if cat $2 | ./gene2grn.rb | groonga $1 > /dev/null; then
  echo "gene95 data loaded."
fi
