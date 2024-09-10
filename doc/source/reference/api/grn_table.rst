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

.. c:function:: int grn_table_get_key(grn_ctx *ctx, grn_obj *table, grn_id id, void *keybuf, int buf_size)

   tableのIDに対応するレコードのkeyを取得します。

   対応するレコードが存在する場合はkey長を返します。見つからない場合は0を返します。対応するキーの検索に成功し、またbuf_sizeの長さがkey長以上であった場合は、keybufに該当するkeyをコピーします。

   :param table: 対象tableを指定します。
   :param id: 対象レコードのIDを指定します。
   :param keybuf: keyを格納するバッファ(呼出側で準備する)を指定します。
   :param buf_size: keybufのサイズ(byte長)を指定します。

.. c:function:: grn_rc grn_table_update_by_id(grn_ctx *ctx, grn_obj *table, grn_id id, const void *dest_key, unsigned int dest_key_size)

   tableのidに対応するレコードのkeyを変更します。新しいkeyとそのbyte長をdest_keyとdest_key_sizeに指定します。

   この操作は、``GRN_TABLE_DAT_KEY`` 型のテーブルのみ使用できます。

   :param table: 対象tableを指定します。
   :param id: レコードIDを指定します。

.. c:function:: grn_rc grn_table_truncate(grn_ctx *ctx, grn_obj *table)

   tableの全レコードを一括して削除します。

   注意: multithread環境では他のthreadのアクセスによって、存在しないアドレスへアクセスし、SIGSEGVが発生する可能性があります。

   :param table: 対象tableを指定します。

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

.. c:function:: grn_rc grn_table_setoperation(grn_ctx *ctx, grn_obj *table1, grn_obj *table2, grn_obj *res, grn_operator op)

   table1とtable2をopの指定に従って集合演算した結果をresに格納します。

   resにtable1あるいはtable2そのものを指定した場合を除けば、table1, table2は破壊されません。

   :param table1: 対象table1を指定します。
   :param table2: 対象table2を指定します。
   :param res: 結果を格納するtableを指定します。
   :param op: 実行する演算の種類を指定します。
