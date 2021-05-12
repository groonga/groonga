.. -*- rst -*-

.. groonga-command
.. database: alias

Alias
=====

.. versionadded:: 5.1.2

You can refer a table and column by multiple names by using alias
feature.

Summary
-------

The alias feature is useful for the following cases:

  * You want to rename a table but you can't change some Groonga
    clients that uses the current table name.

  * You want to change column type without downtime.

In the former case, some Groonga clients can use the current table
name after you rename a table. Because the alias feature maps the
current table name to the renamed new table name.

In the latter case, all Groonga clients access the column by aliased
name such as ``aliased_column``. ``aliased_column`` refers
``current_column``. You create a new column ``new_column`` with new
type and copy data from ``current_column`` by
:doc:`/reference/commands/column_copy`. You change ``aliased_column``
to refer ``new_column`` from ``current_column``. Now, all Groonga
clients access ``new_column`` by ``aliased_column`` without stopping
search requests.

Usage
-----

You manage alias to real name mapping by a normal table and a normal
column.

You can use any table type except :ref:`table-no-key` for the
table. :ref:`table-hash-key` is recommended because exact key match
search is only used for the alias feature. :ref:`table-hash-key` is
the fastest table type for exact key match search.

The column must be :doc:`/reference/columns/scalar` and type is
``ShortText``. You can also use ``Text`` and ``LongText`` types but
they are meaningless. Because the max table/column name size is
4KiB. ``ShortText`` can store 4KiB data.

Here are example definitions of table and column for managing aliases:

.. groonga-command
.. include:: ../example/reference/alias/table_and_column.log
.. table_create Aliases TABLE_HASH_KEY ShortText
.. column_create Aliases real_name COLUMN_SCALAR ShortText

You need to register the table and column by :doc:`configuration`.
The alias feature uses ``alias.column`` configuration item. You can
register the table and column by the following
:doc:`/reference/commands/config_set`:

.. groonga-command
.. include:: ../example/reference/alias/register.log
.. config_set alias.column Aliases.real_name

Here are schema and data to show how to use alias:

.. groonga-command
.. include:: ../example/reference/alias/schema.log
.. table_create Users TABLE_HASH_KEY ShortText
.. column_create Users age COLUMN_SCALAR UInt8
..
.. load --table Users
.. [
.. {"_key": "alice", "age": 14},
.. {"_key": "bob",   "age": 29}
.. ]

You can use ``Users.age`` in :doc:`/reference/commands/select`:

.. groonga-command
.. include:: ../example/reference/alias/select_age.log
.. select Users --filter 'age < 20'

You can't use ``Users.age`` when you rename ``Users.age`` to
``Users.years`` by :doc:`/reference/commands/column_rename`:

.. groonga-command
.. include:: ../example/reference/alias/select_age_after_rename.log
.. column_rename Users age years
.. select Users --filter 'age < 20'

But you can use ``Users.age`` by registering ``Users.age`` to
``Users.years`` mapping to ``Aliases``.

.. groonga-command
.. include:: ../example/reference/alias/select_age_by_alias.log
.. load --table Aliases
.. [
.. {"_key": "Users.age", "real_name": "Users.years"}
.. ]
..
.. select Users --filter 'age < 20'

Now, you can use ``Users.age`` as alias of ``Users.years``.

How to resolve alias
--------------------

This section describes how to resolve alias.

Groonga uses the alias feature when nonexistent object name (table
name, column name, command name, function name and so on) is
referred. It means that you can't override existing object (table,
column, command, function and so on) by the alias feature.

For example, alias isn't resolved in the following example because
``Users.years`` exists:

.. groonga-command
.. include:: ../example/reference/alias/existing_name.log
.. load --table Aliases
.. [
.. {"_key": "Users.years", "real_name": "Users.years_old"}
.. ]
..
.. select Users --filter 'years < 20'

Alias is resolved recursively. If you rename ``Users.years`` to
``Users.years_old`` and you refer ``Users.age``, Groonga replaces
``Users.age`` with ``Users.years`` and then ``Users.years`` with
``Users.years_old``. Because ``Aliases`` table has the following
records:

.. list-table::
   :header-rows: 1

   * - ``_key``
     - ``real_name``
   * - ``Users.age``
     - ``Users.years``
   * - ``Users.years``
     - ``Users.years_old``

Here is an example to ``Users.age`` is resolved recursively:

.. groonga-command
.. include:: ../example/reference/alias/existing_name.log
.. column_rename Users years years_old
.. select Users --filter 'age < 20'

See also
--------

  * :doc:`/reference/configuration`
  * :doc:`/reference/commands/config_set`
  * :doc:`/reference/commands/table_create`
  * :doc:`/reference/commands/column_create`
  * :doc:`/reference/commands/select`
