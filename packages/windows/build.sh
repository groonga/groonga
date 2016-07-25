#!/bin/sh

LANG=C

run()
{
  "$@"
  if test $? -ne 0; then
    echo "Failed $@"
    exit 1
  fi
}

run sudo sed -i'' -e 's/httpredir/ftp.jp/g' /etc/apt/sources.list

run sudo dpkg --add-architecture i386
run sudo apt update
run sudo apt install -V -y \
    build-essential \
    devscripts \
    autoconf \
    libtool \
    cmake \
    pkg-config \
    mingw-w64 \
    wine \
    wine-binfmt \
    rsync \
    ruby

run sudo gem install rake

run cd /vagrant
. tmp/env.sh

for architecture in ${ARCHITECTURES}; do
  run rake build                                        \
      TMP_DIR="/tmp"                                    \
      VERSION="${VERSION}"                              \
      SOURCE="${SOURCE}"                                \
      DEBUG_BUILD="${DEBUG_BUILD}"                      \
      MEMORY_DEBUG_BUILD="${MEMORY_DEBUG_BUILD}"        \
      ARCHITECTURE="${architecture}"
done
