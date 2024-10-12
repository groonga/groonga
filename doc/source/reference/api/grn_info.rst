.. -*- rst -*-

``grn_info``
============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_info_type

   TODO...

.. c:function:: grn_obj *grn_obj_get_info(grn_ctx *ctx, grn_obj *obj, grn_info_type type, grn_obj *valuebuf)

   objのtypeに対応する情報をvaluebufに格納します。

   :param obj: 対象objを指定します。
   :param type: 取得する情報の種類を指定します。
   :param valuebuf: 値を格納するバッファ（呼出側で準備）を指定します。

.. c:function:: grn_rc grn_obj_set_info(grn_ctx *ctx, grn_obj *obj, grn_info_type type, grn_obj *value)

   objのtypeに対応する情報をvalueの内容に更新します。

   :param obj: 対象objを指定します。
   :param type: 設定する情報の種類を指定します。
