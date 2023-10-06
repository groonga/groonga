.. -*- rst -*-

.. groonga-command
.. database: logical_range_filter

``logical_range_filter``
========================

Summary
-------

.. versionadded:: 5.0.0

``logical_range_filter`` is a sharding version of
:doc:`range_filter`. ``logical_range_filter`` searches records from
multiple tables and outputs them.

``logical_range_filter`` is similar to :doc:`logical_select`. Both of
them searches records from multiples tables and outputs
them. ``logical_range_filter`` stops searching when the number of
matched records is requested the number of
records. :doc:`logical_select` searches all records and outputs only
needed records.

``logical_range_filter`` has performance merit but some restrictions.

If many records are matched and requested records are small,
``logical_range_filter`` will be faster than :doc:`logical_select`.

``logical_range_filter`` doesn't support drilldown because drilldown
requires all matched records. ``logical_range_filter`` may not find
all matched records. So ``logical_range_filter`` doesn't support
drilldown.

``logical_range_filter`` doesn't return the number of all matched
records. Because ``logical_range_filter`` may not search all matched
records.

You need to :doc:`plugin_register` ``sharding`` plugin because
this command is included in ``sharding`` plugin.

Syntax
------

This command takes many parameters.

The required parameters are ``logical_table`` and ``shard_key``::

  logical_range_filter
    logical_table
    shard_key
    [min=null]
    [min_border="include"]
    [max=null]
    [max_border="include"]
    [order="ascending"]
    [filter=null]
    [offset=0]
    [limit=10]
    [output_columns="_key, *"]
    [use_range_index=null]
    [post_filter=null]
    [sort_keys=null]

There are some parameters that can be only used as named
parameters. You can't use these parameters as ordered parameters. You
must specify parameter name.

Here are parameters that can be only used as named parameters:

  * ``cache=no``

.. versionadded:: 7.0.9

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

Usage
-----

Let's learn about usage with examples. This section shows many popular
usages.

You need to register ``sharding`` plugin because this command is
included in ``sharding`` plugin.

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/usage_plugin_register.log
.. plugin_register sharding

Here are a schema definition and sample data to show usage.

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/usage_setup.log
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

TODO

Parameters
----------

This section describes parameters of ``logical_range_filter``.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are required parameters, ``logical_table`` and ``shard_key``.

``logical_table``
"""""""""""""""""

Specifies logical table name. It means table name without
``_YYYYMMDD`` postfix. If you use actual table such as
``Entries_20150708``, ``Entries_20150709`` and so on, logical table
name is ``Entries``.

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/logical_table_existent.log
.. logical_range_filter --logical_table Entries --shard_key created_at

If nonexistent table is specified, an error is returned.

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/logical_table_nonexistent.log
.. logical_range_filter --logical_table Nonexistent --shard_key created_at

``shard_key``
"""""""""""""

Specifies column name which is treated as shared key in each parted table.

TODO: Add examples

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

``min``
"""""""

Specifies the min value of ``shard_key``

TODO: Add examples

``min_border``
""""""""""""""

Specifies whether the min value of borderline must be include or not.
Specify ``include`` or ``exclude`` as the value of this parameter.

TODO: Add examples

``max``
"""""""

Specifies the max value of ``shard_key``.

TODO: Add examples

``max_border``
""""""""""""""

Specifies whether the max value of borderline must be include or not.
Specify ``include`` or ``exclude`` as the value of this parameter.

TODO: Add examples

``order``
""""""""""

Specifies order of search result.
Specify ``ascending`` or ``descending`` as the value of this parameter.

If we set ``ascending`` in this parameter, search results are sorted by ascending order based on ``shared_key``.
If we set ``descending`` in this parameter, search results are sorted by descending order based on ``shared_key``.

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/order_existent.log
.. logical_range_filter --logical_table Entries --shard_key created_at --order "descending"

.. _logical-range-filter-search-related-parameters:

Search related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^

This command provides :doc:`select` compatible search related
parameters.

.. _logical-range-filter-filter:

``filter``
""""""""""

Corresponds to :ref:`select-filter` in :doc:`select`. See
:ref:`select-filter` for details.

Here is an example:

TODO

.. _logical-range-filter-post-filter:

``post_filter``
"""""""""""""""

.. versionadded:: 7.1.1

Specifies the filter text that is processed after ``filtered`` stage
dynamic columns are generated. You can use ``post_filter`` to filter
by ``filtered`` stage dynamic columns. Others are the same as
:ref:`logical-range-filter-filter`.

If you use ``post_filter``, "stop searching when enough records are
matched in a table" feature is disabled. ("Stop searching against rest
tables when enough records are matched" is still enabled.)  Because
``post_filter`` needs to wait dynamic columns generation. See also
:ref:`logical-range-filter-dynamic-column-related-parameters`.

Here is an example that shows entries only in popular tag. All target
entries have ``system`` or ``use`` words:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/post_filter.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_sum_per_tag].stage filtered \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys 'tag' \
..   --filter 'content @ "system" || content @ "use"' \
..   --post_filter 'n_likes_sum_per_tag > 10' \
..   --output_columns _key,n_likes,n_likes_sum_per_tag

