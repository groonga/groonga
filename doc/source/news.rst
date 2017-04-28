.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

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

* [:doc:`/reference/executables/groonga`][:option:`--cache-base-path
  <path>`] Added a new option to use persistent cache.

* [:doc:`/reference/executables/groonga-httpd`] Supported
  ``groonga_cache_base_path`` option to use persistent cache.

* [windows] Updated bundled msgpack to 2.1.1.

* [:doc:`/reference/commands/object_inspect`] Supported not only
  column inspection, but also index column statistics.

* Supported index search for ``".*"`` regexp pattern.  This feature is
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

* [httpd] Fixed a bug that response of :doc:`reference/commands/quit`
  and :doc:`reference/commands/shutdown` is broken JSON when worker is
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
