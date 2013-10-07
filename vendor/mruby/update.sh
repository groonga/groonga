#!/bin/sh

set -u
set -e
set -x

base_dir=$(dirname "$0")
top_src_dir="${base_dir}/../.."

mruby_version_file="${top_src_dir}/mruby_version"
current_mruby_version=$(cat "${mruby_version_file}")

current_mruby_dir="${base_dir}/mruby-${current_mruby_version}"

git rm -rf "${current_mruby_dir}" || :

new_mruby_clone_dir=mruby.master
rm -rf ${new_mruby_clone_dir}
git clone --depth 1 https://github.com/mruby/mruby ${new_mruby_clone_dir}
new_mruby_full_version=$(cat ${new_mruby_clone_dir}/.git/refs/heads/master)
new_mruby_version=$(echo ${new_mruby_full_version} | sed --regexp-extended -e 's/^(.{7}).*/\1/')

new_mruby_dir=../mruby-${new_mruby_version}
rm -rf ${new_mruby_dir}
mkdir -p ${new_mruby_dir}
(cd ${new_mruby_clone_dir} && git archive HEAD) | \
    tar xf - -C ${new_mruby_dir}

relative_mrblib_c=build/host/mrblib/mrblib.c
(cd ${new_mruby_clone_dir} && ${RUBY} ./minirake > /dev/null)
find ${new_mruby_clone_dir}/build/host -name mrblib.c \
    -exec mv '{}' ${new_mruby_dir}/src/ ';'
find ${new_mruby_clone_dir}/build/host -name y.tab.c \
    -exec mv '{}' ${new_mruby_dir}/src/parse.c ';'

rm -rf ${new_mruby_clone_dir}

echo -n "${new_mruby_version}" > "${mruby_version_file}"

(cd "${base_dir}" && ./update_files.sh "${new_mruby_dir}" > sources.am)
