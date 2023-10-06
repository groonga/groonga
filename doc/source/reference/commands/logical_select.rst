.. -*- rst -*-

.. groonga-command
.. database: logical_select

``logical_select``
==================

Summary
-------

.. versionadded:: 5.0.5

``logical_select`` is a sharding version of
:doc:`select`. ``logical_select`` searches records from multiple
tables and outputs them.

You need to :doc:`plugin_register` ``sharding`` plugin because
this command is included in ``sharding`` plugin.

Syntax
------

This command takes many parameters.

The required parameters are ``logical_table`` and ``shard_key``. Other
parameters are optional::

  logical_select logical_table
                 shard_key
                 [min=null]
                 [min_border="include"]
                 [max=null]
                 [max_border="include"]
                 [filter=null]
                 [sortby=null]
                 [output_columns="_id, _key, *"]
                 [offset=0]
                 [limit=10]
                 [drilldown=null]
                 [drilldown_sortby=null]
                 [drilldown_output_columns="_key, _nsubrecs"]
                 [drilldown_offset=0]
                 [drilldown_limit=10]
                 [drilldown_calc_types=NONE]
                 [drilldown_calc_target=null]
                 [sort_keys=null]
                 [drilldown_sort_keys=null]
                 [match_columns=null]
                 [query=null]
                 [drilldown_filter=null]
                 [post_filter=null]
                 [load_table=null]
                 [load_columns=null]
                 [load_values=null]

There are some parameters that can be only used as named
parameters. You can't use these parameters as ordered parameters. You
must specify parameter name.

Here are parameters that can be only used as named parameters:

  * ``cache=no``

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

This command has the following named parameters for advanced
drilldown:

  * ``drilldowns[${LABEL}].keys=null``
  * ``drilldowns[${LABEL}].sort_keys=null``
  * ``drilldowns[${LABEL}].output_columns="_key, _nsubrecs"``
  * ``drilldowns[${LABEL}].offset=0``
  * ``drilldowns[${LABEL}].limit=10``
  * ``drilldowns[${LABEL}].calc_types=NONE``
  * ``drilldowns[${LABEL}].calc_target=null``
  * ``drilldowns[${LABEL}].filter=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].stage=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].flags=COLUMN_SCALAR``
  * ``drilldowns[${LABEL}].columns[${NAME}].type=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].value=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].window.sort_keys=null``
  * ``drilldowns[${LABEL}].columns[${NAME}].window.group_keys=null``

.. deprecated:: 6.1.4

   ``drilldown[...]`` syntax is deprecated. Use ``drilldowns[...]``
   instead.

.. deprecated:: 6.1.5

   :ref:`logical-select-drilldowns-label-sortby` is deprecated.
   Use :ref:`logical-select-drilldowns-label-sort-keys` instead.

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

Differences from ``select``
---------------------------

Most of ``logical_select`` features can be used like corresponding
:doc:`select` features. For example, parameter name is same, output
format is same and so on.

But there are some differences from :doc:`select`:

  * ``logical_table`` and ``shard_key`` parameters are required
    instead of ``table`` parameter.
  * ``sort_keys`` isn't supported when multiple shards are used. (Only
    one shard is used, they are supported. There is one exception
    about ``sort_keys`` for multiple shards. When ``shard_keys`` and
    ``sort_keys`` are same, they are supported. See
    :ref:`logical-select-sort-keys` about details)
  * ``_value.${KEY_NAME}`` in ``drilldowns[${LABEL}].sort_keys``
    doesn't work with multiple shards. It works with one
    shard. ``_key`` in ``drilldowns[${LABEL}].sort_keys`` work with
    multiple shards.
  * ``_value.${KEY_NAME}`` in ``drilldowns[${LABEL}].output_columns``
    also doesn't work with multiple shards either. It works with one
    shard.
  * ``match_escalation_threshold`` isn't supported yet.
  * ``query_flags`` isn't supported yet.
  * ``query_expander`` isn't supported yet.
  * ``adjuster`` isn't supported yet.

Usage
-----

Let's learn about usage with examples. This section shows many popular
usages.

