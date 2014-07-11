.. -*- rst -*-

.. highlightlang:: none

.. groonga-include : data.txt

.. groonga-command
.. database: tutorial

Various search conditions
=========================

Groonga supports to narrow down by using syntax like JavaScript, sort by the calculated value. Additionally, Groonga also supports to narrow down & sort search results by using location information (latitude & longitude).

Narrow down & Full-text search by using syntax like JavaScript
--------------------------------------------------------------

The ``filter`` parameter of ``select`` command accepts the search condition.
There is one difference between ``filter`` parameter and ``query`` parameter, you need to specify the condition by syntax like JavaScript for ``filter`` parameter.

.. groonga-command
.. include:: ../example/tutorial/search-1.log
.. select --table Site --filter "_id <= 1" --output_columns _id,_key

See the detail of above query. Here is the condition which is specified as ``filter`` parameter::

  _id <= 1

In this case, this query returns the records which meets the condition that the value of ``_id`` is equal to 1.

Moreover, you can use ``&&`` for AND search, ``||`` for OR search.

.. groonga-command
.. include:: ../example/tutorial/search-2.log
.. select --table Site --filter "_id >= 4 && _id <= 6" --output_columns _id,_key
.. select --table Site --filter "_id <= 2 || _id >= 7" --output_columns _id,_key

If you specify ``query`` parameter and ``filter`` parameter at the same time, you can get the records which meets both of the condition as a result.

Sort by using ``scorer``
------------------------

``select`` command accepts ``scorer`` parameter which is used to process each record of full-text search results.

This parameter accepts the conditions which is specified by syntax like JavaScript as same as ``filter`` parameter.

.. groonga-command
.. include:: ../example/tutorial/search-3.log
.. select --table Site --filter "1" --scorer "_score = rand()" --output_columns _id,_key,_score --sortby _score
.. select --table Site --filter "1" --scorer "_score = rand()" --output_columns _id,_key,_score --sortby _score

'_score' is one of a pseudo column. The score of full-text search is assigned to it.
See :doc:`/reference/columns/pseudo` about '_score' column.

In the above query, the condition of ``scorer`` parameter is::

  _score = rand()

In this case, the score of full-text search is overwritten by the value of rand() function.

The condition of ``sortby`` parameter is::

  _score

This means that sorting the search result by ascending order.

As a result, the order of search result is randomized.

位置情報を用いた絞込・ソート
----------------------------

Groongaでは、位置情報（経緯度）を保存することができます。また、保存した経緯度を用いて絞込やソートができます。

位置情報を保存するためのカラムの型として、TokyoGeoPoint/WGS84GeoPointの２つの型があります。前者は日本測地系、後者は世界測地系(WGS84相当)の経緯度を保存します。

経緯度は以下のいずれかの形式の文字列として指定します。

* "[緯度のミリ秒]x[経度のミリ秒]"（例: "128452975x503157902"）
* "[緯度のミリ秒],[経度のミリ秒]"（例: "128452975,503157902"）
* "[緯度の小数表記の度数]x[経度の小数表記の度数]"（例: "35.6813819x139.7660839"）
* "[緯度の小数表記の度数],[経度の小数表記の度数]"（例: "35.6813819,139.7660839"）

ここでは、ためしに東京駅と新宿駅とついて、世界測地系での位置情報を保存してみましょう。東京駅は緯度が35度40分52.975秒、経度が139度45分57.902秒です。新宿駅は緯度が35度41分27.316秒、経度が139度42分0.929秒です。よって、ミリ秒表記の場合はそれぞれ"128452975x503157902"/"128487316x502920929"となります。度数表記の場合はそれぞれ"35.6813819x139.7660839"/"35.6909211x139.7002581"となります。ここではミリ秒表記で登録しましょう。

.. groonga-command
.. include:: ../example/tutorial/search-4.log
.. column_create --table Site --name location --type WGS84GeoPoint
.. load --table Site
.. [
..  {"_key":"http://example.org/","location":"128452975x503157902"}
..  {"_key":"http://example.net/","location":"128487316x502920929"},
.. ]
.. select --table Site --query "_id:1 OR _id:2" --output_columns _key,location

scorerパラメータにおいて、 :doc:`/reference/functions/geo_distance` 関数を用いることにより、２点間の距離を計算することができます。

ここでは、秋葉原駅からの距離を表示させてみましょう。世界測地系では、秋葉原駅の位置は緯度が35度41分55.259秒、経度が139度46分27.188秒です。よって、geo_distance関数に与える文字列は"128515259x503187188"となります。

.. groonga-command
.. include:: ../example/tutorial/search-5.log
.. select --table Site --query "_id:1 OR _id:2" --output_columns _key,location,_score --scorer '_score = geo_distance(location, "128515259x503187188")'

この結果を見ると、東京駅と秋葉原駅は2054m、秋葉原駅と新宿駅は6720m離れているようです。

geo_distance関数は、_scoreを通じてソートでも用いることができます。

.. groonga-command
.. include:: ../example/tutorial/search-6.log
.. select --table Site --query "_id:1 OR _id:2" --output_columns _key,location,_score --scorer '_score = geo_distance(location, "128515259x503187188")' --sortby -_score

「ある地点から何m以内に存在する」といった絞込も可能です。

filterパラメータにおいて、 :doc:`/reference/functions/geo_in_circle` 関数を用いることにより、２点間の距離が指定のm以下におさまるかどうかを判定することができます。

たとえば、秋葉原駅から5000m以内にあるレコードを検索してみましょう。

.. groonga-command
.. include:: ../example/tutorial/search-7.log
.. select --table Site --output_columns _key,location --filter 'geo_in_circle(location, "128515259x503187188", 5000)'

また、経緯度が指定の矩形領域内であるかどうかを判定する :doc:`/reference/functions/geo_in_rectangle` 関数も存在します。
