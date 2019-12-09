#!/bin/sh

./version-gen.sh

# AX_CXX_COMPILE_STDCXX macro is required to check compiler option correctly
if which dpkg > /dev/null 2>&1; then
  if ! dpkg -s autoconf-archive > /dev/null; then
    echo "ERROR: autoconf-archive package is not installed yet."
    exit 1
  fi
elif which rpm > /dev/null 2>&1; then
  if yum info autoconf-archive > /dev/null 2>&1 && \
       ! rpm -q autoconf-archive > /dev/null; then
    echo "ERROR: autoconf-archive package is not installed yet."
    exit 1
  fi
elif which brew > /dev/null 2>&1; then
  if ! brew list autoconf-archive > /dev/null 2>&1; then
    echo "ERROR: autoconf-archive formula is not installed yet."
    exit 1
  fi
elif [ -x /usr/local/sbin/pkg ]; then
  if ! /usr/local/sbin/pkg info autoconf-archive > /dev/null 2>&1; then
    echo "ERROR: autoconf-archive package is not installed yet."
    exit 1
  fi
fi

case $(uname -s) in
  Darwin)
    homebrew_aclocal=/usr/local/share/aclocal
    if [ -d $homebrew_aclocal ]; then
      ACLOCAL_PATH="$ACLOCAL_PATH $homebrew_aclocal"
    fi
    gettext_prefix=/usr/local/Cellar/gettext
    if [ -d $gettext_prefix ]; then
      gettext_aclocal=$(ls $gettext_prefix/*/share/aclocal | \
                          gsort --version-sort | \
                          tail -n 1)
      if [ -d $gettext_aclocal ]; then
        ACLOCAL_PATH="$ACLOCAL_PATH $gettext_aclocal"
      fi
    fi
    ;;
  FreeBSD)
    ACLOCAL_PATH="$ACLOCAL_PATH /usr/local/share/aclocal/"
    ;;
esac

if [ ! -e vendor/mruby-source/.git ]; then
  rm -rf vendor/mruby-source
fi
git submodule update --init --recursive

mkdir -p m4

${AUTORECONF:-autoreconf} --force --install "$@"
