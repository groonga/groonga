.. -*- rst -*-

.. groonga-command
.. database: logical_table_remove

``logical_table_remove``
========================

Summary
-------

.. versionadded:: 5.0.5

``logical_table_remove`` removes tables and their columns for the
specified logical table. If there are one or more indexes against key
of the tables and their columns, they are also removed.

If you specify the part of a shard, table of the shard isn't
removed. ``logical_table_remove`` just deletes records in the table.

For example, there are the following records in a table:

  * Record1: ``2016-03-18 00:30:00``
  * Record2: ``2016-03-18 01:00:00``
  * Record3: ``2016-03-18 02:00:00``

``logical_table_remove`` deletes "Record1" and "Record2" when you
specify range as between ``2016-03-18 00:00:00`` and ``2016-03-18
01:30:00``. ``logical_table_remove`` doesn't delete
"Record3". ``logical_table_remove`` doesn't remove the table.

.. versionadded:: 6.0.1

   You can also remove tables and columns that reference the target
   table and tables related with the target shard by using
   ``dependent`` parameter.

.. versionadded:: 6.0.9

   You can remove broken tables and columns as much as possible by
   using ``force`` parameter.

Syntax
------

This command takes many parameters.

The required parameters are ``logical_table`` and ``shard_key``::

  logical_table_remove logical_table
                       shard_key
                       [min=null]
                       [min_border="include"]
                       [max=null]
                       [max_border="include"]
                       [dependent=no]
                       [force=no]

.. _logical-table-remove-usage:

Usage
-----

You specify logical table name and shard key what you want to remove.

This section describes about the followings:

  * Basic usage
  * Removes parts of a logical table
  * Unremovable cases
  * Removes with related tables
  * Removes broken tables as much as possible
  * Decreases used resources

.. _logical-table-remove-basic-usage:

Basic usage
^^^^^^^^^^^

Register ``sharding`` plugin to use this command in advance.

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/usage_register.log
.. plugin_register sharding

You can remove all tables for the logical table by specifying only
``logical_table`` and ``shard_key``.

Here are commands to create 2 shards:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/basic_usage_create_shards.log
.. table_create  Logs_20160318 TABLE_NO_KEY
.. column_create Logs_20160318 timestamp COLUMN_SCALAR Time
..
.. table_create  Logs_20160319 TABLE_NO_KEY
.. column_create Logs_20160319 timestamp COLUMN_SCALAR Time

You can confirm existing shards by :doc:`logical_shard_list`:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/basic_usage_list_shards_before.log
.. logical_shard_list --logical_table Logs

You can remove all shards:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/basic_usage_remove_all.log
.. logical_table_remove \
..   --logical_table Logs \
..   --shard_key timestamp

There are no shards after you remove all shards:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/basic_usage_list_shards_after.log
.. logical_shard_list --logical_table Logs

.. _logical-table-remove-removes-parts:

Removes parts of a logical table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can specify range of shards by the following parameters:

  * ``min``
  * ``min_border``
  * ``max``
  * ``max_border``

See the following documents of :doc:`logical_select` for each
parameter:

  * :ref:`logical-select-min`
  * :ref:`logical-select-min-border`
  * :ref:`logical-select-max`
  * :ref:`logical-select-max-border`

If the specified range doesn't cover all records in a shard, table for
the shard isn't removed. Target records in the table are only deleted.

If the specified range covers all records in a shard, table for the
shard is removed.

Here is a logical table to show the behavior. The logical table has two
shards:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_parts_create_shards.log
.. table_create  Logs_20160318 TABLE_NO_KEY
.. column_create Logs_20160318 timestamp COLUMN_SCALAR Time
.. load --table Logs_20160318
.. [
.. {"timestamp": "2016-03-18 00:30:00"},
.. {"timestamp": "2016-03-18 01:00:00"},
.. {"timestamp": "2016-03-18 02:00:00"}
.. ]
..
.. table_create  Logs_20160319 TABLE_NO_KEY
.. column_create Logs_20160319 timestamp COLUMN_SCALAR Time
.. load --table Logs_20160319
.. [
.. {"timestamp": "2016-03-19 00:30:00"},
.. {"timestamp": "2016-03-19 01:00:00"}
.. ]

There are the following records in ``Logs_20160318`` table:

  * Record1: ``"2016-03-18 00:30:00"``
  * Record2: ``"2016-03-18 01:00:00"``
  * Record3: ``"2016-03-18 02:00:00"``

There are the following records in ``Logs_20160319`` table:

  * Record1: ``"2016-03-19 00:30:00"``
  * Record2: ``"2016-03-19 01:00:00"``

The following range doesn't cover "Record1" in ``Logs_20160318`` table
but covers all records in ``Logs_20160319`` table:

.. list-table::
   :header-rows: 1

   * - Parameter
     - Value
   * - ``min``
     - ``"2016-03-18 01:00:00"``
   * - ``min_border``
     - ``"include"``
   * - ``max``
     - ``"2016-03-19 01:30:00"``
   * - ``max_border``
     - ``"include"``

