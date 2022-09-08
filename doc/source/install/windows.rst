.. -*- rst -*-
.. Groonga Project

Windows
=======

This section describes how to install Groonga on Windows. You can
install Groonga by extracting a zip package.

We distribute both 32-bit and 64-bit packages but we strongly
recommend a 64-bit package for server. You should use a 32-bit package
just only for tests or development. You will encounter an out of
memory error with a 32-bit package even if you just process medium
size data.

zip
---

For 32-bit environment, download x86 zip archive from
packages.groonga.org:

  * https://packages.groonga.org/windows/groonga/groonga-latest-x86-vs2019-with-vcruntime.zip

If we don't need Microsoft Visual C++ Runtime Library, we download from the following URL:

  * https://packages.groonga.org/windows/groonga/groonga-latest-x86-vs2019.zip

Then extract it.

For 64-bit environment, download x64 zip archive from
packages.groonga.org:

  * https://packages.groonga.org/windows/groonga/groonga-latest-x64-vs2019-with-vcruntime.zip

If we don't need Microsoft Visual C++ Runtime Library, we download from the following URL:

  * https://packages.groonga.org/windows/groonga/groonga-latest-x64-vs2019.zip

Then extract it.

You can find :doc:`/reference/executables/groonga` in ``bin`` folder.

Build from source
-----------------

You can build Groonga from the source with CMake.

See :doc:`/install/cmake`.
