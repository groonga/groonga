.. -*- rst -*-

edit_distance
=============

名前
----

edit_distance - 指定した2つの文字列の編集距離を計算する

書式
----
::

 edit_distance(string1, string2)

説明
----

Groonga組込関数の一つであるedit_distanceについて説明します。組込関数は、script形式のgrn_expr中で呼び出すことができます。

edit_distance() 関数は、string1に指定した文字列とstring2に指定した文字列の間の編集距離を求めます。

引数
----

``string1``

  文字列を指定します

``string2``

  もうひとつの文字列を指定します


返値
----

指定した2つ文字列の編集距離をUint32型の値として返します。

例
--
::

 edit_distance(title, "hoge")
 1
