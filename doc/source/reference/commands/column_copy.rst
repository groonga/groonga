.. -*- rst -*-

.. groonga-command
.. database: commands_copy

``column_copy``
===============

Summary
-------

.. versionadded:: 5.0.7

``column_copy`` copies all column values to other column.

You can implement the following features with this command:

  * Changing column configuration
  * Changing table configuration

You can change column configuration by the following steps:

  1. Create a new column with new configuration
  2. Copy all values from the current column to the new column
  3. Remove the current column
  4. Rename the new column to the current column

You can change table configuration by the following steps:

  1. Create a new table with new configuration
  2. Create all same columns to the new table
  3. Copy all column values from the current table to the new table
  4. Remove the current table
  5. Rename the new table to the current table

Concrete examples are showed later.

You can't copy column values from a ``TABLE_NO_KEY`` table to another
table. And you can't copy column values to a ``TABLE_NO_KEY`` table
from another table. Because Groonga can't map records without record
key.

You can copy column values from a ``TABLE_NO_KEY`` table to the same
``TABLE_NO_KEY`` table.

You can copy column values from a ``TABLE_HASH_KEY`` /
``TABLE_PAT_KEY`` / ``TABLE_DAT_KEY`` table to the same or another
``TABLE_HASH_KEY`` / ``TABLE_PAT_KEY`` / ``TABLE_DAT_KEY`` table.

Syntax
------

This command takes four parameters.

All parameters are required::

  column_copy from_table
              from_name
              to_table
              to_name

Usage
-----

Here are use cases of this command:

  * Changing column configuration
  * Changing table configuration

How to change column configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can change column value type. For example, you can change
``UInt32`` column value to ``ShortText`` column value.

You can change column type. For example, you can change
``COLUMN_SCALAR`` column to ``COLUMN_VECTOR`` column.

You can move a column to other table. For example, you can move
``high_score`` column to ``Users`` table from ``Players`` table.

Here are basic steps to change column configuration:

  1. Create a new column with new configuration
  2. Copy all values from the current column to the new column
  3. Remove the current column
  4. Rename the new column to the current column

Here is an example to change column value type to ``Int32`` from
``ShortText``.

Here are schema and data:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_column_value_type_setup.log
.. table_create Logs TABLE_HASH_KEY ShortText
.. column_create Logs serial COLUMN_SCALAR Int32
.. load --table Logs
.. [
.. {"_key": "log1", "serial": 1}
.. ]

The following commands change ``Logs.serial`` column value type to
``ShortText`` from ``Int32``:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_column_value_type.log
.. column_create Logs new_serial COLUMN_SCALAR ShortText
.. column_copy Logs serial Logs new_serial
.. column_remove Logs serial
.. column_rename Logs new_serial serial
.. select Logs

You can find ``Logs.serial`` stores ``ShortText`` value from the
response of ``select``.

Here is an example to change column type to ``COLUMN_VECTOR`` from
``COLUMN_SCALAR``.

Here are schema and data:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_column_type_setup.log
.. table_create Entries TABLE_HASH_KEY ShortText
.. column_create Entries tag COLUMN_SCALAR ShortText
.. load --table Entries
.. [
.. {"_key": "entry1", "tag": "Groonga"}
.. ]

The following commands change ``Entries.tag`` column to
``COLUMN_VECTOR`` from ``COLUMN_SCALAR``:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_column_type.log
.. column_create Entries new_tag COLUMN_VECTOR ShortText
.. column_copy Entries tag Entries new_tag
.. column_remove Entries tag
.. column_rename Entries new_tag tag
.. select Entries

You can find ``Entries.tag`` stores ``COLUMN_VECTOR`` value from the
response of ``select``.

Here is an example to move ``high_score`` column to ``Users`` table
from ``Players`` table.

Here are schema and data:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/move_column_setup.log
.. table_create Players TABLE_HASH_KEY ShortText
.. column_create Players high_score COLUMN_SCALAR Int32
.. load --table Players
.. [
.. {"_key": "player1", "high_score": 100}
.. ]

The following commands move ``high_score`` column to ``Users`` table
from ``Players`` table:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/move_column.log
.. table_create Users TABLE_HASH_KEY ShortText
.. column_create Users high_score COLUMN_SCALAR Int32
.. column_copy Players high_score Users high_score
.. column_remove Players high_score
.. select Users

You can find ``Users.high_score`` is moved from ``Players.high_score``
from the response of ``select``.

How to change table configuration
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can change table key type. For example, you can change
key type to ``ShortText`` from ``Int32``.

You can change table type. For example, you can change
``TABLE_HASH_KEY`` table to ``TABLE_PAT_KEY`` table.

You can also change other options such as default tokenizer and
normalizer. For example, you can change default tokenizer to
``TokenBigramSplitSymbolAlphaDigit`` from ``TokenBigrm``.