Output related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^

.. _logical-range-filter-output-columns:

``output_columns``
""""""""""""""""""

Corresponds to :ref:`select-output-columns` in :doc:`select`. See
:ref:`select-output-columns` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/output_columns.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --output_columns '_key, *'

.. _logical-range-filter-sort-keys:

``sort_keys``
"""""""""""""

.. versionadded:: 8.0.2

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
.. include:: ../../example/reference/commands/logical_range_filter/sort_keys_one.log
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --sort_keys _key \
..   --output_columns _key,created_at

Here is an example that uses ``shard_key`` based value as the first
sort key:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/sort_keys_on_shared_key.log
.. plugin_register functions/time
.. logical_select \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[hour2].stage filtered \
..   --columns[hour2].type Time \
..   --columns[hour2].flags COLUMN_SCALAR \
..   --columns[hour2].value 'time_classify_hour(created_at, 2)' \
..   --sort_keys hour2,-n_likes \
..   --output_columns hour2,n_likes,_key

.. _logical-range-filter-offset:

``offset``
""""""""""

Corresponds to :ref:`select-offset` in :doc:`select`. See
:ref:`select-offset` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/offset.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --offset 2

.. _logical-range-filter-limit:

``limit``
"""""""""

Corresponds to :ref:`select-limit` in :doc:`select`. See
:ref:`select-limit` for details.

The difference from :doc:`select` is that this command stops searching
when all requested records specified by
:ref:`logical-range-filter-offset` and
:ref:`logical-range-filter-limit` are found.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/limit.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --limit 2

Test related parameters
^^^^^^^^^^^^^^^^^^^^^^^

``use_range_index``
"""""""""""""""""""

Specifies whether range_index is used or not.
Note that it's a parameter for test. It should not be used for production.

TODO: Add examples

.. _logical-range-filter-dynamic-column-related-parameters:

Dynamic column related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 7.0.9

All dynamic column related parameters in :doc:`select` are
supported. See :ref:`select-dynamic-column-related-parameters` for
details.

If you use one or more dynamic columns, "stop searching when enough
records are matched in a table" feature is disabled. ("Stop searching
against rest tables when enough records are matched" is still
enabled.)  ``logical_range_filter`` searches all matched records in a
table even when requested the number of matched records is small. It's
for supporting window function. Running window function requires all
target records in a table.

.. _logical-range-filter-columns-name-stage:

``columns[${NAME}].stage``
""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-stage` in :doc:`select`. See
:ref:`select-columns-name-stage` for details.

This is a required parameter.

Here is an example that creates ``is_popular`` column at ``initial``
stage. You can use ``is_popular`` in all parameters such as ``filter``
and ``output_columns``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/columns_name_stage.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[is_popular].stage initial \
..   --columns[is_popular].type Bool \
..   --columns[is_popular].value 'n_likes >= 10' \
..   --filter is_popular \
..   --output_columns _id,is_popular,n_likes

.. _logical-range-filter-columns-name-flags:

``columns[${NAME}].flags``
""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-flags` in :doc:`select`. See
:ref:`select-columns-name-flags` for details.

The default value is ``COLUMN_SCALAR``.

Here is an example that creates a vector column by ``COLUMN_VECTOR``
flags. ``plugin_register functions/vector`` is for using
:doc:`/reference/functions/vector_new` function:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/columns_name_flags.log
.. plugin_register functions/vector
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[vector].stage initial \
..   --columns[vector].flags COLUMN_VECTOR \
..   --columns[vector].type UInt32 \
..   --columns[vector].value 'vector_new(1, 2, 3)' \
..   --output_columns _id,vector

.. _logical-range-filter-columns-name-type:

