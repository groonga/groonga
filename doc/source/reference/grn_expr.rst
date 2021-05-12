.. -*- rst -*-

grn_expr
========

Grn_expr is an object that searches records with specified conditions
and manipulates a database. It's pronounced as ``gurun expression``.

Conditions for searching records from a database can be represented by
conbining condition expressions such as ``equal condition expression``
and ``less than condition expression`` with set operations such as
``AND``, ``OR`` and ``NOT``. Grn_expr executes those conditions to
search records. You can also use advanced searches such as similar
search and near search by grn_expr. You can also use flexible full
text search. For example, you can control hit scores for specified
words and improve recall by re-searching with high-recall algolithm
dinamically. To determine whether re-searching or not, the number of
matched rescords is used.

There are three ways to create grn_expr:

  * Parsing :doc:`/reference/grn_expr/query_syntax` string.
  * Parsing :doc:`/reference/grn_expr/script_syntax` string.
  * Calling grn_expr related APIs.

:doc:`/reference/grn_expr/query_syntax` is for common search form in
Internet search site. It's simple and easy to use but it has a
limitation. You can not use all condition expressions and set
operations in :doc:`/reference/grn_expr/query_syntax`. You can use
:doc:`/reference/grn_expr/query_syntax` with ``query`` option in
:doc:`/reference/commands/select`.

:doc:`/reference/grn_expr/script_syntax` is ECMAScript like
syntax. You can use all condition expressions and set operations in
:doc:`/reference/grn_expr/script_syntax`. You can use
:doc:`/reference/grn_expr/script_syntax` with ``filter`` option and
``scorer`` option in :doc:`/reference/commands/select`.

You can use groonga as a library and create a grn_expr by calling
grn_expr related APIs. You can use full features with calling APIs
like :doc:`/reference/grn_expr/script_syntax`. Calling APIs is useful
creating a custom syntax to create grn_expr. They are used in `rroonga
<http://ranguba.org/#about-rroonga>`_ that is Ruby bindings
of Groonga. Rroonga can create a grn_expr by Ruby's syntax instead of
parsing string.

.. toctree::
   :maxdepth: 1
   :glob:

   grn_expr/query_syntax
   grn_expr/script_syntax

See also
--------

  * :doc:`/reference/api/grn_expr`: grn_expr related APIs
