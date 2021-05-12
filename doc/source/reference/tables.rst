.. -*- rst -*-

Tables
======

Summary
-------

Table in Groonga manages relation between ID and key. Groonga provides
four table types. They are ``TABLE_NO_KEY``, ``TABLE_HASH_KEY``,
``TABLE_PAT_KEY`` and ``TABLE_DAT_KEY``.

All tables except ``TABLE_NO_KEY`` provides both fast ID search by key
and fast key search by ID. ``TABLE_NO_KEY`` doesn't support
key. ``TABLE_NO_KEY`` only manages ID. So ``TABLE_NO_KEY`` doesn't
provides ID search and key search.

Characteristics
---------------

Here is a chracteristic table of all tables in Groonga. (``TABLE_``
prefix is omitted in the table.)

.. list-table:: Characteristics of all tables
   :header-rows: 1

   * - Item
     - ``NO_KEY``
     - ``HASH_KEY``
     - ``PAT_KEY``
     - ``DAT_KEY``
   * - Data structure
     - Array
     - Hash table
     - Patricia trie
     - Double array trie
   * - ID support
     - o
     - o
     - o
     - o
   * - Key support
     - x
     - o
     - o
     - o
   * - Value support
     - o
     - o
     - o
     - x
   * - Key -> ID speed

       * o: fast
       * x: slow
     - \-
     - oo
     - x
     - o
   * - Update speed

       * o: fast
       * x: slow
     - ooo
     - o
     - o
     - x
   * - Size

       * o: small
       * x: large
     - ooo
     - o
     - oo
     - x
   * - Key update
     - \-
     - x
     - x
     - o
   * - Common prefix search
     - \-
     - x
     - o
     - o
   * - Predictive search
     - \-
     - x
     - o
     - o
   * - Range search
     - \-
     - x
     - o
     - o
   * - The maximum one key size
     - \-
     - 4KiB
     - 4KiB
     - 4KiB
   * - The maximum total size of keys
     - \-
     - 4GiB or 1TiB (by specifying ``KEY_LARGE`` flag to
       :ref:`table-create-flags`)
     - 4GiB
     - 4GiB
   * - The maximum number of records
     - 1,073,741,815 (2 :sup:`30` - 9)
     - 536,870,912 (2 :sup:`29`)
     - 1,073,741,823 (2 :sup:`30` - 1)
     - 268,435,455 (2 :sup:`28` - 1)

.. _table-no-key:

``TABLE_NO_KEY``
^^^^^^^^^^^^^^^^

``TABLE_NO_KEY`` is very fast and very small but it doesn't support
key. ``TABLE_NO_KEY`` is a only table that doesn't support key.

You cannot use ``TABLE_NO_KEY`` for lexicon for fulltext search
because lexicon stores tokens as key. ``TABLE_NO_KEY`` is useful for
no key records such as log.

.. _table-hash-key:

``TABLE_HASH_KEY``
^^^^^^^^^^^^^^^^^^

``TABLE_HASH_KEY`` is fast but it doesn't support advanced search
functions such as common prefix search and predictive search.

``TABLE_HASH_KEY`` is useful for index for exact search such as tag
search.

.. _table-pat-key:

``TABLE_PAT_KEY``
^^^^^^^^^^^^^^^^^

``TABLE_PAT_KEY`` is small and supports advanced search functions.

``TABLE_PAT_KEY`` is useful for lexicon for fulltext search and
index for range search.

.. _table-dat-key:

``TABLE_DAT_KEY``
^^^^^^^^^^^^^^^^^

``TABLE_DAT_KEY`` is fast and supports key update but it is large. It
is not suitable for storing many records. ``TABLE_DAT_KEY`` is a only
table that supports key update.

``TABLE_DAT_KEY`` is used in Groonga database. Groonga database needs
to convert object name such as ``ShortText``, ``TokenBigram`` and
table names to object ID. And Groonga database needs to rename object
name. Those features are implemented by ``TABLE_DAT_KEY``. The number
of objects is small. So large data size demerit of ``TABLE_DAT_KEY``
can be ignored.

Record ID
---------

Record ID is assigned automatically. You cannot assign record ID.

Record ID of deleted record may be reused.

Valid record ID range is between 1 and 1073741823. (1 and 1073741823
are valid IDs.)

Persistent table and temporary table
------------------------------------

Table is persistent table or temporary table.

Persistent table
^^^^^^^^^^^^^^^^

Persistent table is named and registered to database. Records in
persistent table aren't deleted after closing table or
database.

Persistent table can be created by
:doc:`/reference/commands/table_create` command.

Temporary table
^^^^^^^^^^^^^^^

Temporary table is anonymous. Records in temporary table are deleted
after closing table. Temporary table is used to store search result,
sort result, group (drilldown) result and so on. ``TABLE_HASH_KEY`` is
used for search result and group result. ``TABLE_NO_KEY`` is used for
sort result.

Limitations
-----------

The max number of records is 268435455. You cannot add 268435456 or
more records in a table.

The max number of a key size is 4096byte. You cannot use 4097byte or
larger key. You can use column instead of key for 4097byte or larger
size data. ``Text`` and ``LargeText`` types supports 4097byte or
larger size data.

The max number of total key size is 4GiB. You need to split a table,
split a database (sharding) or reduce each key size to handle 4GiB or
more larger total key size.

See also
--------

* :doc:`/reference/commands/table_create`
