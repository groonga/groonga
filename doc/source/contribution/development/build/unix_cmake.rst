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

Checkout Groonga from the repository
------------------------------------

Users use released source archive. But developers must build Groonga
at the repository. Because source code in the repository is the
latest.

The Groonga repository is hosted on `GitHub
<https://github.com/groonga/groonga>`_. Checkout the latest source
code from the repository::

  $ git clone --recursive git@github.com:groonga/groonga.git

Build Groonga
-------------

See :doc:`/install/cmake` for details on installing depended software and how to build with CMake.

See also
--------

  * :doc:`/contribution/development/test`
