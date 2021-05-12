.. -*- rst -*-

``grn_table_cursor``
====================

Summary
-------

TODO...

Example
-------

TODO...

Reference
---------

.. c:type:: grn_table_cursor

   TODO...

.. c:function:: grn_table_cursor *grn_table_cursor_open(grn_ctx *ctx, grn_obj *table, const void *min, unsigned int min_size, const void *max, unsigned int max_size, int offset, int limit, int flags)

   tableに登録されているレコードを順番に取り出すためのカーソルを生成して返します。

   :param table: 対象tableを指定します。
   :param min: keyの下限を指定します。（NULLは下限なしと見なします。） ``GRN_CURSOR_PREFIX`` については後述。
   :param min_size: minのsizeを指定します。``GRN_CURSOR_PREFIX`` については後述。
   :param max: keyの上限を指定します。（NULLは上限なしと見なします。） ``GRN_CURSOR_PREFIX`` については後述。
   :param max_size: maxのsizeを指定します。``GRN_CURSOR_PREFIX`` については無視される場合があります。
   :param flags:
      ``GRN_CURSOR_ASCENDING`` を指定すると昇順にレコードを取り出します。

      ``GRN_CURSOR_DESCENDING`` を指定すると降順にレコードを取り出します。(下記 ``GRN_CURSOR_PREFIX`` を指定し、keyが近いレコードを取得する場合、もしくは、common prefix searchを行う場合には、``GRN_CURSOR_ASCENDING`` / ``GRN_CURSOR_DESCENDING`` は無視されます。)

      ``GRN_CURSOR_GT`` を指定するとminに一致したkeyをcursorの範囲に含みません。(minがNULLの場合もしくは、下記 ``GRN_CURSOR_PREFIX`` を指定し、keyが近いレコードを取得する場合、もしくは、common prefix searchを行う場合には、``GRN_CURSOR_GT`` は無視されます。)

      ``GRN_CURSOR_LT`` を指定するとmaxに一致したkeyをcursorの範囲に含みません。(maxがNULLの場合もしくは、下記 ``GRN_CURSOR_PREFIX`` を指定した場合には、``GRN_CURSOR_LT`` は無視されます。)

      ``GRN_CURSOR_BY_ID`` を指定するとID順にレコードを取り出します。(下記 ``GRN_CURSOR_PREFIX`` を指定した場合には、``GRN_CURSOR_BY_ID`` は無視されます。) ``GRN_OBJ_TABLE_PAT_KEY`` を指定したtableについては、``GRN_CURSOR_BY_KEY`` を指定するとkey順にレコードを取り出します。( ``GRN_OBJ_TABLE_HASH_KEY`` , ``GRN_OBJ_TABLE_NO_KEY`` を指定したテーブルでは ``GRN_CURSOR_BY_KEY`` は無視されます。)

      ``GRN_CURSOR_PREFIX`` を指定すると、 ``GRN_OBJ_TABLE_PAT_KEY`` を指定したテーブルに関する下記のレコードを取り出すカーソルが作成されます。maxがNULLの場合には、keyがminと前方一致するレコードを取り出します。max_sizeパラメータは無視されます。

      maxとmax_sizeが指定され、かつ、テーブルのkeyがShortText型である場合、maxとcommon prefix searchを行い、common prefixがmin_sizeバイト以上のレコードを取り出します。minは無視されます。

      maxとmax_sizeが指定され、かつ、テーブルのkeyが固定長型の場合、maxとPAT木上で近い位置にあるノードから順番にレコードを取り出します。ただし、keyのパトリシア木で、min_sizeバイト未満のビットに対するノードで、maxと異なった方向にあるノードに対応するレコードについては取り出しません。PAT木上で位置が近いこととkeyの値が近いことは同一ではありません。この場合、maxで与えられるポインタが指す値は、対象テーブルのkeyサイズと同じか超える幅である必要があります。minは無視されます。

      ``GRN_CURSOR_BY_ID`` / ``GRN_CURSOR_BY_KEY`` / ``GRN_CURSOR_PREFIX`` の3フラグは、同時に指定することができません。

      ``GRN_OBJ_TABLE_PAT_KEY`` を指定して作ったテーブルで、``GRN_CURSOR_PREFIX`` と ``GRN_CURSOR_RK`` を指定すると、半角小文字のアルファベット文字列から、それを旧JIS X 4063:2000規格に従って全角カタカナに変換した文字列に前方一致する値をkeyとするレコードを取り出します。``GRN_ENC_UTF8`` のみをサポートしています。``GRN_CURSOR_ASCENDING`` / ``GRN_CURSOR_DESCENDING`` は無効であり、レコードをkey値の昇降順で取り出すことはできません。
  
   :param offset:
      該当する範囲のレコードのうち、(0ベースで)offset番目からレコードを取り出します。

      ``GRN_CURSOR_PREFIX`` を指定したときは負の数を指定することはできません。

   :param limit:
      該当する範囲のレコードのうち、limit件のみを取り出します。-1が指定された場合は、全件が指定されたものとみなします。

      ``GRN_CURSOR_PREFIX`` を指定したときは-1より小さい負の数を指定することはできません。

.. c:function:: grn_rc grn_table_cursor_close(grn_ctx *ctx, grn_table_cursor *tc)

   :c:func:`grn_table_cursor_open` で生成したcursorを解放します。

   :param tc: 対象cursorを指定します。

.. c:function:: grn_id grn_table_cursor_next(grn_ctx *ctx, grn_table_cursor *tc)

   cursorのカレントレコードを一件進めてそのIDを返します。cursorの対象範囲の末尾に達すると ``GRN_ID_NIL`` を返します。

   :param tc: 対象cursorを指定します。

.. c:function:: int grn_table_cursor_get_key(grn_ctx *ctx, grn_table_cursor *tc, void **key)

   cursorのカレントレコードのkeyをkeyパラメータにセットし、その長さを返します。

   :param tc: 対象cursorを指定します。
   :param key: カレントレコードのkeyへのポインタがセットされます。

.. c:function:: int grn_table_cursor_get_value(grn_ctx *ctx, grn_table_cursor *tc, void **value)

   cursorパラメータのカレントレコードのvalueをvalueパラメータにセットし、その長さを返します。

   :param tc: 対象cursorを指定します。
   :param value: カレントレコードのvalueへのポインタがセットされます。

.. c:function:: grn_rc grn_table_cursor_set_value(grn_ctx *ctx, grn_table_cursor *tc, const void *value, int flags)

   cursorのカレントレコードのvalueを引数の内容に置き換えます。cursorのカレントレコードが存在しない場合は ``GRN_INVALID_ARGUMENT`` を返します。

   :param tc: 対象cursorを指定します。
   :param value: 新しいvalueの値を指定します。
   :param flags: :c:func:`grn_obj_set_value()` のflagsと同様の値を指定できます。

.. c:function:: grn_rc grn_table_cursor_delete(grn_ctx *ctx, grn_table_cursor *tc)

   cursorのカレントレコードを削除します。cursorのカレントレコードが存在しない場合は ``GRN_INVALID_ARGUMENT`` を返します。

   :param tc: 対象cursorを指定します。

.. c:function:: grn_obj *grn_table_cursor_table(grn_ctx *ctx, grn_table_cursor *tc)

   cursorが属するtableを返します。

   :param tc: 対象cursorを指定します。
