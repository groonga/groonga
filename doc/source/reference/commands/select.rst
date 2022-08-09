.. -*- rst -*-

.. groonga-command
.. database: commands_select

``select``
==========

Summary
-------

``select`` searches records that are matched to specified conditions
from a table and then outputs them.

``select`` is the most important command in groonga. You need to
understand ``select`` to use the full power of Groonga.

Syntax
------

This command takes many parameters.

The required parameter is only ``table``. Other parameters are
optional::

  select table
         [match_columns=null]
         [query=null]
         [filter=null]
         [scorer=null]
         [sortby=null]
         [output_columns="_id, _key, *"]
         [offset=0]
         [limit=10]
         [drilldown=null]
         [drilldown_sortby=null]
         [drilldown_output_columns="_key, _nsubrecs"]
         [drilldown_offset=0]
         [drilldown_limit=10]
         [cache=yes]
         [match_escalation_threshold=0]
         [query_expansion=null]
         [query_flags=ALLOW_PRAGMA|ALLOW_COLUMN]
         [query_expander=null]
         [adjuster=null]
         [drilldown_calc_types=NONE]
         [drilldown_calc_target=null]
         [drilldown_filter=null]
         [sort_keys=null]
         [drilldown_sort_keys=null]
         [match_escalation=auto]
         [load_table=null]
         [load_columns=null]
         [load_values=null]
         [drilldown_max_n_target_records=-1]
         [n_workers=0]

This command has the following named parameters for dynamic columns:

   * ``columns[${NAME}].stage=null``
   * ``columns[${NAME}].flags=COLUMN_SCALAR``
   * ``columns[${NAME}].type=null``
   * ``columns[${NAME}].value=null``
   * ``columns[${NAME}].window.sort_keys=null``
   * ``columns[${NAME}].window.group_keys=null``

You can use one or more alphabets, digits, ``_`` for ``${NAME}``. For
example, ``column1`` is a valid ``${NAME}``. This is the same rule as
normal column. See also :ref:`column-create-name`.

Parameters that have the same ``${NAME}`` are grouped.

For example, the following parameters specify one dynamic column:

  * ``--columns[name].stage initial``
  * ``--columns[name].type UInt32``
  * ``--columns[name].value 29``

The following parameters specify two dynamic columns:

  * ``--columns[name1].stage initial``
  * ``--columns[name1].type UInt32``
  * ``--columns[name1].value 29``
  * ``--columns[name2].stage filtered``
  * ``--columns[name2].type Float``
  * ``--columns[name2].value '_score * 0.1'``

This command has the following named parameters for advanced drilldown:

  * ``drilldowns[${LABEL}].keys=null``
  * ``drilldowns[${LABEL}].sort_keys=null``
  * ``drilldowns[${LABEL}].output_columns="_key, _nsubrecs"``
  * ``drilldowns[${LABEL}].offset=0``
  * ``drilldowns[${LABEL}].limit=10``
  * ``drilldowns[${LABEL}].calc_types=NONE``
  * ``drilldowns[${LABEL}].calc_target=null``
  * ``drilldowns[${LABEL}].filter=null``
  * ``drilldowns[${LABEL}].max_n_target_records=-1``
  * ``drilldowns[${LABEL}].columns[${NAME}].stage=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].flags=COLUMN_SCALAR``
  * ``drilldowns[${LABEL}].columns[${NAME}].type=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].value=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].window.sort_keys=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].window.group_keys=null``

.. deprecated:: 6.0.3

   ``drilldown[...]`` syntax is deprecated, Use ``drilldowns[...]``
   instead.

You can use one or more alphabets, digits, ``_`` and ``.`` for
``${LABEL}``. For example, ``parent.sub1`` is a valid ``${LABEL}``.

Parameters that have the same ``${LABEL}`` are grouped.

For example, the following parameters specify one drilldown:

  * ``--drilldowns[label].keys column``
  * ``--drilldowns[label].sort_keys -_nsubrecs``

The following parameters specify two drilldowns:

  * ``--drilldowns[label1].keys column1``
  * ``--drilldowns[label1].sort_keys -_nsubrecs``
  * ``--drilldowns[label2].keys column2``
  * ``--drilldowns[label2].sort_keys _key``

Usage
-----

Let's learn about ``select`` usage with examples. This section shows
many popular usages.

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/commands/select/usage_setup.log
.. table_create Entries TABLE_HASH_KEY ShortText
.. column_create Entries content COLUMN_SCALAR Text
.. column_create Entries n_likes COLUMN_SCALAR UInt32
.. column_create Entries tag COLUMN_SCALAR ShortText
.. table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram --normalizer NormalizerAuto
.. column_create Terms entries_key_index COLUMN_INDEX|WITH_POSITION Entries _key
.. column_create Terms entries_content_index COLUMN_INDEX|WITH_POSITION Entries content
.. load --table Entries
.. [
.. {"_key":    "The first post!",
..  "content": "Welcome! This is my first post!",
..  "n_likes": 5,
..  "tag": "Hello"},
.. {"_key":    "Groonga",
..  "content": "I started to use Groonga. It's very fast!",
..  "n_likes": 10,
..  "tag": "Groonga"},
.. {"_key":    "Mroonga",
..  "content": "I also started to use Mroonga. It's also very fast! Really fast!",
..  "n_likes": 15,
..  "tag": "Groonga"},
.. {"_key":    "Good-bye Senna",
..  "content": "I migrated all Senna system!",
..  "n_likes": 3,
..  "tag": "Senna"},
.. {"_key":    "Good-bye Tritonn",
..  "content": "I also migrated all Tritonn system!",
..  "n_likes": 3,
..  "tag": "Senna"}
.. ]

There is a table, ``Entries``, for blog entries. An entry has title,
content, the number of likes for the entry and tag. Title is key of
``Entries``. Content is value of ``Entries.content`` column. The
number of likes is value of ``Entries.n_likes`` column. Tag is value
of ``Entries.tag`` column.

``Entries._key`` column and ``Entries.content`` column are indexed
using ``TokenBigram`` tokenizer. So both ``Entries._key`` and
``Entries.content`` are fulltext search ready.

OK. The schema and data for examples are ready.

.. _select-simple-usage:

Simple usage
^^^^^^^^^^^^

Here is the most simple usage with the above schema and data. It outputs
all records in ``Entries`` table.

.. groonga-command
.. include:: ../../example/reference/commands/select/simple_usage.log
.. select Entries

Why does the command output all records? There are two reasons. The
first reason is that the command doesn't specify any search
conditions. No search condition means all records are matched. The
second reason is that the number of all records is 5. ``select``
command outputs 10 records at a maximum by default. There are only 5
records. It is less than 10. So the command outputs all records.

Search conditions
^^^^^^^^^^^^^^^^^

Search conditions are specified by ``query`` or ``filter``. You can
also specify both ``query`` and ``filter``. It means that selected
records must be matched against both ``query`` and ``filter``.

Search condition: ``query``
"""""""""""""""""""""""""""

``query`` is designed for search box in Web page. Imagine a search box
in google.com. You specify search conditions for ``query`` as space
separated keywords. For example, ``search engine`` means a matched
record should contain two words, ``search`` and ``engine``.

Normally, ``query`` parameter is used for specifying fulltext search
conditions. It can be used for non fulltext search conditions but
``filter`` is used for the propose.

``query`` parameter is used with ``match_columns`` parameter when
``query`` parameter is used for specifying fulltext search
conditions. ``match_columns`` specifies which columnes and indexes are
matched against ``query``.

Here is a simple ``query`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/simple_query.log
.. select Entries --match_columns content --query fast

The ``select`` command searches records that contain a word ``fast``
in ``content`` column value from ``Entries`` table.

``query`` has query syntax but its deatils aren't described here. See
:doc:`/reference/grn_expr/query_syntax` for datails.

Search condition: ``filter``
""""""""""""""""""""""""""""

``filter`` is designed for complex search conditions. You specify
search conditions for ``filter`` as ECMAScript like syntax.

Here is a simple ``filter`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/simple_filter.log
.. select Entries --filter 'content @ "fast" && _key == "Groonga"'

The ``select`` command searches records that contain a word ``fast``
in ``content`` column value and has ``Groonga`` as ``_key`` from
``Entries`` table. There are three operators in the command, ``@``,
``&&`` and ``==``. ``@`` is fulltext search operator. ``&&`` and
``==`` are the same as ECMAScript. ``&&`` is logical AND operator and
``==`` is equality operator.

``filter`` has more operators and syntax like grouping by ``(...)``
its details aren't described here. See
:doc:`/reference/grn_expr/script_syntax` for datails.

Paging
^^^^^^

You can specify range of outputted records by ``offset`` and ``limit``.
Here is an example to output only the 2nd record.

.. groonga-command
.. include:: ../../example/reference/commands/select/paging.log
.. select Entries --offset 1 --limit 1

``offset`` is zero-based. ``--offset 1`` means output range is
started from the 2nd record.

``limit`` specifies the max number of output records. ``--limit 1``
means the number of output records is 1 at a maximium. If no records
are matched, ``select`` command outputs no records.

The total number of records
^^^^^^^^^^^^^^^^^^^^^^^^^^^

You can use ``--limit 0`` to retrieve the total number of recrods
without any contents of records.

.. groonga-command
.. include:: ../../example/reference/commands/select/no_limit.log
.. select Entries --limit 0

``--limit 0`` is also useful for retrieving only the number of matched
records.

Drilldown
^^^^^^^^^

You can get additional grouped results against the search result in
one ``select``. You need to use two or more ``SELECT`` s in SQL but
``select`` in Groonga can do it in one ``select``.

This feature is called as `drilldown
<http://en.wikipedia.org/wiki/Drill_down>`_ in Groonga. It's also
called as `faceted search
<http://en.wikipedia.org/wiki/Faceted_search>`_ in other search
engine.

For example, think about the following situation.

You search entries that has ``fast`` word:

.. groonga-command
.. include:: ../../example/reference/commands/select/usage_drilldown_only_query.log
.. select Entries --filter 'content @ "fast"'

You want to use ``tag`` for additional search condition like
``--filter 'content @ "fast" && tag == "???"``. But you don't know
suitable tag until you see the result of ``content @ "fast"``.

If you know the number of matched records of each available tag, you
can choose suitable tag. You can use drilldown for the case:

.. groonga-command
.. include:: ../../example/reference/commands/select/usage_drilldown.log
.. select Entries --filter 'content @ "fast"' --drilldown tag

``--drilldown tag`` returns a list of pair of available tag and the
number of matched records. You can avoid "no hit search" case by
choosing a tag from the list. You can also avoid "too many search
results" case by choosing a tag that the number of matched records is
few from the list.

You can create the following UI with the drilldown results:

  * Links to narrow search results. (Users don't need to input a
    search query by their keyboard. They just click a link.)

Most EC sites use the UI. See side menu at Amazon.

Groonga supports not only counting grouped records but also finding
the maximum and/or minimum value from grouped records, summing values
in grouped records and so on. See
:ref:`select-drilldown-related-parameters` for details.

Dynamic column
^^^^^^^^^^^^^^

You can create zero or more columns dynamically while a ``select``
execution. You can use them for drilldown by computed value, window
function and so on.

Here is an example that uses dynamic column for drilldown by computed
value. This example creates a new column named
``n_likes_class``. ``n_likes_class`` column has classified value of
``Entry.n_likes`` value. This example classifies ``Entry.n_likes``
column value ``10`` step and the lowest number in the class is the
classified value. If a ``Entry.n_likes`` value is between ``0`` and
``9`` such as ``3`` and ``5``, ``n_likes_class`` value (classified
value) is ``0``. If ``Entry.n_likes`` value is between ``10`` and
``19`` such as ``10`` and ``15``, ``n_likes_class`` value (classified
value) is ``10``.

