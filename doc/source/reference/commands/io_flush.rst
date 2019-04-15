.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_io_flush

``io_flush``
============

Summary
-------

.. versionadded:: 5.0.5

``io_flush`` flushes all changes in memory to disk
explicitly. Normally, you don't need to use ``io_flush``
explicitly. Because flushing is done automatically by OS. And flushing
by OS is effective.

You need to use ``io_flush`` explicitly when your system may often
crash unexpectedly or you may not shutdown your Groonga process in a
normal way. (For example, using :doc:`shutdown` is a normal shutdown
process.) It's better that you use ``io_flush`` after you change your
Groonga database for the case. Here are commands that change your
Groonga database:

  * :doc:`column_create`
  * :doc:`column_remove`
  * :doc:`column_rename`
  * :doc:`delete`
  * :doc:`load`
  * :doc:`logical_table_remove`
  * :doc:`object_remove`
  * :doc:`plugin_register`
  * :doc:`plugin_unregister`
  * :doc:`table_create`
  * :doc:`table_remove`
  * :doc:`table_rename`
  * :doc:`truncate`

If you're using :ref:`select-scorer` parameter in :doc:`select` to
change existing column values, :doc:`select` is added to the above
list.

Note that ``io_flush`` may be a heavy process. If there are many
changes in memory, flushing them to disk is a heavy process.

.. versionadded:: 8.0.8

   ``io_flush`` locks Groonga database while flushing. It means that
   you can't run the following commands while ``io_flush``:

     * :doc:`column_create`
     * :doc:`column_remove`
     * :doc:`column_rename`
     * :doc:`logical_table_remove`
     * :doc:`object_remove`
     * :doc:`plugin_register`
     * :doc:`plugin_unregister`
     * :doc:`table_create`
     * :doc:`table_remove`
     * :doc:`table_rename`

Syntax
------

This command takes three parameters.

All parameters are optional::

  io_flush [target_name=null]
           [recursive=yes]
           [only_opened=no]
           [dependent=yes]

.. versionadded:: 7.0.4

   :ref:`io-flush-only-opened` is added.

.. versionadded:: 9.0.2

   :ref:`io-flush-dependent` is added.

Usage
-----

You can flush all changes in memory to disk with no arguments:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/all.log
.. io_flush

If you know what is changed, you can narrow flush targets. Here is a
correspondence table between command and flush targets.

.. list-table::
   :header-rows: 1

   * - Command
     - Flush targets
     - ``io_flush`` arguments
   * - :doc:`load` and :doc:`delete`
     - Target table and its columns.

       If there are one or more reference columns in these columns,
       referenced tables are also flush targets.

       If there are one or more indexed columns in these columns,
       tables of corresponding index columns and corresponding index
       columns are also flush targets.
     - Table and its columns::

         io_flush --target_name TABLE_NAME

       A referenced table::

         io_flush --target_name REFERENCED_TABLE_NAME --recursive no

       A table of an index column::

         io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN --recursive no

       An index column::

         io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN.INDEX_COLUMN
   * - :doc:`truncate`
     - Target table and its columns.

       If there are one or more reference columns in these columns,
       referenced tables are also flush targets.

       If there are one or more indexed columns in these columns,
       tables of corresponding index columns and corresponding index
       columns are also flush targets.

       Database is also flush target.
     - Table and its columns::

         io_flush --target_name TABLE_NAME

       A referenced table::

         io_flush --target_name REFERENCED_TABLE_NAME --recursive no

       A table of an index column::

         io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN --recursive no

       An index column::

         io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN.INDEX_COLUMN

       Database::

         io_flush --recursive no
   * - :doc:`table_create`
     - Target table and database.
     - Table::

         io_flush --target_name TABLE_NAME

       Database::

         io_flush --recursive no
   * - :doc:`table_remove`, :doc:`table_rename` and :doc:`logical_table_remove`
     - Database.
     - Database::

         io_flush --recursive no
   * - :doc:`column_create`
     - Target column and database.
     - Table::

         io_flush --target_name TABLE_NAME.COLUMN_NAME

       Database::

         io_flush --recursive no
   * - :doc:`column_remove` and :doc:`column_rename`
     - Database.
     - Database::

         io_flush --recursive no
   * - :doc:`plugin_register` and :doc:`plugin_unregister`
     - Database.
     - Database::

         io_flush --recursive no


Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

.. _io-flush-target-name:

``target_name``
"""""""""""""""

Specifies a flush target object name. Target object is one of
database, table or column.

If you omit this parameter, database is flush target object:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/target_name_database.log
.. io_flush

If you specify table name, the table is flush target object:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/target_name_table.log
.. table_create Users TABLE_HASH_KEY ShortText
.. io_flush --target_name Users

If you specify column name, the column is flush target object:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/target_name_column.log
.. column_create Users age COLUMN_SCALAR UInt8
.. io_flush --target_name Users.age

.. _io-flush-recursive:

``recursive``
"""""""""""""

Specifies whether child objects of the flush target object are also
flush target objects.

Child objects of database is all tables and all columns.

Child objects of table is all its columns.

Child objects of column is nothing.

If you specify ``yes`` to :ref:`io-flush-only-opened`, ``recursive``
is ignored.

``recursive`` value must be ``yes`` or ``no``. ``yes`` means that all
of the specified flush target object and child objects are flush
target objects. ``no`` means that only the specified flush target
object is flush target object.

The following ``io_flush`` flushes all changes in database, all tables
and all columns:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/recursive_yes.log
.. io_flush --recursive yes

The following ``io_flush`` flushes all changes only in database:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/recursive_no.log
.. io_flush --recursive no

If you specify other value (not ``yes`` neither ``no``) or omit
``recursive`` parameter, ``yes`` is used.

``yes`` is used in the following case because invalid ``recursive``
argument is specified:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/recursive_invalid.log
.. io_flush --recursive invalid

``yes`` is used in the following case because ``recursive`` parameter
isn't specified:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/recursive_default.log
.. io_flush

.. _io-flush-only-opened:

``only_opened``
"""""""""""""""

.. versionadded:: 7.0.4

Specifies whether opened objects are only flushed.

If there is only one process that changes the target database, flush
performance will be improved by specifying ``yes`` to
``only_opened``. It skips needless flushes.

You can't specify :ref:`io-flush-target-name` with ``only_opened``
option. If you specify :ref:`io-flush-target-name`, ``only_opened`` is
ignored.

If you specify ``yes`` to ``only_opened``, :ref:`io-flush-recursive`
is ignored.

``only_opened`` value must be ``yes`` or ``no``. ``yes`` means that
opened objects are only flushed. ``no`` means that all target objects
are flushed even they aren't opened.

The following ``io_flush`` flushes all changes in database, all tables
and all columns:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/only_opened_yes.log
.. io_flush --only_opened yes

.. _io-flush-dependent:

``dependent``
"""""""""""""

.. versionadded:: 9.0.2

Specifies whether not only child objects of the flush target object,
but also related index columns and table which contains related index
columns are flushed.

This option is similar to ``recursive``, but there is a different that
related index columns and table are also flushed.  It is useful for
users because this option take care of the objects which objects are related.
It supports not to forget flushing related objects.

``dependent`` value must be ``yes`` or ``no``. ``yes`` means that all
of the specified flush target object and child objects, related objects.
``no`` means that only the specified flush target object is flushed.

If you specify ``yes`` to ``dependent``, :ref:`io-flush-recursive`
is ignored.

For example, ``--dependent yes`` is enabled for ``TABLE_NAME``, this
option executes equivalent to the following commands internally.

- Table and its columns::

    io_flush --target_name TABLE_NAME

- A referenced table::

    io_flush --target_name REFERENCED_TABLE_NAME --recursive no

- A related table of an index column (There is source column in TABLE_NAME)::

    io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN --recursive no

- An related index column (There is source column in TABLE_NAME)::

    io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN.INDEX_COLUMN

The following ``io_flush`` flushes all changes in database, all tables
and all columns:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/dependent_yes.log
.. io_flush --target_name Users --dependent yes

To confirm whether all target objects are flushed correctly, you can check query log::

    > io_flush --dependent "yes" --target_name "Users"
    :000000000000000 flush[Users]
    :000000000000000 flush[Terms]
    :000000000000000 flush[Terms.users_name]
    :000000000000000 flush[Users.name]
    :000000000000000 flush[(anonymous:table:dat_key)]
    :000000000000000 flush[(anonymous:column:var_size)]
    :000000000000000 flush[(anonymous:table:hash_key)]
    :000000000000000 flush[(anonymous:column:var_size)]
    :000000000000000 flush[(DB)]
    <000000000000000 rc=0

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
