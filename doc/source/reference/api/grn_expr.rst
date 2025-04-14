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

.. c:function:: grn_obj *grn_expr_create(grn_ctx *ctx, const char *name, unsigned int name_size)

.. c:function:: grn_rc grn_expr_close(grn_ctx *ctx, grn_obj *expr)

.. c:function:: grn_obj *grn_expr_add_var(grn_ctx *ctx, grn_obj *expr, const char *name, unsigned int name_size)

.. c:function:: grn_obj *grn_expr_get_var_by_offset(grn_ctx *ctx, grn_obj *expr, unsigned int offset)

.. c:function:: grn_obj *grn_expr_append_obj(grn_ctx *ctx, grn_obj *expr, grn_obj *obj, grn_operator op, int nargs)

.. c:function:: grn_obj *grn_expr_append_const(grn_ctx *ctx, grn_obj *expr, grn_obj *obj, grn_operator op, int nargs)

.. c:function:: grn_obj *grn_expr_append_const_str(grn_ctx *ctx, grn_obj *expr, const char *str, unsigned int str_size, grn_operator op, int nargs)

.. c:function:: grn_obj *grn_expr_append_const_int(grn_ctx *ctx, grn_obj *expr, int i, grn_operator op, int nargs)

.. c:function:: grn_rc grn_expr_append_op(grn_ctx *ctx, grn_obj *expr, grn_operator op, int nargs)

.. c:function:: grn_rc grn_expr_compile(grn_ctx *ctx, grn_obj *expr)

.. c:function:: grn_obj *grn_expr_exec(grn_ctx *ctx, grn_obj *expr, int nargs)

.. c:function:: grn_obj *grn_expr_alloc(grn_ctx *ctx, grn_obj *expr, grn_id domain, grn_obj_flags flags)
