.. -*- rst -*-

.. highlightlang:: none

.. groonga-command
.. database: tutorial-micro-blog

マイクロブログ検索システムの作成
================================

これまで学んだGroongaの機能を用いて、マイクロブログの検索システムを作成してみましょう。マイクロブログとは、Twitterのような短いメッセージを投稿するブログです。

テーブルの作成
--------------

まずは、テーブルを作成します。

::

 table_create --name Users --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name Comments --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name HashTags --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name Bigram --flags TABLE_PAT_KEY|KEY_NORMALIZE --key_type ShortText --default_tokenizer TokenBigram
 table_create --name GeoIndex --flags TABLE_PAT_KEY --key_type WGS84GeoPoint

 column_create --table Users --name name --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name follower --flags COLUMN_VECTOR --type Users
 column_create --table Users --name favorites --flags COLUMN_VECTOR --type Comments
 column_create --table Users --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
 column_create --table Users --name location_str --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name description --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name followee --flags COLUMN_INDEX --type Users --source follower

 column_create --table Comments --name comment --flags COLUMN_SCALAR --type ShortText
 column_create --table Comments --name last_modified --flags COLUMN_SCALAR --type Time
 column_create --table Comments --name replied_to --flags COLUMN_SCALAR --type Comments
 column_create --table Comments --name replied_users --flags COLUMN_VECTOR --type Users
 column_create --table Comments --name hash_tags --flags COLUMN_VECTOR --type HashTags
 column_create --table Comments --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
 column_create --table Comments --name posted_by --flags COLUMN_SCALAR --type Users
 column_create --table Comments --name favorited_by --flags COLUMN_INDEX --type Users --source favorites

 column_create --table HashTags --name hash_index --flags COLUMN_INDEX --type Comments --source hash_tags

 column_create --table Bigram --name users_index --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION --type Users --source name,location_str,description
 column_create --table Bigram --name comment_index --flags COLUMN_INDEX|WITH_POSITION --type Comments --source comment

 column_create --table GeoIndex --name users_location --type Users --flags COLUMN_INDEX --source location
 column_create --table GeoIndex --name comments_location --type Comments --flags COLUMN_INDEX --source location

Usersテーブル
^^^^^^^^^^^^^

ユーザーの名前や自己紹介文、フォローしているユーザー一覧など、ユーザー情報を格納するためのテーブルです。

``_key``
  ユーザーID

``name``
  ユーザー名

``follower``
  フォローしているユーザーの一覧

``favorites``
  お気に入りのコメント一覧

``location``
  ユーザーの現在地（緯度経度座標）

``location_str``
  ユーザーの現在地（文字列）

``description``
  ユーザーの自己紹介

``followee``
  Usersテーブルのfollowerカラムに対するインデックス。
  このインデックスを作ることで、あるユーザーをフォローしているユーザーを検索できるようになります。

Commentsテーブル
^^^^^^^^^^^^^^^^

コメント内容や投稿日時、返信先情報など、コメントに関する内容を格納するテーブルです。

``_key``
  コメントID

``comment``
  コメント内容

``last_modified``
  投稿日時

``replied_to``
  返信元のコメント内容

``replied_users``
  返信先のユーザーの一覧

``hash_tags``
  コメントのハッシュタグの一覧

``location``
  投稿場所(緯度経度座標のため)

``posted_by``
  コメントを書いたユーザー

``favorited_by``
  Usersテーブルのfavoritesカラムに対するインデックス。
  このインデックスを作ることで、指定したコメントを誰がお気に入りに入れているのかを検索できるようになります。

HashTagsテーブル
^^^^^^^^^^^^^^^^

コメントのハッシュタグを一覧で保存するためのテーブルです。

``_key``
  ハッシュタグ

``hash_index``
  「Comments.hash_tags」のインデックス。
  このインデックスを作ることで、指定したハッシュタグのついているコメントの一覧を出すことが出来るようになります。

