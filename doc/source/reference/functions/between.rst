.. -*- rst -*-

.. groonga-command
.. database: functions_between

``between``
===========

Summary
-------

``between`` is used for checking the specified value exists in the specific range.
It is often used in combination with :ref:`select-filter` option in :doc:`/reference/commands/select`.

Syntax
------

``between`` has some parameters from three to six parameters::

  between(column_or_value, min, max)
  between(column_or_value, min, max, {"option": "value of option"})
  between(column_or_value, min, min_border, max, max_border)
  between(column_or_value, min, min_border, max, max_border, {"option": "value of option"})

.. _between-usage:

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/functions/between/usage_setup.log
.. table_create Users TABLE_HASH_KEY ShortText
.. column_create Users age COLUMN_SCALAR Int32
.. table_create Ages TABLE_HASH_KEY Int32
.. column_create Ages user_age COLUMN_INDEX Users age
.. load --table Users
.. [
.. {"_key": "Alice",  "age": 12},
.. {"_key": "Bob",    "age": 13},
.. {"_key": "Calros", "age": 15},
.. {"_key": "Dave",   "age": 16},
.. {"_key": "Eric",   "age": 20},
.. {"_key": "Frank",  "age": 21}
.. ]

Here is a query to show users who match PG-13 rating (MPAA).

.. groonga-command
.. include:: ../../example/reference/functions/between/usage_age.log
.. select Users --filter 'between(age, 13, "include", 16, "include")'

It returns 13, 14, 15 and 16 years old users.

``between`` function accepts not only a column but also a value.

If you specify a value as the 1st parameter, it checks whether the value is included or not. If the value matches the specified range, the ``select`` returns all the records because the ``between`` function returns true.
If it doesn't match the specified range, the ``select`` returns no records because the ``between`` function returns false.

.. groonga-command
.. include:: ../../example/reference/functions/between/usage_value.log
.. select Users --filter 'between(14, 13, "include", 16, "include")'

In the above case, it returns all the records because 14 exists in between 13 and 16.
This behavior is used for checking the specified value exists or not in the table.

.. versionadded:: 16.0.6

   ``between`` also accepts a vector as the 1st parameter.

   Vector is supported only in sequential search. ``between`` doesn't use an index for a vector value yet.

The 1st parameter can be a vector value such as a ``COLUMN_VECTOR`` column or a vector literal. In this case, ``between`` returns true if any of the elements is included in the specified range. So you can use ``between`` to search records that have at least one value in the range.

The following example uses a ``COLUMN_VECTOR`` column. Each record has multiple prices. ``between`` selects the records that have at least one price in ``[18, 20)``:

.. groonga-command
.. include:: ../../example/reference/functions/between/usage_vector.log
.. table_create Products TABLE_HASH_KEY ShortText
.. column_create Products prices COLUMN_VECTOR Int32
.. load --table Products
.. [
.. {"_key": "A", "prices": [17, 170, 1700]},
.. {"_key": "B", "prices": [18, 180, 1800]},
.. {"_key": "C", "prices": [19, 190]},
.. {"_key": "D", "prices": [20]},
.. {"_key": "E", "prices": [21, 210, 2100]}
.. ]
.. select Products --filter 'between(prices, 18, "include", 20, "exclude")'

``A`` and ``E`` are excluded because none of their prices is in the range.
``D`` is excluded because ``20`` is excluded by ``"exclude"``.
``B`` and ``C`` are selected because each of them has at least one price in the range.

If the elements of a vector use different types, the type of the first element is used as the target type and the elements that use other types are ignored. For example, in ``[180.0, 18]``, ``18`` is ignored because the first element ``180.0`` is a floating point number while ``18`` isn't.

Then, you can also specify options of ``between``.
Currently, you can only specify ``too_many_index_match_ratio``. The type of this value is ``double``.

You can change the value of ``GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO`` with ``too_many_index_match_ratio``.
The default value of ``GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO`` is ``0.01``.
``GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO`` is used for deciding whether ``between`` use an index or not.

There is a case that sequential search is faster than index search when the number of narrowed down records is small enough in contrast to the number of expected records to narrow down by ``between`` with AND operation which use indexes.

For example, suppose you narrow down records by ``--filter`` and you narrow down them by ``between``.

In the default, ``between`` use sequential search in the following case.

  1. If you narrow down records to 1,000 by ``--filter`` and records of the target of ``between`` are 500,000.

     .. code-block::

        1,000/500,000 = 0.002 < 0.01(GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO) -> between use sequential search.

On the other hand, ``between`` use index in the following case.

  2. If you narrow down records to 1,000 by ``--filter`` and records of the target of ``between`` are 50,000.

     .. code-block::

        1,000/50,000 = 0.02 > 0.01(GRN_BETWEEN_TOO_MANY_INDEX_MATCH_RATIO) -> between use index.


Here is a query to set options of ``between``:

.. groonga-command
.. include:: ../../example/reference/functions/between/usage_options.log
.. select Users --filter 'between(age, 13, "include", 16, "include", {"too_many_index_match_ratio":0.001})'

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There are three required parameters ``column_or_value``, ``min`` and ``max``.

``column_or_value``
"""""""""""""""""""

Specifies a column or value.

.. versionadded:: 16.0.6

   A vector is also accepted only with sequential search. ``between`` returns true if any of the elements is included in the specified range. See :ref:`between-usage` for details.

``min``
"""""""

Specifies the minimal border value of the range.
The range is inclusive by default but you can control the behavior that the value of ``min`` is included or excluded with ``min_border`` parameter.

``max``
"""""""

Specifies the maximum border value of the range.
The range is inclusive by default but you can control the behavior that the value of ``max`` is included or excluded with ``max_border`` parameter.

Optional parameter
^^^^^^^^^^^^^^^^^^

There are two optional parameters ``min_border`` and ``max_border``.

``min_border``
""""""""""""""

Specifies whether the specified range contains the value of ``min`` or not.
The value of ``min_border`` must be either "include" or "exclude". If it is "include", ``min`` value is included. If it is "exclude", ``min`` value is not included.

``max_border``
""""""""""""""

Specifies whether the specified range contains the value of ``max`` or not.
The value of ``max_border`` must be either "include" or "exclude". If it is "include", ``max`` value is included. If it is "exclude", ``max`` value is not included.

``{"option": "value of option"}``
"""""""""""""""""""""""""""""""""

Specify between's option.
Currently, you can only specify ``too_many_index_match_ratio``. The type of this value is ``double``.

Return value
""""""""""""

``between`` returns whether the column value exists in the specified range or not. If a record matches the specified range, it returns true. Otherwise, it returns false.
