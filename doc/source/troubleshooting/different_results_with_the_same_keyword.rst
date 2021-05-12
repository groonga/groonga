.. -*- rst -*-

同じ検索キーワードなのに全文検索結果が異なる
============================================

同じ検索キーワードでも一緒に指定するクエリによっては全文検索の結果が異なることがあります。ここでは、その原因と対策方法を説明します。

例
--

まず、実際に検索結果が異なる例を説明します。

DDLは以下の通りです。BlogsテーブルのbodyカラムをTokenMecabトークナイザーを使ってトークナイズしてからインデックスを作成しています。::

  table_create Blogs TABLE_NO_KEY
  column_create Blogs body COLUMN_SCALAR ShortText
  column_create Blogs updated_at COLUMN_SCALAR Time
  table_create Terms TABLE_PAT_KEY ShortText --default_tokenizer TokenMecab  --normalizer NormalizerAuto
  column_create Terms blog_body COLUMN_INDEX|WITH_POSITION Blogs body

テスト用のデータは1件だけ投入します。::

  load --table Blogs
  [
    ["body", "updated_at"],
    ["東京都民に深刻なダメージを与えました。", "2010/9/21 10:18:34"],
  ]

まず、全文検索のみで検索します。この場合ヒットします。::

  > select Blogs --filter 'body @ "東京都"'
  [[0,4102.268052438,0.000743783],[[[1],[["_id","UInt32"],["updated_at","Time"],["body","ShortText"]],[1,1285031914.0,"東京都民に深刻なダメージを与えました。"]]]]

続いて、範囲指定と全文検索を組み合わせて検索します（1285858800は2010/10/1 0:0:0の秒表記）。この場合もヒットします。::

  > select Blogs --filter 'body @ "東京都" && updated_at < 1285858800'
  [[0,4387.524084839,0.001525487],[[[1],[["_id","UInt32"],["updated_at","Time"],["body","ShortText"]],[1,1285031914.0,"東京都民に深刻なダメージを与えました。"]]]]

最後に、範囲指定と全文検索の順番を入れ替えて検索します。個々の条件は同じですが、この場合はヒットしません。::

  > select Blogs --filter 'updated_at < 1285858800 && body @ "東京都"'
  [[0,4400.292570838,0.000647716],[[[0],[["_id","UInt32"],["updated_at","Time"],["body","ShortText"]]]]]

どうしてこのような挙動になるかを説明します。

原因
----

このような挙動になるのは全文検索時に複数の検索の挙動を使い分けているからです。ここでは簡単に説明するので、詳細は :doc:`/spec/search` を参照してください。

検索の挙動には以下の3種類があります。

1. 完全一致検索
2. 非わかち書き検索
3. 部分一致検索

Groongaは基本的に完全一致検索のみを行います。上記の例では「東京都民に深刻なダメージを与えました。」を「東京都」というクエリで検索していますが、TokenMecabトークナイザーを使っている場合はこのクエリはマッチしません。

検索対象の「東京都民に深刻なダメージを与えました。」は

  東京 / 都民 / に / 深刻 / な / ダメージ / を / 与え / まし / た / 。

とトークナイズされますが、クエリの「東京都」は

  東京 / 都

とトークナイズされるため、完全一致しません。

Groongaは完全一致検索した結果のヒット件数が所定の閾値を超えない場合に限り、非わかち書き検索を行い、それでもヒット件数が閾値を超えない場合は部分一致検索を行います（閾値は1がデフォルト値となっています）。このケースのデータは部分一致検索ではヒットするので、「東京都」クエリのみを指定するとヒットします。

しかし、以下のように全文検索前にすでに閾値が越えている場合（「updated_at < 1285858800」で1件ヒットし、閾値を越える）は、たとえ完全一致検索で1件もヒットしない場合でも部分一致検索などを行いません。::

  select Blogs --filter 'updated_at < 1285858800 && body @ "東京都"'

そのため、条件の順序を変えると検索結果が変わるという状況が発生します。以下で、この情報を回避する方法を2種類紹介しますが、それぞれトレードオフとなる条件があるので採用するかどうかを十分検討してください。

対策方法1: トークナイザーを変更する
-----------------------------------

TokenMecabトークナイザーは事前に準備した辞書を用いてトークナイズするため、再現率よりも適合率を重視したトークナイザーと言えます。一方、TokenBigramなど、N-gram系のトークナイザーは適合率を重視したトークナイザーと言えます。例えば、TokenMecabの場合「東京都」で「京都」に完全一致することはありませんが、TokenBigramでは完全一致します。一方、TokenMecabでは「東京都民」に完全一致しませんが、TokenBigramでは完全一致します。

