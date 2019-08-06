#!/bin/sh

./version-gen.sh

case `uname -s` in
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
git submodule update --init

mkdir -p m4

# ax_cxx_compile_stdcxx macro is required to check compiler option correctly
if [ -x /usr/bin/dpkg -o -x /bin/dpkg ]; then
  if ! dpkg -s autoconf-archive > /dev/null; then
    echo "ERROR: autoconf-archive package is not installed yet."
    exit 1
  fi
elif [ -x /usr/bin/rpm -o -x /bin/rpm ]; then
  if ! rpm -q autoconf-archive > /dev/null; then
    echo "ERROR: autoconf-archive package is not installed yet."
    exit 1
  fi
elif [ ! -f /usr/share/aclocal/ax_cxx_compile_stdcxx_11.m4 -a \
       ! -f /usr/local/share/aclocal/ax_cxx_compile_stdcxx_11.m4 ]; then
  echo "ERROR: autoconf-archive is not installed yet."
  exit 1
fi

${AUTORECONF:-autoreconf} --force --install "$@"
