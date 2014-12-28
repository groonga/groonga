.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_truncate

``truncate``
============

Summary
-------

``truncate`` command deletes all records from specified table or all
values from specified column.

Syntax
------

``truncate`` command takes only one parameter.

The required parameter is only ``target_name``::

  truncate target_name

For backward compatibility, ``truncate`` command accepts ``table``
parameter. But it should not be used for newly written code.

Usage
-----

Here is a simple example of ``truncate`` command against a table.

.. groonga-command
.. include:: ../../example/reference/commands/truncate/truncate-table.log
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

Here is a simple example of ``truncate`` command against a column.

.. groonga-command
.. include:: ../../example/reference/commands/truncate/truncate-column.log
.. table_create Users TABLE_PAT_KEY ShortText
.. load --table Users
.. [
.. {"_key": "Alice",  "score": 2},
.. {"_key": "Bob",    "score": 0},
.. {"_key": "Carlos", "score": -1}
.. ]
.. select Users
.. truncate Users.score
.. select Users

Parameters
----------

This section describes parameters of ``truncate``.

Required parameter
^^^^^^^^^^^^^^^^^^

There is required parameter, ``target_name``.

``target_name``
"""""""""""""""

It specifies the name of table or column.

Return value
------------

::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on error.

