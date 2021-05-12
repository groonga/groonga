.. -*- rst -*-

``grn_ctx``
===========

Summary
-------

:c:type:`grn_ctx` is the most important object. :c:type:`grn_ctx`
keeps the current information such as:

* The last occurred error.
* The current encoding.
* The default thresholds. (e.g. :ref:`select-match-escalation-threshold`)
* The default command version. (See :doc:`/reference/command/command_version`)

:c:type:`grn_ctx` provides platform features such as:

* Memory management.
* Logging.

Most APIs receive :c:type:`grn_ctx` as the first argument.

You can't use the same :c:type:`grn_ctx` from two or more threads. You
need to create a :c:type:`grn_ctx` for a thread. You can use two or
more :c:type:`grn_ctx` in a thread but it is not needed for usual
use-case.

Example
-------

TODO...

Reference
---------

.. c:type:: grn_ctx

   TODO...

.. c:function:: grn_rc grn_ctx_init(grn_ctx *ctx, int flags)

   ctxを初期化します。

   :param ctx: 初期化するctx構造体へのポインタを指定します。
   :param flags: 初期化する ``ctx`` のオプションを指定します。
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_ctx_fin(grn_ctx *ctx)

   ctxの管理するメモリを解放し、使用を終了します。

   If ``ctx`` is initialized by :c:func:`grn_ctx_open()` not
   :c:func:`grn_ctx_init()`, you need to use
   :c:func:`grn_ctx_close()` instead of :c:func:`grn_ctx_fin()`.

   :param ctx: 解放するctx構造体へのポインタを指定します。
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_ctx *grn_ctx_open(int flags)

   初期化された :c:type:`grn_ctx` オブジェクトを返します。

   :c:func:`grn_ctx_init()` で初期化された :c:type:`grn_ctx` オブジェクトは構造体の実体をAPIの呼び元で確保するのに対して、 :c:func:`grn_ctx_open()` ではGroongaライブラリの内部で、実体を確保します。
   どちらで初期化された :c:type:`grn_ctx` も、 :c:func:`grn_ctx_fin()` で解放できます。
   :c:func:`grn_ctx_open()` で確保した :c:type:`grn_ctx` 構造体に関しては、:c:func:`grn_ctx_fin()` で解放した後に、その :c:type:`grn_ctx` で作成した :c:type:`grn_obj` を :c:func:`grn_obj_close()` によって解放しても問題ありません。

   :param flags: 初期化する ``ctx`` のオプションを指定します。
   :return: 初期化された :c:type:`grn_ctx` オブジェクトを返します。

.. c:function:: grn_rc grn_ctx_close(grn_ctx *ctx)

   It calls :c:func:`grn_ctx_fin()` and frees allocated memory for ``ctx`` by :c:func:`grn_ctx_open()`.

   :param ctx: no longer needed :c:type:`grn_ctx`.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_rc grn_ctx_set_finalizer(grn_ctx *ctx, grn_proc_func *func)

   ctxを破棄するときに呼ばれる関数を設定します。

   :param ctx: 対象ctxを指定します。
   :param func: ``ctx`` を破棄するときに呼ばれる関数を指定します。
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_command_version grn_ctx_get_command_version(grn_ctx *ctx)

   command_versionを返します。

.. c:function:: grn_rc grn_ctx_set_command_version(grn_ctx *ctx, grn_command_version version)

   command_versionを変更します。

   :param version: 変更後のcommand_versionを指定します。

.. c:function:: grn_rc grn_ctx_use(grn_ctx *ctx, grn_obj *db)

   ctxが操作対象とするdbを指定します。NULLを指定した場合は、dbを操作しない状態(init直後の状態)になります。

   Don't use it with :c:type:`grn_ctx` that has ``GRN_CTX_PER_DB`` flag.

   :param db: ctxが使用するdbを指定します。

.. c:function:: grn_obj *grn_ctx_db(grn_ctx *ctx)

   ctxが現在操作対象としているdbを返します。dbを使用していない場合はNULLを返します。

.. c:function:: grn_obj *grn_ctx_get(grn_ctx *ctx, const char *name, int name_size)

   ctxが使用するdbからnameに対応するオブジェクトを検索して返す。nameに一致するオブジェクトが存在しなければNULLを返す。

   :param name: 検索しようとするオブジェクトの名前。
   :param name_size: The number of bytes of name. If negative value is specified, name is assumed that NULL-terminated string.

.. c:function:: grn_obj *grn_ctx_at(grn_ctx *ctx, grn_id id)

   ctx、またはctxが使用するdbからidに対応するオブジェクトを検索して返す。idに一致するオブジェクトが存在しなければNULLを返す。

   :param id: 検索しようとするオブジェクトのidを指定します。

.. c:function:: grn_rc grn_ctx_get_all_tables(grn_ctx *ctx, grn_obj *tables_buffer)

   It pushes all tables in the database of ``ctx`` into
   ``tables_buffer``. ``tables_buffer`` should be initialized as
   ``GRN_PVECTOR``. You can use ``GRN_PTR_INIT()`` with
   ``GRN_OBJ_VECTOR`` flags to initialize ``tables_buffer``.

   Here is an example:

   .. code-block :: c

      grn_rc rc;
      grn_obj tables;
      int i;
      int n_tables;

      GRN_PTR_INIT(&tables, GRN_OBJ_VECTOR, GRN_ID_NIL);
      rc = grn_ctx_get_all_tables(ctx, &tables);
      if (rc != GRN_SUCCESS) {
        GRN_OBJ_FIN(ctx, &tables);
        /* Handle error. */
        return;
      }

      n_tables = GRN_BULK_VSIZE(&tables) / sizeof(grn_obj *);
      for (i = 0; i < n_tables; i++) {
        grn_obj *table = GRN_PTR_VALUE_AT(&tables, i);
        /* Use table. */
      }

      /* Free resources. */
      for (i = 0; i < n_tables; i++) {
        grn_obj *table = GRN_PTR_VALUE_AT(&tables, i);
        grn_obj_unlink(ctx, table);
      }
      GRN_OBJ_FIN(ctx, &tables);


   :param ctx: The context object.
   :param table_buffer: The output buffer to store tables.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_content_type grn_ctx_get_output_type(grn_ctx *ctx)

   Gets the current output type of the context.

   Normally, this function isn't needed.

   :param ctx: The context object.
   :return: The output type of the context.

.. c:function:: grn_rc grn_ctx_set_output_type(grn_ctx *ctx, grn_content_type type)

   Sets the new output type to the context. It is used by executing a
   command by :c:func:`grn_expr_exec()`. If you use
   :c:func:`grn_ctx_send()`, the new output type isn't
   used. :c:func:`grn_ctx_send()` sets output type from command line
   internally.

   Normally, this function isn't needed.

   :param ctx: The context object.
   :param type: The new output type.
   :return: ``GRN_SUCCESS`` on success, not ``GRN_SUCCESS`` on error.

.. c:function:: grn_bool_rc grn_ctx_is_opened(grn_ctx *ctx, grn_id id)

   Checks whether object with the ID is opened or not.

   :param ctx: The context object.
   :param id: The object ID to be checked.
   :return: ``GRN_TRUE`` if object with the ID is opened,
            ``GRN_FALSE`` otherwise.
