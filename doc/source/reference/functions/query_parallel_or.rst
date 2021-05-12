.. -*- rst -*-

.. groonga-command
.. database: functions_query_parallel_or

``query_parallel_or``
=====================

.. versionadded:: 11.0.1

Summary
-------

``query_parallel_or`` is similar to :doc:`query` but
``query_parallel_or`` processes query that has multiple ``OR``
conditions in parallel.

Syntax
------

``query_parallel_or`` requires two or more parameters -
``match_columns`` and ``query_string`` s.

The ``options`` parameter is optional.

::

  query(match_columns, query_string0, query_string1, ..., query_stringN)
  query(match_columns, query_string0, query_string1, ..., query_stringN, options)

You must specify at least one ``query_string``.

``options`` uses the following format. All of key-value pairs are
optional::

  {
    "expander": query_expander,
    "default_mode": default_mode,
    "default_operator": default_operator,
    "flags": flags
  }

Usage
-----

Here are a schema definition and sample data to show usage.

Sample schema:

.. groonga-command
.. include:: ../../example/reference/functions/query_parallel_or/usage_setup_schema.log
.. table_create Documents TABLE_NO_KEY
.. column_create Documents content COLUMN_SCALAR Text
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram  --normalizer NormalizerAuto
.. column_create Terms documents_content_index COLUMN_INDEX|WITH_POSITION Documents content
.. table_create Users TABLE_NO_KEY
.. column_create Users name COLUMN_SCALAR ShortText
.. column_create Users memo COLUMN_SCALAR ShortText
.. table_create Lexicon TABLE_HASH_KEY ShortText \
..   --default_tokenizer 'TokenNgram("unify_alphabet", false)' \
..   --normalizer NormalizerNFKC130
.. column_create Lexicon users_name COLUMN_INDEX|WITH_POSITION Users name
.. column_create Lexicon users_memo COLUMN_INDEX|WITH_POSITION Users memo

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/query_parallel_or/usage_setup_data.log
.. load --table Users
.. [
.. {"name": "Alice", "memo": "Groonga user"},
.. {"name": "Alisa", "memo": "Mroonga user"},
.. {"name": "Bob",   "memo": "Rroonga user"},
.. {"name": "Tom",   "memo": "Nroonga user"},
.. {"name": "Tobby", "memo": "Groonga and Mroonga user. Mroonga is ..."}
.. ]

Here is an example to execute full text searches in parallel by
``query_parallel_or`` function:

.. groonga-command
.. include:: ../../example/reference/functions/query_parallel_or/usage_basic.log
.. select Users \
..   --output_columns name,memo,_score \
..   --filter 'query_parallel_or("name * 10 || memo", "alice", "Groonga")'

This ``select`` command needs to execute the following full text
searches:

  * ``"alice"`` against ``name``
  * ``"alice"`` against ``memo``
  * ``"Groonga"`` against ``name``
  * ``"Groonga"`` against ``memo``

And all result sets of them are needed to be combined by ``OR``.

``query`` function executes them in a sequential
order. ``query_parallel_or`` function executes them in
parallel. Result sets of both of them are same.

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There are two required parameters, ``match_columns`` and
``query_string``.

.. _query-parallel-or-match-columns:

``match_columns``
"""""""""""""""""

Specifies the default target columns for full text search by
``query_string`` parameters. It is the same role as
:ref:`select-match-columns` parameter in ``select``.

Each pair of target column and query string is processed in
parallel. For example, ``query_parallel_or("C1 || C2 || C3", "Q1",
"Q2")`` has the following pairs:

  * ``C1 @ "Q1"``
  * ``C2 @ "Q1"``
  * ``C3 @ "Q1"``
  * ``C1 @ "Q2"``
  * ``C2 @ "Q2"``
  * ``C3 @ "Q2"``

Each pair is processed in parallel. Degree of parallelism is depends
on your system. The default max number of threads is ``N_CPU_THREADS /
3``.

``query_string``
""""""""""""""""

Specifies the search condition in
:doc:`/reference/grn_expr/query_syntax`. It is the same role as
``query`` parameter in ``select``.

See :ref:`select-match-columns` about ``query`` parameter in
``select``.

You can specify multiple ``query_string`` s to execute full text
searches in parallel. See also :ref:`query-parallel-or-match-columns`
how to parallelize.

Optional parameter
^^^^^^^^^^^^^^^^^^

There are some optional parameters.

``query_expander``
""""""""""""""""""

See :ref:`query-query-expander` for details.

``default_mode``
""""""""""""""""

See :ref:`query-default-mode` for details.

``default_operator``
""""""""""""""""""""

See :ref:`query-default-operator` for details.

``flags``
"""""""""

See :ref:`query-flags` for details.

Return value
------------

This function returns whether a record is matched or not as boolean.

This function is also worked as a selector. It means that this
function can be executable effectively.

See also
--------

* :doc:`/reference/commands/select`
* :doc:`query`
