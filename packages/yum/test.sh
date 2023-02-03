#!/bin/bash

set -exu

os=$(cut -d: -f4 /etc/system-release-cpe)
case ${os} in
  amazon)
    os=amazon-linux
    version=$(cut -d: -f6 /etc/system-release-cpe)
    amazon-linux-extras install -y epel
    yum install -y ca-certificates
    ;;
  centos)
    version=$(cut -d: -f5 /etc/system-release-cpe)
    ;;
  linux)
    os=oracle-linux
    version=$(cut -d: -f5 /etc/system-release-cpe | sed -e 's/\.[0-9]$//')
    ;;
  *) # For AlmaLinux
    version=$(cut -d: -f5 /etc/system-release-cpe | sed -e 's/\.[0-9]$//')
    ;;
esac

case ${os} in
  oracle-linux)
    DNF="dnf --enablerepo=ol${version}_codeready_builder"
    ${DNF} install -y \
      https://apache.jfrog.io/artifactory/arrow/${os}/${version}/apache-arrow-release-latest.rpm
    ;;
  *)
    case ${version} in
      2)
        DNF=yum
        ;;
      7)
        DNF=yum
        ;;
      8)
        DNF="dnf --enablerepo=powertools"
        ;;
      *)
        DNF="dnf --enablerepo=crb"
        ${DNF} install -y \
          https://apache.jfrog.io/artifactory/arrow/${os}/${version}/apache-arrow-release-latest.rpm
        ;;
    esac
esac

${DNF} install -y \
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
    2)
      amazon-linux-extras install -y ruby3.0
      ${DNF} install -y ruby-devel
      ;;
    7)
      ${DNF} install -y centos-release-scl-rh
      ${DNF} install -y rh-ruby30-ruby-devel
      set +u
      . /opt/rh/rh-ruby30/enable
      set -u
      ;;
    *)
      ${DNF} install -y ruby-devel
      ;;
  esac

  ${DNF} install -y \
    gcc \
    make \
    redhat-rpm-config
  MAKEFLAGS=-j$(nproc) gem install grntest

  export TZ=Asia/Tokyo

  grntest_options=()
  grntest_options+=(--base-directory=command)
  grntest_options+=(--n-retries=3)
  grntest_options+=(--n-workers=$(nproc))
  grntest_options+=(--reporter=mark)
  grntest_options+=(command/suite)

  grntest "${grntest_options[@]}"
  grntest "${grntest_options[@]}" --interface http
  grntest "${grntest_options[@]}" --interface http --testee groonga-httpd
fi

# Should not block system update
${DNF} update -y
