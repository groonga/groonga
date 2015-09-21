#!/bin/sh

./version-gen.sh

case `uname -s` in
Darwin)
        homebrew_aclocal=/usr/local/share/aclocal
        if [ -d $homebrew_aclocal ]; then
          ACLOCAL_ARGS="$ACLOCAL_ARGS -I $homebrew_aclocal"
        fi
        gettext_prefix=/usr/local/Cellar/gettext
        if [ -d $gettext_prefix ]; then
          gettext_aclocal=$(ls $gettext_prefix/*/share/aclocal | \
                               sort --version-sort | \
                               tail -n 1)
          if [ -d $gettext_aclocal ]; then
            ACLOCAL_ARGS="$ACLOCAL_ARGS -I $gettext_aclocal"
          fi
        fi
	;;
FreeBSD)
	ACLOCAL_ARGS="$ACLOCAL_ARGS -I /usr/local/share/aclocal/"
	;;
esac

if [ ! -e vendor/mruby-source/.git ]; then
  rm -rf vendor/mruby-source
fi
git submodule update --init

mkdir -p m4

${AUTORECONF:-autoreconf} --force --install "$@"
