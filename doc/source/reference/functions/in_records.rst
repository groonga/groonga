.. -*- rst -*-

.. groonga-command
.. database: functions_in_records

``in_records``
==============

Summary
-------

.. versionadded:: 7.0.2

You can use ``in_records`` for using an existing table as condition patterns. Each record in the existing table is treated as a condition pattern.

You may be able to reduce multiple queries to only one query by ``in_records``.

``in_records`` is similar to :doc:`sub_filter`. Here are differences of them:

  * ``sub_filter`` requires a reference column to condition table but ``in_records`` doesn't require.

  * ``sub_filter`` requires an index column for the reference column but ``in_records`` doesn't require.

  * ``sub_filter`` can use all logical operations but ``in_records`` can use only AND logical operation in one pattern.

  * ``sub_filter`` can use only the value of one reference column for condition but ``in_records`` can use one or more values for condition. You can use multiple columns, functions and so on for multiple values.

  * ``sub_filter`` uses index search but ``in_records`` uses sequential search.

.. _in-records-syntax:

Syntax
------

``in_records`` has four or more parameters::

  in_records(condition_table,
             value1, mode1, condition_column_name1,
             ...,
             valueN, modeN, condition_column_nameN)

.. _in-records-usage:

Usage
-----

Here is a schema definition and sample data.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/in_records/usage_setup_schema.log
.. table_create Tags TABLE_PAT_KEY ShortText
..
.. table_create Conditions TABLE_PAT_KEY ShortText
.. column_create Conditions user_pattern COLUMN_SCALAR ShortText
.. column_create Conditions tag COLUMN_SCALAR Tags
.. column_create Conditions max_length COLUMN_SCALAR UInt32
..
.. table_create Memos TABLE_HASH_KEY ShortText
.. column_create Memos user COLUMN_SCALAR ShortText
.. column_create Memos tag COLUMN_SCALAR Tags

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/in_records/usage_setup_data.log
.. load --table Memos
.. [
.. {"_key": "Groonga is fast",          "user": "alice", "tag": "groonga"},
.. {"_key": "Mroonga is fast",          "user": "alice", "tag": "mroonga"},
.. {"_key": "Groonga is very good!",    "user": "alice", "tag": "groonga"},
.. {"_key": "Droonga is fast",          "user": "david", "tag": "droonga"},
.. {"_key": "Groonga is a HTTP server", "user": "david", "tag": "groonga"}
.. ]

Sample conditions:

.. groonga-command
.. include:: ../../example/reference/functions/in_records/usage_setup_conditions.log
.. load --table Conditions
.. [
.. {"_key": "lic + groonga", "user_pattern": "lic", "tag": "groonga", max_length: 20},
.. {"_key": "dav + droonga", "user_pattern": "dav", "tag": "droonga", max_length: 50}
.. ]

Here is a simple usage of ``in_records`` that searches by records in ``Conditions`` table. Each record is used as a condition:

.. groonga-command
.. include:: ../../example/reference/functions/in_records/usage_search.log
.. plugin_register functions/string
.. select \
..   --table Memos \
..   --filter 'in_records(Conditions, \
..                        user,                 "@", "user_pattern", \
..                        tag,                 "==", "tag", \
..                        string_length(_key), "<=", "max_length")' \
..   --sort_by _id \
..   --output_columns _key,user,tag

The ``filter`` tries the following three conditions for each record:

  * ``Memos.user`` matches (``@``) ``Conditions.user_pattern``.

  * ``Memos.tag`` equals (``==``) ``Conditions.tag``.

  * The number of characters in ``Memos._key`` is less than or equals to ``Conditions.max_length``.

If at least one record in ``Conditions`` table returns true for the all three conditions, the record in ``Memos`` is matched.

The first record in ``Conditions`` table use the following conditions:

  * ``Memos.user`` has ``"lic"`` substring.

  * ``Memos.tag`` is ``"groonga"``.

  * The number of characters in ``Memos._key`` is less than or equals to ``20``.

This condition matches the following records:

  * ``{"_key": "Groonga is fast", "user": "alice", "tag": "groonga"}``

The second record in ``Conditions`` table use the following conditions:

  * ``Memos.user`` has ``"dav"`` substring.

  * ``Memos.tag`` is ``"droonga"``.

  * The number of characters in ``Memos._key`` is less than or equals to ``50``.

This condition matches the following records:

  * ``{"_key": "Droonga is fast", "user": "david", "tag": "droonga"}``

The result has the all above records.

.. _in-records-parameters:

Parameters
----------

``in_records`` requires four or more parameters.

.. _in-records-required-parameters:

Required parameters
^^^^^^^^^^^^^^^^^^^

``condition_table`` and tuples of ``value``, ``mode_name`` and ``condition_column_name`` are required. You can specify multiple tuples of ``value``, ``mode_name`` and ``condition_column_name``

.. _in-records-condition-table:

``condition_table``
"""""""""""""""""""

Specifies a table that has conditions as its records.

.. _in-records-value:

``value``
^^^^^^^^^

Specifies a value to be compared.

.. _in-records-mode-name:

``mode_name``
^^^^^^^^^^^^^

Specifies a mode name that specifies how to compare :ref:`in-records-value` with a value of :ref:`in-records-condition-column-name`.

See :ref:`query-default-mode` for available mode names. All mode names
except ``"NEAR"``, ``"SIMILAR"`` and ``"SUFFIX"`` are supported.

.. _in-records-condition-column-name:

``condition_column_name``
^^^^^^^^^^^^^^^^^^^^^^^^^

Specifies a column name of :ref:`in-records-condition-table` to be used as condition.

.. _in-records-optional-parameters:

Optional parameter
^^^^^^^^^^^^^^^^^^

There are no optional parameter.

Return value
------------

``in_records`` returns whether the record is matched one of records of the specified condition table or not.

If the record is matched, it returns ``true``. Otherwise, it returns ``false``.
