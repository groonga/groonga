.. -*- rst -*-

.. groonga-command
.. database: commands_column_rename

``column_rename``
=================

Summary
-------

``column_rename`` command renames a column.

It is a light operation. It just changes a relationship between name
and the column object. It doesn't copy column values.

It is a dangerous operation. You must stop all operations including
read operations while you run ``column_rename``. If the following case
is occurred, Groonga process may be crashed:

  * Starts an operation (like ``select``) that accesses the column to
    be renamed by the current column name. The current column name is
    called as ``the old column name`` in the below because the column
    name is renamed.
  * Runs ``column_rename``. The ``select`` is still running.
  * The ``select`` accesses the column to be renamed by the old column
    name. But the ``select`` can't find the column by the old name
    because the column has been renamed to the new column name. It may
    crash the Groonga process.

Syntax
------

This command takes three parameters.

All parameters are required::

  column_rename table name new_name

Usage
-----

Here is a simple example of ``column_rename`` command.

.. groonga-command
.. include:: ../../example/reference/commands/column_rename/column_rename.log
.. table_create Users TABLE_PAT_KEY ShortText
.. column_create Users score COLUMN_SCALAR Int32
.. load --table Users
.. [
.. {"_key": "Alice",  "score": 2},
.. {"_key": "Bob",    "score": 0},
.. {"_key": "Carlos", "score": -1}
.. ]
.. column_rename Users score point
.. column_list Users
.. select Users

Parameters
----------

This section describes parameters of ``column_rename``.

Required parameters
^^^^^^^^^^^^^^^^^^^

All parameters are required.

``table``
"""""""""

Specifies the name of table that has the column to be renamed.

``name``
""""""""

Specifies the column name to be renamed.

``new_name``
""""""""""""

Specifies the new column name.


Return value
------------

::

 [HEADER, SUCCEEDED_OR_NOT]

``HEADER``

  See :doc:`/reference/command/output_format` about ``HEADER``.

``SUCCEEDED_OR_NOT``

  It is ``true`` on success, ``false`` otherwise.

