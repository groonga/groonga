.. -*- rst -*-

How to build Groonga at the repository by CMake on GNU/Linux or Unix
====================================================================

This document describes how to build Groonga at the repository by
CMake on GNU/Linux or Unix.

Unix is \*BSD, Solaris, OS X and so on.

If you want to use Windows for developing Groonga, see
:doc:`windows_cmake`.

You can't choose this way if you want to release Groonga. Groonga
release system is only supported by GNU Autotools build. See
:doc:`unix_autotools` about GNU Autotools build.

Install depended software
-------------------------

TODO

  * `CMake <http://www.cmake.org/>`_
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

Run ``cmake``
-------------

You need to create ``Makefile`` for your environment.

You can custom your build configuration by passing options to
``cmake``.

Here are recommended ``cmake`` options for developers::

  % cmake . -DCMAKE_INSTALL_PREFIX=/tmp/local -DGRN_WITH_DEBUG=on -DGRN_WITH_MRUBY=on

Here are descriptions of these options:

``-DCMAKE_INSTALL_PREFIX=/tmp/local``

    It specifies that you install your Groonga into temporary
    directory. You can do "clean install" by removing
    ``/tmp/local`` directory. It'll be useful for debugging install.

``-DGRN_WITH_DEBUG=on``

    It enables debug options for C/C++ compiler. It's useful for
    debugging on debugger such as GDB and LLDB.

``-DGRN_WITH_MRUBY=on``

    It enables mruby support. The feature isn't enabled by default
    but developers should enable the feature.

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