You can use :doc:`/reference/functions/number_classify` function for
the classification. You need to register ``functions/number`` plugin
by :doc:`plugin_register` command to use
:doc:`/reference/functions/number_classify` function.

This example does drilldown by ``n_likes_class`` value. The drilldown
result will help you to know data trend.

.. groonga-command
.. include:: ../../example/reference/commands/select/usage_dynamic_column.log
.. plugin_register functions/number
.. select \
..   --table Entries \
..   --columns[n_likes_class].stage initial \
..   --columns[n_likes_class].type UInt32 \
..   --columns[n_likes_class].value 'number_classify(n_likes, 10)' \
..   --drilldown n_likes_class \
..   --drilldown_sort_keys _nsubrecs \
..   --output_columns n_likes,n_likes_class

See :ref:`select-dynamic-column-related-parameters` for details.

Window function
^^^^^^^^^^^^^^^

You can compute each record value from values of grouped records. For
example, you can compute sums of each group and puts sums to each
record. The difference against drilldown is drilldown can compute sums
of each group but it puts sums to each group not record.

Here is the result with window function. Each record has sum:

.. list-table::
   :header-rows: 1

   * - Group No.
     - Target value
     - Sum result
   * - 1
     - 5
     - 5
   * - 2
     - 10
     - 25
   * - 2
     - 15
     - 25
   * - 3
     - 3
     - 8
   * - 3
     - 5
     - 8

Here is the result with drilldown. Each group has sum:

.. list-table::
   :header-rows: 1

   * - Group No.
     - Target values
     - Sum result
   * - 1
     - 5
     - 5
   * - 2
     - 10, 15
     - 25
   * - 3
     - 3, 5
     - 8

Window function is useful for data analysis.

Here is an example that sums ``Entries.n_likes`` per
``Entries.tag``:

.. groonga-command
.. include:: ../../example/reference/commands/select/usage_window_function.log
.. plugin_register functions/number
.. select \
..   --table Entries \
..   --columns[n_likes_sum_per_tag].stage initial \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys tag \
..   --output_columns tag,n_likes,n_likes_sum_per_tag

See :ref:`select-window-function-related-parameters` for details.

Parameters
----------

This section describes all parameters. Parameters are categorized.

Required parameters
^^^^^^^^^^^^^^^^^^^

There is a required parameter, ``table``.

.. _select-table:

``table``
"""""""""

Specifies a table to be searched. ``table`` must be specified.

If nonexistent table is specified, an error is returned.

.. groonga-command
.. include:: ../../example/reference/commands/select/table_nonexistent.log
.. select Nonexistent

.. _select-search-related-parameters:

Search related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^

There are search related parameters. Typically, ``match_columns`` and
``query`` parameters are used for implementing a search
box. ``filter`` parameters is used for implementing complex search
feature.

If both ``query`` and ``filter`` are specified, selected records must
be matched against both ``query`` and ``filter``. If both ``query``
and ``filter`` aren't specified, all records are selected.

.. _select-match-columns:

``match_columns``
"""""""""""""""""

Specifies the default target column for fulltext search by ``query``
parameter value. A target column for fulltext search can be specified
in ``query`` parameter. The difference between ``match_columns`` and
``query`` is whether weight and score function are supported or
not. ``match_columns`` supports them but ``query`` doesn't.

Weight is relative importance of target column. A higher weight target
column gets more hit score rather than a lower weight target column
when a record is matched by fulltext search. The default weight is 1.

