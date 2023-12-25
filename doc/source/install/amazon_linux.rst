.. -*- rst -*-

Amazon Linux
============

This section describes how to install Groonga related RPM packages on
Amazon Linux. You can install them by ``yum``.

Amazon Linux 2
--------------

Install::

  % sudo amazon-linux-extras install -y epel
  % sudo yum install -y https://packages.groonga.org/amazon-linux/2/groonga-release-latest.noarch.rpm
  % sudo yum install -y groonga

.. include:: server-use.rst

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo yum install -y groonga-tokenizer-mecab

There is a package that provides `Munin
<http://munin-monitoring.org/>`_ plugins. If you want to monitor
Groonga status by Munin, install groonga-munin-plugins package.

Install groonga-munin-plugins package::

  % sudo yum install -y groonga-munin-plugins

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo yum install -y groonga-normalizer-mysql

Build from source
-----------------

Install required packages to build Groonga::

  % sudo yum install -y wget tar gcc-c++ make mecab-devel

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-13.1.0.tar.gz
  % tar xvzf groonga-13.1.0.tar.gz
  % cd groonga-13.1.0

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(grep '^processor' /proc/cpuinfo | wc -l)

Install::

  % sudo make install