Bigramテーブル
^^^^^^^^^^^^^^

ユーザー情報・コメントで全文検索が出来るようにするためのインデックスを格納するテーブルです。

``_key``
  単語

``users_index``
  ユーザー情報のインデックス。
  このカラムは、ユーザー名「Users.name」、現在地「Users.location_str」、自己紹介文「Users.description」のインデックスになっています。

``comment_index``
  コメント内容「Comments.comment」のインデックス

GeoIndex table
^^^^^^^^^^^^^^

This is the table which stores indexes of location column to search geo location effectively.

``users_location``
  Indexes of location column for Users table

``comments_location``
  Indexes of location column for Comments table

データのロード
--------------

つづいて、テスト用データをロードします。

::

 load --table Users
 [
   {
     "_key": "daijiro",
     "name": "hsiomaneki",
     "follower": ["tasukuchan"],
     "favorites": [],
     "location": "127678039x502643091",
     "location_str": "神奈川県",
     "description": "groonga developer"
   },
   {
     "_key": "tasukuchan",
     "name": "グニャラくん",
     "follower": ["daijiro","OffGao"],
     "favorites": ["daijiro:1","OffGao:1"],
     "location": "128423343x502929252",
     "location_str": "東京都渋谷区",
     "description": "エロいおっさん"
   },
   {
     "_key": "OffGao",
     "name": "OffGao",
     "follower": ["tasukuchan","daijiro"],
     "favorites": ["tasukuchan:1","daijiro:1"],
     "location": "128544408x502801502",
     "location_str": "東京都中野区",
     "description": "がおがお"
   }
 ]

 load --table Comments
 [
   {
     "_key": "daijiro:1",
     "comment": "マイクロブログ作ってみました（甘栗むいちゃいました的な感じで）。",
     "last_modified": "2010/03/17 12:05:00",
     "posted_by": "daijiro",
   },
   {
     "_key": "tasukuchan:1",
     "comment": "初の書き込み。テストテスト。",
     "last_modified": "2010/03/17 12:00:00",
     "posted_by": "tasukuchan",
   },
   {
     "_key": "daijiro:2",
     "comment": "@tasukuchan ようこそ!!!",
     "last_modified": "2010/03/17 12:05:00",
     "replied_to": "tasukuchan:1",
     "replied_users": ["tasukuchan"],
     "posted_by": "daijiro",
   },
   {
     "_key": "tasukuchan:2",
     "comment": "@daijiro ありがとう！",
     "last_modified": "2010/03/17 13:00:00",
     "replied_to": "daijiro:2",
     "replied_users": ["daijiro"],
     "posted_by": "tasukuchan",
   },
   {
     "_key": "tasukuchan:3",
     "comment": "groongaなう #groonga",
     "last_modified": "2010/03/17 14:00:00",
     "hash_tags": ["groonga"],
     "location": "127972422x503117107",
     "posted_by": "tasukuchan",
   },
   {
     "_key": "tasukuchan:4",
     "comment": "groonga開発合宿のため羽田空港に来ました！ #groonga #travel",
     "last_modified": "2010/03/17 14:05:00",
     "hash_tags": ["groonga", "travel"],
     "location": "127975798x502919856",
     "posted_by": "tasukuchan",
   },
   {
     "_key": "OffGao:1",
     "comment": "@daijiro @tasukuchan 登録してみましたよー！",
     "last_modified": "2010/03/17 15:00:00",
     "replied_users": ["daijiro", "tasukuchan"],
     "location": "128551935x502796433",
     "posted_by": "OffGao",
   }
   {
     "_key": "OffGao:2",
     "comment": "中野ブロードウェイなうなう",
     "last_modified": "2010/03/17 15:05:00",
     "location": "128551935x502796434",
     "posted_by": "OffGao",
   }
 ]

