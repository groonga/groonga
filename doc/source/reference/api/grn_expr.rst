.. -*- rst -*-

.. highlight:: c

grn_expr
========

`grn_expr` is an :c:type:`grn_obj` that represents an expression. Here
is a list of what expression can do:

  * Expression can apply some operations to a record by
    :c:func:`grn_expr_exec`.
  * Expression can represents search condition. :c:func:`grn_table_select`
    can select records that match against the search condition represented
    by expression.

There are two string representations of expression:

  * :doc:`/reference/grn_expr/query_syntax`
  * :doc:`/reference/grn_expr/script_syntax`

:c:func:`grn_expr_parse` parses string represented expression and
appends the parsed expression to another expression.

Example
-------

TODO...

Reference
---------

.. note::
   We are currently switching to automatic generation using Doxygen.
