.. -*- rst -*-

How to build Groonga at the repository by GNU Autotools
=======================================================

This document describes how to build Groonga at the repository by GNU
Autotools.

You can't choose this way if you develop Groonga on Windows. If you
want to use Windows for developing Groonga, see :doc:`windows_cmake`.

Install depended software
-------------------------

TODO

  * `Autoconf <http://www.gnu.org/software/autoconf/>`_
  * `Automake <http://www.gnu.org/software/automake/>`_
  * `GNU Libtool <http://www.gnu.org/software/libtool/>`_
  * `Ruby <https://www.ruby-lang.org/>`_
  * `Git <https://git-scm.com/>`_
  * `Cutter <http://cutter.sourceforge.net/>`_
  * ...

Checkout Groonga from the repository
------------------------------------

Users use released source archive. But developers must build Groonga
at the repository. Because source code in the repository is the
latest.

The Groonga repository is hosted on `GitHub
<https://github.com/groonga/groonga>`_. Checkout the latest source
code from the repository::

  % git clone --recursive git@github.com:groonga/groonga.git

Create ``configure``
--------------------

You need to create ``configure``. ``configure`` is included in source
archive but not included in the repository.

``configure`` is a build tool that detects your system and generates
build configurations for your environment.

Run ``autogen.sh`` to create ``configure``::

  % ./autogen.sh

Run ``configure``
-----------------

You can custom your build configuration by passing options to
``configure``.

Here are recommended ``configure`` options for developers::

  % ./configure --prefix=/tmp/local --enable-debug --enable-mruby --with-ruby

Here are descriptions of these options:

``--prefix=/tmp/local``
    It specifies that you install your Groonga into temporary
    directory. You can do "clean install" by removing
    ``/tmp/local`` directory. It'll be useful for debugging install.

``--enable-debug``
    It enables debug options for C/C++ compiler. It's useful for
    debugging on debugger such as GDB and LLDB.

``--eanble-mruby``
    It enables mruby support. The feature isn't enabled by default
    but developers should enable the feature.

``--with-ruby``
    It's needed for ``--enable-mruby`` and running functional tests.

Run ``make``
------------

Now, you can build Groonga.

Here is a recommended ``make`` command line for developers::

  % make -j8 > /dev/null

``-j8`` decreases build time. It enables parallel build. If you have 8
or more CPU cores, you can increase ``8`` to decreases more build
time.

You can just see only warning and error messages by ``>
/dev/null``. Developers shouldn't add new warnings and errors in new
commit.

See also
--------

  * :doc:`/contribution/development/test`
