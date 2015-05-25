.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

.. _release-5-0-4:

Release 5.0.4 - 2015-05-29
--------------------------

Improvements
^^^^^^^^^^^^

* [mruby] Used inspect for error value.
* [mruby] Added bulk content to ``Groonga::Bulk#inspect``.
* [mruby] Supported ``Bulk#value`` for record.
* [mruby] Supported estimating size for ``reference_column == record_id``.
* [:doc:`/reference/functions/sub_filter`] Supported index column as scope.
* [:doc:`/reference/functions/sub_filter`] Supported accessor that contains
  index column as scope.
* Ignored no keys labeled drilldown.
* [:doc:`/reference/grn_expr/script_syntax`] Described clearly about numerical
  value. [GitHub groonga/groonga.org#16] [Suggested by Hiroyuki Sato]
* [:doc:`/reference/commands/select`] Supported accessing other table's record in filter.
  See https://github.com/groonga/groonga/blob/master/test/command/suite/select/filter/record/key/literal.expected
* [:doc:`/reference/commands/select`] Supported operator as table key.
  See https://github.com/groonga/groonga/blob/master/test/command/suite/select/filter/record/key/operator.expected
* [:doc:`/reference/tokenizers`] [TokenRegexp] Don't ignore blank.
* [:doc:`/reference/tokenizers`] [TokenRegexp] Don't require character types to normalizer.
* [:doc:`/reference/tokenizers`] [TokenRegexp] Added one position after blank character.
  By this change, "abcd" isn't matched to "ab\ncd".
* [example] Used Ruby 2.0 or later API.
* Set error message for invalid keys type.
* Migrated sourceforge.jp to osdn.me or osdn.jp. Because SourceForge.jp is
  marked as obsoleted because of branding issue since May 11, 2015.
* [:doc:`/reference/commands/tokenize`] Added force_prefix. [Patch by Naoya Murakami]
* Supported force prefix search when unmatured token is 2 characters or more.
  [Patch by Naoya Murakami]

Fixes
^^^^^

* Fixed rc check position. [GitHub#336] [Reported by Hiroaki Nakamura]
* Don't require "-i" option for sed. Because sed on some BSD systems doesn't
  have "-i" option.
* Initialize msghdr by memset(). Because msg_control, msg_controllen and
  msg_flags doesn't exist on Solaris by default.
* [:doc:`/reference/tokenizers`] Fixed a typo. [GitHub#338] [Reported by Hiroyuki Sato]
* [:doc:`/reference/output`] Fixed markup. [GitHub groonga/groonga.org#17]
  [Reported by Hiroyuki Sato]
* [:doc:`/reference/tokenizers`] [TokenRegexp] Fixed a bug that too much token
  is emitted.
* Reduced getenv() in each grn_ii_cursor_set_min(). This fixes performance
  regression on Windows.
* Fixed a build error on OpenBSD. [groonga-dev,03255]
* [:doc:`/reference/executables/groonga-httpd`] Fixed a bug that same message is
  logged.
* Fixed a crash bug. It is occurred by the following sequence:

  1. thread1: call grn_ii_cursor_open()
  2. thread1: find one entry in ii
  3. thread2: change ii
  4. thread1: detect buffer or chunk change
  5. thread1: call grn_ii_cursor_close() and run the next loop
  6. thread2: delete entry in ii
  7. thread1: find no entry and break loop
  8. thread1: return grn_ii_cursor that is already closed by grn_ii_cursor_close() at 5.
  9. thread1: call grn_ii_cursor_close() for the grn_ii_cursor that is already closed

  We can replace "thread" with "process" in the above sequence.

* Fixd a memory leak. It's occurred when ``--match_columns`` and ``--query`` are
  used for non indexed text field and text fields have a value that isn't bulk
  embeddable. Normally, 32byte over size text isn't bulk embeddable.

Thanks
^^^^^^

* Hiroaki Nakamura
* Hiroyuki Sato
* Naoya Murakami

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
  enable this feature in advance, then use :doc:`/reference/commands/logical_count` to select records.
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
