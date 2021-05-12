.. -*- rst -*-

.. groonga-command
.. database: columns_vector

Vector column
=============

Summary
-------

Vector column is a data store object. It can stores zero or more
scalar values. In short, scalar value is a single value such as number
and string. See :doc:`scalar` about scalar value details.

One of vector column use cases is tags store. You can use a vector
column to store tag values.

You can use vector column as index search target in the same way as
scalar column. You can set weight for each element. The element that
has one or more weight is matched, the record has more score rather
than no weight case. It is a vector column specific feature. Vector
column that can store weight is called weight vector column.

You can also do full text search against each text element. But search
score is too high when weight is used. You should use full text search
with weight carefully.

Usage
-----

There are three vector column types:

  * Normal vector column
  * Reference vector column
  * Weight vector column

This section describes how to use these types.

.. _normal-vector-column:

Normal vector column
^^^^^^^^^^^^^^^^^^^^

Normal vector column stores zero or more scalar data. For example,
scalar data are number, string and so on.

A normal vector column can store the same type elements. You can't mix
types. For example, you can't store a number and a string in the same
normal vector column.

Normal vector column is useful when a record has multiple values with
a key. Tags are the most popular use case.

How to create
"""""""""""""

Use :doc:`/reference/commands/column_create` command to create a
normal vector column. The point is ``COLUMN_VECTOR`` flag:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_create_normal.log
.. table_create Bookmarks TABLE_HASH_KEY ShortText
.. column_create Bookmarks tags COLUMN_VECTOR ShortText

You can set zero or more tags to a bookmark.

How to load
"""""""""""

You can load vector data by JSON array syntax::

  [ELEMENT1, ELEMENT2, ELEMENT3, ...]

Let's load the following data:

.. list-table::
   :header-rows: 1

   * - ``_key``
     - ``tags``
   * - ``http://groonga.org/``
     - ``["groonga"]``
   * - ``http://mroonga.org/``
     - ``["mroonga", "mysql", "groonga"]``
   * - ``http://ranguba.org/``
     - ``["ruby", "groonga"]``

Here is a command that loads the data:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_load_normal.log
.. load --table Bookmarks
.. [
.. {"_key": "http://groonga.org/", "tags": ["groonga"]},
.. {"_key": "http://mroonga.org/", "tags": ["mroonga", "mysql", "groonga"]},
.. {"_key": "http://ranguba.org/", "tags": ["ruby", "groonga"]}
.. ]

The loaded data can be outputted as JSON array syntax:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_output_normal.log
.. select Bookmarks

How to search
"""""""""""""

You need to create an index to search normal vector column:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_index_normal.log
.. table_create Tags TABLE_PAT_KEY ShortText
.. column_create Tags bookmark_index COLUMN_INDEX Bookmarks tags

There is no vector column specific way. You can create an index like
a scalar column.

You can search an element in ``tags`` like full text search syntax.

With :ref:`select-match-columns` and :ref:`select-query`:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_query_normal.log
.. select Bookmarks --match_columns tags --query mysql --output_columns _key,tags,_score

You can also use weight in :ref:`select-match-columns`:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_query_weight_normal.log
.. select Bookmarks --match_columns 'tags * 3' --query mysql --output_columns _key,tags,_score

With :ref:`select-filter`:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_filter_normal.log
.. select Bookmarks --filter 'tags @ "msyql"' --output_columns _key,tags,_score

.. groonga-command
.. table_remove Tags
.. table_remove Bookmarks

.. _reference-vector-column:

Reference vector column
^^^^^^^^^^^^^^^^^^^^^^^

TODO

Reference vector column is space-efficient if there are many same
value elements. Reference vector column keeps reference record IDs not
value itself. Record ID is smaller than value itself.

How to create
"""""""""""""

TODO

How to load
"""""""""""

TODO

How to search
"""""""""""""

TODO

.. _weight-vector-column:

Weight vector column
^^^^^^^^^^^^^^^^^^^^

Weight vector column is similar to normal vector column. It can store
elements. It can also stores weights for them. Weight is degree of
importance of the element.

Weight is positive integer. ``0`` is the default weight. It means that
no weight.

If weight is one or larger, search score is increased by the
weight. If the weight is ``0``, score is ``1``. If the weight is
``10``, score is ``11`` (``= 1 + 10``).

Weight vector column is useful for tuning search score. See also
:ref:`select-adjuster`. You can increase search score of specific
records.

Limitations
"""""""""""

There are some limitations for now. They will be resolved in the
future.

Here are limitations:

  * You need to use string representation for element value on load.
    For example, you can't use ``29`` for number 29. You need to use
    ``"29"`` for number 29.

How to create
"""""""""""""

Use :doc:`/reference/commands/column_create` command to create a
weight vector column. The point is ``COLUMN_VECTOR|WITH_WEIGHT``
flags:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_create_weight.log
.. table_create Bookmarks TABLE_HASH_KEY ShortText
.. column_create Bookmarks tags COLUMN_VECTOR|WITH_WEIGHT ShortText

If you don't specify ``WITH_WEIGHT`` flag, it is just a normal vector
column.

You can set zero or more tags with weight to a bookmark.

How to load
"""""""""""

You can load vector data by JSON object syntax::

  {"ELEMENT1": WEIGHT1, "ELEMENT2": WEIGHT2, "ELEMENT3": WEIGHT3, ...}

Let's load the following data:

.. list-table::
   :header-rows: 1

   * - ``_key``
     - ``tags``
   * - ``http://groonga.org/``
     - ``{"groonga": 100}``
   * - ``http://mroonga.org/``
     - ``{"mroonga": 100, "mysql": 50, "groonga": 10}``
   * - ``http://ranguba.org/``
     - ``{"ruby": 100, "groonga": 50}``

Here is a command that loads the data:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_load_weight.log
.. load --table Bookmarks
.. [
.. {"_key": "http://groonga.org/",
..  "tags": {"groonga": 100}},
.. {"_key": "http://mroonga.org/",
..  "tags": {"mroonga": 100,
..           "mysql":   50,
..           "groonga": 10}},
.. {"_key": "http://ranguba.org/",
..  "tags": {"ruby": 100,
..           "groonga": 50}}
.. ]

The loaded data can be outputted as JSON object syntax:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_output_weight.log
.. select Bookmarks

How to search
"""""""""""""

You need to create an index to search weight vector column. You don't
forget to specify ``WITH_WEIGHT`` flag to ``column_create``:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_index_weight.log
.. table_create Tags TABLE_PAT_KEY ShortText
.. column_create Tags bookmark_index COLUMN_INDEX|WITH_WEIGHT Bookmarks tags

There is no weight vector column specific way except ``WITH_WEIGHT``
flag. You can create an index like a scalar column.

You can search an element in ``tags`` like full text search syntax.

With :ref:`select-match-columns` and :ref:`select-query`:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_query_weight.log
.. select Bookmarks --match_columns tags --query groonga --output_columns _key,tags,_score

You can also use weight in :ref:`select-match-columns`. The score is
``(1 + weight_in_weight_vector) * weight_in_match_columns``:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_query_weight_weight.log
.. select Bookmarks --match_columns 'tags * 3' --query groonga --output_columns _key,tags,_score

With :ref:`select-filter`:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_filter_weight.log
.. select Bookmarks --filter 'tags @ "groonga"' --output_columns _key,tags,_score

How to apply just weight
""""""""""""""""""""""""

You can use weight in weight vector column to just increase search
score without changing a set of matched records.

Use :ref:`select-adjuster` for the purpose:

.. groonga-command
.. include:: ../../example/reference/columns/vector/usage_adjuster_weight.log
.. select Bookmarks \
..   --filter true \
..   --adjuster 'tags @ "mysql" * 10 + tags @ "groonga" * 5' \
..   --output_columns _key,tags,_score

The ``select`` command uses ``--filter true``. So all records are
matched with score 1. Then it applies ``--adjuster``. The adjuster
does the following:

  * ``tags @ "mysql" * 10`` increases score by ``(1 + weight) * 10``
    of records that has ``"mysql"`` tag.
  * ``tags @ "groonga" * 5`` increases score by ``(1 + weight) * 5``
    of records that has ``"groonga"`` tag.

For example, record ``"http://mroonga.org/"`` has both ``"mysql"`` tag
and ``"groonga"`` tag. So its score is increased by ``565`` (``=
((1 + 50) * 10) + ((1 + 10) * 5) = (51 * 10) + (11 * 5) = 510 + 55``).
The search score is 1 by ``--filter true`` before applying
``--adjuster``. So the final search score is ``566`` (``= 1 + 565``)
of record ``"http://mroonga.org/"``.

.. groonga-command
.. table_remove Tags
.. table_remove Bookmarks
