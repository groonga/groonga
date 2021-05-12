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

.. c:type:: grn_proc_type

   TODO...

.. c:type:: grn_proc_func

   TODO...

.. c:function:: grn_obj *grn_proc_create(grn_ctx *ctx, const char *name, int name_size, grn_proc_type type, grn_proc_func *init, grn_proc_func *next, grn_proc_func *fin, unsigned int nvars, grn_expr_var *vars)

   nameに対応する新たなproc(手続き)をctxが使用するdbに定義します。

   :param name: 作成するprocの名前を指定します。
   :param name_size: The number of bytes of name parameter. If negative value is specified, name parameter is assumed that NULL-terminated string.
   :param type: procの種類を指定します。
   :param init: 初期化関数のポインタを指定します。
   :param next: 実処理関数のポインタを指定します。
   :param fin: 終了関数のポインタを指定します。
   :param nvars: procで使用する変数の数を指定します。
   :param vars: procで使用する変数の定義を指定します。( :c:type:`grn_expr_var` 構造体の配列)

.. c:function:: grn_obj *grn_proc_get_info(grn_ctx *ctx, grn_user_data *user_data, grn_expr_var **vars, unsigned int *nvars, grn_obj **caller)

   user_dataをキーとして、現在実行中の :c:type:`grn_proc_func` 関数および定義されている変数( :c:type:`grn_expr_var` )の配列とその数を取得します。

   :param user_data: :c:type:`grn_proc_func` に渡されたuser_dataを指定します。
   :param nvars: 変数の数を取得します。

.. c:function:: grn_rc grn_obj_set_finalizer(grn_ctx *ctx, grn_obj *obj, grn_proc_func *func)

   objectを破棄するときに呼ばれる関数を設定します。

   table, column, proc, exprのみ設定可能です。

   :param obj: 対象objectを指定します。
   :param func: objectを破棄するときに呼ばれる関数を指定します。
