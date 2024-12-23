.. -*- rst -*-

Amazon Linux
============

This section describes how to install Groonga related RPM packages on
Amazon Linux. You can install them by ``dnf``.

Amazon Linux 2023
-----------------

Install:

.. code-block:: console

   $ sudo dnf install -y https://apache.jfrog.io/artifactory/arrow/amazon-linux/$(cut -d: -f6 /etc/system-release-cpe)/apache-arrow-release-latest.rpm
   $ sudo dnf install -y https://packages.groonga.org/amazon-linux/2023/groonga-release-latest.noarch.rpm
   $ sudo dnf install -y groonga

.. include:: server-use.rst

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package:

.. code-block:: console

   $ sudo dnf install -y groonga-tokenizer-mecab

There is a package that provides MySQL compatible normalizer as a
Groonga plugin. If you want to use that one, install
groonga-normalizer-mysql package.

Install groonga-normalizer-mysql package:

.. code-block:: console

   $ sudo dnf install -y groonga-normalizer-mysql

Build from source
-----------------

Build from source is for developers.

See :doc:`/install/cmake` .
