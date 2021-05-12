.. -*- rst -*-

.. groonga-command
.. database: commands_table_rename

``table_rename``
=================

Summary
-------

``table_rename`` command renames a table.

It is a light operation. It just changes a relationship between name
and the table object. It doesn't copy table and its column values.

It is a dangerous operation. You must stop all operations including
read operations while you run ``table_rename``. If the following case
is occurred, Groonga process may be crashed:

  * Starts an operation (like ``select``) that accesses the table to
    be renamed by the current table name. The current table name is
    called as ``the old table name`` in the below because the table
    name is renamed.
  * Runs ``table_rename``. The ``select`` is still running.
  * The ``select`` accesses the table to be renamed by the old table
    name. But the ``select`` can't find the table by the old name
    because the table has been renamed to the new table name. It may
    crash the Groonga process.

Syntax
------

This command takes two parameters.

All parameters are required::

  table_rename name new_name

Usage
-----

Here is a simple example of ``table_rename`` command.

.. groonga-command
.. include:: ../../example/reference/commands/table_rename/usage.log
.. table_create Users TABLE_PAT_KEY ShortText
.. column_create Users score COLUMN_SCALAR Int32
.. load --table Users
.. [
.. {"_key": "Alice",  "score": 2},
.. {"_key": "Bob",    "score": 0},
.. {"_key": "Carlos", "score": -1}
.. ]
.. table_rename Users Players
.. table_list
.. select Players

Parameters
----------

This section describes parameters of ``table_rename``.

Required parameters
^^^^^^^^^^^^^^^^^^^

All parameters are required.

``name``
""""""""

Specifies the table name to be renamed.

``new_name``
""""""""""""

Specifies the new table name.

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