You need to register ``sharding`` plugin because this command is
included in ``sharding`` plugin.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/usage_plugin_register.log
.. plugin_register sharding


Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/usage_setup.log
.. table_create Entries_20150708 TABLE_HASH_KEY ShortText
.. column_create Entries_20150708 created_at COLUMN_SCALAR Time
.. column_create Entries_20150708 content COLUMN_SCALAR Text
.. column_create Entries_20150708 n_likes COLUMN_SCALAR UInt32
.. column_create Entries_20150708 tag COLUMN_SCALAR ShortText
..
.. table_create Entries_20150709 TABLE_HASH_KEY ShortText
.. column_create Entries_20150709 created_at COLUMN_SCALAR Time
.. column_create Entries_20150709 content COLUMN_SCALAR Text
.. column_create Entries_20150709 n_likes COLUMN_SCALAR UInt32
.. column_create Entries_20150709 tag COLUMN_SCALAR ShortText
..
.. table_create Terms TABLE_PAT_KEY ShortText \
..   --default_tokenizer TokenBigram \
..   --normalizer NormalizerAuto
.. column_create Terms entries_key_index_20150708 \
..   COLUMN_INDEX|WITH_POSITION Entries_20150708 _key
.. column_create Terms entries_content_index_20150708 \
..   COLUMN_INDEX|WITH_POSITION Entries_20150708 content
.. column_create Terms entries_key_index_20150709 \
..   COLUMN_INDEX|WITH_POSITION Entries_20150709 _key
.. column_create Terms entries_content_index_20150709 \
..   COLUMN_INDEX|WITH_POSITION Entries_20150709 content
..
.. load --table Entries_20150708
.. [
.. {"_key":       "The first post!",
..  "created_at": "2015/07/08 00:00:00",
..  "content":    "Welcome! This is my first post!",
..  "n_likes":    5,
..  "tag":        "Hello"},
.. {"_key":       "Groonga",
..  "created_at": "2015/07/08 01:00:00",
..  "content":    "I started to use Groonga. It's very fast!",
..  "n_likes":    10,
..  "tag":        "Groonga"},
.. {"_key":       "Mroonga",
..  "created_at": "2015/07/08 02:00:00",
..  "content":    "I also started to use Mroonga. It's also very fast! Really fast!",
..  "n_likes":    15,
..  "tag":        "Groonga"}
.. ]
..
.. load --table Entries_20150709
.. [
.. {"_key":       "Good-bye Senna",
..  "created_at": "2015/07/09 00:00:00",
..  "content":    "I migrated all Senna system!",
..  "n_likes":    3,
..  "tag":        "Senna"},
.. {"_key":       "Good-bye Tritonn",
..  "created_at": "2015/07/09 01:00:00",
..  "content":    "I also migrated all Tritonn system!",
..  "n_likes":    3,
..  "tag":        "Senna"}
.. ]

There are two tables, ``Entries_20150708`` and ``Entries_20150709``,
for blog entries.

.. note::

   You need to use ``${LOGICAL_TABLE_NAME}_${YYYYMMDD}`` naming rule
   for table names. In this example, ``LOGICAL_TABLE_NAME`` is
   ``Entries`` and ``YYYYMMDD`` is ``20150708`` or ``20150709``.

An entry has title, created time, content, the number of likes for the
entry and tag. Title is key of ``Entries_YYYYMMDD``. Created time is
value of ``Entries_YYYYMMDD.created_at`` column. Content is value of
``Entries_YYYYMMDD.content`` column. The number of likes is value of
``Entries_YYYYMMDD.n_likes`` column. Tag is value of
``Entries_YYYYMMDD.tag`` column.

``Entries_YYYYMMDD._key`` column and ``Entries_YYYYMMDD.content``
column are indexed using ``TokenBigram`` tokenizer. So both
``Entries_YYYYMMDD._key`` and ``Entries_YYYYMMDD.content`` are
fulltext search ready.

OK. The schema and data for examples are ready.

Simple usage
^^^^^^^^^^^^

Here is an example that specifies only required parameters.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/logical_table_existent.log
.. logical_select --logical_table Entries --shard_key created_at

It is shown that is searched from Entries_20150708 and Entries_20150709 in above example.

Parameters
----------

This section describes parameters of ``logical_select``.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are required parameters, ``logical_table`` and ``shard_key``.

.. _logical-select-logical-table:

