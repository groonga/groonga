.. -*- rst -*-

.. groonga-command
.. database: functions_prefix_rk_search

``prefix_rk_search``
====================

Summary
-------

``prefix_rk_search`` selects records by
:doc:`/reference/operations/prefix_rk_search`.

You need to create :ref:`table-pat-key` table for prefix RK search.

You can't use ``prefix_rk_search`` for sequential scan. It's a
selector only procedure.

Syntax
------

``prefix_rk_search`` requires two arguments. They are ``column`` and
``query``::

  prefix_rk_search(column, query)

``column`` must be ``_key`` for now.

``query`` must be string.

Usage
-----

Here are a schema definition and sample data to show usage:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_setup.log
.. table_create Readings TABLE_PAT_KEY ShortText --normalizer NormalizerAuto
.. load --table Readings
.. [
.. {"_key": "ニホン"},
.. {"_key": "ニッポン"},
.. {"_key": "ローマジ"}
.. ]

Here is the simple usage of ``prefix_rk_search()`` function which
selects ``ニホン`` and ``ニッポン`` by ``ni``:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_simple.log
.. select Readings --filter 'prefix_rk_search(_key, "ni")'

You can implement :doc:`/reference/suggest/completion` like feature by
combining :doc:`sub_filter`.

Create a table that has candidates of completion as records. Each
records have zero or more readings. They are stored into ``Readings``
table. Don't forget define an index column for ``Items.readings`` in
``Readings`` table. The index column is needed for :doc:`sub_filter`:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_setup_completion.log
.. table_create Items TABLE_HASH_KEY ShortText
.. column_create Items readings COLUMN_VECTOR Readings
..
.. column_create Readings items_index COLUMN_INDEX Items readings
..
.. load --table Items
.. [
.. {"_key": "日本",     "readings": ["ニホン", "ニッポン"]},
.. {"_key": "ローマ字", "readings": ["ローマジ"]},
.. {"_key": "漢字",     "readings": ["カンジ"]}
.. ]

You can find ``日本`` record in ``Items`` table by ``niho``. Because
prefix RK search with ``niho`` selects ``ニホン`` reading and ``ニホン``
reading is one of readings of ``日本`` record:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_prefix_rk_only_completion.log
.. select Items \
..  --filter 'sub_filter(readings, "prefix_rk_search(_key, \\"niho\\")")'

You need to combine :ref:`script-syntax-prefix-search-operator` to
support no reading completion targets.

Add one no reading completion target:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_add_no_reading_completion_target.log
.. load --table Items
.. [
.. {"_key": "nihon", "readings": []}
.. ]

Combine :ref:`script-syntax-prefix-search-operator` to
support no reading completion targets:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_prefix_search_combined_completion.log
.. select Items \
..  --filter 'sub_filter(readings, "prefix_rk_search(_key, \\"niho\\")") || _key @^ "niho"'

Normally, you want to use case insensitive search for completion. Use
``--normalizer NormalizerAuto`` and ``label`` column for the case:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_setup_loose_completion.log
.. table_create LooseItems TABLE_HASH_KEY ShortText --normalizer NormalizerAuto
.. column_create LooseItems label COLUMN_SCALAR ShortText
.. column_create LooseItems readings COLUMN_VECTOR Readings
..
.. column_create Readings loose_items_index COLUMN_INDEX LooseItems readings
..
.. load --table LooseItems
.. [
.. {"_key": "日本",     "label": "日本",     "readings": ["ニホン", "ニッポン"]},
.. {"_key": "ローマ字", "label": "ローマ字", "readings": ["ローマジ"]},
.. {"_key": "漢字",     "label": "漢字",     "readings": ["カンジ"]},
.. {"_key": "Nihon",    "label": "日本",     "readings": []}
.. ]

Use ``LooseItems.label`` for display:

.. groonga-command
.. include:: ../../example/reference/functions/prefix_rk_search/usage_loose_completion.log
.. select LooseItems \
..  --filter 'sub_filter(readings, "prefix_rk_search(_key, \\"nIhO\\")") || _key @^ "nIhO"' \
..  --output_columns '_key,label'

Parameters
----------

There are two required parameter, ``column`` and ``query``.

``column``
^^^^^^^^^^

Always specifies ``_key`` for now.

``query``
^^^^^^^^^

Specifies a query in romaji, katakana or hiragana as string.

Return value
------------

``prefix_rk_search`` function returns matched records.

See also
--------

* :doc:`/reference/operations/prefix_rk_search`
* :doc:`/reference/functions/sub_filter`
