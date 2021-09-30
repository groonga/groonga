.. -*- rst -*-

.. groonga-command
.. database: functions_query

``query``
=========

Summary
-------

``query`` provides ``--match_columns`` and ``--query`` parameters of
:doc:`/reference/commands/select` feature as function. You can specify
multiple ``query`` functions in ``--filter`` parameter in
:doc:`/reference/commands/select`.

Because of such flexibility, you can control full text search behavior
by combination of multiple ``query`` functions.

``query`` can be used in only ``--filter`` in
:doc:`/reference/commands/select`.

Syntax
------

``query`` requires two parameters - ``match_columns`` and
``query_string``.

The parameter ``query_expander`` and ``options`` are optional::

  query(match_columns, query_string)
  query(match_columns, query_string, query_expander)
  query(match_columns, query_string, options)

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
.. include:: ../../example/reference/functions/query/usage_setup_schema.log
.. table_create Documents TABLE_NO_KEY
.. column_create Documents content COLUMN_SCALAR Text
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram  --normalizer NormalizerAuto
.. column_create Terms documents_content_index COLUMN_INDEX|WITH_POSITION Documents content
.. table_create Users TABLE_NO_KEY
.. column_create Users name COLUMN_SCALAR ShortText
.. column_create Users memo COLUMN_SCALAR ShortText
.. table_create Lexicon TABLE_HASH_KEY ShortText \
..   --default_tokenizer TokenBigramSplitSymbolAlphaDigit \
..   --normalizer NormalizerAuto
.. column_create Lexicon users_name COLUMN_INDEX|WITH_POSITION Users name
.. column_create Lexicon users_memo COLUMN_INDEX|WITH_POSITION Users memo

Sample data:

.. groonga-command
.. include:: ../../example/reference/functions/query/usage_setup_data.log
.. load --table Users
.. [
.. {"name": "Alice", "memo": "groonga user"},
.. {"name": "Alisa", "memo": "mroonga user"},
.. {"name": "Bob",   "memo": "rroonga user"},
.. {"name": "Tom",   "memo": "nroonga user"},
.. {"name": "Tobby", "memo": "groonga and mroonga user. mroonga is ..."},
.. ]

Here is the simple usage of ``query`` function which executes full
text search by keyword ``alice`` without using ``--match_columns`` and
``--query`` arguments in ``--filter``.

.. groonga-command
.. include:: ../../example/reference/functions/query/usage_basic.log
.. select Users --output_columns name,_score --filter 'query("name * 10", "alice")'

When executing above query, the keyword ``alice`` is weighted to the
value ``10``.

Here are the contrasting examples with/without ``query``.

.. groonga-command
.. include:: ../../example/reference/functions/query/usage_without_query.log
.. select Users --output_columns name,memo,_score --match_columns "memo * 10" --query "memo:@groonga OR memo:@mroonga OR memo:@user" --sort_keys -_score

In this case, the all keywords ``groonga``, ``mroonga`` and ``user``
use the default weight. You can't pass different weight value to each
keyword in this way.

.. groonga-command
.. include:: ../../example/reference/functions/query/usage_with_query.log
.. select Users --output_columns name,memo,_score --filter 'query("memo * 10", "groonga") || query("memo * 20", "mroonga") || query("memo * 1", "user")' --sort_keys -_score

On the other hand, by specifying multiple ``query``, the keywords
``groonga``, ``mroonga`` and ``user`` use different weight.

As a result, you can control full text search result score by
specifying different weight to the keywords on your purpose.

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There are two required parameters, ``match_columns`` and ``query_string``.

``match_columns``
"""""""""""""""""

Specifies the default target columns for full text search by
``query_string`` parameter value. It is the same role as
:ref:`select-match-columns` parameter in ``select``.

``query_string``
""""""""""""""""

Specifies the search condition in
:doc:`/reference/grn_expr/query_syntax`. It is the same role as
``query`` parameter in ``select``.

See :ref:`select-match-columns` about ``query`` parameter in
``select``.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are some optional parameters.

.. _query-query-expander:

``query_expander``
""""""""""""""""""

Specifies the query expander name or substitution column name for
query expansion.

