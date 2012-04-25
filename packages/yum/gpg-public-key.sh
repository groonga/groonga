#!/bin/sh

script_base_dir=`dirname $0`

if [ $# != 1 ]; then
    echo "Usage: $0 GPG_UID"
    echo " e.g.: $0 'F10399C0'"
    exit 1
fi

GPG_UID=$1

gpg -a --export "${GPG_UID}"