このようにN-gram系のトークナイザーを指定することにより再現率をあげることができますが、適合率が下がり検索ノイズが含まれる可能性が高くなります。この度合いを調整するためには :doc:`/reference/commands/select` のmatch_columnsで使用する索引毎に重み付けを指定します。

ここでも、前述の例を使って具体例を示します。まず、TokenBigramを用いた索引を追加します。::

  table_create Bigram TABLE_PAT_KEY ShortText --default_tokenizer TokenBigram  --normalizer NormalizerAuto
  column_create Bigram blog_body COLUMN_INDEX|WITH_POSITION Blogs body

この状態でも以前はマッチしなかったレコードがヒットするようになります。::

  > select Blogs --filter 'updated_at < 1285858800 && body @ "東京都"'
  [[0,7163.448064902,0.000418127],[[[1],[["_id","UInt32"],["updated_at","Time"],["body","ShortText"]],[1,1285031914.0,"東京都民に深刻なダメージを与えました。"]]]]

しかし、N-gram系のトークナイザーの方がTokenMecabトークナイザーよりも語のヒット数が多いため、N-gram系のヒットスコアの方が重く扱われてしまいます。N-gram系のトークナイザーの方がTokenMecabトークナイザーよりも適合率の低い場合が多いので、このままでは検索ノイズが上位に表示される可能性が高くなります。

そこで、TokenMecabトークナイザーを使って作った索引の方をTokenBigramトークナイザーを使って作った索引よりも重視するように重み付けを指定します。これは、match_columnsオプションで指定できます。::

  > select Blogs --match_columns 'Terms.blog_body * 10 || Bigram.blog_body * 3' --query '東京都' --output_columns '_score, body'
  [[0,8167.364602632,0.000647003],[[[1],[["_score","Int32"],["body","ShortText"]],[13,"東京都民に深刻なダメージを与えました。"]]]]

この場合はスコアが11になっています。内訳は、Terms.blog_body索引（TokenMecabトークナイザーを使用）でマッチしたので10、Bigram.blog_body索引（TokenBigramトークナイザーを使用）でマッチしたので3、これらを合計して13になっています。このようにTokenMecabトークナイザーの重みを高くすることにより、検索ノイズが上位にくることを抑えつつ再現率を上げることができます。

この例は日本語だったのでTokenBigramトークナイザーでよかったのですが、アルファベットの場合はTokenBigramSplitSymbolAlphaトークナイザーなども利用する必要があります。例えば、「楽しいbilliard」はTokenBigramトークナイザーでは

  楽し / しい / billiard

となり、「bill」では完全一致しません。一方、TokenBigramSplitSymbolAlphaトークナイザーを使うと

  楽し / しい / いb / bi / il / ll / li / ia / ar / rd / d

となり、「bill」でも完全一致します。

TokenBigramSplitSymbolAlphaトークナイザーを使う場合も重み付けを考慮する必要があることはかわりありません。

利用できるバイグラム系のトークナイザーの一覧は以下の通りです。

* TokenBigram: バイグラムでトークナイズする。連続する記号・アルファベット・数字は一語として扱う。
* TokenBigramSplitSymbol: 記号もバイグラムでトークナイズする。連続するアルファベット・数字は一語として扱う。
* TokenBigramSplitSymbolAlpha: 記号とアルファベットもバイグラムでトークナイズする。連続する数字は一語として扱う。
* TokenBigramSplitSymbolAlphaDigit: 記号・アルファベット・数字もバイグラムでトークナイズする。
* TokenBigramIgnoreBlank: バイグラムでトークナイズする。連続する記号・アルファベット・数字は一語として扱う。空白は無視する。
* TokenBigramIgnoreBlankSplitSymbol: 記号もバイグラムでトークナイズする。連続するアルファベット・数字は一語として扱う。空白は無視する。
* TokenBigramIgnoreBlankSplitSymbolAlpha: 記号とアルファベットもバイグラムでトークナイズする。連続する数字は一語として扱う。空白は無視する。
* TokenBigramIgnoreBlankSplitSymbolAlphaDigit: 記号・アルファベット・数字もバイグラムでトークナイズする。空白は無視する。

対策方法2: 閾値をあげる
-----------------------

非わかち書き検索・部分一致検索を利用するかどうかの閾値は--with-match-escalation-threshold configureオプションで変更することができます。以下のように指定すると、100件以下のヒット数であれば、たとえ完全一致検索でヒットしても、非わかち書き検索・部分一致検索を行います。::

  % ./configure --with-match-escalation-threshold=100

この場合も対策方法1同様、検索ノイズが上位に現れる可能性が高くなることに注意してください。検索ノイズが多くなった場合は指定する値を低くする必要があります。
