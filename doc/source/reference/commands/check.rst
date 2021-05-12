.. -*- rst -*-

``check``
=========

Summary
-------

check - オブジェクトの状態表示

Groonga組込コマンドの一つであるcheckについて説明します。組込コマンドは、groonga実行ファイルの引数、標準入力、またはソケット経由でgroongaサーバにリクエストを送信することによって実行します。

checkコマンドは、groongaプロセス内の指定したオブジェクトの状態を表示します。主にデータベースが壊れた場合など異常時の問題解決のために使用することを想定しています。デバッグ用のため、返値のフォーマットが安定しているということは保証されません。（フォーマットが変更される可能性が高い）

Syntax
------
::

 check obj

Usage
-----

テーブルTermsのインデックスカラムnameの状態を表示します。::

 check Terms.name
 [{"flags":"00008202",
   "max sid":1,
   "number of garbage segments":0,
   "number of array segments":1,
   "max id of array segment":1,
   "number of buffer segments":110,
   "max id of buffer segment":111,
   "max id of physical segment in use":111,
   "number of unmanaged segments":4294967185,
   "total chunk size":7470239,
   "max id of chunk segments in use":127,
   "number of garbage chunk":[0,0,0,0,0,0,0,0,2,2,0,0,0,0,0]},
  {"buffer id":0,
   "chunk size":94392,
   "buffer term":["596","59777","6",...],
   "buffer free":152944,
   "size in buffer":7361,
   "nterms":237,
   "nterms with chunk":216,
   "buffer id":1,
   "chunk size":71236,
   "buffer term":[["に述",18149,18149,2,25,6,6],
                  ["に追",4505,4505,76,485,136,174],
                  ["に退",26568,26568,2,9,2,2],
                  ...],
   "buffer free":120000,
   "size in buffer":11155,
   "nterms":121,
   "nterms with chunk":116},
   {"buffer id":1,
    ...},
   ...]

Parameters
----------

``obj``

  状態を表示するオブジェクトの名前を指定します。

Return value
------------

チェックするオブジェクトにより返される値が変わります。

インデックスカラムの場合::

  下記のような配列が出力されます。

    [インデックスの状態, バッファの状態1, バッファの状態2, ...]

``インデックスの状態`` には下記の項目がハッシュ形式で出力されます。

  ``flags``

    指定されているフラグ値です。16進数で表現されています。

  ``max sid``

    セグメントのうち最も大きなIDです。

  ``number of garbage segments``

    ゴミセグメントの数です。

  ``number of array segments``

    配列セグメントの数です。

  ``max id of array segment``

    配列セグメントのうち最も大きなIDです。

  ``number of buffer segments``

    バッファセグメントの数です。

  ``max id of buffer segment``

    バッファセグメントのうち最も大きなIDです。

  ``max id of physical segment in use``

    使用中の論理セグメントのうち最も大きなIDです。

  ``number of unmanaged segments``

    管理されていないセグメントの数です。

  ``total chunk size``

    チャンクサイズの合計です。

  ``max id of chunk segments in use``

    使用中のチャンクセグメントのうち最も大きなIDです。

  ``number of garbage chunk``

    各チャンク毎のゴミの数です。

``バッファの状態`` には下記の項目がハッシュ形式で出力されます。

  ``buffer id``

    バッファIDです。

  ``chunk size``

    チャンクのサイズです。

  ``buffer term``

    バッファ内にある語の一覧です。各語の状態は以下のような配列となっています。

      [語, バッファに登録されている語のID, 用語集に登録されている語のID, バッファ内でのサイズ, チャンク内でのサイズ]

  ``buffer free``

    バッファの空き容量です。

  ``size in buffer``

    バッファの使用量です。

  ``nterms``

    バッファ内にある語の数です。

  ``nterms with chunk``

    バッファ内にある語のうち、チャンクを使っている語の数です。