``logical_table``
"""""""""""""""""

Specifies logical table name. It means table name without
``_YYYYMMDD`` postfix. If you use actual table such as
``Entries_20150708``, ``Entries_20150709`` and so on, logical table
name is ``Entries``.

You can show 5 records by specifying ``logical_table`` and
``shard_key`` parameters. They are required parameters.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/logical_table_existent.log
.. logical_select --logical_table Entries --shard_key created_at

If nonexistent table is specified, an error is returned.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/logical_table_nonexistent.log
.. logical_select --logical_table Nonexistent --shard_key created_at

.. _logical-select-shard-key:

``shard_key``
"""""""""""""

Specifies column name which is treated as shared key. Shard key is a
column that stores data that is used for distributing records to
suitable shards.

Shard key must be ``Time`` type for now.

See :ref:`logical-select-logical-table` how to specify ``shard_key``.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

.. _logical-select-min:

``min``
"""""""

Specifies the minimum value of ``shard_key`` column. If shard doesn't
have any matched records, the shard isn't searched.

For example, ``min`` is ``"2015/07/09 00:00:00"``, ``Entry_20150708``
isn't searched. Because ``Entry_20150708`` has only records for
``"2015/07/08"``.

The following example only uses ``Entry_20150709``
table. ``Entry_20150708`` isn't used.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/min.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --min "2015/07/09 00:00:00"

.. _logical-select-min-border:

``min_border``
""""""""""""""

Specifies whether the minimum value is included or not. Here is
available values.

.. list-table::
   :header-rows: 1

   * - Value
     - Description
   * - ``include``
     - Includes ``min`` value. This is the default.
   * - ``exclude``
     - Doesn't include ``min`` value.

Here is an example for ``exclude``. The result doesn't include the
``"Good-bye Senna"`` record because its ``created_at`` value is
``"2015/07/09 00:00:00"``.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/min_border.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --min "2015/07/09 00:00:00" \
..   --min_border "exclude"

.. _logical-select-max:

``max``
"""""""

Specifies the maximum value of ``shard_key`` column. If shard doesn't
have any matched records, the shard isn't searched.

For example, ``max`` is ``"2015/07/08 23:59:59"``, ``Entry_20150709``
isn't searched. Because ``Entry_20150709`` has only records for
``""2015/07/09"``.

The following example only uses ``Entry_20150708``
table. ``Entry_20150709`` isn't used.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/max.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --max "2015/07/08 23:59:59"

.. _logical-select-max-border:

``max_border``
""""""""""""""

Specifies whether the maximum value is included or not. Here is
available values.

.. list-table::
   :header-rows: 1

   * - Value
     - Description
   * - ``include``
     - Includes ``max`` value. This is the default.
   * - ``exclude``
     - Doesn't include ``max`` value.

Here is an example for ``exclude``. The result doesn't include the
``"Good-bye Senna"`` record because its ``created_at`` value is
``"2015/07/09 00:00:00"``.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/max_border.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --max "2015/07/09 00:00:00" \
..   --max_border "exclude"

.. _logical-select-search-related-parameters:

Search related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^

This command provides :doc:`select` compatible search related
parameters.

.. _logical-select-match-columns:

``match_columns``
"""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-match-columns` in :doc:`select`. See
:ref:`select-match-columns` for details.

Here is an example to find records that include ``"fast"`` text in
``content`` column:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/match_columns.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --match_columns content \
..   --query "fast"

.. _logical-select-query:

``query``
"""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-query` in :doc:`select`. See
:ref:`select-query` for details.

Here is an example to find records that include ``"fast"`` text in
``content`` column:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/query.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --query "content:@fast"

See also :ref:`logical-select-match-columns`.

.. _logical-select-filter:

``filter``
""""""""""

Corresponds to :ref:`select-filter` in :doc:`select`. See
:ref:`select-filter` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/filter.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --filter "n_likes <= 5"

.. _logical-select-post-filter:

``post_filter``
"""""""""""""""

.. versionadded:: 8.0.1

Specifies the filter text that is processed after ``filtered`` stage
dynamic columns are generated. You can use ``post_filter`` to filter
by ``filtered`` stage dynamic columns. Others are the same as
:ref:`logical-select-filter`.

Here is an example that shows entries only in popular tag. All target
entries have ``system`` or ``use`` words:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/post_filter.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_sum_per_tag].stage filtered \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys 'tag' \
..   --filter 'content @ "system" || content @ "use"' \
..   --post_filter 'n_likes_sum_per_tag > 10' \
..   --output_columns _key,n_likes,n_likes_sum_per_tag

.. _logical-select-load-table:

``load_table``
""""""""""""""

.. versionadded:: 8.1.1

You can store specified a table a result of ``logical_select`` with ``--load_table``, ``--load-columns`` and ``--load_values`` arguments.
``--load_table`` specifies a table name for storing a result of ``logical_select``.

You must specify a table that already exists.

This argument must use with :ref:`logical-select-load-columns` and :ref:`logical-select-load-values`.

Here is an example that can store ``_id`` and ``timestamp`` that a result of ``logical_select`` in a Logs table specified by ``--load_table``.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/load_table.log
.. table_create Logs_20150203 TABLE_HASH_KEY ShortText
.. column_create Logs_20150203 timestamp COLUMN_SCALAR Time
.. table_create Logs_20150204 TABLE_HASH_KEY ShortText
.. column_create Logs_20150204 timestamp COLUMN_SCALAR Time
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
.. load --table Logs_20150204
.. [
.. {
..   "_key": "2015-02-04:1",
..   "timestamp": "2015-02-04 00:00:00"
.. }
.. ]
.. logical_select \
..   --logical_table Logs \
..   --shard_key timestamp \
..   --load_table Logs \
..   --load_columns "_key, original_id, timestamp_text" \
..   --load_values "_key, _id, timestamp"
.. select --table Logs

