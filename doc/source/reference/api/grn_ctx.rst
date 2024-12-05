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

