.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

.. _release-6-0-4:

Release 6.0.4 - 2016-06-06
--------------------------

It's a bug fix release of 6.0.3. It's recommend that Groonga 6.0.3
users upgrade to 6.0.4. This release fixes some search related
problem.

Fixes
^^^^^

* [:doc:`/reference/commands/select`] Fixed a bug that ``--drilldown``
  against a temporary column created by ``--columns`` refers freed
  memory.

* Fixed a bug that search with large index may refer invalid data. It
  takes a long time to return search result.
  [GitHub#548][groonga-dev,04028][Reported by Atsushi Shinoda]

Thanks
^^^^^^

* Atsushi Shinoda

.. _release-6-0-3:

Release 6.0.3 - 2016-05-29
--------------------------

Improvements
^^^^^^^^^^^^

* [experimental] Added ``GRN_II_OVERLAP_TOKEN_SKIP_ENABLE`` and
  ``GRN_NGRAM_TOKENIZER_REMOVE_BLANK_DISABLE`` environment variables
  to improve performance of N-gram tokenizer.  [GitHub#533][Patch by
  Naoya Murakami]

* [:doc:`/reference/commands/table_create`] Stopped to ignore
  nonexistent default tokenizer, normalizer or token filters. In the
  previous versions, Groonga ignored a typo in
  ``--default_tokenizer``, ``--normalizer`` or ``--token_filters``
  parameter silently. It caused a delay in finding problems.

* [:doc:`/reference/commands/select`] output_columns v1: Supported
  expression such as ``snippet_html(...)`` in output_columns.

* [:doc:`/reference/commands/select`] Removed a limitation about the
  number of labeled drilldowns. In the previous versions, the number
  of max labeled drilldowns is limited to 10.

* [:doc:`/reference/functions/number_classify`] Added a number
  plugin. Use ``number_classify`` function to classify by value.

* Added a time plugin. Use ``time_classify_second``,
  ``time_classify_minute``, ``time_classify_hour``,
  ``time_classify_day``, ``time_classify_week``,
  ``time_classify_month``, ``time_classify_year`` function to classify
  by value.

* [:doc:`/reference/commands/select`] Supported dynamic column
  creation which is used in ``output_columns``, ``drilldown`` or
  ``sortby`` [GitHub#539,#541,#542,#544,#545][Patch by Naoya Murakami]::

    select \
      --columns[LABEL].stage filtered \
      --columns[LABEL].type ShortText \
      --columns[LABEL].flags COLUMN_SCALAR \
      --columns[LABEL].value 'script syntax expression' \
      ...

* [experimental][:doc:`/reference/commands/select`] Improved
  performance for range/equal search and enough filtered case. Set
  ``GRN_TABLE_SELECT_ENOUGH_FILTERED_RATIO`` environment variable to
  enable this feature.

* [:doc:`/reference/commands/select`] Supported index used search for
  filtered tables.

* Supported to detect changed database isn't closed. This feature is
  useful to check database corruption when Groonga is crashed
  unexpectedly.

* [:doc:`/reference/executables/grndb`] Supported detecting database
  wasn't closed successfully case.

* Added ``--drilldown_filter``.

* Supported ``filter`` in labeled drilldown.

* Improved performance for using [:doc:`/reference/functions/between`]
  without index. By between() optimization, there is a case that range
  search is 100x faster than the previous version of between().

* [:doc:`/reference/functions/record_number`] Supported window function.

* [experimental][:doc:`/reference/commands/select`] Supported ``--slices``.

* [:doc:`/reference/commands/select`] Deprecated ``--sortby`` and
  ``--drilldown_sortby``. Use ``--sort_keys`` and
  ``-drilldown_sort_keys`` instead.

* [:doc:`/reference/commands/select`] Deprecated ``--drilldown[...]``.
  Use ``--drilldowns[...]`` instead.

* Added [:doc:`/reference/command/command_version`] 3. It uses object
  literal based envelope.

* [groonga-httpd] Updated bundled nginx version to 1.11.0.

Fixes
^^^^^

* [:doc:`/reference/commands/select`] output_columns v2: Fixed a bug
  that ``*`` isn't expand to columns correctly.

* Fixed a bug that 1usec information is lost for time value.

* Fixed a crash bug when a mruby plugin is initialized with multiple
  threads.

* Fixed a bug that static indexing crashes if a posting list is very long.
  This bug may occur against enormous size of database. [GitHub#546]

Thanks
^^^^^^

* Naoya Murakami

.. _release-6-0-2:

Release 6.0.2 - 2016-04-29
--------------------------

Improvements
^^^^^^^^^^^^

* Supported bool in comparison operators (``>``, ``>=``, ``<``, ``<=``).
  TRUE is casted to 1. FALSE is casted to 0. Thus you specify function
  which returns boolean value in comparison.

* [groonga-http][:doc:`/reference/command/request_timeout`] Supported
  ``request_timeout`` parameter. Canceled request returns
  ``HTTP/1.1 408 Request Timeout`` status code.

* [:doc:`/reference/commands/table_tokenize`] Added ``index_column`` option.
  [GitHub#534] [Patch by Naoya Murakami]

* [:doc:`/reference/commands/table_tokenize`] Supported to output ``estimated_size``.
  [GitHub#518] [Patch by Naoya Murakami]

* [geo_in_rectangle] Supported to work without index. In this case, sequential search
  is executed as a fallback.

* Reduced needless internal loops. It improves phrase search performance.
  [GitHub#519] [Patch by Naoya Murakami]

* [:doc:`/contribution`] Updated documentation about contribution.
  [GitHub#522] [Patch by Hiroshi Ohkubo]

* [:doc:`/reference/command/return_code`] Updated documentation about return code list.

* [:doc:`/reference/executables/groonga`] Added ``--default-request-timeout`` option.

* [windows] Supported DLL version.

* Supported index used search even if value only term exists.
  For example, ``true || column > 0`` doesn't use index even if ``column`` has
  index. In this release, above issue is resolved.

* [:doc:`/reference/commands/select`] Supported specifying grouped table
  [GitHub#524,#526,#527,#528,#529] [Patch by Naoya Murakami]

* Supported grouping by ``Int{8,16,64}/UInt{8,16,64}`` value.
  In the previous versions, only 32bit fixed size value was supported.

* Added table name to error message for invalid sort key.

* [:doc:`/reference/executables/groonga-suggest-httpd`] Updated documentation.

* [:doc:`/reference/suggest/completion`] Fixed a typo about example.
  [groonga-dev,04008] [Reported by Tachikawa Hiroaki]

* [:doc:`/reference/executables/grndb`] Added a workaround to keep
  backward compatibility. use ``object_inspect`` instead of ``inspect``.

* [groonga-httpd] Updated bundled nginx version to 1.9.15.

* [centos] Supported systemd.

* [doc] Supported only HTML output. [GitHub#532] [Patch by Hiroshi Ohkubo]

* [:doc:`/reference/executables/groonga-httpd`][centos] Supported to customize
  environment variables.

* [:doc:`/install/others`] Updated documentation about ``--with-package-platorm`` option.

* [ubuntu] Supported Ubuntu 16.04 (Xenial Xerus)

Fixes
^^^^^

* Fixed a bug that tokenization of zero-length values are failed.
  For example, if ``description`` column is indexed column, tokenizer reports an error.
  [GitHub#508] [Reported by Naoya Murakami]::

    load --table docs
    [
    ["_key","description"],
    [2,""]
    ]

* Fixed a crash bug because of invalid critical section handling. [GitHub#507]

* [:doc:`/contribution/development/release`] Fixed a typo about grntest howto.
  [GitHub#511] [Patch by Hiroshi Ohkubo]

* [doc] Removed man support.

* Removed invalid debug log messages which make user confused.

* Fixed a bug that data is not correctly flushed because internal counter is wrongly cleared.
  In the previous version, when size of data exceeds specific one, it was failed to create indexes.
  [GitHub#517] [Reported by Naoya Murakami]

* Fixed a bug that a process can't use more than one caches in parallel.
  [GitHub#515]

* Fixed a bug that internally used ``alloc_info`` structure which is used to find memory leaks is
  not exclusively accessed. Without this fix, it may causes a crash. [GitHub#523]

* [tokenizer mecab] Fixed a memory leak on dictionary encoding mismatch error.
  [groonga-dev,04012] [Reported by Naoya Murakami]

* Fixed a bug that combination of [:doc:`/reference/executables/groonga-suggest-httpd`] and
  [:doc:`/reference/executables/groonga-suggest-learner`] didn't work.

* [doc] Removed needless uuid from \*.po [GitHub#531] [Patch by Hiroshi Ohkubo]

* [:doc:`/reference/functions/highlight_html`] Fixed a bug that duplicated text
  is returned. This bug occurs when highlighted keyword occurred 1024 or more times.

* Fixed a bug that ``KEY_LARGE`` conflicts with existing flag.
  If you use ``TABLE_HASH_KEY|KEY_LARGE`` in the previous version,
  there is a possibility to break database. Please recreate the table.

Thanks
^^^^^^

* Naoya Murakami
* Hiroshi Ohkubo
* Tachikawa Hiroaki

.. _release-6-0-1:

Release 6.0.1 - 2016-03-29
--------------------------

Improvements
^^^^^^^^^^^^

* [mruby] Updated bundled mruby to fix a crash bug related to GC and
  backtrace.

* Exported the following API

  * grn_expr_take_obj()
  * grn_request_canceler_cancel_all()
  * grn_obj_remove_dependent()
  * grn_obj_is_text_family_type()

* [hash] Supported 4GiB over total key size when ``KEY_LARGE`` flag is set
  to a table::

    table_create Users TABLE_HASH_KEY|KEY_LARGE ShortText

* [:doc:`/reference/commands/load`] Supported
  :doc:`/reference/command/request_id` when you specify input data as
  raw JSON instead of parameter value::

    POST /d/load?table=XXX&request_id=x
    
    load --table XXX --request_id x
    [
      ...
    ]

* [:doc:`/reference/commands/shutdown`] Added ``mode`` argument to
  shutdown immediately. Use ``shutdown --mode immediate`` in such a purpose.

* [:doc:`/install/mac_os_x`] Added a instruction to setup MeCab dictionary for Homebrew.

* [:doc:`/reference/commands/load`] Supported to stop load when cancel
  is requested.

* [:doc:`/reference/commands/table_remove`] Supported to remove
  dependent tables. Use ``--dependent yes`` for it.

* [:doc:`/reference/commands/logical_table_remove`] Supported to
  remove dependent tables.

* [Windows] Supported memory debug mode on Windows.

* Supported to dump allocation information by status on memory debug mode

* [:doc:`/contribution/documentation/i18n`] Added installation step for Sphinx.

* [experimental] Supported to split chunks in static indexing.
  Use ``GRN_INDEX_CHUNK_SPLIT_ENABLE=yes`` to enable it. [GitHub#479]

Fixes
^^^^^

* [:doc:`/reference/commands/load`] Fixed a crash bug when
  nonexistent column is specified. [GitHub#493]

* [:doc:`/reference/commands/load`] Fixed a bug that load command does
  not return error code correctly. [GitHub#495]

* [:doc:`/reference/commands/load`] Fixed a memory leak when parsing
  columns parameter in load command.

* [:doc:`/reference/commands/load`] Fixed a bug that only the first
  array in ``--values`` is handled as a list of column names if ``--columns`` is not
  specified. [GitHub#497]

* [:doc:`/reference/commands/load`] Fixed to check ``--columns`` more precisely
  [GitHub#496]

* Fixed a insufficient critical section handling for
  thread-safety.

* [:doc:`/reference/commands/column_create`] Fixed a crash bug when
  failed to create a column.

* [:doc:`/reference/commands/table_remove`] Fixed a crash bug to
  remove nonexistent table. [GitHub#504]

* Fixed a bug that offline index construction against ``WITH_POSITION`` +
  non-text ``VECTOR`` column ignores position.

* [:doc:`/reference/executables/grndb`] Fixed a bug that cycle
  reference causes stack over flow.

* [deb] Dropped support for Ubuntu 15.04 (Vivid Vervet)

Thanks
^^^^^^

* YUKI Hiroshi

.. _release-6-0-0:

Release 6.0.0 - 2016-02-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/executables/grndb`] Supported check against table
  specified as table domain in ``--target`` mode.

* [``grn_pat_fuzzy_search()``] Added a new API that provides fuzzy
  search feature by patricia trie. [GitHub#460][Patch by Naoya
  Murakami]

* [``functions/string``] Added a new plugin that provides string
  related functions. [GitHub#459][Patch by Naoya Murakami]

* [``string_length()``] Added a new function that returns the number
  of characters in the specified string. It's contained in
  ``functions/string`` plugin. [GitHub#459][Patch by Naoya Murakami]

* [``grn_table_fuzzy_search()``] Added a new DB API that provides
  fuzzy search feature. [GitHub#463][Patch by Naoya Murakami]

* [``GRN_OP_FUZZY``] Added a new operator. [GitHub#463][Patch by Naoya
  Murakami]

* [``grn_obj_search()``] Supported ``GRN_OP_FUZZY``
  operator. [GitHub#463][Patch by Naoya Murakami]

* [``GRN_TABLE_FUZZY_SEARCH_WITH_TRANSPOSITION``] Added a flag for
  ``grn_table_fuzzy_search()``. [GitHub#463][Patch by Naoya Murakami]

* [``GRN_TOKENIZE_ONLY``] Added a new tokenization mode that returns
  all tokens even if the token doesn't exist in
  lexicon. [GitHub#463][Patch by Naoya Murakami]

* [``grn_obj_type_to_string()``] Add a new function that stringify
  type ID such as ``GRN_VOID`` and ``GRN_BULK``.

* [:doc:`/reference/commands/object_inspect`] Added a new command that
  returns information of the target object.

* Supported compare operations against vector. If left hand side
  vector includes any element that satisfies ``left_hand_side_element
  OP right_hand_side``, it returns true.

  Example::

    [1, 2, 3] < 2 # -> true because 1 is less than 2
    [1, 2, 3] > 4 # -> false because all elements are less than 4

* [``fuzzy_search()``] Added a new function that provides fuzzy search
  feature. [GitHub#464][Patch by Naoya Murakami]

* [:doc:`/reference/functions/edit_distance`] Supported transposition
  flag. [GitHub#464][Patch by Naoya Murakami]

* Supported index search for ``vector_column[2] == 29``.

* [``GRN_PLUGIN_CALLOC()``] Added a new API for plugin that provides
  ``calloc()`` feature. [GitHub#472][Patch by Naoya Murakami]

* Supported index search for compare operations against vector element
  such as ``vector_column[2] < 29``.

* [``grn_plugin_proc_get_var_bool()``] Add a new API for plugin that
  provides getting boolean argument value feature.

* [``grn_plugin_proc_get_var_int32()``] Add a new API for plugin that
  provides getting 32bit integer argument value feature.

* [``grn_plugin_proc_get_var_string()``] Add a new API for plugin that
  provides getting string argument value feature.

* [:doc:`/reference/commands/object_remove`] Added a new command that
  removes an object. ``object_remove`` can also remove a broken object.

* Supported mips/mipsel. [debian-bugs:770243][Reported by Nobuhiro
  Iwamatsu][Reported by Thorsten Glaser][Reported by YunQiang
  Su][Reported by Dejan Latinovic][Reported by Steve Langasek]

* [:doc:`/reference/executables/grndb`][CMake] Supported.

* [``grn_expr_syntax_expand_query()``] Added a new API that provides
  query expansion feature.

* [``snippet()``] Add a new function that provides snippet feature.
  [GitHub#481][Patch by Naoya Murakami]

* [``highlight()``] Add a new function that provides highlight feature.
  [GitHub#487][Patch by Naoya Murakami]

* Added ``XXX && column != xxx`` optimization. It's converted to ``XXX
  &! column == xxx`` internally.

* [:doc:`/server/memcached`] Added ``--memcached-column``. You can
  access existing column by memcached protocol.

* [:doc:`/reference/executables/groonga-httpd`] Supported TLS.
  [groonga-dev,03948][Reported by KITAITI Makoto]

* [:doc:`/reference/executables/groonga-httpd`] Updated bundled nginx
  version to 1.9.11 from 1.9.10.

* [Windows][CMake] Supported LZ4. LZ4 is bundled.

Fixes
^^^^^

* [:doc:`/reference/commands/select`] Added a missing error check for
  outputting column. [GitHub#332][Reported by Masafumi Yokoyama]

* Fixed a bug that ``function(column_with_index) == 29`` ignores
  ``function()``. [groonga-dev,03884][Reported by Naoya Murakami]

* [:doc:`/reference/commands/reindex`] Fixed a bug that ``reindex``
  doesn't clear query cache.

* [patricia trie] Fixed a bug that sorting by integer patricia trie
  key returns unsorted result. [GitHub#476][Reported by Ryunosuke SATO]

* [:doc:`/reference/commands/select`] Fixed a crash bug that is
  occurred when too many keywords is specified into ``--query``.
  [GitHub#484][Reported by Hiroyuki Sato]

* [:doc:`/reference/commands/select`] Fixed a bug that wrong cache is
  used when :doc:`/reference/command/command_version` or
  :doc:`/reference/command/pretty_print` is
  used. [GitHub#490][Reported by KITAITI Makoto]

Thanks
^^^^^^

* Masafumi Yokoyama

* Naoya Murakami

* Nobuhiro Iwamatsu

* Thorsten Glaser

* YunQiang Su

* Dejan Latinovic

* Steve Langasek

* Ryunosuke SATO

* Hiroyuki Sato

* KITAITI Makoto

.. _release-5-1-2:

Release 5.1.2 - 2016-01-29
--------------------------

Improvements
^^^^^^^^^^^^

* Improved performance for sequential search against constant value
  such as ``true`` and ``29``.

* Improved performance for sequential search against binary operation
  with constant value such as ``x == 29`` and ``x < 29``.

* [:doc:`/reference/commands/select`] Changed score type to ``Float``
  from ``Int32`` when :doc:`/reference/command/command_version` is 2.
  The current default command version is 1. Command version 2 is
  experimental. So this change isn't affected to normal users.
  [GitHub#450][Patch by Naoya Murakami]

* [grn_ts] Supported match operator.

* [:doc:`/reference/executables/grndb`] Added ``--target`` option to
  ``check`` command. It reduces check target.

* [Windows] Updated bundled msgpack to 1.3.0 from 1.0.1.

* [Windows] Updated bundled MeCab to 0.996 from 0.98.

* [``grn_hash_size()``] Added a new API that returns the number of
  records in the hash table.

* [``GRN_HASH_TINY``] Added a new flag to create tiny hash table.

* [``grn_dump_table_create_flags()``] Added a new API that converts
  ``flags`` value of a table to ``flags`` parameter format of
  :doc:`/reference/commands/table_create`.

* [``grn_dump_column_create_flags()``] Added a new API that converts
  ``flags`` value of a column to ``flags`` parameter format of
  :doc:`/reference/commands/column_create`.

* [``grn_plugin_get_names()``] Added a new API that returns all plugin
  names in a database.

* [``grn_column_get_all_index_data()``] Added a new API that returns
  all index data for a table or data column.

* [:doc:`/reference/commands/schema`] Added indexes information to
  indexed tables and data columns.

* [``grn_config_get()``] Renamed from ``grn_conf_get()``.

* [``grn_config_set()``] Renamed from ``grn_conf_set()``.

* [``grn_config_delete()``] Added a new API that deletes a
  configuration item.

* [:doc:`/reference/commands/config_set`] Added a new command that
  sets a configuration item.

* [:doc:`/reference/commands/config_get`] Added a new command that
  gets a configuration item value.

* [:doc:`/reference/commands/config_delete`] Added a new command that
  deletes a configuration item.

* [``grn_config_cursor_open()``] Added a new API that opens a new
  cursor that iterates all configuration items.

* [``grn_config_cursor_next()``] Added a new API that moves to the
  next configuration item in the cursor.

* [``grn_config_cursor_get_key()``] Added a new API that gets the
  key of the current configuration item.

* [``grn_config_cursor_get_value()``] Added a new API that gets the
  value of the current configuration item.

* [:doc:`/reference/alias`] Supported aliasing table and column names.

* [hash table] Added total key size overflow check.

* [:doc:`/reference/commands/dump`] Supported dumping configurations
  set by :doc:`/reference/commands/config_set`.

* [patricia trie] Improved inspection.
  [GitHub#452][GitHub#457][Patch by Naoya Murakami]

* [``grn_get_global_error_message()``] Added a new API that gets the
  current error message in the process.

* [:doc:`/reference/commands/lock_acquire`] Added a new command that
  acquires a lock of a database, table or column.

* [:doc:`/reference/commands/lock_release`] Added a new command that
  releases a lock of a database, table or column.

* [:doc:`/reference/executables/groonga-httpd`] Updated bundled nginx
  version to 1.9.10 from 1.9.7.

Fixes
^^^^^

* Fixed mruby related crash bugs.

* [Windows] Fixed label in installer.
  [groonga-dev,03825][Reported by Atsushi Shinoda]

* [doc] Fixed typos.
  [GitHub#456][GitHub#458][Patch by tSU_RooT]

* [:doc:`/server/memcached`] Added more description.
  [GitHub#454][Patch by Hiroyuki Sato]

* Fixed a bug that :doc:`/reference/command/command_version` specified
  by ``command_version`` parameter in a request isn't reset.

Thanks
^^^^^^

* Naoya Murakami

* Atsushi Shinoda

* tSU_RooT

* Hiroyuki Sato

.. _release-5-1-1:

Release 5.1.1 - 2015-12-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/grn_expr/script_syntax`] Supported ``INDEX_COLUMN
  == VALUE`` as index search with an equal supported index.

* Reduced the continuous same messages from inverted index module.

* Supported detecting truncated tables by another process.
  [GitHub#447] [groonga-dev,03761] [Reported by Yutaro SHIMAMURA]

* [:c:func:`grn_db_recover()`] Supported detecting a broken object.

* Improved performance for sequential scan search by
  :doc:`/reference/regular_expression` match with constant pattern
  such as ``COLUMN @~ "CONSTANT_PATTERN"``.

Fixes
^^^^^

* Fixed build error on MessagePack support build.
  [groonga-dev,03708] [Reported by Ryo Sugimoto]

* [mruby] Fixed a crash bug on error.
  mruby exception feature is disabled as workaround for now.

* [:doc:`/reference/commands/thread_limit`] Fixed a bug that
  ``thread_limit?max=1`` may not reduce the number of current running
  threads to ``1``.

* [:doc:`/reference/commands/thread_limit`] Fixed a bug that
  ``thread_limit?max=1`` may not return.

* [:doc:`/reference/tuning`] Fixed wrong ``sysctl`` argument.
  [GitHub#448] [Reported by Hiroyuki Sato]

Thanks
^^^^^^

* Ryo Sugimoto
* Yutaro SHIMAMURA
* Hiroyuki Sato

.. _release-5-1-0:

Release 5.1.0 - 2015-11-29
--------------------------

Improvements
^^^^^^^^^^^^

* [patiricia trie] Added an error check for the max total key
  size. See :doc:`/limitations` about the max total key size.

* [:doc:`/reference/executables/grndb`] Added a check for broken
  object. The check can detect a case that the object can't be opened.

* [``grn_obj_reindex()``] Added a new API that recreates existing
  indexes.

* [:doc:`/reference/commands/reindex`] Added a new command that
  recreates existing indexes.

* [inverted index] Improved estimation precision for query.

* [:doc:`/reference/commands/logical_range_filter`] Added fallback
  mode for sequential search. If sequential search checked many
  records but didn't find required the number of records, index search
  is used as fallback.

* [``grn_get_package_label()``] Added a new API that returns package
  label. It returns ``Groonga``.

* [:doc:`/reference/executables/groonga-server-http`] Added ``Server:
  Groonga/VERSION`` response header.

* [:doc:`/reference/executables/groonga-httpd`] Improved performance
  by reusing ``grn_ctx`` object.

* [``grn_file_reader``] Added a new API that provides ``fgets()``
  feature. It fixes a crash bug of
  :doc:`/reference/executables/groonga`. If
  :doc:`/reference/executables/groonga` is built with static C runtime
  by Visual Studio, the crash bug is occurred.

* [:doc:`/reference/functions/prefix_rk_search`] Added a new selector
  that provides prefix RK search feature.

* [``grn_obj_is_accessor()``] Added a new predicate that checks
  whether the object is an accessor.

* [``grn_obj_is_key_accessor()``] Added a new predicate that checks
  whether the object is an accessor for ``_key``
  :doc:`/reference/columns/pseudo`.

* Supported :doc:`/reference/command/pretty_print` for JSON output.

Fixes
^^^^^

* [inverted index] Fixed a possible infinite loop bug when log level
  is ``debug``.

* Fixed a bug that ``@`` operator (match operator) may not match
  record that should be matched in sequential search mode.

* [patricia trie] Fixed a bug that invalid value may be returned for
  empty string key. [groonga-dev,03632] [Reported by Naoya Murakami]

Thanks
^^^^^^

* Naoya Murakami

.. _release-5-0-9:

Release 5.0.9 - 2015-10-29
--------------------------

Improvements
^^^^^^^^^^^^

* [inverted index] Reduced log levels of logs for developers.

* Flushed pending changed on creating new database. It guards database
  from crash.

* [``grn_geo_table_sort()``] Added a new API that sorts table by
  geometry index.

* [experimental] Added expression rewrite mechanism. You can write
  custom expression rewriter by mruby. Expression rewriter can be used
  for optimizing an expression, changing conditions in an expression
  and so on.

* [experimental] Added database global configuration mechanism. You
  can put configurations (key and value pairs) into database. For
  example, it will be used in :ref:`token-filter-stop-word` to custom
  column name from ``is_stop_word``.

* [``grn_conf_set()``] Added a new API that sets a configuration.

* [``grn_conf_get()``] Added a new API that gets a configuration.

* [deb] Changed to ``all`` from ``any`` for
  ``Architecture`` value.
  [debian-bugs:799167][Reported by Matthias Klose]

* [Windows][CMake] Supported building bundled MeCab.
  [groonga-dev,03562][Reported by Sato]

* [:doc:`/reference/commands/schema`] Added a new command that returns
  schema. Schema is consists with loaded plugins, loaded tokenizers,
  loaded normalizers, loaded token filters, defined tables and defined
  columns.

* [:c:func:`grn_plugin_win32_base_dir()`] Deprecated. Use
  :c:func:`grn_plugin_windows_base_dir()` instead.

* [:c:func:`grn_plugin_windows_base_dir()`] Renamed from
  :c:func:`grn_plugin_win32_base_dir()`.

* [``grn_obj_is_type()``] Add a new API that returns true when the
  passed object is a type object.

* [``grn_obj_is_tokenizer_proc()``] Add a new API that returns true
  when the passed object is a tokenizer object.

* [``grn_obj_is_normalizer_proc()``] Add a new API that returns true
  when the passed object is a normalizer object.

* [``grn_obj_is_token_filter_proc()``] Add a new API that returns true
  when the passed object is a token filter object.

* [``grn_ctx_get_all_types()``] Add a new API that returns all type
  objects in database.

* [``grn_ctx_get_all_tokenizers()``] Add a new API that returns all
  tokenizer objects in database.

* [``grn_ctx_get_all_normalizers()``] Add a new API that returns all
  normalizer objects in database.

* [``grn_ctx_get_all_token_filters()``] Add a new API that returns all
  token filter objects in database.

* [``grn_ctx_output_uint64()``] Add a new API that outputs 64bit
  unsigned integer value.

* [``grn_ctx_output_null()``] Add a new API that outputs ``NULL``.

* [``GRN_OBJ_IS_TRUE()``] Add a new API that returns true when the
  passed object is true value.

* [experimental] Enabled grn_ts by default.

* [:doc:`/install/ubuntu`] Added Ubuntu 15.10 Wily Werewolf support.

Fixes
^^^^^

* [patricia trie] Fixed a bug that the number of records may be
  counted up unexpectedly on adding a new entry. [GitHub#417]

* [patricia trie] Fixed a bug that a variable may be used
  uninitialized.

* [patricia trie] Fixed a bug that ``grn_pat_cursor_next()`` may enter
  an infinite loop. [GitHub#419]

* [patricia trie] Fixed a bug that deleting an entry may break
  patricia trie.
  [GitHub#415][groonga-dev,03515][Reported by Hiroshi Kagami]

* [patricia trie] Fixed a bug that deleting a nonexistent entry may
  break patricia trie. [GitHub#420]

* Fixed a bug that wrong proc type is used for token filter objects.

Thanks
^^^^^^

* Matthias Klose
* Hiroshi Kagami
* Sato

.. _release-5-0-8:

Release 5.0.8 - 2015-09-29
--------------------------

Improvements
^^^^^^^^^^^^

* [Windows] Supported build with MySQL again.

* [:doc:`/reference/grn_expr/script_syntax`] Changed return value type
  to ``Bool`` from ``Int32`` for predicate operations such as
  :ref:`script-syntax-match-operator` and
  :ref:`script-syntax-equal-operator`.

* [:doc:`/reference/api`] Supported owning other ``grn_obj`` by
  ``GRN_PTR`` and ``GRN_PVECTOR`` bulk. If you specify ``GRN_OBJ_OWN``
  flag to ``GRN_PTR`` and ``GRN_PVECTOR`` bulks, they call
  :c:func:`grn_obj_close()` against ``grn_obj`` that is held by
  them when they are closed.

* [incompatible][:doc:`/reference/regular_expression`] Changed to
  normalize regular expression match target text before matching. It's
  for consistency and performance.

  Other operations such as :ref:`script-syntax-prefix-search-operator`
  normalize target text.

  Some simple regular expressions such as ``\Ahello`` can be
  evaluated by index. It's fast.

  If target text isn't normalized, you need to use complex regular
  expressions such as ``\A[Hh]ello`` and ``\A(?i)hello``. Complex
  regular expressions can't be evaluated by index. If target text is
  normalized, you can use simple regular expressions. They may be
  evaluated by index. It's fast.

* [doc] Improved documents.
  [GitHub#393][GitHub#396][GitHub#397][GitHub#399][GitHub#403]
  [GitHub#405][GitHub#409]
  [Reported by Hiroyuki Sato][Patch by Hiroyuki Sato]

* [:doc:`/reference/functions/highlight_html`] Improved performance.
  [groonga-dev,03427] [Reported by Hiroyuki Sato]

* [:doc:`/reference/functions/snippet_html`] Improved performance.

* [CMake] Stopped to run ``pkg-config`` twice.
  [Patch by Sergei Golubchik]

* Removed needless check for year in time. B.C. is always invalid time
  without this change. B.C. is valid time when system (``mktime()``)
  supports it with this change.

* [:c:func:`grn_ctx_is_opened()`] Added a new API that checks whether
  object with the ID is opened or not.

* [:c:func:`grn_obj_remove()`] Reduced the maximum memory usage. If
  :c:func:`grn_thread_get_limit()` returns ``1``, it closes temporary
  opened objects after it finished to use them.

* [doc][:doc:`/reference/commands/table_remove`] Updated with many
  descriptions.

* [:doc:`/reference/executables/groonga`] Ensured to add the last new
  line to :doc:`/reference/commands/dump` result on stand alone mode.

* [:ref:`process-log`] Added Groonga version into ``grn_init`` log.

* Opened `chat room on Gitter <https://gitter.im/groonga/public>`_.

* [:doc:`/reference/commands/status`] Added ``start_time`` as alias of
  ``starttime`` for consistency. Other keys use ``snake_case`` style.
  ``starttime`` is deprecated. Use ``start_time`` instead.

* Updated bundled Onigmo.

* [doc][:doc:`/reference/scorers/scorer_tf_at_most`] Documented.

* Supported columns for temporary table. It's only available C API
  users for now. :doc:`/reference/commands/select` will use this
  feature in the next release.

* [``grn_vector_pop_element()``] Exported.

* [:doc:`/reference/executables/groonga`] Added checks whether
  acquiring lock is succeeded or not.

* [:doc:`/reference/executables/groonga-suggest-create-dataset`]
  Changed to use ``--normalizer`` instead of ``KEY_NORMALIZE`` because
  ``KEY_NORMALIZE`` is deprecated.

* [``grn_obj_cast()``] Exported.

* [experimental][``grn_ii_cursor``] Exported.

* [experimental][``grn_ii_cursor_open()``] Exported.

* [experimental][``grn_ii_cursor_next()``] Exported.

* [experimental][``grn_ii_cursor_close()``] Exported.

* [:ref:`script-syntax-match-operator`] Improved index detection.
  Index its lexicon has a tokenizer is preferred.

* [:doc:`/reference/executables/groonga-httpd`] Updated bundled nginx
  to 1.9.5 that supports HTTP/2. HTTP/2 module is enabled.

Fixes
^^^^^

* [:doc:`/reference/grn_expr/script_syntax`] Fixed a bug that ``&!``
  does nothing when right hand side is ``true``.

* Fixed performance regression with libtool 2.4.6.
  [GitHub#406][GitHub#407] [Patch by Hiroyuki Sato]

* [:ref:`script-syntax-equal-operator`] Fixed a bug that section is
  ignored.

Thanks
^^^^^^

* Hiroyuki Sato
* Sergei Golubchik

.. _release-5-0-7:

Release 5.0.7 - 2015-08-31
--------------------------

This release includes a bug fix of :ref:`offline-index-construction`.

If you're using any multiple column index (index column with
``WITH_SECTION`` flag) and :ref:`offline-index-construction`, we
recommend that you upgrade your Groonga.

This release has an important experimental feature for Windows users.
See "sparse file support" entry in the following improvement list for
details.

Improvements
^^^^^^^^^^^^

* [experimental][Windows] Added sparse file support. It's experimental
  feature. It's disabled by default. You can enable it by specifying
  ``GRN_IO_USE_SPARSE=yes`` environment variable.

  It reduces database file size on Windows. Please try the feature and
  report the result. Groonga developers are interested in the
  followings:

  * Disk usage
  * Performance (Improved? Degraded? No difference?)
  * Memory usage (Especially virtual memory usage)

* [experimental][:doc:`/reference/commands/logical_shard_list`] Added
  a command that returns a shard list of the specified logical table.

* [experimental][:ref:`script-syntax-regular-expression-operator`]
  Supported regular expression match against vector column without
  index.

* [:doc:`/reference/commands/logical_range_filter`] Supported
  ``--cache no`` option. It's same as :ref:`select-cache` option in
  :doc:`/reference/commands/select`.

* [:doc:`/reference/executables/groonga-httpd`] Supported returning
  the max number of threads feature of
  :doc:`/reference/commands/thread_limit`. You can't set the max
  number of threads.

* [:c:func:`grn_db_unmap()`] Added a new API that unmaps all opened
  tables and columns. It's a thread unsafe operation. You can't touch
  the database while :c:func:`grn_db_unmap()` is running.

* [:doc:`/reference/commands/database_unmap`] Added a command that
  unmaps all opened tables and columns in database.

* [:doc:`/reference/commands/object_exist`] Added a command that
  checks whether object with the specified name exists or not in
  database.

* [:doc:`/reference/commands/column_copy`] Added a command that copies
  all values from source column to destination column.

  You can use this command to change column value type, column type,
  table type and so on.

* Stopped to use non-standard ``__uint32_t``
  type. [GitHub#375][Reported by Natanael Copa]

* [experimental][Windows] Supported Windows Event log.

* [mruby] Supported error handling on mruby initialization error.

* [experimental][:doc:`/reference/commands/thread_limit`] Renamed from
  ``thread_count``.

* Supported logging used indexes in ``info`` level and ``debug``
  level. It can be used like ``EXPLAIN`` in RDBMS. It's useful to
  improve slow query.

* [doc] Replaced deprecated ``KEY_NORMALIZE`` flags.
  [GitHub#378][GitHub#380][GitHub#382] [Patch by Hiroyuki Sato]

* [doc] Removed needless Sphinx configurations.
  [GitHub#379] [Patch by Ayumu Osanai]

* [experimental][incompatible][:ref:`script-syntax-regular-expression-operator`]
  Changed ``.`` match behavior. ``.`` matches new line. It's backward
  incompatible change.

* [doc][:doc:`/contribution/development/build`] Added a document about
  building Groonga as Groonga developer.
  [GitHub#353] [Suggested by Hiro Yoshioka]

Fixes
^^^^^

* [mruby] Fixed a time overflow bug.

* [:doc:`/reference/executables/groonga`] Fixed a crash bug when
  PID file can't be created. [GitHub#368] [Reported by Hiroyuki Sato]

* Fixed a bug that :ref:`offline-index-construction` may generate
  broken index. It may be caused for multiple column index. In other
  words, index column with ``WITH_SECTION`` flag may be broken.

  If you're using :ref:`online-index-construction` for index columns
  with ``WITH_SECTION`` flag, this bug isn't affected.

  You can recover this bug by recreating existing multiple column
  indexes.

* [:doc:`/reference/functions/query`] Fixed a crash bug when
  :doc:`/reference/functions/query` is used in :ref:`select-scorer`.

* [:ref:`select-filter`] Fixed a bug that
  :ref:`script-syntax-bitwise-not` against unsigned int value doesn't
  work for comparing to ``-NUMBER_LITERAL``.

  For example, the following expression doesn't work::

    ~UINT32_COLUMN == -6

* Fixed a bug that :ref:`script-syntax-regular-expression-operator`
  doesn't work in multithread.

* Fixed some memory leaks.

* Fixed a build error. [GitHub#381] [Patch by Hiroshi Hatake]

Thanks
^^^^^^

* Hiroyuki Sato
* Natanael Copa
* Ayumu Osanai
* Hiroshi Hatake
* Hiro Yoshioka

.. _release-5-0-6:

Release 5.0.6 - 2015-07-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/install/ubuntu`] Dropped Ubuntu 14.10 (Utopic Unicorn) support. It had been
  End of Life on July 23, 2015.
* Supported offline index construction for reference vector. For example, ``load``
  data before ``column_create`` an index::

    table_create Entries TABLE_NO_KEY
    column_create Entries numbers COLUMN_VECTOR Int32

    load --table Entries
    [
    ["numbers"],
    [[18, 19, 20]],
    [[100, 200]]
    ]

    table_create Numbers TABLE_PAT_KEY Int32
    column_create Numbers entries_numbers COLUMN_INDEX Entries numbers

    select Numbers --output_columns _key

* Supported ``'vector_text_column @ "element"'`` without index. For example, the
  ``select`` command in the following commands::

    table_create Memos TABLE_NO_KEY
    column_create Memos tags COLUMN_VECTOR Text

    load --table Memos
    [
    {"tags": ["Groonga", "Rroonga", "Mroonga"]}
    ]

    select Memos --filter 'tags @ "Rroonga"'

* Supported ``'fixed_size_type_vector_column @ n'`` without index. For example, the
  ``select`` command in the following commands::

    table_create Memos TABLE_NO_KEY
    column_create Memos numbers COLUMN_VECTOR Int32

    load --table Memos
    [
    {"numbers": [1, 2, 3]}
    ]

    select Memos --filter 'numbers @ 2'

* [:doc:`/install/centos`][:doc:`/server/package`][:doc:`/reference/executables/groonga-httpd`]
  Show exit status. [GitHub#357] [Patch by jacob16bit]
* [:doc:`/install/windows`][:doc:`/reference/executables/groonga`] Supported ``--pid-path``.
* [:doc:`/install/windows`] Allowed to delete file that is opened by other process.
* Accepted selector only proc. Note that the proc can't be used as function. It means
  that the proc can't be used with sequential search.
* Supported function call with complex argument. An example complex argument is
  ``Table["key"].column``. For example::

    function(_key, Table["key"].column)

* [doc][:doc:`/tutorial`] Added more description about database creation fails if DB_PATH
  points to an existing file. [GitHub#354] [Suggested by Hirotaka Takayama]
* [doc][:doc:`/tutorial`] Described JSON formatting tools.
  [GitHub#355] [Suggested by tiwawan]
* [experimental] Added an API to get/set the number of threads. It's a experimental API.
* [experimental][``thread_count``] Added a command that get/set the number of threads.
  It's a experimental command.
* [experimental][:doc:`/reference/executables/groonga`] Supported changing the number
  of threads by ``thread_count`` command. It's a experimental feature.
* [experimental][:doc:`/install/windows`] Added Windows event log support.
  It's a experimental feature.
* [experimental][:doc:`/reference/executables/groonga`] Added Windows event log related
  option ``--use-windows-event-log``. It reports logs as Windows events.
  It's a experimental feature.
* [:doc:`/install/windows`] Used Groonga's default encoding for log message.
* Log used indexes in ``INFO`` level. The default level ``NOTICE``. So the logs aren't
  showed by default.
* [API] Added :c:func:`grn_log_level_to_string()` and :c:func:`grn_log_level_parse()`.
* [:doc:`/reference/executables/groonga`] Accepted log level name (e.g. ``info``,
  ``debug`` and so on) for ``--log-level`` value.
* [:doc:`/reference/commands/log_level`][:doc:`/reference/commands/log_put`]
  Accepted log level name for ``--level`` argument.
* [plugin] Added :c:func:`grn_command_input_get_arguments()`.
* Updated sharding plugins.

  * [:doc:`/reference/commands/logical_select`] Fixed output format.
    It has become :doc:`/reference/commands/select` compatible format.
  * [:doc:`/reference/commands/logical_select`] Supported the following parameters.

    * ``--output_columns``
    * ``--offset``
    * ``--limit``
    * ``--drilldown``
    * ``--drilldown_sortby``
    * ``--drilldown_offset``
    * ``--drilldown_limit``

  * [:doc:`/reference/commands/logical_select`] Used the same default output_columns
    (``"_id, _key, *"``) as :doc:`/reference/commands/select`.
  * [:doc:`/reference/commands/logical_select`] Supported
    :ref:`logical-select-drilldown-label-calc-types` and
    :ref:`logical-select-drilldown-label-calc-target` for labeled drilldown.
  * [:doc:`/reference/commands/logical_select`] Supported cache.
  * [:doc:`/reference/commands/logical_count`] Supported logging whether range index is
    used or not.
  * [:doc:`/reference/commands/logical_count`] Show target table name in debug log.
  * [:doc:`/reference/commands/logical_count`] Supported cache.
  * [:doc:`/reference/commands/logical_range_filter`] Supported
    ``'fixed_size_type_vector_column @ element'``.
  * [:doc:`/reference/commands/logical_range_filter`] Added ``use_range_index`` parameter.
    It's a parameter for test. It should not be used for production.
  * [:doc:`/reference/commands/logical_range_filter`] Log which mode (range-index or
    select mode) is used.
  * [:doc:`/reference/commands/logical_range_filter`] Supported cache.
  * [:doc:`/reference/commands/logical_range_filter`] Supported nested reference vector
    accessor.
  * [:doc:`/reference/commands/logical_range_filter`] Used range_index value set by
    :doc:`/reference/commands/logical_parameters`.
  * [:doc:`/reference/commands/logical_parameters`] Added.

* Added mruby APIs.

  * [mrb] Added ``Accessor#name``.
  * [mrb] Added ``Column#[]``.
  * [mrb] Added ``Column#scalar?``, ``Column#vector?`` and ``Column#index?``.
  * [mrb] Added ``Context#command_version`` and ``Context#command_version=`` (accessors).
  * [mrb] Added ``Context#with_command_version``.
  * [mrb] Added ``Database#each_name``.
  * [mrb] Added ``Groonga::Cache.current``.
  * [mrb] Added ``Record``.
  * [mrb] Added ``Table#each``.
  * [mrb] Added ``TableCursor#key``.
  * [mrb] Binded :c:func:`grn_command_input_get_arguments()` to ``CommandInput#arguments``.
  * [mrb] Binded :c:func:`grn_table_group()` to ``Table#group``.
  * [mrb] Binded :c:func:`grn_table_group_flags()` to ``TableGroupFlags``.
  * [mrb] Binded ``GRN_COMMAND_VERSION_DEFAULT``.
  * [mrb] Binded ``grn_cache``.
  * [mrb][estimate_size] Supported ``(... || ...) && (... || ...)`` as expression case.
  * [mrb] Supported query log.

Fixes
^^^^^

* Fixed a memory leak when an error is occurred in :c:func:`grn_expr_exec()`.
  For example, unsupported operator (e.g. ``GRN_OP_TERM_EXTRACT``) is used
  (``not implemented operator assigned`` is occurred for the case).
* [bindings/php] Added a missing check for a memory allocation failure.
  [Reported by Bill Parker]
* [:doc:`/install/centos`][:doc:`/server/package`][logrotate] Fixed syntax error in script.
* [:doc:`/install/centos`][:doc:`/server/package`][logrotate] Fixed wrong daemon running check.
* [:doc:`/install/centos`][:doc:`/server/package`][logrotate] Stop to set owner/group to log files.
  Because it's not consistent. groonga-httpd creates log files with root
  owner/group. But logrotated log files are created with groonga
  owner/group. [GitHub#358] [Reported by jacob16bit]
* [:doc:`/reference/executables/groonga`] Fixed reported the maximum number of threads.
* [:doc:`/reference/executables/groonga-httpd`] Remove a needless space in log message::

    |n|  grn_fin (0) ->
    |n| grn_fin (0)
        ^

* Fixed a bug that estimating size by regexp query with anchor (e.g. ``\\\\A`` in
  ``--filter 'comment @~ "\\\\Abc"'``) doesn't work. The feature is used in
  :doc:`/reference/commands/logical_range_filter`.
* [:doc:`/reference/command/request_id`] Fixed a memory leak when ``request_id`` byte size >= 24.
* [:doc:`/reference/commands/lock_clear`] Fixed a typo in command name in Syntax section.
  [GitHub#363] [Reported by Christian Kakesa]
* [sharding] Fixed wrong min include detection for month range type.

Thanks
^^^^^^

* Bill Parker
* jacob16bit
* Hirotaka Takayama
* tiwawan
* Christian Kakesa

.. _release-5-0-5:

Release 5.0.5 - 2015-06-29
--------------------------

Improvements
^^^^^^^^^^^^

* Show correct error information such as NoSuchFileOrDirectory when opening a database.
* Don't set the default logger path for library use.

  * It's backward incompatible change. But it will not effect to many users.
  * Server use (groonga command, Mroonga, PGroonga and so on) users can get
    log by default. In server use, developers set up log in their software.
  * Most library use (Rroonga, groonga-gobject and so on) users couldn't get
    log by default with earlier versions. The default log path is system
    path such as /var/log/groonga/groonga.log. It's not writable for normal
    users.

* [windows] Show error information when memory isn't enough on failing ``CreateFileMapping()``.
* [:doc:`/reference/commands/tokenize`] Updated example to show new "force_prefix" value.
  This value is added since 5.0.4.
* [windows] Show error information when disk has any problem (disk full and so on) on failing ``FlushViewOfFile()``.
* [API] Added :c:func:`grn_obj_flush()`.
* [API] Added :c:func:`grn_obj_flush_recursive()`.
* [:doc:`/reference/commands/io_flush`] Added. It flushes memory mapped data to disk.
  Usually memory data automatically flush by an OS, but you can explicitly flush with
  this command.
* [mruby] Binded ``grn_obj_remove()`` to Object#remove.
* [mruby] Binded ``grn_table_delete()`` and ``grn_table_delete_by_id()`` to Table#delete.
* [:doc:`/reference/commands/logical_table_remove`] Added.
* [:doc:`/reference/commands/logical_select`] Added. ``--filter`` is only supported for now.
* [cmake] Supported embedded MeCab tokenizer.
* [:doc:`/reference/commands/logical_count`] Supported month and day mixed shards.
  In the same month, month shard must have earlier records rather than day
  shards in the same month. For example::

    XXX_201506   <- includes only 2015-06-01 and 2015-06-02 records
    XXX_20150603 <- includes only 2015-06-03 records
    XXX_20150604 <- includes only 2015-06-04 records

Fixes
^^^^^

* Fixed wrong macro to include netinet/in.h.
  [GitHub#348] [Reported by OBATA Akio]
* [rpm][:doc:`/reference/executables/groonga-httpd`] Fixed failing restart.
  [GitHub#351] [Patch by jacob16bit]

Thanks
^^^^^^

* OBATA Akio
* jacob16bit

.. _release-5-0-4:

Release 5.0.4 - 2015-05-29
--------------------------

Improvements
^^^^^^^^^^^^

* [mruby] Changed to use ``inspect`` to show meaningful error message for error value.
* [mruby] Supported ``Groonga::Bulk#inspect`` to inspect bulk content.
* [mruby] Supported ``Bulk#value`` to extract the value of record from bulk content.
* [mruby] Supported estimating size for ``reference_column == record_id`` in
  :doc:`/reference/commands/logical_range_filter`. In above case, it can be searched
  more effectively.
* [:doc:`/reference/functions/sub_filter`] Supported index column as ``scope`` parameter.
* [:doc:`/reference/grn_expr/script_syntax`] Described clearly about numerical
  value. [GitHub groonga/groonga.org#16] [Suggested by Hiroyuki Sato]
* [:doc:`/reference/commands/select`] Supported accessing other table's record in filter.
  You can use ``--filter 'OTHER_TABLE[KEY].COLUMN'`` for example.
* [:doc:`/reference/commands/select`] Supported operator in table key.
  You can use ``--filter 'OTHER_TABLE["SOME STRING" + "ANOTHER STRING"].COLUMN'"`` for example.
* [example] Used Ruby 2.0 or later API in script for converting dictionary data.
* Changed to show error message about invalid type of keys about table.
* [doc] Fixed link from sourceforge.jp to osdn.me or osdn.jp about mailing list preference page.
  SourceForge.jp is marked as obsoleted because of branding issue since May 11, 2015.
* [:doc:`/reference/commands/tokenize`] Added ``force_prefix`` value to each token information. [Patch by Naoya Murakami]
* Supported to search by shorter words such as 2 or less characters for :ref:`token-trigram`.
  [Patch by Naoya Murakami]
* [deb] Added service file for Systemd into groonga-httpd and groonga-server-gqtp packages.
* [:doc:`/reference/commands/select`] Ignored ``--query`` when its
  value consists of only space characters. Space characters include
  full-width space (``U+3000 IDEOGRAPHIC SPACE`` in
  Unicode). [Suggested by TomyGX]

Fixes
^^^^^

* Fixed a crash bug when empty key is specified for ``drilldown[label].keys``.
* Fixed a bug that the return value of ``grn_parse_query_flags`` is not properly checked.
  [GitHub#336] [Reported by Hiroaki Nakamura]
* Fixed a build error on some BSD systems. They doesn't have ``-i`` option for ``sed``.
* Fixed a build error on Solaris. It is changed to initialize by ``sizeof(msghdr)`` in ``memset()`` because
  ``msg_control``, ``msg_controllen`` and ``msg_flags`` doesn't exist on Solaris by default.
* [:doc:`/reference/tokenizers`] Fixed a typo. [GitHub#338] [Reported by Hiroyuki Sato]
* [:doc:`/reference/output`] Fixed markup. [GitHub groonga/groonga.org#17]
  [Reported by Hiroyuki Sato]
* Reduced getenv() in each ``grn_ii_cursor_set_min()``. This fixes performance
  regression on Windows.
* Fixed a build error on OpenBSD. [groonga-dev,03255] [Reported by fbnteqr]
* [:doc:`/reference/executables/groonga-httpd`] Fixed a bug that same message is
  logged.
* Fixed a crash bug which is caused by double free memory.
* Fixed a memory leak. It's occurred when ``--match_columns`` and ``--query`` are
  used for non indexed text field and text fields have a value that isn't bulk
  embeddable. Normally, 32byte over size text isn't bulk embeddable, so this bug
  doesn't affect to the case if only small text less than 32byte are stored.
* [:doc:`/reference/tokenizers`] [TokenRegexp] Fixed a bug that it can't be searched
  correctly when query contains characters which are treated as blank character.
  For example, the newline - "\\n" is typical one.

Thanks
^^^^^^

* Hiroaki Nakamura
* Hiroyuki Sato
* Naoya Murakami
* fbnteqr
* TomyGX

.. _release-5-0-3:

Release 5.0.3 - 2015-04-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/tokenizers`][:doc:`/reference/regular_expression`]
  Skip the last one character token.
* [mruby] Supported regexp pattern for estimating size.
* [mruby] Supported size estimation for accessor.
* [:doc:`/reference/commands/logical_range_filter`] Removed ``GRN_LOGICAL_RANGE_FILTER_ENABLED``
  environment variable which is introduced since Groonga 5.0.2.
  Use ``GRN_LOGICAL_RANGE_FILTER_THRESHOLD=0`` to disable range index search
  feature.
* [:doc:`/reference/commands/logical_range_filter`] Supported negative limit and offset.
* [:doc:`/install/windows`] Used `Groonga Admin <https://github.com/groonga/groonga-admin>`_ in package.
* [:doc:`/reference/commands/logical_range_filter`] Changed threshold meaning:

  * threshold <= 0.0: always use range index
  * threshold >= 1.0: never use range index

* [:doc:`/reference/commands/dump`] Supported plugin.
* [:doc:`/reference/commands/dump`] Added the following options:

  * ``--dump_plugins [yes(default)/no]``
  * ``--dump_schema [yes(default)/no]``
  * ``--dump_records [yes(default)/no]``
  * ``--dump_indexes [yes(default)/no]``

* [API] Added :c:func:`grn_plugin_get_ruby_suffix()`.
* [:doc:`/reference/commands/dump`] Fixed order to put index columns after reference columns
  because index column may refer reference columns.
* [:doc:`/reference/commands/dump`] Don't dump records of lexicon.
* [:doc:`/reference/commands/dump`] Show ``_id`` for ``TABLE_NO_KEY`` again.
* [:doc:`/reference/commands/dump`] Used offline index construnction.
* Increased max hash key size from 4KiB (4096Byte) to 64KiB - 1 (65535Byte).
* Increased max cache key size from 4KiB (4096Byte) to 64KiB - 1 (65535Byte).
* Improved performance for nested index search.
* Used index for nonexistent reference column value.
* [experimental] Added plugin functions/vector. It includes :doc:`/reference/functions/vector_size` function.
* [:doc:`/install/windows`] Updated Visual Studio version
  [GitHub groonga/meetup#4] [Reported by Hiroyuki Mizuhara]
* [:doc:`/reference/commands/cache_limit`] Expired old caches when the max N caches is decreased.
  [Suggested by Gurunavi, Inc.]
* Show more information such as errno for errors.
* [windows] Used secure functions on Windows.
* Added the following APIs to change log rotate threshold in file size.

  * :c:func:`grn_default_logger_set_rotate_threshold_size()`
  * :c:func:`grn_default_logger_get_rotate_threshold_size()`
  * :c:func:`grn_default_query_logger_set_rotate_threshold_size()`
  * :c:func:`grn_default_query_logger_get_rotate_threshold_size()`

* [experimental] Supported log rotation. The feature is disabled by default.
  You can enable log rotation by the following options:

  * ``--log-rotate-threshold-size``
  * ``--query-log-rotate-threshold-size``

* [:doc:`/server/gqtp`] Documented about GQTP server.
* [:doc:`/reference/executables/groonga`] Documented groonga executable file partially.
* Supported Ubuntu 15.04 (Vivid Vervet).
* Supported Debian 8.0 (Jessie).
* [:doc:`/reference/executables/groonga-httpd`] Updated bundled nginx version to the latest mainline (1.8.0).

Fixes
^^^^^

* [windows] Fixed a bug that :ref:`offline-index-construction` is
  failed for large data (at least 1GB or larger) with Groonga built by
  Microsoft Visual C++. [Reported by Hideki ARAI]
* [mruby] Made ``\\`` index searchable in regular expression.
* Fixed a bug that ``GRN_II_CURSOR_SET_MIN_ENABLE=yes`` doesn't return some matched records.
* [sharding] Fixed a bug that partial range is handled as all range.
* [:doc:`/reference/commands/logical_range_filter`] Fixed a bug that ``:order => "descending"`` doesn't work.
* [:doc:`/reference/commands/logical_count`] Re-supported counting with range index.
* Fixed a bug causing malfunction of :c:func:`grn_pat_del()`
  and added a test for invalid patricia trie node add case.
  [groonga-dev,03177] [Reported by yuya sako]

Thanks
^^^^^^

* Hideki ARAI
* Hiroyuki Mizuhara
* Gurunavi, Inc.
* yuya sako

.. _release-5-0-2:

Release 5.0.2 - 2015-03-31
--------------------------

It's a bug fix release of 5.0.1.

Improvements
^^^^^^^^^^^^

* Supported MessagePack 1.0.1. [Reported by Hiroshi Hatake]
* [logical_range_filter] Disabled range index by default. It's enabled
  when you set the enviromnent variable ``GRN_LOGICAL_RANGE_FILTER_ENABLED``
  to ``yes``.

Fixes
^^^^^

* Fixed a regression bug that JSONP doesn't work. It was introduced
  in Groonga 4.1.1.
* [windows] Fixed a bug that crash on x86 for Groonga 5.0.1.
  [groonga-dev,03131] [Reported by Atsushi Shinoda]
* Fixed a crash bug that libedit is not properly initialized. The
  problem is fixed in the environment such as CentOS 7.

Thanks
^^^^^^

* Atsushi Shinoda
* Hiroshi Hatake

.. _release-5-0-1:

Release 5.0.1 - 2015-03-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/logical_range_filter`] Supported filter
  and sort.
* Supported range search by multiple column index.
* Added API :doc:`/reference/api/overview` document for users who want
  to use Groonga as library.
* [incompatible] Changed internal type of ``_score`` to floating point
  number from 32bit integer number. This is incompatible change for DB
  API users. This *isn't* incompatible change for query API users. It
  means that users who just use :doc:`/reference/commands/select`
  aren't affected. Use the following code that works with both older
  and newer Groonga:

  .. code-block:: c

     grn_obj *score;
     double score_value;

     if (score->header.domain == GRN_DB_FLOAT) {
       score_value = GRN_FLOAT_VALUE(score);
     } else {
       score_value = (double)GRN_INT32_VALUE_FLOAT_VALUE(score);
     }

* [:doc:`/reference/commands/select`] Added more strict check for
  invalid drilldown parameter.
* Added :c:func:`grn_ctx_get_all_tables()`. [Suggested by Masatoshi
  Teruya]
* Supported to customize score function. See :doc:`/reference/scorer`
  for details.
* [incompatible] Custom score function feature introduced API and ABI
  incompatibilities in DB API layer. If you're using
  :c:type:`grn_search_optarg`, please check that your code initializes
  your :c:type:`grn_search_optarg` by ``0`` like the following:

  .. code-block:: c

     grn_search_optarg options;
     memset(&options, 0, sizeof(grn_search_optarg));

  If your code do the above thing, your code is API compatible and ABI
  incompatible. You just need to rebuild your code without
  modification.

  If your code doesn't the above thing, you need to added the above
  thing to your code.

* Added the following predicates that check :c:type:`grn_obj` type to
  DB API:

  * :c:func:`grn_obj_is_table()`
  * :c:func:`grn_obj_is_proc_proc()`
  * :c:func:`grn_obj_is_function_proc()`
  * :c:func:`grn_obj_is_selector_proc()`
  * :c:func:`grn_obj_is_scorer_proc()`

* [experimental] Supported skipping posting list when searching
  popular term and rare term at the same time. It will improve
  performance. Set ``GRN_II_CURSOR_SET_MIN_ENABLE`` environment
  variable to ``1`` to enable the feature. The feature is disabled by
  default.
* [doc] Added :doc:`/reference/functions/in_values` document.
* [doc] Added :doc:`/reference/commands/logical_count` document.
* [mruby] Implemented custom ``#inspect`` method. Is is useful for
  debugging.
* Added :doc:`/reference/scorers/scorer_tf_at_most` scorer. It
  limits not to exceed specified score regardless of term frequency.
* [mruby] Supported estimating matched records for selecting index
  search or sequential search.
* Added the following functions to estimate size by index:

  * :c:func:`grn_expr_estimate_size()`
  * :c:func:`grn_ii_estimate_size_for_query()`
  * :c:func:`grn_ii_estimate_size_for_lexicon_cursor()`

* Added missing :ref:`normalizer-auto` availability check. [GitHub#283]
  [Reported by Tasuku SUENAGA]
* Dropped Visual Studio 2010 support.
* [experimental][mecab] Supported chunked tokenization. This feature
  is a workaround for MeCab's "too long sentense" error.  Specify
  ``yes`` to ``GRN_MECAB_CHUNKED_TOKENIZE_ENABLED`` environment
  variable to enable it. By this configuration, Groonga splits a long
  text (8192 bytes over text by default) into small chunks and passes
  each chunk to MeCab. By this process, the above error isn't
  occurred. Additionally, you can customize chunk threshold bytes by
  ``GRN_MECAB_CHUNK_SIZE_THRESHOLD`` environment variable. Note that
  ``,``, ``.``, ``!``, ``?``, ``U+3001 IDEOGRAPHIC COMMA``, ``U+3002
  IDEOGRAPHIC FULL STOP``, ``U+FF01 FULLWIDTH EXCLAMATION MARK`` and
  ``U+FF1F FULLWIDTH QUESTION MARK`` are treated as chunk delimiter
  characters.
* Supported ``--pid-file`` in server mode of
  :doc:`/reference/executables/groonga`.
* [groonga-httpd] Supported graceful stop to clean Groonga. It doesn't
  terminate the open connections immediately.
* [experimental] Supported regular expression. See
  :doc:`/reference/regular_expression` to know about how to use regular
  expression.
* [experimental] Added :doc:`/reference/commands/plugin_unregister`
  command.
* [http][:doc:`/reference/commands/load`] Added "," as chunk separator
  in POST data. It decreases internal buffer size and improves load
  time when POST data don't include any new line.
* [doc] Added :doc:`/reference/tokenizers` document.
* Improved POSIX.2 compatibility by using ``.`` as bash's "source"
  command replacement. [GitHub#317] [Patch by Jun Kuriyama]
* [windows] Changed to the default IO version 1. It reduces disk usage
  on Windows. [groonga-dev,03118] [Tested by ongaeshi]
* [httpd] Updated bundled nginx version to the latest mainline
  (1.7.11).
* Changed mime-type for TSV output to ``text/tab-separated-values``
  from ``text/plain``.
* [:ref:`token-filter-stop-word`] Supported
  :ref:`offline-index-construction`. [GitHub#296] [Patch by Naoya
  Murakami]

Fixes
^^^^^

* Fixed not to use obsolete ``--address`` parameter in the default
  groonga.conf. ``--bind-address`` is used instead.  [Groonga-talk]
  [Reported by Dewangga]
* [:doc:`/reference/commands/truncate`] Fixed a bug that
  :ref:`table-no-key` table can't be truncated.
* [mecab] Reduced needless logs for "empty token" and "ignore empty
  token".
* Fixed a bug that wrong section in index is used. It means that wrong
  search result is returned. If you satisfy all of the following
  conditions, this bug is occurred:

  * Multiple indexes are available.
  * The first defined index or the last defined index are
    multi-column indexes.
  * When both of the first defined index and the last defined index are
    multi-column indexes, source column orders are different in them.

* Fixed a bug that passing Groonga command line to
  :doc:`/reference/executables/groonga` from shell command line style
  usage always returns ``0`` as exit code. For example, ``groonga
  DB_PATH nonexistent_command`` always returned ``0`` as exist code.
* Fixed a bug that plugin path may be broken when two or more plugins
  registered. [Reported by Naoya Murakami]
* Fixed a bug that ``Lexicon.index.source_column_name`` style in
  :ref:`select-match-columns` doesn't work when source
  column specified by ``source_column_name`` has two or more
  indexes. [Reported by Naoya Murakami]

Thanks
^^^^^^

* Masatoshi Teruya
* Tasuku SUENAGA
* Dewangga
* Jun Kuriyama
* ongaeshi
* Naoya Murakami

.. _release-5-0-0:

Release 5.0.0 - 2015-02-09
--------------------------

* Bump version to 5.0.0!

Improvements
^^^^^^^^^^^^

* [doc] Added :ref:`script-syntax-security` about :doc:`/reference/grn_expr/script_syntax`.
* [experimental] Added sharding plugin. Execute `register sharding` to
  enable this feature in advance, then use :doc:`/reference/commands/logical_count` to get the number of records.
* [cmake] Supported embedded Groonga with Clang. It fixed compilation failure
  on FreeBSD 10.1. `[MDEV-7293] <https://mariadb.atlassian.net/browse/MDEV-7293>`_
  [Reported by Bernard Spil]
* Supported to customize plugins directory. Set `GRN_PLUGINS_DIR` environment variable.

Fixes
^^^^^

* Fixed build failure when system has an incompatible version of onigmo/oniguruma
  headers installed. [GitHub#276] [Patch by Akinori MUSHA]
* Fixed time related build failure on MSVC [GitHub#237]

Thanks
^^^^^^

* Akinori MUSHA
* Bernard Spil

The old releases
----------------

.. toctree::
   :maxdepth: 2

   news/4.x
   news/3.x
   news/2.x
   news/1.3.x
   news/1.2.x
   news/1.1.x
   news/1.0.x
   news/0.x
   news/senna
