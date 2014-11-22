#!/bin/sh

set -u
set -e
set -x

if [ $# != 1 ]; then
    echo "Usage: $0 VERSION"
    echo " e.g.: $0 1.2.6"
    exit 1
fi

new_nginx_version="$1"

base_dir="$(dirname "$0")"
top_src_dir="${base_dir}/.."

nginx_version_file="${top_src_dir}/nginx_version"
current_nginx_version=$(cat "${nginx_version_file}")

current_nginx_dir="${base_dir}/nginx-${current_nginx_version}"

new_nginx_base_name="nginx-${new_nginx_version}"
hg clone http://hg.nginx.org/nginx "${new_nginx_base_name}"

cd "${new_nginx_base_name}"
hg "checkout" "release-${new_nginx_version}"
rm -rf .hg .hgtags
cd -

echo "${new_nginx_version}" > "${nginx_version_file}"

git add "${new_nginx_base_name}"
git rm -rf "${current_nginx_dir}" || :
