.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: commands_logical_shard_list

``logical_shard_list``
======================

Summary
-------

.. versionadded:: 5.0.7

``logical_shard_list`` returns all existing shard names against the
specified logical table name.

Syntax
------

This command takes only one required parameter::

  logical_shard_list logical_table

Usage
-----

You need to register ``sharding`` plugin to use this command:

.. groonga-command
.. include:: ../../example/reference/commands/logical_shard_list/usage_register.log
.. plugin_register sharding

Here are sample shards:

.. groonga-command
.. include:: ../../example/reference/commands/logical_shard_list/usage_shards.log
.. table_create  Logs_20150801           TABLE_HASH_KEY ShortText
.. column_create Logs_20150801 timestamp COLUMN_SCALAR  Time
.. table_create  Logs_20150802           TABLE_HASH_KEY ShortText
.. column_create Logs_20150802 timestamp COLUMN_SCALAR  Time
.. table_create  Logs_20150930           TABLE_HASH_KEY ShortText
.. column_create Logs_20150930 timestamp COLUMN_SCALAR  Time

You can get the all shard names in ascending order by specifying
``Logs`` as the logical table name:

.. groonga-command
.. include:: ../../example/reference/commands/logical_shard_list/usage_list.log
.. logical_shard_list --logical_table Logs


Parameters
----------

This section describes parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

.. _logical-shard-logical-table:

``logical_table``
"""""""""""""""""

Specifies the logical table name. ``logical_shard_list`` returns a
list of shard name of the logical table:

.. groonga-command
.. include:: ../../example/reference/commands/logical_shard_list/logical_table.log
.. logical_shard_list --logical_table Logs

The list is sorted by shard name in ascending order.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

The command returns a list of shard names in ascending order::

  [
    HEADER,
    [
      {"name": "SHARD_NAME_1"},
      {"name": "SHARD_NAME_2"},
      ...
      {"name": "SHARD_NAME_N"}
    ]
  ]

See :doc:`/reference/command/output_format` for ``HEADER``.

See also
--------

  * :doc:`/reference/sharding`
