.. -*- rst -*-

.. groonga-command
.. database: commands_object_inspect

``object_inspect``
==================

Summary
-------

.. versionadded:: 6.0.0

``object_inspect`` inspects an object. You can confirm details of an
object.

For example:

  * If the object is a table, you can confirm the number of records in
    the table.

  * If the object is a column, you can confirm the type of value of
    the column.

Syntax
------

This command takes only one optional parameter::

  object_inspect [name=null]

Usage
-----

You can inspect an object in the database specified by ``name``:

.. groonga-command
.. include:: ../../example/reference/commands/object_inspect/usage-name.log
.. table_create Users TABLE_HASH_KEY ShortText
.. load --table Users
.. [
.. {"_key": "Alice"}
.. ]
.. object_inspect Users

The ``object_inspect Users`` returns the following information:

  * The name of the table: ``"name": Users``

  * The total used key size: ``"key": {"total_size": 5}``
    (``"Alice"`` is 5 byte data)

  * The maximum total key size: ``"key": {"max_total_size": 4294967295}``

  * and so on.

You can inspect the database by not specifying ``name``:

.. groonga-command
.. include:: ../../example/reference/commands/object_inspect/usage-database.log
.. object_inspect

The ``object_inspect`` returns the following information:

  * The table type for object name management:
    ``"key": {"type": {"name": "table:dat_key"}}``

  * and so on.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is no required parameter.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is only one optional parameter.

.. _object-inspect-name:

``name``
""""""""

Specifies the object name to be inspected.

If ``name`` isn't specified, the database is inspected.

Return value
------------

The command returns an object (nested key and value pairs) that
includes details of the object (such as table) as body::

  [HEADER, object]

See :doc:`/reference/command/output_format` for ``HEADER``.

The format of the details is depends on object type. For example,
table has key information but function doesn't have key information.

.. _object-inspect-return-value-database:

Database
^^^^^^^^

Database inspection returns the following information::

  {
    "type": {
      "id": DATABASE_TYPE_ID,
      "name": DATABASE_TYPE_NAME
    },
    "name_table": DATABASE_NAME_TABLE
  }

.. _object-inspect-return-value-database-type-id:

``DATABASE_TYPE_ID``
""""""""""""""""""""

``DATABASE_TYPE_ID`` is always ``55``.

.. _object-inspect-return-value-database-type-name:

``DATABASE_TYPE_NAME``
""""""""""""""""""""""

``DATABASE_TYPE_NAME`` is always ``"db"``.

.. _object-inspect-return-value-database-name-table:

``DATABASE_NAME_TABLE``
"""""""""""""""""""""""

``DATABASE_NAME_TABLE`` is a table for managing object names in the
database. The table is :ref:`table-pat-key` or
:ref:`table-dat-key`. Normally, it's :ref:`table-dat-key`.

See :ref:`object-inspect-return-value-table` for format details.

.. _object-inspect-return-value-table:

Table
^^^^^

Table inspection returns the following information::

  {
    "id": TABLE_ID,
    "name": TABLE_NAME,
    "type": {
      "id": TABLE_TYPE_ID,
      "name": TABLE_TYPE_NAME
    },
    "key": {
      "type": TABLE_KEY_TYPE,
      "total_size": TABLE_KEY_TOTAL_SIZE
      "max_total_size": TABLE_KEY_MAX_TOTAL_SIZE
    },
    "value": {
      "type": TABLE_VALUE_TYPE,
    },
    "n_records": TABLE_N_RECORDS
  }

There are some exceptions:

  * :ref:`table-no-key` doesn't return key information because it
    doesn't have key.

  * :ref:`table-dat-key` doesn't return value information because it
    doesn't have value.

.. _object-inspect-return-value-table-id:

``TABLE_ID``
""""""""""""

The ID of the inspected table.

.. _object-inspect-return-value-table-name:

``TABLE_NAME``
""""""""""""""

The name of the inspected table.

.. _object-inspect-return-value-table-type-id:

