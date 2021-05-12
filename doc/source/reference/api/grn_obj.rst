.. -*- rst -*-

``grn_obj``
===========

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_obj

   TODO...

.. c:function:: grn_obj *grn_obj_column(grn_ctx *ctx, grn_obj *table, const char *name, unsigned int name_size)

   nameがカラム名の場合、それに対応するtableのカラムを返します。対応するカラムが存在しなければNULLを返します。

   nameがアクセサ文字列の場合、それに対応するaccessorを返します。アクセサ文字列とは、カラム名等を'.'で連結した文字列です。'_id', '_key'は特殊なアクセサで、それぞれレコードID/keyを返します。例) 'col1' / 'col2.col3' / 'col2._id'

   :param table: 対象tableを指定します。
   :param name: カラム名を指定します。

.. c:function:: grn_bool grn_obj_is_builtin(grn_ctx *ctx, grn_obj *obj)

   Check whether Groonga built-in object.

   :param ctx: context
   :param obj: target object
   :return: ``GRN_TRUE`` for built-in groonga object, ``GRN_FALSE`` otherwise.

.. c:function:: grn_bool grn_obj_is_index_column(grn_ctx *ctx, grn_obj *obj)

   Check whether index column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is index column, ``GRN_FALSE`` otherwise. Even if the target object isn't column, return ``GRN_FALSE``.

.. c:function:: grn_bool grn_obj_is_vector_column(grn_ctx *ctx, grn_obj *obj)

   Check whether vector column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is vector column, ``GRN_FALSE`` otherwise. Even if the target object isn't column, return ``GRN_FALSE``.

.. c:function:: grn_obj *grn_obj_get_value(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value)

   objのIDに対応するレコードのvalueを取得します。valueを戻り値として返します。

   :param obj: 対象objectを指定します。
   :param id: 対象レコードのIDを指定します。
   :param value: 値を格納するバッファ（呼出側で準備する）を指定します。

.. c:function:: int grn_obj_get_values(grn_ctx *ctx, grn_obj *obj, grn_id offset, void **values)

   objに指定されたカラムについて、offsetに指定されたレコードIDを開始位置として、IDが連続するレコードに対応するカラム値が昇順に格納された配列へのポインタをvaluesにセットします。

   取得できた件数が戻り値として返されます。エラーが発生した場合は -1 が返されます。

   .. note:: 値が固定長であるカラムのみがobjに指定できます。範囲内のIDに対応するレコードが有効であるとは限りません。delete操作を実行したことのあるテーブルに対しては、:c:func:`grn_table_at()` などによって各レコードの存否を別途確認しなければなりません。

   :param obj: 対象objectを指定します。
   :param offset: 値を取得する範囲の開始位置となるレコードIDを指定します。
   :param values: 値の配列がセットされます。

.. c:function:: grn_rc grn_obj_set_value(grn_ctx *ctx, grn_obj *obj, grn_id id, grn_obj *value, int flags)

   objのIDに対応するレコードの値を更新します。対応するレコードが存在しない場合は ``GRN_INVALID_ARGUMENT`` を返します。

   :param obj: 対象objectを指定します。
   :param id: 対象レコードのIDを指定します。
   :param value: 格納する値を指定します。
   :param flags:
      以下の値を指定できます。

      .. hlist::
         :columns: 3

         * :c:macro:`GRN_OBJ_SET`
         * :c:macro:`GRN_OBJ_INCR`
         * :c:macro:`GRN_OBJ_DECR`
         * :c:macro:`GRN_OBJ_APPEND`
         * :c:macro:`GRN_OBJ_PREPEND`
         * :c:macro:`GRN_OBJ_GET`
         * :c:macro:`GRN_OBJ_COMPARE`
         * :c:macro:`GRN_OBJ_LOCK`
         * :c:macro:`GRN_OBJ_UNLOCK`

.. c:macro:: GRN_OBJ_SET_MASK

.. c:macro:: GRN_OBJ_SET

   レコードの値をvalueと置き換えます。

.. c:macro:: GRN_OBJ_INCR

   レコードの値にvalueを加算します。

.. c:macro:: GRN_OBJ_DECR

   レコードの値にvalueを減算します。

.. c:macro:: GRN_OBJ_APPEND

   レコードの値の末尾にvalueを追加します。

.. c:macro:: GRN_OBJ_PREPEND

   レコードの値の先頭にvalueを追加します。

.. c:macro:: GRN_OBJ_GET

   新しいレコードの値をvalueにセットします。

.. c:macro:: GRN_OBJ_COMPARE

   レコードの値とvalueが等しいか調べます。

.. c:macro:: GRN_OBJ_LOCK

   当該レコードをロックします。:c:macro:`GRN_OBJ_COMPARE` と共に指定された場合は、レコードの値とvalueが等しい場合に限ってロックします。

.. c:macro:: GRN_OBJ_UNLOCK

   当該レコードのロックを解除します。

.. c:function:: grn_rc grn_obj_remove(grn_ctx *ctx, grn_obj *obj)

   objをメモリから解放し、それが永続オブジェクトであった場合は、該当するファイル一式を削除します。

   :param obj: 対象objectを指定します。

.. c:function:: grn_rc grn_obj_rename(grn_ctx *ctx, grn_obj *obj, const char *name, unsigned int name_size)

   ctxが使用するdbにおいてobjに対応する名前をnameに更新します。objは永続オブジェクトでなければいけません。

   :param obj: 対象objectを指定します。
   :param name: 新しい名前を指定します。
   :param name_size: nameパラメータのsize（byte）を指定します。

