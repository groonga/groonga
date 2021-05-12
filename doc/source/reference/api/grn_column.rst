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

.. c:macro:: GRN_COLUMN_NAME_ID

   It returns the name of :doc:`/reference/columns/pseudo` ``_id``.

   It is useful to use with :c:macro:`GRN_COLUMN_NAME_ID_LEN` like
   the following::

     grn_obj *id_column;
     id_column = grn_ctx_get(ctx, GRN_COLUMN_NAME_ID, GRN_COLUMN_NAME_ID_LEN);

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_ID_LEN

   It returns the byte size of :c:macro:`GRN_COLUMN_NAME_ID`.

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_KEY

   It returns the name of :doc:`/reference/columns/pseudo` ``_key``.

   It is useful to use with :c:macro:`GRN_COLUMN_NAME_KEY_LEN` like
   the following::

     grn_obj *key_column;
     key_column = grn_ctx_get(ctx, GRN_COLUMN_NAME_KEY, GRN_COLUMN_NAME_KEY_LEN);

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_KEY_LEN

   It returns the byte size of :c:macro:`GRN_COLUMN_NAME_KEY`.

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_VALUE

   It returns the name of :doc:`/reference/columns/pseudo` ``_value``.

   It is useful to use with :c:macro:`GRN_COLUMN_NAME_VALUE_LEN` like
   the following::

     grn_obj *value_column;
     value_column = grn_ctx_get(ctx, GRN_COLUMN_NAME_VALUE, GRN_COLUMN_NAME_VALUE_LEN);

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_VALUE_LEN

   It returns the byte size of :c:macro:`GRN_COLUMN_NAME_VALUE`.

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_SCORE

   It returns the name of :doc:`/reference/columns/pseudo` ``_score``.

   It is useful to use with :c:macro:`GRN_COLUMN_NAME_SCORE_LEN` like
   the following::

     grn_obj *score_column;
     score_column = grn_ctx_get(ctx, GRN_COLUMN_NAME_SCORE, GRN_COLUMN_NAME_SCORE_LEN);

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_SCORE_LEN

   It returns the byte size of :c:macro:`GRN_COLUMN_NAME_SCORE`.

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_NSUBRECS

   It returns the name of :doc:`/reference/columns/pseudo` ``_nsubrecs``.

   It is useful to use with :c:macro:`GRN_COLUMN_NAME_NSUBRECS_LEN` like
   the following::

     grn_obj *nsubrecs_column;
     nsubrecs_column = grn_ctx_get(ctx, GRN_COLUMN_NAME_NSUBRECS, GRN_COLUMN_NAME_NSUBRECS_LEN);

   Since 3.1.1.

.. c:macro:: GRN_COLUMN_NAME_NSUBRECS_LEN

   It returns the byte size of :c:macro:`GRN_COLUMN_NAME_NSUBRECS`.

   Since 3.1.1.

.. c:function:: grn_obj *grn_column_create(grn_ctx *ctx, grn_obj *table, const char *name, unsigned int name_size, const char *path, grn_obj_flags flags, grn_obj *type)

   tableに新たなカラムを定義します。nameは省略できません。一つのtableに同一のnameのcolumnを複数定義することはできません。

   :param table: 対象tableを指定します。
   :param name: カラム名を指定します。
   :param name_size: nameパラメータのsize(byte)を指定します。
   :param path:
      カラムを格納するファイルパスを指定します。
      flagsに ``GRN_OBJ_PERSISTENT`` が指定されている場合のみ有効です。
      NULLなら自動的にファイルパスが付与されます。
   :param flags:
      ``GRN_OBJ_PERSISTENT`` を指定すると永続columnとなります。

      ``GRN_OBJ_COLUMN_INDEX`` を指定すると転置インデックスとなります。

      ``GRN_OBJ_COLUMN_SCALAR`` を指定するとスカラ値(単独の値)を格納します。

      ``GRN_OBJ_COLUMN_VECTOR`` を指定すると値の配列を格納します。

      ``GRN_OBJ_COMPRESS_ZLIB`` を指定すると値をzlib圧縮して格納します。

      ``GRN_OBJ_COMPRESS_LZO`` を指定すると値をlzo圧縮して格納します。

      ``GRN_OBJ_COLUMN_INDEX`` と共に ``GRN_OBJ_WITH_SECTION`` を指定すると、転置索引にsection(段落情報)を合わせて格納します。

      ``GRN_OBJ_COLUMN_INDEX`` と共に ``GRN_OBJ_WITH_WEIGHT`` を指定すると、転置索引にweight情報を合わせて格納します。

      ``GRN_OBJ_COLUMN_INDEX`` と共に ``GRN_OBJ_WITH_POSITION`` を指定すると、転置索引に出現位置情報を合わせて格納します。
   :param type: カラム値の型を指定します。定義済みのtypeあるいはtableを指定できます。

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