.. _logical-select-load-columns:

``load_columns``
""""""""""""""""

.. versionadded:: 8.1.1

Specifies columns of a table that specifying ``--load-table``.
Stores value of columns that specified with :ref:`logical-select-load-values` in columns that specified with this argument.
You must specify columns that already exists.

This argument must use with :ref:`logical-select-load-table` and :ref:`logical-select-load-values`.

See example of ``--load_table`` for how to use this argument.

.. _logical-select-load-values:

``load_values``
"""""""""""""""

.. versionadded:: 8.1.1

Specifies columns of result of ``logical_select``.
Specifies columns for storing values into columns that specified with :ref:`logical-select-load-columns`.
You must specify columns that already exists.

This argument must use with :ref:`logical-select-load-table` and :ref:`logical-select-load-columns`.

See example of ``--load_table`` for how to use this argument.

.. _logical-select-advanced-search-parameters:

Advanced search parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^

``logical_select`` doesn't implement advanced search parameters yet.

.. _logical-select-match-escalation-threshold:

``match_escalation_threshold``
""""""""""""""""""""""""""""""

Not implemented yet.

.. _logical-select-query-flags:

``query_flags``
"""""""""""""""

Not implemented yet.

.. _logical-select-query-expander:

``query_expander``
""""""""""""""""""

Not implemented yet.

Output related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^

.. _logical-select-output-columns:

``output_columns``
""""""""""""""""""

Corresponds to :ref:`select-output-columns` in :doc:`select`. See
:ref:`select-output-columns` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/output_columns.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --output_columns '_key, *'

.. _logical-select-sortby:

``sortby``
""""""""""

.. deprecated:: 6.1.5

   Use :ref:`logical-select-sort-keys` instead.

.. _logical-select-sort-keys:

``sort_keys``
"""""""""""""

Corresponds to :ref:`select-sort-keys` in :doc:`select`. See
:ref:`select-sort-keys` for details.

``sort_keys`` has a limitation. It works only when the number of
search target shards is one. If the number of search target shards is
larger than one, ``sort_keys`` doesn't work.

.. note::

   There is one exception for multiple shards. When the same value is
   specified for ``shard_key`` and ``sort_keys``, they are supported.
   This command processes target shards one by one by ascending
   order. Thus, in this process, magnitude correlation about the value
   of ``shard_key`` is kept among them. This is because ``sort_keys``
   is supported when ``shard_key`` and ``sort_keys`` is same.

Here is an example that uses only one shard:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/sort_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --min "2015/07/08 00:00:00" \
..   --min_border "include" \
..   --max "2015/07/09 00:00:00" \
..   --max_border "exclude" \
..   --sort_keys _key

.. _logical-select-offset:

``offset``
""""""""""

Corresponds to :ref:`select-offset` in :doc:`select`. See
:ref:`select-offset` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/offset.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --offset 2

.. _logical-select-limit:

``limit``
"""""""""

Corresponds to :ref:`select-limit` in :doc:`select`. See
:ref:`select-limit` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/limit.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 2

.. _logical-select-scorer:

``scorer``
""""""""""

Not implemented yet.

.. _logical-select-dynamic-column-related-parameters:

Dynamic column related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 7.0.1

All dynamic column related parameters in :doc:`select` are
supported. See :ref:`select-dynamic-column-related-parameters` for
details.

.. _logical-select-columns-name-stage:

``columns[${NAME}].stage``
""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-columns-name-stage` in :doc:`select`. See
:ref:`select-columns-name-stage` for details.

This is a required parameter.

Here is an example that creates ``is_popular`` column at ``initial``
stage. You can use ``is_popular`` in all parameters such as ``filter``
and ``output_columns``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/columns_name_stage.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[is_popular].stage initial \
..   --columns[is_popular].type Bool \
..   --columns[is_popular].value 'n_likes >= 10' \
..   --filter is_popular \
..   --output_columns _id,is_popular,n_likes

.. _logical-select-columns-name-flags:

``columns[${NAME}].flags``
""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-columns-name-flags` in :doc:`select`. See
:ref:`select-columns-name-flags` for details.

The default value is ``COLUMN_SCALAR``.

Here is an example that creates a vector column by ``COLUMN_VECTOR``
flags. ``plugin_register functions/vector`` is for using
:doc:`/reference/functions/vector_new` function:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/columns_name_flags.log
.. plugin_register functions/vector
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[vector].stage initial \
..   --columns[vector].flags COLUMN_VECTOR \
..   --columns[vector].type UInt32 \
..   --columns[vector].value 'vector_new(1, 2, 3)' \
..   --output_columns _id,vector

.. _logical-select-columns-name-type:

``columns[${NAME}].type``
"""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-columns-name-type` in :doc:`select`. See
:ref:`select-columns-name-type` for details.

This is a required parameter.

Here is an example that creates a ``ShortText`` type column. Stored
value is casted to ``ShortText`` automatically. In this example,
number is casted to ``ShortText``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/columns_name_type.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_string].stage initial \
..   --columns[n_likes_string].type ShortText \
..   --columns[n_likes_string].value n_likes \
..   --output_columns _id,n_likes,n_likes_string

.. _logical-select-columns-name-value:

``columns[${NAME}].value``
""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-columns-name-value` in :doc:`select`. See
:ref:`select-columns-name-value` for details.

