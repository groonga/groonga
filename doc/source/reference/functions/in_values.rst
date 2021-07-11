.. -*- rst -*-

.. groonga-command
.. database: functions_in_values

``in_values``
=============

Summary
-------

.. versionadded:: 4.0.7

``in_values`` enables you to simplify the query which uses multiple ``OR`` or ``==``. It is recommended to use this function in point of view about performance improvements in such a case.

Syntax
------

``in_values`` requires two or more arguments - ``target_value``, multiple ``value``, and options.

::

  in_values(target_value, value1, ..., valueN)
  in_values(target_value, value1, ..., valueN, {"option": "value of option"}

Usage
-----

Here is a schema definition and sample data.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/in_values/usage_setup_schema.log
.. table_create Tags TABLE_PAT_KEY ShortText
.. table_create Memos TABLE_HASH_KEY ShortText
.. column_create Memos tag COLUMN_SCALAR ShortText
.. column_create Tags memos_tag COLUMN_INDEX Memos tag

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/in_values/usage_setup_data.log
.. load --table Memos
.. [
.. {"_key": "Groonga is fast",          "tag": "groonga"},
.. {"_key": "Mroonga is fast",          "tag": "mroonga"},
.. {"_key": "Rroonga is fast",          "tag": "rroonga"},
.. {"_key": "Droonga is fast",          "tag": "droonga"},
.. {"_key": "Groonga is a HTTP server", "tag": "groonga"}
.. ]

Here is the simple usage of ``in_values`` function which selects the records - the value of ``tag`` column is "groonga" or "mroonga" or "droonga".

.. groonga-command
.. include:: ../../example/reference/functions/in_values/usage_only.log
.. select Memos --output_columns _key,tag --filter 'in_values(tag, "groonga", "mroonga", "droonga")' --sort_keys _id

When executing the above query, you can get the records except "rroonga" because "rroonga" is not specified as value in ``in_values``.

How to set options of ``in_values`` as below.

.. groonga-command
.. include:: ../../example/reference/functions/in_values/usage_option.log
.. select Memos --output_columns _key,tag --filter 'in_values(tag, "groonga", "mroonga", "droonga", {"too_many_index_match_ratio":0.001})' --sort_keys _id

Parameters
----------

There are two or more required parameter, ``target_value`` and multiple ``value``.

``target_value``
^^^^^^^^^^^^^^^^

Specifies a column of the table that is specified by ``table`` parameter in ``select``.

``value``
^^^^^^^^^

Specifies a value of the column which you want to select.

``{"option": "value of option"}``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Specify in_values's option.
Currently, you can only specify ``too_many_index_match_ratio``. The type of this value is ``double``.

You can change the value of ``GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO`` with ``too_many_index_match_ratio``.
The default value of ``GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO`` is ``0.01``.
``GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO`` is used for deciding whether ``in_values`` use an index or not. 

There is a case that sequential search is faster than index search when the number of narrowed down records is small enough in contrast to the number of expected records to narrow down by ``in_values`` with AND operation which use indexes.

For example, suppose you narrow down records by ``--filter`` and you narrow down them by ``in_values``.

In the default, ``in_values`` use sequential search in the following case.

  1. If you narrow down records to 1,000 by ``--filter`` and records of the target of ``in_values`` are 500,000.

     .. code-block::

        1,000/500,000 = 0.002 < 0.01(GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO) -> in_values use sequential search.

On the other hand, ``in_values`` use index in the following case.

  2. If you narrow down records to 1,000 by ``--filter`` and records of the target of ``in_values`` are 50,000.

     .. code-block::

        1,000/50,000 = 0.02 > 0.01(GRN_IN_VALUES_TOO_MANY_INDEX_MATCH_RATIO) -> in_values use index.

Return value
------------

``in_values`` returns whether the value of column exists in specified the value of parameters or not.

If record is matched to specified the value of parameters, it returns true. Otherwise, it returns false.
