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

``in_values`` requires two or more arguments - ``target_value`` and multiple ``value``.

::

  in_values(target_value, value1, ..., valueN)

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

Parameters
----------

There are two or more required parameter, ``target_value`` and multiple ``value``.

``target_value``
^^^^^^^^^^^^^^^^

Specifies a column of the table that is specified by ``table`` parameter in ``select``.

``value``
^^^^^^^^^

Specifies a value of the column which you want to select.

Return value
------------

``in_values`` returns whether the value of column exists in specified the value of parameters or not.

If record is matched to specified the value of parameters, it returns true. Otherwise, it returns false.
