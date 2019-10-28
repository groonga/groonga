#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Usage: $0 database_dump groonga_path database_path"
  echo " e.g.: $0 dump.grn /usr/local/groonga/bin/groonga ~/database/db"
  exit 1
fi

grep -w COLUMN_INDEX $1 | \
  awk '{ print "index_column_diff --table "$2" --name "$3 }' \
  > index_column_diff.query

$2 \
  --log-path groonga.log \
  --log-level debug \
  --query-log-path query.log \
  $3 \
  < index_column_diff.query \
  > index_column_diff_result.log

rm index_column_diff.query
