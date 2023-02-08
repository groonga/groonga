:orphan:

How to build Groonga with CMake
===============================

This document describes how to build Groonga from source with CMake.

To get more details about installing Groonga from the source with CMake on a specific environment,
find the document for the specific environment from :doc:`/install`.

Install depended software
-------------------------

Here is depended software for GNU/Linux, UNIX and Windows.

TODO

GNU/Linux or UNIX
+++++++++++++++++

  * `CMake <http://www.cmake.org/>`_

Windows
+++++++

  * `Microsoft Visual Studio Community
    <https://visualstudio.microsoft.com/vs/community/>`_
  * `CMake <http://www.cmake.org/>`_

Download source
---------------

You can get the latest source from `packages.groonga.org <https://packages.groonga.org/source/groonga>`_.

GNU/Linux or UNIX
+++++++++++++++++

.. code-block:: console

   $ wget https://packages.groonga.org/source/groonga/groonga-13.0.0.tar.gz
   $ tar xvzf groonga-13.0.0.tar.gz

Windows
+++++++

Download the latest zipped source from packages.groonga.org.

  * https://packages.groonga.org/source/groonga/groonga-13.0.0.zip

Then extract it.

.. _cmake-run:

Run ``cmake``
-------------

You need to generate build files such as ``Makefile`` for your environment.

You can custom your build configuration by passing options to ``cmake``.

.. _cmake-options:

CMake options
+++++++++++++

This section describes important options of CMake.

``-G GENERATOR``
^^^^^^^^^^^^^^^^

Specify a generator.

The default is depending on the system.

You can check the default generator and available generators by ``cmake --help``.

.. code-block:: console

   $ cmake --help
   ...
   The following generators are available on this platform (* marks default):
     Green Hills MULTI            = Generates Green Hills MULTI files
                                   (experimental, work-in-progress).
   * Unix Makefiles               = Generates standard UNIX makefiles.
     Ninja                        = Generates build.ninja files.
     Ninja Multi-Config           = Generates build-<Config>.ninja files.
     Watcom WMake                 = Generates Watcom WMake makefiles.
     CodeBlocks - Ninja           = Generates CodeBlocks project files.
     CodeBlocks - Unix Makefiles  = Generates CodeBlocks project files.
     CodeLite - Ninja             = Generates CodeLite project files.
     CodeLite - Unix Makefiles    = Generates CodeLite project files.
     Eclipse CDT4 - Ninja         = Generates Eclipse CDT 4.0 project files.
     Eclipse CDT4 - Unix Makefiles= Generates Eclipse CDT 4.0 project files.
     Kate - Ninja                 = Generates Kate project files.
     Kate - Unix Makefiles        = Generates Kate project files.
     Sublime Text 2 - Ninja       = Generates Sublime Text 2 project files.
     Sublime Text 2 - Unix Makefiles

Here is an example how to specify ``Unix Makefiles`` on GNU/Linux or UNIX.

.. code-block:: console

   $ cmake . -G "Unix Makefiles"

Here is an example how to specify ``Visual Studio 17 2022 x64`` as a generator on Windows.
You can specify a target platform name (architecture) with the ``-A`` option.

.. code-block:: pwsh-session

   > cmake . -G "Visual Studio 17 2022" -A x64

``-DCMAKE_INSTALL_PREFIX``
^^^^^^^^^^^^^^^^^^^^^^^^^^

Specify a directory to install Groonga.

The default is depending on the system, e.g. ``/usr/local`` or ``C:/Program Files/groonga``.

Here is an example how to specify ``/tmp/local/`` as an install directory on GNU/Linux or UNIX.

.. code-block:: console

   $ cmake . -DCMAKE_INSTALL_PREFIX="/tmp/local/"

Here is an example how to specify ``C:\Groonga`` as an install directory on Windows.

.. code-block:: 

   > cmake . -DCMAKE_INSTALL_PREFIX="C:\Groonga"

``-DGRN_WITH_MRUBY``
^^^^^^^^^^^^^^^^^^^^

Enables mruby support.

You can use the :doc:`/reference/sharding` plugin and :doc:`/reference/commands/ruby_eval` 
with the mruby support.

The default is ``OFF``.

Groonga builds bundled mruby if the mruby support is enabled. In order to build mruby, you must 
install some requierd libraries. See the `mruby compile guide <https://github.com/mruby/mruby/blob/master/doc/guides/compile.md>`_ 
for more details.

Here is an example how to enable the mruby support.

.. code-block:: console

   $ cmake . -DGRN_WITH_MRUBY=ON

``-DGRN_WITH_DEBUG``
^^^^^^^^^^^^^^^^^^^^

Enables debug options for C/C++ compiler. It's useful for debugging on debugger such as GDB and LLDB.

The default is ``OFF``.

Here is an example how to enable debug options.

.. code-block:: console

   $ cmake . -DGRN_WITH_DEBUG=ON

``-DGRN_WITH_APACHE_ARROW``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Enables Apache Arrow support.

In addition to using Apache Arrow IPC streaming format output, you can also use multithreading processing that is used in :ref:`select-n-workers` 
and :doc:`/reference/functions/query_parallel_or` with the Apache Arrow support.   

The default is ``OFF``.

You can install Apache Arrow following to `the official installation procedure <https://arrow.apache.org/install/>`_. 

Here is an example how to enable the Apache Arrow support.

.. code-block:: console

   $ cmake . -DGRN_WITH_APACHE_ARROW=ON

.. note::

   If you install Apache Arrow manually, you need to use the :ref:`cmake-options-cmake-prefix-path` option.

.. _cmake-options-cmake-prefix-path:

``-DCMAKE_PREFIX_PATH=PATHS``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Adds search paths for ``.cmake`` files.

You can specify multiple path separating them with ``:`` on GNU/Linux or UNIX, ``;`` on Windows.

In case of using libraries installed via a package manager, you do not need to specify this 
parameter. It is because ``.cmake`` files for those libraries are in the default search paths of CMake.

In case of using libraries installed in non-system directories such as ``/usr``, you need to specify ``.cmake`` file paths of those libraries by this parameter.

Here is an example how to specify a ``.cmake`` file path for ``/tmp/local/lib/cmake/Arrow/ArrowConfig.cmake`` on GNU/Linux or UNIX.

.. code-block:: console

   $ cmake . -DCMAKE_PREFIX_PATH="/tmp/local"

Here is an example how to specify a ``.cmake`` file path for ``C:\arrow\lib\cmake\Arrow\ArrowConfig.cmake`` on Windows.

.. code-block:: pwsh-session

   > cmake . -DCMAKE_PREFIX_PATH="C:\arrow"

.. _cmake-build-and-install:

Build and install Groonga
-------------------------

Now, you can build Groonga.

GNU/Linux or UNIX
+++++++++++++++++

You can use ``make``.

Here is a command line to build and install Groonga by ``make``.

.. code-block:: console

   $ make -j$(nproc || PATH="/sbin:$PATH" sysctl -n hw.ncpu) > /dev/null
   $ sudo make install

We recommend to add ``> /deb/null`` to ``make`` in order to see only warning and error messages.
Developers shouldn't add new warnings and errors in new commit.

Windows
+++++++

You can use Visual Studio or ``cmake --build``.

Here is a command line to build and install Groonga by ``cmake --build``.

.. code-block:: pwsh-session

   > cmake --build . --config Release
   > cmake --build . --config Release --target Install

You should specify ``--config Debug`` instead of ``--config Release`` when debugging.