You need to specify :doc:`/reference/window_function` as ``value``
value and other window function related parameters when you use window
function. See :ref:`logical-select-window-function-related-parameters`
for details.

This is a required parameter.

Here is an example that creates a new dynamic column that stores the
number of characters of content. This example uses
:doc:`/reference/functions/string_length` function in
``functions/string`` plugin to compute the number of characters in a
string. :doc:`plugin_register` is used to register
``functions/string`` plugin:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/columns_name_value.log
.. plugin_register functions/string
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[content_length].stage initial \
..   --columns[content_length].type UInt32 \
..   --columns[content_length].value 'string_length(content)' \
..   --output_columns _id,content,content_length

.. _logical-select-window-function-related-parameters:

Window function related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 7.0.1

All window function related parameters in :doc:`select` are
supported. See :ref:`select-window-function-related-parameters` for
details.

.. note::

   Window function over multiple tables aren't supported version 9.0.1 or before.
   Groonga supported it since version 9.0.2 or later.
   However, we need to align the same order for shard key and leading group key or sort key.

   For example, we can apply the window function to over multiple tables as below case.
   Because the below example aligns the same order for shard key and leading group key.

   The leading group key is ``price`` and shard key is ``timestamp`` in the below example.
   We can apply the window function to over multiple tables in the below example.
   Because ``price`` and ``timestamp`` aligns the same order.

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/window_function_for_over_shard.log
.. plugin_register sharding
.. 
.. table_create ItemLogs_20170415 TABLE_NO_KEY
.. column_create ItemLogs_20170415 timestamp COLUMN_SCALAR Time
.. column_create ItemLogs_20170415 price COLUMN_SCALAR UInt32
.. column_create ItemLogs_20170415 n_likes COLUMN_SCALAR UInt32
.. 
.. table_create ItemLogs_20170416 TABLE_NO_KEY
.. column_create ItemLogs_20170416 timestamp COLUMN_SCALAR Time
.. column_create ItemLogs_20170416 price COLUMN_SCALAR UInt32
.. column_create ItemLogs_20170416 n_likes COLUMN_SCALAR UInt32
.. 
.. load --table ItemLogs_20170415
.. [
.. {"timestamp": "2017/04/15 00:00:00", "n_likes": 2, "price": 100},
.. {"timestamp": "2017/04/15 01:00:00", "n_likes": 1, "price": 100},
.. {"timestamp": "2017/04/15 01:00:00", "n_likes": 2, "price": 200}
.. ]
.. 
.. load --table ItemLogs_20170416
.. [
.. {"timestamp": "2017/04/16 10:00:00", "n_likes": 1, "price": 200},
.. {"timestamp": "2017/04/16 11:00:00", "n_likes": 2, "price": 300},
.. {"timestamp": "2017/04/16 11:00:00", "n_likes": 1, "price": 300}
.. ]
.. 
.. logical_select ItemLogs \
..   --shard_key timestamp \
..   --columns[count].stage initial \
..   --columns[count].type UInt32 \
..   --columns[count].flags COLUMN_SCALAR \
..   --columns[count].value 'window_count()' \
..   --columns[count].window.group_keys price \
..   --output_columns price,count

.. _logical-select-columns-name-window-sort-keys:

``columns[${NAME}].window.sort_keys``
"""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-columns-name-window-sort-keys` in
:doc:`select`. See :ref:`select-columns-name-window-sort-keys` for
details.

You must specify :ref:`logical-select-columns-name-window-sort-keys`
or :ref:`logical-select-columns-name-window-group-keys` to use window
function.

Here is an example that computes cumulative sum per
``Entries.tag``. Each group is sorted by ``Entries._key``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/columns_name_window_sort_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_cumulative_sum_per_tag].stage initial \
..   --columns[n_likes_cumulative_sum_per_tag].type UInt32 \
..   --columns[n_likes_cumulative_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_cumulative_sum_per_tag].window.sort_keys _key \
..   --columns[n_likes_cumulative_sum_per_tag].window.group_keys tag \
..   --sort_keys _key \
..   --output_columns tag,_key,n_likes,n_likes_cumulative_sum_per_tag

.. _logical-select-columns-name-window-group-keys:

``columns[${NAME}].window.group_keys``
""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-columns-name-window-group-keys` in
:doc:`select`. See :ref:`select-columns-name-window-group-keys` for
details.

You must specify :ref:`logical-select-columns-name-window-sort-keys`
or :ref:`logical-select-columns-name-window-group-keys` to use window
function.

Here is an example that computes sum per ``Entries.tag``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/columns_name_window_group_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_sum_per_tag].stage initial \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys tag \
..   --sort_keys _key \
..   --output_columns tag,_key,n_likes,n_likes_sum_per_tag

.. _logical-select-drilldown-related-parameters:

Drilldown related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All drilldown related parameters in :doc:`select` are supported. See
:ref:`select-drilldown-related-parameters` for details.

.. _logical-select-drilldown:

``drilldown``
"""""""""""""

Corresponds to :ref:`select-drilldown` in :doc:`select`. See
:ref:`select-drilldown` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldown.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --output_columns _key,tag \
..   --drilldown tag

.. _logical-select-drilldown-sortby:

``drilldown_sortby``
""""""""""""""""""""

.. deprecated:: 6.1.5

   Use :ref:`logical-select-drilldown-sort-keys` instead.

.. _logical-select-drilldown-sort-keys:

``drilldown_sort_keys``
"""""""""""""""""""""""

Corresponds to :ref:`select-drilldown-sort-keys` in :doc:`select`. See
:ref:`select-drilldown-sort-keys` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldown_sort_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_sort_keys -_nsubrecs,_key

.. _logical-select-drilldown-output-columns:

``drilldown_output_columns``
""""""""""""""""""""""""""""

Corresponds to :ref:`select-drilldown-output-columns` in
:doc:`select`. See :ref:`select-drilldown-output-columns` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldown_output_columns.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_output_columns _key

.. _logical-select-drilldown-offset:

``drilldown_offset``
""""""""""""""""""""

Corresponds to :ref:`select-drilldown-offset` in :doc:`select`. See
:ref:`select-drilldown-offset` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldown_offset.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_offset 1

.. _logical-select-drilldown-limit:

``drilldown_limit``
"""""""""""""""""""

Corresponds to :ref:`select-drilldown-limit` in :doc:`select`. See
:ref:`select-drilldown-limit` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldown_limit.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_limit 2

.. _logical-select-drilldown-calc-types:

``drilldown_calc_types``
""""""""""""""""""""""""

Corresponds to :ref:`select-drilldown-calc-types` in
:doc:`select`. See :ref:`select-drilldown-calc-types` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldown_calc_types.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit -1 \
..   --output_columns tag,n_likes \
..   --drilldown tag \
..   --drilldown_calc_types MAX,MIN,SUM,AVG \
..   --drilldown_calc_target n_likes \
..   --drilldown_output_columns _key,_nsubrecs,_max,_min,_sum,_avg

.. _logical-select-drilldown-calc-target:

``drilldown_calc_target``
"""""""""""""""""""""""""

Corresponds to :ref:`select-drilldown-calc-target` in
:doc:`select`. See :ref:`select-drilldown-calc-target` for details.

See also :ref:`logical-select-drilldown-calc-types` for an example.

.. _logical-select-drilldown-filter:

``drilldown_filter``
""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-drilldown-filter` in
:doc:`select`. See :ref:`select-drilldown-filter` for details.

Here is an example to suppress drilled down tags that are occurred
only once:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldown_filter.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldown tag \
..   --drilldown_filter "_nsubrecs > 1" \
..   --drilldown_output_columns _key,_nsubrecs

.. _logical-select-advanced-drilldown-related-parameters:

Advanced drilldown related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All advanced drilldown related parameters in :doc:`select` are
supported. See :ref:`select-advanced-drilldown-related-parameters` for
details.

There are some limitations:

  * ``_value.${KEY_NAME}`` in ``drilldowns[${LABEL}].sort_keys``
    doesn't work with multiple shards. It works with one
    shard. ``_key`` in ``drilldowns[${LABEL}].sort_key`` work with
    multiple shards.

.. _logical-select-drilldowns-label-keys:

``drilldowns[${LABEL}].keys``
"""""""""""""""""""""""""""""

