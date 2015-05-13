.. -*- rst -*-

.. highlightlang:: none

Data types
==========

Name
----

Groonga data types

Description
-----------

Groonga identifies data types to store.

A primary key of table and column value belong to some kind of data types in Groonga database. And normaly, column values become in common with all records in one table.

A primary key type and column type can be specified Groonga defined types, user defined types or user defined table.

If you specify other table to primary key type, this table becomes subset of the table of primary key type.

If you specify other table to column type, this column becomes reference key of the table of column type.

組込型
------

以下の型が組込型としてあらかじめ定義されています。

``Object``

  任意のテーブルに属する全てのレコードです。 [#]_

``Bool``

  ブーリアン型やブール型などと呼ばれる型であり、真偽値を表します。取り得る値はtrueとfalseです。（デフォルト値: false）

  :doc:`/reference/commands/load` コマンドで値を格納するときは、false、0、空文字列のいずれかを指定するとfalseになり、それ以外を指定するとtrueになります。

``Int8``

  8bit符号付き整数であり、-128以上127以下の整数を表します。（デフォルト値: 0）

``UInt8``

  8bit符号なし整数であり、0以上255以下の整数を表します。（デフォルト値: 0）

``Int16``

  16bit符号付き整数であり、-32,768以上32,767以下の整数を表します。（デフォルト値: 0）

``UInt16``

  16bit符号なし整数であり、0以上65,535以下の整数を表します。（デフォルト値: 0）

``Int32``

  32bit符号付き整数であり、-2,147,483,648以上2,147,483,647以下の整数を表します。（デフォルト値: 0）

``UInt32``

  32bit符号なし整数であり、0以上4,294,967,295以下の整数を表します。（デフォルト値: 0）

``Int64``

  64bit符号付き整数であり、-9,223,372,036,854,775,808以上9,223,372,036,854,775,807以下の整数を表します。（デフォルト値: 0）

``UInt64``

  64bit符号なし整数であり、0以上18,446,744,073,709,551,615以下の整数を表します。（デフォルト値: 0）

``Float``

  IEEE 754形式の倍精度浮動小数点数であり、実数を表します。（デフォルト値: 0.0）

  IEEE 754形式の詳細については、 `IEEE 754 - Wikipedia <http://ja.wikipedia.org/wiki/IEEE_754>`_ や `IEEE 754: Standard for Binary Floating-Point <http://grouper.ieee.org/groups/754/>`_ を参照してください。

``Time``

  日時を表す型であり、1970年1月1日0時0分0秒からの経過時間を、マイクロ秒単位で64bit符号付き整数により表現します。（デフォルト値: 0）

  :doc:`/reference/commands/load` コマンドで値を格納するときは、1970年1月1日0時0分0秒からの経過秒数を指定します。秒単位より詳細な日時を指定するには、小数を使います。

``ShortText``

  4,095バイト以下の文字列を表します。（デフォルト値: ""）

``Text``

  65,535バイト以下の文字列を表します。（デフォルト値: ""）

``LongText``

  2,147,483,647バイト以下の文字列を表します。（デフォルト値: ""）

``TokyoGeoPoint``

  旧日本測地系による経緯度であり、経度と緯度をミリ秒単位で表現した整数の組により表現します。（デフォルト値: 0x0）

  度分秒形式でx度y分z秒となる経度・緯度は、(((x * 60) + y) * 60 + z) * 1000という計算式でミリ秒単位へと変換されます。
  :doc:`/reference/commands/load` コマンドで値を格納するときは、"ミリ秒単位の経度xミリ秒単位の緯度" もしくは "経度の小数表記x緯度の小数表記" という文字列表現を使って指定します。経度と緯度の区切りとしては、'x' のほかに ',' を使うことができます。

  測地系の詳細については、 `測地系 - Wikipedia <http://ja.wikipedia.org/wiki/%E6%B8%AC%E5%9C%B0%E7%B3%BB>`_ を参照してください。

``WGS84GeoPoint``

  世界測地系（World Geodetic System, WGS 84）による経緯度であり、経度と緯度をミリ秒単位で表現した整数の組により表現します。（デフォルト値: 0x0）

  度分秒形式からミリ秒形式への変換方法や :doc:`/reference/commands/load` コマンドにおける指定方法はTokyoGeoPointと同じです。

型に関する制限事項
------------------

テーブルの主キーに指定できない型
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Text型とLongText型については、テーブルの主キーに指定することはできません。

ベクターとして格納できない型
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Groongaのカラムは、ある型のベクターを保存することができます。しかし、ShortText, Text, LongTextの３つの型についてはベクターとして保存したり出力したりすることはできますが、検索条件やドリルダウン条件に指定することができません。

テーブル型は、ベクターとして格納することができます。よって、ShortTextのベクターを検索条件やドリルダウン条件に使用したい場合には、主キーがShortText型のテーブルを別途作成し、そのテーブルを型として利用します。

.. rubric:: 脚注

.. [#] Object型はv1.2でサポートされます。
