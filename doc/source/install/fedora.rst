.. -*- rst -*-

.. highlightlang:: none

Fedora
======

This section describes how to install Groonga related RPM packages on
Fedora. You can install them by ``yum``.

.. note::

  Since Groonga 3.0.2 release, Groonga related RPM pakcages are in the
  official Fedora yum repository (Fedora 18). So you can use them
  instead of the Groonga yum repository now. There is some exceptions
  to use the Groonga yum repository because mecab dictionaries
  (mecab-ipadic or mecab-jumandic) are provided by the Groonga yum
  repository.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

Fedora 21
---------

Install::

  % sudo yum install -y groonga

Note that additional packages such as ``mecab-dic`` and ``mecab-jumandic`` packages require to install
``groonga-release`` package which provides the Groonga yum repository beforehand::

  % sudo rpm -ivh https://packages.groonga.org/fedora/groonga-release-1.1.0-1.noarch.rpm
  % sudo yum update

.. include:: server-use.inc

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo yum install -y groonga-tokenizer-mecab

Then install MeCab dictionary. (mecab-ipadic or mecab-jumandic)

Install IPA dictionary::

  % sudo yum install -y mecab-ipadic

Or install Juman dictionary::

  % sudo yum install -y mecab-jumandic

There is a package that provides `Munin
<http://munin-monitoring.org/>`_ plugins. If you want to monitor
Groonga status by Munin, install groonga-munin-plugins package.

Install groonga-munin-plugins package::

  % sudo yum install -y groonga-munin-plugins

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo yum install -y install groonga-normalizer-mysql

Build from source
-----------------

Install required packages to build Groonga::

  % sudo yum install -y wget tar gcc-c++ make mecab-devel libedit-devel

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-11.0.0.tar.gz
  % tar xvzf groonga-11.0.0.tar.gz
  % cd groonga-11.0.0

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(grep '^processor' /proc/cpuinfo | wc -l)

Install::

  % sudo make install
