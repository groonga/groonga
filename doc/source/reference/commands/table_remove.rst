.. -*- rst -*-

.. highlightlang:: none

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

   This removes the specified table and its all dependencies, if you
   specify the ``--dependent yes`` parameter. Then all other tables
   and columns referencing the disappearing table itself are also
   removed together at a time.

Syntax
------

This command takes two parameters::

  table_remove name
               [dependent=no]

.. _table-remove-usage:

Usage
-----

This command requires a name of a table you want to remove. The
specified table and its all columns will be removed. All index columns
for the table and its columns are also removed, if they are indexed.

This section describes about the followings:

  * Basic usage
  * Unremovable cases
  * Automatic removing of depending tables and columns
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

If you remove the ``Entries`` table, the following tables and columns
are removed:

  * ``Entries``
  * ``Entries.title``
  * ``Entries.context``
  * ``EntryKeys.key_index``
  * ``Terms.content_index``

The following tables (lexicons) are left:

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

You cannot remove a table, if there is any dependency. In other words,
any table satisfying one or more following conditions is unremovable:

  * One or more tables use the table as their key type.
  * One or more columns use the table as their value type.

``table_remove`` fails for such referenced tables, to avoid breaking
of left tables with missing references.

Here is an example for a table which is used as a key type.

The following commands create a table ``User`` going to be removed
and another table ``AdminUser` depending on the first table via its key
type:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_key_type_create.log
.. table_create User TABLE_HASH_KEY ShortText
.. table_create AdminUser TABLE_HASH_KEY User

``table_remove`` against ``User`` fails:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_key_type_remove_fail.log
.. table_remove User

You need to remove ``AdminUser`` before ``User``:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_key_type_remove_success.log
.. table_remove AdminUser
.. table_remove User

Here is another example for a table which is used as a value type.

The following commands create a table ``User`` going to be removed
and another table ``GeneralUser`` with a column depending on the first
table via its value type:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_value_type_create.log
.. table_create User TABLE_HASH_KEY ShortText
.. table_create GeneralUser TABLE_NO_KEY
.. column_create GeneralUser id COLUMN_SCALAR User

``table_remove`` against ``User`` fails:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_value_type_remove_fail.log
.. table_remove User

You need to remove ``GeneralUser.id`` before ``User``:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/unremovable_cases_value_type_remove_success.log
.. column_remove GeneralUser id
.. table_remove User

.. _table-remove-remove-dependents:

Automatic removing of depending tables and columns
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 6.0.1

If you understand what you'll do, you can remove a table and other
tables depending on it together at a time, by the ``--dependent yes``
parameter.

``User`` in the following schema is referenced from a table
and a column:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/remove_dependents_schema.log
.. table_create User TABLE_HASH_KEY ShortText
.. table_create AdminUser TABLE_HASH_KEY User
.. table_create GeneralUser TABLE_NO_KEY
.. column_create GeneralUser id COLUMN_SCALAR User

You can't remove ``User`` by default:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/remove_dependents_default.log
.. table_remove User

Additional parameter ``--dependent yes`` for the command line allows
you to remove the ``User`` table, then both ``AdminUser`` and
``GeneralUser.id`` depending on the ``User`` are also removed at a
time:

.. groonga-command
.. include:: ../../example/reference/commands/table_remove/remove_dependents_yes.log
.. table_remove User --dependent yes

After all, only the independent ``GeneralUser`` table is left.

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
