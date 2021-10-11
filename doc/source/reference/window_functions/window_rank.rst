.. -*- rst -*-

.. groonga-command
.. database: window_functions_window_rank

``window_rank``
===============

Summary
-------

.. versionadded:: 11.0.9

This window function computes the rank of each record with gaps. This
is similar to :doc:`window_record_number`. :doc:`window_record_number`
computes the number of each record. The number is always incremented
but the rank isn't incremented when multiple records that are the same
order. The next rank after multiple records that are the same order
has gap. If values of sort keys are ``100, 100, 200`` then the ranks
of them are ``1, 1, 3``. The rank of the last record is ``3`` not
``2`` because there are two ``1`` rank records.

Syntax
------

This window function doesn't require any parameters::

  window_rank()

Usage
-----

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/window_functions/window_rank/usage_setup.log
.. table_create Points TABLE_NO_KEY
.. column_create Points game COLUMN_SCALAR ShortText
.. column_create Points score COLUMN_SCALAR UInt32
..
.. load --table Points
.. [
.. ["game",  "score"],
.. ["game1", 100],
.. ["game1", 200],
.. ["game1", 100],
.. ["game1", 400],
.. ["game2", 150],
.. ["game2", 200],
.. ["game2", 200],
.. ["game2", 200]
.. ]

Here is an example that specifies only sort keys:

.. groonga-command
.. include:: ../../example/reference/window_functions/window_rank/usage_sort_keys.log
.. select Points \
..   --columns[rank].stage filtered \
..   --columns[rank].value 'window_rank()' \
..   --columns[rank].type UInt32 \
..   --columns[rank].window.sort_keys score \
..   --output_columns 'game, score, rank' \
..   --sort_keys score

Here is an example that computes ranks for each game:

.. groonga-command
.. include:: ../../example/reference/window_functions/window_rank/usage_group.log
.. select Points \
..   --columns[rank].stage filtered \
..   --columns[rank].value 'window_rank()' \
..   --columns[rank].type UInt32 \
..   --columns[rank].window.group_keys game \
..   --columns[rank].window.sort_keys score \
..   --output_columns 'game, score, rank' \
..   --sort_keys game,score

Here is an example that uses descending order:

.. groonga-command
.. include:: ../../example/reference/window_functions/window_rank/usage_group_descending.log
.. select Points \
..   --columns[rank].stage filtered \
..   --columns[rank].value 'window_rank()' \
..   --columns[rank].type UInt32 \
..   --columns[rank].window.group_keys game \
..   --columns[rank].window.sort_keys -score \
..   --output_columns 'game, score, rank' \
..   --sort_keys game,-score

Parameters
----------

Nothing.

Return value
------------

The rank as ``UInt32`` value.

See also
--------

* :doc:`window_record_number`

