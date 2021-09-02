#!/bin/bash

set -eux

cd "$(dirname "$0")"

# TODO: Use version.sh instead?
version=$(cat base_version)
base_name=groonga-${version}
rm -rf ${base_name}
git archive --format=tar --prefix=${base_name}/ HEAD | tar xf -
submodules=()
submodules+=(vendor/mruby-source)
submodules+=(vendor/onigmo-source)
submodules+=(vendor/ngx_mruby-source)
submodules+=(vendor/groonga-log-source)
for submodule in ${submodules[@]}; do
  GIT_DIR=${submodule}/.git \
         git archive --format=tar --prefix=${base_name}/${submodule}/ HEAD | \
    tar xf -
done
tar czf ${base_name}.tar.gz ${base_name}
rm -rf ${base_name}