Corresponds to :ref:`select-drilldowns-label-keys` in
:doc:`select`. See :ref:`select-drilldowns-label-keys` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag.n_likes].keys tag,n_likes \
..   --drilldowns[tag.n_likes].output_columns _value.tag,_value.n_likes,_nsubrecs

.. _logical-select-drilldowns-label-output-columns:

``drilldowns[${LABEL}].output_columns``
"""""""""""""""""""""""""""""""""""""""

Corresponds to :ref:`select-drilldowns-label-output-columns` in
:doc:`select`. See :ref:`select-drilldowns-label-output-columns` for
details.

``drilldowns[${LABEL}].output_columns`` has a limitation.

``_value.${KEY_NAME}`` in ``drilldowns[${LABEL}].output_columns`` doesn't
work with multiple shards. It works with one shard.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_output_columns.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].output_columns _key,_nsubrecs

.. _logical-select-drilldowns-label-sortby:

``drilldowns[${LABEL}].sortby``
"""""""""""""""""""""""""""""""

.. deprecated:: 6.1.5

   Use :ref:`logical-select-drilldowns-label-sort-keys` instead.

.. _logical-select-drilldowns-label-sort-keys:

``drilldowns[${LABEL}].sort_keys``
""""""""""""""""""""""""""""""""""

Corresponds to :ref:`logical-select-drilldown-sort-keys` in not
labeled drilldown.

``drilldowns[${LABEL}].sort_keys`` has a limitation.

``_value.${KEY_NAME}`` in ``drilldowns[${LABEL}].sort_keys`` doesn't
work with multiple shards. It works with one shard. ``_key`` in
``drilldowns[${LABEL}].sort_keys`` work with multiple shards.

Here is an example that uses ``_value.${KEY_NAME}`` with only one
shard:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_sort_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --min "2015/07/08 00:00:00" \
..   --min_border "include" \
..   --max "2015/07/09 00:00:00" \
..   --max_border "exclude" \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag.n_likes].keys tag,n_likes \
..   --drilldowns[tag.n_likes].output_columns _nsubrecs,_value.n_likes,_value.tag \
..   --drilldowns[tag.n_likes].sort_keys -_nsubrecs,_value.n_likes,_value.tag

.. _logical-select-drilldowns-label-offset:

``drilldowns[${LABEL}].offset``
"""""""""""""""""""""""""""""""

Corresponds to :ref:`logical-select-drilldown-offset` in not labeled
drilldown.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_offset.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag.n_likes].keys tag \
..   --drilldowns[tag.n_likes].offset 1

.. _logical-select-drilldowns-label-limit:

``drilldowns[${LABEL}].limit``
""""""""""""""""""""""""""""""

Corresponds to :ref:`logical-select-drilldown-limit` in not labeled
drilldown.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_limit.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag.n_likes].keys tag \
..   --drilldowns[tag.n_likes].limit 2

.. _logical-select-drilldowns-label-calc-types:

``drilldowns[${LABEL}].calc_types``
"""""""""""""""""""""""""""""""""""

Corresponds to :ref:`logical-select-drilldown-calc-types` in not
labeled drilldown.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_calc_types.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].calc_types MAX,MIN,SUM,AVG \
..   --drilldowns[tag].calc_target n_likes \
..   --drilldowns[tag].output_columns _key,_nsubrecs,_max,_min,_sum,_avg

.. _logical-select-drilldowns-label-calc-target:

``drilldowns[${LABEL}].calc_target``
""""""""""""""""""""""""""""""""""""

Corresponds to :ref:`logical-select-drilldown-calc-target` in not
labeled drilldown.

See also :ref:`logical-select-drilldowns-label-calc-types`
for an example.

.. _logical-select-drilldowns-label-filter:

``drilldowns[${LABEL}].filter``
"""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`logical-select-drilldown-filter` in not labeled
drilldown.

Here is an example to suppress drilled down tags that are occurred
only once:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_filter.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].filter "_nsubrecs > 1" \
..   --drilldowns[tag].output_columns _key,_nsubrecs

.. _logical-select-drilldowns-label-columns-name-stage:

``drilldowns[${LABEL}].columns[${NAME}].stage``
"""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`select-drilldowns-label-columns-name-stage` in
:doc:`select`. See :ref:`select-drilldowns-label-columns-name-stage`
for details.

Here is an example that creates a dynamic column at ``initial``
stage. This example creates a dynamic column that stores whether each
drilled down tag is occurred only once or not:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_columns_name_stage.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].columns[is_popular].stage initial \
..   --drilldowns[tag].columns[is_popular].type Bool \
..   --drilldowns[tag].columns[is_popular].value '_nsubrecs > 1' \
..   --drilldowns[tag].output_columns _key,_nsubrecs,is_popular

.. _logical-select-drilldowns-label-columns-name-flags:

``drilldowns[${LABEL}].columns[${NAME}].flags``
"""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`logical-select-columns-name-flags` in main
search.