``TABLE_TYPE_ID``
"""""""""""""""""

The type ID of the inspected table.

Here is a list of type IDs:

.. list-table::
   :header-rows: 1

   * - Table type
     - ID
   * - :ref:`table-hash-key`
     - ``48``
   * - :ref:`table-pat-key`
     - ``49``
   * - :ref:`table-dat-key`
     - ``50``
   * - :ref:`table-no-key`
     - ``51``

.. _object-inspect-return-value-table-type-name:

``TABLE_TYPE_NAME``
"""""""""""""""""""

The type name of the inspected table.

Here is a list of type names:

.. list-table::
   :header-rows: 1

   * - Table type
     - Name
   * - :ref:`table-hash-key`
     - ``"table:hash_key"``
   * - :ref:`table-pat-key`
     - ``"table:pat_key"``
   * - :ref:`table-dat-key`
     - ``"table:dat_key"``
   * - :ref:`table-no-key`
     - ``"table:no_key"``

.. _object-inspect-return-value-table-key-type:

``TABLE_KEY_TYPE``
""""""""""""""""""

The type of key of the inspected table.

See :ref:`object-inspect-return-value-type` for format details.

.. _object-inspect-return-value-table-total-key-size:

``TABLE_KEY_TOTAL_SIZE``
""""""""""""""""""""""""

The total key size of the inspected table in bytes.

.. _object-inspect-return-value-table-max-total-key-size:

``TABLE_KEY_MAX_TOTAL_SIZE``
""""""""""""""""""""""""""""

The maximum total key size of the inspected table in bytes.

.. _object-inspect-return-value-table-value-type:

``TABLE_VALUE_TYPE``
""""""""""""""""""""

The type of value of the inspected table.

See :ref:`object-inspect-return-value-type` for format details.

.. _object-inspect-return-value-table-n-records:

``TABLE_N_RECORDS``
"""""""""""""""""""

The number of records of the inspected table.

It's a 64bit unsigned integer value.

.. _object-inspect-return-value-column:

Column
^^^^^^

.. versionadded:: 7.0.2

Data column (scalar column and vector column) returns the following
information::

  {
    "id": COLUMN_ID,
    "name": COLUMN_NAME
    "table": COLUMN_TABLE,
    "full_name": COLUMN_FULL_NAME,
    "type": {
      "name": COLUMN_TYPE_NAME,
      "raw": {
        "id": COLUMN_TYPE_RAW_ID,
        "name": COLUMN_TYPE_RAW_NAME
      }
    },
    "value": {
      "type": COLUMN_VALUE_TYPE,
      "compress": DATA_COLUMN_VALUE_COMPRESS_METHOD,
    }
  }

Index column is similar to data column but there are some differences.

  * Index column doesn't have ``value.compress`` key.

  * Index column has ``value.section`` key.

  * Index column has ``value.weight`` key.

  * Index column has ``value.position`` key.

  * Index column has ``value.size`` key.

  * Index column has ``value.statistics`` key.

  * Index column has ``sources`` key.

Index column returns the following information::

  {
    "id": COLUMN_ID,
    "name": COLUMN_NAME
    "table": COLUMN_TABLE,
    "full_name": COLUMN_FULL_NAME,
    "type": {
      "name": COLUMN_TYPE_NAME,
      "raw": {
        "id": COLUMN_TYPE_RAW_ID,
        "name": COLUMN_TYPE_RAW_NAME
      }
    },
    "value": {
      "type": COLUMN_VALUE_TYPE,
      "section": INDEX_COLUMN_VALUE_SECTION,
      "weight": INDEX_COLUMN_VALUE_WEIGHT,
      "position": INDEX_COLUMN_VALUE_POSITION,
      "size": INDEX_COLUMN_VALUE_SIZE,
      "statistics": {
        "max_section_id": INDEX_COLUMN_VALUE_STATISTICS_MAX_SECTION_ID,
        "n_garbage_segments": INDEX_COLUMN_VALUE_STATISTICS_N_GARBAGE_SEGMENTS,
        "max_array_segment_id": INDEX_COLUMN_VALUE_STATISTICS_MAX_ARRAY_SEGMENT_ID,
        "n_array_segments": INDEX_COLUMN_VALUE_STATISTICS_N_ARRAY_SEGMENTS,
        "max_buffer_segment_id": INDEX_COLUMN_VALUE_STATISTICS_MAX_BUFFER_SEGMENT_ID,
        "n_buffer_segments": INDEX_COLUMN_VALUE_STATISTICS_N_BUFFER_SEGMENTS,
        "max_in_use_physical_segment_id": INDEX_COLUMN_VALUE_STATISTICS_MAX_IN_USE_PHYSICAL_SEGMENT_ID,
        "n_unmanaged_segments": INDEX_COLUMN_VALUE_STATISTICS_N_UNMANAGED_SEGMENTS,
        "total_chunk_size": INDEX_COLUMN_VALUE_STATISTICS_TOTAL_CHUNK_SIZE,
        "max_in_use_chunk_id": INDEX_COLUMN_VALUE_STATISTICS_MAX_IN_USE_CHUNK_ID,
        "n_garbage_chunks": INDEX_COLUMN_VALUE_STATISTICS_N_GARBAGE_CHUNKS
        "next_physical_segment_id": INDEX_COLUMN_VALUE_STATISTICS_NEXT_PHYSICAL_SEGMENT_ID
        "max_n_physical_segments": INDEX_COLUMN_VALUE_STATISTICS_N_PHYSICAL_SEGMENTS
      }
    },
    "sources": [
      {
        "id": INDEX_COLUMN_SOURCE_ID,
        "name": INDEX_COLUMN_SOURCE_NAME,
        "table": INDEX_COLUMN_SOURCE_TABLE,
        "full_name": INDEX_COLUMN_SOURCE_FULL_NAME
      },
      ...
    ]
  }

