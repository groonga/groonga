.. -*- rst -*-

``truncate``
============

Summary
-------

``truncate`` command deletes all records from specified table or all
values from specified column.

Syntax
------

This command takes only one required parameter::

  truncate target_name

.. versionadded:: 4.0.9

   ``target_name`` parameter can be used since 4.0.9. You need to use
   ``table`` parameter for 4.0.8 or earlier.

   For backward compatibility, ``truncate`` command accepts ``table``
   parameter. But it should not be used for newly written code.

Usage
-----

Here is a simple example of ``truncate`` command against a table.

.. groonga-command
.. database: commands_truncate_table
.. include:: ../../example/reference/commands/truncate/truncate_table.log
.. table_create Users TABLE_PAT_KEY ShortText
.. column_create Users score COLUMN_SCALAR Int32
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
.. database: commands_truncate_column
.. include:: ../../example/reference/commands/truncate/truncate_column.log
.. table_create Users TABLE_PAT_KEY ShortText
.. column_create Users score COLUMN_SCALAR Int32
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

Required parameters
^^^^^^^^^^^^^^^^^^^

There is required parameter, ``target_name``.

``target_name``
"""""""""""""""

Specifies the name of table or column.

Return value
------------

``truncate`` command returns whether truncation is succeeded or not::

  [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  If command succeeded, it returns true, otherwise it returns false on error.

