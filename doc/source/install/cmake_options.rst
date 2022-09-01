.. -*- rst -*-

CMake options
=============

This section describes important CMake options.

To get more details about installing Groonga from the source on a specific environment 
by ``cmake``, find the document for the specific environment from :doc:`/install`.

``-G GENERATOR``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Specify a generator.

This is a mandatory option.

You can check available generators by ``cmake --help``.

Here is an example of specifying ``Visual Studio 12 2013 Win64`` as a generator::

  > cmake . -G "Visual Studio 12 2013 Win64" 

``-DCMAKE_INSTALL_PREFIX``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Specify a directory/folder to install Groonga.

The default is depending on the system, e.g. `/usr/local` or `C:/Program Files (x86)/groonga`.

Here is an example of specifying ``C:\Groonga`` as an install folder::

 > cmake . -DCMAKE_INSTALL_PREFIX=C:\Groonga

``-DGRN_WITH_MRUBY``
^^^^^^^^^^^^^^^^^^^^^^^

Enables mruby support.

You can use the :doc:`/reference/sharding` plugin and :doc:`/reference/commands/ruby_eval` 
with the mruby support.

The default is ``off``.

Groonga builds bundled mruby if the mruby support is enabled, so you must install some required libraries to build mruby.
See the `mruby compile guide <https://github.com/mruby/mruby/blob/master/doc/guides/compile.md>`_ for more details.

Here is an example of enabling the mruby support::

  > cmake . -DGRN_WITH_MRUBY=on

``-DGRN_WITH_APACHE_ARROW``
^^^^^^^^^^^^^^^^^^^^^^^^^^^

Enables Apache Arrow support.

You can use not only Apache Arrow IPC streaming format output but also multithreading processing that is used in :ref:`select-n-workers` 
and :doc:`/reference/functions/query_parallel_or` with the Apache Arrow support.   

The default is ``off``.

You can install Apache Arrow according to `the official installation procedure <https://arrow.apache.org/install/>`_. 

Here is an example of enabling the Apache Arrow support::

  > cmake . -DGRN_WITH_APACHE_ARROW=on

.. note::

   If you install Apache Arrow manually, use the :ref:`windows-cmake-cmake-prefix-path` option.

.. _windows-cmake-cmake-prefix-path:

``-DCMAKE_PREFIX_PATH=PATHS``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Adds search paths of ``.cmake`` files.

You can specify multiple paths with separating them with ``:``.

When using libraries installed via a package manager, you do not need to specify this 
parameter because ``.cmake`` files for the libraries are in the default search paths of cmake.

When using manual built libraries, you need to specify ``.cmake`` file paths of those libraries by this parameter.

Here is an example of specifying a ``.cmake`` file path for ``C:\arrow\cmake\arrowConfig.cmake``::

  > cmake . -DCMAKE_PREFIX_PATH="C:\arrow\cmake"