Usersテーブルのfollowerカラムとfavoritesカラム、そしてCommentsテーブルのreplied_usersカラムは、ベクターカラムです。そのため、これらのカラムは配列で値を指定します。

Usersテーブルのlocationカラムと、Commentsテーブルのlocationカラムは、GeoPoint型です。この型での値の指定は、"[緯度]x[経度]"と記述して指定します。

Commentsテーブルのlast_modifiedカラムは、Time型です。この型での値を指定する方法は2つあります。
1つ目の方法は、1970年1月1日0時0分0秒からの経過秒数の値を直接指定する方法です。このとき、小数部分を指定することでマイクロ秒数での指定が可能です。指定した値は、データのロードの際にマイクロ秒を単位とする整数値に変換後、格納されます。
2つ目の方法は、文字列で日時と時刻を指定する方法です。"年/月/日 時:分:秒"というフォーマットで記述することで、データロードの際に文字列からキャストされ、マイクロ秒数の値が格納されます。

.. groonga-command
..
 table_create --name Users --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name Comments --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name HashTags --flags TABLE_HASH_KEY --key_type ShortText
 table_create --name Bigram --flags TABLE_PAT_KEY|KEY_NORMALIZE --key_type ShortText --default_tokenizer TokenBigram
 table_create --name GeoIndex --flags TABLE_PAT_KEY --key_type WGS84GeoPoint

 column_create --table Users --name name --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name follower --flags COLUMN_VECTOR --type Users
 column_create --table Users --name favorites --flags COLUMN_VECTOR --type Comments
 column_create --table Users --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
 column_create --table Users --name location_str --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name description --flags COLUMN_SCALAR --type ShortText
 column_create --table Users --name followee --flags COLUMN_INDEX --type Users --source follower

 column_create --table Comments --name comment --flags COLUMN_SCALAR --type ShortText
 column_create --table Comments --name last_modified --flags COLUMN_SCALAR --type Time
 column_create --table Comments --name replied_to --flags COLUMN_SCALAR --type Comments
 column_create --table Comments --name replied_users --flags COLUMN_VECTOR --type Users
 column_create --table Comments --name hash_tags --flags COLUMN_VECTOR --type HashTags
 column_create --table Comments --name location --flags COLUMN_SCALAR --type WGS84GeoPoint
 column_create --table Comments --name posted_by --flags COLUMN_SCALAR --type Users
 column_create --table Comments --name favorited_by --flags COLUMN_INDEX --type Users --source favorites

 column_create --table HashTags --name hash_index --flags COLUMN_INDEX --type Comments --source hash_tags

 column_create --table Bigram --name users_index --flags COLUMN_INDEX|WITH_POSITION|WITH_SECTION --type Users --source name,location_str,description
 column_create --table Bigram --name comment_index --flags COLUMN_INDEX|WITH_POSITION --type Comments --source comment

 column_create --table GeoIndex --name users_location --type Users --flags COLUMN_INDEX --source location
 column_create --table GeoIndex --name comments_location --type Comments --flags COLUMN_INDEX --source location

 load --table Users
 [
   {
     "_key": "daijiro",
     "name": "hsiomaneki",
     "follower": ["tasukuchan"],
     "favorites": [],
     "location": "127678039x502643091",
     "location_str": "神奈川県",
     "description": "groonga developer"
   },
   {
     "_key": "tasukuchan",
     "name": "グニャラくん",
     "follower": ["daijiro","OffGao"],
     "favorites": ["daijiro:1","OffGao:1"],
     "location": "128423343x502929252",
     "location_str": "東京都渋谷区",
     "description": "エロいおっさん"
   },
   {
     "_key": "OffGao",
     "name": "OffGao",
     "follower": ["tasukuchan","daijiro"],
     "favorites": ["tasukuchan:1","daijiro:1"],
     "location": "128544408x502801502",
     "location_str": "東京都中野区",
     "description": "がおがお"
   }
 ]

 load --table Comments
 [
   {
     "_key": "daijiro:1",
     "comment": "マイクロブログ作ってみました（甘栗むいちゃいました的な感じで）。",
     "last_modified": "2010/03/17 12:05:00",
     "posted_by": "daijiro",
   },
   {
     "_key": "tasukuchan:1",
     "comment": "初の書き込み。テストテスト。",
     "last_modified": "2010/03/17 12:00:00",
     "posted_by": "tasukuchan",
   },
   {
     "_key": "daijiro:2",
     "comment": "@tasukuchan ようこそ!!!",
     "last_modified": "2010/03/17 12:05:00",
     "replied_to": "tasukuchan:1",
     "replied_users": ["tasukuchan"],
     "posted_by": "daijiro",
   },
   {
     "_key": "tasukuchan:2",
     "comment": "@daijiro ありがとう！",
     "last_modified": "2010/03/17 13:00:00",
     "replied_to": "daijiro:2",
     "replied_users": ["daijiro"],
     "posted_by": "tasukuchan",
   },
   {
     "_key": "tasukuchan:3",
     "comment": "groongaなう #groonga",
     "last_modified": "2010/03/17 14:00:00",
     "hash_tags": ["groonga"],
     "location": "127972422x503117107",
     "posted_by": "tasukuchan",
   },
   {
     "_key": "tasukuchan:4",
     "comment": "groonga開発合宿のため羽田空港に来ました！ #groonga #travel",
     "last_modified": "2010/03/17 14:05:00",
     "hash_tags": ["groonga", "travel"],
     "location": "127975798x502919856",
     "posted_by": "tasukuchan",
   },
   {
     "_key": "OffGao:1",
     "comment": "@daijiro @tasukuchan 登録してみましたよー！",
     "last_modified": "2010/03/17 15:00:00",
     "replied_users": ["daijiro", "tasukuchan"],
     "location": "128551935x502796433",
     "posted_by": "OffGao",
   }
   {
     "_key": "OffGao:2",
     "comment": "中野ブロードウェイなうなう",
     "last_modified": "2010/03/17 15:05:00",
     "location": "128551935x502796434",
     "posted_by": "OffGao",
   }
 ]



