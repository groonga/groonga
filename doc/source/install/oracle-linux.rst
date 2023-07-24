.. -*- rst -*-

Oracle Linux
============

This section describes how to install Groonga related RPM packages on
Oracle Linux OS. You can install them by ``dnf``.

We distribute only 64-bit packages.

.. include:: server-use.inc

Oracle Linux 9
--------------

Install

.. code-block:: console

   % sudo dnf install -y https://apache.jfrog.io/artifactory/arrow/almalinux/9/apache-arrow-release-latest.rpm
   % sudo dnf install -y https://packages.groonga.org/oracle-linux/9/groonga-release-latest.noarch.rpm
   % sudo dnf install -y --enablerepo=ol9_codeready_builder groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo dnf install -y --enablerepo=epel groonga-tokenizer-mecab

Oracle Linux 8
--------------

Install::

  % sudo dnf install -y https://apache.jfrog.io/artifactory/arrow/almalinux/8/apache-arrow-release-latest.rpm
  % sudo dnf install -y https://packages.groonga.org/oracle-linux/8/groonga-release-latest.noarch.rpm
  % sudo dnf install -y --enablerepo=ol8_codeready_builder groonga

If you want to use `MeCab <https://taku910.github.io/mecab/>`_ as a
tokenizer, install groonga-tokenizer-mecab package.

Install groonga-tokenizer-mecab package::

  % sudo dnf install -y --enablerepo=epel groonga-tokenizer-mecab

Build from source
-----------------

Install required packages to build Groonga::

  % sudo dnf install -y https://packages.groonga.org/oracle-linux/8/groonga-release-latest.noarch.rpm
  % sudo dnf install -y wget gcc-c++ make mecab-devel

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-13.0.3.tar.gz
  % tar xvzf groonga-13.0.3.tar.gz
  % cd groonga-13.0.3

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(nproc)

Install::

  % sudo make install
