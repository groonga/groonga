.. -*- rst -*-

.. highlightlang:: none

``grn_obj``
===========

Summary
-------

:c:type:`grn_obj` is the object which stores comprehensive data such as built-in object, table, column and so on.

Reference
---------

.. c:type:: grn_obj

   TODO...

.. c:function:: grn_obj *grn_obj_column(grn_ctx *ctx, grn_obj *table, const char *name, unsigned int name_size)

   nameがカラム名の場合、それに対応するtableのカラムを返します。対応するカラムが存在しなければNULLを返します。

   nameはアクセサ文字列の場合、それに対応するaccessorを返します。アクセサ文字列とは、カラム名等を'.'で連結した文字列です。'_id', '_key'は特殊なアクセサで、それぞれレコードID/keyを返します。例) 'col1' / 'col2.col3' / 'col2._id'

   :param table: 対象tableを指定します。
   :param name: カラム名を指定します。

.. c:function:: grn_bool grn_obj_is_accessor(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.1.0

   Check whether target object is a accessor object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is accessor, ``GRN_FALSE`` otherwise.

.. c:function:: grn_bool grn_obj_is_builtin(grn_ctx *ctx, grn_obj *obj)

   Check whether target object is a built-in object. The built-in objects are listed in :doc:`/reference/types`.

   :param ctx: context
   :param obj: target object
   :return: ``GRN_TRUE`` for built-in groonga object, ``GRN_FALSE`` otherwise.

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Int8", strlen("Int8"));
      printf("builtin?: %s\n", grn_obj_is_builtin(ctx, obj) ? "true" : "false");

   As ``Int8`` is built-in object, it prints like the following::

     builtin?: true

.. c:function:: grn_bool grn_obj_is_bulk(grn_ctx *ctx, grn_obj *obj);

   Check whether target object is a bulk object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is bulk, ``GRN_FALSE`` otherwise.

.. c:function:: grn_bool grn_obj_is_column(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 6.0.0

   Check whether target object is a column object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is column, ``GRN_FALSE`` otherwise.

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Users.name", strlen("Users.name"));
      printf("column?: %s\n", grn_obj_is_column(ctx, obj) ? "true" : "false");

   If ``Users.name`` is column object, it prints like the following::

     column?: true

.. c:function:: grn_bool grn_obj_is_corrupt(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 7.0.4

   Check whether target object is corrupted.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is corrupted, ``GRN_FALSE`` otherwise.

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Users", strlen("Users"));
      printf("corrupted?: %s\n", grn_obj_is_corrupt(ctx, obj) ? "true" : "false");

   If ``Users`` table is corrupted, it prints like the following::

     corrupted?: true

.. c:function:: grn_bool grn_obj_is_data_column(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 7.0.1

   Check whether column is a data column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is data column, ``GRN_FALSE`` otherwise. Even if the target object isn't column, return ``GRN_FALSE``.

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Users.name", strlen("Users.name"));
      printf("data column?: %s\n", grn_obj_is_data_column(ctx, obj) ? "true" : "false");

   If ``Users.name`` is data column, it prints like the following::

     data column?: true

.. c:function:: grn_bool grn_obj_is_dirty(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 6.0.5

   Check whether target object is marked as dirty.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object status is dirty. ``GRN_FALSE`` otherwise. TODO: dirty

.. c:function:: grn_bool grn_obj_is_expr(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 7.0.1

   Check whether target object is an expression object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is expression object. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_function_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 5.0.1

   Check whether target object is a function procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is function object. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_index_column(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 8.0.4

   Check whether index column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is index column, ``GRN_FALSE`` otherwise. Even if the target object isn't column, return ``GRN_FALSE``.

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Terms.name_index", strlen("Terms.name_index"));
      printf("index column?: %s\n", grn_obj_is_index_column(ctx, obj) ? "true" : "false");

   If ``Terms.name_index`` is index column, it prints like the following::

     index column?: true

.. c:function:: grn_bool grn_obj_is_id_accessor(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 9.0.2

   Check whether target object is a id accessor.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is id accessor. ``GRN_FALSE`` otherwise. TODO: difference between is_accessor and is_id_accessor.

.. c:function:: grn_bool grn_obj_is_key_accessor(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.1.0

   Check whether target object is a key accessor.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a key accessor. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_lexicon(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 8.0.2

   Check whether target object is a lexicon.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a lexicon. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_number_family_bulk(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 8.0.4

   Check whether target object is a number bulk object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a number bulk object. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_normalizer_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a normalizer object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a normalizer object. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a procedure. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_reference_column(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 6.0.0

   Check whether target object is a reference column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a reference column. ``GRN_FALSE`` otherwise. TODO:

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Site.user", strlen("Site.user"));
      printf("reference column?: %s\n", grn_obj_is_reference_column(ctx, obj) ? "true" : "false");

   If ``Site.user`` is reference column, it prints like the following::

     reference column?: true

.. c:function:: grn_bool grn_obj_is_scalar_column(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 7.0.1

   Check whether target object is a scalar column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a scalar column. ``GRN_FALSE`` otherwise. TODO:

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Users.name", strlen("Users.name"));
      printf("scalar column?: %s\n", grn_obj_is_scalar_column(ctx, obj) ? "true" : "false");

   If ``Users.name`` is scalar column, it prints like the following::

     scalar column?: true

.. c:function:: grn_bool grn_obj_is_scorer_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a scorer procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a scorer procedure. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_selector_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a selector procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a selector procedure. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_selector_only_proc(grn_ctx *ctx, grn_obj *obj)

   Check whether target object is a selector only procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a selector only procedure. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_table(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a table.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a table. ``GRN_FALSE`` otherwise. TODO:

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Users", strlen("Users"));
      printf("table?: %s\n", grn_obj_is_table(ctx, obj) ? "true" : "false");

   If ``Users`` is table object, it prints like the following::

     table?: true

.. c:function:: grn_bool grn_obj_is_text_family_bulk(grn_ctx *ctx, grn_obj *obj)

   Check whether target object is a bulk object which belongs to text family.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a bulk object which belongs to text family. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_text_family_type(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 6.0.1

   Check whether target object is a type which belongs to text family.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a type which belongs to a text family. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_tokenizer_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a tokenizer procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a tokenizer procedure. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_token_filter_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a token filter procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a token filter procedure. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_true(grn_ctx *ctx, grn_obj *obj)

   Check whether target object is true.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is true. ``GRN_FALSE`` otherwise. If target object type is ``Int32`` or ``UInt32``, ``GRN_TRUE`` means that the value is not zero. If target object type is ``ShortText``, ``GRN_TRUE`` means that it is not empty string.

.. c:function:: grn_bool grn_obj_is_type(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a type.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is type object. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_vector(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 8.0.8

   Check whether target object is a vector.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a vector object. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: grn_bool grn_obj_is_vector_column(grn_ctx *ctx, grn_obj *obj)

   .. versionadded:: 8.0.4

   Check whether target object is a vector column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is vector column, ``GRN_FALSE`` otherwise. Even if the target object isn't column, return ``GRN_FALSE``.

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Users.names", strlen("Users.names"));
      printf("vector column?: %s\n", grn_obj_is_vector_column(ctx, obj) ? "true" : "false");

   If ``Users.names`` is vector column, it prints like the following::

     vector column?: true

.. c:function:: grn_bool grn_obj_is_weight_vector_column(grn_ctx *ctx, grn_obj *obj)

   Check whether target object is a weight vector column.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a weight vector column. ``GRN_FALSE`` otherwise.
            Even if the target object isn't column, return ``GRN_FALSE``. The weight vector column is created with ``WITH_WEIGHT`` flags.

   .. code-block:: c

      grn_obj *obj;
      obj = grn_ctx_get(ctx, "Users.tags", strlen("Users.tags"));
      printf("weight vector column?: %s\n", grn_obj_is_weight_vector_column(ctx, obj) ? "true" : "false");

   If ``Users.tags`` is weight vector column, it prints like the following::

     weight vector column?: true

.. c:function:: grn_bool grn_obj_is_window_function_proc(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.0.9

   Check whether target object is a window function procedure.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_TRUE`` for if the target object is a weight vector column. ``GRN_FALSE`` otherwise. TODO:

.. c:function:: size_t grn_obj_get_disk_usage(grn_ctx *ctx, grn_obj *obj)

   Check disk usage of target object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: The amount of disk usage about specified ``obj``.

.. c:function:: uint32_t grn_obj_get_last_modified(grn_ctx *ctx, grn_obj *obj, grn_timeval *tv)

   .. versionadded:: 6.0.5

   Check last modified timestamp of target object.

   :param ctx: The context object.
   :param obj: The target object.
   :param tv: TODO:
   :return: TODO:

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

.. c:function:: grn_bool grn_obj_name_is_column(grn_ctx *ctx, const char *name, int name_len)

   :param ctx: The context object.
   :param name: The target name.
   :param name_len: The length of target name.
   :return: ``GRN_TRUE`` for if the target name is column, ``GRN_FALSE`` otherwise.

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

.. c:function:: grn_rc grn_obj_cast(grn_ctx *ctx, grn_obj *source, grn_obj *destination, grn_bool add_record_if_not_exist)

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

.. c:function:: grn_rc grn_obj_reindex(grn_ctx *ctx, grn_obj *obj)

   .. versionadded: 5.1.0

   Reindex target object.

   :param ctx: The context object.
   :param obj: The target object.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: void grn_obj_touch(grn_ctx *ctx, grn_obj *obj, grn_timeval *tv)

   .. versionadded:: 6.0.5

   Touch target object.

   :param ctx: The context object.
   :param obj: The target object.

.. c:function:: const char *grn_obj_type_to_string(uint8_t type);

   Convert specified type to string.

   :param type: The type.
   :return: string corresponding to specified ``type``.

.. c:function:: grn_rc grn_obj_set_option_values(grn_ctx *ctx, grn_obj *obj, const char *name, int name_length, grn_obj *values)

   Set specified option values.

   :param ctx: The context object.
   :param obj: The target object.
   :param name: The name of ``values``.
   :param name_length: The length of ``name``.
   :param values: The option value.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_option_revision grn_obj_get_option_values(grn_ctx *ctx, grn_obj *obj, const char *name, int name_length, grn_option_revision revision, grn_obj *values)
   Get specified option values.

   :param ctx: The context object.
   :param obj: The target object.
   :param name: The name of ``values``.
   :param name_length: The length of ``name``.
   :param revision: The revision of specified option.
   :param values: The option value.
   :return: TODO:

.. c:function:: grn_rc grn_obj_clear_option_values(grn_ctx *ctx, grn_obj *obj)

   Clear option values.

   :param ctx: The context object.
   :param obj: The target object.
   :param name_length: The length of ``name``.
   :param revision: The revision of specified option.
   :param values: The option value.
   :return: TODO:
