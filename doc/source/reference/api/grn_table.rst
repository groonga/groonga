.. -*- rst -*-

``grn_table``
=============

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_table_sort_key

   TODO...

.. c:type:: grn_table_sort_flags

   TODO...

.. c:type:: grn_table_group_result

   TODO...

.. c:type:: grn_table_group_flags

   TODO...

.. c:function:: grn_rc grn_table_group(grn_ctx *ctx, grn_obj *table, grn_table_sort_key *keys, int n_keys, grn_table_group_result *results, int n_results)

   tableのレコードを特定の条件でグループ化します。

   :param table: 対象tableを指定します。
   :param keys: group化キー構造体の配列へのポインタを指定します。
   :param n_keys: group化キー構造体の配列のサイズを指定します。
   :param results: group化の結果を格納する構造体の配列へのポインタを指定します。
   :param n_results: group化の結果を格納する構造体の配列のサイズを指定します。
