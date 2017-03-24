.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

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