.. _object-inspect-return-value-column-id:

``COLUMN_ID``
"""""""""""""

The ID of the inspected column.

.. _object-inspect-return-value-column-name:

``COLUMN_NAME``
"""""""""""""""

The name of the inspected column.

It doesn't include table name. It's just only column name.

If you want full column name (``TABLE_NAME.COLUMN_NAME`` style), use
:ref:`object-inspect-return-value-column-full-name` instead.

.. _object-inspect-return-value-column-table:

``COLUMN_TABLE``
""""""""""""""""

The table of the inspected column.

See :ref:`object-inspect-return-value-table` for format details.

.. _object-inspect-return-value-column-full-name:

``COLUMN_FULL_NAME``
""""""""""""""""""""

The full name of the inspected column.

It includes both table name and column name as
``TABLE_NAME.COLUMN_NAME`` format.

If you just want only column name, use
:ref:`object-inspect-return-value-column-name` instead.

.. _object-inspect-return-value-column-type-name:

``COLUMN_TYPE_NAME``
""""""""""""""""""""

The type name of the inspected column.

Here is a list of type names:

.. list-table::
   :header-rows: 1

   * - Column type
     - Name
   * - :doc:`/reference/columns/scalar`
     - ``"scalar"``
   * - :doc:`/reference/columns/vector`
     - ``"vector"``
   * - :doc:`/reference/columns/index`
     - ``"index"``

.. _object-inspect-return-value-column-type-raw-id:

``COLUMN_TYPE_RAW_ID``
""""""""""""""""""""""

The raw type ID of the inspected column.

Here is a list of raw type IDs:

.. list-table::
   :header-rows: 1

   * - Raw column type
     - ID
   * - Fix size column
     - ``64``
   * - Variable size column
     - ``65``
   * - Index column
     - ``72``

.. _object-inspect-return-value-column-type-raw-name:

``COLUMN_TYPE_RAW_NAME``
""""""""""""""""""""""""

The raw type name of the inspected column.

Here is a list of raw type names:

.. list-table::
   :header-rows: 1

   * - Raw column type
     - Name
   * - Fix size column
     - ``"column:fix_size"``
   * - Variable size column
     - ``"column:var_size"``
   * - Index column
     - ``"column:index"``

.. _object-inspect-return-value-column-value-type:

``COLUMN_VALUE_TYPE``
"""""""""""""""""""""

The type of value of the inspected column.

See :ref:`object-inspect-return-value-type` for format details.

.. _object-inspect-return-value-data-column-value-compress-method:

``DATA_COLUMN_VALUE_COMPRESS_METHOD``
"""""""""""""""""""""""""""""""""""""

The compress method of value of the inspected data column.

Here is a list of compress methods:

.. list-table::
   :header-rows: 1

   * - Compress method
     - Value
   * - zlib
     - ``"zlib"``
   * - LZ4
     - ``"lz4"``
   * - Zstandard
     - ``"zstd"``
   * - None
     - ``null``

.. _object-inspect-return-value-index-column-value-section:

``INDEX_COLUMN_VALUE_SECTION``
""""""""""""""""""""""""""""""

Whether the inspected column is created with ``WITH_SECTION`` flag or
not.  The value is ``true`` if ``WITH_SECTION`` was specified,
``false`` otherwise.

.. seealso:: :ref:`column-create-flags`