Here is a simple ``match_columns`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/match_columns_simple.log
.. select Entries --match_columns content --query fast --output_columns '_key, _score'

``--match_columns content`` means the default target column for
fulltext search is ``content`` column and its weight
is 1. ``--output_columns '_key, _score'`` means that the ``select``
command outputs ``_key`` value and ``_score`` value for matched
records.

Pay attention to ``_score`` value. ``_score`` value is the number of
matched counts against ``query`` parameter value. In the example,
``query`` parameter value is ``fast``. The fact that ``_score`` value
is 1 means that ``fast`` appers in ``content`` column only once.  The
fact that ``_score`` value is 2 means that ``fast`` appears in
``content`` column twice.

To specify weight, ``column * weight`` syntax is used. Here is a
weight usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/match_columns_weight.log
.. select Entries --match_columns 'content * 2' --query fast --output_columns '_key, _score'

``--match_columns 'content * 2'`` means the default target column for
fulltext search is ``content`` column and its weight is 2.

Pay attention to ``_score`` value. ``_score`` value is doubled because
weight is 2.

You can specify one or more columns as the default target columns for
fulltext search. If one or more columns are specified, fulltext search
is done for all columns and scores are accumulated. If one of the
columns is matched against ``query`` parameter value, the record is
treated as matched.

To specify one or more columns, ``column1 * weight1 || column2 *
weight2 || ...`` syntax is used. ``* weight`` can be omitted. If it is
omitted, 1 is used for weight. Here is a one or more columns usage
example.

.. groonga-command
.. include:: ../../example/reference/commands/select/match_columns_some_columns.log
.. select Entries --match_columns '_key * 10 || content' --query groonga --output_columns '_key, _score'

``--match_columns '_key * 10 || content'`` means the default target
columns for fulltext search are ``_key`` and ``content`` columns and
``_key`` column's weight is 10 and ``content`` column's weight
is 1. This weight allocation means ``_key`` column value is more
important rather than ``content`` column value. In this example, title
of blog entry is more important rather thatn content of blog entry.

You can also specify score function. See :doc:`/reference/scorer` for
details.

Note that score function isn't related to :ref:`select-scorer`
parameter.

You can also specify index columns to ``--match_columns``
parameter. In such a case, you can control what index column should be
used and what data column should be searched explicitly.

If data column is specified to ``--match_columns``, Groonga searches
related index column automatically, but it is not defined as a spec
that which index column is actually selected. Thus, if multiple index
columns are created against one data column, there is no way to know
which index column will be selected in advance. (In most cases, latest
created index column will be selected, but it is undocumented
behavior.)

Here is a use case how to specify index column.

* index column for one data column
* index column for multiple data column

If you use index column for one data column, index column which
corresponds to data column is paired. Thus,
``TERMS_TABLE.INDEX_COLUMN`` should be specified. On the otherhand, if
you use index column for multiple data column, all data column is
searched by default. If you want to use specific multiple index column
to search specific data column, Specify index column with data column
name such as ``TERMS_TABLE.INDEX_COLUMN.DATA_COLUMN``. See example use
case :ref:`full-text-search-with-specific-index-name` for details.

.. _select-query:

``query``
"""""""""

Specifies the query text. Normally, it is used for fulltext search
with ``match_columns`` parameter. ``query`` parameter is designed for
a fulltext search form in a Web page. A query text should be formatted
in :doc:`/reference/grn_expr/query_syntax`. The syntax is similar to common search
form like Google's search form. For example, ``word1 word2`` means
that groonga searches records that contain both ``word1`` and
``word2``. ``word1 OR word2`` means that groonga searches records that
contain either ``word1`` or ``word2``.

Here is a simple logical and search example.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_and.log
.. select Entries --match_columns content --query "fast groonga"

The ``select`` command searches records that contain two words
``fast`` and ``groonga`` in ``content`` column value from ``Entries``
table.

Here is a simple logical or search example.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_or.log
.. select Entries --match_columns content --query "groonga OR mroonga"

The ``select`` command searches records that contain one of two words
``groonga`` or ``mroonga`` in ``content`` column value from
``Entries`` table.

See :doc:`/reference/grn_expr/query_syntax` for other syntax.

It can be used for not only fulltext search but also other
conditions. For example, ``column:value`` means the value of
``column`` column is equal to ``value``. ``column:<value`` means the
value of ``column`` column is less than ``value``.

Here is a simple equality operator search example.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_equal.log
.. select Entries --query _key:Groonga

The ``select`` command searches records that ``_key`` column value is
``Groonga`` from ``Entries`` table.

Here is a simple less than operator search example.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_less_than.log
.. select Entries --query n_likes:<11

The ``select`` command searches records that ``n_likes`` column value
is less than ``11`` from ``Entries`` table.

See :doc:`/reference/grn_expr/query_syntax` for other operations.

.. _select-filter:

``filter``
""""""""""

Specifies the filter text. Normally, it is used for complex search
conditions. ``filter`` can be used with ``query`` parameter. If both
``filter`` and ``query`` are specified, there are conbined with
logical and. It means that matched records should be matched against
both ``filter`` and ``query``.

``filter`` parameter is designed for complex conditions. A filter text
should be formatted in :doc:`/reference/grn_expr/script_syntax`. The syntax is
similar to ECMAScript. For example, ``column == "value"`` means that
the value of ``column`` column is equal to ``"value"``. ``column <
value`` means that the value of ``column`` column is less than
``value``.

Here is a simple equality operator search example.

.. groonga-command
.. include:: ../../example/reference/commands/select/filter_equal.log
.. select Entries --filter '_key == "Groonga"'

The ``select`` command searches records that ``_key`` column value is
``Groonga`` from ``Entries`` table.

Here is a simple less than operator search example.

.. groonga-command
.. include:: ../../example/reference/commands/select/filter_less_than.log
.. select Entries --filter 'n_likes < 11'

The ``select`` command searches records that ``n_likes`` column value
is less than ``11`` from ``Entries`` table.

See :doc:`/reference/grn_expr/script_syntax` for other operators.

.. _select-load-table:

``load_table``
""""""""""""""

.. versionadded:: 9.0.1

You can store specified a table a result of ``select`` with ``--load_table``, ``--load-columns`` and ``--load_values`` arguments.
``--load_table`` specifies a table name for storing a result of ``select``.

You must specify a table that already exists.

This argument must use with :ref:`select-load-columns` and :ref:`select-load-values`.

Here is an example that can store ``_id`` and ``timestamp`` that a result of ``select`` in a Logs table specified by ``--load_table``.

.. groonga-command
.. include:: ../../example/reference/commands/select/load_table.log
.. table_create Logs_20150203 TABLE_HASH_KEY ShortText
.. column_create Logs_20150203 timestamp COLUMN_SCALAR Time
..
.. table_create Logs TABLE_HASH_KEY ShortText
.. column_create Logs original_id COLUMN_SCALAR UInt32
.. column_create Logs timestamp_text COLUMN_SCALAR ShortText
..
.. load --table Logs_20150203
.. [
.. {
..   "_key": "2015-02-03:1",
..   "timestamp": "2015-02-03 10:49:00"
.. },
.. {
..   "_key": "2015-02-03:2",
..   "timestamp": "2015-02-03 12:49:00"
.. }
.. ]
.. select \
..   --table Logs_20150203 \
..   --load_table Logs \
..   --load_columns "_key, original_id, timestamp_text" \
..   --load_values "_key, _id, timestamp"
.. select --table Logs

.. _select-load-columns:

``load_columns``
""""""""""""""""

.. versionadded:: 9.0.1

Specifies columns of a table that specifying ``--load-table``.
Stores value of columns that specified with :ref:`select-load-values` in columns that specified with this argument.
You must specify columns that already exists.

This argument must use with :ref:`select-load-table` and :ref:`select-load-values`.

See example of ``--load_table`` for how to use this argument.

.. _select-load-values:

``load_values``
"""""""""""""""

.. versionadded:: 9.0.1

Specifies columns of result of ``select``.
Specifies columns for storing values into columns that specified with :ref:`select-load-columns`.
You must specify columns that already exists.

This argument must use with :ref:`select-load-table` and :ref:`select-load-columns`.

See example of ``--load_table`` for how to use this argument.

.. _select-advanced-search-parameters:

Advanced search parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. _select-match-escalation-threshold:

``match_escalation_threshold``
""""""""""""""""""""""""""""""

.. versionadded:: 8.0.1

Specifies threshold to determine whether search storategy
escalation is used or not. The threshold is compared against the
number of matched records. If the number of matched records is equal
to or less than the threshold, the search storategy escalation is
used. See :doc:`/spec/search` about the search storategy escalation.

The default threshold is 0. It means that search storategy escalation
is used only when no records are matched.

The default threshold can be customized by one of the followings.

  * ``--with-match-escalation-threshold`` option of configure
  * ``--match-escalation-threshold`` option of groonga command
  * ``match-escalation-threshold`` configuration item in configuration
    file

Here is a simple ``match_escalation_threshold`` usage example. The
first ``select`` doesn't have ``match_escalation_threshold``
parameter. The second ``select`` has ``match_escalation_threshold``
parameter.

.. groonga-command
.. include:: ../../example/reference/commands/select/match_escalation_threshold.log
.. select Entries --match_columns content --query groo
.. select Entries --match_columns content --query groo --match_escalation_threshold -1

The first ``select`` command searches records that contain a word
``groo`` in ``content`` column value from ``Entries`` table. But no
records are matched because the ``TokenBigram`` tokenizer tokenizes
``groonga`` to ``groonga`` not ``gr|ro|oo|on|ng|ga``. (The
``TokenBigramSplitSymbolAlpha`` tokenizer tokenizes ``groonga`` to
``gr|ro|oo|on|ng|ga``. See :doc:`/reference/tokenizers` for details.)
It means that ``groonga`` is indexed but ``groo`` isn't indexed. So no
records are matched against ``groo`` by exact match. In the case, the
search storategy escalation is used because the number of matched
records (0) is equal to ``match_escalation_threshold`` (0). One record
is matched against ``groo`` by unsplit search.

The second ``select`` command also searches records that contain a
word ``groo`` in ``content`` column value from ``Entries`` table. And
it also doesn't find matched records. In this case, the search
storategy escalation is not used because the number of matched
records (0) is larger than ``match_escalation_threshold`` (-1). So no
more searches aren't executed. And no records are matched.

.. _select-match-escalation:

``match_escalation``
""""""""""""""""""""

Specifies how to use match escalation. See also
:ref:`select-match-escalation` and :doc:`/spec/search` about the match
escalation.

Here are available values:

.. list-table::
   :header-rows: 1

   * - Value
     - Description
   * - ``auto``
     - Groonga uses :ref:`select-match-escalation-threshold` to
       determine whether match escalation is used or not.

       This is the default.
   * - ``yes``
     - Groonga always uses match escalation.
   * - ``no``
     - Groonga never use match escalation.

``--match_escalation yes`` is stronger than
``--match_escalation_threshold 9999...999``. ``--filter 'true &&
column @ "query"`` with ``--match_escalation yes`` uses match
escalation. ``--filter 'true && column @ "query"`` with
``--match_escalation_threshold 9999...999`` doesn't use match
escalation.

Here is a simple ``match_escalation`` usage example. The first
``select`` doesn't have ``match_escalation`` parameter. The
second ``select`` has ``match_escalation`` parameter.

.. groonga-command
.. include:: ../../example/reference/commands/select/match_escalation.log
.. select Entries --filter 'true && content @ "groo"'
.. select Entries --filter 'true && content @ "groo"' --match_escalation yes

The first ``select`` command searches records that contain a word
``groo`` in ``content`` column value from ``Entries`` table. But no
records are matched because the ``TokenBigram`` tokenizer tokenizes
``groonga`` to ``groonga`` not ``gr|ro|oo|on|ng|ga``.

The second ``select`` command also searches records that contain a
word ``groo`` in ``content`` column value from ``Entries`` table. And
it uses match escalation. So it can find matched records.

.. _select-query-expansion:

``query_expansion``
"""""""""""""""""""

.. deprecated:: 3.0.2
   Use :ref:`select-query-expander` instead.

.. _select-query-flags:

``query_flags``
"""""""""""""""

It customs ``query`` parameter syntax. You cannot update column value
by ``query`` parameter by default. But if you specify
``ALLOW_COLUMN|ALLOW_UPDATE`` as ``query_flags``, you can update
column value by ``query``.

Here are available values:

* ``ALLOW_PRAGMA``
* ``ALLOW_COLUMN``
* ``ALLOW_UPDATE``
* ``ALLOW_LEADING_NOT``
* ``QUERY_NO_SYNTAX_ERROR``
* ``NONE``

``ALLOW_PRAGMA`` enables pragma at the head of ``query``. This is not
implemented yet.

``ALLOW_COLUMN`` enables search againt columns that are not included
in ``match_columns``. To specify column, there are ``COLUMN:...``
syntaxes.

``ALLOW_UPDATE`` enables column update by ``query`` with
``COLUMN:=NEW_VALUE`` syntax. ``ALLOW_COLUMN`` is also required to
update column because the column update syntax specifies column.

``ALLOW_LEADING_NOT`` enables leading NOT condition with ``-WORD``
syntax. The query searches records that doesn't match
``WORD``. Leading NOT condition query is heavy query in many cases
because it matches many records. So this flag is disabled by
default. Be careful about it when you use the flag.

``QUERY_NO_SYNTAX_ERROR`` enables never causes syntax error for query.
This flag is useful when an application uses user input directly and doesn't want to show syntax error to the user and in a log.
This flag is disabled by default.

``NONE`` is just ignores. You can use ``NONE`` for specifying no flags.

They can be combined by separated ``|`` such as
``ALLOW_COLUMN|ALLOW_UPDATE``.

The default value is ``ALLOW_PRAGMA|ALLOW_COLUMN``.

Here is a usage example of ``ALLOW_COLUMN``.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_flags_allow_column.log
.. select Entries --query content:@mroonga --query_flags ALLOW_COLUMN

The ``select`` command searches records that contain ``mroonga`` in
``content`` column value from ``Entries`` table.

Here is a usage example of ``ALLOW_UPDATE``.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_flags_allow_update.log
.. table_create Users TABLE_HASH_KEY ShortText
.. column_create Users age COLUMN_SCALAR UInt32
.. load --table Users
.. [
.. {"_key": "alice", "age": 18},
.. {"_key": "bob",   "age": 20}
.. ]
.. select Users --query age:=19 --query_flags ALLOW_COLUMN|ALLOW_UPDATE
.. select Users

The first ``select`` command sets ``age`` column value of all records
to ``19``. The second ``select`` command outputs updated ``age``
column values.

Here is a usage example of ``ALLOW_LEADING_NOT``.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_flags_allow_leading_not.log
.. select Entries --match_columns content --query -mroonga --query_flags ALLOW_LEADING_NOT

The ``select`` command searches records that don't contain ``mroonga``
in ``content`` column value from ``Entries`` table.

Here are a schema definition and sample data to describe other flags:

.. groonga-command
.. include:: ../../example/reference/commands/select/query_flags_setup.log
.. table_create --name Magazine --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table Magazine --name title --type ShortText
.. load --table Magazine
.. [
.. {"_key":"http://test.jp/magazine/webplus","title":"WEB+"},
.. {"_key":"http://test.jp/magazine/database","title":"DataBase"},
.. ]

Here is an example of ``QUERY_NO_SYNTAX_ERROR``:

.. groonga-command
.. include:: ../../example/reference/commands/select/query_flags_query_no_syntax_error.log
.. select Magazine --match_columns title --query 'WEB +'  --query_flags ALLOW_PRAGMA|ALLOW_COLUMN|QUERY_NO_SYNTAX_ERROR

If you don't specify this flag, the quey causes a syntax error as below.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_flags_no_query_no_syntax_error.log
.. select Magazine --match_columns title --query 'WEB +'  --query_flags ALLOW_PRAGMA|ALLOW_COLUMN

Here is a usage example of ``NONE``.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_flags_none.log
.. select Entries --match_columns content --query 'mroonga OR _key:Groonga' --query_flags NONE

The ``select`` command searches records that contain one of two words
``mroonga`` or ``_key:Groonga`` in ``content`` from ``Entries`` table.
Note that ``_key:Groonga`` doesn't mean that the value of ``_key``
column is equal to ``Groonga``. Because ``ALLOW_COLUMN`` flag is not
specified.

See also :doc:`/reference/grn_expr/query_syntax`.

.. _select-query-expander:

``query_expander``
""""""""""""""""""

It's for query expansion. Query expansion substitutes specific words
to another words in query. Nomally, it's used for synonym search.

It specifies a column that is used to substitute ``query`` parameter
value. The format of this parameter value is
"``${TABLE}.${COLUMN}``". For example, "``Terms.synonym``" specifies
``synonym`` column in ``Terms`` table.

Table for query expansion is called "substitution table". Substitution
table's key must be ``ShortText``. So array table (``TABLE_NO_KEY``)
can't be used for query expansion. Because array table doesn't have
key.

Column for query expansion is called "substitution
column". Substitution column's value type must be
``ShortText``. Column type must be vector (``COLUMN_VECTOR``).

Query expansion substitutes key of substitution table in query with
values in substitution column. If a word in ``query`` is a key of
substitution table, the word is substituted with substitution column
value that is associated with the key. Substition isn't performed
recursively. It means that substitution target words in substituted
query aren't substituted.

Here is a sample substitution table to show a simple
``query_expander`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_expander_substitution_table.log
.. table_create Thesaurus TABLE_PAT_KEY ShortText --normalizer NormalizerAuto
.. column_create Thesaurus synonym COLUMN_VECTOR ShortText
.. load --table Thesaurus
.. [
.. {"_key": "mroonga", "synonym": ["mroonga", "tritonn", "groonga mysql"]},
.. {"_key": "groonga", "synonym": ["groonga", "senna"]}
.. ]

``Thesaurus`` substitution table has two synonyms, ``"mroonga"`` and
``"groonga"``. If an user searches with ``"mroonga"``, Groonga
searches with ``"((mroonga) OR (tritonn) OR (groonga mysql))"``. If an
user searches with ``"groonga"``, Groonga searches with ``"((groonga)
OR (senna))"``.

Normally, it's good idea that substitution table uses a
normalizer. For example, if normalizer is used, substitute target word
is matched in case insensitive manner. See
:doc:`/reference/normalizers` for available normalizers.

Note that those synonym values include the key value such as
``"mroonga"`` and ``"groonga"``. It's recommended that you include the
key value. If you don't include key value, substituted value doesn't
include the original substitute target value. Normally, including the
original value is better search result. If you have a word that you
don't want to be searched, you should not include the original
word. For example, you can implement "stop words" by an empty vector
value.

