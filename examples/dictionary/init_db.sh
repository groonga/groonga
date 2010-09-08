#!/bin/sh

if [ 1 != $# ]; then
  echo "usage: $0 db_path"
  exit 1
fi

if groonga --file ddl.grn -n $1 > /dev/null; then
  echo "db initialized."
fi
