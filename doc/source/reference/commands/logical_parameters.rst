.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: logical_parameters

``logical_parameters``
========================

Summary
-------

.. note::

   This command is an experimental feature.

.. versionadded:: 5.0.6

TODO: Write summary

Syntax
------

  logical_parameters [range_index]

Usage
-----

Register ``sharding`` plugin to use ``logical_parameters`` command in advance.

.. groonga-command
.. plugin_register sharding

Parameters
----------

This section describes parameters of ``logical_parameters``.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

``range_index``
"""""""""""""""""

TODO

Return value
------------

::

  [HEADER, {"range_index":"PARAMETER_NAME"}]