Here is a simple ``query_expander`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_expander_substitute.log
.. select Entries --match_columns content --query "mroonga"
.. select Entries --match_columns content --query "mroonga" --query_expander Thesaurus.synonym
.. select Entries --match_columns content --query "((mroonga) OR (tritonn) OR (groonga mysql))"

The first ``select`` command doesn't use query expansion. So a record
that has ``"tritonn"`` isn't found. The second ``select`` command uses
query expansion. So a record that has ``"tritonn"`` is found. The
third ``select`` command doesn't use query expansion but it is same as
the second ``select`` command. The third one uses expanded query.

Each substitute value can contain any :doc:`/reference/grn_expr/query_syntax` syntax
such as ``(...)`` and ``OR``. You can use complex substitution by
using those syntax.

Here is a complex substitution usage example that uses query syntax.

.. groonga-command
.. include:: ../../example/reference/commands/select/query_expander_complex.log
.. load --table Thesaurus
.. [
.. {"_key": "popular", "synonym": ["popular", "n_likes:>=10"]}
.. ]
.. select Entries --match_columns content --query "popular" --query_expander Thesaurus.synonym

The ``load`` command registers a new synonym ``"popular"``. It is
substituted with ``((popular) OR (n_likes:>=10))``. The substituted
query means that "popular" is containing the word "popular" or 10 or
more liked entries.

The ``select`` command outputs records that ``n_likes`` column value
is equal to or more than ``10`` from ``Entries`` table.

.. _select-n-worksers:

``n_workers``
"""""""""""""

TODO: translate

.. note::

  これは試験的な機能です。

  :doc:`/reference/command/command_version` 3 以降を指定する必要があります。
  
  Apache Arrow が有効である必要があります。

このパラメータの値に ``-1`` または ``2`` 以上を指定すると、 :ref:`_select-drilldown` と :ref:`スライス <select-slice-related-parameters>` を並列で実行します。

デフォルトでは各 ``drilldown`` 、 ``drilldowns`` 、 ``slices`` を直列に実行します。
つまり、1つの処理が終わったら次の処理を実行します。
そのため、 ``drilldown`` 、 ``drilldowns`` 、 ``slices`` がたくさんある場合は、クエリーの実行時間が長くなる傾向にあります。

``n_workers`` を使うと複数の ``drilldown`` 、 ``drilldowns`` 、 ``slices``  を並列に実行できます。
そのため、従来はすべての処理の総和分の実行時間がかかっていたところが並列に実行する分短縮できます。
この並列実行は、 ``select`` コマンドごとに行います。

依存関係のないスライスとドリルダウンを並列実行します。

依存関係がないとは、 ``dorilldowns.table`` を使用して他のドリルダウンやスライスの結果を参照していないことです。

依存関係がある場合、つまり、 ``drilldowns.table`` を使用している場合、依存するドリルダウンやスライスの処理の終了を待ちます。
したがって依存関係がある場合は並列度が下がります。

並列に実行するということは、複数のCPUを同時に使用するということです。
CPUのリソースに空きがないのに並列に実行しようとするとかえって遅くなることがあります。
対象のCPUが実行している別処理が終わるのを待たないといけないからです。

CPUのリソースに空きがあるかどうか、どのくらいの ``n_workers`` を指定すべきかの判断基準はどのようなシステム構成かに依存します。

たとえば、CPUが4個のシステムで :doc:`/reference/executables/groonga-server-http` を使うケースを考えます。

:doc:`/reference/executables/groonga-server-http` は各リクエストごとに1スレッド（= 1CPU）を割り当てて処理します。

平均同時接続数が4のとき、CPUを既に4つ使用しているのでCPUのリソースに空きはありません。
各リクエストを処理するためにすべてのCPUが使われているからです。

平均同時接続数が2のとき、リクエストを処理するためにはCPUを2つしか使用していないので、2つ空きがあります。
``n_workers`` に2を指定すると、 ``select`` コマンドはリクエストを処理するためのスレッドを含んで最大で2つのCPUを使用します。
そのため、 ``select`` コマンドが同時にリクエストされたときに、最大で4つのCPUを使用することになり丁度よくなります。
2より大きな値を指定すると、CPUのリソースよりも高い並列度になってしまうので、かえって実行が遅くなる可能性があります。

``n_workers`` は指定した値に応じて以下の動作をします。

* ``0`` または ``1`` を指定した場合

  * 並列実行しません。

* ``2`` 以上を指定した場合
  
  * 最大で指定したスレッド数で並列実行します。

* ``-1`` 以下を指定した場合
  
  * 最大でCPUのコア数分のスレッドで並列実行します。

デフォルト値は0です。つまり、並列実行しません。

.. note::
    
    環境変数 ``GRN_SELECT_N_WORKERS_DEFAULT`` で指定することでデフォルト値を変更することができます。

Output related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^

.. _select-output-columns:

``output_columns``
""""""""""""""""""

Specifies output columns separated by ``,``.

Here is a simple ``output_columns`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/output_columns_simple.log
.. select Entries --output_columns '_id, _key' --limit 1

The ``select`` command just outputs ``_id`` and ``_key`` column
values.

``*`` is a special value. It means that all columns that are not
:doc:`/reference/columns/pseudo`.

Here is a ``*`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/output_columns_asterisk.log
.. select Entries --output_columns '_key, *' --limit 1

The ``select`` command outputs ``_key`` pseudo column, ``content``
column and ``n_likes`` column values but doesn't output ``_id`` pseudo
column value.

The default value is ``_id, _key, *``. It means that all column
values except ``_score`` are outputted.

.. _select-sortby:

``sortby``
""""""""""

.. deprecated:: 6.0.3
   Use :ref:`select-sort-keys` instead.

.. _select-sort-keys:

``sort_keys``
"""""""""""""

Specifies sort keys separated by ``,``. Each sort key is column
name.

Here is a simple ``sort_keys`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/sort_keys_simple.log
.. select Entries --sort_keys 'n_likes, _id'

The ``select`` command sorts by ``n_likes`` column value in ascending
order. For records that has the same ``n_likes`` are sorted by ``_id``
in ascending order. ``"Good-bye Senna"`` and ``"Good-bye Tritonn"``
are the case.

If you want to sort in descending order, add ``-`` before column name.

