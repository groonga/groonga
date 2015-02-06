.. -*- rst -*-

:orphan:

.. highlightlang:: none

News
====

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
