.. -*- rst -*-

.. groonga-command
.. database: commands_column_list

``column_list``
===============

Summary
-------

``column_list`` command lists columns in a table.

Syntax
------

This command takes only one required parameter::

  column_list table

Usage
-----

Here is a simple example of ``column_list`` command.

.. groonga-command
.. include:: ../../example/reference/commands/column_list/column_list.log
.. table_create Users TABLE_PAT_KEY ShortText
.. column_create Users age COLUMN_SCALAR UInt8
.. column_create Users tags COLUMN_VECTOR ShortText
.. column_list Users


Parameters
----------

This section describes parameters of ``column_list``.

Required parameters
^^^^^^^^^^^^^^^^^^^

All parameters are required.

``table``
"""""""""

Specifies the name of table to be listed columns.

Return value
------------

``column_list`` returns the list of column information in the table::

  [
    HEADER,
    [
      COLUMN_LIST_HEADER,
      COLUMN_INFORMATION1,
      COLUMN_INFORMATION2,
      ...
    ]
  ]

``HEADER``
^^^^^^^^^^

See :doc:`/reference/command/output_format` about ``HEADER``.

``COLUMN_LIST_HEADER``
^^^^^^^^^^^^^^^^^^^^^^

``COLUMN_LIST_HEADER`` describes about content of each
:ref:`column-list-column-information`.

``COLUMN_LIST_HEADER`` uses the following format:

.. code-block:: json

   [
     ["id",     "UInt32"],
     ["name",   "ShortText"],
     ["path",   "ShortText"],
     ["type",   "ShortText"],
     ["flags",  "ShortText"],
     ["domain", "ShortText"],
     ["range",  "ShortText"],
     ["source", "ShortText"]
     ["generator", "ShortText"]
   ]

It means the following:

* The first content in :ref:`column-list-column-information` is
  ``id`` value and the value type is ``UInt32``.
* The second content in :ref:`column-list-column-information` is
  ``name`` value and the value type is ``ShortText``.
* The third content :ref:`column-list-column-information` is ....

This field provides meta-data of column information. So this field
will be useful for programs rather than humans.

.. _column-list-column-information:

``COLUMN_INFORMATION``
^^^^^^^^^^^^^^^^^^^^^^

Each ``COLUMN_INFORMATION`` uses the following format::

  [
    ID,
    NAME,
    PATH,
    TYPE,
    FLAGS,
    DOMAIN,
    RANGE,
    SOURCES,
    GENERATOR
  ]

``ID``
""""""

The column ID in the Groonga database. Normally, you don't care about
it.

``NAME``
""""""""

The column name.

``PATH``
""""""""

The path for storing column data.

``TYPE``
""""""""

The type of the column. It is one of the followings:

.. list-table::
   :header-rows: 1

   * - Value
     - Description
   * - ``fix``
     - The column is a fixed size column. Scalar column that its
       type is fixed size type is fixed size column.
   * - ``var``
     - The column is a variable size column. Vector column or
       scalar column that its type is variable size type are
       variable size column.
   * - ``index``
     - The column is an index column.

``FLAGS``
"""""""""

The flags of the column. Each flag is separated by ``|`` like
``COLUMN_VECTOR|WITH_WEIGHT``. ``FLAGS`` must include one of
``COLUMN_SCALAR``, ``COLUMN_VECTOR`` or ``COLUMN_INDEX``. Other
flags are optional.

Here is the available flags:

.. list-table::
   :header-rows: 1

   * - Flag
     - Description
   * - ``COLUMN_SCALAR``
     - The column is a scalar column.
   * - ``COLUMN_VECTOR``
     - The column is a vector column.
   * - ``COLUMN_INDEX``
     - The column is an index column.
   * - ``WITH_WEIGHT``
     - The column can have weight. ``COLUMN_VECTOR`` and ``COLUMN_INDEX``
       may have it. ``COLUMN_SCALAR`` doesn't have it.
   * - ``WITH_SECTION``
     - The column can have section information. ``COLUMN_INDEX``
       may have it.  ``COLUMN_SCALAR`` and ``COLUMN_VECTOR`` don't
       have it.

       Multiple column index has it.
   * - ``WITH_POSITION``
     - The column can have position information. ``COLUMN_INDEX``
       may have it.  ``COLUMN_SCALAR`` and ``COLUMN_VECTOR`` don't
       have it.

       Full text search index must has it.
   * - ``PERSISTENT``
     - The column is a persistent column. It means that the column
       isn't a :doc:`/reference/columns/pseudo`.

``DOMAIN``
""""""""""

The name of table that has the column.

``RANGE``
"""""""""

The value type name of the column. It is a type name or a table name.

``SOURCES``
"""""""""""

An array of the source column names of the index column or the
generated column.

If the index column is a multiple column index, the array has two or
more source column names.

``GENERATOR``
"""""""""""""

.. versionadded:: 14.1.0

The generator expression in :doc:`../grn_expr/script_syntax`.

This is not an empty string only when the column is a
:ref:`generated-column`. This is an empty string otherwise.

See also
--------

* :doc:`/reference/commands/column_create`
* :doc:`/reference/column`
