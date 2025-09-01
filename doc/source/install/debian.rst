.. -*- rst -*-

Debian GNU/Linux
================

This section describes how to install Groonga related deb packages on
Debian GNU/Linux. You can install them by ``apt``.

.. _trixie:

trixie
------

.. versionadded:: 15.1.5

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

.. include:: server-use.rst

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

bookworm
--------

.. versionadded:: 13.0.2

bookworm's install procedure is same trixie's install procedure.
Please refer :ref:`trixie`.

Build from source
-----------------

Build from source is for developers.

See :doc:`/install/cmake` .
