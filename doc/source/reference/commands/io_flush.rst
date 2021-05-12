.. -*- rst -*-

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

.. versionadded:: 7.0.4

   :ref:`io-flush-only-opened` is added.

.. versionadded:: 9.0.2

   ``--recursive dependent`` option is added. It is recommended way to flush
   target object and its related objects since 9.0.2. See :ref:`io-flush-recursive` about details.

Usage
-----

You can flush all changes in memory to disk with no arguments:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/all.log
.. io_flush

If you know what is changed, you can narrow flush targets. Here is a
correspondence table between command and flush targets for Groonga 9.0.2 or later.

.. note:: There is a different recommended way which depends on Groonga version.
   If Groonga is 9.0.1 or earlier ( ``--recursive dependent`` is not available ), you
   need to flush related objects explicitly, otherwise, using ``--recursive dependent``
   is a recommended way not to forget flush target objects.

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

     - Use ``--recursive dependent`` to flush target table and
       its columns, referenced tables and tables of corresponding index columns
       and corresponding index columns at once::

         io_flush --target_name TABLE_NAME --recursive dependent

   * - :doc:`truncate`
     - Target table and its columns.

       If there are one or more reference columns in these columns,
       referenced tables are also flush targets.

       If there are one or more indexed columns in these columns,
       tables of corresponding index columns and corresponding index
       columns are also flush targets.

       Database is also flush target.

     - Use ``--recursive dependent`` to flush target table and its columns,
       referenced tables and tables of corresponding index columns and corresponding index
       columns at once::

         io_flush --target_name TABLE_NAME --recursive dependent

   * - :doc:`table_create`
     - Target table and database.
     - Table::

         io_flush --target_name TABLE_NAME --recursive dependent

   * - :doc:`table_remove`, :doc:`table_rename` and :doc:`logical_table_remove`
     - Database.
     - Database::

         io_flush --recursive no
   * - :doc:`column_create`
     - Target column and database.
     - Table::

         io_flush --target_name TABLE_NAME.COLUMN_NAME --recursive dependent
   * - :doc:`column_remove` and :doc:`column_rename`
     - Database.
     - Database::

         io_flush --recursive no
   * - :doc:`plugin_register` and :doc:`plugin_unregister`
     - Database.
     - Database::

         io_flush --recursive no

If Groonga is 9.0.1 or earlier ( ``--recursive dependent`` is not available ), flush
objects explicitly. Here is a correspondence table between command and flush targets
for Groonga 9.0.1 or earlier.

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

Specifies a target object name. Target object is one of database,
table or column.

If you omit this parameter, database is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/target_name_database.log
.. io_flush

If you specify a table name, the table is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/target_name_table.log
.. table_create Users TABLE_HASH_KEY ShortText
.. io_flush --target_name Users

If you specify a column name, the column is the target object:

.. groonga-command
.. include:: ../../example/reference/commands/io_flush/target_name_column.log
.. column_create Users age COLUMN_SCALAR UInt8
.. io_flush --target_name Users.age

.. _io-flush-recursive:

``recursive``
"""""""""""""

Specifies whether child objects of the target object are also target
objects.

Child objects of database are all tables and all columns.

Child objects of table are all its columns.

Child objects of column are nothing.

If you specify ``yes`` to :ref:`io-flush-only-opened`, ``recursive``
is ignored.

``recursive`` value must be ``yes``, ``no`` or ``dependent``. ``yes``
means that all of the specified target object and child objects are
the target objects. ``no`` means that only the specified target object
is the target object. ``dependent`` means that all of the specified
target object, child objects, corresponding table of index column and
corresponding index column are the target objects.

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

Since 9.0.2, ``--recursive dependent`` is added to flush not only target object
and child objects, but also related objects. The related objects are:

* A referenced table
* A related index column (There is source column in target ``TABLE_NAME``)
* A table of related index column (There is source column in target ``TABLE_NAME``)

It is useful not to forget flushing related objects.

For example, ``--recursive dependent`` is specified for ``TABLE_NAME``, this
option executes equivalent to the following commands internally.

- Flush table and its columns::

    io_flush --target_name TABLE_NAME --recursive yes

- Flush a referenced table::

    io_flush --target_name REFERENCED_TABLE_NAME --recursive no

- Flush a related index column (There is source column in ``TABLE_NAME``)::

    io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN.INDEX_COLUMN

- Flush a table of related index column (There is source column in ``TABLE_NAME``)::

    io_flush --target_name TABLE_NAME_OF_INDEX_COLUMN --recursive no

To confirm whether all target objects are flushed correctly, you can check query log::

    > io_flush --recursive "dependent" --target_name "Users"
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

In above example, specified not only ``Users`` table, related lexicon table ``Terms`` and
index column ``Terms.users_name`` (data source is ``Users.name``) are also flushed.

``flush[(anonymous:...)]`` and ``flush[(DB)]`` means that Groonga's internal objects
are also flushed.

.. list-table::
   :header-rows: 1

   * - Log
     - Description

   * - ``flush[(anonymous:table:dat_key)]``
     - The internal object names in DB are flushed. If ``GRN_DB_KEY=pat`` is set, ``TABLE_PAT_KEY`` is used instead.

   * - ``flush[(anonymous:column:var_size)]`` (logged as first ``(anonymous:column:var_size)`` object)
     - The internal object metadata (builtin types, token filter and so on) are flushed.

       This is a variable sized column which stores the above internal object metadata.

   * - ``flush[(anonymous:table:hash_key)]``

     - The internal configuration objects (which is set by ``config_set``) are flushed.

   * - ``flush[(anonymous:column:var_size)]`` (logged as second ``(anonymous:column:var_size)`` object)
     - The internal object metadata (options about internal objects such as specified tokenizer options) are flushed.

       This is a variable sized column which stores the above internal object metadata.

   * - ``flush[(DB)]``

     - The DB changes (lock acquired during ``io_flush``) are flushed.

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

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
