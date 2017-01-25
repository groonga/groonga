.. -*- rst -*-

.. highlightlang:: none

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

``query`` requires two arguments - ``match_columns`` and ``query_string``.

The parameter ``query_expander``, ``substitution_table`` and
``options`` are optional.

::

  query(match_columns, query_string)
  query(match_columns, query_string, query_expander)
  query(match_columns, query_string, substitution_table)
  query(match_columns, query_string, options)

``options`` accepts the following keys::

  {
    "expander": query_expander,
    "default_mode": default_mode,
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

Here is the simple usage of ``query`` function which execute full text
search by keyword 'alice' without using ``--match_columns`` and
``--query`` arguments in ``--filter``.

.. groonga-command
.. include:: ../../example/reference/functions/query/usage_basic.log
.. select Users --output_columns name,_score --filter 'query("name * 10", "alice")'

When executing above query, the keyword ``alice`` is weighted to the
value ``10``.

Here are the contrasting examples with/without ``query``.

.. groonga-command
.. include:: ../../example/reference/functions/query/usage_without_query.log
.. select Users --output_columns name,memo,_score --match_columns "memo * 10" --query "memo:@groonga OR memo:@mroonga OR memo:@user" --sortby -_score

In this case, the all keywords ``groonga``, ``mroonga`` and ``user``
use the default weight. You can't pass different weight value to each
keyword in this way.

.. groonga-command
.. include:: ../../example/reference/functions/query/usage_with_query.log
.. select Users --output_columns name,memo,_score --filter 'query("memo * 10", "groonga") || query("memo * 20", "mroonga") || query("memo * 1", "user")' --sortby -_score

On the other hand, by specifying multiple ``query``, the keywords
``groonga``, ``mroonga`` and ``user`` use different weight.

As a result, you can control full text search result score by
specifying different weight to the keywords on your purpose.

Parameters
----------

Required parameters
^^^^^^^^^^^^^^^^^^^

There are two required parameter, ``match_columns`` and ``query_string``.

``match_columns``
"""""""""""""""""

Specifies the default target column for fulltext search by
``query_string`` parameter value. It is the same role as
:ref:`select-match-columns` parameter in ``select``.

``query_string``
""""""""""""""""

Specifies the search condition in
:doc:`/reference/grn_expr/query_syntax`. It is the same role as
``query`` parameter in ``select``.

See :ref:`select-match-columns` about ``query`` parameter in
``select``.

Optional parameter
^^^^^^^^^^^^^^^^^^

There are some optional parameters.

``query_expander``
""""""""""""""""""

Specifies the plugin name for query expansion.

There is one plugin bundled in official release - :doc:`/reference/query_expanders/tsv`.

See :doc:`/reference/query_expanders/tsv` about details.


``substitution_table``
""""""""""""""""""""""

Specifies the substitution table and substitution column name
by following format such as ``${TABLE}.${COLUMN}`` for query expansion.

See :ref:`select-query-expander` about details.

``default_mode``
""""""""""""""""

Specifies the default search operation. The search operation can
customize like ``column:@keyword`` syntax. The default search
operation is used when you just specify ``keyword`` instead of
``column:@keyword``. See :doc:`/reference/grn_expr/query_syntax` for
more syntax details.

Here are available modes. The default is ``MATCH`` mode. It does full
text search.

.. list-table::
   :header-rows: 1

   * - Mode
     - Aliases
     - Description
   * - ``EQUAL``
     - ``==``
     - Use :ref:`query-syntax-equal-condition` as the default operator.
   * - ``NOT_EQUAL``
     - ``!=``
     - Use :ref:`query-syntax-not-equal-condition` as the default operator.
   * - ``LESS``
     - ``<``
     - Use :ref:`query-syntax-less-than-condition` as the default operator.
   * - ``GREATER``
     - ``>``
     - Use :ref:`query-syntax-greater-than-condition` as the default operator.
   * - ``LESS_EQUAL``
     - ``<=``
     - Use :ref:`query-syntax-less-than-or-equal-condition` as the
       default operator.
   * - ``GREATER_EQUAL``
     - ``>=``
     - Use :ref:`query-syntax-greater-than-or-equal-condition` as the
       default operator.
   * - ``MATCH``
     - ``@``
     - Use :ref:`query-syntax-full-text-search-condition` as the
       default operator.

       It's the default.
   * - ``NEAR``
     - ``*N``
     - Use :ref:`query-syntax-near-search-condition` as the default
       operator.
   * - ``SIMILAR``
     - ``*S``
     - Use :ref:`query-syntax-similar-search-condition` as the default
       operator.
   * - ``PREFIX``
     - ``^``, ``@^``
     - Use :ref:`query-syntax-prefix-search-condition` as the default
       operator.
   * - ``SUFFIX``
     - ``$``, ``@$``
     - Use :ref:`query-syntax-suffix-search-condition` as the default
       operator.
   * - ``REGEXP``
     - ``~``, ``@~``
     - Use :ref:`query-syntax-regular-expression-condition` as the default
       operator.

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

``query`` returns whether any record is matched or not. If one or more
records are matched, it returns ``true``. Otherwise, it returns
``false``.

See also
--------

* :doc:`/reference/commands/select`
