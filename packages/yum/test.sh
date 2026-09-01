#!/bin/bash

set -exu

os=$(cut -d: -f4 /etc/system-release-cpe)
case ${os} in
  centos)
    version=$(cut -d: -f5 /etc/system-release-cpe)
    ;;
  linux)
    os=oracle-linux
    version=$(cut -d: -f5 /etc/system-release-cpe)
    ;;
  *) # For AlmaLinux
    version=$(cut -d: -f5 /etc/system-release-cpe | sed -e 's/\.[0-9]$//')
    ;;
esac

case ${os} in
  oracle-linux)
    DNF="dnf --enablerepo=ol${version}_codeready_builder"
    ${DNF} install -y \
      https://apache.jfrog.io/artifactory/arrow/almalinux/$(cut -d: -f5 /etc/system-release-cpe | cut -d. -f1)/apache-arrow-release-latest.rpm
    ;;
  *)
    case ${version} in
      7)
        DNF=yum
        ;;
      8)
        DNF="dnf --enablerepo=powertools"
        ${DNF} install -y \
          https://packages.apache.org/artifactory/arrow/${os}/${version}/apache-arrow-release-latest.rpm
        ${DNF} install -y \
          arrow-devel-16.1.0-1.el8.x86_64 \
          arrow16-libs-16.1.0-1.el8.x86_64
        ;;
      *)
        DNF="dnf --enablerepo=crb"
        ${DNF} install -y \
          https://apache.jfrog.io/artifactory/arrow/${os}/${version}/apache-arrow-release-latest.rpm
        ;;
    esac
    ;;
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
    7)
      ${DNF} install -y centos-release-scl-rh
      ${DNF} install -y rh-ruby30-ruby-devel
      set +u
      . /opt/rh/rh-ruby30/enable
      set -u
      ;;
    8)
      ${DNF} module disable -y ruby
      ${DNF} module enable -y ruby:3.1
      ${DNF} install -y ruby-devel
      ;;
    *)
      ${DNF} install -y ruby-devel
      ;;
  esac

  ${DNF} install -y \
    gcc \
    make \
    redhat-rpm-config

  cat > Gemfile <<'EOF'
source "https://rubygems.org"

gem "json", "2.7.2"
gem "groonga-command-parser", "1.1.4"
gem "grntest", "1.7.5"
EOF
  MAKEFLAGS=-j$(nproc) bundle install

  export TZ=Asia/Tokyo

  grntest_options=()
  grntest_options+=(--base-directory=command)
  grntest_options+=(--n-retries=3)
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
