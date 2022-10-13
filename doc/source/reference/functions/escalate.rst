.. -*- rst -*-

.. groonga-command
.. database: functions_escalate

``escalate``
============

.. versionadded:: 12.0.8

Summary
-------

.. note::

   This function is experimental.

It's similar to the existing match escalation mechanism but this is more generic.

Match escalation is auto loose search.
If the number of matched records is equal or less than the threshold specified by ``match_escalation_threshold``, loose search is done automatically. It's match escalation.

Please refer to :doc:`/spec/search` about the search storategy escalation.

``match_escalation_threshold`` is ``select``'s argument. In addition, the default value of ``match_escalation_threshold`` is `0`.
Please refer to :doc:`/reference/commands/select` about ``match_escalation_threshold``.

The existing match escalation mechanism is just for one full text search by inverted index.
Therefore, for example, if we can't get record in a search with a index that execute search strictly, we need to search with a index that execute search loosely once again.
This procedure has many overhead.

However, ``escalte`` is for multiple full text search by inverted index.
Therefore, we can execute a search with a index that execute search strictly and a search with a index that execute search loosely in one query.
We can reduce overhead by using ``escalate``.

Syntax
------

``escalate`` is require one or more parameters.

  .. code-block::

    escalate(CONDITION_1,
             THRESHOLD_2, CONDITION_2,
             ...,
             THRESHOLD_N, CONDITION_N)

``CONDITION_N`` and ``THRESHOLD_N`` are pair.
However, ``CONDITION_1`` has not a threshold as pair. Because this condition is always executed.

Usage
-----

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/escalate/usage_setup_schema.log
.. table_create Users TABLE_HASH_KEY ShortText
.. column_create Users age COLUMN_SCALAR Int32
.. table_create Ages TABLE_HASH_KEY Int32
.. column_create Ages user_age COLUMN_INDEX Users age

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/escalate/usage_setup_data.log
.. load --table Users
.. [
.. {"_key": "Alice",  "age": 12},
.. {"_key": "Bob",    "age": 13},
.. {"_key": "Calros", "age": 15},
.. {"_key": "Dave",   "age": 16},
.. {"_key": "Eric",   "age": 20},
.. {"_key": "Frank",  "age": 21}
.. ]

Here is a simple example.

.. groonga-command
.. include:: ../../example/reference/functions/escalate/usage_age.log
.. select Users --filter "escalate('age >= 65', 0, 'age >= 60', 0, 'age >= 50', 0, 'age >= 40', 0, 'age >= 30', 0, 'age >= 20')"

Parameters
----------

``CONDITION_1``
~~~~~~~~~~~~~~~

``CONDITION_1`` is required.

A condition that we specify ``CONDITION_1`` is always executed.
Therefore, ``CONDITION_1`` has not a threshold as pair.

Normally, we specify the condition that we can the most narrow down search results.

``CONDITION_1`` is a string that uses script syntax such as "number_column > 29".

``CONDITION_N``
~~~~~~~~~~~~~~~

``CONDITION_N`` is optional.

If the number of results that we search with the one before condition in threshold or less, ``escalate`` evaluate ``CONDITION_N``.

``CONDITION_N`` is a string that uses script syntax such as "number_column > 29".

``THRESHOLD_N``
~~~~~~~~~~~~~~~

``THRESHOLD_N`` is optional. However, when ``CONDITION_N`` exist ``THRESHOLD_N`` is required. (However, ``CONDITION_1`` has not a threshold as pair.)

If the number of results that we search with ``CONDITION_N`` in ``THRESHOLD_N`` or less, ``escalate`` evaluate next condition.
If the number of results that we search with ``CONDITION_N`` in more than ``THRESHOLD_N``, ``escalate`` doesn't evaluate next condition.

``THRESHOLD_N`` is a positive number such 0 and 29.

Return value
------------

``escalate`` returns whether a record is matched or not as boolean.