Here is a descending order ``sort_keys`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/sort_keys_descending.log
.. select Entries --sort_keys '-n_likes, _id'

The ``select`` command sorts by ``n_likes`` column value in descending
order. But ascending order is used for sorting by ``_id``.

You can use ``_score`` pseudo column in ``sort_keys`` if you use
``query`` or ``filter`` parameter.

.. groonga-command
.. include:: ../../example/reference/commands/select/sort_keys_score_with_query.log
.. select Entries --match_columns content --query fast --sort_keys -_score --output_columns '_key, _score'

The ``select`` command sorts matched records by hit score in
descending order and outputs record key and hit score.

If you use ``_score`` without ``query`` nor ``filter`` parameters,
it's just ignored but get a warning in log file.

.. _select-offset:

``offset``
""""""""""

Specifies offset to determine output records range. Offset is
zero-based. ``--offset 1`` means output range is started from the 2nd
record.

.. groonga-command
.. include:: ../../example/reference/commands/select/offset_simple.log
.. select Entries --sort_keys _id --offset 3 --output_columns _key

The ``select`` command outputs from the 4th record.

You can specify negative value. It means that ``the number of matched
records + offset``. If you have 3 matched records and specify
``--offset -2``, you get records from the 2nd (``3 + -2 = 1``. ``1``
means 2nd. Remember that offset is zero-based.) record to the 3rd
record.

.. groonga-command
.. include:: ../../example/reference/commands/select/offset_negative.log
.. select Entries --sort_keys _id --offset -2 --output_columns _key

The ``select`` command outputs from the 4th record because the total
number of records is ``5``.

The default value is ``0``.

.. _select-limit:

``limit``
"""""""""

Specifies the max number of output records. If the number of
matched records is less than ``limit``, all records are outputted.

Here is a simple ``limit`` usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/limit_simple.log
.. select Entries --sort_keys _id --offset 2 --limit 3 --output_columns _key

The ``select`` command outputs the 3rd, the 4th and the 5th records.

You can specify negative value. It means that ``the number of matched
records + limit + 1``. For example, ``--limit -1`` outputs all
records. It's very useful value to show all records.

Here is a simple negative ``limit`` value usage example.

.. groonga-command
.. include:: ../../example/reference/commands/select/limit_negative.log
.. select Entries --limit -1

The ``select`` command outputs all records.

The default value is ``10``.

.. _select-scorer:

``scorer``
""""""""""

TODO: write in English and add example.

検索条件にマッチする全てのレコードに対して適用するgrn_exprをscript形式で指定します。

scorerは、検索処理が完了し、ソート処理が実行される前に呼び出されます。従って、各レコードのスコアを操作する式を指定しておけば、検索結果のソート順序をカスタマイズできるようになります。

.. _select-dynamic-column-related-parameters:

Dynamic column related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 6.0.6

This section describes dynamic column related parameters. You can use
dynamic column for window function but this section doesn't describe
window function. See :ref:`select-window-function-related-parameters`
for window function.

You can create zero or more columns and fill values into these columns
while a ``select`` execution. These columns are called as "dynamic
columns". You can use dynamic columns as same as normal columns after
dynamic columns are created.

Dynamic column has performance merit because its values are computed
at once and reused computed values.

Dynamic column increases memory usage because its values are kept
while the ``select`` execution.

You need to use dynamic column in the following cases:

  * You want to name values like ``AS`` in SQL.

  * You want to use computed values for drilldown. Groonga doesn't
    support drilldown target value computation in drilldown.

  * You want to use window function.

There are some points to create dynamic columns. You must specify
``stage`` to each dynamic column to control dynamic columns creation
points. It's important that you choose proper point to get better
performance.

For example, it's not a good idea that you create a dynamic column
that is only used for output for all records. The number of output
records will be a little even if there are many records in a
table. Because you will filter, sort and limit all records and output
only the limited records in many cases.

See :ref:`select-columns-name-stage` for ``stage`` detail.

Here are parameters for dynamic column. They don't include window
function related parameters. See
:ref:`select-window-function-related-parameters` for window function
related parameters:

.. list-table::
   :header-rows: 1

   * - Name
     - Default value
     - Required or optional
   * - ``columns[${NAME}].stage``
     - ``null``
     - Required
   * - ``columns[${NAME}].flags``
     - ``COLUMN_SCALAR``
     - Optional
   * - ``columns[${NAME}].type``
     - ``null``
     - Required
   * - ``columns[${NAME}].value``
     - ``null``
     - Required

You need to specify multiple parameters for a dynamic
column. ``${NAME}`` is the name for each dynamic column. Parameters
that use the same ``${NAME}`` are treated as parameters for the same
dynamic column. Here is an example to specify parameters for 2 dynamic
columns (``name1`` and ``name2``)::

    --columns[name1].stage initial
    --columns[name1].type UInt32
    --columns[name1].value 29

    --columns[name2].stage filtered
    --columns[name2].type ShortText
    --columns[name2].value "29"

.. _select-columns-name-stage:

``columns[${NAME}].stage``
""""""""""""""""""""""""""

.. versionadded:: 6.0.6

Specifies when the dynamic column is created. This is a required
parameter to create a dynamic column.

Here are available stages:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``initial``
     - Dynamic column is created at first.
   * - ``filtered``
     - Dynamic column is created after :ref:`select-query` and
       :ref:`select-filter` are evaluated.
   * - ``output``
     - Dynamic column is created before :ref:`select-output-columns`
       is evaluated.

Here is the ``select`` process flow with dynamic column creation
points. You should choose stage as late as possible:

  #. Creates dynamic columns for ``initial`` stage. All records have
     these dynamic columns.

  #. Evaluates :ref:`select-query` and :ref:`select-filter`. You can
     use dynamic columns created in ``initial`` stage.

  #. Creates dynamic columns for ``filtered`` stage. Only filtered
     records have these dynamic columns.

  #. Evaluates :ref:`select-adjuster`. You can use dynamic columns
     created in ``initial`` stage and ``filtered`` stage.

  #. Evaluates :ref:`select-scorer`. You can use dynamic columns
     created in ``initial`` stage and ``filtered`` stage.

  #. Evaluates :ref:`select-sort-keys`, :ref:`select-offset` and
     :ref:`select-limit`. You can use dynamic columns created in
     ``initial`` stage and ``filtered`` stage.

  #. Evaluates :ref:`select-slice-related-parameters`. You can use
     dynamic columns created in ``initial`` stage and ``filtered`` stage.

  #. Evaluates :ref:`select-drilldown-related-parameters` and
     :ref:`select-advanced-drilldown-related-parameters`. You can use
     dynamic columns created in ``initial`` stage and ``filtered`` stage.
     Note that you can create dynamic columns in each drilldown.

  #. Creates dynamic columns for ``output`` stage. Only
     :ref:`select-limit` records have these dynamic columns.

  #. Evaluates :ref:`select-output-columns`. You can use dynamic
     columns created in ``initial`` stage, ``filtered`` stage and
     ``output`` stage.

Here is an example that creates ``is_popular`` column at ``initial``
stage. You can use ``is_popular`` in all parameters such as ``filter``
and ``output_columns``:

.. groonga-command
.. include:: ../../example/reference/commands/select/columns_name_stage.log
.. select Entries \
..   --columns[is_popular].stage initial \
..   --columns[is_popular].type Bool \
..   --columns[is_popular].value 'n_likes >= 10' \
..   --filter is_popular \
..   --output_columns _id,is_popular,n_likes

.. _select-columns-name-flags:

``columns[${NAME}].flags``
""""""""""""""""""""""""""

.. versionadded:: 6.0.6

Specifies flags for the dynamic column. It's the same as ``flags``
parameter for :doc:`column_create`. See :ref:`column-create-flags` for
available flags.

The default value is ``COLUMN_SCALAR``.

Here is a ``columns[${NAME}].flags`` example. It creates a vector
column by ``COLUMN_VECTOR`` flags. ``plugin_register
functions/vector`` is for using :doc:`/reference/functions/vector_new`
function:

.. groonga-command
.. include:: ../../example/reference/commands/select/columns_name_flags.log
.. plugin_register functions/vector
.. select Entries \
..   --columns[vector].stage initial \
..   --columns[vector].flags COLUMN_VECTOR \
..   --columns[vector].type UInt32 \
..   --columns[vector].value 'vector_new(1, 2, 3)' \
..   --output_columns _id,vector

.. _select-columns-name-type:

``columns[${NAME}].type``
"""""""""""""""""""""""""

.. versionadded:: 6.0.6

Specifies value type for the dynamic column. It's the same as ``type``
parameter for :doc:`column_create`. See :ref:`column-create-type` for
available types.

This is a required parameter.

Here is an example that creates a ``ShortText`` type column. Stored
value is casted to ``ShortText`` automatically. In this example,
number is casted to ``ShortText``:

.. groonga-command
.. include:: ../../example/reference/commands/select/columns_name_type.log
.. select Entries \
..   --columns[n_likes_string].stage initial \
..   --columns[n_likes_string].type ShortText \
..   --columns[n_likes_string].value n_likes \
..   --output_columns _id,n_likes,n_likes_string

.. _select-columns-name-value:

``columns[${NAME}].value``
""""""""""""""""""""""""""

.. versionadded:: 6.0.6

Specifies expression that generates values for the dynamic column. The
expression uses :doc:`/reference/grn_expr/script_syntax`. It's the
same syntax in :ref:`select-filter`. For example, ``1 + 1``,
``string_length("Hello")``, ``column * 1.08`` and so on are valid
expressions.

You need to specify :doc:`/reference/window_function` as ``value``
value and other window function related parameters when you use window
function. See :ref:`select-window-function-related-parameters` for
details.

This is a required parameter.

Here is an example that creates a new dynamic column that stores the
number of characters of content. This example uses
:doc:`/reference/functions/string_length` function in
``functions/string`` plugin to compute the number of characters in a
string. :doc:`plugin_register` is used to register
``functions/string`` plugin:

.. groonga-command
.. include:: ../../example/reference/commands/select/columns_name_value.log
.. plugin_register functions/string
.. select Entries \
..   --columns[content_length].stage initial \
..   --columns[content_length].type UInt32 \
..   --columns[content_length].value 'string_length(content)' \
..   --output_columns _id,content,content_length

.. _select-window-function-related-parameters:

Window function related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 6.0.6

This section describes window function related parameters. You need to
use dynamic column for using window function. See
:ref:`select-dynamic-column-related-parameters` for dynamic column.

Window function in Groonga is similar to window function in
SQL. Normal function computes its result with only the current
record. On the other hand, window function computes its result with
multiple records. Window function is useful to data analysis because
it can process multiple records.

You can find supported window functions at
:doc:`/reference/window_function`. For example,
:doc:`/reference/window_functions/window_sum` is a window function. It
sums numbers in the target records.

Window function processes records that are grouped by the specified
group keys. For example, window function processes three groups
(``Hello`` group, ``Groonga`` group and ``Senna`` group) in the
following case. :doc:`/reference/window_functions/window_sum` sums
``n_likes`` values in each group:

.. list-table::
   :header-rows: 1

   * - Group No.
     - Group key value
     - ``n_likes`` value
     - :doc:`/reference/window_functions/window_sum` result
   * - 1
     - ``Hello``
     - 5
     - 5
   * - 2
     - ``Groonga``
     - 10
     - 25
   * - 2
     - ``Groonga``
     - 15
     - 25
   * - 3
     - ``Senna``
     - 3
     - 6
   * - 3
     - ``Senna``
     - 3
     - 6

You can specify no group keys. In the case, window function processes
only one group that includes all
records. :doc:`/reference/window_functions/window_sum` sums all
``n_likes`` values in the following case:

.. list-table::
   :header-rows: 1

   * - Group No.
     - ``n_likes`` value
     - :doc:`/reference/window_functions/window_sum` result
   * - 1
     - 5
     - 36
   * - 1
     - 10
     - 36
   * - 1
     - 15
     - 36
   * - 1
     - 3
     - 36
   * - 1
     - 3
     - 36

Window function processes records in each group in the specified
order. You can specify no sort keys like the above group keys
example.

The behavior when you specify no sort keys depends on each window
function specification. For example,
:doc:`/reference/window_functions/window_sum` uses different behavior
whether sort keys are specified or not. If you specify not sort keys,
:doc:`/reference/window_functions/window_sum` sums values of all
records in the group and puts it to all target records like the above
group keys example. If you specify sort keys,
:doc:`/reference/window_functions/window_sum` behaves as cumulative
sum. :doc:`/reference/window_functions/window_sum` sums values of all
records in the group in sequence and puts the current sum to the
current record like the following:

.. list-table::
   :header-rows: 1

   * - Group No.
     - Group key value
     - Sort key value
     - ``n_likes`` value
     - :doc:`/reference/window_functions/window_sum` result
     - Note
   * - 1
     - ``Hello``
     - 1
     - 5
     - 5
     - The first record in group No. 1. (``5 = 5``)
   * - 2
     - ``Groonga``
     - 90
     - 10
     - 10
     - The first record in group No. 2. (``10 = 10``)
   * - 2
     - ``Groonga``
     - 91
     - 15
     - 25
     - The second record in group No. 2. (``10 + 15 = 25``)
   * - 3
     - ``Senna``
     - 200
     - 3
     - 8
     - The second record in group No. 3. (``5 + 3 = 8``)
   * - 3
     - ``Senna``
     - 100
     - 5
     - 5
     - The first record in group No. 3. (``5 = 5``)

Here are parameters for window function. You need to specify both
window function related parameters and required dynamic columns
parameters. Because window function is implemented based on dynamic
column. See :ref:`select-dynamic-column-related-parameters` for
dynamic column related parameters:

.. list-table::
   :header-rows: 1

   * - Name
     - Required or optional
     - Note
   * - ``columns[${NAME}].value``
     - Required
     - Use :doc:`/reference/window_function`.
   * - ``columns[${NAME}].window.sort_keys``
     - Required if ``columns[${NAME}].window.group_keys`` isn't specified.
     -
   * - ``columns[${NAME}].window.group_keys``
     - Required if ``columns[${NAME}].window.sort_keys`` isn't specified.
     -

.. _select-columns-name-window-sort-keys:

``columns[${NAME}].window.sort_keys``
"""""""""""""""""""""""""""""""""""""

.. versionadded:: 6.0.6

Specifies sort keys in each group. Window function processes records
in each group in the specified order.

Sort keys are separated by ``,``. Each sort key is column name. It's
the same as :ref:`select-sort-keys`.

You must specify :ref:`select-columns-name-window-sort-keys` or
:ref:`select-columns-name-window-group-keys` to use window function.

Here is an example that computes cumulative sum per
``Entries.tag``. Each group is sorted by ``Entries._key``:

.. groonga-command
.. include:: ../../example/reference/commands/select/columns_name_window_sort_keys.log
.. select \
..   --table Entries \
..   --columns[n_likes_cumulative_sum_per_tag].stage initial \
..   --columns[n_likes_cumulative_sum_per_tag].type UInt32 \
..   --columns[n_likes_cumulative_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_cumulative_sum_per_tag].window.sort_keys _key \
..   --columns[n_likes_cumulative_sum_per_tag].window.group_keys tag \
..   --sort_keys _key \
..   --output_columns tag,_key,n_likes,n_likes_cumulative_sum_per_tag

.. _select-columns-name-window-group-keys:

``columns[${NAME}].window.group_keys``
""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.0

Specifies group keys. Window function processes records in each
group. If you specify no group keys, window function processes one
group that includes all records.

Group keys are separated by ``,``. Each group key is column name. It's
the same as :ref:`select-drilldown`.

You must specify :ref:`select-columns-name-window-sort-keys` or
:ref:`select-columns-name-window-group-keys` to use window function.

Here is an example that computes sum per ``Entries.tag``:

.. groonga-command
.. include:: ../../example/reference/commands/select/columns_name_window_group_keys.log
.. select \
..   --table Entries \
..   --columns[n_likes_sum_per_tag].stage initial \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys tag \
..   --sort_keys _key \
..   --output_columns tag,_key,n_likes,n_likes_sum_per_tag

.. _select-drilldown-related-parameters:

Drilldown related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This section describes basic drilldown related parameters. Advanced
drilldown related parameters are described in another section.

.. _select-drilldown:

``drilldown``
"""""""""""""

Specifies keys for grouping separated by ``,``.

Matched records by specified search conditions are grouped by each
key. If you specify no search condition, all records are grouped by
each key.

Here is a simple ``drilldown`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_simple.log
.. select Entries \
..   --output_columns _key,tag \
..   --drilldown tag

The ``select`` command outputs the following information:

  * There is one record that has "Hello" tag.
  * There is two records that has "Groonga" tag.
  * There is two records that has "Senna" tag.

Here is a ``drilldown`` with search condition example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_with_filter.log
.. select Entries \
..   --output_columns _key,tag \
..   --filter 'n_likes >= 5' \
..   --drilldown tag

The ``select`` command outputs the following information:

  * In records that have 5 or larger as ``n_likes`` value:

    * There is one record that has "Hello" tag.
    * There is two records that has "Groonga" tag.

Here is a ``drilldown`` with multiple group keys example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_multiple.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag,n_likes

The ``select`` command outputs the following information:

  * About ``tag``:

    * There is one record that has "Hello" tag.
    * There is two records that has "Groonga" tag.
    * There is two records that has "Senna" tag.

  * About ``n_likes``:

    * There is one record that has "Hello" tag.
    * There is two records that has "Groonga" tag.
    * There is two records that has "Senna" tag.

.. _select-drilldown-sortby:

``drilldown_sortby``
""""""""""""""""""""

.. deprecated:: 6.0.3
   Use :ref:`select-drilldown-sort-keys` instead.

.. _select-drilldown-sort-keys:

