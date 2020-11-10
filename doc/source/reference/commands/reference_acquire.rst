.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_reference_acquire

``reference_acquire``
=====================

Summary
-------

.. versionadded:: 10.0.4

``reference_acquire`` acquires a reference of target objects.

This command is meaningless unless you use the reference count
mode. You can enable the reference count mode by the
``GRN_ENABLE_REFERENCE_COUNT=yes`` environment variable.

If you acquires a reference of an object, the object isn't closed
automatically because you have at least one reference of the
object. If you need to call multiple :doc:`load` in a short time, auto
close by the reference count mode will degrade performance. You can
avoid the performance degrading by calling ``reference_acquire``
before multiple :doc:`load` and calling :doc:`reference_release` after
multiple :doc:`load`. Between ``reference_acquire`` and
:doc:`reference_release`, auto close is disabled.

You must call :doc:`reference_release` after you finish performance
impact operations. If you don't call :doc:`reference_release`, the
reference count mode doesn't work.

Syntax
------

This command takes two parameters.

All parameters are optional::

  reference_acquire [target_name=null]
                    [recursive=yes]

Usage
-----

You can acquire a reference of the all objects with no arguments:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/all.log
.. reference_acquire

If you know what should be referred, you can narrow targets.

Here is a schema definition to show usage:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/schema.log
.. table_create Users TABLE_HASH_KEY ShortText
.. column_create Users age COLUMN_SCALAR UInt8
.. column_create Users introduction COLUMN_SCALAR ShortText
..
.. table_create Ages TABLE_PAT_KEY UInt8
.. column_create Ages user_age COLUMN_INDEX Users age

If you want to call multiple :doc:`select` without any condition
against ``Users`` table, the following command acquires a reference of
``Users``, ``Users.age`` and ``Users.introduction``:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/for_output.log
.. reference_acquire --target_name Users

If you want to call multiple :doc:`select` with condition that uses
indexes against ``Users`` table, the following command acquires a
reference of ``Users``, ``Users.age``, ``Users.introduction``,
``Ages`` (lexicon) and ``Ages.user_age`` (index column). This command
is suitable for :doc:`load` too:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/for_search.log
.. reference_acquire --target_name Users --recursive dependent

If you want to just refer ``Users``, you can specify a table with
``recursive=no``:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/table.log
.. reference_acquire --target_name Users --recursive no

If you want to just refer ``Users.introduction``, you can specify a
column:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/column.log
.. reference_acquire --target_name Users.introduction

You can release acquired references by calling
:doc:`reference_release` with the same arguments:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/release.log
.. reference_acquire --target_name Users --recursive dependent
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

.. _reference-acquire-target-name:

``target_name``
"""""""""""""""

Specifies a target object name. Target object is one of database,
table or column.

If you omit this parameter, database is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/target_name_database.log
.. reference_acquire

If you specify a table name, the table is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/target_name_table.log
.. reference_acquire --target_name Users

If you specify a column name, the column is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/target_name_column.log
.. reference_acquire --target_name Users.age

.. _reference-acquire-recursive:

``recursive``
"""""""""""""

Specifies whether child objects of the target object are also target
objects.

Child objects of database are all tables and all columns.

Child objects of table are all its columns.

Child objects of column are nothing.

``recursive`` value must be ``yes``, ``no`` or ``dependent``. ``yes``
means that all of the specified target object and child objects are
the target objects. ``no`` means that only the specified target object
is the target object. ``dependent`` means that all of the specified
target object, child objects, corresponding table of index column and
corresponding index column are the target objects.

The following ``reference_acquire`` acquires a reference of all tables
and all columns:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/recursive_yes.log
.. reference_acquire --recursive yes

The following ``reference_acquire`` acquires a reference of only
``Users`` table:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/recursive_no.log
.. reference_acquire --target_name Users --recursive no

If you specify other value (not ``yes`` neither ``no``) or omit
``recursive`` parameter, ``yes`` is used.

``yes`` is used in the following case because invalid ``recursive``
argument is specified:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/recursive_invalid.log
.. reference_acquire --target_name Users --recursive invalid

``yes`` is used in the following case because ``recursive`` parameter
isn't specified:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/recursive_default.log
.. reference_acquire --target_name Users

``dependent`` acquires a reference of not only the target object and
the child objects but also related objects. The related objects are:

* A referenced table
* Data columns of referenced tables
* A related index column (There is source column in target ``TABLE_NAME``)
* A table of related index column (There is source column in target ``TABLE_NAME``)
* Data columns of tables of related index columns

It is useful to keep reference for :doc:`load` and :doc:`select`.

If you want to call multiple :doc:`load` for ``Users`` table, you can
use the following command:

.. groonga-command
.. include:: ../../example/reference/commands/reference_acquire/recursive_dependent.log
.. reference_acquire --target_name Users --recursive dependent

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