``logical_table_remove`` with the range deletes "Record2" and
"Record3" in ``Logs_20160318`` table but doesn't remove
``Logs_20160318`` table. Because there is "Record1" in
``Logs_20160318`` table.

``logical_table_remove`` with the range removes ``Logs_20160319``
table because the range covers all records in ``Logs_20160319`` table.

Here is an example to use ``logical_table_remove`` with the range:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_parts_remove.log
.. logical_table_remove \
..   --logical_table Logs \
..   --shard_key timestamp \
..   --min "2016-03-18 01:00:00" \
..   --min_border "include" \
..   --max "2016-03-19 01:30:00" \
..   --max_border "include"

:doc:`dump` shows that there is "Record1" in ``Logs_20160318`` table:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_parts_dump.log
.. dump

.. _logical-table-remove-unremovable-cases:

Unremovable cases
^^^^^^^^^^^^^^^^^

There are some unremovable cases. See
:ref:`table-remove-unremovable-cases` for details. Because
``logical_table_remove`` uses the same checks.

.. _logical-table-remove-removes-with-related-tables:

Removes with related tables
^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 6.0.1

If you understand what you'll do, you can also remove tables and
columns that depend on the target shard with one
``logical_table_remove`` command by using ``--dependent yes``
parameter.

Here are conditions for dependent. If table or column satisfies one of
the conditions, the table or column depends on the target shard:

  * Tables and columns that reference the target shard
  * Tables for the shard (= The table has the same ``_YYYYMMDD``
    postfix as the target shard and is referenced from the target
    shard)

If there are one or more tables and columns that reference the target
shard, ``logical_table_remove`` is failed. It's for avoiding dangling
references.

``Bookmarks.log_20160320`` column in the following is the column that
references the target shard:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_with_reference_create.log
.. table_create  Logs_20160320 TABLE_NO_KEY
.. column_create Logs_20160320 timestamp COLUMN_SCALAR Time
..
.. table_create Bookmarks TABLE_HASH_KEY ShortText
.. column_create Bookmarks log_20160320 COLUMN_SCALAR Logs_20160320

You can't remove ``Logs_20160320`` by ``logical_table_remove`` by
default:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_with_reference_default.log
.. logical_table_remove \
..   --logical_table Logs \
..   --shard_key timestamp

You can remove ``Logs_20160320`` by ``logical_table_remove`` with
``--dependent yes`` parameter. ``Bookmarks.log_20160320`` is also
removed:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_with_reference_dependent.log
.. logical_table_remove \
..   --logical_table Logs \
..   --shard_key timestamp \
..   --dependent yes

:doc:`object_exist` shows that ``Logs_20160320`` table and
``Bookmarks.log_20160320`` column are removed:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_with_reference_confirm.log
.. object_exist Logs_20160320
.. object_exist Bookmarks.log_20160320

If there is one or more tables for the target shard,
``logical_table_remove`` with ``--dependent yes`` also removes
them. Tables that have the same ``_YYYYMMDD`` postfix as the target
shard are treated as tables for the target shard.

Here are two tables that have ``_20160320``
postfix. ``NotRelated_20160320`` table isn't used by ``Logs_20160320``
table. ``Users_20160320`` table is used by ``Logs_20160320``
table. ``Servers`` table exists and used by ``Logs_20160320`` table:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_with_for_shard_create.log
.. table_create  NotRelated_20160320 TABLE_PAT_KEY ShortText
.. table_create  Users_20160320 TABLE_PAT_KEY ShortText
.. table_create  Servers TABLE_PAT_KEY ShortText
.. table_create  Logs_20160320 TABLE_NO_KEY
.. column_create Logs_20160320 timestamp COLUMN_SCALAR Time
.. column_create Logs_20160320 user COLUMN_SCALAR Users_20160320
.. column_create Logs_20160320 server COLUMN_SCALAR Servers

``logical_table_remove`` with ``--dependent yes`` parameter removes
only ``Logs_20160320`` table and ``Users_20160320`` table. Because
``Users_20160320`` table has ``_20160320`` postfix and used by
``Logs_20160320``. ``NotRelated_20160320`` table and ``Servers`` table
aren't removed. Because ``NotRelated_20160320`` table has
``_20160320`` postfix but isn't used by ``Logs_20160320``. ``Servers``
table is used by ``Logs_20160320`` but doesn't have ``_20160320``
postfix:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_with_for_shard_remove.log
.. logical_table_remove \
..   --logical_table Logs \
..   --shard_key timestamp \
..   --dependent yes

You can confirm that ``Logs_20160320`` table and ``Users_20160320``
table are removed but ``NotRelated_20160320`` table and ``Servers``
table aren't removed:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_with_for_shard_configm.log
.. object_exist Logs_20160320
.. object_exist Users_20160320
.. object_exist NotRelated_20160320
.. object_exist Servers

