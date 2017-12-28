.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

.. _release-7-1-0:

Release 7.1.0 - 2017-12-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/load`] Improved the ``load``'s
  query-log format.
  Added detail below items in the ``load``'s query-log.

    * outputs number of loaded records.
    * outputs number of error records and columns.
    * outputs number of total records.

* [:doc:`/reference/commands/logical_count`] Improved the
  ``logical_count``'s query-log format.
  Added detail below items in the ``logical_count``'s query-log.

    * outputs number of count.

* [:doc:`/reference/commands/logical_select`] Improve the
  ``logical_select``'s query-log format.
  Added detail below items in the ``logical_select``'s query-log.

    * log N outputs.
    * outputs plain drilldown.
    * outputs labeled drilldown.
    * outputs selected in each shard.
    * use "[...]" for target information.

* [:doc:`/reference/commands/delete`] Improved the ``delete``'s
  query-log format.
  Added detail below items in the ``delete``'s query-log.

    * outputs number of deleted and error records.
    * outputs number of rest number of records.

* [groonga] Ensured stopping by C-c.

* Used ``NaN`` and ``Infinity``, ``-Infinity`` instead of Lisp
  representations(``#<nan>`` and  ``#i1/0``, ``#-i1/0``).

* Supported vector for drilldown calc target.

* Partially supported keyword extraction from regexp search.
  It enables ``highlight_html`` and ``snippet_html`` for regexp search.
  [GitHub#787][Reported by takagi01]

* [bulk] Reduced the number of ``realloc()``.
  ``grn_bulk_*()`` API supports it.

  It improves performance for large output case on Windows.
  For example, it causes 100x faster for 100MB over output.

  Because ``realloc()`` is heavy on Windows.

* Enabled ``GRN_II_OVERLAP_TOKEN_SKIP_ENABLE`` only when its value is "yes".

* Deprecated ``GRN_NGRAM_TOKENIZER_REMOVE_BLANK_DISABLE``.
  Use ``GRN_NGRAM_TOKENIZER_REMOVE_BLANK_ENABLE=no`` instead.

* Added new function ``index_column_source_records``.
  It gets source records of index column.[Patch by Naoya Murakami]

* [:doc:`/reference/commands/select`] Supported negative "offset" for "offset + size - limit" >= 0

* Added ``grn_column_cache``.
  It'll improve performance for getter of fixed size column value.

* [:doc:`/reference/executables/groonga`] Added ``--listen-backlog option``.
  You can customize ``listen(2)``'s backlog by this option.

Fixes
^^^^^

* Fixed a memory leak in ``highlight_full``

* Fixed a crash bug by early unlink
  It's not caused by instruction in ``grn_expr_parse()`` but it's caused when
  libgroonga user such as Mroonga uses the following instructions:

    1. ``grn_expr_append_const("_id")``
    2. ``grn_expr_append_op(GRN_OP_GET_VALUE)``

Thanks
^^^^^^

* takagi01
* Naoya Murakami

.. _release-7-0-9:

Release 7.0.9 - 2017-11-29
--------------------------

Improvements
^^^^^^^^^^^^

* Supported newer version of Apache Arrow. In this release, 0.8.0 or
  later is required for Apache Arrow support.

* [sharding] Added new API for dynamic columns.

  * Groonga::LabeledArguments

* [sharding] Added convenient ``Table#select_all`` method.

* [:doc:`/reference/commands/logical_range_filter`] Supported dynamic
  columns. Note that ``initial`` and ``filtered`` stage are only
  supported.

* [:doc:`/reference/commands/logical_range_filter`] Added documentation
  about ``cache`` parameter and dynamic columns.

* [:doc:`/reference/commands/logical_count`] Supported dynamic
  columns. Note that ``initial`` stage is only supported.

* [:doc:`/reference/commands/logical_count`] Added documentation about
  named parameters.

* [:doc:`/reference/commands/select`] Supported ``--match_columns _key``
  without index.

* [:doc:`/reference/functions/in_values`] Supported to specify more
  than 126 values. [GitHub#760] [GitHub#781] [groonga-dev,04449]
  [Reported by Murata Satoshi]

* [httpd] Updated bundled nginx to 1.13.7.

Fixes
^^^^^

* [httpd] Fixed build error when old Groonga is already installed.
  [GitHub#775] [Reported by myamanishi3]

* [:doc:`/reference/functions/in_values`] Fixed a bug that
  ``in_values`` with too many arguments can cause a crash. This bug is
  found during supporting more than 126 values. [GitHub#780]

* [cmake] Fixed LZ4 and MessagePack detection. [Reported by Sergei
  Golubchik]

* [:ref:`offline-index-construction`] Fixed a bug that offline index
  construction for vector column consumes unnecessary resources. If
  you have a log of elements in one vector column and many records,
  Groogna will crash.
  [groonga-dev,04533][Reported by Toshio Uchiyama]

Thanks
^^^^^^

* Murata Satoshi
* myamanishi3
* Sergei Golubchik
* Toshio Uchiyama

.. _release-7-0-8:

Release 7.0.8 - 2017-10-29
--------------------------

Improvements
^^^^^^^^^^^^

* [windows] Supported backtrace on crash.
  This feature not only function call history but also source filename
  and number of lines can be displayed as much as possible.
  This feature makes problem solving easier.

* Supported ``( )`` (empty block) only query (``--query "( )"``) for
  ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error. [GitHub#767]

* Supported ``(+)`` (only and block) only query (``--query "(+)"``)
  for ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error. [GitHub#767]

* Supported ``~foo`` (starting with "~") query (``--query "~y"``) for
  ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error. [GitHub#767]

* Modified log level of ``expired`` from ``info`` to ``debug``.
  ``2017-10-29 14:05:34.123456|i| <0000000012345678:0> expired
  i=000000000B123456 max=10 (2/2)`` This message is logged when memory
  mapped area for index is unmapped.  Thus, this log message is useful
  information for debugging, in other words, as it is unnecessary
  information in normal operation, we changed log level from ``info``
  to ``debug``.

* Supported Ubuntu 17.10 (Artful Aardvark)

Fixes
^^^^^

* [dat] Fixed a bug that large file is created unexpectedly in the
  worst case during database expansion process. This bug may occurs
  when you create/delete index columns so frequently. In 7.0.7
  release, a related bug was fixed - "``table_create`` command fails
  when there are many deleted keys", but it turns out that it is not
  enough in the worst case.

* [:doc:`/reference/commands/logical_select`] Fixed a bug that when
  ``offset`` and ``limit`` were applied to multiple shards at the same
  time, there is a case that it returns a fewer number of records
  unexpectedly.

.. _release-7-0-7:

Release 7.0.7 - 2017-09-29
--------------------------

Improvements
^^^^^^^^^^^^

* Supported ``+`` only query (``--query "+"``) for
  ``QUERY_NO_SYNTAX_ERROR``. In the previous version, it caused an
  error.

* [httpd] Updated bundled nginx to 1.13.5.

* [:doc:`/reference/commands/dump`] Added the default argument values
  to the syntax section.

* [:doc:`/reference/command/command_version`] Supported ``--default-command-version 3``.

* Supported caching select result with function call. Now, most of
  existing functions supports this feature. There are two exception,
  when ``now()`` and ``rand()`` are used in query, select result will
  not cached. Because of this default behavior change, new APIs are
  introduced.

  * ``grn_proc_set_is_stable()``
  * ``grn_proc_is_stable()``

  Note that if you add a new function that may return different result
  with the same argument, you must call ``grn_proc_is_stable(ctx,
  proc, GRN_FALSE)``.  If you don't call it, select result with the
  function call is cached and is wrong result for multiple requests.

Fixes
^^^^^

* [windows] Fixed to clean up file handle correctly on failure when
  ``database_unmap`` is executed. There is a case that critical
  section is not initialized when request is canceled before executing
  ``database_unmap``. In such a case, it caused a crach bug.

* [:doc:`/reference/tokenizers`] Fixed document for wrong tokenizer
  names. It should be ``TokenBigramIgnoreBlankSplitSymbolAlpha`` and
  ``TokenBigramIgnoreBlankSplitSymbolAlphaDigit``.

* Changed not to keep created empty file on error.

  In the previous versions, there is a case that empty file keeps
  remain on error.

  Here is the senario to reproduce:

    1. creating new file by grn_fileinfo_open succeeds
    2. mapping file by DO_MAP() is failed

  In such a case, it causes an another error such as
  "already file exists" because of the file which
  isn't under control. so these file should be removed during
  cleanup process.

* Fixed a bug that Groonga may be crashed when search process is
  executed during executing many updates in a short time.

* [:doc:`/reference/commands/table_create`] Fixed a bug that
  ``table_create`` failed when there are many deleted keys.

.. _release-7-0-6:

Release 7.0.6 - 2017-08-29
--------------------------

Improvements
^^^^^^^^^^^^

* Supported prefix match search using multiple
  indexes. (e.g. ``--query "Foo*" --match_columns
  "TITLE_INDEX_COLUMN||BODY_INDEX_COLUMN"``).

* [:doc:`/reference/window_functions/window_count`] Supported
  ``window_count`` function to add count data to result set. It is
  useful to analyze or filter additionally.

* Added the following API

  * ``grn_obj_get_disk_usage():``
  * ``GRN_EXPR_QUERY_NO_SYNTAX_ERROR``
  * ``grn_expr_syntax_expand_query_by_table()``
  * ``grn_table_find_reference_object()``

* [:doc:`/reference/commands/object_inspect`] Supported to show disk
  usage about specified object.

* Supported falling back query parse feature. It is enabled when
  ``QUERY_NO_SYNTAX_ERROR`` flag is set to ``query_flags``. (this
  feature is disabled by default). If this flag is set, query never
  causes syntax error. For example, "A +" is parsed and escaped
  automatically into "A \+". This behavior is useful when application
  uses user input directly and doesn't want to show syntax error to
  user and in log.

* Supported to adjust score for term in query. ">", "<", and "~"
  operators are supported. For example, ">Groonga" increments score of
  "Groonga", "<Groonga" decrements score of "Groonga". "~Groonga"
  decreases score of matched document in the current search
  result. "~" operator doesn't change search result itself.

* Improved performance to remove table. ``thread_limit=1`` is not
  needed for it. The process about checking referenced table existence
  is done without opening objects. As a result, performance is
  improved.

* [httpd] Updated bundled nginx to 1.13.4.

Fixes
^^^^^

* [:doc:`/reference/commands/dump`] Fixed a bug that the 7-th unnamed
  parameter for `--sort_hash_table` option is ignored.

* [:doc:`/reference/commands/schema`] Fixed a typo in command line
  parameter name. It should be `source` instead of `sources`.
  [groonga-dev,04449] [Reported by murata satoshi]

* [:doc:`/reference/commands/ruby_eval`] Fixed crash when ruby_eval
  returned syntax error. [GitHub#751] [Patch by ryo-pinus]

Thanks
^^^^^^

* murata satoshi

* ryo-pinus

.. _release-7-0-5:

Release 7.0.5 - 2017-07-29
--------------------------

Improvements
^^^^^^^^^^^^

* [httpd] Updated bundled nginx to 1.13.3. Note that this version
  contains security fix for CVE-2017-7529.

* [:doc:`/reference/commands/load`] Supported to load the value of max
  UInt64. In the previous versions, max UInt64 value is converted into
  0 unexpectedlly.

* Added the following API

  * ``grn_window_get_size()`` [GitHub#725] [Patch by Naoya Murakami]

* [:doc:`/reference/functions/math_abs`] Supported ``math_abs()``
  function to calculate absolute value. [GitHub#721]

* Supported to make ``grn_default_logger_set_path()`` and
  ``grn_default_query_logger_set_path()`` thread safe.

* [windows] Updated bundled pcre library to 8.41.

* [:doc:`/reference/commands/normalize`] Improved not to output
  redundant empty string ``""`` on error. [GitHub#730]

* [functions/time] Supported to show error message when division by
  zero was happened. [GitHub#733] [Patch by Naoya Murakami]

* [windows] Changed to map ``ERROR_NO_SYSTEM_RESOURCES`` to
  ``GRN_RESOURCE_TEMPORARILY_UNAVAILABLE``. In the previous versions,
  it returns ``rc=-1`` as a result code. It is not helpful to
  investigate what actually happened. With this fix, it returns
  ``rc=-12``.

* [functions/min][functions/max] Supported vector column. Now you need
  not to care scalar column or vector column to use. [GitHub#735]
  [Patch by Naoya Murakami]

* [:doc:`/reference/commands/dump`] Supported ``--sort_hash_table``
  option to sort by ``_key`` for hash table. Specify
  ``--sort_hash_table yes`` to use it.

* [:doc:`/reference/functions/between`] Supported to specify index
  column. [GitHub#740] [Patch by Naoya Murakami]

* [load] Supported Apache Arrow 0.5.0 or later.

* [:doc:`/troubleshooting/how_to_analyze_error_message`]
  Added howto article to analyze error message in Groonga.

* [:doc:`/install/debian`] Updated required package list to
  build from source.

* [:doc:`/install/ubuntu`] Dropped Ubuntu 16.10 (Yakkety
  Yak) support. It has reached EOL at July 20, 2017.

Fixes
^^^^^

* Fixed to construct correct fulltext indexes against vector column
  which type belongs to text family (```ShortText`` and so on). This
  fix resolves that fulltext search doesn't work well against text
  vector column after updating indexes. [GitHub#494]

* [:doc:`/reference/commands/thread_limit`] Fixed a bug that deadlock
  occurs when thread_limit?max=1 is requested at once.

* [:doc:`/reference/executables/groonga-httpd`] Fixed a mismatch path
  of pid file between default one and restart command assumed. This
  mismatch blocked restarting groonga-httpd. [GitHub#743] [Reported by
  sozaki]

Thanks
^^^^^^

* Naoya Murakami

.. _release-7-0-4:

Release 7.0.4 - 2017-06-29
--------------------------

Improvements
^^^^^^^^^^^^

* Added physical create/delete operation logs to identify problem for
  troubleshooting. [GitHub#700,#701]

* [:doc:`/reference/functions/in_records`] Improved performance for
  fixed sized column. It may reduce 50% execution time.

* [:doc:`/reference/executables/grndb`] Added ``--log-path`` option.
  [GitHub#702,#703]

* [:doc:`/reference/executables/grndb`] Added ``--log-level`` option.
  [GitHub#706,#708]

* Added the following API

  * ``grn_operator_to_exec_func()``
  * ``grn_obj_is_corrupt()``

* Improved performance for "FIXED_SIZE_COLUMN OP CONSTANT". Supported
  operators are: ``==``, ``!=``, ``<``, ``>``, ``<=`` and ``>=``.

* Improved performance for "COLUMN OP VALUE && COLUMN OP VALUE && ...".

* [:doc:`/reference/executables/grndb`] Supported corrupted object
  detection with ``grndb check``.

* [:doc:`/reference/commands/io_flush`] Supported ``--only_opened``
  option which enables to flush only opened database objects.

* [:doc:`/reference/executables/grndb`] Supported to detect/delete
  orphan "inspect" object. The orphaned "inspect" object is created by
  renamed command name from ``inspect`` to ``object_inspect``.

Fixes
^^^^^

* [rpm][centos] Fixed unexpected macro expansion problem with
  customized build. This bug only affects when rebuilding Groonga SRPM
  with customized ``additional_configure_options`` parameter in spec
  file.

* Fixed missing null check for ``grn_table_setoperation()``. There is a
  possibility of crash bug when indexes are broken. [GitHub#699]

Thanks
^^^^^^

.. _release-7-0-3:

Release 7.0.3 - 2017-05-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/select`] Add document about
  :ref:`full-text-search-with-specific-index-name`.

* [index] Supported to log warning message which record causes posting
  list overflows.

* [:doc:`/reference/commands/load`][:doc:`/reference/commands/dump`]
  Supported Apache Arrow. [GitHub#691]

* [cmake] Supported linking lz4 in embedded static library build.
  [Original patch by Sergei Golubchik]

* [:doc:`/reference/commands/delete`] Supported to cancel.

* [httpd] Updated bundled nginx to 1.13.0

* Exported the following API

  * grn_plugin_proc_get_caller()

* Added index column related function and selector.

  * Added new selector: index_column_df_ratio_between()

  * Added new function: index_column_df_ratio()

Fixes
^^^^^

* [:doc:`/reference/commands/delete`] Fixed a bug that error isn't
  cleared correctly. It affects to following deletions so that it
  causes unexpected behavior.

* [windows] Fixed a bug that IO version is not detected correctly when the
  file is opened with ``O_CREAT`` flag.

* [:doc:`/reference/functions/vector_slice`] Fixed a bug that non 4
  bytes vector columns can't slice. [GitHub#695] [Patch by Naoya
  Murakami]

* Fixed a bug that non 4 bytes fixed vector column can't sequential
  match by specifying index of vector. [GitHub#696] [Patch by Naoya
  Murakami]

* [:doc:`/reference/commands/logical_select`] Fixed a bug that
  "argument out of range" occurs when setting last day of month to the
  min. [GitHub#698]

Thanks
^^^^^^

* Sergei Golubchik

* Naoya Murakami

.. _release-7-0-2:

Release 7.0.2 - 2017-04-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/logical_select`] Supported multiple
  :ref:`logical-select-drilldowns-label-columns-name-window-sort-keys`
  and
  :ref:`logical-select-drilldowns-label-columns-name-window-group-keys`.

* [windows] Updated bundled LZ4 to 1.7.5.

* [cache] Supported persistent cache feature.

* [:doc:`/reference/commands/log_level`] Update English documentation.

* Added the following APIs:

  * ``grn_set_default_cache_base_path()``
  * ``grn_get_default_cache_base_path()``
  * ``grn_persistent_cache_open()``
  * ``grn_cache_default_open()``

* [:option:`groonga --cache-base-path`] Added a new option to use
  persistent cache.

* [:doc:`/reference/executables/groonga-httpd`]
  [:ref:`groonga-httpd-groonga-cache-base-path`] Added new
  configuration to use persistent cache.

* [windows] Updated bundled msgpack to 2.1.1.

* [:doc:`/reference/commands/object_inspect`] Supported not only
  column inspection, but also index column statistics.

* Supported index search for "``.*``" regexp pattern.  This feature is
  enabled by default. Set
  ``GRN_SCAN_INFO_REGEXP_DOT_ASTERISK_ENABLE=no`` environment variable
  to disable this feature.

* [:doc:`/reference/functions/in_records`] Added function to use an
  existing table as condition patterns.

* [:doc:`/install/ubuntu`] Dropped Ubuntu 12.04 (Precise Pangolin)
  support because of EOL.

Fixes
^^^^^

* [:doc:`/reference/commands/logical_select`] Fixed a bug that wrong
  cache is used. This bug was occurred when dynamic column parameter
  is used.

* [:doc:`/reference/commands/logical_select`] Fixed a bug that dynamic
  columns aren't created. It's occurred when no match case.

* [:doc:`/reference/commands/reindex`] Fixed a bug that data is lost
  by reindex. [GitHub#646]

* [httpd] Fixed a bug that response of :doc:`/reference/commands/quit`
  and :doc:`/reference/commands/shutdown` is broken JSON when worker is
  running as another user. [GitHub ranguba/groonga-client#12]

.. _release-7-0-1:

Release 7.0.1 - 2017-03-29
--------------------------

Improvements
^^^^^^^^^^^^

* Exported the following API

  * grn_ii_cursor_next_pos()
  * grn_table_apply_expr()
  * grn_obj_is_data_column()
  * grn_obj_is_expr()
  * grn_obj_is_scalar_column()

* [:doc:`/reference/commands/dump`] Supported to dump weight reference
  vector.

* [:doc:`/reference/commands/load`] Supported to load
  ``array<object>`` style weight vector column. The example of
  ``array<object>`` style is: ``[{"key1": weight1}, {"key2":
  weight2}]``.

* Supported to search ``!(XXX OPERATOR VALUE)`` by index. Supported
  operator is not only ``>`` but also ``>=``, ``<``, ``<=``, ``==``
  and ``!=``.

* Supported index search for "!(column == CONSTANT)". The example in
  this case is: ``!(column == 29)`` and so on.

* Supported more "!" optimization in the following patterns.

  * ``!(column @ "X") && (column @ "Y")``
  * ``(column @ "Y") && !(column @ "X")``
  * ``(column @ "Y") &! !(column @ "X")``

* Supported to search ``XXX || !(column @ "xxx")`` by index.

* [:doc:`/reference/commands/dump`] Changed to use ``'{"x": 1, "y":
  2}'`` style for not referenced weight vector. This change doesn't
  affect to old Groonga because it already supports one.

* [experimental] Supported ``GRN_ORDER_BY_ESTIMATED_SIZE_ENABLE``
  environment variable. This variable controls whether query
  optimization which is based on estimated size is applied or not.
  This feature is disabled by default. Set
  ``GRN_ORDER_BY_ESTIMATED_SIZE_ENABLE=yes`` if you want to try it.

* [:doc:`/reference/commands/select`] Added query log for ``columns``,
  ``drilldown`` evaluation.

* [:doc:`/reference/commands/select`] Changed query log format for
  ``drilldown``. This is backward incompatible change, but it only
  affects users who convert query log by own programs.

* [:doc:`/reference/commands/table_remove`] Reduced temporary memory
  usage. It's enabled when the number of max threads is 0.

* [:doc:`/reference/commands/select`] ``columns[LABEL](N)`` is used
  for query log format instead of ``columns(N)[LABEL]``..

* [:doc:`/tutorial/query_expansion`] Updated example to use vector
  column because it is recommended way. [Reported by Gurunavi, Inc]

* Supported to detect canceled request while locking. It fixes the
  problem that ``request_cancel`` is ignored unexpectedly while locking.

* [:doc:`/reference/commands/logical_select`] Supported ``initial``
  and ``filtered`` stage dynamic columns. The examples are:
  ``--columns[LABEL].stage initial`` or ``--columns[LABEL].stage
  filtered``.

* [:doc:`/reference/commands/logical_select`] Supported
  ``match_columns``, ``query`` and ``drilldown_filter`` option.

* [:doc:`/reference/functions/highlight_html`] Supported similar
  search.

* [:doc:`/reference/commands/logical_select`] Supported ``initial`` 
  and stage dynamic columns in labeled drilldown. The example is: 
  ``--drilldowns[LABEL].stage initial``.

* [:doc:`/reference/commands/logical_select`] Supported window
  function in dynamic column.

* [:doc:`/reference/commands/select`] Added documentation about
  dynamic columns.

* [:doc:`/reference/window_function`] Added section about window
  functions.

* [:doc:`/install/centos`] Dropped CentOS 5 support because of EOL.

* [httpd] Updated bundled nginx to 1.11.12

* Supported to disable AND match optimization by environment variable.
  You can disable this feature by
  ``GRN_TABLE_SELECT_AND_MIN_SKIP_ENABLE=no``. This feature is enable
  by default.

* [:doc:`/reference/functions/vector_new`] Added a new function to
  create a new vector.

* [:doc:`/reference/commands/select`] Added documentation about
  ``drilldown_filter``.

Fixes
^^^^^

* [:doc:`/reference/commands/lock_clear`] Fixed a crash bug against
  temporary database.

* Fixed a problem that dynamically updated index size was increased
  for natural language since Grooonga 6.1.4.

* [:doc:`/reference/commands/select`] Fixed a bug that "A && B.C @ X"
  may not return records that should be matched.

* Fixed a conflict with ``grn_io_flush()`` and
  ``grn_io_expire()``. Without this change, if ``io_flush`` and ``load``
  command are executed simultaneously in specific timing, it causes a
  crash bug by access violation.

* [:doc:`/reference/commands/logical_table_remove`] Fixed a crash bug
  when the max number of threads is 1.

Thanks
^^^^^^

* Gurunavi, Inc.
  
.. _release-7-0-0:

Release 7.0.0 - 2017-02-09
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/functions/in_values`] Supported sequential search
  for reference vector column. [Patch by Naoya Murakami] [GitHub#629]

* [:doc:`/reference/commands/select`] Changed to report error instead
  of ignoring on invalid ``drilldown[LABEL].sort_keys``.

* [:doc:`/reference/commands/select`] Removed needless metadata
  updates on DB. It reduces the case that database lock remains
  even though ``select`` command is executed. [Reported by aomi-n]

* [:doc:`/reference/commands/lock_clear`] Changed to clear metadata lock
  by lock_clear against DB.

* [:doc:`/install/centos`] Enabled EPEL by default to install Groonga
  on Amazon Linux.

* [:doc:`/reference/functions/query`] Supported "@X" style in script
  syntax for prefix("@^"), suffix("@$"), regexp("@^") search.

* [:doc:`/reference/functions/query`] Added documentation about
  available list of mode. The default mode is ``MATCH`` ("@") mode
  which executes full text search.

* [rpm][centos] Supported groonga-token-filter-stem package which
  provides stemming feature by ``TokenFilterStem`` token filter on
  CentOS 7. [GitHub#633] [Reported by Tim Bellefleur]

* [:doc:`/reference/window_functions/window_record_number`] Marked
  ``record_number`` as deprecated. Use ``window_record_number``
  instead. ``record_number`` is still available for backward
  compatibility.

* [:doc:`/reference/window_functions/window_sum`] Added ``window_sum``
  window function. It's similar behavior to window function sum() on
  PostgreSQL.

* Supported to construct offline indexing with in-memory (temporary)
  ``TABLE_DAT_KEY`` table. [GitHub#623] [Reported by Naoya Murakami]

* [onigmo] Updated bundled Onigmo to 6.1.1.

* Supported ``columns[LABEL].window.group_keys``. It's used to apply
  window function for every group.

* [:doc:`/reference/commands/load`] Supported to report error on
  invalid key. It enables you to detect mismatch type of key.

* [:doc:`/reference/commands/load`] Supported ``--output_errors yes``
  option. If you specify "yes", you can get errors for each load
  failed record. Note that this feature requires command version 3.

* [:doc:`/reference/commands/load`] Improve error message on table key
  cast failure. Instead of "cast failed", type of table key and target
  type of table key are also contained in error message.

* [httpd] Updated bundled nginx to 1.11.9.

Fixes
^^^^^

* Fixed a bug that nonexistent sort keys for ``drilldowns[LABEL]`` or
  ``slices[LABEL]`` causes invalid JSON parse error. [Patch by Naoya
  Murakami] [GitHub#627]

* Fixed a bug that access to nonexistent sub records for group causes
  a crash.  For example, This bug affects the case when you use
  ``drilldowns[LABEL].sort_keys _sum`` without specifying
  ``calc_types``.  [Patch by Naoya Murakami] [GitHub#625]

* Fixed a crash bug when tokenizer has an error. It's caused when
  tokenizer and token filter are registered and tokenizer has an
  error.

* [:doc:`/reference/window_functions/window_record_number`] Fixed a
  bug that arguments for window function is not correctly
  passed. [GitHub#634][Patch by Naoya Murakami]

Thanks
^^^^^^

* Naoya Murakami
* aomi-n

The old releases
----------------

.. toctree::
   :maxdepth: 2

   news/6.x
   news/5.x
   news/4.x
   news/3.x
   news/2.x
   news/1.3.x
   news/1.2.x
   news/1.1.x
   news/1.0.x
   news/0.x
   news/senna
