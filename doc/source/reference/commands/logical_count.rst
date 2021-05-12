.. -*- rst -*-

.. groonga-command
.. database: logical_count

``logical_count``
=================

Summary
-------

.. versionadded:: 5.0.0

``logical_count`` is a command that has only count feature in
:doc:`logical_select`. :doc:`logical_select` searches records from
multiple tables, outputs the number of matched records, outputs
columns of the matched records and so on. ``logical_count`` only
searches records from multiple tables and output the number of matched
records.

``logical_count`` is useful when you just want the number of matched
records. You can use ``logical_count`` and :doc:`logical_range_filter`
at once. You can show the first N matched records before you get the
number of matched records. If you use only :doc:`logical_select`, you
need to wait finishing all search.

You need to :doc:`plugin_register` ``sharding`` plugin because
this command is included in ``sharding`` plugin.

Syntax
------

This command takes many parameters.

The required parameters are ``logical_table`` and ``shard_key``. Other
parameters are optional::

  logical_count
    logical_table
    shard_key
    [min=null]
    [min_border="include"]
    [max=null]
    [max_border="include"]
    [filter=null]
    [post_filter=null]

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
     * ``--columns[name2].stage initial``
     * ``--columns[name2].type Float``
     * ``--columns[name2].value '_score * 0.1'``

.. _logical-count-usage:

Usage
-----

Let's learn about usage with examples. This section shows many popular
usages.

You need to register ``sharding`` plugin because this command is
included in ``sharding`` plugin.

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/usage_plugin_register.log
.. plugin_register sharding


Note that ``logical_count`` is implemented as an experimental plugin, and the specification may be changed in the future.

Here is the simple example which shows how to use this feature. Let's consider to count specified logs which are stored into multiple tables.

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

Here is an example to count the number of records which have
``Groonga`` or ``Senna`` in ``content`` column. ``logical_count``
searches records over all ``Entries_YYYYMMDD`` tables.

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/usage.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --filter 'query("content", "Groonga OR Senna")'

Here are matched records:

  * ``_key:"Groonga"`` in ``Entries_20150708``

  * ``_key:"Good-bye Senna"`` in ``Entries_20150709``

Parameters
----------

This section describes parameters of this command.

Required parameters
^^^^^^^^^^^^^^^^^^^

There are required parameters, ``logical_table`` and ``shard_key``.

.. _logical-count-logical-table:

``logical_table``
"""""""""""""""""

Specifies logical table name. It means table name without
``_YYYYMMDD`` postfix.  If you use actual table such as
``Logs_20150203``, ``Logs_20150203`` and so on, logical table name is
``Logs``.

You can count all records by specifying only ``logical_table`` and
``shard_key`` parameters. They are required parameters.

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/logical_table_existent.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at

If nonexistent table is specified, an error is returned.

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/logical_table_nonexistent.log
.. logical_count \
..   --logical_table Nonexistent \
..   --shard_key created_at

.. _logical-count-shard-key:

``shard_key``
"""""""""""""

Specifies column name which is treated as shared key. Shard key is a
column that stores data that is used for distributing records to
suitable shards.

Shard key must be ``Time`` type for now.

See :ref:`logical-count-logical-table` how to specify ``shard_key``.

Optional parameters
^^^^^^^^^^^^^^^^^^^

There are optional parameters.

.. _logical-count-min:

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
.. include:: ../../example/reference/commands/logical_count/min.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --min "2015/07/09 00:00:00"

.. _logical-count-min-border:

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
.. include:: ../../example/reference/commands/logical_count/min_border.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --min "2015/07/09 00:00:00" \
..   --min_border "exclude"

.. _logical-count-max:

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
.. include:: ../../example/reference/commands/logical_count/max.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --max "2015/07/08 23:59:59"

.. _logical-count-max-border:

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
.. include:: ../../example/reference/commands/logical_count/max_border.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --max "2015/07/09 00:00:00" \
..   --max_border "exclude"

.. _logical-count-search-related-parameters:

Search related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^

This command provides :doc:`select` compatible search related
parameters.

.. _logical-count-filter:

``filter``
""""""""""

Corresponds to :ref:`select-filter` in :doc:`select`. See
:ref:`select-filter` for details.

Here is an example:

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/filter.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --filter "n_likes <= 5"

.. _logical-count-post-filter:

``post_filter``
"""""""""""""""

.. versionadded:: 8.0.1

Specifies the filter text that is processed after ``filtered`` stage
dynamic columns are generated. You can use ``post_filter`` to filter
by ``filtered`` stage dynamic columns. Others are the same as
:ref:`select-filter`.

Here is an example that shows entries only in popular tag. All target
entries have ``system`` or ``use`` words:

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/post_filter.log
.. logical_cout \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_sum_per_tag].stage filtered \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys 'tag' \
..   --filter 'content @ "system" || content @ "use"' \
..   --post_filter 'n_likes_sum_per_tag > 10' \

.. _logical-count-dynamic-column-related-parameters:

Dynamic column related parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. versionadded:: 7.0.9

All dynamic column related parameters in :doc:`select` are
supported. See :ref:`select-dynamic-column-related-parameters` for
details.

.. _logical-count-columns-name-stage:

``columns[${NAME}].stage``
""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-stage` in :doc:`select`. See
:ref:`select-columns-name-stage` for details.

This is a required parameter.

``initial`` stage and ``filtered`` stage are valid. Because there are
no processes after ``output`` stages.

.. versionadded:: 8.0.1

   ``filtered`` stage is valid since 8.0.1.

Here is an example that creates ``is_popular`` column at ``initial``
stage. You can use ``is_popular`` in all parameters such as ``filter``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/columns_name_stage.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[is_popular].stage initial \
..   --columns[is_popular].type Bool \
..   --columns[is_popular].value 'n_likes >= 10' \
..   --filter is_popular

.. _logical-count-columns-name-flags:

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
.. include:: ../../example/reference/commands/logical_count/columns_name_flags.log
.. plugin_register functions/vector
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[vector].stage initial \
..   --columns[vector].flags COLUMN_VECTOR \
..   --columns[vector].type UInt32 \
..   --columns[vector].value 'vector_new(1, 2, 3)' \
..   --filter 'vector_size(vector) > 2'

.. _logical-count-columns-name-type:

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
.. include:: ../../example/reference/commands/logical_count/columns_name_type.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_string].stage initial \
..   --columns[n_likes_string].type ShortText \
..   --columns[n_likes_string].value n_likes \
..   --filter 'n_likes_string == "3"'

.. _logical-count-columns-name-value:

``columns[${NAME}].value``
""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-value` in :doc:`select`. See
:ref:`select-columns-name-value` for details.

You need to specify :doc:`/reference/window_function` as ``value``
value and other window function related parameters when you use window
function. See :ref:`logical-count-window-function-related-parameters`
for details.

This is a required parameter.

Here is an example that creates a new dynamic column that stores the
number of characters of content. This example uses
:doc:`/reference/functions/string_length` function in
``functions/string`` plugin to compute the number of characters in a
string. :doc:`plugin_register` is used to register
``functions/string`` plugin:

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/columns_name_value.log
.. plugin_register functions/string
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[content_length].stage initial \
..   --columns[content_length].type UInt32 \
..   --columns[content_length].value 'string_length(content)' \
..   --filter 'content_length >= 40'

.. _logical-count-window-function-related-parameters:

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
.. include:: ../../example/reference/commands/logical_count/window_function_for_over_shard.log

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
.. logical_count Logs \
..   --shard_key timestamp \
..   --columns[count].stage initial \
..   --columns[count].type UInt32 \
..   --columns[count].flags COLUMN_SCALAR \
..   --columns[count].value 'window_count()' \
..   --columns[count].window.group_keys price \
..   --output_columns price,count

.. _logical-count-columns-name-window-sort-keys:

``columns[${NAME}].window.sort_keys``
"""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-window-sort-keys` in
:doc:`select`. See :ref:`select-columns-name-window-sort-keys` for
details.

You must specify :ref:`logical-count-columns-name-window-sort-keys`
or :ref:`logical-count-columns-name-window-group-keys` to use window
function.

Here is an example that computes cumulative sum per
``Entries.tag``. Each group is sorted by ``Entries._key``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/columns_name_window_sort_keys.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_cumulative_sum_per_tag].stage initial \
..   --columns[n_likes_cumulative_sum_per_tag].type UInt32 \
..   --columns[n_likes_cumulative_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_cumulative_sum_per_tag].window.sort_keys _key \
..   --columns[n_likes_cumulative_sum_per_tag].window.group_keys tag \
..   --filter 'n_likes_cumulative_sum_per_tag > 5'

.. _logical-count-columns-name-window-group-keys:

``columns[${NAME}].window.group_keys``
""""""""""""""""""""""""""""""""""""""

.. versionadded:: 7.0.9

Corresponds to :ref:`select-columns-name-window-group-keys` in
:doc:`select`. See :ref:`select-columns-name-window-group-keys` for
details.

You must specify :ref:`logical-count-columns-name-window-sort-keys`
or :ref:`logical-count-columns-name-window-group-keys` to use window
function.

Here is an example that computes sum per ``Entries.tag``:

.. groonga-command
.. include:: ../../example/reference/commands/logical_count/columns_name_window_group_keys.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --columns[n_likes_sum_per_tag].stage initial \
..   --columns[n_likes_sum_per_tag].type UInt32 \
..   --columns[n_likes_sum_per_tag].value 'window_sum(n_likes)' \
..   --columns[n_likes_sum_per_tag].window.group_keys tag \
..   --filter 'n_likes_sum_per_tag > 5'

Cache related parameter
^^^^^^^^^^^^^^^^^^^^^^^

.. _logical-count-cache:

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
.. include:: ../../example/reference/commands/logical_count/cache_no.log
.. logical_count \
..   --logical_table Entries \
..   --shard_key created_at \
..   --cache no

The default value is ``yes``.

Return value
------------

The command returns a response with the following format::

  [HEADER, N_HITS]

If the command fails, error details are in ``HEADER``.

See :doc:`/reference/command/output_format` for ``HEADER``.

``N_HITS`` is the number of matched records.