.. _logical-table-remove-removes-broken-tables-as-much-as-possible:

Removes broken tables as much as possible
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 6.0.9

If the target tables are broken, you can't remove them. But you can
remove them as much as possible by using ``force`` parameter.

If you specify ``--force yes``, they are removed as much as
possible. You can also use ``--dependent yes`` with ``--force yes``.

Here is a sample schema to show how ``--force yes`` behavior:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_broken_tables_create.log
.. table_create  Logs_20160320 TABLE_NO_KEY
.. column_create Logs_20160320 timestamp COLUMN_SCALAR Time
..
.. table_create  Timestamps TABLE_PAT_KEY Time
.. column_create Timestamps logs_20160320_timestamp \
..   COLUMN_INDEX Logs_20160320 timestamp

You can't remove ``Logs_20160320`` when
``Timestamps.logs_20160320_timestamp`` is broken:

.. groonga-command
.. thread_limit 1
.. database_unmap
.. .. ${DB_PATH}.0000109 is Timestamps.logs_20160320_timestamp.
.. .. You can confirm ID of Timestamps.logs_20160320_timestamp by object_list.
.. % echo > ${DB_PATH}.0000109

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_broken_tables_remove.log
.. logical_table_remove \
..   --logical_table Logs \
..   --shard_key timestamp

You can remove ``Logs_20160320`` and its columns by using ``--force
yes`` parameter even when ``Timestamps.logs_20160320_timestamp`` is
broken:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_broken_tables_remove_force.log
.. logical_table_remove \
..   --logical_table Logs \
..   --shard_key timestamp \
..   --force yes

``Logs_20160320``, its columns and
``Timestamps.logs_20160320_timestamp`` are removed:

.. groonga-command
.. include:: ../../example/reference/commands/logical_table_remove/remove_broken_tables_index_column_exist.log
.. object_exist Logs_20160320
.. object_exist Logs_20160320.timestamp
.. object_exist Timestamps.logs_20160320_timestamp

``--force yes`` parameter is a dangerous parameter. Normally, you
don't need to use it.

.. _logical-table-remove-decreases-used-resources:

Decreases used resources
^^^^^^^^^^^^^^^^^^^^^^^^

You can decrease resources for this command. See
:ref:`table-remove-decreases-used-resources` for details. Because
``logical_table_remove`` uses the same logic as :doc:`table_remove`.

Parameters
----------

This section describes parameters of ``logical_table_remove``.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are required parameters.

.. _logical-table-remove-logical-table:

``logical_table``
"""""""""""""""""

Specifies logical table name. It means table name without
``_YYYYMMDD`` postfix.  If you use actual table such as
``Logs_20150203``, ``Logs_20150203`` and so on, logical table name is
``Logs``.

See also :ref:`logical-select-logical-table`.

.. _logical-table-remove-shard-key:

``shard_key``
"""""""""""""

Specifies column name which is treated as shared key.

See also :ref:`logical-select-shard-key`.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

.. _logical-table-remove-min:

``min``
"""""""

Specifies the minimum value of ``shard_key`` column.

See also :ref:`logical-select-min`.

.. _logical-table-remove-min-border:

``min_border``
""""""""""""""

Specifies whether the minimum value is included or not. ``include``
and ``exclude`` are available. The default is ``include``.

See also :ref:`logical-select-min-border`.

.. _logical-table-remove-max:

``max``
"""""""

Specifies the maximum value of ``shard_key`` column.

See also :ref:`logical-select-max`.

.. _logical-table-remove-max-border:

``max_border``
""""""""""""""

Specifies whether the maximum value is included or not. ``include``
and ``exclude`` are available. The default is ``include``.

See also :ref:`logical-select-max-border`.

.. _logical-table-remove-dependent:

``dependent``
"""""""""""""

.. versionadded:: 6.0.1

Specifies whether tables and columns that depend on the target shard
are also removed or not.

Here are conditions for dependent. If table or column satisfies one of
the conditions, the table or column depends on the target shard:

  * Tables and columns that reference the target shard
  * Tables for the shard (= The table has the same ``_YYYYMMDD``
    postfix as the target shard and is referenced from the target
    shard)

If this value is ``yes``, tables and columns that depend on the target
shard are also removed. Otherwise, they aren't removed. If there are
one or more tables that reference the target shard, an error is
returned. If there are tables for the shared, they are not touched.

You should use this parameter carefully. This is a danger parameter.

See :ref:`logical-table-remove-removes-with-related-tables` how to use
this parameter.

.. _logical-table-remove-force:

``force``
"""""""""

.. versionadded:: 6.0.9

Specifies whether you want to remove target tables and columns even if
some problems exist.

You should use this parameter carefully. This is a danger parameter.

See
:ref:`logical-table-remove-removes-broken-tables-as-much-as-possible`
how to use this parameter.

Return value
------------

The command returns ``true`` as body on success such as::

  [HEADER, true]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.