``drilldown_sort_keys``
"""""""""""""""""""""""

Specifies sort keys for drilldown outputs separated by ``,``. Each
sort key is column name.

You can refer the number of grouped records by ``_nsubrecs``
:doc:`/reference/columns/pseudo`.

Here is a simple ``drilldown_sort_keys`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_sort_keys_simple.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_sort_keys '-_nsubrecs, _key'

Drilldown result is sorted by the number of grouped records (=
``_nsubrecs`` ) in descending order. If there are grouped results that
the number of records in the group are the same, these grouped results
are sorted by grouped key (= ``_key`` ) in ascending order.

The sort keys are used in all group keys specified in ``drilldown``:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_sort_keys_simple.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown 'tag, n_likes' \
..   --drilldown_sort_keys '-_nsubrecs, _key'

The same sort keys are used in ``tag`` drilldown and ``n_likes``
drilldown.

If you want to use different sort keys for each drilldown, use
:ref:`select-advanced-drilldown-related-parameters`.

.. _select-drilldown-output-columns:

``drilldown_output_columns``
""""""""""""""""""""""""""""

Specifies output columns for drilldown separated by ``,``.

Here is a ``drilldown_output_columns`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_output_columns_simple.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_output_columns _key

The ``select`` command just outputs grouped key.

If grouped key is a referenced type column (= column that its type is
a table), you can access column of the table referenced by the
referenced type column.

Here are a schema definition and sample data to show drilldown against
referenced type column:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_output_columns_referenced_type_column_definition.log
.. table_create Tags TABLE_HASH_KEY ShortText --normalizer NormalizerAuto
.. column_create Tags label COLUMN_SCALAR ShortText
.. column_create Tags priority COLUMN_SCALAR Int32
..
.. table_create Items TABLE_HASH_KEY ShortText
.. column_create Items tag COLUMN_SCALAR Tags
..
.. load --table Tags
.. [
.. {"_key": "groonga", label: "Groonga", priority: 10},
.. {"_key": "mroonga", label: "Mroonga", priority: 5}
.. ]
..
.. load --table Items
.. [
.. {"_key": "A", "tag": "groonga"},
.. {"_key": "B", "tag": "groonga"},
.. {"_key": "C", "tag": "mroonga"}
.. ]

``Tags`` table is a referenced table. ``Items.tag`` is a referenced
type column.

You can refer ``Tags.label`` by ``label`` in
``drilldown_output_columns``:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_output_columns_referenced_type_column_label.log
.. select Items \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_output_columns '_key, label'

You can use ``*`` to refer all columns in referenced table (= ``Tags``):

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_output_columns_referenced_type_column_asterisk.log
.. select Items \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_output_columns '_key, *'

``*`` is expanded to ``label, priority``.

The default value of ``drilldown_output_columns`` is ``_key,
_nsubrecs``. It means that grouped key and the number of records in
the group are output.

You can use more :doc:`/reference/columns/pseudo` in
``drilldown_output_columns`` such as ``_max``, ``_min``, ``_sum`` and
``_avg`` when you use :ref:`select-drilldown-calc-types`. See
``drilldown_calc_types`` document for details.

.. _select-drilldown-offset:

``drilldown_offset``
""""""""""""""""""""

Specifies offset to determine range of drilldown output
records. Offset is zero-based. ``--drilldown_offset 1`` means output
range is started from the 2nd record.

Here is a ``drilldown_offset`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_offset_simple.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_sort_keys _key \
..   --drilldown_offset 1

The ``select`` command outputs from the 2nd record.

You can specify negative value. It means that ``the number of grouped
results + offset``. If you have 3 grouped results and specify
``--drilldown_offset -2``, you get grouped results from the 2st
(``3 + -2 = 1``. ``1`` means 2nd. Remember that offset is zero-based.)
grouped result to the 3rd grouped result.

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_offset_negative.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_sort_keys _key \
..   --drilldown_offset -2

The ``select`` command outputs from the 2nd grouped result because the
total number of grouped results is ``3``.

The default value of ``drilldown_offset`` is ``0``.

.. _select-drilldown-limit:

``drilldown_limit``
"""""""""""""""""""

Specifies the max number of groups in a drilldown. If the number of
groups is less than ``drilldown_limit``, all groups are outputted.

Here is a ``drilldown_limit`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_limit_simple.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_sort_keys _key \
..   --drilldown_offset 1 \
..   --drilldown_limit 2

The ``select`` command outputs the 2rd and the 3rd groups.

You can specify negative value. It means that ``the number of groups +
drilldown_limit + 1``. For example, ``--drilldown_limit -1`` outputs
all groups. It's very useful value to show all groups.

Here is a negative ``drilldown_limit`` value example.

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_limit_negative.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_sort_keys _key \
..   --drilldown_limit -1

The ``select`` command outputs all groups.

The default value of ``drilldown_limit`` is ``10``.

.. _select-drilldown-calc-types:

``drilldown_calc_types``
""""""""""""""""""""""""

Specifies how to calculate (aggregate) values in grouped records by
a drilldown. You can specify multiple calculation types separated by
"``,``". For example, ``MAX,MIN``.

Calculation target values are read from a column of grouped
records. The column is specified by
:ref:`select-drilldown-calc-target`.

You can read calculated value by :doc:`/reference/columns/pseudo` such
as ``_max`` and ``_min`` in :ref:`select-drilldown-output-columns`.

You can use the following calculation types:

.. list-table::
   :header-rows: 1

   * - Type name
     - :doc:`/reference/columns/pseudo` name
     - Need :ref:`select-drilldown-calc-target`
     - Description
   * - ``NONE``
     - Nothing.
     - Not needs.
     - Just ignored.
   * - ``COUNT``
     - ``_nsubrecs``
     - Not needs.
     - Counting grouped records. It's always enabled. So you don't
       need to specify it.
   * - ``MAX``
     - ``_max``
     - Needs.
     - Finding the maximum integer value from integer values in
       grouped records.
   * - ``MIN``
     - ``_min``
     - Needs.
     - Finding the minimum integer value from integer values in
       grouped records.
   * - ``SUM``
     - ``_sum``
     - Needs.
     - Summing integer values in grouped records.
   * - ``AVG``
     - ``_avg``
     - Needs.
     - Averaging integer/float values in grouped records.

Here is a ``MAX`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_calc_types_max.log
.. select Entries \
..   --limit -1 \
..   --output_columns _id,n_likes \
..   --drilldown tag \
..   --drilldown_calc_types MAX \
..   --drilldown_calc_target n_likes \
..   --drilldown_output_columns _key,_max

The ``select`` command groups all records by ``tag`` column value,
finding the maximum ``n_likes`` column value for each group and
outputs pairs of grouped key and the maximum ``n_likes`` column value
for the group. It uses ``_max`` :doc:`/reference/columns/pseudo` to
read the maximum ``n_likes`` column value.

Here is a ``MIN`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_calc_types_min.log
.. select Entries \
..   --limit -1 \
..   --output_columns _id,n_likes \
..   --drilldown tag \
..   --drilldown_calc_types MIN \
..   --drilldown_calc_target n_likes \
..   --drilldown_output_columns _key,_min

The ``select`` command groups all records by ``tag`` column value,
finding the minimum ``n_likes`` column value for each group and
outputs pairs of grouped key and the minimum ``n_likes`` column value
for the group. It uses ``_min`` :doc:`/reference/columns/pseudo` to
read the minimum ``n_likes`` column value.

Here is a ``SUM`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_calc_types_sum.log
.. select Entries \
..   --limit -1 \
..   --output_columns _id,n_likes \
..   --drilldown tag \
..   --drilldown_calc_types SUM \
..   --drilldown_calc_target n_likes \
..   --drilldown_output_columns _key,_sum

The ``select`` command groups all records by ``tag`` column value,
sums all ``n_likes`` column values for each group and outputs pairs
of grouped key and the summed ``n_likes`` column values for the
group. It uses ``_sum`` :doc:`/reference/columns/pseudo` to read the
summed ``n_likes`` column values.

Here is a ``AVG`` example:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_calc_types_avg.log
.. select Entries \
..   --limit -1 \
..   --output_columns _id,n_likes \
..   --drilldown tag \
..   --drilldown_calc_types AVG \
..   --drilldown_calc_target n_likes \
..   --drilldown_output_columns _key,_avg

The ``select`` command groups all records by ``tag`` column value,
averages all ``n_likes`` column values for each group and outputs
pairs of grouped key and the averaged ``n_likes`` column values for
the group. It uses ``_avg`` :doc:`/reference/columns/pseudo` to read
the averaged ``n_likes`` column values.

Here is an example that uses all calculation types:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_calc_types_all.log
.. select Entries \
..   --limit -1 \
..   --output_columns _id,n_likes \
..   --drilldown tag \
..   --drilldown_calc_types MAX,MIN,SUM,AVG \
..   --drilldown_calc_target n_likes \
..   --drilldown_output_columns _key,_nsubrecs,_max,_min,_sum,_avg

The ``select`` command specifies multiple calculation types separated
by "``,``" like ``MAX,MIN,SUM,AVG``. You can use ``_nsubrecs``
:doc:`/reference/columns/pseudo` in
:ref:`select-drilldown-output-columns` without specifying ``COUNT`` in
``drilldown_calc_types``. Because ``COUNT`` is always enabled.

The default value of ``drilldown_calc_types`` is ``NONE``. It means
that only ``COUNT`` is enabled. Because ``NONE`` is just ignored and
``COUNT`` is always enabled.

.. _select-drilldown-calc-target:

``drilldown_calc_target``
"""""""""""""""""""""""""

.. versionadded:: 6.0.3

Specifies the target column for :ref:`select-drilldown-calc-types`.

If you specify a calculation type that needs a target column such as
``MAX`` in :ref:`select-drilldown-calc-types` but you omit
``drilldown_calc_target``, the calculation result is always ``0``.

You can specify only one column name like ``--drilldown_calc_target
n_likes``. You can't specify multiple column name like
``--drilldown_calc_target _key,n_likes``.

You can use referenced value from the target record by combining
"``.``" like ``--drilldown_calc_target
reference_column.nested_reference_column.value``.

See :ref:`select-drilldown-calc-types` to know how to use
``drilldown_calc_target``.

The default value of ``drilldown_calc_target`` is ``null``. It means
that no calculation target column is specified.

.. _select-drilldown-filter:

``drilldown_filter``
""""""""""""""""""""

.. versionadded:: 6.0.3

Specifies the filter condition against the drilled down result.

The syntax is :doc:`/reference/grn_expr/script_syntax`. It's the same
as :ref:`select-filter`.

Here is an example to suppress tags that are occurred only once:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_filter.log
.. select Entries \
..   --limit -1 \
..   --output_columns _id,tag \
..   --drilldown tag \
..   --drilldown_filter '_nsubrecs > 1' \
..   --drilldown_output_columns _key,_nsubrecs

.. _select-drilldown-max-n-target-records:

``drilldown_max_n_target_records``
""""""""""""""""""""""""""""""""""

.. versionadded:: 12.0.0

Specifies the max number of records of the drilldown target table
(filtered result) to use drilldown. If the number of filtered result
is larger than the specified value, some records in filtered result
aren't used for drilldown.

If the specified value is negative, it's processed same as
:ref:`select-limit`. For example, ``-1`` uses all records. The default
value is ``-1``. It means that all filtered records are used by
default.

This feature is useful when filtered result may be very large. A
drilldown against large filtered result may be slow. You can limit the
max number of records to be used for drilldown by this feature.

Here is an example to limit the max number of records to be used for
drilldown. The last 2 records, ``{"_id": 4, "tag": "Senna"}`` and
``{"_id": 5, "tag": "Senna"}``, aren't used:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldown_max_n_target_records.log
.. select Entries \
..   --limit -1 \
..   --output_columns _id,tag \
..   --drilldown tag \
..   --drilldown_max_n_target_records 3

.. _select-advanced-drilldown-related-parameters:

Advanced drilldown related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 4.0.8

You can get multiple drilldown results by specifying multiple group
keys by :ref:`select-drilldown`. But you need to use the same
configuration for all drilldowns. For example,
:ref:`select-drilldown-output-columns` is used by all drilldowns.

You can use a configuration for each drilldown by the following
parameters:

  * ``drilldowns[${LABEL}].keys``
  * ``drilldowns[${LABEL}].sort_keys``
  * ``drilldowns[${LABEL}].output_columns``
  * ``drilldowns[${LABEL}].offset``
  * ``drilldowns[${LABEL}].limit``
  * ``drilldowns[${LABEL}].calc_types``
  * ``drilldowns[${LABEL}].calc_target``
  * ``drilldowns[${LABEL}].filter``
  * ``drilldowns[${LABEL}].max_n_target_records``
  * ``drilldowns[${LABEL}].columns[${NAME}].stage=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].flags=COLUMN_SCALAR``
  * ``drilldowns[${LABEL}].columns[${NAME}].type=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].value=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].window.sort_keys=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].window.group_keys=null``

``${LABEL}`` is a variable. You can use the following characters for
``${LABEL}``:

  * Alphabets
  * Digits
  * ``.``
  * ``_``

``${NAME}`` is a variable. You can use the following characters for
``${NAME}``:

  * Alphabets
  * Digits
  * ``_``

.. note::

   You can use more characters but it's better that you use only these
   characters.

Parameters that has the same ``${LABEL}`` value are grouped. Grouped
parameters are used for one drilldown.

For example, there are 2 groups for the following parameters:

  * ``--drilldowns[label1].keys _key``
  * ``--drilldowns[label1].output_columns _nsubrecs``
  * ``--drilldowns[label2].keys tag``
  * ``--drilldowns[label2].output_columns _key,_nsubrecs``

``drilldowns[label1].keys`` and ``drilldowns[label1].output_columns``
are grouped. ``drilldowns[label2].keys`` and
``drilldowns[label2].output_columns`` are also grouped.

In ``label1`` group, ``_key`` is used for group key and ``_nsubrecs``
is used for output columns.

In ``label2`` group, ``tag`` is used for group key and
``_key,_nsubrecs`` is used for output columns.

See document for corresponding ``drilldown_XXX`` parameter to know how
to use it for the following parameters:

  * ``drilldowns[${LABEL}].sort_keys``: :ref:`select-drilldown-sort-keys`
  * ``drilldowns[${LABEL}].offset``: :ref:`select-drilldown-offset`
  * ``drilldowns[${LABEL}].limit``: :ref:`select-drilldown-limit`
  * ``drilldowns[${LABEL}].calc_types``: :ref:`select-drilldown-calc-types`
  * ``drilldowns[${LABEL}].calc_target``: :ref:`select-drilldown-calc-target`
  * ``drilldowns[${LABEL}].filter``: :ref:`select-drilldown-filter`
  * ``drilldowns[${LABEL}].max_n_target_records``: :ref:`select-drilldown-max-n-target-records`

See document for corresponding ``columns[${NAME}].XXX`` parameter to
know how to use it for the following parameters:

  * ``drilldowns[${LABEL}].columns[${NAME}].flags=COLUMN_SCALAR``:
    :ref:`select-columns-name-flags`
  * ``drilldowns[${LABEL}].columns[${NAME}].type=null``:
    :ref:`select-columns-name-type`
  * ``drilldowns[${LABEL}].columns[${NAME}].value=null``:
    :ref:`select-columns-name-value`
  * ``drilldowns[${LABEL}].columns[${NAME}].window.sort_keys=null``:
    :ref:`select-columns-name-window-sort-keys`
  * ``drilldowns[${LABEL}].columns[${NAME}].window.group_keys=null``:
    :ref:`select-columns-name-window-group-keys`

The following parameters are needed more description:

  * ``drilldowns[${LABEL}].keys``
  * ``drilldowns[${LABEL}].output_columns``
  * ``drilldowns[${LABEL}].columns[${NAME}].stage=null``

Output format is different a bit. It's also needed more description.

.. _select-drilldowns-label-keys:

``drilldowns[${LABEL}].keys``
"""""""""""""""""""""""""""""

.. versionadded:: 4.0.8

:ref:`select-drilldown` can specify multiple keys for multiple
drilldowns. But it can't specify multiple keys for one drilldown.

``drilldowns[${LABEL}].keys`` can't specify multiple keys for multiple
drilldowns. But it can specify multiple keys for one drilldown.

You can specify multiple keys separated by "``,``".

Here is an example to group by multiple keys, ``tag`` and ``n_likes``
column values:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldowns_label_keys_multiple.log
.. select Entries \
..   --limit -1 \
..   --output_columns tag,n_likes \
..   --drilldowns[tag.n_likes].keys tag,n_likes \
..   --drilldowns[tag.n_likes].output_columns _value.tag,_value.n_likes,_nsubrecs

``tag.n_likes`` is used as the label for the drilldown parameters
group. You can refer grouped keys by ``_value.${KEY_NAME}`` syntax in
:ref:`select-drilldowns-label-output-columns`. ``${KEY_NAME}`` is a
column name to be used by group key. ``tag`` and ``n_likes`` are
``${KEY_NAME}`` in this case.

Note that you can't use ``_value.${KEY_NAME}`` syntax when you just
specify one key as ``drilldowns[${LABEL}].keys`` like ``--drilldowns[tag].keys
tag``. You should use ``_key`` for the case. It's the same rule in
:ref:`select-drilldown-output-columns`.

.. _select-drilldowns-label-output-columns:

``drilldowns[${LABEL}].output_columns``
"""""""""""""""""""""""""""""""""""""""

.. versionadded:: 4.0.8

It's almost same as :ref:`select-drilldown-output-columns`. The
difference between :ref:`select-drilldown-output-columns` and
``drilldowns[${LABEL}].output_columns`` is how to refer group keys.

:ref:`select-drilldown-output-columns` uses ``_key``
:doc:`/reference/columns/pseudo` to refer group
key. ``drilldowns[${LABEL}].output_columns`` also uses ``_key``
:doc:`/reference/columns/pseudo` to refer group key when you specify
only one group key by :ref:`select-drilldowns-label-keys`.

Here is an example to refer single group key by ``_key``
:doc:`/reference/columns/pseudo`:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldowns_label_output_columns_single_group_key.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag.n_likes].keys tag \
..   --drilldowns[tag.n_likes].output_columns _key

But you can't refer each group key by ``_key``
:doc:`/reference/columns/pseudo` in
``drilldowns[${LABEL}].output_columns``. You need to use
``_value.${KEY_NAME}`` syntax. ``${KEY_NAME}`` is a column name that is
used for group key in :ref:`select-drilldowns-label-keys`.

Here is an example to refer each group key in multiple group keys by
``_value.${KEY_NAME}`` syntax:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldowns_label_output_columns_multiple_group_key.log
.. select Entries \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag.n_likes].keys tag,n_likes \
..   --drilldowns[tag.n_likes].output_columns _value.tag,_value.n_likes

.. tip:: Why ``_value.${KEY_NAME}`` syntax?

   It's implementation specific information.

   ``_key`` is a vector value. The vector value is consists of all
   group keys. You can see byte sequence of the vector value by
   referring ``_key`` in ``drilldowns[${LABEL}].output_columns``.

   There is one grouped record in ``_value`` to refer each grouped
   values when you specify multiple group keys to
   :ref:`select-drilldowns-label-keys`. So you can refer each group key
   by ``_value.${KEY_NAME}`` syntax.

   On the other hand, there is no grouped record in ``_value`` when
   you specify only one group key to
   :ref:`select-drilldowns-label-keys`. So you can't refer group key by
   ``_value.${KEY_NAME}`` syntax.

.. _select-drilldowns-label-columns-name-stage:

``drilldowns[${LABEL}].columns[${NAME}].stage``
"""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 6.0.5

Specifies when the dynamic column is created. This is a required
parameter to create a dynamic column.

Here are available stages:

.. list-table::
   :header-rows: 1

   * - Name
     - Description
   * - ``initial``
     - Dynamic column is created at first.
   * - ``filtered``
     - Dynamic column is created after ``drilldowns[${LABEL}].filter``
       is evaluated.
   * - ``output``
     - Dynamic column is created before :ref:`select-drilldowns-label-output-columns`
       is evaluated.

.. note::

   ``filtered`` stage and ``output`` stage will be able to use from 10.0.3 or later.

Here is one drilldown process flow with dynamic column creation
points. You should choose stage as late as possible:

  #. Evaluates :ref:`select-drilldowns-label-keys`,
     ``drilldowns[${LABEL}].calc_types`` and
     ``drilldowns[${LABEL}].calc_target``.

  #. Creates dynamic columns for ``initial`` stage. All drilldown
     result records have these dynamic columns.

  #. Evaluates ``drilldowns[${LABEL}].filter``. You can use dynamic
     columns created in ``initial`` stage.

  #. Creates dynamic columns for ``filtered`` stage.
     Only filtered records have these dynamic columns.

  #. Evaluates ``drilldowns[${LABEL}].sort_keys``,
     ``drilldowns[${LABEL}].offset`` and
     ``drilldowns[${LABEL}].limit``. You can use dynamic columns
     created in ``initial`` stage and ``filtered`` stage.

  #. Creates dynamic columns for ``output`` stage.
     Only ``drilldowns[${LABEL}].limit`` records have these dynamic columns.

  #. Evaluates :ref:`select-drilldowns-label-output-columns`. You can
     use dynamic columns created in ``initial`` stage, ``filtered`` stage,
     and ``output`` stage.

Here is a ``drilldowns[${LABEL}].columns[${NAME}].stage`` example. It
creates ``is_popular`` column at ``initial`` stage. You can use
``is_popular`` in all parameters such as
``drilldowns[${LABEL}].filter`` and
``drilldowns[${LABEL}].output_columns``:

.. groonga-command
.. include:: ../../example/reference/commands/select/drilldowns_label_columns_name_stage.log
.. select Entries \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].columns[is_popular].stage initial \
..   --drilldowns[tag].columns[is_popular].type Bool \
..   --drilldowns[tag].columns[is_popular].value '_nsubrecs > 1' \
..   --drilldowns[tag].filter is_popular \
..   --drilldowns[tag].output_columns _key,is_popular,_nsubrecs

.. versionadded:: 4.0.8

.. _select-drilldowns-label-output-format:

Output format for ``drilldowns[${LABEL}]`` style
""""""""""""""""""""""""""""""""""""""""""""""""

There is a difference in output format between :ref:`select-drilldown`
and :ref:`select-drilldowns-label-keys`. :ref:`select-drilldown` uses
array to output multiple drilldown results.
:ref:`select-drilldowns-label-keys` uses pairs of label and drilldown
result.

:ref:`select-drilldown` uses the following output format::

  [
    HEADER,
    [
      SEARCH_RESULT,
      DRILLDOWN_RESULT1,
      DRILLDOWN_RESULT2,
      ...
    ]
  ]

:ref:`select-drilldowns-label-keys` uses the following output format::

  [
    HEADER,
    [
      SEARCH_RESULT,
      {
        "LABEL1": DRILLDOWN_RESULT1,
        "LABEL2": DRILLDOWN_RESULT2,
        ...
      }
    ]
  ]