.. _object-inspect-return-value-index-column-value-weight:

``INDEX_COLUMN_VALUE_WEIGHT``
"""""""""""""""""""""""""""""

Whether the inspected column is created with ``WITH_WEIGHT`` flag or
not.  The value is ``true`` if ``WITH_WEIGHT`` was specified,
``false`` otherwise.

.. seealso:: :ref:`column-create-flags`

             .. _object-inspect-return-value-index-column-value-position:

``INDEX_COLUMN_VALUE_POSITION``
"""""""""""""""""""""""""""""""

Whether the inspected column is created with ``WITH_POSITION`` flag or
not.  The value is ``true`` if ``WITH_POSITION`` was specified,
``false`` otherwise.

.. seealso:: :ref:`column-create-flags`

.. _object-inspect-return-value-index-column-value-size:

``INDEX_COLUMN_VALUE_SIZE``
"""""""""""""""""""""""""""

The size of the inspected index column. Index size can be specified by
:ref:`column-create-flags`.

Here is a list of index column sizes:

.. list-table::
   :header-rows: 1

   * - Index column size
     - Value
   * - ``INDEX_SMALL``
     - ``"small"``
   * - ``INDEX_MEDIUM``
     - ``"medium"``
   * - ``INDEX_LARGE``
     - ``"large"``
   * - Default
     - ``"normal"``

.. _object-inspect-return-value-index-column-value-statistics-max-section-id:

``INDEX_COLUMN_VALUE_STATISTICS_MAX_SECTION_ID``
""""""""""""""""""""""""""""""""""""""""""""""""

The max section ID in the inspected index column.

It's always ``0`` for index column that is created without
``WITH_SECTION`` flag.

It's ``0`` or larger for index column that is created with
``WITH_SECTION`` flag. It's ``0`` for empty ``WITH_SECTION`` index
column. It's ``1`` or larger for non-empty ``WITH_SECTION`` index
column.

The max value for ``WITH_SECTION`` index column is the number of
source columns.

.. _object-inspect-return-value-index-column-value-statistics-n-garbage-segments:

``INDEX_COLUMN_VALUE_STATISTICS_N_GARBAGE_SEGMENTS``
""""""""""""""""""""""""""""""""""""""""""""""""""""

The number of garbage segments in the inspected index column.

Index column reuses segment (internal allocated space) that is no
longer used. It's called "garbage segment".

The max value is the max number of segments. See
:ref:`object-inspect-return-value-index-column-value-statistics-n-physical-segments`
for the max number of segments.

.. _object-inspect-return-value-index-column-value-statistics-max-array-segment-id:

``INDEX_COLUMN_VALUE_STATISTICS_MAX_ARRAY_SEGMENT_ID``
""""""""""""""""""""""""""""""""""""""""""""""""""""""

The max ID of segment used for "array" in the inspected index column.

"array" is used for managing "buffer".

The max value is the max number of segments. See
:ref:`object-inspect-return-value-index-column-value-statistics-n-physical-segments`
for the max number of segments.

.. _object-inspect-return-value-index-column-value-statistics-n-array-segments:

``INDEX_COLUMN_VALUE_STATISTICS_N_ARRAY_SEGMENTS``
""""""""""""""""""""""""""""""""""""""""""""""""""

The number of segments used for "array" in the inspected index column.

"array" is used for managing "buffer".

The max value is ``the max number of segments - the number of segments
used for "buffer"``. See
:ref:`object-inspect-return-value-index-column-value-statistics-n-physical-segments`
for the max number of segments.

.. _object-inspect-return-value-index-column-value-statistics-max-buffer-segment-id:

``INDEX_COLUMN_VALUE_STATISTICS_MAX_BUFFER_SEGMENT_ID``
"""""""""""""""""""""""""""""""""""""""""""""""""""""""

The max ID of segment used for "buffer" in the inspected index column.

"buffer" is used for storing posting lists.

The max value is the max number of segments. See
:ref:`object-inspect-return-value-index-column-value-statistics-n-physical-segments`
for the max number of segments.

.. _object-inspect-return-value-index-column-value-statistics-n-buffer-segments:

``INDEX_COLUMN_VALUE_STATISTICS_N_BUFFER_SEGMENTS``
"""""""""""""""""""""""""""""""""""""""""""""""""""

The number of segments used for "buffer" in the inspected index column.

"buffer" is used for storing posting lists.

