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
  awk '{ print "object_inspect", $2 "." $3 }' | \
  while read line; do \
    echo "${line}" | \
      groonga \
        --log-path groonga.log \
        --log-level debug \
        "${DATABASE_PATH}" | \
    jq -r '.[1] | {full_name: .full_name, n_buffer_segments: .value.statistics.n_buffer_segments}'
  done
