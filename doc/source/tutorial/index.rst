.. -*- rst -*-

.. highlightlang:: none

.. groonga-include : drilldown.txt

.. groonga-command
.. database: tutorial

Tag search and reverse resolution of reference relationships
============================================================

As you know, Groonga supports to store array in column which refers other table.
In fact, you can do tag search by using array data which refers other table.

Tag search is very fast because Groonga use inverted index as data structure.

Tag search
----------

Let's consider to create a search engine for an web site to share movies. Each movie may be associated with multiple keywords which represents the content of movie.

Let's create tables for movie information, then search the movies.

First, create the ``Video`` table which stores movie information. the ``Video`` table has two columns. the ``title`` column stores title of the movie. the ``tags`` column stores multiple tag information in reference Tag table.

Next, create the ``Tag`` table which stores tag information. the ``Tag`` table has one column. The tag string is stored as primary key, then ``index_tags`` stores indexes for tags column of Video table.

.. groonga-command
.. include:: ../example/tutorial/index-1.log
.. table_create --name Video --flags TABLE_HASH_KEY --key_type UInt32
.. table_create --name Tag --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table Video --name title --flags COLUMN_SCALAR --type ShortText
.. column_create --table Video --name tags --flags COLUMN_VECTOR --type Tag
.. column_create --table Tag --name index_tags --flags COLUMN_INDEX --type Video --source tags
.. load --table Video
.. [
.. {"_key":1,"title":"Soccer 2010","tags":["Sports","Soccer"]},
.. {"_key":2,"title":"Zenigata Kinjirou","tags":["Variety","Money"]},
.. {"_key":3,"title":"groonga Demo","tags":["IT","Server","groonga"]},
.. {"_key":4,"title":"Moero!! Ultra Baseball","tags":["Sports","Baseball"]},
.. {"_key":5,"title":"Hex Gone!","tags":["Variety","Quiz"]},
.. {"_key":6,"title":"Pikonyan 1","tags":["Animation","Pikonyan"]},
.. {"_key":7,"title":"Draw 8 Month","tags":["Animation","Raccoon"]},
.. {"_key":8,"title":"K.O.","tags":["Animation","Music"]}
.. ]

After creating indexed column, you can do full-text search very fast. The indexed column is also automatically updated when stored data is refreshed.

List up the movies that specific keywords are given.

.. groonga-command
.. include:: ../example/tutorial/index-2.log
.. select --table Video --query tags:@Variety --output_columns _key,title
.. select --table Video --query tags:@Sports --output_columns _key,title
.. select --table Video --query tags:@Animation --output_columns _key,title

You can search by tags such as "Variety", "Sports" and "Animation".

参照関係の逆引き
----------------

Groongaはテーブル間の参照関係の逆引きを高速に行うためのインデックスを付与することができます。タグ検索は、その1例にすぎません。

例えば、ソーシャルネットワーキングサイトにおける友人関係を逆引き検索することができます。

以下の例では、ユーザー情報を格納するUserテーブルを作成し、ユーザー名を格納するusernameカラム、ユーザーの友人一覧を配列で格納するfriendsカラムとそのインデックスのindex_friendsカラムを追加しています。

.. groonga-command
.. include:: ../example/tutorial/index-3.log
.. table_create --name User --flags TABLE_HASH_KEY --key_type ShortText
.. column_create --table User --name username --flags COLUMN_SCALAR --type ShortText
.. column_create --table User --name friends --flags COLUMN_VECTOR --type User
.. column_create --table User --name index_friends --flags COLUMN_INDEX --type User --source friends
.. load --table User
.. [
.. {"_key":"ken","username":"健作","friends":["taro","jiro","tomo","moritapo"]}
.. {"_key":"moritapo","username":"森田","friends":["ken","tomo"]}
.. {"_key":"taro","username":"ぐるんが太郎","friends":["jiro","tomo"]}
.. {"_key":"jiro","username":"ぐるんが次郎","friends":["taro","tomo"]}
.. {"_key":"tomo","username":"トモちゃん","friends":["ken","hana"]}
.. {"_key":"hana","username":"花子","friends":["ken","taro","jiro","moritapo","tomo"]}
.. ]

指定したユーザーを友人リストに入れているユーザーの一覧を表示してみましょう。

.. groonga-command
.. include:: ../example/tutorial/index-4.log
.. select --table User --query friends:@tomo --output_columns _key,username
.. select --table User --query friends:@jiro --output_columns _key,username

さらに、ドリルダウンを使って、友人リストに入っている数の一覧を表示してみましょう。

.. groonga-command
.. include:: ../example/tutorial/index-5.log
.. select --table User --limit 0 --drilldown friends

このように、テーブルの参照関係を逆にたどる検索ができました。

インデックス付きジオサーチ
--------------------------

位置情報のカラムに対して、インデックスを付与することが出来ます。大量の位置情報レコードを検索する場合に、検索速度が速くなります。

.. groonga-command
.. include:: ../example/tutorial/index-6.log
.. table_create --name GeoIndex --flags TABLE_PAT_KEY --key_type WGS84GeoPoint
.. column_create --table GeoIndex --name index_point --type Site --flags COLUMN_INDEX --source location
.. load --table Site
.. [
..  {"_key":"http://example.org/","location":"128452975x503157902"},
..  {"_key":"http://example.net/","location":"128487316x502920929"}
.. ]
.. select --table Site --filter 'geo_in_circle(location, "128515259x503187188", 5000)' --output_columns _key,location

同様に、位置情報レコードを用いてソートする場合に、ソート速度が速くなります。

.. groonga-command
.. include:: ../example/tutorial/index-7.log
.. select --table Site --filter 'geo_in_circle(location, "128515259x503187188", 50000)' --output_columns _key,location,_score --sortby '-geo_distance(location, "128515259x503187188")' --scorer '_score = geo_distance(location, "128515259x503187188")'