The max value is ``the max number of segments - the number of segments
used for "array"``. See
:ref:`object-inspect-return-value-index-column-value-statistics-n-physical-segments`
for the max number of segments.

.. _object-inspect-return-value-index-column-value-statistics-max-in-use-physical-segment-id:

``INDEX_COLUMN_VALUE_STATISTICS_MAX_IN_USE_PHYSICAL_SEGMENT_ID``
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

The max segment ID in use as "garbage", "array" or "buffer" in the
inspected index column.

The max value is the max number of segments. See
:ref:`object-inspect-return-value-index-column-value-statistics-n-physical-segments`
for the max number of segments.

.. _object-inspect-return-value-index-column-value-statistics-n-unmanaged-segments:

``INDEX_COLUMN_VALUE_STATISTICS_N_UNMANAGED_SEGMENTS``
""""""""""""""""""""""""""""""""""""""""""""""""""""""

The number of unmanaged segments in the inspected index column.

It must be ``0``.

.. _object-inspect-return-value-index-column-value-statistics-total-chunk-size:

``INDEX_COLUMN_VALUE_STATISTICS_TOTAL_CHUNK_SIZE``
""""""""""""""""""""""""""""""""""""""""""""""""""

The total "chunk" size in the inspected index column.

"chunk" is used for storing posting lists. "buffer" is mutable but
"chunk" is immutable. "chunk" is more space effective than
"buffer". "buffer" is more update effective than "chunk".

Small posting lists are stored into "buffer". Posting lists in
"buffer" are moved to "chunk" when these posting lists are grew.

The max value is ``the max size of a chunk * the max number of
chunks``. But you will not be able to use all spaces because there are
overheads.

The max size of a chunk is ``2 ** 22`` bytes (4MiB). The max
number of chunks depend on index size:

.. list-table::
   :header-rows: 1

   * - Index column size
     - The max number of chunks
   * - ``INDEX_SMALL``
     - ``2**10`` (1024)
   * - ``INDEX_MEDIUM``
     - ``2**14`` (16384)
   * - Default
     - ``2**18`` (262144)

.. _object-inspect-return-value-index-column-value-statistics-max-in-use-chunk-id:

``INDEX_COLUMN_VALUE_STATISTICS_MAX_IN_USE_CHUNK_ID``
"""""""""""""""""""""""""""""""""""""""""""""""""""""

The max "chunk" ID in use in the inspected index column.

The max value is the max number of chunks. See
:ref:`object-inspect-return-value-index-column-value-statistics-total-chunk-size`
for the max number of chunks.

.. _object-inspect-return-value-index-column-value-statistics-n-garbage-chunks:

``INDEX_COLUMN_VALUE_STATISTICS_N_GARBAGE_CHUNKS``
""""""""""""""""""""""""""""""""""""""""""""""""""

The array of the number of garbage "chunks" in the inspected index
column.

Garbage "chunks" are managed by separated 14 spaces. It shows all the
number of garbage "chunks" as an array like the following::

  [
    N_GARBAGE_CHUNKS_IN_SPACE0,
    N_GARBAGE_CHUNKS_IN_SPACE1,
    ...
    N_GARBAGE_CHUNKS_IN_SPACE13
  ]

The max value of each space is the max number of chunks. See
:ref:`object-inspect-return-value-index-column-value-statistics-total-chunk-size`
for the max number of chunks.

.. _object-inspect-return-value-index-column-value-statistics-next-physical-segment-id:

``INDEX_COLUMN_VALUE_STATISTICS_NEXT_PHYSICAL_SEGMENT_ID``
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 9.0.2

This value is the ID of the segment. The inspected index column use it as the next segment ID.
The max value is the max number of segments. See
:ref:`object-inspect-return-value-index-column-value-statistics-n-physical-segments`
for the max number of segments.

.. _object-inspect-return-value-index-column-value-statistics-n-physical-segments:

``INDEX_COLUMN_VALUE_STATISTICS_N_PHYSICAL_SEGMENTS``
"""""""""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 9.0.2

This value the max number of segments. It depends on index size:

.. list-table::
   :header-rows: 1

   * - Index column size
     - The max number of segments
   * - ``INDEX_SMALL``
     - ``2**9`` (512)
   * - ``INDEX_MEDIUM``
     - ``2**16`` (65536)
   * - ``INDEX_LARGE``
     - ``2**17 * 2`` (262144)
   * - Default
     - ``2**17`` (131072)

