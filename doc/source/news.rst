.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

.. _release-4-0-6:

Release 4.0.6 - 2014/09/29
--------------------------

Improvements
^^^^^^^^^^^^

* [deb] Updated to follow the rule about machine-readable
  debian/copyright file. [Reported by Hideki Yamane]
* [deb][rpm] Changed to use signal instead of HTTP to control nginx.
  This way meets nginx style init script.
* [doc] Updated English documentation about [ :doc:`/reference/executables` ].
  [GitHub#196] [Patch by Masafumi Yokoyama]
* [OS X] Added missing sphinx error message for Homebrew users.
  [GitHub#198] [Patch by cosmo0920]
* Exported :c:func:`grn_expr_alloc_const` which allocates constant object.
  It is exported for mruby.
* [mruby] Query optimizer by mruby became a practical for some range select query.
  For example, mruby optimizer overhead is negligible enough for query such as
  "between(X, 0, 'exclude', 100, 'include')" - "X > 0 && X <= 100".
* [doc] Changed to recommend GitHub for reporting issues.
  Until now, Redmine account must be approved by administrator, so you can not
  report at feel free. [GitHub#202] [Patch by cosmo0920]
* [doc] Translated C API document. [GitHub#203] [Patch by NOKUBI Takatsugu]
* Supported to customize similarity threshold by ``'*S10"..."'`` syntax.
* Added source column validation to
  :doc:`/reference/commands/column_create`.  If source column doesn't
  exist, :doc:`/reference/commands/column_create` returns ``false`` and
  column isn't created.
* [debian] Dropped Debian jessie and sid support.

Fixes
^^^^^

* [windows] Disabled KyTea by default because of redistribution license issue.
* [windows] Disabled libedit support by default.
* Fixed a bug that similar search doesn't use IDF (Inverse Document Frequency)
  for choosing target tokens.
  In the previous versions, it used token ID instead of IDF.
  [groonga-dev,02731] [groonga-dev,02754] [Reported by Satoshi Mitani, warp kawada]
* Fixed a crash bug when deleting a record.
  If the table of the record has ``_key`` and ``_key`` is
  indexed from two or more index columns, deleting causes a crash.
* Fixed compiler warning about unused variable 'e'. [GitHub#197] [Patch by cosmo0920]
* [fedora][httpd] Fixed service start failure without log directory.
* [mruby] Fixed build error with ``--disable-mruby``.
  [GitHub#200] [Patch by cosmo0920]
* Fixed build warning about CMP0014 for MariaDB bundled Mroonga package.
  [GitHub#201] [Patch by cosmo0920]
* Fixed a bug that "*" is parsed as prefix search with empty string.
  [groonga-dev,02796] [Reported by Kazuhiko]
* [httpd] Updated bundled nginx version to the latest mainline (1.7.5).
  This update contains vulnerability fix (CVE-2014-3616).

Thanks
^^^^^^

* Hideki Yamane
* Masafumi Yokoyama
* cosmo0920
* NOKUBI Takatsugu
* Satoshi Mitani
* warp kawada
* Kazuhiko

.. _release-4-0-5:

Release 4.0.5 - 2014/08/29
--------------------------

Improvements
^^^^^^^^^^^^

* Dropped Ubuntu 13.10 (Saucy Salamander).
* Supported to show weight vector element as XML.
* [ :doc:`/reference/functions/between` ] Supported outside of key range.
  [GitHub#183] [Patch by Naoya Murakami]
* [mruby] Changed to use Onigmo as regexp implementation.
* Added :doc:`/reference/functions/highlight_full` and
  :doc:`/reference/functions/highlight_html` which tags
  output text. [GitHub#185] [Patch by Naoya Murakami]
* [doc] Updated outdated functions/commands reference
  [GitHub#188] [GitHub#189] [Patch by Naoya Murakami]
* [benchmark] Added average and median in benchmark result.
* Exported :c:func:`grn_selector_func` and :c:func:`grn_proc_set_selector`
  for plugin developer. [GitHub#190] [Patch by Naoya Murakami]
* [ :doc:`/reference/commands/tokenize` ] Added "mode" option to control behavior.
  You can use ``GET`` or ``ADD`` as a mode.
  [GitHub#191] [GitHub#192] [Patch by Naoya Murakami]
* Supported near search by '\*N"A Z"' syntax.
* Supported to customize behavior of tokenizer whether it skips token or
  skips token with position. This feature is useful for developing tokenizer plugin.
  [GitHub#193] [Patch by Naoya Murakami]
* [doc] Updated description that a bug report is accepted in Japanese on GitHub issue tracker.
  [Reported by 6elz] [groonga-dev,02651]
* [doc] Updated tutorial in English. [GitHub#194] [Patch by cosmo0920]

Fixes
^^^^^

* [php] Fixed memory leak when :c:func:`grn_ctx_recv()` is used.
  [GitHub#184] [Patch by @do-aki]
* [php] Fixed to avoid double free when :c:func:`grn_ctx_close()` is called.
  [GitHub#184] [Patch by @do-aki]
* Fixed to support out-of-tree build again.
* [httpd] Updated bundled nginx version to the latest mainline (1.7.4). This update contains vulnerability fix.
* Fixed a bug that infinite loop occurs sometimes when updating indexes.
  [groonga-dev,02673] [Reported by MINAGAWA Sho]
* Fixed a bug that document frequency is invalid when
  :ref:`offline-index-construction` is used.
  [groonga-dev,02691] [Reported by Naoya Murakami]

Thanks
^^^^^^

* @do-aki
* Naoya Murakami
* 6elz
* MINAGAWA Sho
* cosmo0920

.. _release-4-0-4:

Release 4.0.4 - 2014/07/29
--------------------------

Improvements
^^^^^^^^^^^^

* [httpd] Updated bundled nginx version to the latest mainline (1.7.3).
* Supported weight in vector column.
* [ :doc:`/reference/executables/groonga-benchmark` ] Added missing description about HTTP.
* [ :doc:`/tutorial` ] Updated English tutorial documentation.
* [example] Supported Ruby 1.9 or later for edict2grn.rb.
* [http][admin] Updated to use English as default language. [GitHub#182]

Fixes
^^^^^

* Fixed to show right syntax error location in response.
* [httpd] Fixed build error on Clang. [GitHub clear-code/rurema-search#11] [Reported by niku]

Thanks
^^^^^^

* niku

.. _release-4-0-3:


Release 4.0.3 - 2014/06/29
--------------------------

Improvements
^^^^^^^^^^^^

* [experimental][ :doc:`/reference/executables/groonga-server-http` ]
  Supported POST.
* [windows] Bundled libwinpthread-1.dll. Because newer
  libgcc_s_sjlj-1.dll depends on libwinpthread-1.dll.
  [groonga-dev,02398] [Reported by Masafumi Yokoyama]
* [ :doc:`/reference/executables/groonga` ] Changed the default port number of
  GQTP to 10043. Because server packages use 10043 by default.
  [GitHub#172] [Patch by Naoya Murakami]
* [ :doc:`/reference/commands/normalize` ] Added more validations for invalid
  argument.
  [groonga-dev,02409][GitHub:#174]
  [Reported by GMO Media, Inc.][Patch by Naoya Murakami]
* [ :doc:`/reference/commands/tokenize` ] Added more validations for invalid
  argument.
* [ :doc:`/reference/executables/groonga-server-http` ] Supported ``Expect:
  100-Continue`` request.
* Exported ``grn_proc_get_type()``.
* [ :doc:`/reference/executables/groonga-suggest-learner` ]
  Added ``--log-path`` and ``--log-level`` options.
* Deprecated ``GRN_CTX_USE_QL``.
* Deprecated ``GRN_CTX_BATCH_MODE``.
* Added ``grn_text_printf()``.
* Added ``grn_text_vprintf()``.
* Removed limitation of one query log size.
* Added :c:func:`grn_plugin_expr_var_init()`.
  [GitHub#175][Patch by Naoya Murakami]
* Added :c:func:`grn_plugin_command_create()`.
  [GitHub#175][Patch by Naoya Murakami]
* [GitHub#176] Supported reference vector column with weight.
* [plugin] Used public API instead of internal API.
  [GitHub#177][GitHub#178] [Patch by Naoya Murakami]
* [doc][plugin] Added plugin APIs.
  [GitHub#179] [Patch by Naoya Murakami]
* [windows] Re-supported Visual C++ 2010.
  [groonga-dev,02454] [Reported by cosmo0920].

Fixes
^^^^^

* [rpm][groonga-server-gqtp] Fixed a bug that HTTP protocol is used
  not GQTP.
  [GitHub#173] [Patch by Naoya Murakami]
* [ :doc:`/reference/commands/select` ] Fixed a crash bug when
  :ref:`select-adjuster` has a syntax error.

Thanks
^^^^^^

* Masafumi Yokoyama
* Naoya Murakami
* GMO Media, Inc.
* cosmo0920

.. _release-4-0-2:


Release 4.0.2 - 2014/05/29
--------------------------

Improvements
^^^^^^^^^^^^

* [doc] Updated documentation about
  :doc:`/reference/executables/groonga-suggest-leaner`.
* [doc] Added documentation about how to update files.
  [GitHub#160] [Patch by cosmo0920]
* [doc] Updated to caplitalized "Groonga" terms in
  documentation. [GitHub#162][GitHub#163][GitHub#164]
  [Patch by cosmo0920]
* Supported Ubuntu 14.04 (Trusty Tahr).
* Dropped Ubuntu 12.10.
* Migrated Ubuntu package distribution site to PPA on Launchpad.
  See :doc:`/install/ubuntu` for details.
* Handled all requests that start with ``/d/`` as API requests. You
  need to put files to directories that don't start with ``/d/`` to
  serve by Groonga HTTP service.
* [munin] Supported :doc:`/reference/executables/groonga-httpd`.
  [Reported by Naoya Murakami]
* Supported daylight saving time.
  [#2546]
* [doc] Added a description about ``--with-mecab`` in
  :doc:`/install/mac_os_x`.
* [http] Changed HTTP return code to ``400 Bad Request`` from ``500
  Internal Server Error`` for syntax error case.
* [http][admin] Removed jQuery JSON plugin.
  [GitHub#168] [Patch by Tetsuharu OHZEKI]
* [http][admin] Enabled strict mode.
  [GitHub#169] [Patch by Tetsuharu OHZEKI]
* Exported getting variable APIs to :doc:`/reference/api/plugin`.
  [GitHub#170] [Patch by Naoya Murakami]

  * Added ``grn_plugin_proc_get_var()``.
  * Added ``grn_plugin_proc_get_var_by_offset()``.

* [experimental] Added :doc:`/reference/commands/tokenizer_list`.
  [GitHub#171] [Patch by Naoya Murakami]
* [experimental] Added :doc:`/reference/commands/normalizer_list`.
  [GitHub#171] [Patch by Naoya Murakami]

Fixes
^^^^^

* [index] Fixed a bug that wrong max segment. It causes a crash when
  you use all allocated resource for an index column.
  [#2438] [Reported by GMO Media, Inc.]
* [doc] Fixed a typo in :doc:`/install/centos`.
  [GitHub#166] [Patch by Naoya Murakami]
* [doc] Fixed the wrong default value of ``drilldown_output_columns`` in
  :doc:`/reference/commands/select`.
  [GitHub#167] [Patch by Naoya Murakami]
* [doc] Added a missing ``\`` escape exception in
  :doc:`/reference/grn_expr/query_syntax`.
  [Reported by @Yappo]

Thanks
^^^^^^

* cosmo0920
* Naoya Murakami
* Tetsuharu OHZEKI
* GMO Media, Inc.
* @Yappo

.. _release-4-0-1:


Release 4.0.1 - 2014/03/29
--------------------------

Improvements
^^^^^^^^^^^^

* [doc] Added a link in return value to detailed header description (:doc:`/reference/command/output_format`).
* Supported to inspect vector and object value in JSON load.
  It shows more details about data which is failed to load.
* Added ``adjuster`` option to select command.
  adjuster options accepts following syntax: INDEX_COLUMN @ STRING_LITERAL (* FACTOR).
* Supported :ref:`weight-vector-column`. You need to specify 'COLUMN_VECTOR|WITH_WEIGHT' flags 
  to create weight vector column.
* Added missing MIN/MAX macros on SunOS. [GitHub#154] [Patch by Sebastian Wiedenroth]
* Improved recycling garbage data. It suppress to increse database size.
* [doc] Added documentation about GET parameters for :doc:`/reference/executables/groonga-suggest-httpd`.
* [doc] Added documentation about :doc:`/reference/column`.
* [doc] Added documentation about :doc:`/reference/columns/vector`.
* [column_list] Supported to show weight vector column.
* [column_create] Added error check for creating multi column index without WITH_SECTION.
* [httpd] Enabled stub status module (NginxHttpStubStatusModule) for groonga-httpd. [Suggested by Masahiro Nagano]

Fixes
^^^^^

* Fixed a bug that a division overflow caused a fatal error.
  For example, it occurs when you execute 'COLUMN / -1' operation to Int32 or Int64 column. [#2307]
* Fixed a bug that '%' operations performs '/' operations. [#2307]
* [doc] Fixed a wrong documentation about :doc:`reference/commands/column_rename`. [Reported by nise_nabe]
* Fixed the issue that out of bound array element access may occurs. [GitHub#158] [Reported by dcb314]

Thanks
^^^^^^

* Sebastian Wiedenroth
* Masahiro Nagano
* nise_nabe
* dcb314

.. _release-4-0-0:


Release 4.0.0 - 2014/02/09
--------------------------

* Bump version to 4.0.0!

Improvements
^^^^^^^^^^^^

* [normalizer] Supported to show "checks" which is used for calculating next character position.
  Use WITH_CHECKS flag to enable this feature.
* [deb] Dropped Ubuntu 13.04 support.

Fixes
^^^^^

* Fixed a crash bug that an object in grn_expr is used after it is freed.
  Normally Groonga server users aren't affected this bug. This bug mainly affects Rroonga users.
  Because this bug is occured by specifying column name including pseudo column name -
  such as '_key' - Rroonga users may use the usage.
* Fixed not to execute unexpected cascade delete which is introduced Groonga 3.0.8 release.
  If source's range and index's domain are different, Groonga doesn't execute cascade delete.
  [groonga-dev,02073] [Reported by yoku]
* Fixed not to publish grn_snip structure. Use grn_obj instead of grn_snip.
  If you use grn_snip_close, please replace grn_snip_close to grn_obj_close.
* [snippet_html] Fixed a crash bug when --query is empty.
  [groonga-dev,02097] [Reported by Naoya Murakami]
* [snippet_html] Fixed to suppress ALERT level message when contents of column is empty text.
  [groonga-dev,02097] [Reported by Naoya Murakami]
* [groonga-httpd] Fixed a bug "off" is used as path name in groonga_query_log_path.
  [groonga-dev,02113] [Reported by Ryoji Yamamoto]

Thanks
^^^^^^

* yoku
* Naoya Murakami
* Ryoji Yamamoto

The old releases
----------------

.. toctree::
   :maxdepth: 2

   news/3.x
   news/2.x
   news/1.3.x
   news/1.2.x
   news/1.1.x
   news/1.0.x
   news/0.x
   news/senna