``columns[${NAME}].type``
"""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-type` in :doc:`select`. See
:ref:`select-columns-name-type` for details.

This is a required parameter.

Here is an example that creates a ``ShortText`` type column. Stored
value is casted to ``ShortText`` automatically. In this example,
number is casted to ``ShortText``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/columns_name_type.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_string].stage initial \
..   --columns[n_likes_string].type ShortText \
..   --columns[n_likes_string].value n_likes \
..   --output_columns _id,n_likes,n_likes_string

.. _logical-range-filter-columns-name-value:

``columns[${NAME}].value``
""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-value` in :doc:`select`. See
:ref:`select-columns-name-value` for details.

You need to specify :doc:`/reference/window_function` as ``value``
value and other window function related parameters when you use window
function. See :ref:`logical-range-filter-window-function-related-parameters`
for details.

This is a required parameter.

Here is an example that creates a new dynamic column that stores the
number of characters of content. This example uses
:doc:`/reference/functions/string_length` function in
``functions/string`` plugin to compute the number of characters in a
string. :doc:`plugin_register` is used to register
``functions/string`` plugin:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/columns_name_value.log
.. plugin_register functions/string
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[content_length].stage initial \
..   --columns[content_length].type UInt32 \
..   --columns[content_length].value 'string_length(content)' \
..   --output_columns _id,content,content_length

.. _logical-range-filter-window-function-related-parameters:

Window function related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 7.0.9

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
.. include:: ../../example/reference/commands/logical_range_filter/window_function_for_over_shard.log
.. plugin_register sharding
.. 
.. table_create Logs_20170415 TABLE_NO_KEY
.. column_create Logs_20170415 timestamp COLUMN_SCALAR Time
.. column_create Logs_20170415 price COLUMN_SCALAR UInt32
.. column_create Logs_20170415 n_likes COLUMN_SCALAR UInt32
.. 
.. table_create Logs_20170416 TABLE_NO_KEY
.. column_create Logs_20170416 timestamp COLUMN_SCALAR Time
.. column_create Logs_20170416 price COLUMN_SCALAR UInt32
.. column_create Logs_20170416 n_likes COLUMN_SCALAR UInt32
.. 
.. load --table Logs_20170415
.. [
.. {"timestamp": "2017/04/15 00:00:00", "n_likes": 2, "price": 100},
.. {"timestamp": "2017/04/15 01:00:00", "n_likes": 1, "price": 100},
.. {"timestamp": "2017/04/15 01:00:00", "n_likes": 2, "price": 200}
.. ]
.. 
.. load --table Logs_20170416
.. [
.. {"timestamp": "2017/04/16 10:00:00", "n_likes": 1, "price": 200},
.. {"timestamp": "2017/04/16 11:00:00", "n_likes": 2, "price": 300},
.. {"timestamp": "2017/04/16 11:00:00", "n_likes": 1, "price": 300}
.. ]
.. 
.. logical_range_filter Logs \
..   --shard_key timestamp \
..   --columns[count].stage initial \
..   --columns[count].type UInt32 \
..   --columns[count].flags COLUMN_SCALAR \
..   --columns[count].value 'window_count()' \
..   --columns[count].window.group_keys price \
..   --output_columns price,count

.. _logical-range-filter-columns-name-window-sort-keys:

``columns[${NAME}].window.sort_keys``
"""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-window-sort-keys` in
:doc:`select`. See :ref:`select-columns-name-window-sort-keys` for
details.

You must specify :ref:`logical-range-filter-columns-name-window-sort-keys`
or :ref:`logical-range-filter-columns-name-window-group-keys` to use window
function.

Here is an example that computes cumulative sum per
``Entries.tag``. Each group is sorted by ``Entries._key``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/columns_name_window_sort_keys.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_cumulative_sum_per_tag].stage initial \
..   --columns[n_likes_cumulative_sum_per_tag].type UInt32 \
..   --columns[n_likes_cumulative_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_cumulative_sum_per_tag].window.sort_keys _key \
..   --columns[n_likes_cumulative_sum_per_tag].window.group_keys tag \
..   --output_columns tag,_key,n_likes,n_likes_cumulative_sum_per_tag

.. _logical-range-filter-columns-name-window-group-keys:

``columns[${NAME}].window.group_keys``
""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-window-group-keys` in
:doc:`select`. See :ref:`select-columns-name-window-group-keys` for
details.

You must specify :ref:`logical-range-filter-columns-name-window-sort-keys`
or :ref:`logical-range-filter-columns-name-window-group-keys` to use window
function.

Here is an example that computes sum per ``Entries.tag``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_range_filter/columns_name_window_group_keys.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_sum_per_tag].stage initial \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys tag \
..   --output_columns tag,_key,n_likes,n_likes_sum_per_tag

Cache related parameter
^^^^^^^^^^^^^^^^^^^^^^^

.. _logical-range-filter-cache:

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
.. include:: ../../example/reference/commands/logical_range_filter/cache_no.log
.. logical_range_filter \
..   --logical_table Entries \
..   --shard_key created_at \
..   --cache no

The default value is ``yes``.

Return value
------------

The command returns a response with the following format::

  [
    HEADER,
    [
      COLUMNS,
      RECORDS
    ]
  ]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.

``COLUMNS`` describes about output columns specified by
:ref:`logical-range-filter-output-columns`. It uses the following
format::

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
:ref:`logical-range-filter-output-columns`.

Column type is Groonga's type name or ``null``. It doesn't describe
whether the column value is vector or scalar. You need to determine it
by whether real column value is array or not.

See :doc:`/reference/types` for type details.

``null`` is used when column value type isn't determined. For example,
function call in :ref:`logical-range-filter-output-columns` such as
``--output_columns "snippet_html(content)"`` uses ``null``.

Here is an example of ``COLUMNS``::

  [
    ["_id",     "UInt32"],
    ["_key",    "ShortText"],
    ["n_likes", "UInt32"],
  ]

``RECORDS`` includes column values for each matched record. Included
records are selected by :ref:`logical-range-filter-offset` and
:ref:`logical-range-filter-limit`. It uses the following format::

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
