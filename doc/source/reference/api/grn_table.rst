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

.. c:function:: grn_id grn_table_lcp_search(grn_ctx *ctx, grn_obj *table, const void *key, unsigned int key_size)

   tableが ``GRN_TABLE_PAT_KEY`` もしくは ``GRN_TABLE_DAT_KEY`` を指定して作ったtableなら、longest common prefix searchを行い、対応するIDを返します。

   tableが ``GRN_TABLE_HASH_KEY`` を指定して作ったtableなら、完全に一致するキーを検索し、対応するIDを返します。

   :param table: 対象tableを指定します。
   :param key: 検索keyを指定します。

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