See :doc:`/reference/query_expanders` for available query expanders.

Substitution column name uses ``${TABLE}.${COLUMN}`` format.

See :ref:`select-query-expander` for details.

.. _query-default-mode:

``default_mode``
""""""""""""""""

Specifies the default search mode. You can change search mode by
``column:@keyword`` like syntax. The default search mode is used when
you just specify ``keyword`` instead of ``column:@keyword``. See
:doc:`/reference/grn_expr/query_syntax` for more syntax details.

Here are available modes. The default is ``"MATCH"`` mode. It does
full text search.

.. list-table::
   :header-rows: 1

   * - Mode
     - Aliases
     - Description
   * - ``"EQUAL"``
     - ``"=="``
     - It uses :ref:`query-syntax-equal-condition` as the default mode.
   * - ``"NOT_EQUAL"``
     - ``"!="``
     - It uses :ref:`query-syntax-not-equal-condition` as the default mode.
   * - ``"LESS"``
     - ``"<"``
     - It uses :ref:`query-syntax-less-than-condition` as the default mode.
   * - ``"GREATER"``
     - ``">"``
     - It uses :ref:`query-syntax-greater-than-condition` as the default mode.
   * - ``"LESS_EQUAL"``
     - ``"<="``
     - It uses :ref:`query-syntax-less-than-or-equal-condition` as the
       default mode.
   * - ``"GREATER_EQUAL"``
     - ``">="``
     - It uses :ref:`query-syntax-greater-than-or-equal-condition` as the
       default mode.
   * - ``"MATCH"``
     - ``"@"``
     - It uses :ref:`query-syntax-full-text-search-condition` as the
       default mode.

       It's the default.
   * - ``"NEAR"``
     - ``"*N"``
     - It uses :ref:`query-syntax-near-search-condition` as the default
       mode.
   * - ``"SIMILAR"``
     - ``"*S"``
     - It uses :ref:`query-syntax-similar-search-condition` as the default
       mode.
   * - ``"PREFIX"``
     - ``"^"``, ``"@^"``
     - It uses :ref:`query-syntax-prefix-search-condition` as the default
       mode.
   * - ``"SUFFIX"``
     - ``"$"``, ``"@$"``
     - It uses :ref:`query-syntax-suffix-search-condition` as the default
       mode.
   * - ``"REGEXP"``
     - ``"~"``, ``"@~"``
     - It uses :ref:`query-syntax-regular-expression-condition` as the default
       mode.

.. _query-default-operator:

``default_operator``
""""""""""""""""""""

Specifies the default logical operator. It's used when no logical
operator such as ``OR`` and ``-`` are specified between conditional
expressions like ``keyword1 keyword2``. The default logical operator
is used to combine ``keyword1`` result set and ``keyword2`` result
set. The default logical operator is ``AND``. So ``keyword1 keyword2``
result set includes only records that they are included in both
``keyword1`` result set and ``keyword2`` result set.

Here are available logical operators. The default is ``"AND"``.

.. list-table::
   :header-rows: 1

   * - Logical operator
     - Aliases
     - Description
   * - ``"AND"``
     - ``"&&"``, ``"+"``
     - It uses :ref:`query-syntax-logical-and` as the default logical
       operator.
   * - ``"OR"``
     - ``"||"``
     - It uses :ref:`query-syntax-logical-or` as the default logical
       operator.
   * - ``"AND_NOT"``
     - ``"&!"``, ``"-"``
     - It uses :ref:`query-syntax-logical-and-not` as the default
       logical operator.

.. _query-flags:

``flags``
"""""""""

Specifies the flags that customizes how to parse query.

You can specify multiple flags by separating each flags by ``|``. Here
is the example to specify multiple flags::

  query("title * 10 || content",
        "keyword",
        {"flags": "ALLOW_COLUMN|ALLOW_LEADING_NOT"})

See :ref:`select-query-flags` for available flags.

Return value
------------

This function returns whether a record is matched or not as boolean.

This function is also worked as a selector. It means that this
function can be executable effectively.

See also
--------

* :doc:`/reference/commands/select`
* :doc:`query_parallel_or`
