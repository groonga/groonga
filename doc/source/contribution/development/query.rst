.. -*- rst -*-

クエリの実現
============

Groongaのデータベースには大量のデータを格納し、その中から必要な部分を高速に取り出すことができます。必要な部分をGroongaのデータベースに問い合わせるためのクエリの表現と実行に関して、Groongaは複数の手段を用意しています。

クエリ実行のためのインタフェース
--------------------------------

Groongaは低機能で単純なライブラリインタフェースから、高機能で複雑なコマンドインタフェースまでいくつかの階層的なインタフェースをユーザプログラムに提供しています。

クエリ実行のためのインタフェースも階層的なインタフェースのそれぞれに対応する形で用意されています。以下に低レイヤなインタフェースから順に説明します。

DB_API
^^^^^^

DB_APIは、Groongaデータベースを操作するための一群のC言語向けAPI関数を提供します。DB_APIはデータベースを構成する個々の部分に対する単純な操作関数を提供します。DB_APIの機能を組み合わせることによって複雑なクエリを実行することができます。後述のすべてのクエリインタフェースはDB_APIの機能を組み合わせることによって実現されています。

grn_expr
^^^^^^^^

grn_exprは、Groongaデータベースに対する検索処理や更新処理のための条件を表現するためのデータ構造で、複数の条件を再帰的に組み合わせてより複雑な条件を表現することができます。grn_exprによって表現されたクエリを実行するためには、grn_table_select()関数を使用します。

Groonga実行ファイル
^^^^^^^^^^^^^^^^^^^

Groongaデータベースを操作するためのコマンドインタープリタです。渡されたコマンドを解釈し、実行結果を返します。コマンドの実処理はC言語で記述されます。ユーザがC言語で定義した関数を新たなコマンドとしてGroonga実行ファイルに組み込むことができます。各コマンドはいくつかの文字列引数を受け取り、これをクエリとして解釈して実行します。引数をgrn_exprとして解釈するか、別の形式として解釈してDB_APIを使ってデータベースを操作するかはコマンド毎に自由に決めることができます。

grn_exprで表現できるクエリ
--------------------------

grn_exprは代入や関数呼び出しのような様々な操作を表現できますが、この中で検索クエリを表現するgrn_exprのことを特に条件式とよびます。条件式を構成する個々の要素を関係式と呼びます。条件式は一個以上の関係式か、あるいは条件式を論理演算子で結合したものです。

論理演算子は、以下の3種類があります。
::

 && (論理積)
 || (論理和)
 !  (否定)

関係式は、下記の11種類が用意されています。また、ユーザが定義した関数を新たな関係式として使うこともできます。
::

 equal(==)
 not_equal(!=)
 less(<)
 greater(>)
 less_equal(<=)
 greater_equal(>=)
 contain()
 near()
 similar()
 prefix()
 suffix()

grn_table_select()
------------------

grn_table_select()関数は、grn_exprで表現された検索クエリを実行するときに使います。引数として、検索対象となるテーブル、クエリを表すgrn_expr、検索結果を格納するテーブル、それに検索にマッチしたレコードを検索結果にどのように反映するかを指定する演算子を渡します。演算子と指定できるのは下記の4種類です。
::

 GRN_OP_OR
 GRN_OP_AND
 GRN_OP_BUT
 GRN_OP_ADJUST

GRN_OP_ORは、検索対象テーブルの中からクエリにマッチするレコードを検索結果テーブルに加えます。GRN_OP_OR以外の演算子は、検索結果テーブルが空でない場合にだけ意味を持ちます。GRN_OP_ANDは、検索結果テーブルの中からクエリにマッチしないレコードを取り除きます。GRN_OP_BUTは、検索結果テーブルの中からクエリにマッチするレコードを取り除きます。GRN_OP_ADJUSTは、検索結果テーブルの中でクエリにマッチするレコードに対してスコア値の更新のみを行います。

grn_table_select()は、データベース上に定義されたテーブルや索引などを組み合わせて可能な限り高速に指定されたクエリを実行しようとします。

関係式
------

関係式は、検索しようとしているデータが満たすべき条件を、指定した値の間の関係として表現します。いずれの関係式も、その関係が成り立ったときに評価されるcallback、コールバック関数に渡されるargとを引数として指定することができます。callbackが与えられず、argのみが数値で与えられた場合はスコア値の係数とみなされます。主な関係式について説明します。

