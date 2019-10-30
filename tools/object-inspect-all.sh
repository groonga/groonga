#!/bin/bash

if [ $# -ne 1 ]; then
  echo "Usage: $0 DATABASE_PATH"
  echo " e.g.: $0 ~/database/db"
  exit 1
fi

readonly DATABASE_PATH="$1"

groonga "${DATABASE_PATH}" \
        schema | \
  jq ".[1].tables[].name" | \
  while read table; do \
    groonga \
      "${DATABASE_PATH}" \
      object_inspect \
      "${table}" \
      --output_pretty yes
  done

groonga "${DATABASE_PATH}" \
        schema | \
  jq ".[1].tables[].columns[].full_name" | \
  while read column; do \
    groonga \
      "${DATABASE_PATH}" \
      object_inspect \
      "${column}" \
      --output_pretty yes
  done