検索
----

それでは、実際に検索をしてみましょう。

キーワードでユーザー検索
^^^^^^^^^^^^^^^^^^^^^^^^
ここでは、 :doc:`match_columns` で扱った、複数カラムを対象とした検索を行います。
指定された文字列で、ユーザー名・現在地・自己紹介文を対象に検索をします。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-1.log
.. select --table Users --match_columns name,location_str,description --query 東京 --output_columns _key,name

「東京」をキーワードにユーザー検索した結果、東京都に住んでいる「グニャラくん」と「OffGao」がヒットしました。

GeoPointでユーザー検索
^^^^^^^^^^^^^^^^^^^^^^

ここでは、 :doc:`search` で扱った、GeoPoint型のカラムで検索をします。
以下の例では、指定された位置から5000m以内にいるユーザーを検索しています。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-2.log
.. select --table Users --filter 'geo_in_circle(location,"128484216x502919856",5000)' --output_columns _key,name

新宿駅から5km以内にすんでいるユーザーを検索したところ、「グニャラくん」と「OffGao」がヒットしました。

あるユーザーをフォローしているユーザーの検索
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

ここでは、 :doc:`index` で扱った、参照関係の逆引きをします。
以下の例では、Usersテーブルのfollowerカラムにあるフォローリストを逆引きします。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-3.log
.. select --table Users --query follower:@tasukuchan --output_columns _key,name

「グニャラくん」をフォローしている「hsiomaneki」と「OffGao」がヒットしました。

GeoPointでコメント検索
^^^^^^^^^^^^^^^^^^^^^^
ある範囲内で書かれたコメントを検索します。
また、 :doc:`drilldown` で扱ったドリルダウンも行います。検索結果をハッシュタグとユーザーでドリルダウンし、ユーザー別・ハッシュタグ別のカウントを出します。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-4.log
.. select --table Comments --filter 'geo_in_circle(location,"127975798x502919856",20000)' --output_columns posted_by.name,comment --drilldown hash_tags,posted_by

