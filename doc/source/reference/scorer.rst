.. -*- rst -*-

.. groonga-command
.. database: scorer

Scorer
======

Summary
-------

Groonga has scorer module that customizes score function. Score
function computes score of matched record. The default scorer function
uses the number of appeared terms. It is also known as TF (term
frequency).

TF is a fast score function but it's not suitable for the following
cases:

  * Search query contains one or more frequently-appearing words such
    as "the" and "a".
  * Document contains many same keywords such as "They are keyword,
    keyword, keyword ... and keyword". Search engine spammer may use
    the technique.

Score function can solve these cases. For example, `TF-IDF
<https://en.wikipedia.org/wiki/Tf%E2%80%93idf>`_ (term
frequency-inverse document frequency) can solve the first case.
`Okapi BM25 <https://en.wikipedia.org/wiki/Okapi_BM25>`_ can solve the
second case. But their are slower than TF.

Groonga provides TF-IDF based scorer as
:doc:`/reference/scorers/scorer_tf_idf` but doesn't provide Okapi BM25
based scorer yet.

.. include:: scoring_note.rst

Usage
-----

This section describes how to use scorer.

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../example/reference/scorer/usage_setup_schema.log
.. table_create Memos TABLE_HASH_KEY ShortText
.. column_create Memos title COLUMN_SCALAR ShortText
.. column_create Memos content COLUMN_SCALAR Text
..
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Terms title_index COLUMN_INDEX|WITH_POSITION Memos title
.. column_create Terms content_index COLUMN_INDEX|WITH_POSITION Memos content

Sample data:

.. groonga-command
.. include:: ../example/reference/scorer/usage_setup_data.log
.. load --table Memos
.. [
.. {
..   "_key": "memo1",
..   "title": "Groonga is easy",
..   "content": "Groonga is very easy full text search engine!"
.. },
.. {
..   "_key": "memo2",
..   "title": "Mroonga is easy",
..   "content": "Mroonga is more easier full text search engine!"
.. },
.. {
..   "_key": "memo3",
..   "title": "Rroonga is easy",
..   "content": "Ruby is very helpful."
.. },
.. {
..   "_key": "memo4",
..   "title": "Groonga is fast",
..   "content": "Groonga! Groonga! Groonga! Groonga is very fast!"
.. },
.. {
..   "_key": "memo5",
..   "title": "PGroonga is fast",
..   "content": "PGroonga is very fast!"
.. },
.. {
..   "_key": "memo6",
..   "title": "PGroonga is useful",
..   "content": "SQL is easy because many client libraries exist."
.. },
.. {
..   "_key": "memo7",
..   "title": "Mroonga is also useful",
..   "content": "MySQL has replication feature. Mroonga can use it."
.. }
.. ]

You can specify custom score function in :ref:`select-match-columns`.
There are some syntaxes.

For score function that doesn't require any parameter such as
:doc:`/reference/scorers/scorer_tf_idf`::

  SCORE_FUNCTION(COLUMN)

You can specify weight::

  SCORE_FUNCTION(COLUMN) * WEIGHT

For score function that requires one or more parameters such as
:doc:`/reference/scorers/scorer_tf_at_most`::

  SCORE_FUNCTION(COLUMN, ARGUMENT1, ARGUMENT2, ...)

You can specify weight::

  SCORE_FUNCTION(COLUMN, ARGUMENT1, ARGUMENT2, ...) * WEIGHT

You can use different score function for each
:ref:`select-match-columns`::

  SCORE_FUNCTION1(COLUMN1) ||
    SCORE_FUNCTION2(COLUMN2) * WEIGHT ||
    SCORE_FUNCTION3(COLUMN3, ARGUMENT1) ||
    ...

Here is a simplest example:

.. groonga-command
.. include:: ../example/reference/scorer/usage_one_no_argument_no_weight.log
.. select Memos \
..   --match_columns "scorer_tf_idf(content)" \
..   --query "Groonga" \
..   --output_columns "content, _score" \
..   --sort_keys "-_score"

``Groonga! Groonga! Groonga! Groonga is very fast!`` contains 4
``Groonga``. If you use TF based scorer that is the default scorer,
``_score`` is ``4``. But the actual ``_score`` is ``2``. Because the
``select`` command uses TF-IDF based scorer ``scorer_tf_idf()``.

Here is an example that uses weight:

.. groonga-command
.. include:: ../example/reference/scorer/usage_one_no_argument_weight.log
.. select Memos \
..   --match_columns "scorer_tf_idf(content) * 10" \
..   --query "Groonga" \
..   --output_columns "content, _score" \
..   --sort_keys "-_score"

``Groonga! Groonga! Groonga! Groonga is very fast!`` has ``22`` as
``_score``. It had ``2`` as ``_score`` in the previous example that
doesn't specify weight.

Here is an example that uses scorer that requires one
argument. :doc:`/reference/scorers/scorer_tf_at_most` scorer requires
one argument. You can limit TF score by the scorer.

.. groonga-command
.. include:: ../example/reference/scorer/usage_one_one_argument_no_weight.log
.. select Memos \
..   --match_columns "scorer_tf_at_most(content, 2.0)" \
..   --query "Groonga" \
..   --output_columns "content, _score" \
..   --sort_keys "-_score"

``Groonga! Groonga! Groonga! Groonga is very fast!`` contains 4
``Groonga``. If you use normal TF based scorer that is the default
scorer, ``_score`` is ``4``. But the actual ``_score`` is ``2``.
Because the scorer used in the ``select`` command limits the maximum
score value to ``2``.

Here is an example that uses multiple scorers:

.. groonga-command
.. include:: ../example/reference/scorer/usage_multiple_scorers.log
.. select Memos \
..   --match_columns "scorer_tf_idf(title) || scorer_tf_at_most(content, 2.0)" \
..   --query "Groonga" \
..   --output_columns "title, content, _score" \
..   --sort_keys "-_score"

The ``--match_columns`` uses ``scorer_tf_idf(title)`` and
``scorer_tf_at_most(content, 2.0)``. ``_score`` value is sum of them.

You can use the default scorer and custom scorer in the same
``--match_columns``. You can use the default scorer by just specifying
a match column:

.. groonga-command
.. include:: ../example/reference/scorer/usage_default_and_custom_scorers.log
.. select Memos \
..   --match_columns "title || scorer_tf_at_most(content, 2.0)" \
..   --query "Groonga" \
..   --output_columns "title, content, _score" \
..   --sort_keys "-_score"

The ``--match_columns`` uses the default scorer (TF) for ``title`` and
:doc:`/reference/scorers/scorer_tf_at_most` for
``content``. ``_score`` value is sum of them.

Built-in scorers
----------------

Here are built-in scores:

.. toctree::
   :maxdepth: 1
   :glob:

   scorers/*