.. c:function:: grn_rc grn_obj_close(grn_ctx *ctx, grn_obj *obj)

   一時的なobjectであるobjをメモリから解放します。objに属するobjectも再帰的にメモリから解放されます。

   永続的な、table, column, exprなどは解放してはいけません。一般的には、一時的か永続的かを気にしなくてよい :c:func:`grn_obj_unlink()` を用いるべきです。

   :param obj: 対象objectを指定します。

.. c:function:: grn_rc grn_obj_reinit(grn_ctx *ctx, grn_obj *obj, grn_id domain, unsigned char flags)

   objの型を変更します。

   objは :c:func:`GRN_OBJ_INIT()` マクロなどで初期化済みでなければいけません。

   :param obj: 対象objectを指定します。
   :param domain: 変更後のobjの型を指定します。
   :param flags: ``GRN_OBJ_VECTOR`` を指定するとdomain型の値のベクタを格納するオブジェクトになります。

.. c:function:: void grn_obj_unlink(grn_ctx *ctx, grn_obj *obj)

   objをメモリから解放します。objに属するobjectも再帰的にメモリから解放されます。

.. c:function:: const char *grn_obj_path(grn_ctx *ctx, grn_obj *obj)

   objに対応するファイルパスを返します。一時objectならNULLを返します。

   :param obj: 対象objectを指定します。

.. c:function:: int grn_obj_name(grn_ctx *ctx, grn_obj *obj, char *namebuf, int buf_size)

   objの名前の長さを返します。無名objectなら0を返します。

   名前付きのobjectであり、buf_sizeの長さが名前の長以上であった場合は、namebufに該当する名前をコピーします。

   :param obj: 対象objectを指定します。
   :param namebuf: 名前を格納するバッファ（呼出側で準備する）を指定します。
   :param buf_size: namebufのサイズ（byte長）を指定します。

.. c:function:: grn_id grn_obj_get_range(grn_ctx *ctx, grn_obj *obj)

   objパラメータのとる値の範囲を表わしているオブジェクトのIDを返します。例えば、:c:type:`grn_builtin_type` にある ``GRN_DB_INT`` などを返します。

   :param obj: 対象objectを指定します。

.. c:function:: int grn_obj_expire(grn_ctx *ctx, grn_obj *obj, int threshold)

   objの占有するメモリのうち、可能な領域をthresholdを指標として解放します。

   :param obj: 対象objectを指定します。

.. c:function:: int grn_obj_check(grn_ctx *ctx, grn_obj *obj)

   objに対応するファイルの整合性を検査します。

   :param obj: 対象objectを指定します。

.. c:function:: grn_rc grn_obj_lock(grn_ctx *ctx, grn_obj *obj, grn_id id, int timeout)

   objをlockします。timeout（秒）経過してもlockを取得できない場合は ``GRN_RESOURCE_DEADLOCK_AVOIDED`` を返します。

   :param obj: 対象objectを指定します。

.. c:function:: grn_rc grn_obj_unlock(grn_ctx *ctx, grn_obj *obj, grn_id id)

   objをunlockします。

   :param obj: 対象objectを指定します。

.. c:function:: grn_rc grn_obj_clear_lock(grn_ctx *ctx, grn_obj *obj)

   強制的にロックをクリアします。

   :param obj: 対象objectを指定します。

.. c:function:: unsigned int grn_obj_is_locked(grn_ctx *ctx, grn_obj *obj)

   objが現在lockされていれば0以外の値を返します。

   :param obj: 対象objectを指定します。

.. c:function:: int grn_obj_defrag(grn_ctx *ctx, grn_obj *obj, int threshold)

   objの占有するDBファイル領域のうち、可能な領域をthresholdを指標としてフラグメントの解消を行います。

   フラグメント解消が実行されたセグメントの数を返します。

   :param obj: 対象objectを指定します。

.. c:function:: grn_id grn_obj_id(grn_ctx *ctx, grn_obj *obj)

   objのidを返します。

   :param obj: 対象objectを指定します。

.. c:function:: grn_rc grn_obj_delete_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, grn_bool removep)

   dbからidに対応するテーブルやカラムなどを削除します。mroonga向けに用意した内部APIです。

   :param db: The target database.
   :param id: The object (table, column and so on) ID to be deleted.
   :param removep: If ``GRN_TRUE``, clear object cache and remove relation between ID and key in database. Otherwise, just clear object cache.

.. c:function:: grn_rc grn_obj_path_by_id(grn_ctx *ctx, grn_obj *db, grn_id id, char *buffer)

   dbのidに対応するpathを返します。mroonga向けに用意した内部APIです。

   :param db: The target database.
   :param id: The object (table, column and so on) ID to be deleted.
   :param buffer: path string corresponding to the id will be set in this buffer.

.. c:function:: grn_rc grn_obj_cast_by_id(grn_ctx *ctx, grn_obj *source, grn_obj *destination, grn_bool add_record_if_not_exist)

   It casts value of ``source`` to value with type of
   ``destination``. Casted value is appended to ``destination``.

   Both ``source`` and ``destination`` must be bulk.

   If ``destination`` is a reference type bulk. (Reference type bulk
   means that type of ``destination`` is a table.)
   ``add_record_if_not_exist`` is used. If ``source`` value doesn't
   exist in the table that is a type of ``destination``. The ``source``
   value is added to the table.

   :param ctx: The context object.
   :param source: The bulk to be casted.
   :param destination: The bulk to specify cast target type and store
                       casted value.
   :param add_record_if_not_exist: Whether adding a new record if
                                   ``source`` value doesn't exist in
                                   cast target table. This parameter
                                   is only used when ``destination``
                                   is a reference type bulk.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.
