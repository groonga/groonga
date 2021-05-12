.. -*- rst -*-

.. groonga-command
.. database: commands_table_remove

``table_remove``
================

Summary
-------

``table_remove`` removes a table and its columns. If there are one or
more indexes against key of the table and its columns, they are also
removed.

.. versionadded:: 6.0.1

   You can also remove tables and columns that reference the target
   table by using ``dependent`` parameter.

Syntax
------

This command takes two parameters::

  table_remove name
               [dependent=no]

.. _table-remove-usage:

Usage
-----

You just specify table name that you want to remove. ``table_remove``
removes the table and its columns. If the table and its columns are
indexed, all index columns for the table and its columns are also
removed.

This section describes about the followings:

  * Basic usage
  * Unremovable cases
  * Removes a table with tables and columns that reference the target table
  * Decreases used resources

.. _table-remove-basic-usage:

Basic usage
^^^^^^^^^^^

Let's think about the following case:

  * There is one table ``Entries``.
  * ``Entries`` table has some columns.
  * ``Entries`` table's key is indexed.
  * A column of ``Entries`` is indexed.

Here are commands that create ``Entries`` table:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/basic_usage_create_entries_table.log
.. table_create Entries TABLE_HASH_KEY UInt32
.. column_create Entries title COLUMN_SCALAR ShortText
.. column_create Entries content COLUMN_SCALAR Text

Here are commands that create an index for ``Entries`` table's key:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/basic_usage_create_index_for_entries_table.log
.. table_create EntryKeys TABLE_HASH_KEY UInt32
.. column_create EntryKeys key_index COLUMN_INDEX Entries _key

Here are commands that create an index for ``Entries`` table's column:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/basic_usage_create_index_for_entries_table_column.log
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Terms content_index COLUMN_INDEX Entries content

Let's confirm the current schema before running ``table_remove``:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/basic_usage_dump_before_table_remove.log
.. dump

If you remove ``Entries`` table, the following tables and columns are
removed:

  * ``Entries``
  * ``Entries.title``
  * ``Entries.context``
  * ``EntryKeys.key_index``
  * ``Terms.content_index``

The following tables (lexicons) aren't removed:

  * ``EntryKeys``
  * ``Terms``

Let's run ``table_remove``:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/basic_usage_table_remove.log
.. table_remove Entries

Here is schema after ``table_remove``. Only ``EntryKeys`` and
``Terms`` exist:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/basic_usage_dump_after_table_remove.log
.. dump

.. _table-remove-unremovable-cases:

Unremovable cases
^^^^^^^^^^^^^^^^^

There are some unremovable cases:

  * One or more tables use the table as key type.
  * One or more columns use the table as value type.

Both cases blocks dangling references. If the table is referenced as
type and the table is removed, tables and columns that refer the table
are broken.

If the target table satisfies one of them, ``table_remove`` is
failed. The target table and its columns aren't removed.

Here is an example for the table is used as key type case.

The following commands create a table to be removed and a table that
uses the table to be removed as key type:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_key_type_create.log
.. table_create ReferencedByTable TABLE_HASH_KEY ShortText
.. table_create ReferenceTable TABLE_HASH_KEY ReferencedByTable

``table_remove`` against ``ReferencedByTable`` is failed:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_key_type_remove_fail.log
.. table_remove ReferencedByTable

You need to remove ``ReferenceTable`` before you remove
``ReferencedByTable``:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_key_type_remove_success.log
.. table_remove ReferenceTable
.. table_remove ReferencedByTable

Here is an example for the table is used as value type case.

The following commands create a table to be removed and a column that
uses the table to be removed as value type:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_value_type_create.log
.. table_create ReferencedByColumn TABLE_HASH_KEY ShortText
.. table_create Table TABLE_NO_KEY
.. column_create Table reference_column COLUMN_SCALAR ReferencedByColumn

``table_remove`` against ``ReferencedByColumn`` is failed:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_value_type_remove_fail.log
.. table_remove ReferencedByColumn

You need to remove ``Table.reference_column`` before you remove
``ReferencedByColumn``:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_value_type_remove_success.log
.. column_remove Table reference_column
.. table_remove ReferencedByColumn

.. _table-remove-remove-dependents:

Removes a table with tables and columns that reference the target table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 6.0.1

If you understand what you'll do, you can also remove tables and
columns that reference the target table with one ``table_remove``
command by using ``--dependent yes`` parameter.

``ReferencedTable`` in the following schema is referenced from a table
and a column:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/remove_dependents_schema.log
.. table_create ReferencedTable TABLE_HASH_KEY ShortText
.. table_create Table1 TABLE_HASH_KEY ReferencedTable
.. table_create Table2 TABLE_NO_KEY
.. column_create Table2 reference_column COLUMN_SCALAR ReferencedTable

You can't remove ``ReferencedTable`` by default:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/remove_dependents_default.log
.. table_remove ReferencedTable

You can remove ``ReferencedTable``, ``Table1`` and
``Table2.reference_column`` by using ``--dependent yes``
parameter. ``Table1`` and ``Table2.reference_column`` reference
``ReferencedTable``:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/remove_dependents_yes.log
.. table_remove ReferencedTable --dependent yes

.. _table-remove-decreases-used-resources:

Decreases used resources
^^^^^^^^^^^^^^^^^^^^^^^^

``table_remove`` opens all tables and columns in database to check
:ref:`table-remove-unremovable-cases`.

If you have many tables and columns, ``table_remove`` may use many
resources. There is a workaround to avoid the case.

``table_remove`` closes temporary opened tables and columns for
checking when the max number of threads is ``1``.

You can confirm and change the current max number of threads by
:doc:`thread_limit`.

.. groonga-command
.. thread_limit 4

The feature is used in the following case:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/decreases_used_resources_close_temporary_opened_objects.log
.. table_create Entries TABLE_NO_KEY
.. thread_limit 1
.. table_remove Entries

The feature isn't used in the following case:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/decreases_used_resources_close_temporary_opened_objects.log
.. table_create Entries TABLE_NO_KEY
.. thread_limit 2
.. table_remove Entries

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

.. _table-remove-name:

``name``
""""""""

Specifies the table name to be removed.

See :ref:`table-remove-usage` how to use this parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is only one optional parameter.

.. _table-remove-dependent:

``dependent``
"""""""""""""

.. versionadded:: 6.0.1

Specifies whether tables and columns that reference the target table
are also removed or not.

If this value is ``yes``, tables and columns that reference the target
table are also removed. Otherwise, they aren't removed and an error is
returned.

In other words, if there are any tables and columns that reference the
target table, the target table isn't removed by default.

You should use this parameter carefully. This is a danger parameter.

See :ref:`table-remove-remove-dependents` how to use this parameter.

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
