.. -*- rst -*-

``grn_hook``
============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_hook_entry

   TODO...

.. c:function:: grn_rc grn_obj_add_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, int offset, grn_obj *proc, grn_obj *data)

   objに対してhookを追加します。

   :param obj: 対象objectを指定します。
   :param entry:
      ``GRN_HOOK_GET`` は、objectの参照時に呼び出されるhookを定義します。

      ``GRN_HOOK_SET`` は、objectの更新時に呼び出されるhookを定義します。

      ``GRN_HOOK_SELECT`` は、検索処理の実行中に適時呼び出され、処理の実行状況を調べたり、実行の中断を指示することができます。
   :param offset:
      hookの実行順位。offsetに対応するhookの直前に新たなhookを挿入します。

      0を指定した場合は先頭に挿入されます。-1を指定した場合は末尾に挿入されます。

      objectに複数のhookが定義されている場合は順位の順に呼び出されます。
   :param proc: 手続きを指定します。
   :param data: hook固有情報を指定します。

.. c:function:: int grn_obj_get_nhooks(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry)

   objに定義されているhookの数を返します。

   :param obj: 対象objectを指定します。
   :param entry: hookタイプを指定します。

.. c:function:: grn_obj *grn_obj_get_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, int offset, grn_obj *data)

   objに定義されているhookの手続き（proc）を返します。hook固有情報が定義されている場合は、その内容をdataにコピーして返します。

   :param obj: 対象objectを指定します。
   :param entry: hookタイプを指定します。
   :param offset: 実行順位を指定します。
   :param data: hook固有情報格納バッファを指定します。

.. c:function:: grn_rc grn_obj_delete_hook(grn_ctx *ctx, grn_obj *obj, grn_hook_entry entry, int offset)

   objに定義されているhookを削除します。

   :param obj: 対象objectを指定します。
   :param entry: hookタイプを指定します。
   :param offset: 実行順位を指定します。