.. note::

   You can't change ``TABLE_NO_KEY`` table. Because ``TABLE_NO_KEY``
   doesn't have record key. Groonga can't identify copy destination
   record without record key.

Here are basic steps to change table configuration:

  1. Create a new table with new configuration
  2. Create all same columns to the new table
  3. Copy all column values from the current table to the new table
  4. Remove the current table
  5. Rename the new table to the current table

Here is an example to change table key type to ``ShortText`` from
``Int32``.

Here are schema and data:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_table_key_type_setup.log
.. table_create IDs TABLE_HASH_KEY Int32
.. column_create IDs label COLUMN_SCALAR ShortText
.. column_create IDs used COLUMN_SCALAR Bool
.. load --table IDs
.. [
.. {"_key": 100, "label": "ID 100", used: true}
.. ]

The following commands change ``IDs`` table key type to ``ShortText``
from ``Int32``:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_table_key_type.log
.. table_create NewIDs TABLE_HASH_KEY Int32
.. column_create NewIDs label COLUMN_SCALAR ShortText
.. column_create NewIDs used COLUMN_SCALAR Bool
.. column_copy IDs label NewIDs label
.. column_copy IDs used  NewIDs used
.. table_remove IDs
.. table_rename NewIDs IDs
.. select IDs

You can find ``IDs`` stores ``ShortText`` key from the response of
``select``.

Here is an example to change table type to ``TABLE_PAT_KEY`` from
``TABLE_HASH_KEY``.

Here are schema and data:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_table_type_setup.log
.. table_create Names TABLE_HASH_KEY ShortText
.. column_create Names used COLUMN_SCALAR Bool
.. load --table Names
.. [
.. {"_key": "alice", "used": false}
.. ]

The following commands change ``Names`` table to ``TABLE_PAT_KEY``
from ``TABLE_HASH_KEY``:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/change_table_type.log
.. table_create NewNames TABLE_PAT_KEY ShortText
.. column_create NewNames used COLUMN_SCALAR Bool
.. column_copy Names used NewNames used
.. table_remove Names
.. table_rename NewNames Names
.. select Names --filter '_key @^ "ali"'

You can find ``Names`` is a ``TABLE_PAT_KEY`` because ``select`` can
use :ref:`script-syntax-prefix-search-operator`. You can't use
:ref:`script-syntax-prefix-search-operator` with ``TABLE_HASH_KEY``.

Parameters
----------

This section describes parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

All parameters are required.

.. _column-copy-from-table:

``from_table``
""""""""""""""

Specifies the table name of source column.

You can specify any table including ``TABLE_NO_KEY`` table.

If you specify ``TABLE_NO_KEY`` table, :ref:`column-copy-to-table`
must be the same table.

Here is an example to use ``from_table``.

Here are schema and data:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/from_table_setup.log
.. table_create FromTable TABLE_HASH_KEY ShortText
.. column_create FromTable from_column COLUMN_SCALAR ShortText
.. column_create FromTable to_column   COLUMN_SCALAR ShortText
.. load --table FromTable
.. [
.. {"_key": "key1", "from_column": "value1"}
.. ]
.. select FromTable --output_columns _key,from_column,to_column

You can copy all values to ``to_column`` from ``from_column``:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/from_table.log
.. column_copy FromTable from_column FromTable to_column
.. select FromTable --output_columns _key,from_column,to_column

.. _column-copy-from-name:

``from_name``
"""""""""""""

Specifies the column name to be copied values.

See :ref:`column-copy-from-table` for example.

.. _column-copy-to-table:

``to_table``
""""""""""""

Specifies the table name of destination column.

You can specify the same table name as :ref:`column-copy-from-table`
when you want to copy column values in the same table.

You can't specify ``TABLE_NO_KEY`` table to ``to_table`` because
Groonga can't identify destination records without record key.

There is one exception. If you specify the same name as ``from_table``
to ``to_table``, you can use ``TABLE_NO_KEY`` table as
``to_table``. Because Groonga can identify destination records when
source table and destination table is the same table.

Here is an example to use ``to_table``.

Here are schema and data:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/to_table_setup.log
.. table_create Table TABLE_HASH_KEY ShortText
.. column_create Table column COLUMN_SCALAR ShortText
.. table_create ToTable TABLE_HASH_KEY ShortText
.. column_create ToTable to_column COLUMN_SCALAR ShortText
.. load --table Table
.. [
.. {"_key": "key1", "column": "value1"}
.. ]

You can copy all values to ``ToTable.to_column`` from
``Table.column``:

.. groonga-command
.. include:: ../../example/reference/commands/column_copy/to_table.log
.. column_copy Table column ToTable to_column
.. select ToTable --output_columns _key,to_column

.. _column-copy-to-name:

``to_name``
"""""""""""

Specifies the destination column name.

See :ref:`column-copy-to-table` for example.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
