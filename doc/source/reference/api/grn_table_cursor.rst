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

.. c:function:: grn_rc grn_table_cursor_close(grn_ctx *ctx, grn_table_cursor *tc)

   :c:func:`grn_table_cursor_open` で生成したcursorを解放します。

   :param tc: 対象cursorを指定します。

.. c:function:: grn_id grn_table_cursor_next(grn_ctx *ctx, grn_table_cursor *tc)

   cursorのカレントレコードを一件進めてそのIDを返します。cursorの対象範囲の末尾に達すると ``GRN_ID_NIL`` を返します。

   :param tc: 対象cursorを指定します。

.. c:function:: int grn_table_cursor_get_key(grn_ctx *ctx, grn_table_cursor *tc, void **key)

   cursorのカレントレコードのkeyをkeyパラメータにセットし、その長さを返します。

   :param tc: 対象cursorを指定します。
   :param key: カレントレコードのkeyへのポインタがセットされます。

.. c:function:: int grn_table_cursor_get_value(grn_ctx *ctx, grn_table_cursor *tc, void **value)

   cursorパラメータのカレントレコードのvalueをvalueパラメータにセットし、その長さを返します。

   :param tc: 対象cursorを指定します。
   :param value: カレントレコードのvalueへのポインタがセットされます。

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
