.. -*- rst -*-

AlmaLinux
=========

This section describes how to install Groonga related RPM packages on
AlmaLinux OS. You can install them by ``dnf``.

We distribute only 64-bit packages.

AlmaLinux 8
-----------

Install::

  % sudo dnf install -y https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm
  % sudo dnf install -y --enablerepo=epel --enablerepo=powertools groonga

.. include:: server-use.inc

Build from source
-----------------

Install required packages to build Groonga::

  % sudo dnf install -y https://packages.groonga.org/almalinux/8/groonga-release-latest.noarch.rpm
  % sudo dnf install -y wget gcc-c++ make mecab-devel

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-12.0.3.tar.gz
  % tar xvzf groonga-12.0.3.tar.gz
  % cd groonga-12.0.3

Configure (see :ref:`source-configure` about ``configure`` options)::

  % ./configure

Build::

  % make -j$(nproc)

Install::

  % sudo make install
