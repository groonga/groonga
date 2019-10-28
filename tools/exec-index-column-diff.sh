#!/bin/bash

if [ $# -ne 3 ]; then
  echo "Usage: $0 database_dump groonga_path database_path"
  echo " e.g.: $0 dump.grn /usr/local/groonga/bin/groonga ~/database/db"
  exit 1
fi

readonly DUMP_FILE=$1
readonly GROONGA_PATH=$2
readonly DATABASE_PATH=$3

grep -w COLUMN_INDEX $DUMP_FILE | \
  awk '{ print "index_column_diff --table "$2" --name "$3 }' \
  > index_column_diff.query

$GROONGA_PATH \
  --log-path groonga.log \
  --log-level debug \
  --query-log-path query.log \
  $DATABASE_PATH \
  < index_column_diff.query \
  > index_column_diff_result.log

rm index_column_diff.query
