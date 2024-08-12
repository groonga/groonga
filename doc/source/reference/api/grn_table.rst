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

.. c:function:: grn_id grn_table_get(grn_ctx *ctx, grn_obj *table, const void *key, unsigned int key_size)

   It finds a record that has key parameter and returns ID of the found record. If table parameter is a database, it finds an object (table, column and so on) that has key parameter  and returns ID of the found object.

   :param table: The table or database.
   :param key: The record or object key to be found.
 
.. c:function:: grn_id grn_table_at(grn_ctx *ctx, grn_obj *table, grn_id id)

   tableにidに対応するrecordが存在するか確認し、存在すれば指定されたIDを、存在しなければ ``GRN_ID_NIL`` を返します。

   注意: 実行には相応のコストがかかるのであまり頻繁に呼ばないようにして下さい。

   :param table: 対象tableを指定します。
   :param id: 検索idを指定します。

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
 
.. c:function:: grn_rc grn_table_delete(grn_ctx *ctx, grn_obj *table, const void *key, unsigned int key_size)

   tableのkeyに対応するレコードを削除します。対応するレコードが存在しない場合は ``GRN_INVALID_ARGUMENT`` を返します。
 
   :param table: 対象tableを指定します。
   :param key: 検索keyを指定します。
   :param key_size: 検索keyのサイズを指定します。

.. c:function:: grn_rc grn_table_delete_by_id(grn_ctx *ctx, grn_obj *table, grn_id id)

   tableのidに対応するレコードを削除します。対応するレコードが存在しない場合は ``GRN_INVALID_ARGUMENT`` を返します。

   :param table: 対象tableを指定します。
   :param id: レコードIDを指定します。

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

.. c:function:: grn_rc grn_table_difference(grn_ctx *ctx, grn_obj *table1, grn_obj *table2, grn_obj *res1, grn_obj *res2)

   table1とtable2から重複するレコードを取り除いた結果をそれぞれres1, res2に格納します。

   :param table1: 対象table1を指定します。
   :param table2: 対象table2を指定します。
   :param res1: 結果を格納するtableを指定します。
   :param res2: 結果を格納するtableを指定します。

.. c:function:: int grn_table_columns(grn_ctx *ctx, grn_obj *table, const char *name, unsigned int name_size, grn_obj *res)
 
   nameパラメータから始まるtableのカラムIDをresパラメータに格納します。name_sizeパラメータが0の場合はすべてのカラムIDを格納します。

   :param table: 対象tableを指定します。
   :param name: 取得したいカラム名のprefixを指定します。
   :param name_size: nameパラメータの長さを指定します。
   :param res: 結果を格納する ``GRN_TABLE_HASH_KEY`` のtableを指定します。
   :return: 格納したカラムIDの数を返します。

.. c:function:: unsigned int grn_table_size(grn_ctx *ctx, grn_obj *table)

   tableに登録されているレコードの件数を返します。

   :param table: 対象tableを指定します。

.. c:function:: grn_rc grn_table_rename(grn_ctx *ctx, grn_obj *table, const char *name, unsigned int name_size)

   ctxが使用するdbにおいてtableに対応する名前をnameに更新します。tableの全てのcolumnも同時に名前が変更されます。tableは永続オブジェクトでなければいけません。

   :param name_size: nameパラメータのsize(byte)を指定します。
