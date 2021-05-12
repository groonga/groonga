.. -*- rst -*-

.. groonga-command
.. database: commands_load

``load``
========

Summary
-------


``load`` loads data as records in the current database and updates values of each columns.

Syntax
------

The required parameters are only ``values`` and ``table``. Other
parameters are optional::

  load values
       table
       [columns=null]
       [ifexists=null]
       [input_type=json]
       [each=null]
       [output_ids=no]
       [output_errors=no]
       [lock_table=no]

This command is a special command. Other commands need to pass all
parameters to one line but this command can accept ``values`` as
followed data.

If you use command line style, you can pass ``values`` like the
following::

  load --table Bookmarks
  [
  {"_key": "http://groonga.org/", "title": "Groonga"},
  {"_key": "http://mroonga.org/", "title": "Mroonga"}
  ]

``[...]`` is value of ``values``.

If you use HTTP style, you can pass ``values`` as body::

  % curl \
      --request POST \
      --header "Content-Type: application/json" \
      --data-raw '[{"_key": "http://groonga.org/"}]' \
      http://localhost:10041/d/load?table=Bookmarks"


Usage
-----

Here is a schema definition to show usage:

.. groonga-command
.. include:: ../../example/reference/commands/load/usage_setup.log
.. table_create Entries TABLE_HASH_KEY ShortText
.. column_create Entries content COLUMN_SCALAR Text

Here is an example to add records to ``Entries`` table by parameter:

.. groonga-command
.. include:: ../../example/reference/commands/load/usage_parameter.log
.. load \
..   --table Entries \
..   --values "[{\"_key\":\"Groonga\",\"content\":\"It's very fast!!\"}]"

Here is an example to add records to ``Entries`` table from standard input:

.. groonga-command
.. include:: ../../example/reference/commands/load/usage_standard_input.log
.. load --table Entries
.. [
.. {"_key": "Groonga", "content": "It's very fast!!"}
.. ]

Here is an example to lock table while updating columns:

.. groonga-command
.. include:: ../../example/reference/commands/load/usage_lock_table.log
.. load --table Entries --lock_table yes
.. [
.. {"_key": "Groonga", "content": "It's very fast!!"}
.. ]

Parameters
----------

This section describes all parameters. Parameters are categorized.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are some required parameters.

.. _load-values:

``values``
""""""""""

Specifies values to be loaded.

Values should satisfy ``input_type`` format. If you specify ``json``
as ``input_type``, you can choose a format from below:

Bracket style::

  [
  [COLUMN_NAME1, COLUMN_NAME2, ...],
  [VALUE1, VALUE2, ...],
  [VALUE1, VALUE2, ...],
  ...
  ]

Brace style::

  [
  {"COLUMN_NAME1": VALUE1, "COLUMN_NAME2": VALUE2, ...},
  {"COLUMN_NAME1": VALUE1, "COLUMN_NAME2": VALUE2, ...},
  ...
  ]

``[COLUMN_NAME1, COLUMN_NAME2, ...]`` in bracket style is effective
only when :ref:`load-columns` parameter isn't specified.

When a target table contains primary key, you must specify ``_key``
column (pseudo column associated primary key) as the one of
``COLUMN_NAME``.

If you specify ``apache-arrow`` as ``input_type``, you must use
`Apache Arrow IPC Streaming Format`_. You can't use `Apache Arrow IPC
File Format`_.

You must use HTTP interface to use Apache Arrow. You can't use Apache
Arrow in command line interface.

You must set ``application/x-apache-arrow-streaming`` to
``Content-Type`` HTTP request header.

You must choose suitable record batch size. Groonga loads data per
record batch. If you choose very large record batch size, Groonga
can't start loading until whole data of a record batch are received.
If you choose very small record batch size, Groonga can load data
incrementally but overhead will be large. Suitable record batch size
depends on your system but 1024 or so will be suitable.

If ``values`` isn't specified any values, they are read from the
standard input in command line style or body in HTTP style.

.. _load-table:

``table``
"""""""""

Specifies a table name you want to add records.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are some optional parameters.

.. _load-columns:

``columns``
"""""""""""

Specifies column names in added records with comma separations.

.. _load-ifexists:

``ifexists``
""""""""""""

Specifies executed expression in
:doc:`/reference/grn_expr/script_syntax` when the same primary key as
added records already exists in your table.

If ``ifexists`` specifies expression and its value is ``true``, values
in other (all columns excluding ``_key`` column) columns is updated.

.. _load-input-type:

``input_type``
""""""""""""""

Specifies an input format for ``values``.

Note that you must also specify suitable HTTP ``Content-Type`` header
value when you use ``input_type`` with HTTP interface.

Here are available types and ``Content-Type`` values:

.. list-table::
   :header-rows: 1

   * - Type
     - ``Content-Type``
     - Description
   * - ``json``
     - ``application/json``
     - Use JSON for ``values`` format.

       This is the default.
   * - ``apache-arrow``
     - ``application/x-apache-arrow-streaming``
     - .. versionadded:: 9.1.1

       Use Apache Arrow for ``values`` format.

.. _load-each:

``each``
""""""""

TODO

.. _load-output-ids:

``output_ids``
""""""""""""""

TODO

.. _load-output-errors:

``output_errors``
"""""""""""""""""

TODO

.. _load-lock-table:

``lock_table``
""""""""""""""

.. versionadded:: 8.0.6

Specifies whether locking table while updating columns.

The default is ``no``.

If you may run destructive commands such as ``load``, ``delete`` and
so on concurrently, it may break database. For example, if you're
updating a record by ``load`` and deleting the updating record by
``delete``, the ``load`` may refer the delete record.

You can guard the update conflict by locking the target table but it
reduces load performance.

If you specify ``yes`` to this parameter, you can lock the target
table while updating columns. Here is the update sequence of each
record:

  1. Lock the target table
  2. Add or refer a record to the target table
  3. Unlock the target table
  4. Lock the target table when ``lock_table`` is ``yes``
  5. Update columns of the target record
  6. Unlock the target table when ``lock_table`` is ``yes``

Return value
------------

The command returns a response with the following format::

  [THE_NUMBER_OF_LOADED_RECORDS]

The command returns a response with the following format with
:doc:`/reference/command/command_version` 3 or later::

  {
    "n_loaded_records": THE_NUMBER_OF_LOADED_RECORDS,
    "loaded_ids": [
      LOADED_RECORD'S_ID1,
      LOADED_RECORD'S_ID2,
      ...
    ],
    "errors": [
      {
        "return_code": RETURN_CODE_FOR_1ST_RECORD,
        "message": MESSAGE_FOR_1ST_RECORD
      },
      {
        "return_code": RETURN_CODE_FOR_2ND_RECORD,
        "message": MESSAGE_FOR_2ND_RECORD
      },
      ...
    ]
  }

``loaded_ids`` is only included when :ref:`load-output-ids` is ``yes``.

``errors`` is only included when :ref:`load-output-errors` is ``yes``.

See also
--------

  * :doc:`/reference/grn_expr/script_syntax`

.. _Apache Arrow IPC Streaming Format: https://arrow.apache.org/docs/format/Columnar.html#ipc-streaming-format

.. _Apache Arrow IPC File Format: https://arrow.apache.org/docs/format/Columnar.html#ipc-file-format
