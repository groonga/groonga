.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

.. _release-6-0-7:

Release 6.0.7 - 2016-07-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/functions/string_substring`] Added
  ``string_substring`` function to extract a substring from given
  string. The syntax of ``string_substring`` is
  ``string_substring(string, from, [length])``. [GitHub#564] [Patch by
  Naoya Murakami]

* [experimental] Added ``GRN_II_MAX_N_SEGMENTS_TINY`` and
  ``GRN_II_MAX_N_CHUNKS_TINY`` environment variables to customize the
  value about default max N segments/chunks. This feature is affected to
  index column for fixed size scalar column. It reduces memory usage
  but not tested widely yet.

* [:doc:`/news/5.x`] Extracted Groonga 5.x news entries from :doc:`/news`.

* [:doc:`/reference/functions/vector_slice`] Added ``vector_slice``
  function to extract specific elements in vector column. [GitHub#582]
  [Patch by Naoya Murakami]

* Supported index range search for ``_key`` of PAT/DAT table.
  [GitHub#583]

* [:doc:`/reference/commands/object_list`] Added ``object_list``
  command for debugging. It is useful to investigate whether database
  is corrupted or not.

* Added a script that checks ``object_list`` response.

* [mruby] Supported float bulk in expression_rewriter. [GitHub#587]
  [Patch by Naoya Murakami]

* [:doc:`/reference/commands/dump`] Changed output order about table
  by name instead of ID. It breaks dump output compatibility but it
  can be restored as usual.

* [windows] Updated bundled msgpack to 2.0.0.

* [windows] Added
  :doc:`/reference/executables/groonga-suggest-create-dataset`.

* [httpd] Updated bundled nginx to 1.11.3.

* [deb] Dropped support for Ubuntu 15.10 (Wily werewolf).

Fixes
^^^^^

* [examples edict] Fixed to use ``gzcat`` instead of ``zcat`` if
  exists. [GitHub#576] [Patch by Yuya TAMANO]

* Added missing null-check before dereferencing a
  pointer. [GitHub#579] [Patch by Sho Minagawa]

* Fixed not to perform a sequential search if an index is available.
  [GitHub#580]

* [:doc:`/reference/commands/load`] Fixed a bug that ``Time`` column
  can reduce the precision of values. [GitHub#581]

* Fixed a bug that object literal expression codes is broken when
  executing multiple logical operations. [GitHub#584] [Patch by Naoya
  Murakami]

* Fixed a bug that columns of Float, WGS84GeoPoint and TokyoGeoPoint
  were created with ``GRN_OBJ_COMPRESS_ZLIB`` even if the flag was not
  specified. [GitHub#586] [Reported by Naoya Murakami]

Thanks
^^^^^^

* Naoya Murakami
* Yuya TAMANO
* Sho Minagawa

.. _release-6-0-5:

Release 6.0.5 - 2016-06-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/io_flush`] Marked as stable command.

* [mruby] Supported to optimize prefix search by estimating
  data size efficiently

* [:doc:`/reference/functions/fuzzy_search`] Supported
  ``max_distance`` option as 3rd argument to specify it
  easily. [GitHub#553] [Patch by Naoya Murakami]

* [:doc:`/reference/commands/query_expand`] Supported command to
  expand query. It is useful if there are many synonyms.

* [:doc:`/reference/commands/select`] Supported ``--drilldown`` with
  ``command_version=3``. [groonga-dev,04055] [Reported by Naoya
  Murakami]

* Removed needless code from ``grn_table_select_sequential()``.
  [GitHub#560] [Reported by Sho Minagawa]

* grn_table_setoperation(): Changed to update score instead of
  overwriting by ``GRN_OP_ADJUST``. This change is introduced to keep
  consistency with ``grn_ii_posting_add()``. [groonga-dev,04058]
  [Reported by Naoya Murakami]

* [:doc:`/reference/commands/dump`] Reduced the max opened
  table/column files when 1 thread mode.

Fixes
^^^^^

* [CMake][Windows] Fixed to install missing mruby
  script. [groonga-dev,04040] [Reported by Soichiro Kiyokawa]

* [Windows] Changed to bundle msgpack-c.

* [:doc:`/install/others`] Fixed a typo
  about default database encoding (utf8). [GitHub#549] [Patch by IWAI, Masaharu]

* [:doc:`/contribution/development/cooperation`] Fixed a typo
  about product name (Twitter). [GitHub#550] [Patch by IWAI, Masaharu]

* Fixed a bug that specific records are not included into search
  result when multiple index column is created with ``WITH_SECTION``
  flag. [GitHub#551]

* Fixed a crash bug that searching while loading data with
  ``GRN_II_CURSOR_SET_MIN_ENABLE=yes``. ``GRN_II_CURSOR_SET_MIN_ENABLE``
  is enabled by default since Groonga 6.0.3.

* [:doc:`/reference/token_filters`] Fixed thread unsafe implementation.

* [doc] Fixed a typo in 6.0.4 release entry. [GitHub#559] [Patch by cafedomancer]

Thanks
^^^^^^

* Naoya Murakami
* Soichiro Kiyokawa
* IWAI, Masaharu
* cafedomancer
  

.. _release-6-0-4:

Release 6.0.4 - 2016-06-06
--------------------------

It's a bug fix release of 6.0.3. It's recommend that Groonga 6.0.3
users upgrade to 6.0.4. This release fixes some search related
problems.

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

The old releases
----------------

.. toctree::
   :maxdepth: 2

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
