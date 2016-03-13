.. -*- rst -*-

.. highlightlang:: none

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