equal(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値とv2の値が等しいことを表します。

not_equal(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値とv2の値が等しくないことを表します。

less(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値がv2の値よりも小さいことを表します。

greater(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値がv2の値よりも大きいことを表します。

less_equal(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値がv2の値と等しいか小さいことを表します。

greater_equal(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値がv2の値と等しいか大きいことを表します。

contain(v1, v2, mode, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値がv2の値を含んでいることを表します。また、v1の値が要素に分解されるとき、それぞれの要素に対して二つ目の要素が一致するためのmodeとして下記のいずれかを指定することができます。

::

 EXACT: v2の値もv1の値と同様に要素に分解したとき、それぞれの要素が完全に一致する(デフォルト)
 UNSPLIT: v2の値は要素に分解しない
 PREFIX: v1の値の要素がv2の値に前方一致する
 SUFFIX: v1の値の要素がv2の値に後方一致する
 PARTIAL: v1の値の要素がv2の値に中間一致する

near(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値の中に、v2の値の要素が接近して含まれていることを表します。(v2には値の配列を渡します)

similar(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値とv2の値が類似していることを表します。

prefix(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値がv2の値に対して前方一致することを表します。

suffix(v1, v2, arg, callback)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
v1の値がv2の値に対して後方一致することを表します。

クエリの実例
============

grn_exprを使って様々な検索クエリを表現することができます。

検索例1
-------
::

 GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, var);
 grn_expr_append_obj(ctx, query, contain, GRN_OP_PUSH, 1);
 grn_expr_append_obj(ctx, query, column, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, string, GRN_OP_PUSH, 1);
 grn_expr_append_op(ctx, query, GRN_OP_CALL, 3);
 result = grn_table_select(ctx, table, query, NULL, GRN_OP_OR);

tableのcolumnの値がstringを含むレコードをresultに返します。columnの値が'needle in haystack'であるレコードr1と、columnの値が'haystack'であるレコードr2がtableに登録されていたとき、stringに'needle'を指定したなら、レコードr1のみがヒットします。


検索例2
-------
::

 GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, var);
 grn_expr_append_obj(ctx, query, contain, GRN_OP_PUSH, 1);
 grn_expr_append_obj(ctx, query, column1, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, string, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, exact, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, score1, GRN_OP_PUSH, 1);
 grn_expr_append_op(ctx, query, GRN_OP_CALL, 5);
 result = grn_table_select(ctx, table, query, NULL, GRN_OP_OR);
 grn_obj_close(ctx, query);
 GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, var);
 grn_expr_append_obj(ctx, query, contain, GRN_OP_PUSH, 1);
 grn_expr_append_obj(ctx, query, column2, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, string, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, exact, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, score2, GRN_OP_PUSH, 1);
 grn_expr_append_op(ctx, query, GRN_OP_CALL, 5);
 grn_table_select(ctx, table, query, result, GRN_OP_ADJUST);
 grn_obj_close(ctx, query);

tableのcolumn1の値がstringにexactモードでヒットするレコードについて得られるスコア値にscore1を積算してresultにセットします。次に、resultにセットされたレコードのうち、column2の値がstringにexactモードでヒットするレコードについては、得られたスコア値にscore2を積算したものを、元のスコア値に加えます。

検索例3
-------
::

 GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, var);
 grn_expr_append_obj(ctx, query, contain, GRN_OP_PUSH, 1);
 grn_expr_append_obj(ctx, query, column1, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, string, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, exact, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, score1, GRN_OP_PUSH, 1);
 grn_expr_append_op(ctx, query, GRN_OP_CALL, 5);
 result = grn_table_select(ctx, table, query, NULL, GRN_OP_OR);
 grn_obj_close(ctx, query);
 if (grn_table_size(ctx, result) < t1) {
   GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, var);
   grn_expr_append_obj(ctx, query, contain, GRN_OP_PUSH, 1);
   grn_expr_append_obj(ctx, query, column1, GRN_OP_PUSH, 1);
   grn_expr_append_const(ctx, query, string, GRN_OP_PUSH, 1);
   grn_expr_append_const(ctx, query, partial, GRN_OP_PUSH, 1);
   grn_expr_append_const(ctx, query, score2, GRN_OP_PUSH, 1);
   grn_expr_append_op(ctx, query, GRN_OP_CALL, 3);
   grn_table_select(ctx, table, query, result, GRN_OP_OR);
   grn_obj_close(ctx, query);
 }

tableのcolumn1の値がstringにexactモードでヒットするレコードについて得られるスコア値にscore1を積算してresultにセットします。得られた検索結果数がt1よりも小さい場合は、partialモードで再度検索し、ヒットしたレコードについて得られるスコア値にscore2を積算してresultに追加します。

検索例4
-------
::

 GRN_EXPR_CREATE_FOR_QUERY(ctx, table, query, var);
 grn_expr_append_obj(ctx, query, contain, GRN_OP_PUSH, 1);
 grn_expr_append_const(ctx, query, string, GRN_OP_PUSH, 1);
 grn_expr_append_obj(ctx, query, column, GRN_OP_PUSH, 1);
 grn_expr_append_op(ctx, query, GRN_OP_CALL, 3);
 result = grn_table_select(ctx, table, query, NULL, GRN_OP_OR);

tableのcolumnの値がstringに含まれるレコードをresultに返します。
columnの値が'needle'であるレコードr1と、columnの値が'haystack'であるレコードr2がtableに登録されていたとき、stringに'hay in haystack'を指定したなら、レコードr2のみがヒットします。
