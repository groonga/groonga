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
