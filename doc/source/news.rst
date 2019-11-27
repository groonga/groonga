.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

.. _release-9-1-0:

Release 9.1.0 - 2019-11-29
--------------------------

Improvements
^^^^^^^^^^^^

* Improved the performance of the sort by value.

* Improved the performance of the "&&" operation.

  * For example, the performance of condition expression such as the following is increased.

  * ( A || B ) && ( C || D ) && ( E || F) ...

* [:doc:`/reference/tokenizers/token_mecab`] Added a new option ``use_base_form``

  * We can search using the base form of a token by this option.

  * For example, if we search "支えた" using this option, "支える" is hit also.

Fixes
^^^^^

* Fix a bug that when the accessor is index, performance decreases.

  * For example, it occurs with the query include the following conditions.

    * ``sccessor @ query``

    * ``accessor == query``

* Fixed a bug the estimated size of a search result was overflow when the buffer is big enough. [PGroonga#GitHub#115][Reported by Albert Song]

* Improved a test(1) portability. [GitHub#1065][Patched by OBATA Akio]

* Added missing tools.

  * Because ``index-column-diff-all.sh`` and ``object-inspect-all.sh`` had not bundled in before version.

Thanks
^^^^^^

* Albert Song

* OBATA Akio

.. _release-9-0-9:

Release 9.0.9 - 2019-10-30
--------------------------

.. note::

    Maybe performance decreases from this version.
    Therefore, If performance decreases than before, please report us with reproducible steps.

Improvements
^^^^^^^^^^^^

* [:doc:`reference/log`] Improved that output the sending time of response into query-log.

* [:doc:`reference/commands/status`] Added that the number of current jobs in the ``status`` command response.

* [:doc:`reference/executables/groonga-httpd`] Added support for ``$request_time`` in log.

  * In the previous version, even if we specified the ``$request_time`` in the ``log_format`` directive, it was always 0.00.
  * If we specify the ``$request_time``, groonga-httpd output the correct time form this version.

* [:doc:`reference/executables/groonga-httpd`] Added how to set the ``$request_time`` in the document.

* Supported Ubuntu 19.10 (Eoan Ermine)

* Supported CentOS 8 (experimental)

  * The package for CentOS 8 can't use a part of features(e.g. we can't use ``TokenMecab`` and can't cast to int32 vector from JSON string) for lacking some packages for development.

* [tools] Added a script for executing the ``index_column_diff`` command simply.

  * This script name is index-column-diff-all.sh.
  * This script extracts index columns form Groonga's database and execute the ``index_column_diff`` command to them.

* [tools] Added a script for executing ``object_inspect`` against
  all objects.

  * This script name is object-inspect-all.sh.

Fixes
^^^^^

* Fixed a bug that Groonga crash when we specify the value as the first argument of between.[GitHub#1045][Reported by yagisumi]

Thanks
^^^^^^

* yagisumi

.. _release-9-0-8:

Release 9.0.8 - 2019-09-27
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`reference/commands/log_reopen`] Added a supplementary explanation when we use ``groonga-httpd`` with 2 or more workers.

* Improved that Groonga ignores the index being built.

  * We can get correct search results even if the index is under construction.

  * However, the search is slow because of Groonga out of use the index to search in this case.

* [:doc:`reference/functions/sub_filter`] Added a feature that ``sub_filter`` executes a sequential search when Groonga is building indexes for the target column or the target column hasn't indexed.

  * ``sub_filter`` was an error if the above situation in before
    version.
  * From this version, ``sub_filter`` returns search results if the above situation.
  * However if the above situation, ``sub_filter`` is slow. Because it is executed as a sequential search.

* [:doc:`/install/centos`] Dropped 32-bit package support on CentOS 6.

Fixes
^^^^^

* [:doc:`reference/commands/logical_range_filter`] Fixed a bug that exception about closing the same object twice occurs when we have enough records and the number of records that unmatch filter search criteria is more than the estimated value of it.

.. _release-9-0-7:

Release 9.0.7 - 2019-08-29
--------------------------

Improvements
^^^^^^^^^^^^

* [httpd] Updated bundled nginx to 1.17.3.

  * Contains security fix for CVE-2019-9511, CVE-2019-9513, and CVE-2019-9516.

Fixes
^^^^^

* Fixed a bug that Groonga crash when posting lists were huge.

  * However, this bug almost doesn't occur by general data. Because posting lists don't grow bigger so much by them.

* Fixed a bug that returns an empty result when we specify ``initial`` into a stage of a dynamic column and search for using index. [GitHub#683]

* Fixed a bug that the configure phase didn't detect libedit despite installing it. [GitHub#1030][Patched by yu]

* Fixed a bug that ``--offset`` and ``--limit`` options didn't work
  with ``--sort_keys`` and ``--slices`` options. [clear-code/redmine_full_text_search#70][Reported by a9zawa]

* Fixed a bug that search result is empty when the result of ``select`` command is huge. [groonga-dev,04770][Reported by Yutaro Shimamura]

* Fixed a bug that doesn't use a suitable index when prefix search and suffix search. [GitHub#1007, PGroonga#GitHub#96][Reported by oknj]

Thanks
^^^^^^

* oknj

* Yutaro Shimamura

* yu

* a9zawa

.. _release-9-0-6:

Release 9.0.6 - 2019-08-05
--------------------------

Improvements
^^^^^^^^^^^^

* Added support for Debian 10 (buster).

Fixes
^^^^^

* [:doc:`reference/commands/select`] Fixed a bug that search is an error when occurring search escalation.

* [:doc:`reference/commands/select`] Fixed a bug that may return wrong search results when we use nested equal condition.

* [geo_distance_location_rectangle] Fixed an example that has wrong ``load`` format. [GitHub#1023][Patched by yagisumi]

* [:doc:`tutorial/micro_blog`] Fixed an example that has wrong search results. [GutHub#1024][Patched by yagisumi]

Thanks
^^^^^^

* yagisumi

.. _release-9-0-5:

Release 9.0.5 - 2019-07-30
--------------------------

.. warning::
   There are some critical bugs are found in this release. ``select`` command returns wrong search results.
   We will release the new version (9.0.6) which fixes the issues.
   Please do not use Groonga 9.0.5, and recommends to upgrade to 9.0.6 in the future.
   The detail of this issues are explained at http://groonga.org/en/blog/2019/07/30/groonga-9.0.5.html.

Improvements
^^^^^^^^^^^^

* [:doc:`reference/commands/logical_range_filter`] Improved that only apply an optimization when the search target shard is large enough.

  * This feature reduces that duplicate search result between offset when we use same sort key.
  * Large enough threshold is 10000 records by default.

* [:doc:`/reference/normalizers`] Added new option ``unify_to_katakana`` for ``NormalizerNFKC100``.

  * This option normalize hiragana to katakana.
  * For example, ``ゔぁゔぃゔゔぇゔぉ`` is normalized to ``ヴァヴィヴヴェヴォ``.

* [:doc:`reference/commands/select`] Added drilldowns support as a slices parameter.

* [:doc:`reference/commands/select`] Added columns support as a slices parameter.

* [:doc:`reference/commands/select`] Improved that we can refer ``_score`` in the initial stage for slices parameter.

* [:doc:`/reference/functions/highlight_html`], [:doc:`reference/functions/snippet_html`] Improved that extract a keyword also from an expression of before executing a slices when we specify the slices parameter.

* Improved that collect scores also from an expression of before executing a slices when we specify the slices parameter.

* Stopped add 1 in score automatically when add posting to posting list.

  * ``grn_ii_posting_add`` is backward incompatible changed by this change.
    * Caller must increase the score to maintain compatibility.

* Added support for index search for nested equal like ``XXX.YYY.ZZZ == AAA``.

* Reduce rehash interval when we use hash table.

  * This feature improve performance for output result.

* Improved to we can add tag prefix in the query log.

  * We become easy to understand that it is filtered which the condition.

* Added support for Apache Arrow 1.0.0.

  * However, It's not released this version yet.

* Added support for Amazon Linux 2.

Fixes
^^^^^

* Fixed a bug that vector values of JSON like ``"[1, 2, 3]"`` are not indexed.

* Fixed wrong parameter name in ``table_create`` tests. [GitHub#1000][Patch by yagisumi]

* Fixed a bug that drilldown label is empty when a drilldown command is executed by ``command_version=3``. [GitHub#1001][Reported by yagisumi]

* Fixed build error for Windows package on MinGW.

* Fixed install missing COPYING for Windows package on MinGW.

* Fixed a bug that don't highlight when specifing non-text query as highlight target keyword.

* Fixed a bug that broken output of MessagePack format of [:doc:`/reference/commands/object_inspect`]. [GitHub#1009][Reported by yagisumi]

* Fixed a bug that broken output of MessagePack format of ``index_column_diff``. [GitHub#1010][Reported by yagisumi]

* Fixed a bug that broken output of MessagePack format of [:doc:`reference/commands/suggest`]. [GitHub#1011][Reported by yagisumi]

* Fixed a bug that allocate size by realloc isn't enough when a search for a table of patricia trie and so on. [Reported by Shimadzu Corporation]

  * Groonga may be crashed by this bug.

* Fix a bug that ``groonga.repo`` is removed when updating 1.5.0 from ``groonga-release`` version before 1.5.0-1. [groonga-talk:429][Reported by Josep Sanz]

Thanks
^^^^^^

* yagisumi

* Shimadzu Corporation

* Josep Sanz

.. _release-9-0-4:

Release 9.0.4 - 2019-06-29
--------------------------

Improvements
^^^^^^^^^^^^

* Added support for array literal with multiple elements.

* Added support equivalence operation of a vector.

* [:doc:`reference/commands/logical_range_filter`] Increase outputting logs into query log.

  * ``logical_range_filter`` command comes to output a log for below timing.

    * After filtering by ``logical_range_filter``.
    * After sorting by ``logical_range_filter``.
    * After applying dynamic column.
    * After output results.

  * We can see how much has been finished this command by this feature.

* [:doc:`/reference/tokenizers`] Added document for ``TokenPattern`` description.

* [:doc:`/reference/tokenizers`] Added document for ``TokenTable`` description.

* [:doc:`/reference/tokenizers`] Added document for ``TokenNgram`` description.

* [:doc:`reference/executables/grndb`] Added output operation log into groonga.log

  * ``grndb`` command comes to output execution result and execution process.

* [:doc:`reference/executables/grndb`] Added support for checking empty files.

  * We can check if the empty files exist by this feature.

* [:doc:`reference/executables/grndb`] Added support new option ``--since``

  * We can specify a scope of an inspection.

* [:doc:`reference/executables/grndb`] Added document about new option ``--since``    

* Bundle RapidJSON

  * We can use RapidJson as Groonga's JSON parser partly. (This feature is partly yet)
  * We can more exactly parse JSON by using this.

* Added support for casting to int32 vector from JSON string.

  * This feature requires RapidJSON.

* [:doc:`reference/functions/query`] Added ``default_operator``.

  * We can customize operator when "keyword1 keyword2".
  * "keyword1 Keyword2" is AND operation in default.
  * We can change "keyword1 keyword2"'s operator except AND.

Fixes
^^^^^

* [optimizer] Fix a bug that execution error when specified multiple filter conditions and like ``xxx.yyy=="keyword"``.

* Added missing LICENSE files in Groonga package for Windows(VC++ version).

* Added UCRT runtime into Groonga package for Windows(VC++ version).

* [:doc:`/reference/window_function`] Fix a memory leak.

  * This occurs when multiple windows with sort keys are used. [Patched by Takashi Hashida]

Thanks
^^^^^^

* Takashi Hashida

.. _release-9-0-3:

Release 9.0.3 - 2019-05-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`reference/commands/select`] Added more query logs.

  * ``select`` command comes to output a log for below timing.

    * After sorting by drilldown.
    * After filter by drilldown.

  * We can see how much has been finished this command by this feature.

* [:doc:`reference/commands/logical_select`] Added more query logs.

  * ``logical_select`` command comes to output a log for below timing.

    * After making dynamic columns.
    * After grouping by drilldown.
    * After sorting by drilldown.
    * After filter by drilldown.
    * After sorting by ``logical_select``.

  * We can see how much has been finished this command by this feature.

* [:doc:`reference/commands/logical_select`] Improved performance of sort a little when we use ``limit`` option.

* [index_column_diff] Improved performance.

  * We have greatly shortened the execution speed of this command.

* [index_column_diff] Improved ignore invalid reference.

* [index_column_diff] Added support for duplicated vector element case.

* [Normalizers] Added a new Normalizer ``NormalizerNFKC121`` based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 12.1.

* [TokenFilters] Added a new TokenFilter ``TokenFilterNFKC121`` based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 12.1.

* [:doc:`reference/executables/grndb`] Added a new option ``--log-flags``

  * We can specify output items of a log as with groonga executable file.
  * See [:doc:`reference/executables/groonga`] to know about supported log flags.

* [:doc:`reference/functions/snippet_html`] Added a new option for changing a return value when no match by search.

* [:doc:`reference/commands/plugin_unregister`] Added support full path of Windows.

* Added support for multiline log message.

  * The multiline log message is easy to read by this feature.

* Output key in Groonga's log when we search by index.

* [:doc:`tutorial/match_columns`] Added a document for indexes with weight.

* [:doc:`reference/commands/logical_range_filter`] Added a explanation for ``order`` parameter.

* [:doc:`reference/commands/object_inspect`] Added an explanation for new statistics ``INDEX_COLUMN_VALUE_STATISTICS_NEXT_PHYSICAL_SEGMENT_ID`` and ``INDEX_COLUMN_VALUE_STATISTICS_N_PHYSICAL_SEGMENTS``.

* Dropped Ubuntu 14.04 support.

Fixes
^^^^^

* [index_column_diff] Fixed a bug that too much ``remains`` are reported.

* Fixed a build error when we use ``--without-onigmo`` option. [GitHub#951] [Reported by Tomohiro KATO]

* Fixed a vulnerability of "CVE: 2019-11675". [Reported by Wolfgang Hotwagner]

* Removed extended path prefix ``\\?\`` at Windows version of Groonga. [GitHub#958] [Reported by yagisumi]

  * This extended prefix causes a bug that plugin can't be found correctly.

Thanks
^^^^^^

* Tomohiro KATO
* Wolfgang Hotwagner
* yagisumi

.. _release-9-0-2:

Release 9.0.2 - 2019-04-29
--------------------------

We provide a package for Windows made from VC++ from this release.

We also provide a package for Windows made form MinGW as in the past.
However, we will provide it made from VC++ instead of making from MinGW sooner or later.

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/column_create`] Added a new flag ``INDEX_LARGE`` for index column.

  * We can make an index column has space that two times of default by this flag.
  * However, note that it also uses two times of memory usage.
  * This flag useful when index target data are large.
  * Large data must have many records (normally at least 10 millions records) and at least one of the following features.

    * Index targets are multiple columns
    * Index table has tokenizer

* [:doc:`/reference/commands/object_inspect`] Added a new statistics ``next_physical_segment_id`` and ``max_n_physical_segments`` for physical segment information.

  * We can confirm usage of index column space and max value of index column space by this information.

* [:doc:`/reference/commands/logical_select`] Added support for window function over shard.

* [:doc:`/reference/commands/logical_range_filter`] Added support for window function over shard.

* [:doc:`/reference/commands/logical_count`] Added support for window function over shard.

* We provided a package for Windows made from VC++.

* [:doc:`/reference/commands/io_flush`] Added a new option ``--recursive dependent``

  * We can all of the specified flush target object, child objects, corresponding table of an index column and corresponding index column are flush target objects.

Fixes
^^^^^

* Fixed "unknown type name 'bool'" compilation error in some environments.

* Fixed a bug that incorrect output number over Int32 by command of execute via mruby (e.g. ``logical_select``, ``logical_range_filter``, ``logical_count``, etc.). [GitHub#936] [Patch by HashidaTKS]

Thanks
^^^^^^

* HashidaTKS

.. _release-9-0-1:

Release 9.0.1 - 2019-03-29
--------------------------

Improvements
^^^^^^^^^^^^

* Added support to acccept null for vector value.

  * You can use `select ... --columns[vector].flags COLUMN_VECTOR --columns[vector].value "null"`

* [:doc:`/reference/commands/dump`] Translated document into English.

* Added more checks and logging for invalid indexes. It helps to clarify the index related bugs.

* Improved an explanation about ``GRN_TABLE_SELECT_ENOUGH_FILTERED_RATIO`` behavior in news at :ref:`release-8-0-6`.

* [:doc:`/reference/commands/select`] Added new argument ``--load_table``, ``--load_columns`` and ``--load_values``.

  * You can store a result of ``select`` in a table that specifying ``--load_table``.

  * ``--load_values`` option specifies columns of result of ``select``.

  * ``--load_columns`` options specifies columns of table that specifying ``--load_table``.

  * In this way, you can store values of columns that specifying with ``--load_values`` into columns that specifying with ``--load_columns``.

* [:doc:`/reference/commands/select`] Added documentation about ``load_table``, ``load_columns`` and ``load_values``.

* [:doc:`/reference/commands/load`] Added supoort to display a table of load destination in a query log.

  * A name of table of load destination display as string in ``[]`` as below.

  * ``:000000000000000 load(3): [LoadedLogs][3]``

* Added a new API:

  * ``grn_ii_get_flags()``

  * ``grn_index_column_diff()``

  * ``grn_memory_get_usage()``

* Added ``index_column_diff`` command to check broken index column. If you want to log progress of command execution, set log level to debug.

Fixes
^^^^^

* [:doc:`/reference/functions/snippet_html`] Changed to return an empty vector for no match.

  * In such a case, an empty vector ``[]`` is returned instead of ``null``.

* Fixed a warning about possibility of counting threads overflow.
  In real world, it doesn't affect user because enourmous number of threads is not used. [GitHub#904]

* Fixed build error on macOS [GitHub#909] [Reported by shiro615]

* Fixed a stop word handling bug.

  * This bug occurs when we set the first token as a stop word in our query.

  * If this bug occurs, our search query isn't hit.

* [:doc:`/reference/api/global_configurations`] Fixed a typo about parameter name of ``grn_lock_set_timeout``.

* Fixed a bug that deleted records may be matched because of updating indexes incorrectly.

  * It may occure when large number of records is added or deleted.

* Fixed a memory leak when ``logical_range_filter`` returns no records. [GitHub#911] [Patch by HashidaTKS]

* Fixed a bug that query will not match because of loading data is not normalized correctly.
  [PGroonga#GitHub#93, GitHub#912,GitHub#913] [Reported by kamicup and dodaisuke]

  * This bug occurs when load data contains whitespace after KATAKANA and ``unify_kana`` option is used for normalizer.

* Fixed a bug that an indexes is broken during updating indexes.

  * It may occurs when repeating to add large number of records or delete them for a long term.

* Fixed a crash bug that allocated working area is not enough size when updating indexes.

Thanks
^^^^^^

* shiro615

* HashidaTKS

* kamicup

* dodaisuke

.. _release-9-0-0:

Release 9.0.0 - 2019-02-09
--------------------------

This is a major version up! But It keeps backward compatibility.
You can upgrade to 9.0.0 without rebuilding database.

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/tokenizers`] Added a new tokenizer ``TokenPattern``.

  * You can extract tokens by regular expression.

    * This tokenizer extracts only token that matches the regular expression.

  * You can also specify multiple patterns of regular expression.

* [:doc:`/reference/tokenizers`] Added a new tokenizer ``TokenTable``.

  * You can extract tokens by a value of columns of existing a table.

* [:doc:`/reference/commands/dump`] Added support for dumping binary data.

* [:doc:`/reference/commands/select`] Added support for similer search against index column.

  * If you have used multi column index, you can similar search against all source columns by this feature.

* [:doc:`/reference/normalizers`] Added new option ``remove_blank`` for ``NormalizerNFKC100``.

  * This option remove white spaces.

* [:doc:`/reference/executables/groonga`] Improve display of thread id in log.

  * Because It was easy to confuse thread id and process id on Windows version, it made clear which is a thread id or a process id.

.. _release-8-1-1:

Release 8.1.1 - 2019-01-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/logical_select`] Added new argument ``--load_table``, ``--load_columns`` and ``--load_values``.

  * You can store a result of ``logical_select`` in a table that specifying ``--load_table``.

  * ``--load_values`` option specifies columns of result of ``logical_select``.

  * ``--load_columns`` options specifies columns of table that specifying ``--load_table``.

  * In this way, you can store values of columns that specifying with ``--load_values`` into columns that specifying with ``--load_columns``.

* Improve error log when update error of index.

  * Added more information in the log.

    * For example, output source buffer and chunk when occur merge of posting lists error.
    * Also, outputting the log a free space size of a buffer and request size of a buffer when occurs error of allocating a buffer.

* [:doc:`/reference/executables/groonga`] Added a new option ``--log-flags``.

  * We can specify output items of a log of the Groonga.

  * We can output as below items.

    * Timestamp
    * Log message
    * Location(the location where the log was output)
    * Process id
    * Thread id

  * We can specify prefix as below.

    * ``+``

      * This prefix means that "add the flag".

    * ``-``

      * This prefix means that "remove the flag".

    * No prefix means that "replace existing flags".

  * Specifically, we can specify flags as below.

    * ``none``

      * Output nothing into the log.

    * ``time``

      * Output a timestamp into the log.

    * ``message``

      * Output log messages into the log.

    * ``location``

      * Output the location where the log was output( a file name, a line and a function name) and process id.

    * ``process_id``

      * Output a process id into the log.

    * ``pid``

      * This flag is an alias of ``process_id``.

    * ``thread_id``

      * Output thread id into the log.

    * ``all``

      * This flag specifies all flags except ``none`` and ``default`` flags.

    * ``default``

      * Output a timestamp and log messages into the log.

  * We can also specify multiple log flags by separating flags with ``|``.

Fixes
^^^^^

* Fixed a memory leak when occurs index update error.

* [:doc:`/reference/normalizers`] Fixed a bug that stateless normalizers and stateful normalizers return wrong results when we use them at the same time.

    * Stateless normalizers are below.

      * ``unify_kana``
      * ``unify_kana_case``
      * ``unify_kana_voiced_sound_mark``
      * ``unify_hyphen``
      * ``unify_prolonged_sound_mark``
      * ``unify_hyphen_and_prolonged_sound_mark``
      * ``unify_middle_dot``

    * Stateful normalizers are below.

      * ``unify_katakana_v_sounds``
      * ``unify_katakana_bu_sound``
      * ``unify_to_romaji``

.. _release-8-1-0:

Release 8.1.0 - 2018-12-29
--------------------------

Improvements
^^^^^^^^^^^^

* [httpd] Updated bundled nginx to 1.15.8.

Fixes
^^^^^

* Fixed a bug that unlock against DB is always executed after flush when after execute a ``io_flush`` command.

* Fixed a bug that ``reindex`` command doesn't finish when execute a ``reindex`` command against table that has record that has not references.

.. _release-8-0-9:

Release 8.0.9 - 2018-11-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/tokenizers`] Improved that output a tokenizer name in error message when create tokenizer fail.

* [:doc:`/reference/tokenizers`][TokenDelimit] Supported that customizing delimiter of a token.

  * You can use token other than whitespace as a token of delimiter.

* [:doc:`/reference/tokenizers`][TokenDelimit] Added new option ``pattern``.

  * You can specify delimiter with regular expression by this option.

* [:doc:`/reference/tokenizers`] Added force_prefix_search value to each token information.

  * "force_prefix" is kept for backward compatibility.

* [:doc:`/reference/token_filters`] Added built-in token filter ``TokenFilterNFKC100``.

  * You can convert katakana to hiragana like NormalizerNFKC100 with a ``unify_kana`` option.

* [:doc:`/reference/token_filters`][TokenFilterStem] Added new option ``algorithm``.

  * You can also stem language other than English(French, Spanish, Portuguese, Italian, Romanian, German, Dutch, Swedish, Norwegian, Danish, Russian, Finnish) by this option.

* [:doc:`/reference/token_filters`][TokenFilterStopWord] Added new option ``column``.

  * You can specify stop word in a column other than is_stop_word column by this option.

* [:doc:`/reference/commands/dump`] Supported output options of token filter options.

  * If you specify a tokenizer like ``TokenNgram`` or ``TokenMecab`` etc that has options, you can output these options with ``table_list`` command.

* [:doc:`/reference/commands/truncate`] Supported a table that it has token filter option.

  * You can ``truncate`` even a tabel that it has token filter like ``TokenFilterStem`` or ``TokenStopWord`` that has options.

* [:doc:`/reference/commands/schema`] Support output of options of token filter.

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that ``unify_to_romaji`` option.

  * You can normalize hiragana and katakana to romaji by this option.

* [query-log][show-condition] Supported "func() > 0" case.

* [Windows] Improved that ensure flushing on unmap.

* Improved error message on opening input file error.

* [httpd] Updated bundled nginx to 1.15.7.

  * contains security fix for CVE-2018-16843 and CVE-2018-16844.

Fixes
^^^^^

* Fixed a memory leak when evaluating window function.

* [:doc:`/reference/executables/groonga-httpd`] Fixed bug that log content may be mixed.

* Fixed a bug that generates invalid JSON when occurs error of slice on output_columns.

* Fixed a memory leak when getting nested reference vector column value.

* Fixed a crash bug when outputting warning logs of index corruption.

* Fix a crash bug when temporary vector is reused in expression evaluation.

  * For example, crash when evaluating an expression that uses a vector as below.

  ``_score = _score + (vector_size(categories) > 0)``

* Fix a bug that hits a value of vector columns deleted by a delete command.[GitHub PGroonga#85][Reported by dodaisuke]

Thanks
^^^^^^

* dodaisuke

.. _release-8-0-8:

Release 8.0.8 - 2018-10-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/table_list`] Supported output options of default tokenizer.

  * If you specify a tokenizer like ``TokenNgram`` or ``TokenMecab`` etc that has options, you can output these options with ``table_list`` command.

* [:doc:`/reference/commands/select`] Supported normalizer options in sequential match with ``record @ 'query'``.

* [:doc:`/reference/commands/truncate`] Supported a table that it has tokenizer option.

  * You can ``truncate`` even a tabel that it has tokenizer like ``TokenNgram`` or ``TokenMecab`` etc that has options.

* [:doc:`/reference/tokenizers`][TokenMecab] Added new option ``target_class``

  * This option searches a token of specifying a part-of-speech. For example, you can search only a noun.
  * This option can also specify subclasses and exclude or add specific part-of-speech of specific using ``+`` or ``-``. So, you can also search except a pronoun as below.

    ``'TokenMecab("target_class", "-名詞/代名詞", "target_class", "+")'``

* [:doc:`/reference/commands/io_flush`] Supported locking of a database during a ``io_flush``.

  * Because Groonga had a problem taht is a crash when deleteing a table of a target of a ``io_flush`` during execution of a ``io_flush``.

* [:doc:`/reference/functions/cast_loose`] Added a new function ``cast_loose``.

  * This function cast to a type to specify. If a value to specify can't cast, it become to a default value to specify.

* Added optimize the order of evaluation of a conditional expression.(experimental)

  * You can active this feature by setting environment value as below.

    ``GRN_EXPR_OPTIMIZE=yes``

* Supported ``(?-mix:XXX)`` form for index searchable regular expression. [groonga-dev,04683][Reported by Masatoshi SEKI]

  * ``(?-mix:XXX)`` form treats the same as XXX.

* [httpd] Updated bundled nginx to 1.15.5.

* Supported Ubuntu 18.10 (Cosmic Cuttlefish)

Fixes
^^^^^

* Fixed a bug that the Groonga GQTP server may fail to accept a new connection. [groonga-dev,04688][Reported by Yutaro Shimamura]

  * It's caused when interruption client process without using quit.

Thanks
^^^^^^

* Masatoshi SEKI
* Yutaro Shimamura

.. _release-8-0-7:

Release 8.0.7 - 2018-09-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/tokenizers`][TokenMecab] support outputting metadata of Mecab.

  * Added new option ``include_class`` for ``TokenMecab``.

    This option outputs ``class`` and ``subclass`` in Mecab's metadata.

  * Added new option ``include_reading`` for ``TokenMecab``.

    This option outputs ``reading`` in Mecab's metadata.

  * Added new option ``include_form`` for ``TokenMecab``.

    This option outputs ``inflected_type``, ``inflected_form`` and ``base_form`` in Mecab's metadata.

  * Added new option ``use_reading`` for ``TokenMecab``.

    This option supports a search by kana.

    This option is useful for countermeasure of orthographical variants because it searches with kana.

* [plugin] Groonga now can grab plugins from multiple directories.

  You can specify multiple directories to ``GRN_PLUGINS_PATH`` separated with ":" on non Windows, ";" on Windows.

  ``GRN_PLUGINS_PATH`` has high priority than the existing ``GRN_PLUGINS_DIR``.
  Currently, this option is not supported Windows.

* [:doc:`/reference/tokenizers`][TokenNgram] Added new option ``unify_alphabet`` for ``TokenNgram``.

  If we use ``unify_alphabet`` as ``false``, ``TokenNgram`` uses bigram tokenize method for ASCII character.

* [:doc:`/reference/tokenizers`][TokenNgram] Added new option ``unify_symbol`` for ``TokenNgram``.

  ``TokenNgram("unify_symbol", false)`` is same behavior of ``TokenBigramSplitSymbol``.

* [:doc:`/reference/tokenizers`][TokenNgram] Added new option ``unify_digit`` for ``TokenNgram``.

  If we use ``unify_digit`` as ``false``, If we set false, ``TokenNgram`` uses bigram tokenize method for digits.

* [httpd] Updated bundled nginx to 1.15.4.

Fixes
^^^^^

* Fixed wrong score calculations on some cases.

  * It's caused when adding, multiplication or division numeric to a bool value.
  * It's caused when comparing a scalar and vector columns using ``!=`` or ``==``.

.. _release-8-0-6:

Release 8.0.6 - 2018-08-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/tokenizers`][TokenMecab] add ``chunked_tokenize`` and ``chunk_size_threshold`` options.

* [optimizer] support estimation for query family expressions.
  It will generate more effective execution plan with query family expressions such as ``column @ query``, ``column @~ pattern`` and so on.

* [optimizer] plug-in -> built-in
  It's disabled by default for now.
  We can enable it by defining ``GRN_EXPR_OPTIMIZE=yes`` environment variable or using ``expression_rewriters`` table as before.

* Enable sequential search for enough filtered case by default.
  If the current result is enough filtered, sequential search is faster than index search.
  If the current result has only 1% records of all records in a table and less than 1000 records, sequential search is used even when index search is available.

  Cullently, this optimization is applied when search by ``==``, ``>``, ``<``, ``>=``, or ``<=``.

  When a key of a table that has columns specified by the filter is ``ShortText``, you must set ``NormalizerAuto`` to normalizer of the table to apply this optimization.

  You can disable this feature by ``GRN_TABLE_SELECT_ENOUGH_FILTERED_RATIO=0.0`` environment variable.

* [load] improve error message.
  Table name is included.

* [load] add ``lock_table`` option.
  If ``--lock_table yes`` is specified, ``load`` locks the target table while updating columns and applying ``--each``.
  This option avoids ``load`` and ``delete`` conflicts but it'll reduce load performance.

* [vector_find] avoid to crash with unsupported modes

Fixes
^^^^^

* [index] fix a bug that offline index construction for text vector with ``HASH_KEY``.
  It creates index with invalid section ID.

* Fix a bug that ``--match_columns 'index[0] || index[9]'`` uses wrong section.

* [highlighter] fix a wrong highlight bug
  It's caused when lexicon is hash table and keyword is less than N of N-gram.

* [mruby] fix a bug that real error is hidden.
  mruby doesn't support error propagation by no argument raise.
  https://github.com/mruby/mruby/issues/290

* [:doc:`/reference/tokenizers`][TokenNgram loose]: fix a not found bug when query has only loose types.
  ``highlight_html()`` with lexicon was also broken.

* Fix a bug that text->number cast ignores trailing garbage.
  "0garbage" should be cast error.

* Fix an optimization bug for ``reference_column >= 'key_value'`` case

.. _release-8-0-5:

Release 8.0.5 - 2018-07-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/grn_expr/script_syntax`] Added complementary explain about similar search against Japanese documents.
  [GitHub#858][Patch by Yasuhiro Horimoto]

* [:doc:`/reference/functions/time_classify_day_of_week`] Added a new API: ``time_classify_day_of_week()``.

* Suppressed a warning with ``-fstack-protector``.
  Suggested by OBATA Akio.

* Added a new API: ``time_format_iso8601()``.

* Exported a struct ``grn_raw_string``.

* Added a new API: ``grn_obj_clear_option_values()``.
  It allows you to clear option values on remove (for persistent) / close (for temporary.)

* [log] Reported index column name for error message ``[ii][update][one]``.

* [httpd] Updated bundled nginx to 1.15.2.

* [:doc:`/install/ubuntu`] Dropped Ubuntu 17.10 (Artful Aardvark) support.
  It has reached EOL at July 19, 2018.

* [:doc:`/install/debian`] Dropped jessie support.
  Debian's security and release team will no longer produce updates for jessie.

Fixes
^^^^^

* Fixed returning wrong result after unfinished ``/d/load`` data by POST.

* Fixed wrong function call around KyTea.

* [:doc:`/reference/executables/grndb`] Added a missing label for the ``--force-truncate`` option.

* Fixed crash on closing of a database, when a normalizer provided by a plugin (ex. ``groonga-normalizer-mysql``) is used with any option.

* Fixed a bug that normalizer/tokenizer options may be ignored.
  It's occurred when the same object ID is reused.

.. _release-8-0-4:

Release 8.0.4 - 2018-06-29
--------------------------

Improvements
^^^^^^^^^^^^

* [log] Add sub error for error message ``[ii][update][one]``.

* Added a new API: ``grn_highlighter_clear_keywords()``.

* Added a new predicate: ``grn_obj_is_number_family_bulk()``.

* Added a new API: ``grn_plugin_proc_get_value_mode()``.

* [:doc:`reference/functions/vector_find`] Added a new function ``vector_find()``.

* Suppress memcpy warnings in msgpack.

* Updated mruby from 1.0.0 to 1.4.1.

* [doc][:doc:`/reference/api/grn_obj`] Added API reference for ``grn_obj_is_index_column()``.

* [windows] Suppress printf format warnings.

* [windows] Suppress warning by msgpack.

* [:doc:`/reference/api/grn_obj`][:doc:`/reference/api/plugin`] Added encoding converter.
  rules:

  * grn_ctx::errbuf: grn_encoding

  * grn_logger_put: grn_encoding

  * mruby: UTF-8

  * path: locale

* [mrb] Added ``LocaleOutput``.

* [windows] Supported converting image path to grn_encoding.

* [:doc:`/reference/tokenizers`][TokenMecab] Convert error message encoding.

* [:doc:`/reference/window_functions/window_sum`] Supported dynamic column as a target column.

* [doc][:doc:`/reference/api/grn_obj`] Added API reference for ``grn_obj_is_vector_column()``.

* [:doc:`/reference/commands/column_create`] Added more validations.

  * 1: Full text search index for vector column must have ``WITH_SECTION`` flag.
    (Note that TokenDelmit with ``WITH_POSITION`` without ``WITH_SECTION`` is permitted.
    It's useful pattern for tag search.)

  * 2: Full text search index for vector column must not be multi column index.
    detail: https://github.com/groonga/groonga/commit/08e2456ba35407e3d5172f71a0200fac2a770142

* [:doc:`/reference/executables/grndb`] Disabled log check temporarily.
  Because it's not completed yet.

Fixes
^^^^^

* [:doc:`reference/functions/sub_filter`] Fixed too much score with a too filtered case.

* Fixed build error if KyTea is installed.

* [:doc:`/reference/executables/grndb`] Fixed output channel.

* [query-log][show-condition] Maybe fixed a crash bug.

* [highlighter][lexicon] Fixed a not highlighted bug.
  The keyword wasn't highlighted if keyword length is less than N ("N"-gram.
  In many cases, it's Bigram so "less than 2").

* [windows] Fixed a base path detection bug.
  If system locale DLL path includes 0x5c (``\`` in ASCII) such as "U+8868
  CJK UNIFIED IDEOGRAPH-8868" in CP932, the base path detection is buggy.

* [:doc:`/reference/tokenizers`][TokenNgram] Fixed wrong first character length.
  It's caused for "PARENTHESIZED IDEOGRAPH" characters such as
  "U+3231 PARENTHESIZED IDEOGRAPH STOCK".

.. _release-8-0-3:

Release 8.0.3 - 2018-05-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/functions/highlight_html`] Support highlight of results of
  the search by ``NormalizerNFKC100`` or ``TokenNgram``.

* [:doc:`/reference/tokenizers`] Added new option for ``TokenNgram`` that
  ``report_source_location option`` .
  This option used when highlighting with ``highlight_html`` use a lexicon.

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that
  ``unify_middle_dot option``.
  This option normalizes middle dot. You can search with or without ``・``
  (middle dot) and regardless of ``・`` position.

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that
  ``unify_katakana_v_sounds option``.
  This option normalizes ``ヴァヴィヴヴェヴォ`` (katakana) to ``バビブベボ`` (katakana).
  For example, you can search ``バイオリン`` (violin) in ``ヴァイオリン`` (violin).

* [:doc:`/reference/normalizers`] Added new option for ``NormalizerNFKC100`` that
  ``unify_katakana_bu_sound option``.
  This option normalizes ``ヴァヴィヴゥヴェヴォ`` (katakana) to ``ブ`` (katakana).
  For example, you can search ``セーブル`` (katakana) and ``セーヴル`` in
  ``セーヴェル`` (katakana).

* [:doc:`reference/functions/sub_filter`] Supported ``sub_filter`` optimization
  for the too filter case.
  this optimize is valid when records are enough narrowed down before
  ``sub_filter`` execution as below.

* [:doc:`/reference/executables/groonga-httpd`] Made all workers context address
  to unique.
  context address is ``#{ID}`` of below query log.

  | #{TIME_STAMP}|#{MESSAGE}
  | #{TIME_STAMP}|#{ID}|>#{QUERY}
  | #{TIME_STAMP}|#{ID}|:#{ELAPSED_TIME} #{PROGRESS}
  | #{TIME_STAMP}|#{ID}|<#{ELAPSED_TIME} #{RETURN_CODE}

* [:doc:`/reference/commands/delete`] Added new options that ``limit``.
  You can limit the number of delete records as below example.
  ``delete --table Users --filter '_key @^ "b"' --limit 4``

* [httpd] Updated bundled nginx to 1.14.0.

Fixes
^^^^^

* [:doc:`/reference/commands/logical_select`] Fixed memory leak when an error occurs
  in filtered dynamic columns.

* [:doc:`/reference/commands/logical_count`] Fixed memory leak on initial dynamic
  column error.

* [:doc:`/reference/commands/logical_range_filter`] Fixed memory leak when an error
  occurs in dynamic column evaluation.

* [:doc:`/reference/tokenizers`] Fixed a bug that the wrong ``source_offset`` when a
  loose tokenizing such as ``loose_symbol`` option.

* [:doc:`/reference/normalizers`] Fixed a bug that FULLWIDTH LATIN CAPITAL LETTERs
  such as ``U+FF21 FULLWIDTH LATIN CAPITAL LETTER A`` aren't normalized to LATIN SMALL
  LETTERs such as ``U+0061 LATIN SMALL LETTER A``.
  If you have been used ``NormalizerNFKC100`` , you must recreate your indexes.

.. _release-8-0-2:

Release 8.0.2 - 2018-04-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/executables/grndb`][:ref:`grndb-force-truncate`] Improved
  ``grndb recover --force-truncate`` option that it can be truncated even if
  locks are left on the table.

* [:doc:`/reference/commands/logical_range_filter`] Added ``sort_keys`` option.

* Added a new function ``time_format()``.
  You can specify time format against a column of ``Time`` type.
  You can specify with use format of ``strftime`` .

* [:doc:`/reference/tokenizers`] Support new tokenizer ``TokenNgram``.
  You can change its behavior dynamically via options.
  Here is a list of available options:

    * ``n`` : "N" of Ngram. For example, "3" for trigram.
    * ``loose_symbol`` : Tokenize keywords including symbols, to be searched
      by both queries with/without symbols. For example, a keyword
      "090-1111-2222" will be found by any of "09011112222", "090", "1111",
      "2222" and "090-1111-2222".
    * ``loose_blank`` : Tokenize keywords including blanks, to be searched
      by both queries with/without blanks. For example, a keyword
      "090 1111 2222" will be found by any of "09011112222", "090", "1111",
      "2222" and "090 1111 2222".
    * ``remove_blank`` : Tokenize keywords including blanks, to be searched
      by queries without blanks. For example, a keyword "090 1111 2222" will
      be found by any of "09011112222", "090", "1111" or "2222". Note that
      the keyword won't be found by a query including blanks like
      "090 1111 2222".

* [:doc:`/reference/normalizers`] Support new normalizer "NormalizerNFKC100" based on Unicode NFKC (Normalization Form Compatibility Composition) for Unicode 10.0.

* [:doc:`/reference/normalizers`] Support options for "NormalizerNFKC51" and "NormalizerNFKC100" normalizers.
  You can change their behavior dynamically.
  Here is a list of available options:

    * ``unify_kana`` : Same pronounced characters in all of full-width
      Hiragana, full-width Katakana and half-width Katakana are regarded as
      the same character.
    * ``unify_kana_case`` : Large and small versions of same letters in all of
      full-width Hiragana, full-width Katakana and half-width Katakana are
      regarded as the same character.
    * ``unify_kana_voiced_sound_mark`` : Letters with/without voiced sound
      mark and semi voiced sound mark in all of full-width Hiragana,
      full-width Katakana and half-width Katakana are regarded as the same
      character.
    * ``unify_hyphen`` : The characters like hyphen are regarded as the hyphen.
    * ``unify_prolonged_sound_mark`` : The characters like prolonged sound mark
      are regarded as the prolonged sound mark.
    * ``unify_hyphen_and_prolonged_sound_mark`` : The characters like hyphen
      and prolonged sound mark are regarded as the hyphen.

* [:doc:`/reference/commands/dump`] Support output of tokenizer's options and
  normalizer's options. Groonga 8.0.1 and earlier versions cannot import dump
  including options for tokenizers or normalizers generated by Groonga 8.0.2
  or later, and it will occurs error due to unsupported information.

* [:doc:`/reference/commands/schema`] Support output of tokenizer's options and
  normalizer's options. Groonga 8.0.1 and earlier versions cannot import schema
  including options for tokenizers or normalizers generated by Groonga 8.0.2
  or later, and it will occurs error due to unsupported information.

* Supported Ubuntu 18.04 (Bionic Beaver)

Fixes
^^^^^

* Fixed a bug that unexpected record is matched with space only query.
  [groonga-dev,04609][Reported by satouyuzh]

* Fixed a bug that wrong scorer may be used.
  It's caused when multiple scorers are used as below.
  ``--match_columns 'title || scorer_tf_at_most(content, 2.0)'``.

* Fixed a bug that it may also take so much time to change "thread_limit".

Thanks
^^^^^^

* satouyuzh

.. _release-8-0-1:

Release 8.0.1 - 2018-03-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/log`] Show ``filter`` conditions in query log.
  It's disabled by default. To enable it, you need to set an environment
  variable ``GRN_QUERY_LOG_SHOW_CONDITION=yes``.

* Install ``*.pdb`` into the directory where ``*.dll`` and ``*.exe``
  are installed.

* [:doc:`/reference/commands/logical_count`] Support ``filtered``
  stage dynamic columns.

* [:doc:`/reference/commands/logical_count`]
  [:ref:`logical-count-post-filter`] Added a new filter timing.
  It's executed after ``filtered`` stage columns are generated.

* [:doc:`/reference/commands/logical_select`]
  [:ref:`logical-select-post-filter`] Added a new filter timing.
  It's executed after ``filtered`` stage columns are generated.

* Support LZ4/Zstd/zlib compression for vector data.

* Support alias to accessor such as ``_key``.

* [:doc:`/reference/commands/logical_range_filter`] Optimize
  window function for large result set.
  If we find enough matched records, we don't apply window function
  to the remaining windows.

  TODO: Disable this optimization for small result set if its overhead
  is not negligible. The overhead is not evaluated yet.

* [:doc:`/reference/commands/select`] Added ``match_escalation`` parameter.
  You can force to enable match escalation by ``--match_escalation yes``.
  It's stronger than ``--match_escalation_threshold 99999....999``
  because ``--match_escalation yes`` also works with
  ``SOME_CONDITIONS && column @ 'query'``.
  ``--match_escalation_threshold`` isn't used in this case.

  The default is ``--match_escalation auto``. It doesn't change the
  current behavior.

  You can disable match escalation by ``--match_escalation no``.
  It's the same as ``--match_escalation_threshold -1``.

* [httpd] Updated bundled nginx to 1.13.10.

Fixes
^^^^^

* Fixed memory leak that occurs when a prefix query doesn't match any token.
  [GitHub#820][Patch by Naoya Murakami]

* Fixed a bug that a cache for different databases is used when
  multiple databases are opened in the same process.

* Fixed a bug that a wrong index is constructed.
  This occurs only when the source of a column is a vector column and
  ``WITH_SECTION`` isn't specified.

* Fixed a bug that a constant value can overflow or underflow in
  comparison (>,>=,<,<=,==,!=).

Thanks
^^^^^^

* Naoya Murakami

.. _release-8-0-0:

Release 8.0.0 - 2018-02-09
--------------------------

This is a major version up! But It keeps backward compatibility.
You can upgrade to 8.0.0 without rebuilding database.

Improvements
^^^^^^^^^^^^

* [:doc:`/reference/commands/select`] Added ``--drilldown_adjuster`` and
  ``--drilldowns[LABEL].adjuster``.
  You can adjust score against result of drilldown.

* [:ref:`online-index-construction`] Changed environment variable name
  ``GRN_II_REDUCE_EXPIRE_ENABLE`` to ``GRN_II_REDUCE_EXPIRE_THRESHOLD``.

  ``GRN_II_REDUCE_EXPIRE_THRESHOLD=0 == GRN_II_REDUCE_EXPIRE_ENABLE=no``.
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD=-1`` uses
  ``ii->chunk->max_map_seg / 2`` as threshold.
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD > 0`` uses
  ``MIN(ii->chunk->max_map_seg / 2, GRN_II_REDUCE_EXPIRE_THRESHOLD)``
  as threshold.
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD=32`` is the default.

* [:doc:`/reference/functions/between`] Accept ``between()`` without borders.
  If the number of arguments passed to ``between()`` is 3, the 2nd and 3rd
  arguments are handled as the inclusive edges. [GitHub#685]

Fixes
^^^^^

* Fixed a memory leak for normal hash table.
  [GitHub:mroonga/mroonga#190][Reported by fuku1]

* Fix a memory leak for normal array.

* [:doc:`/reference/commands/select`] Stopped to cache when ``output_columns``
  uses not stable function.

* [Windows] Fixed wrong value report on ``WSASend`` error.

Thanks
^^^^^^

* fuku1

.. _release-7-1-1:

Release 7.1.1 - 2018-01-29
--------------------------

Improvements
^^^^^^^^^^^^

* [:doc:`/install/ubuntu`] Dropped Ubuntu 17.04 (Zesty Zapus) support.
  It has reached EOL at Jan 13, 2018.

* Added quorum match support.
  You can use quorum match in both script syntax and query syntax.
  [groonga-talk,385][Suggested by 付超群]

  TODO: Add documents for quorum match syntax and link to them.

* Added custom similarity threshold support in script syntax.
  You can use custom similarity threshold in script syntax.

  TODO: Add document for the syntax and link to it.

* [:doc:`/reference/executables/grndb`][:ref:`grndb-force-lock-clear`]
  Added ``--force-lock-clear`` option. With this option, ``grndb``
  forces to clear locks of database, tables and data columns. You can
  use your database again even if locks are remained in database,
  tables and data columns.

  But this option very risky. Normally, you should not use it. If your
  database is broken, your database is still broken. This option just
  ignores locks.

* [:doc:`/reference/commands/load`] Added surrogate pairs support in
  escape syntax. For example, ``\uD83C\uDF7A`` is processed as ``🍺``.

* [Windows] Changed to use sparse file on Windows. It reduces disk
  space and there are no performance demerit.

* [:ref:`online-index-construction`] Added
  ``GRN_II_REDUCE_EXPIRE_THRESHOLD`` environment variable to control
  when memory maps are expired in index column. It's ``-1`` by
  default. It means that expire timing is depends on index column
  size. If index column is smaller, expire timing is more. If index
  column is larger, expire timing is less.

  You can use the previous behavior by ``0``. It means that Groonga
  always tries to expire.

* [:doc:`/reference/commands/logical_range_filter`]
  [:ref:`logical-range-filter-post-filter`] Added a new filter timing.
  It's executed after ``filtered`` stage generated columns are generated.

Fixes
^^^^^

* Reduced resource usage for creating index for reference vector.
  [GitHub#806][Reported by Naoya Murakami]

* [:doc:`/reference/commands/table_create`] Fixed a bug that a table
  is created even when ``token_filters`` is invalid.
  [GitHub#266]

Thanks
^^^^^^

* 付超群

* Naoya Murakami

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

* [:doc:`/reference/executables/groonga-server-http`] The server
  executed by ``groonga -s`` ensure stopping by C-c.

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

* [httpd] Updated bundled nginx to 1.13.8.

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
  Groonga will crash.
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
