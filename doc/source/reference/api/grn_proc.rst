.. -*- rst -*-

``grn_proc``
============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_proc_func

   TODO...

.. c:function:: grn_obj *grn_proc_get_info(grn_ctx *ctx, grn_user_data *user_data, grn_expr_var **vars, unsigned int *nvars, grn_obj **caller)

   user_dataをキーとして、現在実行中の :c:type:`grn_proc_func` 関数および定義されている変数( :c:type:`grn_expr_var` )の配列とその数を取得します。

   :param user_data: :c:type:`grn_proc_func` に渡されたuser_dataを指定します。
   :param nvars: 変数の数を取得します。
