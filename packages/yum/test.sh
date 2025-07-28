#!/bin/bash

set -exu

os=$(cut -d: -f4 /etc/system-release-cpe)
case ${os} in
  amazon)
    os=amazon-linux
    version=$(cut -d: -f6 /etc/system-release-cpe)
    ;;
  *) # For AlmaLinux
    version=$(cut -d: -f5 /etc/system-release-cpe | sed -e 's/\.[0-9]$//')
    ;;
esac

case ${os} in
  amazon-linux)
    DNF="dnf"
    ;;
  *)
    case ${version} in
      8)
        DNF="dnf --enablerepo=powertools"
        ;;
      *)
        DNF="dnf --enablerepo=crb"
        ;;
    esac
    ;;
esac

${DNF} install -y \
  https://packages.apache.org/artifactory/arrow/${os}/${version}/apache-arrow-release-latest.rpm \
  https://packages.groonga.org/${os}/${version}/groonga-release-latest.noarch.rpm

repositories_dir=/groonga/packages/yum/repositories
${DNF} install -y \
  ${repositories_dir}/${os}/${version}/$(arch)/Packages/*.rpm

groonga --version
if ! groonga --version | grep -q apache-arrow; then
  echo "Apache Arrow isn't enabled"
  exit 1
fi

run_test=yes
case $(arch) in
  aarch64)
    run_test=no
    ;;
esac

if [ "${run_test}" = "yes" ]; then
  mkdir -p /test
  cd /test
  cp -a /groonga/test/command ./

  case ${version} in
    8)
      ${DNF} module disable -y ruby
      ${DNF} module enable -y ruby:3.1
      ${DNF} install -y ruby-devel
      ;;
    *)
      ${DNF} install -y ruby-devel
      ;;
  esac

  if [ "${os}-${version}-${arch}" == "almalinux-10-x86_64" ]; then
    # Float32 value format is different.
    rm command/suite/tokenizers/document_vector_bm25/alphabet.test
    rm command/suite/tokenizers/document_vector_bm25/reindex.test
    rm command/suite/tokenizers/document_vector_bm25/token_column.test
    rm command/suite/tokenizers/document_vector_bm25/token_column_different_lexicon.test
  fi

  ${DNF} install -y \
    gcc \
    make
  if [ ${os} != "amazon-linux" ]; then
    ${DNF} install -y redhat-rpm-config
  fi
  gem install rubygems-requirements-system
  MAKEFLAGS=-j$(nproc) gem install grntest

  export TZ=Asia/Tokyo

  grntest_options=()
  grntest_options+=(--base-directory=command)
  grntest_options+=(--n-retries=2)
  grntest_options+=(--reporter=mark)
  grntest_options+=(command/suite)

  grntest "${grntest_options[@]}"
  # Run only one job to reduce CI time
  if [ "${os}-${version}" == "almalinux-9" ]; then
    grntest "${grntest_options[@]}" --interface http
  fi
fi

# Should not block system update
${DNF} update -y
