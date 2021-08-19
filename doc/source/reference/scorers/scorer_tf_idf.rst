.. -*- rst -*-

.. groonga-command
.. database: scorer_tf_idf

``scorer_tf_idf``
=================

.. versionadded:: 5.0.1

Summary
-------

``scorer_tf_idf`` is a scorer based of `TF-IDF
<https://en.wikipedia.org/wiki/Tf%E2%80%93idf>`_ (term
frequency-inverse document frequency) score function.

To put it simply, TF (term frequency) divided by DF (document
frequency) is TF-IDF. "TF" means that "the number of occurrences is
more important". "TF divided by DF" means that "the number of
occurrences of important term is more important".

The default score function in Groonga is TF (term frequency). It
doesn't care about term importance but is fast.

TF-IDF cares about term importance but is slower than TF.

TF-IDF will compute more suitable score rather than TF for many cases.
But it's not perfect.

If document contains many same keywords such as "They are keyword,
keyword, keyword ... and keyword", it increases score by TF and
TF-IDF. Search engine spammer may use the technique. But TF-IDF
doesn't guard from the technique.

`Okapi BM25 <https://en.wikipedia.org/wiki/Okapi_BM25>`_ can solve the
case. But it's more slower than TF-IDF and not implemented yet in
Groonga.

Groonga provides :doc:`scorer_tf_at_most` scorer that can also solve
the case.

.. include:: ../scoring_note.rst

Syntax
------

This scorer has only one parameter::

  scorer_tf_idf(column)
  scorer_tf_idf(index)

Usage
-----

This section describes how to use this scorer.

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/scorers/scorer_tf_idf/usage_setup_schema.log
.. table_create Logs TABLE_NO_KEY
.. column_create Logs message COLUMN_SCALAR Text
..
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Terms message_index COLUMN_INDEX|WITH_POSITION Logs message

Sample data:

.. groonga-command
.. include:: ../../example/reference/scorers/scorer_tf_idf/usage_setup_data.log
.. load --table Logs
.. [
.. {"message": "Error"},
.. {"message": "Warning"},
.. {"message": "Warning Warning"},
.. {"message": "Warning Warning Warning"},
.. {"message": "Info"},
.. {"message": "Info Info"},
.. {"message": "Info Info Info"},
.. {"message": "Info Info Info Info"},
.. {"message": "Notice"},
.. {"message": "Notice Notice"},
.. {"message": "Notice Notice Notice"},
.. {"message": "Notice Notice Notice Notice"},
.. {"message": "Notice Notice Notice Notice Notice"}
.. ]

You specify ``scorer_tf_idf`` in :ref:`select-match-columns` like the
following:

.. groonga-command
.. include:: ../../example/reference/scorers/scorer_tf_idf/usage_no_weight.log
.. select Logs \
..   --match_columns "scorer_tf_idf(message)" \
..   --query "Error OR Info" \
..   --output_columns "message, _score" \
..   --sort_keys "-_score"

Both the score of ``Info Info Info`` and the score of ``Error`` are
``2`` even ``Info Info Info`` includes three ``Info`` terms. Because
``Error`` is more important term rather than ``Info``. The number of
documents that include ``Info`` is ``4``. The number of documents that
include ``Error`` is ``1``. Term that is included in less documents
means that the term is more characteristic term. Characteristic term
is important term.

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

Score is computed as TF-IDF based algorithm.

See also
--------

* :doc:`../scorer`
