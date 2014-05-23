.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_truncate

``truncate``
============

Summary
-------

``truncate`` command deletes all records from specified table.

Syntax
------

``truncate`` command takes only one parameter.

The required parameter is only ``table``::

  truncate table

Usage
-----

Here is a simple example of ``truncate`` command.

.. groonga-command
.. include:: ../../example/reference/commands/truncate/truncate.log
.. table_create Users TABLE_PAT_KEY ShortText
.. load --table Users
.. [
.. {"_key": "Alice",  "score": 2},
.. {"_key": "Bob",    "score": 0},
.. {"_key": "Carlos", "score": -1}
.. ]
.. select Users
.. truncate Users
.. select Users

Parameters
----------

This section describes parameters of ``truncate``.

Required parameter
^^^^^^^^^^^^^^^^^^

There is required parameter, ``table_name``.

``table_name``
""""""""""""""

It specifies the name of table.

Return value
------------

::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on error.

