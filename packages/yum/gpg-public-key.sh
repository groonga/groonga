#!/bin/sh

script_base_dir=`dirname $0`

gpg -a --export `$script_base_dir/gpg-uid.sh`
