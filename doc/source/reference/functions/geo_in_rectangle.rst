.. -*- rst -*-

geo_in_rectangle
================

名前
----

geo_in_rectangle - 座標が矩形の範囲内に存在するかどうかを調べます。

書式
----
::

 geo_in_rectangle(point, top_left, bottom_right)

説明
----

Groonga組込関数の一つであるgeo_in_rectangleについて説明します。組込関数は、script形式のgrn_expr中で呼び出すことができます。

geo_in_rectangle() 関数は、pointに指定した座標が、top_leftとbottom_rightがなす矩形の範囲内にあるかどうかを調べます。

引数
----

``point``

  矩形の範囲内に存在するかどうかを調べる座標を指定します。Point型の値を指定できます。 [#]_

``top_left``

  矩形の左上隅となる座標を指定します。Point型の値、あるいは座標を示す文字列を指定できます。

``bottom_right``

  矩形の右下隅となる座標を指定します。Point型の値、あるいは座標を示す文字列を指定できます。

返値
----

pointに指定した座標が矩形の範囲内にあるかどうかをBool型の値で返します。

例
--
::

 geo_in_rectangle(pos, "150x100", "100x150")
 true

.. rubric:: 脚注

.. [#] TokyoGeoPoint(日本測地系座標)かWGS84GeoPoint(世界測地系座標)のいずれかを指定できます。
