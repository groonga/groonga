.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

.. _release-5-0-1:

Release 5.0.1 - 2015/03/29
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

* Added more strict check for invalid drilldown parameter.
* Added :c:func:`grn_ctx_get_all_tables()`. [Suggested by Masatoshi
  Teruya]
* ??? Supported sort by index value.
* Supported to customize score function.
* Added some functions to distinct the type of :c:type:`grn_obj` for
  API users.
* Supported scorer in :ref:`select-match-columns`.
* [experimental] Supported skip posting list when searching popular
  term and rare term at the same time. It will improve
  performance. Use ``GRN_II_CURSOR_SET_MIN_ENABLE`` environment
  variable.
* [doc] Added :doc:`/reference/functions/in_values` document.
* [doc] Added :doc:`/reference/commands/logical_count` document.
* [mruby] Added #inspect. Is is useful for debugging.
* Added :doc:`/reference/scorers/scorer_tf_at_most` scorer. It
  limits not to exceed specified score regardless of term frequency.
* [mruby] Supported estimating size for index search. Added some
  ``grn_ii_estimate_*`` functions to support it.
* Added missing NormalizerAuto availability check. [GitHub#283]
  [Reported by Tasuku SUENAGA]
* Dropped Visual Studio 2010 support.
* [experimental][mecab] Supported chunked tokenization. This feature
  is a workaround for MeCab's "too long sentense" error.  Specify
  ``GRN_MECAB_CHUNKED_TOKENIZE_ENABLED=yes`` environment variable to
  enable it. By this configuration, Groonga splits a long text (8192
  bytes over text by default) into small chunks and passes each chunk
  to MeCab. so above error doesn't occur. Additionally, you can
  customize chunk threshold bytes by
  ``GRN_MECAB_CHUNK_SIZE_THRESHOLD`` environment variable. Note that
  ``,``, ``.``, ``!``, ``?``, ``U+3001 IDEOGRAPHIC COMMA``, ``U+3002
  IDEOGRAPHIC FULL STOP``, ``U+FF01 FULLWIDTH EXCLAMATION MARK`` and
  ``U+FF1F FULLWIDTH QUESTION MARK`` are treated as chunk delimiter
  characters.
* Supported scorer with weight. Use ``SCORER(...) * 10'`` for example.
* Supported ``--pid-file`` in server mode for groonga command.
* [groonga-httpd] Supported graceful stop to clean Groonga. It doesn't
  terminate the open connections immediately.
* Supported REGEXP. Use ``COLUMN_NAME:~REGEXP`` in query syntax,
  ``TARGET @~ REGEXP`` in script syntax.
* [experimental] Added :doc:`/reference/commands/plugin_unregister`
  command.
* [http] Supported new line and "," for chunk separator in POST
  data. It decreases buffer size, as a result, it reduces load time
  without these separators.
* Added ``TokenRegexp`` tokenizer. See
  :doc:`/reference/regular_expression` about details.
* [doc] Added :doc:`/reference/tokenizers` document.
* Improved POSIX.2 compatibility by using ``.`` as bash's "source"
  command replacement. [GitHub#317] [Patch by Jun Kuriyama]
* [windows] Changed to default IO version 1. It reduces disk usage on
  Windows. [groonga-dev,03118] [Tested by ongaeshi]
* [httpd] Updated bundled Nginx version to the latest mainline
  (1.7.11).

Fixes
^^^^^

* Fixed not to use obsolete ``--address`` parameter in
  groonga.conf. Use ``--bind-address`` instead.  [Groonga-talk]
  [Reported by Dewangga]
* Fixed a bug that array can't be truncated properly if TABLE_NO_KEY
  is specified for table.
* Fixed to return proper mime-type.
* [mecab] Fixed not to report needless "empty token" or "ignore empty
  token" multiple times.
* Fixed a bug that wrong section is used. It is known that this bug is
  occurred following the three conditions are met. The first one is:
  multiple indexes are available. The second one is: the first defined
  index and/or the last defined index are multi-column indexes. The
  last one is: when both of the first define index and the last
  defined index are multi-column indexes.
* Fixed a bug that ``stop_word`` plugin use offline index
  construction. [GitHub#296] [Patch by Naoya Murakami]
* Fixed a bug that "groonga /tmp/db XXX" always returns 0 as exit
  code.
* Fixed a bug that plugin path may be broken when two or more plugins
  registered. [Reported by Naoya Murakami]
* Fixed a bug that "Lexicon.index.section_name" doesn't work. This bug
  happen when source column specified by "section_name" has two or
  more indexes. [Reported by Naoya Murakami]

Thanks
^^^^^^

* Masatoshi Teruya
* Tasuku SUENAGA
* Dewangga
* Jun Kuriyama
* ongaeshi
* Naoya Murakami

.. _release-5-0-0:

Release 5.0.0 - 2015/02/09
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
