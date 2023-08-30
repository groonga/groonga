.. -*- rst -*-

Debian GNU/Linux
================

This section describes how to install Groonga related deb packages on
Debian GNU/Linux. You can install them by ``apt``.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

bookworm
--------

.. versionadded:: 13.0.2

Install ``groonga-apt-source``::

  % sudo apt update
  % sudo apt install -y -V ca-certificates lsb-release wget
  % wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
  % sudo apt install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb
  % sudo apt update
  % wget https://packages.groonga.org/debian/groonga-apt-source-latest-$(lsb_release --codename --short).deb
  % sudo apt install -y -V ./groonga-apt-source-latest-$(lsb_release --codename --short).deb
  % sudo apt update

Install::

  % sudo apt install -y -V groonga

.. include:: server-use.inc

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo apt install -y -V groonga-tokenizer-mecab

If you want to use ``TokenFilterStem`` as a token filter, install
groonga-token-filter-stem package.

Install groonga-token-filter-stem package::

  % sudo apt install -y -V groonga-token-filter-stem

There is a package that provides `Munin
<http://munin-monitoring.org/>`_ plugins. If you want to monitor
Groonga status by Munin, install groonga-munin-plugins package.

Install groonga-munin-plugins package::

  % sudo apt install -y -V groonga-munin-plugins

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo apt install -y -V groonga-normalizer-mysql

bullseye
--------

.. versionadded:: 11.0.2

Install ``groonga-apt-source``::

  % sudo apt update
  % sudo apt install -y -V wget
  % wget https://packages.groonga.org/debian/groonga-apt-source-latest-bullseye.deb
  % sudo apt install -y -V ./groonga-apt-source-latest-bullseye.deb
  % sudo apt update

Install::

  % sudo apt install -y -V groonga

.. include:: server-use.inc

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo apt install -y -V groonga-tokenizer-mecab

If you want to use ``TokenFilterStem`` as a token filter, install
groonga-token-filter-stem package.

Install groonga-token-filter-stem package::

  % sudo apt install -y -V groonga-token-filter-stem

There is a package that provides `Munin
<http://munin-monitoring.org/>`_ plugins. If you want to monitor
Groonga status by Munin, install groonga-munin-plugins package.

Install groonga-munin-plugins package::

  % sudo apt install -y -V groonga-munin-plugins

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo apt install -y -V groonga-normalizer-mysql

Build from source
-----------------

Install required packages to build Groonga for Debian bullseye::

  % sudo apt install -y -V build-essential pkg-config zlib1g-dev libmsgpack-dev libzmq3-dev libevent-dev libmecab-dev liblz4-dev

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-13.0.6.tar.gz
  % tar xvzf groonga-13.0.6.tar.gz
  % cd groonga-13.0.6

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(nproc)

Install::

  % sudo make install
