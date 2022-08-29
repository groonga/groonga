.. -*- rst -*-

Ubuntu
======

This section describes how to install Groonga related deb packages on
Ubuntu. You can install them by ``apt``.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

PPA (Personal Package Archive)
------------------------------

The Groonga APT repository for Ubuntu uses PPA (Personal Package
Archive) on Launchpad. You can install Groonga by APT from the PPA.

Here are supported Ubuntu versions:

  * 18.04 LTS Bionic Beaver
  * 20.04 LTS Focal Fossa
  * 22.04 LTS Jammy Jellyfish

Enable the universe repository to install Groonga::

  % sudo apt-get -y install software-properties-common
  % sudo add-apt-repository -y universe

Add the ``ppa:groonga/ppa`` PPA to your system::

  % sudo add-apt-repository -y ppa:groonga/ppa
  % sudo apt-get update

Install::

  % sudo apt-get -y install groonga

.. include:: server-use.inc

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo apt-get -y install groonga-tokenizer-mecab

If you want to use ``TokenFilterStem`` as a token filter, install
groonga-token-filter-stem package.

Install groonga-token-filter-stem package::

  % sudo apt-get -y install groonga-token-filter-stem

There is a package that provides `Munin
<http://munin-monitoring.org/>`_ plugins. If you want to monitor
Groonga status by Munin, install groonga-munin-plugins package.

Install groonga-munin-plugins package::

  % sudo apt-get -y install groonga-munin-plugins

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo apt-get -y install groonga-normalizer-mysql

Build from source
-----------------

Install required packages to build Groonga::

  % sudo apt-get -V -y install wget tar build-essential zlib1g-dev liblzo2-dev libmsgpack-dev libzmq-dev libevent-dev libmecab-dev

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-12.0.7.tar.gz
  % tar xvzf groonga-12.0.7.tar.gz
  % cd groonga-12.0.7

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(grep '^processor' /proc/cpuinfo | wc -l)

Install::

  % sudo make install