.. _select-slice-related-parameters:

Slice related parameters
^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 6.0.3

This section describes slice related parameters.

TODO

Here are parameters for slice:

.. list-table::
   :header-rows: 1

   * - Name
     - Required
   * - ``--slices[${LABEL}].match_columns``
     - Optional
   * - ``--slices[${LABEL}].query``
     - Required if ``--slices[${LABEL}].filter`` isn't specified.
   * - ``--slices[${LABEL}].filter``
     - Required if ``--slices[${LABEL}].query`` isn't specified.
   * - ``--slices[${LABEL}].query_expander``
     - Optional
   * - ``--slices[${LABEL}].query_flags``
     - Optional
   * - ``--slices[${LABEL}].sort_keys``
     - Optional
   * - ``--slices[${LABEL}].output_columns``
     - Optional
   * - ``--slices[${LABEL}].offset``
     - Optional
   * - ``--slices[${LABEL}].limit``
     - Optional
   * - ``--slices[${LABEL}].table``
     - Optional

.. _select-slices-label-match-columns:

``slices[${LABEL}].match_columns``
""""""""""""""""""""""""""""""""""

TODO

.. _select-slices-label-query:

``slices[${LABEL}].query``
""""""""""""""""""""""""""

TODO

.. _select-slices-label-filter:

``slices[${LABEL}].filter``
"""""""""""""""""""""""""""

TODO

.. _select-slices-label-query-expander:

``slices[${LABEL}].query_expander``
"""""""""""""""""""""""""""""""""""

TODO

.. _select-slices-label-query-flags:

``slices[${LABEL}].query_flags``
""""""""""""""""""""""""""""""""

TODO

.. _select-slices-label-sort-keys:

``slices[${LABEL}].sort_keys``
""""""""""""""""""""""""""""""

TODO

.. _select-slices-label-output-columns:

``slices[${LABEL}].output_columns``
"""""""""""""""""""""""""""""""""""

TODO

.. _select-slices-label-offset:

``slices[${LABEL}].offset``
"""""""""""""""""""""""""""

TODO

.. _select-slices-label-limit:

``slices[${LABEL}].limit``
""""""""""""""""""""""""""

TODO

.. _select-slices-label-table:

``slices[${LABEL}].table``
""""""""""""""""""""""""""

TODO

Cache related parameter
^^^^^^^^^^^^^^^^^^^^^^^

.. _select-cache:

``cache``
"""""""""

Specifies whether caching the result of this query or not.

If the result of this query is cached, the next same query returns
response quickly by using the cache.

It doesn't control whether existing cached result is used or not.

Here are available values:

.. list-table::
   :header-rows: 1

   * - Value
     - Description
   * - ``no``
     - Don't cache the output of this query.
   * - ``yes``
     - Cache the output of this query.
       It's the default value.

Here is an example to disable caching the result of this query:

.. groonga-command
.. include:: ../../example/reference/commands/select/cache_no.log
.. select Entries --cache no

The default value is ``yes``.

Score related parameters
^^^^^^^^^^^^^^^^^^^^^^^^

There is a score related parameter, ``adjuster``.

.. _select-adjuster:


``adjuster``
""""""""""""

Specifies one or more score adjust expressions. You need to use
``adjuster`` with ``query`` or ``filter``. ``adjuster`` doesn't work
with not searched request.

You can increase score of specific records by ``adjuster``. You can
use ``adjuster`` to set high score for important records.

For example, you can use ``adjuster`` to increase score of records
that have ``groonga`` tag.

Here is the syntax::

  --adjuster "SCORE_ADJUST_EXPRESSION1 + SCORE_ADJUST_EXPRESSION2 + ..."

Here is the ``SCORE_ADJUST_EXPRESSION`` syntax::

  COLUMN @ "KEYWORD" * FACTOR

Note the following:

  * ``COLUMN`` must be indexed.
  * ``"KEYWORD"`` must be a string.
  * ``FACTOR`` must be a positive integer.

Here is a sample ``adjuster`` usage example that uses just one
``SCORE_ADJUST_EXPRESSION``:

.. groonga-command
.. include:: ../../example/reference/commands/select/adjuster_one.log
.. select Entries \
..   --filter true \
..   --adjuster 'content @ "groonga" * 5' \
..   --output_columns _key,content,_score

The ``select`` command matches all records. Then it applies
``adjuster``. The adjuster increases score of records that have
``"groonga"`` in ``Entries.content`` column by 5. There is only one
record that has ``"groonga"`` in ``Entries.content`` column.  So the
record that its key is ``"Groonga"`` has score 6 (``= 1 + 5``).

You can omit ``FACTOR``. If you omit ``FACTOR``, it is treated as 1.

Here is a sample ``adjuster`` usage example that omits ``FACTOR``:

.. groonga-command
.. include:: ../../example/reference/commands/select/adjuster_no_factor.log
.. select Entries \
..   --filter true \
..   --adjuster 'content @ "groonga"' \
..   --output_columns _key,content,_score

The ``adjuster`` in the ``select`` command doesn't have ``FACTOR``. So
the factor is treated as 1. There is only one record that has
``"groonga"`` in ``Entries.content`` column. So the record that its
key is ``"Groonga"`` has score 2 (``= 1 + 1``).

Here is a sample ``adjuster`` usage example that uses multiple
``SCORE_ADJUST_EXPRESSION``:

.. groonga-command
.. include:: ../../example/reference/commands/select/adjuster_multiple.log
.. select Entries \
..   --filter true \
..   --adjuster 'content @ "groonga" * 5 + content @ "started" * 3' \
..   --output_columns _key,content,_score

The ``adjuster`` in the ``select`` command has two
``SCORE_ADJUST_EXPRESSION`` s. The final increased score is sum of
scores of these ``SCORE_ADJUST_EXPRESSION`` s. All
``SCORE_ADJUST_EXPRESSION`` s in the ``select`` command are applied to
a record that its key is ``"Groonga"``. So the final increased score
of the record is sum of scores of all ``SCORE_ADJUST_EXPRESSION`` s.

The first ``SCORE_ADJUST_EXPRESSION`` is ``content @ "groonga" * 5``.
It increases score by 5.

The second ``SCORE_ADJUST_EXPRESSION`` is ``content @ "started" * 3``.
It increases score by 3.

The final increased score is 9 (``= 1 + 5 + 3``).

A ``SCORE_ADJUST_EXPRESSION`` has a factor for ``"KEYWORD"``. This
means that increased scores of all records that has ``"KEYWORD"`` are
the same value. You can change increase score for each record that has
the same ``"KEYWORD"``. It is useful to tune search score. See
:ref:`weight-vector-column` for details.

.. _select-return-value:

Return value
------------

The command returns a response with the following format::

  [
    HEADER,
    [
      SEARCH_RESULT,
      DRILLDOWN_RESULT_1,
      DRILLDOWN_RESULT_2,
      ...,
      DRILLDOWN_RESULT_N
    ]
  ]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.

There are zero or more ``DRILLDOWN_RESULT``. If no ``drilldown`` and
``drilldowns[${LABEL}].keys`` are specified, they are omitted like the
following::

  [
    HEADER,
    [
      SEARCH_RESULT
    ]
  ]

If ``drilldown`` has two or more keys like ``--drilldown "_key,
column1, column2"``, multiple ``DRILLDOWN_RESULT`` exist::

  [
    HEADER,
    [
      SEARCH_RESULT,
      DRILLDOWN_RESULT_FOR_KEY,
      DRILLDOWN_RESULT_FOR_COLUMN1,
      DRILLDOWN_RESULT_FOR_COLUMN2
    ]
  ]

If ``drilldowns[${LABEL}].keys`` is used, only one ``DRILLDOWN_RESULT``
exist::

  [
    HEADER,
    [
      SEARCH_RESULT,
      DRILLDOWN_RESULT_FOR_LABELED_DRILLDOWN
    ]
  ]

``DRILLDOWN_RESULT`` format is different between ``drilldown`` and
``drilldowns[${LABEL}].keys``. It's described later.

``SEARCH_RESULT`` is the following format::

  [
    [N_HITS],
    COLUMNS,
    RECORDS
  ]

See :ref:`select-simple-usage` for concrete example of the format.

``N_HITS`` is the number of matched records before :ref:`select-limit`
is applied.

``COLUMNS`` describes about output columns specified by
:ref:`select-output-columns`. It uses the following format::

  [
    [COLUMN_NAME_1, COLUMN_TYPE_1],
    [COLUMN_NAME_2, COLUMN_TYPE_2],
    ...,
    [COLUMN_NAME_N, COLUMN_TYPE_N]
  ]

``COLUMNS`` includes one or more output column information. Each
output column information includes the followings:

  * Column name as string
  * Column type as string or ``null``

Column name is extracted from value specified as
:ref:`select-output-columns`.

Column type is Groonga's type name or ``null``. It doesn't describe
whether the column value is vector or scalar. You need to determine it
by whether real column value is array or not.

See :doc:`/reference/types` for type details.

``null`` is used when column value type isn't determined. For example,
function call in :ref:`select-output-columns` such as
``--output_columns "snippet_html(content)"`` uses ``null``.

Here is an example of ``COLUMNS``::

  [
    ["_id",     "UInt32"],
    ["_key",    "ShortText"],
    ["n_likes", "UInt32"],
  ]

``RECORDS`` includes column values for each matched record. Included
records are selected by :ref:`select-offset` and
:ref:`select-limit`. It uses the following format::

  [
    [
      RECORD_1_COLUMN_1,
      RECORD_1_COLUMN_2,
      ...,
      RECORD_1_COLUMN_N
    ],
    [
      RECORD_2_COLUMN_1,
      RECORD_2_COLUMN_2,
      ...,
      RECORD_2_COLUMN_N
    ],
    ...
    [
      RECORD_N_COLUMN_1,
      RECORD_N_COLUMN_2,
      ...,
      RECORD_N_COLUMN_N
    ]
  ]

Here is an example ``RECORDS``::

  [
    [
      1,
      "The first post!",
      5
    ],
    [
      2,
      "Groonga",
      10
    ],
    [
      3,
      "Mroonga",
      15
    ]
  ]

``DRILLDOWN_RESULT`` format is different between ``drilldown`` and
``drilldowns[${LABEL}].keys``.

``drilldown`` uses the same format as ``SEARCH_RESULT``::

  [
    [N_HITS],
    COLUMNS,
    RECORDS
  ]

And ``drilldown`` generates one or more ``DRILLDOWN_RESULT`` when
:ref:`select-drilldown` has one ore more keys.

``drilldowns[${LABEL}].keys`` uses the following format. Multiple
``drilldowns[${LABEL}].keys`` are mapped to one object (key-value
pairs)::

  {
    "LABEL_1": [
      [N_HITS],
      COLUMNS,
      RECORDS
    ],
    "LABEL_2": [
      [N_HITS],
      COLUMNS,
      RECORDS
    ],
    ...,
    "LABEL_N": [
      [N_HITS],
      COLUMNS,
      RECORDS
    ]
  }

Each ``drilldowns[${LABEL}].keys`` corresponds to the following::

  "LABEL": [
    [N_HITS],
    COLUMNS,
    RECORDS
  ]

The following value part is the same format as ``SEARCH_RESULT``::

  [
    [N_HITS],
    COLUMNS,
    RECORDS
  ]

See also :ref:`select-drilldowns-label-output-format` for
``drilldowns[${LABEL}]`` style drilldown output format.

See also
--------

  * :doc:`/reference/grn_expr/query_syntax`
  * :doc:`/reference/grn_expr/script_syntax`
