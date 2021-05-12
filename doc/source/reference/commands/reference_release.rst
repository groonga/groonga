.. -*- rst -*-

.. groonga-command
.. database: commands_reference_release

``reference_release``
=====================

Summary
-------

.. versionadded:: 10.0.4

``reference_release`` releases a reference of target objects acquired
by :doc:`reference_acquire`.

This command is meaningless unless you use the reference count
mode. You can enable the reference count mode by the
``GRN_ENABLE_REFERENCE_COUNT=yes`` environment variable.

You must call a corresponding ``reference_release`` for a
:doc:`reference_acquire` call. If you forget to call
``reference_release``, target objects by :doc:`reference_acquire`
are never closed automatically.

Syntax
------

This command takes two parameters.

All parameters are optional::

  reference_release [target_name=null]
                    [recursive=yes]

Usage
-----

Here is a schema definition to show usage:

.. groonga-command
.. include:: ../../example/reference/commands/reference_release/schema.log
.. table_create Users TABLE_HASH_KEY ShortText
.. column_create Users age COLUMN_SCALAR UInt8
.. column_create Users introduction COLUMN_SCALAR ShortText
..
.. table_create Ages TABLE_PAT_KEY UInt8
.. column_create Ages user_age COLUMN_INDEX Users age

You must call ``reference_release`` with the same arguments as
corresponding :doc:`reference_acquire`.

If you call :doc:`reference_acquire` with ``--target_name
Users --recursive dependent``, you must call ``reference_release``
with ``--target_name Users --recursive dependent``:

.. groonga-command
.. include:: ../../example/reference/commands/reference_release/same_arguments.log
.. reference_release --target_name Users --recursive dependent
.. # select Users ...
.. # load --table Users ...
.. reference_release --target_name Users --recursive dependent

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

.. _reference-release-target-name:

``target_name``
"""""""""""""""

Specifies a target object name. Target object is one of database,
table or column.

If you omit this parameter, database is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/reference_release/target_name_database.log
.. reference_release

If you specify a table name, the table is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/reference_release/target_name_table.log
.. reference_release --target_name Users

If you specify a column name, the column is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/reference_release/target_name_column.log
.. reference_release --target_name Users.age

.. _reference-release-recursive:

``recursive``
"""""""""""""

Specifies whether child objects of the target object are also target
objects.

See :ref:`reference-acquire-recursive` for details.

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
