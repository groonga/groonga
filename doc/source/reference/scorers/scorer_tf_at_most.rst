.. -*- rst -*-

.. groonga-command
.. database: scorer_tf_at_most

``scorer_tf_at_most``
=====================

.. note::

   This scorer is an experimental feature.

.. versionadded:: 5.0.1

Summary
-------

``scorer_tf_at_most`` is a scorer based on TF (term frequency).

TF based scorer includes TF-IDF based scorer has a problem for the
following case:

If document contains many same keywords such as "They are keyword,
keyword, keyword ... and keyword", the document has high score. It's
not expected. Search engine spammer may use the technique.

``scorer_tf_at_most`` is a TF based scorer but it can solve the case.

``scorer_tf_at_most`` limits the maximum score value. It means that
``scorer_tf_at_most`` limits effect of a match.

If document contains many same keywords such as "They are keyword,
keyword, keyword ... and keyword", ``scorer_tf_at_most(column, 2.0)``
returns at most ``2`` as score.

.. include:: ../scoring_note.rst

Syntax
------

This scorer has two parameters::

  scorer_tf_at_most(column, max)
  scorer_tf_at_most(index, max)

Usage
-----

This section describes how to use this scorer.

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/scorers/scorer_tf_at_most/usage_setup_schema.log
.. table_create Logs TABLE_NO_KEY
.. column_create Logs message COLUMN_SCALAR Text
..
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Terms message_index COLUMN_INDEX|WITH_POSITION Logs message

Sample data:

.. groonga-command
.. include:: ../../example/reference/scorers/scorer_tf_at_most/usage_setup_data.log
.. load --table Logs
.. [
.. {"message": "Notice"},
.. {"message": "Notice Notice"},
.. {"message": "Notice Notice Notice"},
.. {"message": "Notice Notice Notice Notice"},
.. {"message": "Notice Notice Notice Notice Notice"}
.. ]

You specify ``scorer_tf_at_most`` in :ref:`select-match-columns` like
the following:

.. groonga-command
.. include:: ../../example/reference/scorers/scorer_tf_at_most/usage_no_weight.log
.. select Logs \
..   --match_columns "scorer_tf_at_most(message, 3.0)" \
..   --query "Notice" \
..   --output_columns "message, _score" \
..   --sort_keys "-_score"

If a document has three or more ``Notice`` terms, its score is ``3``.
Because the ``select`` specify ``3.0`` as the max score.

If a document has one or two ``Notice`` terms, its score is ``1`` or
``2``. Because the score is less than ``3.0`` specified as the max score.

Parameters
----------

This section describes all parameters.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is only one required parameter.

``column``
""""""""""

The data column that is match target. The data column must be indexed.

``index``
"""""""""

The index column to be used for search.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There is no optional parameter.

Return value
------------

This scorer returns score as :ref:`builtin-type-float`.

:doc:`/reference/commands/select` returns ``_score`` as ``Int32`` not
``Float``. Because it casts to ``Int32`` from ``Float`` for keeping
backward compatibility.

Score is computed as TF with limitation.

See also
--------

* :doc:`../scorer`
