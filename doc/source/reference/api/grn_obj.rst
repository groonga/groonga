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

.. note::
   We are currently switching to automatic generation using Doxygen.

.. c:type:: grn_obj

   TODO...

.. c:function:: void grn_obj_unlink(grn_ctx *ctx, grn_obj *obj)

   objをメモリから解放します。objに属するobjectも再帰的にメモリから解放されます。

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
