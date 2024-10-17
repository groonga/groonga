.. -*- rst -*-

AlmaLinux
=========

This section describes how to install Groonga related RPM packages on
AlmaLinux OS. You can install them by ``dnf``.

We distribute only 64-bit packages.

.. include:: server-use.rst

AlmaLinux 9
-----------

Install

.. code-block:: console

   % sudo dnf install -y https://apache.jfrog.io/artifactory/arrow/almalinux/9/apache-arrow-release-latest.rpm
   % sudo dnf install -y https://packages.groonga.org/almalinux/9/groonga-release-latest.noarch.rpm
   % sudo dnf install -y --enablerepo=epel --enablerepo=crb groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo dnf install -y --enablerepo=epel groonga-tokenizer-mecab

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo dnf install -y --enablerepo=epel groonga-normalizer-mysql

AlmaLinux 8
-----------

Install::

  % sudo dnf install -y https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm
  % sudo dnf install -y --enablerepo=epel --enablerepo=powertools groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo dnf install -y --enablerepo=epel groonga-tokenizer-mecab

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package::

  % sudo dnf install -y --enablerepo=epel groonga-normalizer-mysql

Build from source
-----------------

Build from source is for developers.

See :doc:`/install/cmake` .