If the number of segments tend to exceeds near the future, you need to consider to adopt ``INDEX_XXX`` flags.

.. _object-inspect-return-value-index-column-source-id:

``INDEX_COLUMN_SOURCE_ID``
""""""""""""""""""""""""""

The ID of a source column of the inspected index column.

.. _object-inspect-return-value-index-column-source-name:

``INDEX_COLUMN_SOURCE_NAME``
""""""""""""""""""""""""""""

The name of a source column of the inspected index column.

It doesn't include table name. It's just only column name.

If you want full column name (``TABLE_NAME.COLUMN_NAME`` style), use
:ref:`object-inspect-return-value-index-column-source-full-name`
instead.

.. _object-inspect-return-value-index-column-source-table:

``INDEX_COLUMN_SOURCE_TABLE``
"""""""""""""""""""""""""""""

The table of a source column of the inspected index column.

See :ref:`object-inspect-return-value-table` for format details.

.. _object-inspect-return-value-index-column-source-full-name:

``INDEX_COLUMN_SOURCE_FULL_NAME``
"""""""""""""""""""""""""""""""""

The full name of a source column of the inspected index column.

It includes both table name and column name as
``TABLE_NAME.COLUMN_NAME`` format.

If you just want only column name, use
:ref:`object-inspect-return-value-index-column-source-name` instead.


.. _object-inspect-return-value-type:

Type
""""

Type inspection returns the following information::

  {
    "id": TYPE_ID,
    "name": TYPE_NAME,
    "type": {
      "id": TYPE_ID_OF_TYPE,
      "name": TYPE_NAME_OF_TYPE
    },
    "size": TYPE_SIZE
  }

.. _object-inspect-return-value-type-id:

``TYPE_ID``
"""""""""""

The ID of the inspected type.

Here is an ID list of builtin types:

.. list-table::
   :header-rows: 1

   * - Type
     - ID
   * - :ref:`builtin-type-bool`
     - ``3``
   * - :ref:`builtin-type-int8`
     - ``4``
   * - :ref:`builtin-type-uint8`
     - ``5``
   * - :ref:`builtin-type-int16`
     - ``6``
   * - :ref:`builtin-type-uint16`
     - ``7``
   * - :ref:`builtin-type-int32`
     - ``8``
   * - :ref:`builtin-type-uint32`
     - ``9``
   * - :ref:`builtin-type-int64`
     - ``10``
   * - :ref:`builtin-type-uint64`
     - ``11``
   * - :ref:`builtin-type-float`
     - ``12``
   * - :ref:`builtin-type-time`
     - ``13``
   * - :ref:`builtin-type-short-text`
     - ``14``
   * - :ref:`builtin-type-text`
     - ``15``
   * - :ref:`builtin-type-long-text`
     - ``16``
   * - :ref:`builtin-type-tokyo-geo-point`
     - ``17``
   * - :ref:`builtin-type-wgs84-geo-point`
     - ``18``

.. _object-inspect-return-value-type-name:

``TYPE_NAME``
"""""""""""""

The name of the inspected type.

Here is a name list of builtin types:

  * :ref:`builtin-type-bool`
  * :ref:`builtin-type-int8`
  * :ref:`builtin-type-uint8`
  * :ref:`builtin-type-int16`
  * :ref:`builtin-type-uint16`
  * :ref:`builtin-type-int32`
  * :ref:`builtin-type-uint32`
  * :ref:`builtin-type-int64`
  * :ref:`builtin-type-uint64`
  * :ref:`builtin-type-float`
  * :ref:`builtin-type-time`
  * :ref:`builtin-type-short-text`
  * :ref:`builtin-type-text`
  * :ref:`builtin-type-long-text`
  * :ref:`builtin-type-tokyo-geo-point`
  * :ref:`builtin-type-wgs84-geo-point`

.. _object-inspect-return-value-type-id-of-type:

``TYPE_ID_OF_TYPE``
"""""""""""""""""""

``TYPE_ID_OF_TYPE`` is always ``32``.

.. _object-inspect-return-value-type-name-of-type:

``TYPE_NAME_OF_TYPE``
"""""""""""""""""""""

``TYPE_NAME_OF_TYPE`` is always ``type``.

.. _object-inspect-return-value-type-size:

``TYPE_SIZE``
"""""""""""""

``TYPE_SIZE`` is the size of the inspected type in bytes. If the
inspected type is variable size type, the size means the maximum size.
