.. -*- rst -*-

``grn_search``
==============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------
 
.. c:type:: grn_search_optarg

.. c:function:: grn_rc grn_obj_search(grn_ctx *ctx, grn_obj *obj, grn_obj *query, grn_obj *res, grn_operator op, grn_search_optarg *optarg)

   objを対象としてqueryにマッチするレコードを検索し、opの指定に従ってresにレコードを追加あるいは削除します。

   :param obj: 検索対象のobjectを指定します。
   :param query: 検索クエリを指定します。
   :param res: 検索結果を格納するテーブルを指定します。
   :param op: ``GRN_OP_OR``, ``GRN_OP_AND``, ``GRN_OP_AND_NOT``, ``GRN_OP_ADJUST`` のいずれかを指定します。
   :param optarg: 詳細検索条件を指定します。
