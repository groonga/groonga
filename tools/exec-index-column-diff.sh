#!/bin/bash

grep -w COLUMN_INDEX $1 | awk '{ print "index_column_diff --table "$2" --name "$3 }' > index_column_diff.query

$2 --log-path groonga.log --log-level debug --query-log-path query.log $3 < index_column_diff.query > index_column_diff_result.log

rm index_column_diff.query
