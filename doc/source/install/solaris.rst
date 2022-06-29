.. -*- rst -*-

Oracle Solaris
==============

This section describes how to install Groonga from source on Oracle
Solaris.

Oracle Solaris 11
-----------------

Install required packages to build Groonga::

  % sudo pkg install gnu-tar gcc-45 system/header

Download source::

  % wget https://packages.groonga.org/source/groonga/groonga-12.0.5.tar.gz
  % gtar xvzf groonga-12.0.5.tar.gz
  % cd groonga-12.0.5

Configure with ``CFLAGS="-m64" CXXFLAGS="-m64"`` variables. They are
needed for building 64-bit version. To build 32-bit version, just
remove those variables. (see :ref:`source-configure` about ``configure``
options)::

  % ./configure CFLAGS="-m64" CXXFLAGS="-m64"

Build::

  % make

..
   Build with multi processes. ( ``% sudo pkg install gnu-make`` is
   required)::

      % gmake -j$(psrinfo -p)

Install::

  % sudo make install
