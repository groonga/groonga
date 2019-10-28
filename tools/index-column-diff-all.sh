#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 DATABASE_PATH"
  echo " e.g.: $0 ~/database/db"
  exit 1
fi

readonly DATABASE_PATH="$1"

groonga "${DATABASE_PATH}" \
        dump \
        --dump_plugins no \
        --dump_schema no \
        --dump_records no \
        --dump_configs no | \
  awk '{ print "index_column_diff", $2, $3, "--output_pretty yes" }' | \
  while read line; do \
    echo "// ${line}"
    echo "${line}" | \
      groonga \
        --log-path groonga.log \
        --log-level debug \
        "${DATABASE_PATH}"
  done