The default value is ``COLUMN_SCALAR``.

Here is an example that creates a vector column by ``COLUMN_VECTOR``
flags. ``plugin_register functions/vector`` is for using
:doc:`/reference/functions/vector_new` function:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_columns_name_flags.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].columns[vector].stage initial \
..   --drilldowns[tag].columns[vector].flags COLUMN_VECTOR \
..   --drilldowns[tag].columns[vector].type ShortText \
..   --drilldowns[tag].columns[vector].value 'vector_new("a", "b", "c")' \
..   --drilldowns[tag].output_columns _key,vector

.. _logical-select-drilldowns-label-columns-name-type:

``drilldowns[${LABEL}].columns[${NAME}].type``
""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`logical-select-columns-name-type` in main
search.

This is a required parameter.

Here is an example that creates a ``ShortText`` type column. Stored
value is casted to ``ShortText`` automatically. In this example,
number is casted to ``ShortText``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_columns_name_type.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].columns[nsubrecs_string].stage initial \
..   --drilldowns[tag].columns[nsubrecs_string].type ShortText \
..   --drilldowns[tag].columns[nsubrecs_string].value _nsubrecs \
..   --drilldowns[tag].output_columns _key,_nsubrecs,nsubrecs_string

.. _logical-select-drilldowns-label-columns-name-value:

``drilldowns[${LABEL}].columns[${NAME}].value``
"""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`logical-select-columns-name-value` in main
search.

This is a required parameter.

Here is an example that creates a new dynamic column that stores the
number of characters of content. This example uses
:doc:`/reference/functions/string_length` function in
``functions/string`` plugin to compute the number of characters in a
string. :doc:`plugin_register` is used to register
``functions/string`` plugin:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_columns_name_value.log
.. plugin_register functions/string \
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].columns[tag_length].stage initial \
..   --drilldowns[tag].columns[tag_length].type UInt32 \
..   --drilldowns[tag].columns[tag_length].value 'string_length(_key)' \
..   --drilldowns[tag].output_columns _key,tag_length

.. _logical-select-drilldowns-label-columns-name-window-sort-keys:

``drilldowns[${LABEL}].columns[${NAME}].window.sort_keys``
""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`logical-select-columns-name-window-sort-keys` in
main search.

You must specify
:ref:`logical-select-drilldowns-label-columns-name-window-sort-keys`
or
:ref:`logical-select-drilldowns-label-columns-name-window-group-keys`
to use window function.

Here is an example that computes the Nth record in the number of sub
records order:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_columns_name_window_sort_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].columns[record_number].stage initial \
..   --drilldowns[tag].columns[record_number].type UInt32 \
..   --drilldowns[tag].columns[record_number].value 'window_record_number()' \
..   --drilldowns[tag].columns[record_number].window.sort_keys _nsubrecs,_key \
..   --drilldowns[tag].sort_keys record_number \
..   --drilldowns[tag].output_columns _key,_nsubrecs,record_number

.. _logical-select-drilldowns-label-columns-name-window-group-keys:

``drilldowns[${LABEL}].columns[${NAME}].window.group_keys``
"""""""""""""""""""""""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.1

Corresponds to :ref:`logical-select-columns-name-window-group-keys` in
main search.

You must specify
:ref:`logical-select-drilldowns-label-columns-name-window-sort-keys`
or
:ref:`logical-select-drilldowns-label-columns-name-window-group-keys`
to use window function.

Here is an example that computes the Nth record ordered by tag name
for each the same number of sub records:

.. groonga-command
.. include:: ../../example/reference/commands/logical_select/drilldowns_label_columns_name_window_group_keys.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 0 \
..   --output_columns _id \
..   --drilldowns[tag].keys tag \
..   --drilldowns[tag].columns[record_number].stage initial \
..   --drilldowns[tag].columns[record_number].type UInt32 \
..   --drilldowns[tag].columns[record_number].value 'window_record_number()' \
..   --drilldowns[tag].columns[record_number].window.sort_keys _key \
..   --drilldowns[tag].columns[record_number].window.group_keys _nsubrecs \
..   --drilldowns[tag].sort_keys _nsubrecs,record_number \
..   --drilldowns[tag].output_columns _key,_nsubrecs,record_number

Cache related parameter
^^^^^^^^^^^^^^^^^^^^^^^

.. _logical-select-cache:

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
.. include:: ../../example/reference/commands/logical_select/cache_no.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --cache no

The default value is ``yes``.

Return value
------------

The return value format of ``logical_select`` is compatible with
:doc:`select`. See :ref:`select-return-value` for details.
