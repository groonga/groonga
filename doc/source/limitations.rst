.. -*- rst -*-

Limitations
===========

Groonga has some limitations.

Limitations of table
--------------------

A table has the following limitations.

* The maximum one key size: 4KiB
* The maximum total size of keys:

  * 4GiB by default.

  * 1TiB by specifying ``KEY_LARGE`` flag to
    :ref:`table-create-flags`. ``KEY_LARGE`` can be used only with
    :ref:`table-hash-key`.

* The maximum number of records:

  * :ref:`table-no-key`: 1,073,741,815 (2 :sup:`30` - 9)
  * :ref:`table-hash-key`: 536,870,912 (2 :sup:`29`)
  * :ref:`table-pat-key`: 1,073,741,823 (2 :sup:`30` - 1)
  * :ref:`table-dat-key`: 268,435,455 (2 :sup:`28` - 1)

Keep in mind that these limitations may vary depending on
conditions.

For example, you need to use small size type for key to store many
records. Because the maximum total size of keys limitation is exceeded
before the maximum number of records limitation is exceeded. If you
use ``UInt64`` (8byte) type and store 2 :sup:`29` records, total key
size is 4GiB (= 8 * (2 :sup:`29`)). You can't add more records. You
need to choose decreasing key size (e.g. ``UInt32``) or using
``KEY_LARGE`` and :ref:`table-hash-key` to store more records.

Limitations of indexing
-----------------------

A full-text index has the following limitations.

* The maximum number of distinct terms: 268,435,455 (more than 268 million)
* The maximum index size: 256GiB

Keep in mind that these limitations may vary depending on conditions.

Limitations of column
---------------------

A column has the following limitation.

* The maximum stored data size of a column: 256GiB

