.. -*- rst -*-

``grn_table_cursor``
====================

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_table_cursor

   TODO...

.. c:function:: grn_rc grn_table_cursor_set_value(grn_ctx *ctx, grn_table_cursor *tc, const void *value, int flags)

   cursorのカレントレコードのvalueを引数の内容に置き換えます。cursorのカレントレコードが存在しない場合は ``GRN_INVALID_ARGUMENT`` を返します。

   :param tc: 対象cursorを指定します。
   :param value: 新しいvalueの値を指定します。
   :param flags: :c:func:`grn_obj_set_value()` のflagsと同様の値を指定できます。

.. c:function:: grn_rc grn_table_cursor_delete(grn_ctx *ctx, grn_table_cursor *tc)

   cursorのカレントレコードを削除します。cursorのカレントレコードが存在しない場合は ``GRN_INVALID_ARGUMENT`` を返します。

   :param tc: 対象cursorを指定します。

.. c:function:: grn_obj *grn_table_cursor_table(grn_ctx *ctx, grn_table_cursor *tc)

   cursorが属するtableを返します。

   :param tc: 対象cursorを指定します。
