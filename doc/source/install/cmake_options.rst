.. -*- rst -*-

CMake options
=============

This section describes important options of CMake.

To get more details about installing Groonga from the source with CMake on a specific environment,
find the document for the specific environment from :doc:`/install`.

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

Here is an example of specifying ``Visual Studio 12 2013 Win64`` as a generator::

  > cmake . -G "Visual Studio 12 2013 Win64" 

``-DCMAKE_INSTALL_PREFIX``
^^^^^^^^^^^^^^^^^^^^^^^^^^

Specify a directory to install Groonga.

The default is depending on the system, e.g. ``/usr/local`` or ``C:/Program Files/groonga``.

Here is an example of specifying ``C:\Groonga`` as an install directory::

.. code-block:: pwsh-session

   > cmake . -DCMAKE_INSTALL_PREFIX=C:\Groonga

``-DGRN_WITH_MRUBY``
^^^^^^^^^^^^^^^^^^^^^^^

Enables mruby support.

You can use the :doc:`/reference/sharding` plugin and :doc:`/reference/commands/ruby_eval` 
with the mruby support.

The default is ``OFF``.

Groonga builds bundled mruby if the mruby support is enabled. In order to build mruby, you must 
install some requierd libraries. See the `mruby compile guide <https://github.com/mruby/mruby/blob/master/doc/guides/compile.md>`_ 
for more details.

Here is an example of enabling the mruby support::

.. code-block:: pwsh-session

   > cmake . -DGRN_WITH_MRUBY=on

``-DGRN_WITH_APACHE_ARROW``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Enables Apache Arrow support.

In addition to using Apache Arrow IPC streaming format output, you can also use multithreading processing that is used in :ref:`select-n-workers` 
and :doc:`/reference/functions/query_parallel_or` with the Apache Arrow support.   

The default is ``OFF``.

You can install Apache Arrow following to `the official installation procedure <https://arrow.apache.org/install/>`_. 

Here is an example of enabling the Apache Arrow support::

.. code-block:: pwsh-session

   > cmake . -DGRN_WITH_APACHE_ARROW=on

.. note::

   If you install Apache Arrow manually, you need to use the :ref:`cmake-options-cmake-prefix-path` option.

.. _cmake-options-cmake-prefix-path:

``-DCMAKE_PREFIX_PATH=PATHS``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Adds search paths for ``.cmake`` files.

You can specify multiple paths using ``:`` on UNIX or ``;`` on Windows to separate.

In case of using libraries installed via a package manager, you do not need to specify this 
parameter. It is because ``.cmake`` files for those libraries are in the default search paths of CMake.

In case of using manual built libraries, you need to specify ``.cmake`` file paths of those libraries by this parameter.

Here is an example of specifying a ``.cmake`` file path for ``C:\arrow\cmake\arrowConfig.cmake``::

.. code-block:: pwsh-session

   > cmake . -DCMAKE_PREFIX_PATH="C:\arrow\cmake"
