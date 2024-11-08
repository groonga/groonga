.. -*- rst -*-

.. highlight:: c

``grn_column``
==============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:function:: grn_rc grn_column_index_update(grn_ctx *ctx, grn_obj *column, grn_id id, unsigned int section, grn_obj *oldvalue, grn_obj *newvalue)

   oldvalue, newvalueの値から得られるキーに対応するcolumnの値の中の、id, sectionに対応するエントリを更新します。columnは ``GRN_OBJ_COLUMN_INDEX`` 型のカラムでなければなりません。

   :param column: 対象columnを指定します。
   :param id: 対象レコードのIDを指定します。
   :param section: 対象レコードのセクション番号を指定します。
   :param oldvalue: 更新前の値を指定します。
   :param newvalue: 更新後の値を指定します。

.. c:function:: grn_obj *grn_column_table(grn_ctx *ctx, grn_obj *column)

   columnが属するtableを返します。

   :param column: 対象columnを指定します。

.. c:function:: grn_rc grn_column_rename(grn_ctx *ctx, grn_obj *column, const char *name, unsigned int name_size)

   ctxが使用するdbにおいてcolumnに対応する名前をnameに更新します。columnは永続オブジェクトでなければいけません。

   :param column: 対象columnを指定します。
   :param name: 新しい名前を指定します。
   :param name_size: nameパラメータのsize（byte）を指定します。

.. c:function:: int grn_column_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size)

   カラムobjの名前の長さを返します。buf_sizeの長さが名前の長さ以上であった場合は、namebufに該当する名前をコピーします。

   :param obj: 対象objectを指定します。
   :param namebuf: 名前を格納するバッファ（呼出側で準備する）を指定します。
   :param buf_size: namebufのサイズ（byte長）を指定します。

.. c:function:: int grn_column_index(grn_ctx *ctx, grn_obj *column, grn_operator op, grn_obj **indexbuf, int buf_size, int *section)

   columnに張られているindexのうち、opの操作を実行可能なものの数を返します。またそれらのidを、buf_sizeに指定された個数を上限としてindexbufに返します。

   :param column: 対象のcolumnを指定します。
   :param op: indexで実行したい操作を指定します。
   :param indexbuf: indexを格納するバッファ（呼出側で準備する）を指定します。
   :param buf_size: indexbufのサイズ（byte長）を指定します。
   :param section: section番号を格納するint長バッファ（呼出側で準備する）を指定します。

.. c:function:: grn_rc grn_column_truncate(grn_ctx *ctx, grn_obj *column)

   .. note::

      This is a dangerous API. You must not use this API when other
      thread or process accesses the target column. If you use this
      API against shared column, the process that accesses the column
      may be broken and the column may be broken.

   .. versionadded:: 4.0.9

   Clears all values in the column.

   :param column: The column to be truncated.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.
