.. -*- rst -*-

AlmaLinux
=========

This section describes how to install Groonga related RPM packages on
AlmaLinux OS. You can install them by ``dnf``.

We distribute only 64-bit packages.

.. include:: server-use.inc

AlmaLinux 9
-----------

Install

.. code-block:: console

   % sudo dnf install -y https://apache.jfrog.io/artifactory/arrow/almalinux/9/apache-arrow-release-latest.rpm
   % sudo dnf install -y https://packages.groonga.org/almalinux/9/groonga-release-latest.noarch.rpm
   % sudo dnf install -y --enablerepo=epel --enablerepo=crb groonga

AlmaLinux 8
-----------

Install::

  % sudo dnf install -y https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm
  % sudo dnf install -y --enablerepo=epel --enablerepo=powertools groonga

Build from source
-----------------

Install required packages to build Groonga::

  % sudo dnf install -y https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm
  % sudo dnf install -y wget gcc-c++ make mecab-devel

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-12.0.8.tar.gz
  % tar xvzf groonga-12.0.8.tar.gz
  % cd groonga-12.0.8

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(nproc)

Install::

  % sudo make install
