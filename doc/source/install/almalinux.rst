.. -*- rst -*-

AlmaLinux
=========

This section describes how to install Groonga related RPM packages on
AlmaLinux OS. You can install them by ``dnf``.

We distribute only 64-bit packages.

.. include:: server-use.rst

AlmaLinux 10
------------

Install:

.. code-block:: console

   $ sudo dnf install -y https://apache.jfrog.io/artifactory/arrow/almalinux/10/apache-arrow-release-latest.rpm
   $ sudo dnf install -y https://packages.groonga.org/almalinux/10/groonga-release-latest.noarch.rpm
   $ sudo dnf install -y --enablerepo=epel --enablerepo=crb groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package:

.. code-block:: console

   $ sudo dnf install -y --enablerepo=epel groonga-tokenizer-mecab

If you want to use :doc:`/reference/token_filters/token_filter_stem`, install groonga-token-filter-stem package.

Install groonga-token-filter-stem package:

.. code-block:: console

   $ sudo dnf install -y groonga-token-filter-stem

TODO:

- Write here how to install groonga-normalizer-mysql package after groonga-normalizer-mysql released for AlmaLinux 10.

AlmaLinux 9
-----------

Install:

.. code-block:: console

   $ sudo dnf install -y https://apache.jfrog.io/artifactory/arrow/almalinux/9/apache-arrow-release-latest.rpm
   $ sudo dnf install -y https://packages.groonga.org/almalinux/9/groonga-release-latest.noarch.rpm
   $ sudo dnf install -y --enablerepo=epel --enablerepo=crb groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package:

.. code-block:: console

   $ sudo dnf install -y --enablerepo=epel groonga-tokenizer-mecab

If you want to use :doc:`/reference/token_filters/token_filter_stem`, install groonga-token-filter-stem package.

Install groonga-token-filter-stem package:

.. code-block:: console

   $ sudo dnf install -y groonga-token-filter-stem

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package:

.. code-block:: console

   $ sudo dnf install -y --enablerepo=epel groonga-normalizer-mysql

AlmaLinux 8
-----------

Install:

.. code-block:: console

   $ sudo dnf install -y https://packages.apache.org/artifactory/arrow/almalinux/8/apache-arrow-release-latest.rpm
   $ sudo dnf install -y https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm
   $ sudo dnf install -y --enablerepo=epel --enablerepo=powertools groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package:

.. code-block:: console

   $ sudo dnf install -y --enablerepo=epel groonga-tokenizer-mecab

If you want to use :doc:`/reference/token_filters/token_filter_stem`, install groonga-token-filter-stem package.

Install groonga-token-filter-stem package:

.. code-block:: console

   $ sudo dnf install -y groonga-token-filter-stem

There is a package that provides MySQL compatible normalizer as
a Groonga plugin.
If you want to use that one, install groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package:

.. code-block:: console

   $ sudo dnf install -y --enablerepo=epel groonga-normalizer-mysql

Build from source
-----------------

Build from source is for developers.

See :doc:`/install/cmake` .
