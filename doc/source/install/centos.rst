.. -*- rst -*-

CentOS
======

This section describes how to install Groonga related RPM packages on
CentOS. You can install them by ``yum``.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

CentOS 7
--------

Install::

  % sudo yum install -y https://packages.groonga.org/centos/7/groonga-release-latest.noarch.rpm
  % sudo yum install -y --enablerepo=epel groonga

.. include:: server-use.inc

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo yum install -y --enablerepo=epel groonga-tokenizer-mecab

There is a package that provides `Munin
<http://munin-monitoring.org/>`_ plugins. If you want to monitor
Groonga status by Munin, install groonga-munin-plugins package.

Install groonga-munin-plugins package::

  % sudo yum install -y --enablerepo=epel groonga-munin-plugins

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo yum install -y --enablerepo=epel groonga-normalizer-mysql

Build from source
-----------------

Install required packages to build Groonga::

  % sudo yum install -y wget tar gcc-c++ make mecab-devel

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-13.0.5.tar.gz
  % tar xvzf groonga-13.0.5.tar.gz
  % cd groonga-13.0.5

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(grep '^processor' /proc/cpuinfo | wc -l)

Install::

  % sudo make install
