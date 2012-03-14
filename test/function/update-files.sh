#!/bin/sh

list_paths()
{
    variable_name=$1
    echo "$variable_name = \\"
    LC_ALL=C sort | \
    sed \
      -e 's,^,\t,' \
      -e 's,$, \\,'
    echo "\t\$(NULL)"
    echo
}

find . -type f -name '*.test' | \
    sed -e 's,\./,,' | \
    sort | \
    list_paths "test_files"

find . -type f -name '*.expected' | \
    sed -e 's,\./,,' | \
    sort | \
    list_paths "expected_files"

find . -type f -name '*.grn' | \
    sed -e 's,\./,,' | \
    sort | \
    list_paths "fixture_files"
