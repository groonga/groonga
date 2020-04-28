.. -*- rst -*-
.. Groonga Project

.. highlightlang:: none

Windows
=======

This section describes how to install Groonga on Windows. You can
install Groonga by extracting a zip package or running an installer.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

Installer
---------

For 32-bit environment, download x86 executable binary from
packages.groonga.org:

  * https://packages.groonga.org/windows/groonga/groonga-10.0.2-x86.exe

Then run it.

For 64-bit environment, download x64 executable binary from
packages.goronga.org:

  * https://packages.groonga.org/windows/groonga/groonga-10.0.2-x64.exe

Then run it.

Use command prompt in start menu to run
:doc:`/reference/executables/groonga`.

zip
---

For 32-bit environment, download x86 zip archive from
packages.groonga.org:

  * https://packages.groonga.org/windows/groonga/groonga-10.0.2-x86.zip

Then extract it.

For 64-bit environment, download x64 zip archive from
packages.groonga.org:

  * https://packages.groonga.org/windows/groonga/groonga-10.0.2-x64.zip

Then extract it.

You can find :doc:`/reference/executables/groonga` in ``bin`` folder.

Build from source
-----------------

First, you need to install required tools for building Groonga on
Windows. Here are required tools:

  * `Microsoft Visual Studio Express 2013 for Windows Desktop
    <https://www.visualstudio.com/downloads/#d-2013-express>`_
  * `CMake <http://www.cmake.org/>`_

Download zipped source from packages.groonga.org:

  * https://packages.groonga.org/source/groonga/groonga-10.0.2.zip

Then extract it.

Move to the Groonga's source folder::

  > cd c:\Users\%USERNAME%\Downloads\groonga-10.0.2

Configure by ``cmake``. The following commnad line is for 64-bit
version. To build 32-bit version, use ``-G "Visual Studio 12 2013"``
parameter instead::

  groonga-10.0.2> cmake . -G "Visual Studio 12 2013 Win64" -DCMAKE_INSTALL_PREFIX=C:\Groonga

Build::

  groonga-10.0.2> cmake --build . --config Release

Install::

  groonga-10.0.2> cmake --build . --config Release --target Install

After the above steps, :doc:`/reference/executables/groonga` is found at
``c:\Groonga\bin\groonga.exe``.
