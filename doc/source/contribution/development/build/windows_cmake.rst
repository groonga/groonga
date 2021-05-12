.. -*- rst -*-

How to build Groonga at the repository by CMake on Windows
==========================================================

This document describes how to build Groonga at the repository by
CMake on Windows.

If you want to use GNU/Linux or Unix for developing Groonga, see
:doc:`unix_cmake`.

Unix is \*BSD, Solaris, OS X and so on.

Install depended software
-------------------------

  * `Microsoft Visual Studio Express 2013 for Windows Desktop
    <https://www.visualstudio.com/downloads/#d-2013-express>`_
  * `CMake <http://www.cmake.org/>`_
  * `Ruby <https://www.ruby-lang.org/>`_

    * `RubyInstaller for Windows <http://rubyinstaller.org/>`_ is
      recommended.

  * `Git <https://git-scm.com/>`_: There are some Git clients for
    Windows. For example:

    * `The official Git package <https://git-scm.com/download/win>`_
    * `TortoiseGit <https://tortoisegit.org/>`_
    * `Git for Windows <https://git-for-windows.github.io/>`_
    * `GitHub Desktop <https://desktop.github.com/>`_

Checkout Groonga from the repository
------------------------------------

Users use released source archive. But developers must build Groonga
at the repository. Because source code in the repository is the
latest.

The Groonga repository is hosted on `GitHub
<https://github.com/groonga/groonga>`_. Checkout the latest source
code from the repository::

  > git clone --recursive git@github.com:groonga/groonga.git

Run ``cmake``
-------------

You need to create ``Makefile`` for your environment.

You can custom your build configuration by passing options to
``cmake``.

You must to pass ``-G`` option. Here are available ``-G`` value:

  * ``"Visual Studio 12 2013"``: For 32bit build.
  * ``"Visual Studio 12 2013 Win64"``: For 64bit build.

Here are recommended ``cmake`` options for developers::

  > cmake . -G "Visual Studio 12 2013 Win64" -DCMAKE_INSTALL_PREFIX=C:\Groonga -DGRN_WITH_MRUBY=on

Here are descriptions of these options:

``-G "Visual Studio 12 2013 Win64"``

``-DCMAKE_INSTALL_PREFIX=C:\Groonga``

    It specifies that you install your Groonga into ``C:\\Groonga``
    folder.

``-DGRN_WITH_MRUBY=on``

    It enables mruby support. The feature isn't enabled by default
    but developers should enable the feature.

Build Groonga
-------------

Now, you can build Groonga.

You can use Visual Studio or ``cmake --build``.

Here is a command line to build Groonga by ``cmake --build``::

  > cmake --build . --config Debug

See also
--------

  * :doc:`/contribution/development/test`
