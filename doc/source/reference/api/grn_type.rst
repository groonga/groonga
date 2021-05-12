.. -*- rst -*-

``grn_type``
============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_builtin_type

   TODO...

.. c:function:: grn_obj *grn_type_create(grn_ctx *ctx, const char *name, unsigned int name_size, grn_obj_flags flags, unsigned int size)

   nameに対応する新たなtype（型）をdbに定義します。

   :param name: 作成するtypeの名前を指定します。
   :param flags: ``GRN_OBJ_KEY_VAR_SIZE``, ``GRN_OBJ_KEY_FLOAT``, ``GRN_OBJ_KEY_INT``, ``GRN_OBJ_KEY_UINT`` のいずれかを指定します。
   :param size: ``GRN_OBJ_KEY_VAR_SIZE`` の場合は最大長、それ以外の場合は長さ（単位:byte）を指定します。
