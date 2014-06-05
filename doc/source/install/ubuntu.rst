.. -*- rst -*-

.. highlightlang:: none

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

  * 12.04 LTS Precise Pangolin
  * 13.10 Saucy Salamander
  * 14.04 LTS Trusty Tahr

Enable the universe repository to install Groonga::

  % sudo apt-get -y install software-properties-common
  % sudo add-apt-repository -y universe

Add the ``ppa:groonga/ppa`` PPA to your system::

  % sudo add-apt-repository -y ppa:groonga/ppa
  % sudo apt-get update

Install::

  % sudo apt-get -y install groonga

.. include:: server-use.inc

If you want to use `MeCab <http://mecab.sourceforge.net/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo apt-get -y install groonga-tokenizer-mecab

There is a package that provides `Munin
<http://munin-monitoring.org/>`_ plugins. If you want to monitor
groonga status by Munin, install groonga-munin-plugins package.

Install groonga-munin-plugins package::

  % sudo apt-get -y install groonga-munin-plugins

There is a package that provides MySQL compatible normalizer as
Groonga plugins.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo apt-get -y install groonga-normalizer-mysql

Build from source
-----------------

Install required packages to build Groonga::

  % sudo apt-get -V -y install wget tar build-essential zlib1g-dev liblzo2-dev libmsgpack-dev libzmq-dev libevent-dev libmecab-dev

Download source::

  % wget http://packages.groonga.org/source/groonga/groonga-4.0.2.tar.gz
  % tar xvzf groonga-4.0.2.tar.gz
  % cd groonga-4.0.2

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(grep '^processor' /proc/cpuinfo | wc -l)

Install::

  % sudo make install
