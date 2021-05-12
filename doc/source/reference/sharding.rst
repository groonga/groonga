.. -*- rst -*-

Sharding
========

.. versionadded:: 5.0.0

Groonga has :doc:`/limitations` against table size. You can't add
268,435,455 more records in one table.

Groonga supports time based sharding to resolve the limitation.

It works in the same database. It doesn't work with multiple
databases. It means that this sharding feature isn't for distributing
large data to multiple hosts.

If you want distributed sharding feature, use `Mroonga
<http://mroonga.org/>`_ or `PGroonga
<http://pgroonga.github.io/>`_. You can use sharding feature by MySQL
or PostgreSQL. You'll be able to use `Droonga <http://droonga.org/>`_
for distributed sharding feature soon.

Summary
-------

Sharding is implemented in ``sharding`` plugin. The plugin is written
in mruby. You need to enable mruby when you build Groonga.

You can confirm whether your Groonga supports mruby or not by
``--version`` command line argument of
:doc:`/reference/executables/groonga`::

  % groonga --version
  groonga 5.0.5 [...,mruby,...]

  configure options: <...>

If you find ``mruby``, your Groonga supports mruby.

``sharding`` plugin provides only search commands. They have
``logical_`` prefix in their command names such as
:doc:`/reference/commands/logical_select` and
:doc:`/reference/commands/logical_range_filter`.

``sharding`` plugin doesn't provide schema define commands and data
load commands yet. You need to use existing commands such as
:doc:`/reference/commands/table_create`,
:doc:`/reference/commands/column_create` and
:doc:`/reference/commands/load`.

``sharding`` plugin requires some rules against table and column. You
need to follow these rules. They are described later.

.. _sharding-glossary:

Glossary
--------

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - Logical table
     - It's a table that consists of shards. It doesn't exist in
       Groonga database. It just exists in our minds.
   * - Logical table name
     - The name of logical table. It's prefix of shard names. For
       example, ``Logs`` is a logical table name and ``Logs_20150814``
       and ``Logs_20150815`` are shard names.
   * - Shard
     - It's a table that has records in a day or month.  One shard has
       only partial records.

       Shard name (= table name) must follow
       ``${LOGICAL_TABLE_NAME}_${YYYYMMDD}`` format or
       ``${LOGICAL_TABLE_NAME}_${YYYYMM}``
       format. ``${LOGICAL_TABLE_NAME}`` is expanded to logical table
       name. ``${YYYYMMDD}`` is expanded to day. ``${YYYYMM}`` is
       expanded to month.

       For example, ``Logs_20150814`` is consists of ``Logs`` logical
       name and ``20150814`` day.


.. _sharding-rules:

Rules
-----

TODO

.. _sharding-commands:

Commands
--------

* :doc:`commands/logical_count`
* :doc:`commands/logical_parameters`
* :doc:`commands/logical_range_filter`
* :doc:`commands/logical_select`
* :doc:`commands/logical_shard_list`
* :doc:`commands/logical_table_remove`