範囲を広く指定したため、位置情報のあるすべてのコメントがヒットしました。そして、ヒットしたコメントからドリルダウンされた結果も返ってきており、ハッシュタグは「#groonga」が2つに「#travel」が1つ、投稿者は「グニャラくん」「OffGao」がそれぞれ2件ずつであることがわかります。

キーワードでコメント検索
^^^^^^^^^^^^^^^^^^^^^^^^
あるキーワードを含むコメントを検索します。
さらに、 :doc:`search` で扱った、スコア値_scoreも出してみましょう。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-5.log
.. select --table Comments --query comment:@なう --output_columns comment,_score

「なう」をキーワードにコメント検索した結果、2件のコメントがヒットしました。また、_scoreの値も返ってきており、「なう」の数が出力されていることが確認できます。

GeoPointとキーワードでコメント検索
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
今度は、キーワードとGeoPointの両方を条件に検索をしてみます。--queryと--filterの両方を使用した場合、両方の条件に一致するレコードがヒットします。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-6.log
.. select --table Comments --query comment:@羽田 --filter 'geo_in_circle(location,"127975798x502919856",20000)' --output_columns posted_by.name,comment --drilldown hash_tags,posted_by

両方の条件を満たすコメントが1件ヒットしました。また、ドリルダウンの結果も返ってきており、「グニャラくん」のコメント1件であることがわかります。

ハッシュタグでコメント検索
^^^^^^^^^^^^^^^^^^^^^^^^^^
あるハッシュタグのついているコメントを検索します。
これも、 :doc:`index` で扱った、参照関係の逆引きを使います。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-7.log
.. select --table Comments --query hash_tags:@groonga --output_columns posted_by.name,comment --drilldown posted_by

#groongaタグの付いている2件のコメントがヒットしました。また、投稿者のドリルダウンも返ってきており、2件とも「グニャラくん」のものであることがわかります。

ユーザーIDでコメント検索
^^^^^^^^^^^^^^^^^^^^^^^^
あるユーザーが投稿したコメントを検索します。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-8.log
.. select --table Comments --query posted_by:tasukuchan --output_columns comment --drilldown hash_tags

「グニャラくん」が書き込んだ4件のコメントがヒットしました。また、ハッシュタグでドリルダウンした結果も返ってきており、ハッシュタグは「#groonga」が2つに「#travel」が1つあることがわかります。

ユーザーのお気に入りコメントを検索
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
あるユーザーがお気に入りに入れているコメントを検索します。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-9.log
.. select --table Users --query _key:tasukuchan --output_columns favorites.posted_by,favorites.comment

「グニャラくん」がお気に入りに入れている2件のコメントがヒットしました。

投稿時間でコメント検索
^^^^^^^^^^^^^^^^^^^^^^
コメントの投稿時間で検索をします。Time型については :doc:`data` で扱っています。
この例では、指定した時間よりも前に投稿されているコメントを検索します。

.. groonga-command
.. include:: ../example/tutorial/micro_blog-10.log
.. select Comments --filter 'last_modified<=1268802000' --output_columns posted_by.name,comment,last_modified --drilldown hash_tags,posted_by

2010/03/17 14:00:00以前に書かれたコメント5件がヒットしました。また、ドリルダウンの結果も返ってきており、「hsiomaneki」が2件、「グニャラくん」が3件ヒットしていることがわかります。

.. TODO: 以下の機能はgroonga本体での支援が必要。
.. タイムライン表示: あるユーザがfollowしているユーザの発言を、時系列順の逆順で並べて10件のみ表示。
.. 複数の条件での検索: favoriteの数がn以上の発言で全文検索、さらにユーザでドリルダウン
