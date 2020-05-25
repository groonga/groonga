.. -*- rst -*-

.. highlightlang:: none

``time_classify_day``
=====================

Summary
-------

``time_classify_day`` rounds time to a day unit.

This rounded ``2020-01-30 11:50:11.000000`` and ``2020-01-30 22:50:11.000000`` to ``2020-01-30 00:00:00.000000``.

Syntax
------

This function has only one parameter::

  time_classify_day(column)

Usage
-----

Here are a schema definition and sample data to show usage.

Execution example::

  plugin_register functions/time

  table_create Sales TABLE_NO_KEY
  column_create Sales name COLUMN_SCALAR ShortText
  column_create Sales price COLUMN_SCALAR UInt32
  column_create Sales timestamp COLUMN_SCALAR Time

  load --table Sales
  [
  {"name": "Apple" , "price": "256", "timestamp": "2020-05-01 11:50:11.000000"},
  {"name": "Apple" , "price": "256", "timestamp": "2020-05-01 10:20:00.000000"},
  {"name": "Orange", "price": "122", "timestamp": "2020-05-01 11:44:12.000001"},
  {"name": "Apple" , "price": "256", "timestamp": "2020-05-01 19:50:23.000020"},
  {"name": "Banana", "price": "88" , "timestamp": "2020-05-01 11:00:02.000000"},
  {"name": "Banana", "price": "88" , "timestamp": "2020-05-01 21:34:12.000001"}
  ]

Here is query to show round value of ``timestamp`` to a day unit.

Execution example::

  select --table Sales --output_columns "name, time_classify_day(timestamp)"
  # [
  #   [
  #     0,
  #     1590136820.452553,
  #     0.0008840560913085938
  #   ],
  #   [
  #     [
  #       [
  #         6
  #       ],
  #       [
  #         [
  #           "name",
  #           "ShortText"
  #         ],
  #         [
  #           "time_classify_day",
  #           null
  #         ]
  #       ],
  #       [
  #         "Apple",
  #         1588258800.0
  #       ],
  #       [
  #         "Apple",
  #         1588258800.0
  #       ],
  #       [
  #         "Orange",
  #         1588258800.0
  #       ],
  #       [
  #         "Apple",
  #         1588258800.0
  #       ],
  #       [
  #         "Banana",
  #         1588258800.0
  #       ],
  #       [
  #         "Banana",
  #         1588258800.0
  #       ]
  #     ]
  #   ]
  # ]

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

``column``
""""""""""

Specify a target column.

Return value
------------

It returns a value that rounded time to a day unit.

The return value is UNIX time.
