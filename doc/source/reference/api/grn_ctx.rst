.. -*- rst -*-

``grn_ctx``
===========

Summary
-------

Example
-------

TODO...

Reference
---------

.. c:type:: grn_ctx

   TODO...

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
