.. -*- rst -*-

``grn_index_cursor``
====================

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:function:: grn_obj *grn_index_cursor_open(grn_ctx *ctx, grn_table_cursor *tc, grn_obj *index, grn_id rid_min, grn_id rid_max, int flags)
 
   :c:type:`grn_table_cursor` から取得できるそれぞれのレコードについて、 ``GRN_OBJ_COLUMN_INDEX`` 型のカラムの値を順番に取り出すためのカーソルを生成して返します。

   rid_min, rid_maxを指定して取得するレコードidの値を制限することができます。

   戻り値であるgrn_index_cursorは :c:func:`grn_obj_close()` を使って解放します。

   :param tc: 対象cursorを指定します。
   :param index: 対象インデックスカラムを指定します。
   :param rid_min: 出力するレコードidの下限を指定します。
   :param rid_max: 出力するレコードidの上限を指定します。
